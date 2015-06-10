// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_mersenne_twister.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Jun 04 13:28:00 MDT 2004
// License      : BSD.
// Description  : C++ adaptation of the Mersenne Twister uniform random
//                number generator developed by Takuji Nishimura and
//                Makoto Matsumoto, derived from "A C-program for MT19937,
//                with initialization improved 2002/1/26".
//
// NOTE: This implementation is probably slower than the original, and there
//       are other C++ adaptations of this algorithm available that are faster
//       and more feature-rich (Richard J. Wagner MersenneTwister.h).
//       This version preserves the interface of the original C implementation,
//       and has been altered slightly to conform to my coding standards.
//
//       To use in a multithreaded application, each thread needs to create
//       it's own instance of the_mersenne_twister_t class, and has to
//       initialize it in a thread safe manner - lock, seed, unlock.

// the following is an excerpt from mt19937ar.c:
/*
   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#ifndef THE_MERSENNE_TWISTER_HXX_
#define THE_MERSENNE_TWISTER_HXX_


//----------------------------------------------------------------
// the_mersenne_twister_t
//
// Before using, initialize the state by using init_genrand(seed)
// or init_by_array(init_key, key_length).
//
class the_mersenne_twister_t
{
  // period parameters:
  enum
  {
    N = 624,
    M = 397,
    MATRIX_A = 0x9908b0dfUL,   // constant vector a
    UPPER_MASK = 0x80000000UL, // most significant w-r bits
    LOWER_MASK = 0x7fffffffUL  // least significant r bits
  };

public:
  the_mersenne_twister_t():
    mti(N + 1)
  {
    // use a default initial seed:
    init_genrand(5489UL);
  }

  //----------------------------------------------------------------
  // init_genrand
  //
  // initializes mt[N] with a seed:
  //
  void init_genrand(const unsigned long & s)
  {
    mt[0] = s & 0xffffffffUL;

    for (mti = 1; mti < N; mti++)
    {
      mt[mti] = (1812433253UL * (mt[mti - 1] ^ (mt[mti - 1] >> 30)) + mti);

      // See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier.
      // In the previous versions, MSBs of the seed affect
      // only MSBs of the array mt[].
      // 2002/01/09 modified by Makoto Matsumoto

      // for machines with WORDSIZE > 32:
      mt[mti] &= 0xffffffffUL;
    }
  }

  //----------------------------------------------------------------
  // init_by_array
  //
  // initialize by an array with array-length.
  // init_key is the array for initializing keys.
  // key_length is its length.
  // slight change for C++, 2004/2/26.
  //
  void init_by_array(const unsigned long * init_key,
		     const unsigned int & key_length)
  {
    init_genrand(19650218UL);

    unsigned int i = 1;
    unsigned int j = 0;

    for (unsigned int k = (N > key_length ? N : key_length); k != 0; k--)
    {
      // non linear:
      mt[i] = ((mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1664525UL)) +
	       init_key[j] + j);

      // for machines with WORDSIZE > 32:
      mt[i] &= 0xffffffffUL;

      i++;
      if (i >= N)
      {
	mt[0] = mt[N - 1];
	i = 1;
      }

      j++;
      if (j >= key_length) j = 0;
    }

    for (unsigned int k = N - 1; k != 0; k--)
    {
      // non linear:
      mt[i] = (mt[i] ^ ((mt[i - 1] ^ (mt[i - 1] >> 30)) * 1566083941UL)) - i;

      // for machines with WORDSIZE > 32:
      mt[i] &= 0xffffffffUL;

      i++;
      if (i >= N)
      {
	mt[0] = mt[N - 1];
	i = 1;
      }
    }

    // MSB is 1; assuring non-zero initial array:
    mt[0] = 0x80000000UL;
  }

  //----------------------------------------------------------------
  // genrand_int32
  //
  // generates a random number on [0, 0xffffffff] interval
  //
  unsigned long genrand_int32() const
  {
    // x = 0, 1;
    // mag01[x] = x * MATRIX_A
    static const unsigned long mag01[] =
    {
      0x0UL,
      MATRIX_A
    };

    unsigned long y;
    if (mti >= N)
    {
      // generate N words at one time:
      for (unsigned int kk = 0; kk < N - M; kk++)
      {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
      }

      for (unsigned int kk = N - M; kk < N - 1; kk++)
      {
	y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
	mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
      }

      y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
      mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

      mti = 0;
    }

    y = mt[mti++];

    // Tempering:
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
  }

  // generates a random number on [0, 0x7fffffff] interval
  inline long genrand_int31() const
  {
    return (long)(genrand_int32() >> 1);
  }

  // These real versions are due to Isaku Wada,
  // added on 2002/01/09:

  // generates a random number on [0, 1] real interval:
  inline double genrand_real1() const
  {
    // divided by 2^32 - 1
    return genrand_int32() * (1.0 / 4294967295.0);
  }

  // generates a random number on [0, 1) real interval:
  inline double genrand_real2() const
  {
    // divided by 2^32:
    return genrand_int32() * (1.0 / 4294967296.0);
  }

  // generates a random number on (0, 1) real interval:
  inline double genrand_real3() const
  {
    // divided by 2^32:
    return (((double)genrand_int32()) + 0.5) * (1.0 / 4294967296.0);
  }

  // generates a random number on [0, 1) with 53-bit resolution:
  inline double genrand_res53() const
  {
    unsigned long a = genrand_int32() >> 5;
    unsigned long b = genrand_int32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
  }

private:
  // the array for the state vector:
  mutable unsigned long mt[N];

  // mti == N + 1 means mt[N] is not initialized:
  mutable unsigned int mti;
};


#endif // THE_MERSENNE_TWISTER_HXX_
