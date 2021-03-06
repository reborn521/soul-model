/******************************************************************
 Structure OUtput Layer (SOUL) Language Model Toolkit
 (C) Copyright 2009 - 2012 Hai-Son LE LIMSI-CNRS

 Class for n-gram neural network word factored translation model
 *******************************************************************/
#include "mainModel.H"

NgramWordTranslationModel::NgramWordTranslationModel() {
}

NgramWordTranslationModel::~NgramWordTranslationModel() {
	delete baseNetwork;
	for (int idel = 0; idel < outputNetworkNumber; idel++) {
		delete outputNetwork[idel];
	}
	delete[] outputNetwork;
	delete inputVoc;
	delete outputVoc;
	delete dataSet;
}

void NgramWordTranslationModel::allocation() {
	otl = new outils();
	otl->sgenrand(time(NULL));
	hiddenStep = 1;
	if (nonLinearType == TANH) {
		hiddenStep = 2;
	} else if (nonLinearType == SIGM) {
		hiddenStep = 2;
	}

	baseNetwork = new Sequential(hiddenLayerSizeArray.length * hiddenStep);

	int i;
	baseNetwork->lkt = new LookupTable(inputVoc->wordNumber, dimensionSize,
			nm - 1, blockSize, 1, otl);

	Module* module;
	if (name == WTOVN || name == WTOVN_NCE) {
		module = new Linear((nm - 1) * dimensionSize, hiddenLayerSizeArray(0),
				blockSize, otl);
	}
	baseNetwork->add(module);

	if (nonLinearType == TANH) {
		module = new Tanh(hiddenLayerSizeArray(0), blockSize); // non linear
		baseNetwork->add(module);
	} else if (nonLinearType == SIGM) {
		module = new Sigmoid(hiddenLayerSizeArray(0), blockSize); // non linear
		baseNetwork->add(module);
	}
	for (i = 1; i < hiddenLayerSizeArray.size[0]; i++) {
		module = new Linear(hiddenLayerSizeArray(i - 1),
				hiddenLayerSizeArray(i), blockSize, otl);
		baseNetwork->add(module);
		if (nonLinearType == TANH) {
			module = new Tanh(hiddenLayerSizeArray(i), blockSize); // non linear
			baseNetwork->add(module);
		} else if (nonLinearType == SIGM) {
			module = new Sigmoid(hiddenLayerSizeArray(i), blockSize); // non linear
			baseNetwork->add(module);
		}
	}
	probabilityOne.resize(blockSize, 1);
	int outputNetworkNumber = outputNetworkSize.size[0];
	outputNetwork = new ProbOutput*[outputNetworkNumber];
	ProbOutput* sl;
	if (name == WTOVN) {
		sl = new LinearSoftmax(hiddenLayerSize, outputNetworkSize(0), blockSize,
				otl);
	} else if (name == WTOVN_NCE) {
		sl = new LinearSoftmax_NCE(hiddenLayerSize, outputNetworkSize(0),
				blockSize, otl);
	} else {
		cout
				<< "NgramWordTranslationModel::allocation what is your model type: WTOVN , WTOVN_NCE"
				<< endl;
	}
	outputNetwork[0] = sl;
	for (i = 1; i < outputNetworkNumber; i++) {
		if (name == WTOVN) {
			sl = new LinearSoftmax(hiddenLayerSize, outputNetworkSize(i), 1,
					otl);
		} else if (name == WTOVN_NCE) {
			sl = new LinearSoftmax_NCE(hiddenLayerSize, outputNetworkSize(i), 1,
					otl);
		} else {
			cout
					<< "NgramWordTranslationModel::allocation what is your model type: WTOVN , WTOVN_NCE"
					<< endl;
		}
		outputNetwork[i] = sl;
	}
	doneForward.resize(outputNetworkNumber, 1);
	contextFeature = baseNetwork->output;
	gradContextFeature.resize(contextFeature);
	localCodeWord.resize(blockSize, maxCodeWordLength);
	if (name == WTOVN) {
		dataSet = new NgramWordTranslationDataSet(ngramType, n, BOS, inputVoc,
				outputVoc, mapIUnk, mapOUnk, BLOCK_NGRAM_NUMBER);
	} else if (name == WTOVN_NCE) {
		dataSet = new NgramNCEWordTranslationDataSet(ngramType, n, BOS,
				inputVoc, outputVoc, mapIUnk, mapOUnk, BLOCK_NGRAM_NUMBER);
	} else {
		cout
				<< "NgramWordTranslationModel::allocation what is your model type: WTOVN , WTOVN_NCE"
				<< endl;
	}

	delete otl;
}
NgramWordTranslationModel::NgramWordTranslationModel(string name, int ngramType,
		char* inputVocFileString, char* outputVocFileString, int mapIUnk,
		int mapOUnk, int BOS, int blockSize, int n, int dimensionSize,
		string nonLinearType, intTensor& hiddenLayerSizeArray,
		char* codeWordFileString, char* outputNetworkSizeFileString) {
	recurrent = 0;
	this->name = name;
	this->ngramType = ngramType;
	this->inputVoc = new SoulVocab(inputVocFileString);
	this->outputVoc = new SoulVocab(outputVocFileString);
	this->mapIUnk = mapIUnk;
	this->mapOUnk = mapOUnk;
	this->BOS = BOS;
	this->blockSize = blockSize;
	this->n = n;
	nm = n * 2;
	if (BOS > n) {
		this->BOS = n;
	}

	this->dimensionSize = dimensionSize;
	this->nonLinearType = nonLinearType;
	ioFile readIof;
	if (!strcmp(codeWordFileString, "xxx")) {
		codeWord.resize(outputVoc->wordNumber, 2);
		codeWord = 0;
		for (int wordIndex = 0; wordIndex < outputVoc->wordNumber;
				wordIndex++) {
			codeWord(wordIndex, 1) = wordIndex;
		}
	} else {
		readIof.takeReadFile(codeWordFileString);
		codeWord.read(&readIof);
	}
	if (!strcmp(outputNetworkSizeFileString, "xxx")) {
		outputNetworkSize.resize(1, 1);
		outputNetworkSize(0) = outputVoc->wordNumber;
	} else {
		readIof.takeReadFile(outputNetworkSizeFileString);
		outputNetworkSize.read(&readIof);
	}
	this->hiddenLayerSizeArray.resize(hiddenLayerSizeArray);
	this->hiddenLayerSizeArray.copy(hiddenLayerSizeArray);
	hiddenLayerSize = hiddenLayerSizeArray(hiddenLayerSizeArray.length - 1);
	hiddenNumber = hiddenLayerSizeArray.length;
	maxCodeWordLength = this->codeWord.size[1];
	outputNetworkNumber = outputNetworkSize.size[0];
	allocation();
}

