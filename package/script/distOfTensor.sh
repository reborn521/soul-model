#!/bin/bash

if [ $# -ne 4 ]; then
    echo "directory start end destination"
    exit
fi

directory=$1
start=$2
end=$3
destination=$4
key1="0OutputWeight0"
key2="OutputWeight0"

for (( i = $start; i<=$end; i++))
do
    echo "distAngle.exe $directory$key1 $directory$i$key2 >> $destination"
    distAngle.exe $directory$key1 $directory$i$key2 >> $destination
done