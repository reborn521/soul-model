#include "mainTensor.H"

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

outils::outils()
{
}

void
outils::sgenrand(unsigned long seed)

{

  int i;

  for (i = 0; i < N; i++)
    {
      mt[i] = seed & 0xffff0000;
      seed = 69069 * seed + 1;
      mt[i] |= (seed & 0xffff0000) >> 16;
      seed = 69069 * seed + 1;
    }
  mti = N;
}

/* Initialization by "sgenrand()" is an example. Theoretically,      */
/* there are 2^19937-1 possible states as an intial state.           */
/* This function allows to choose any of 2^19937-1 ones.             */
/* Essential bits in "seed_array[]" is following 19937 bits:         */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]. */
/* (seed_array[0]&LOWER_MASK) is discarded.                          */
/* Theoretically,                                                    */
/*  (seed_array[0]&UPPER_MASK), seed_array[1], ..., seed_array[N-1]  */
/* can take any values except all zeros.                             */
void
outils::lsgenrand(unsigned long *seed_array)

/* the length of seed_array[] must be at least N */
{
  int i;

  for (i = 0; i < N; i++)
    mt[i] = seed_array[i];
  mti = N;
}

double
outils::genrand() /* generating reals */
/* unsigned long *//* for integer generation */
{
  unsigned long y;
  static unsigned long mag01[2] =
    { 0x0, MATRIX_A };
  /* mag01[x] = x * MATRIX_A  for x=0,1 */

  if (mti >= N)
    { /* generate N words at one time */
      int kk;

      if (mti == N + 1) /* if sgenrand() has not been called, */
        sgenrand(4357); /* a default initial seed is used   */

      for (kk = 0; kk < N - M; kk++)
        {
          y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
          mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
      for (; kk < N - 1; kk++)
        {
          y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
          mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
      y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
      mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

      mti = 0;
    }

  y = mt[mti++];
  y ^= TEMPERING_SHIFT_U(y);
  y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
  y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
  y ^= TEMPERING_SHIFT_L(y);

  return ((double) y * 2.3283064370807974e-10); /* reals */
  /* return y; *//* for integer generation */
}
#undef N
#undef M
#undef MATRIX_A
#undef UPPER_MASK
#undef LOWER_MASK

/* Tempering parameters */
#undef TEMPERING_MASK_B
#undef TEMPERING_MASK_
#undef TEMPERING_SHIFT_U
#undef TEMPERING_SHIFT_S
#undef TEMPERING_SHIFT_T
#undef TEMPERING_SHIFT_L