NgramWordTranslationModel::NgramWordTranslationModel(string name, int ngramType,
		int inputSize, int outputSize, int blockSize, int n, int dimensionSize,
		string nonLinearType, intTensor& hiddenLayerSizeArray,
		intTensor& codeWord, intTensor& outputNetworkSize) {
	recurrent = 0;
	this->name = name;
	this->ngramType = ngramType;
	inputVoc = new SoulVocab();
	outputVoc = new SoulVocab();
	for (int i = 0; i < inputSize; i++) {
		stringstream out;
		out << i;
		inputVoc->add(out.str(), i);
	}
	for (int i = 0; i < outputSize; i++) {
		stringstream out;
		out << i;
		outputVoc->add(out.str(), i);
	}
	this->mapIUnk = 1;
	this->mapOUnk = 1;
	this->BOS = n;
	this->blockSize = blockSize;
	this->n = n;
	nm = n * 2;
	this->dimensionSize = dimensionSize;
	this->nonLinearType = nonLinearType;
	this->hiddenLayerSizeArray = hiddenLayerSizeArray;
	this->codeWord = codeWord;
	this->outputNetworkSize = outputNetworkSize;
	hiddenLayerSize = hiddenLayerSizeArray(hiddenLayerSizeArray.length - 1);
	hiddenNumber = hiddenLayerSizeArray.length;
	maxCodeWordLength = this->codeWord.size[1];
	outputNetworkNumber = outputNetworkSize.size[0];
	allocation();
}

void NgramWordTranslationModel::firstTime() {

}
void NgramWordTranslationModel::firstTime(intTensor& context) {

}

