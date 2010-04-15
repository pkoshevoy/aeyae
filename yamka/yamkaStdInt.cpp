// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 12:34:03 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaStdInt.h>
#include <yamkaIStorage.h>
#include <yamkaCrc32.h>

// system includes:
#include <assert.h>
#include <string.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // UI64
  // 
# ifdef _WIN32
# define UI64(i) uint64(i)
# else
# define UI64(i) i##LLU
# endif
  
  //----------------------------------------------------------------
  // SI64
  // 
# ifdef _WIN32
# define SI64(i) int64(i)
# else
# define SI64(i) i##LL
# endif
  
  //----------------------------------------------------------------
  // uintMax
  // 
  // Constant max unsigned int for each byte size:
  // 
  const uint64 uintMax[9] = {
    UI64(0x0),
    UI64(0xFF),
    UI64(0xFFFF),
    UI64(0xFFFFFF),
    UI64(0xFFFFFFFF),
    UI64(0xFFFFFFFFFF),
    UI64(0xFFFFFFFFFFFF),
    UI64(0xFFFFFFFFFFFFFF),
    UI64(0xFFFFFFFFFFFFFFFF)
  };
  
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
    else if (i < UI64(0x3FFF))
    {
      return 2;
    }
    else if (i < UI64(0x1FFFFF))
    {
      return 3;
    }
    else if (i < UI64(0xFFFFFFF))
    {
      return 4;
    }
    else if (i < UI64(0x7FFFFFFFF))
    {
      return 5;
    }
    else if (i < UI64(0x3FFFFFFFFFF))
    {
      return 6;
    }
    else if (i < UI64(0x1FFFFFFFFFFFF))
    {
      return 7;
    }
    
    assert(i < UI64(0xFFFFFFFFFFFFFF));
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
  // vsizeDecodeBytes
  //
  template <typename bytes_t>
  uint64
  vsizeDecodeBytes(const bytes_t & v, uint64 & vsizeSize)
  {
    uint64 i = 0;
    
    if (v[0] & LeadingBits1)
    {
      vsizeSize = 1;
      i = (v[0] - LeadingBits1);
    }
    else if (v[0] & LeadingBits2)
    {
      vsizeSize = 2;
      i = ((uint64(v[0] - LeadingBits2) << 8) |
           v[1]);
    }
    else if (v[0] & LeadingBits3)
    {
      vsizeSize = 3;
      i = ((uint64(v[0] - LeadingBits3) << 16) |
           (uint64(v[1]) << 8) |
           v[2]);
    }
    else if (v[0] & LeadingBits4)
    {
      vsizeSize = 4;
      i = ((uint64(v[0] - LeadingBits4) << 24) |
           (uint64(v[1]) << 16) |
           (uint64(v[2]) << 8) |
           v[3]);
    }
    else if (v[0] & LeadingBits5)
    {
      vsizeSize = 5;
      i = ((uint64(v[0] - LeadingBits5) << 32) |
           (uint64(v[1]) << 24) |
           (uint64(v[2]) << 16) |
           (uint64(v[3]) << 8) |
           v[4]);
    }
    else if (v[0] & LeadingBits6)
    {
      vsizeSize = 6;
      i = ((uint64(v[0] - LeadingBits6) << 40) |
           (uint64(v[1]) << 32) |
           (uint64(v[2]) << 24) |
           (uint64(v[3]) << 16) |
           (uint64(v[4]) << 8) |
           v[5]);
    }
    else if (v[0] & LeadingBits7)
    {
      vsizeSize = 7;
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
      vsizeSize = 8;
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
      vsizeSize = 0;
    }
    
    return i;
  }

  //----------------------------------------------------------------
  // vsizeDecode
  // 
  uint64
  vsizeDecode(const Bytes & bytes, uint64 & vsizeSize)
  {
    uint64 i = vsizeDecodeBytes(bytes, vsizeSize);
    return i;
  }

  //----------------------------------------------------------------
  // vsizeDecode
  // 
  uint64
  vsizeDecode(const TByteVec & bytes, uint64 & vsizeSize)
  {
    uint64 i = vsizeDecodeBytes(bytes, vsizeSize);
    return i;
  }
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  TByteVec
  vsizeEncode(uint64 vsize)
  {
    unsigned int numBytes = vsizeNumBytes(vsize);
    TByteVec v(numBytes);
    
    for (unsigned int j = 0; j < numBytes; j++)
    {
      unsigned char n = 0xFF & (vsize >> (j * 8));
      v[numBytes - j - 1] = n;
    }
    v[0] |= (1 << (8 - numBytes));
    
    return v;
  }

  //----------------------------------------------------------------
  // vsizeLoad
  // 
  // helper function for loading a vsize unsigned integer
  // from a storage stream
  // 
  static bool
  vsizeLoad(TByteVec & v,
            IStorage & storage,
            Crc32 * crc,
            unsigned int maxBytes)
  {
    Bytes vsize(1);
    if (!storage.loadAndCalcCrc32(vsize, crc))
    {
      return false;
    }
    
    // find how many bytes remain to be read:
    const TByte firstByte = vsize[0];
    TByte leadingBitsMask = 1 << 7;
    unsigned int numBytesToLoad = 0;
    for (; numBytesToLoad < maxBytes; numBytesToLoad++)
    {
      if (firstByte & leadingBitsMask)
      {
        break;
      }
      
      leadingBitsMask >>= 1;
    }
    
    if (numBytesToLoad > maxBytes - 1)
    {
      return false;
    }
    
    // load the remaining vsize bytes:
    Bytes bytes(numBytesToLoad);
    if (!storage.loadAndCalcCrc32(bytes, crc))
    {
      return false;
    }
    
    v = TByteVec(vsize + bytes);
    return true;
  }
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  // helper function for loading and decoding a payload size
  // descriptor from a storage stream
  // 
  uint64
  vsizeDecode(IStorage & storage,
              uint64 & vsizeSize,
              Crc32 * crc)
  {
    TByteVec v;
    if (vsizeLoad(v, storage, crc, 8))
    {
      return vsizeDecode(v, vsizeSize);
    }

    // invalid vsize or vsize insufficient storage:
    return uintMax[8];
  }
  
  //----------------------------------------------------------------
  // loadEbmlId
  // 
  uint64
  loadEbmlId(IStorage & storage, Crc32 * crc)
  {
    TByteVec v;
    if (vsizeLoad(v, storage, crc, 4))
    {
      return uintDecode(v, v.size());
    }
    
    // invalid EBML ID or insufficient storage:
    return 0;
  }
  
  
  //----------------------------------------------------------------
  // uintDecode
  // 
  uint64
  uintDecode(const TByteVec & v, uint64 numBytes)
  {
    uint64 ui = 0;
    for (unsigned int j = 0; j < numBytes; j++)
    {
      ui = (ui << 8) | v[j];
    }
    return ui;
  }
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  TByteVec
  uintEncode(uint64 ui, uint64 numBytes)
  {
    TByteVec v((std::size_t)numBytes);
    for (uint64 j = 0, k = numBytes - 1; j < numBytes; j++, k--)
    {
      unsigned char n = 0xFF & ui;
      ui >>= 8;
      v[(std::size_t)k] = n;
    }
    return v;
  }
  
  //----------------------------------------------------------------
  // uintNumBytes
  // 
  unsigned int
  uintNumBytes(uint64 ui)
  {
    if (ui <= UI64(0xFF))
    {
      return 1;
    }
    else if (ui <= UI64(0xFFFF))
    {
      return 2;
    }
    else if (ui <= UI64(0xFFFFFF))
    {
      return 3;
    }
    else if (ui <= UI64(0xFFFFFFFF))
    {
      return 4;
    }
    else if (ui <= UI64(0xFFFFFFFFFF))
    {
      return 5;
    }
    else if (ui <= UI64(0xFFFFFFFFFFFF))
    {
      return 6;
    }
    else if (ui <= UI64(0xFFFFFFFFFFFFFF))
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
    unsigned int numBytes = uintNumBytes(ui);
    return uintEncode(ui, numBytes);
  }
  
  //----------------------------------------------------------------
  // intDecode
  // 
  int64
  intDecode(const TByteVec & v, uint64 numBytes)
  {
    uint64 ui = uintDecode(v, numBytes);
    uint64 mu = uintMax[numBytes];
    uint64 mi = mu >> 1;
    int64 i = (ui > mi) ? (ui - mu) - 1 : ui;
    return i;
  }
  
  //----------------------------------------------------------------
  // intEncode
  // 
  TByteVec
  intEncode(int64 si, uint64 numBytes)
  {
    TByteVec v((std::size_t)numBytes);
    for (uint64 j = 0, k = numBytes - 1; j < numBytes; j++, k--)
    {
      unsigned char n = 0xFF & si;
      si >>= 8;
      v[(std::size_t)k] = n;
    }
    return v;
  }
  
  //----------------------------------------------------------------
  // intNumBytes
  // 
  unsigned int
  intNumBytes(int64 si)
  {
    if (si >= -SI64(0x80) && si <= SI64(0x7F))
    {
      return 1;
    }
    else if (si >= -SI64(0x8000) && si <= SI64(0x7FFF))
    {
      return 2;
    }
    else if (si >= -SI64(0x800000) && si <= SI64(0x7FFFFF))
    {
      return 3;
    }
    else if (si >= -SI64(0x80000000) && si <= SI64(0x7FFFFFFF))
    {
      return 4;
    }
    else if (si >= -SI64(0x8000000000) && si <= SI64(0x7FFFFFFFFF))
    {
      return 5;
    }
    else if (si >= -SI64(0x800000000000) && si <= SI64(0x7FFFFFFFFFFF))
    {
      return 6;
    }
    else if (si >= -SI64(0x80000000000000) && si <= SI64(0x7FFFFFFFFFFFFF))
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
    unsigned int numBytes = intNumBytes(si);
    return intEncode(si, numBytes);
  }
  
  //----------------------------------------------------------------
  // floatEncode
  // 
  TByteVec
  floatEncode(float d)
  {
    const unsigned char * b = (const unsigned char *)&d;
    uint64 i = 0;
    memcpy(&i, b, 4);
    return uintEncode(i, 4);
  }
  
  //----------------------------------------------------------------
  // floatDecode
  // 
  float
  floatDecode(const TByteVec & v)
  {
    float d = 0;
    uint64 i = uintDecode(v, 4);
    memcpy(&d, &i, 4);
    return d;
  }
  
  //----------------------------------------------------------------
  // doubleEncode
  // 
  TByteVec
  doubleEncode(double d)
  {
    const unsigned char * b = (const unsigned char *)&d;
    uint64 i = 0;
    memcpy(&i, b, 8);
    return uintEncode(i, 8);
  }
  
  //----------------------------------------------------------------
  // doubleDecode
  // 
  double
  doubleDecode(const TByteVec & v)
  {
    double d = 0;
    uint64 i = uintDecode(v, 8);
    memcpy(&d, &i, 8);
    return d;
  }
  
}
