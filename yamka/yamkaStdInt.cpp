// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 12:34:03 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaStdInt.h>

// system includes:
#include <assert.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // vsizeNumBytes
  // 
  unsigned int
  vsizeNumBytes(uint64 i)
  {
    if (i < 0x7F)
    {
      return 1;
    }
    else if (i < 0x3FFF)
    {
      return 2;
    }
    else if (i < 0x1FFFFF)
    {
      return 3;
    }
    else if (i < 0xFFFFFFF)
    {
      return 4;
    }
    else if (i < 0x7FFFFFFFFLLU)
    {
      return 5;
    }
    else if (i < 0x3FFFFFFFFFFLLU)
    {
      return 6;
    }
    else if (i < 0x1FFFFFFFFFFFFLLU)
    {
      return 7;
    }
    
    assert(i < 0xFFFFFFFFFFFFFFLLU);
    return 8;
  }
  
  //----------------------------------------------------------------
  // LeadingBits
  // 
  enum LeadingBits
  {
    LeadingBits1 = 1 << 7,
    LeadingBits2 = 1 << 6,
    LeadingBits3 = 1 << 5,
    LeadingBits4 = 1 << 4,
    LeadingBits5 = 1 << 3,
    LeadingBits6 = 1 << 2,
    LeadingBits7 = 1 << 1,
    LeadingBits8 = 1 << 0
  };

  //----------------------------------------------------------------
  // vsizeDecode
  // 
  uint64
  vsizeDecode(const TByteVec & v)
  {
    uint64 i = 0;
    
    if (v[0] & LeadingBits1)
    {
      // 1 byte:
      i = (v[0] - LeadingBits1);
    }
    else if (v[0] & LeadingBits2)
    {
      // 2 bytes:
      i = ((uint64(v[0] - LeadingBits2) << 8) |
           v[1]);
    }
    else if (v[0] & LeadingBits3)
    {
      // 3 bytes:
      i = ((uint64(v[0] - LeadingBits3) << 16) |
           (uint64(v[1]) << 8) |
           v[2]);
    }
    else if (v[0] & LeadingBits4)
    {
      // 4 bytes:
      i = ((uint64(v[0] - LeadingBits4) << 24) |
           (uint64(v[1]) << 16) |
           (uint64(v[2]) << 8) |
           v[3]);
    }
    else if (v[0] & LeadingBits5)
    {
      // 5 bytes:
      i = ((uint64(v[0] - LeadingBits5) << 32) |
           (uint64(v[1]) << 24) |
           (uint64(v[2]) << 16) |
           (uint64(v[3]) << 8) |
           v[4]);
    }
    else if (v[0] & LeadingBits6)
    {
      // 6 bytes:
      i = ((uint64(v[0] - LeadingBits6) << 40) |
           (uint64(v[1]) << 32) |
           (uint64(v[2]) << 24) |
           (uint64(v[3]) << 16) |
           (uint64(v[4]) << 8) |
           v[5]);
    }
    else if (v[0] & LeadingBits7)
    {
      // 7 bytes:
      i = ((uint64(v[0] - LeadingBits7) << 48) |
           (uint64(v[1]) << 40) |
           (uint64(v[2]) << 32) |
           (uint64(v[3]) << 24) |
           (uint64(v[4]) << 16) |
           (uint64(v[5]) << 8) |
           v[6]);
    }
    else if (v[0] & LeadingBits8)
    {
      // 8 bytes:
      i = ((uint64(v[1]) << 48) |
           (uint64(v[2]) << 40) |
           (uint64(v[3]) << 32) |
           (uint64(v[4]) << 24) |
           (uint64(v[5]) << 16) |
           (uint64(v[6]) << 8) |
           v[7]);
    }
    else
    {
      assert(false);
    }
    
    return i;
  }
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  TByteVec
  vsizeEncode(uint64 vsize)
  {
    unsigned int nbytes = vsizeNumBytes(vsize);
    TByteVec v(nbytes);
    
    for (unsigned int j = 0; j < nbytes; j++)
    {
      unsigned char n = 0xFF & (vsize >> (j * 8));
      v[nbytes - j - 1] = n;
    }
    v[0] |= (1 << (8 - nbytes));
    
    return v;
  }
  
  //----------------------------------------------------------------
  // maxInt
  // 
  static const uint64 maxUInt[] = {
    0x0,
    0xFF,
    0xFFFF,
    0xFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFFFFLLU,
    0xFFFFFFFFFFFFLLU,
    0xFFFFFFFFFFFFFFLLU,
    0xFFFFFFFFFFFFFFFFLLU
  };
  
  //----------------------------------------------------------------
  // uintDecode
  // 
  uint64
  uintDecode(const TByteVec & v, unsigned int nbytes)
  {
    uint64 ui = 0;
    for (unsigned int j = 0; j < nbytes; j++)
    {
      ui = (ui << 8) | v[j];
    }
    return ui;
  }
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  TByteVec
  uintEncode(uint64 ui, unsigned int nbytes)
  {
    TByteVec v(nbytes);
    for (unsigned int j = 0, k = nbytes - 1; j < nbytes; j++, k--)
    {
      unsigned char n = 0xFF & ui;
      ui >>= 8;
      v[k] = n;
    }
    return v;
  }
  
  //----------------------------------------------------------------
  // uintNumBytes
  // 
  unsigned int
  uintNumBytes(uint64 ui)
  {
    if (ui <= 0xFF)
    {
      return 1;
    }
    else if (ui <= 0xFFFF)
    {
      return 2;
    }
    else if (ui <= 0xFFFFFF)
    {
      return 3;
    }
    else if (ui <= 0xFFFFFFFF)
    {
      return 4;
    }
    else if (ui <= 0xFFFFFFFFFFLLU)
    {
      return 5;
    }
    else if (ui <= 0xFFFFFFFFFFFFLLU)
    {
      return 6;
    }
    else if (ui <= 0xFFFFFFFFFFFFFFLLU)
    {
      return 7;
    }
    
    return 8;
  }
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  TByteVec
  uintEncode(uint64 ui)
  {
    unsigned int nbytes = uintNumBytes(ui);
    return uintEncode(ui, nbytes);
  }
  
  //----------------------------------------------------------------
  // intDecode
  // 
  int64
  intDecode(const TByteVec & v, unsigned int nbytes)
  {
    uint64 ui = uintDecode(v, nbytes);
    uint64 mu = maxUInt[nbytes];
    uint64 mi = mu >> 1;
    int64 i = (ui > mi) ? (ui - mu) - 1 : ui;
    return i;
  }
  
  //----------------------------------------------------------------
  // intEncode
  // 
  TByteVec
  intEncode(int64 si, unsigned int nbytes)
  {
    TByteVec v(nbytes);
    for (unsigned int j = 0, k = nbytes - 1; j < nbytes; j++, k--)
    {
      unsigned char n = 0xFF & si;
      si >>= 8;
      v[k] = n;
    }
    return v;
  }
  
  //----------------------------------------------------------------
  // intNumBytes
  // 
  unsigned int
  intNumBytes(int64 si)
  {
    if (si >= -0x80 && si <= 0x7F)
    {
      return 1;
    }
    else if (si >= -0x8000 && si <= 0x7FFF)
    {
      return 2;
    }
    else if (si >= -0x800000 && si <= 0x7FFFFF)
    {
      return 3;
    }
    else if (si >= -0x80000000 && si <= 0x7FFFFFFF)
    {
      return 4;
    }
    else if (si >= -0x8000000000LL && si <= 0x7FFFFFFFFFLL)
    {
      return 5;
    }
    else if (si >= -0x800000000000LL && si <= 0x7FFFFFFFFFFFLL)
    {
      return 6;
    }
    else if (si >= -0x80000000000000LL && si <= 0x7FFFFFFFFFFFFFLL)
    {
      return 7;
    }
    
    return 8;
  }
  
  //----------------------------------------------------------------
  // intEncode
  // 
  TByteVec
  intEncode(int64 si)
  {
    unsigned int nbytes = intNumBytes(si);
    return intEncode(si, nbytes);
  }
  
}