int NgramWordTranslationModel::train(char* dataFileString, int maxExampleNumber,
		int iteration, string learningRateType, float learningRate,
		float learningRateDecay) {
	firstTime();
	ioFile dataIof;
	dataIof.takeReadFile(dataFileString);
	int ngramNumber;
	dataIof.readInt(ngramNumber);
	int N;
	dataIof.readInt(N);
	if (N < nm) {
		cerr << "ERROR: N in data is wrong:" << N << " < " << nm << endl;
		exit(1);
	}

	if (maxExampleNumber > ngramNumber || maxExampleNumber == 0) {
		maxExampleNumber = ngramNumber;
	}
	float currentLearningRate;
	int nstep;
	nstep = maxExampleNumber * (iteration - 1);
	intTensor readTensor(blockSize, N);
	intTensor context;
	intTensor word;
	floatTensor coefTensor(blockSize, 1);
	context.sub(readTensor, 0, blockSize - 1, N - nm, N - 2);
	context.t();
	word.select(readTensor, 1, N - 1);
	int currentExampleNumber = 0;
	int percent = 1;
	float aPercent = maxExampleNumber * CONSTPRINT;
	float iPercent = aPercent * percent;
	int blockNumber = maxExampleNumber / blockSize;
	int remainingNumber = maxExampleNumber - blockSize * blockNumber;
	int i;
	cout << maxExampleNumber << " examples" << endl;
	for (i = 0; i < blockNumber; i++) {
		//Read one line and then train
		// for test
		//cout << "NgramWordTranslationModel::train here" << endl;
		this->readStripInt(dataIof, readTensor, coefTensor); // read file n gram for word and context
		// for test
		//cout << "NgramWordTranslationModel::train here 1" << endl;
		if (dataIof.getEOF()) {
			break;
		}
		currentExampleNumber += blockSize;
		currentLearningRate = this->takeCurrentLearningRate(learningRate,
				learningRateType, nstep, learningRateDecay);
		// for test
		//cout << "NgramWordTranslationModel::train here 2" << endl;
		trainOne(context, word, coefTensor, currentLearningRate, blockSize);
		// for test
		//cout << "NgramWordTranslationModel::train here 3" << endl;
		nstep += blockSize;
#if PRINT_DEBUG
		if (currentExampleNumber > iPercent) {
			percent++;
			iPercent = aPercent * percent;
			cout << (float) currentExampleNumber / maxExampleNumber << " ... "
					<< flush;
		}
#endif
	}
	if (remainingNumber != 0 && !dataIof.getEOF()) {
		context = 0;
		word = SIGN_NOT_WORD;
		// for test
		//cout << "NgramWordTranslationModel::train here 4" << endl;
		intTensor lastReadTensor(remainingNumber, N);
		// for test
		//cout << "NgramWordTranslationModel::train remainingNumber: " << remainingNumber << endl;
		this->readStripInt(dataIof, lastReadTensor, coefTensor);
		// for test
		//cout << "NgramWordTranslationModel::train here 5" << endl;
		intTensor subReadTensor;
		subReadTensor.sub(readTensor, 0, remainingNumber - 1, 0, N - 1);
		subReadTensor.copy(lastReadTensor);
		//if (!dataIof.getEOF()) {
		currentLearningRate = this->takeCurrentLearningRate(learningRate,
				learningRateType, nstep, learningRateDecay);
		// for test
		//cout << "NgramWordTranslationModel::train here 6" << endl;
		trainOne(context, word, coefTensor, currentLearningRate,
				remainingNumber);
		// for test
		//cout << "NgramWordTranslationModel::train here 7" << endl;
		//}
	}
#if PRINT_DEBUG
	cout << endl;
#endif
	return 1;
}

int NgramWordTranslationModel::forwardProbability(intTensor& ngramTensor,
		floatTensor& probTensor) {
	int bkBlockSize = blockSize;
	if (cont && recurrent) {
		changeBlockSize(1);
	}
	firstTime();
	int localWord;
	int idParent;
	int i;
	float localProb;
	int idWord;
	int ngramNumber = ngramTensor.size[0];
	intTensor oneLocalCodeWord;
	intTensor bContext(nm - 1, blockSize);
	intTensor selectContext;
	intTensor selectBContext;
	intTensor context;
	context.sub(ngramTensor, 0, ngramNumber - 1, 0, nm - 2);
	intTensor contextFlag;
	contextFlag.select(ngramTensor, 1, nm + 2);
	intTensor word;
	word.select(ngramTensor, 1, nm - 1);
	intTensor order;
	order.select(ngramTensor, 1, nm + 1);
	int ngramId = 0;
	int ngramId2 = 0;
	int rBlockSize;
	int nextId;
	int percent = 1;
	float aPercent = ngramNumber * CONSTPRINT;
	float iPercent = aPercent * percent;
	bContext = 0;
	do {
		ngramId2 = ngramId;
		rBlockSize = 0;

		while (rBlockSize < blockSize && ngramId < ngramNumber) {
			selectBContext.select(bContext, 1, rBlockSize);
			selectContext.select(context, 0, ngramId);
			selectBContext.copy(selectContext);
			ngramId = contextFlag(ngramId);
			rBlockSize++;
		}
		rBlockSize = 0;
		firstTime(bContext);
		baseNetwork->forward(bContext);
		mainProb = outputNetwork[0]->forward(contextFeature);
		while (rBlockSize < blockSize && ngramId2 < ngramNumber) {
			doneForward = 0;
			nextId = contextFlag(ngramId2);
			for (; ngramId2 < nextId; ngramId2++) {
				if (order(ngramId2) != SIGN_NOT_WORD) {
					intTensor oneLocalCodeWord;
					idWord = word(ngramId2);
					oneLocalCodeWord.select(codeWord, 0, idWord);
					localWord = oneLocalCodeWord(1);
					localProb = mainProb(localWord, rBlockSize);
					for (i = 2; i < maxCodeWordLength; i += 2) {
						if (oneLocalCodeWord(i) == SIGN_NOT_WORD) {
							break;
						}
						localWord = oneLocalCodeWord(i + 1);
						idParent = oneLocalCodeWord(i);
						if (!doneForward(idParent)) {
							selectContextFeature.select(contextFeature, 1,
									rBlockSize);
							outputNetwork[idParent]->forward(
									selectContextFeature);
							doneForward(idParent) = 1;
						}
						localProb *= outputNetwork[idParent]->output(localWord);
					}
					probTensor(order(ngramId2)) = localProb;
				}
			}
			rBlockSize++;
		}
#if PRINT_DEBUG
		if (ngramId > iPercent) {
			percent++;
			iPercent = aPercent * percent;
			cout << (float) ngramId / ngramNumber << " ... " << flush;
		}
#endif
	} while (ngramId < ngramNumber);
#if PRINT_DEBUG
	cout << endl;
#endif
	if (cont && recurrent) {
		changeBlockSize(bkBlockSize);
	}
	return 1;
}

