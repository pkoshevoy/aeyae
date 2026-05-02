// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri May  1 07:45:03 PM MDT 2026
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_endian.h"


//----------------------------------------------------------------
// yae::bswap_16
//
void
yae::bswap_16(void * dst,
              void * dst_end,
              const void * src,
              const void * src_end)
{
  YAE_ASSERT(dst_end <= src || src_end <= dst);

  const uint8_t * i = (const uint8_t *)src;
  const uint8_t * i1 = (const uint8_t *)src_end;
  YAE_ASSERT((i1 - i) % 2 == 0);

  uint8_t * j = (uint8_t *)dst;
  uint8_t * j1 = (uint8_t *)dst_end;
  YAE_ASSERT((j1 - j) % 2 == 0);

  for (; i < i1 && j < j1; i += 2, j += 2)
  {
    j[0] = i[1];
    j[1] = i[0];
  }
}

//----------------------------------------------------------------
// yae::bswap_32
//
void
yae::bswap_32(void * dst,
              void * dst_end,
              const void * src,
              const void * src_end)
{
  YAE_ASSERT(dst_end <= src || src_end <= dst);

  const uint8_t * i = (const uint8_t *)src;
  const uint8_t * i1 = (const uint8_t *)src_end;
  YAE_ASSERT((i1 - i) % 4 == 0);

  uint8_t * j = (uint8_t *)dst;
  uint8_t * j1 = (uint8_t *)dst_end;
  YAE_ASSERT((j1 - j) % 4 == 0);

  for (; i < i1 && j < j1; i += 4, j += 4)
  {
    j[0] = i[3];
    j[1] = i[2];
    j[2] = i[1];
    j[3] = i[0];
  }
}

//----------------------------------------------------------------
// yae::bswap_64
//
void
yae::bswap_64(void * dst,
              void * dst_end,
              const void * src,
              const void * src_end)
{
  YAE_ASSERT(dst_end <= src || src_end <= dst);

  const uint8_t * i = (const uint8_t *)src;
  const uint8_t * i1 = (const uint8_t *)src_end;
  YAE_ASSERT((i1 - i) % 8 == 0);

  uint8_t * j = (uint8_t *)dst;
  uint8_t * j1 = (uint8_t *)dst_end;
  YAE_ASSERT((j1 - j) % 8 == 0);

  for (; i < i1 && j < j1; i += 8, j += 8)
  {
    j[0] = i[7];
    j[1] = i[6];
    j[2] = i[5];
    j[3] = i[4];
    j[4] = i[3];
    j[5] = i[2];
    j[6] = i[1];
    j[7] = i[0];
  }
}

//----------------------------------------------------------------
// yae::bswap_16_inplace
//
void
yae::bswap_16_inplace(void * u16, void * u16_end)
{
  uint8_t * i = (uint8_t *)u16;
  uint8_t * i1 = (uint8_t *)u16_end;
  YAE_ASSERT((i1 - i) % 2 == 0);

  for (; i < i1; i += 2)
  {
    yae::bswap(i[0], i[1]);
  }
}

//----------------------------------------------------------------
// yae::bswap_32_inplace
//
void
yae::bswap_32_inplace(void * u32, void * u32_end)
{
  uint8_t * i = (uint8_t *)u32;
  uint8_t * i1 = (uint8_t *)u32_end;
  YAE_ASSERT((i1 - i) % 4 == 0);

  for (; i < i1; i += 4)
  {
    yae::bswap(i[0], i[3]);
    yae::bswap(i[1], i[2]);
  }
}

//----------------------------------------------------------------
// yae::bswap_64_inplace
//
void
yae::bswap_64_inplace(void * u64, void * u64_end)
{
  uint8_t * i = (uint8_t *)u64;
  uint8_t * i1 = (uint8_t *)u64_end;
  YAE_ASSERT((i1 - i) % 8 == 0);

  for (; i < i1; i += 8)
  {
    yae::bswap(i[0], i[7]);
    yae::bswap(i[1], i[6]);
    yae::bswap(i[2], i[5]);
    yae::bswap(i[3], i[4]);
  }
}