void NgramWordTranslationModel::read(ioFile* iof, int allocation,
		int blockSize) {
	// for test
	//cout << "NgramWordTranslationModel::read here" << endl;
	string readFormat;
	iof->readString(name);
	iof->readString(readFormat);
	iof->readInt(ngramType);
	inputVoc = new SoulVocab();
	outputVoc = new SoulVocab();
	iof->readInt(inputVoc->wordNumber);
	iof->readInt(outputVoc->wordNumber);
	iof->readInt(mapIUnk);
	iof->readInt(mapOUnk);
	iof->readInt(BOS);

	if (blockSize != 0) {
		this->blockSize = blockSize;
	} else {
		this->blockSize = DEFAULT_BLOCK_SIZE;
	}
	iof->readInt(n);
	nm = n * 2;
	iof->readInt(dimensionSize);
	iof->readInt(hiddenNumber);
	iof->readString(nonLinearType);
	iof->readInt(maxCodeWordLength);
	iof->readInt(outputNetworkNumber);

	codeWord.resize(outputVoc->wordNumber, maxCodeWordLength);
	outputNetworkSize.resize(outputNetworkNumber, 1);
	codeWord.read(iof);
	outputNetworkSize.read(iof);
	hiddenLayerSizeArray.resize(hiddenNumber, 1);
	hiddenLayerSizeArray.read(iof);

	hiddenLayerSize = hiddenLayerSizeArray(hiddenLayerSizeArray.length - 1);
	if (allocation) {
		this->allocation();
	}

	baseNetwork->read(iof);

	int i;
	for (i = 0; i < outputNetworkSize.size[0]; i++) {
		outputNetwork[i]->read(iof);
	}
	inputVoc->read(iof);
	outputVoc->read(iof);
}

void NgramWordTranslationModel::write(ioFile* iof, int closeFile) {
	iof->writeString(name);

	iof->writeString(iof->format);

	iof->writeInt(ngramType);
	iof->writeInt(inputVoc->wordNumber);
	iof->writeInt(outputVoc->wordNumber);
	iof->writeInt(mapIUnk);
	iof->writeInt(mapOUnk);
	iof->writeInt(BOS);
	iof->writeInt(n);
	iof->writeInt(dimensionSize);
	iof->writeInt(hiddenNumber);
	iof->writeString(nonLinearType);
	iof->writeInt(maxCodeWordLength);
	iof->writeInt(outputNetworkNumber);
	codeWord.write(iof);
	outputNetworkSize.write(iof);
	hiddenLayerSizeArray.write(iof);
	baseNetwork->write(iof);
	int i;
	for (i = 0; i < outputNetworkSize.size[0]; i++) {
		outputNetwork[i]->write(iof);
	}
	inputVoc->write(iof);
	outputVoc->write(iof);
	if (closeFile == 1) {
		iof->freeWriteFile();
	}
}

float NgramWordTranslationModel::distance2(NeuralModel& anotherModel) {
	float distSquared = 0;
	distSquared += baseNetwork->lkt->distance2(
			*(anotherModel.baseNetwork->lkt));
	// for test
	cout << "NgramWordTranslationModel::distance2 distSquared after lkt: "
			<< distSquared << endl;
	for (int i = 0; i < hiddenLayerSizeArray.size[0]; i++) {
		distSquared += baseNetwork->modules[i]->distance2(
				*anotherModel.baseNetwork->modules[i]);
	}
	// for test
	cout << "NgramWordTranslationModel::distance2 distSquared after linear: "
			<< distSquared << endl;
	for (int i = 0; i < outputNetworkNumber; i++) {
		distSquared += outputNetwork[i]->distance2(
				*anotherModel.outputNetwork[i]);
	}
	// for test
	cout << "NgramWordTranslationModel::distance2 distSquared final: "
			<< distSquared << endl;
	return distSquared;
}
