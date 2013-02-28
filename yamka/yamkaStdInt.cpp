// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 12:34:03 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaStdInt.h>
#include <yamkaIStorage.h>
#include <yamkaHodgePodge.h>

// system includes:
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits>
#include <ctime>


namespace Yamka
{

  //----------------------------------------------------------------
  // YamkaUnsignedInt64
  //
# ifdef _WIN32
# define YamkaUnsignedInt64(i) uint64(i)
# else
# define YamkaUnsignedInt64(i) i##LLU
# endif

  //----------------------------------------------------------------
  // YamkaSignedInt64
  //
# ifdef _WIN32
# define YamkaSignedInt64(i) int64(i)
# else
# define YamkaSignedInt64(i) i##LL
# endif

  //----------------------------------------------------------------
  // uintMax
  //
  // Constant max unsigned int for each byte size:
  //
  const uint64 uintMax[9] =
  {
    YamkaUnsignedInt64(0x0),
    YamkaUnsignedInt64(0xFF),
    YamkaUnsignedInt64(0xFFFF),
    YamkaUnsignedInt64(0xFFFFFF),
    YamkaUnsignedInt64(0xFFFFFFFF),
    YamkaUnsignedInt64(0xFFFFFFFFFF),
    YamkaUnsignedInt64(0xFFFFFFFFFFFF),
    YamkaUnsignedInt64(0xFFFFFFFFFFFFFF),
    YamkaUnsignedInt64(0xFFFFFFFFFFFFFFFF)
  };

  //----------------------------------------------------------------
  // vsizeRange
  //
  // NOTE: it's 0x...E and not 0x...F, because 0x...F are reserved
  //       for "unknown size" special value per EBML specs.
  //
  static const uint64 vsizeRange[9] =
  {
    YamkaUnsignedInt64(0x0),
    YamkaUnsignedInt64(0x7E),
    YamkaUnsignedInt64(0x3FFE),
    YamkaUnsignedInt64(0x1FFFFE),
    YamkaUnsignedInt64(0x0FFFFFFE),
    YamkaUnsignedInt64(0x07FFFFFFFE),
    YamkaUnsignedInt64(0x03FFFFFFFFFE),
    YamkaUnsignedInt64(0x01FFFFFFFFFFFE),
    YamkaUnsignedInt64(0x00FFFFFFFFFFFFFE)
  };

  //----------------------------------------------------------------
  // vsizeUnknown
  //
  const uint64 vsizeUnknown[9] =
  {
    YamkaUnsignedInt64(0x0),
    YamkaUnsignedInt64(0x7F),
    YamkaUnsignedInt64(0x3FFF),
    YamkaUnsignedInt64(0x1FFFFF),
    YamkaUnsignedInt64(0x0FFFFFFF),
    YamkaUnsignedInt64(0x07FFFFFFFF),
    YamkaUnsignedInt64(0x03FFFFFFFFFF),
    YamkaUnsignedInt64(0x01FFFFFFFFFFFF),
    YamkaUnsignedInt64(0x00FFFFFFFFFFFFFF)
  };

  //----------------------------------------------------------------
  // vsizeNumBytes
  //
  unsigned int
  vsizeNumBytes(uint64 vsize)
  {
    for (unsigned int j = 1; j < 8; j++)
    {
      if (vsize <= vsizeRange[j])
      {
        return j;
      }
    }

    assert(vsize <= vsizeRange[8]);
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

    if (vsizeUnknown[vsizeSize] == i)
    {
      i = uintMax[8];
    }

    return i;
  }

  //----------------------------------------------------------------
  // vsizeDecode
  //
  uint64
  vsizeDecode(const HodgePodgeConstIter & byteIter, uint64 & vsizeSize)
  {
    uint64 i = vsizeDecodeBytes(byteIter, vsizeSize);
    return i;
  }

  //----------------------------------------------------------------
  // vsizeDecode
  //
  uint64
  vsizeDecode(const unsigned char * bytes, uint64 & vsizeSize)
  {
    uint64 i = vsizeDecodeBytes(bytes, vsizeSize);
    return i;
  }

  //----------------------------------------------------------------
  // vsizeEncode
  //
  void
  vsizeEncode(uint64 vsize, unsigned char * v, uint64 numBytes)
  {
    for (unsigned int j = 0; j < numBytes; j++)
    {
      unsigned char n = 0xFF & (vsize >> (j * 8));
      v[(std::size_t)(numBytes - j - 1)] = n;
    }
    v[0] |= (1 << (8 - numBytes));
  }

  //----------------------------------------------------------------
  // vsizeEncode
  //
  unsigned int
  vsizeEncode(uint64 vsize, unsigned char * v)
  {
    unsigned int numBytes = vsizeNumBytes(vsize);
    vsizeEncode(vsize, v, numBytes);
    return numBytes;
  }

  //----------------------------------------------------------------
  // vsizeHalfRange
  //
  static const int64 vsizeHalfRange[9] =
  {
    YamkaSignedInt64(0x0),
    YamkaSignedInt64(0x3F),
    YamkaSignedInt64(0x1FFF),
    YamkaSignedInt64(0x0FFFFF),
    YamkaSignedInt64(0x07FFFFFF),
    YamkaSignedInt64(0x03FFFFFFFF),
    YamkaSignedInt64(0x01FFFFFFFFFF),
    YamkaSignedInt64(0x00FFFFFFFFFFFF),
    YamkaSignedInt64(0x007FFFFFFFFFFFFF)
  };

  //----------------------------------------------------------------
  // vsizeSignedNumBytes
  //
  unsigned int
  vsizeSignedNumBytes(int64 vsize)
  {
    for (unsigned int j = 1; j < 8; j++)
    {
      if (vsize >= -vsizeHalfRange[j] &&
          vsize <= vsizeHalfRange[j])
      {
        return j;
      }
    }

    assert(vsize >= -vsizeHalfRange[8] &&
           vsize <= vsizeHalfRange[8]);
    return 8;
  }

  //----------------------------------------------------------------
  // vsizeSignedDecodeBytes
  //
  template <typename bytes_t>
  int64
  vsizeSignedDecodeBytes(const bytes_t & v, uint64 & vsizeSize)
  {
    uint64 u = vsizeDecodeBytes(v, vsizeSize);
    int64 i = u - vsizeHalfRange[vsizeSize];
    return i;
  }

  //----------------------------------------------------------------
  // vsizeSignedDecode
  //
  int64
  vsizeSignedDecode(const HodgePodgeConstIter & byteIter, uint64 & vsizeSize)
  {
    int64 i = vsizeSignedDecodeBytes(byteIter, vsizeSize);
    return i;
  }

  //----------------------------------------------------------------
  // vsizeSignedDecode
  //
  int64
  vsizeSignedDecode(const unsigned char * bytes, uint64 & vsizeSize)
  {
    int64 i = vsizeSignedDecodeBytes(bytes, vsizeSize);
    return i;
  }

  //----------------------------------------------------------------
  // vsizeEncode
  //
  unsigned int
  vsizeSignedEncode(int64 vsize, unsigned char * v)
  {
    unsigned int numBytes = vsizeSignedNumBytes(vsize);
    uint64 u = vsize + vsizeHalfRange[numBytes];
    vsizeEncode(u, v, numBytes);
    return numBytes;
  }

  //----------------------------------------------------------------
  // vsizeLoad
  //
  // helper function for loading a vsize unsigned integer
  // from a storage stream
  //
  static unsigned int
  vsizeLoad(unsigned char * vsize,
            IStorage & storage,
            unsigned int maxBytes)
  {
    if (!storage.peek(vsize, 1))
    {
      return 0;
    }

    // find how many bytes remain to be read:
    const unsigned char firstByte = vsize[0];
    unsigned char leadingBitsMask = 1 << 7;
    unsigned int numBytesToLoad = 0;
    for (; numBytesToLoad < maxBytes; numBytesToLoad++)
    {
      if (firstByte & leadingBitsMask)
      {
        break;
      }

      leadingBitsMask >>= 1;
    }

    if (numBytesToLoad + 1 > maxBytes)
    {
      return 0;
    }

    // load the remaining vsize bytes:
    if (numBytesToLoad && !storage.peek(vsize, numBytesToLoad + 1))
    {
      return 0;
    }

    return (unsigned int)storage.skip(numBytesToLoad + 1);
  }

  //----------------------------------------------------------------
  // vsizeDecode
  //
  // helper function for loading and decoding a payload size
  // descriptor from a storage stream
  //
  uint64
  vsizeDecode(IStorage & storage, uint64 & vsizeSize)
  {
    unsigned char v[8];
    vsizeSize = vsizeLoad(v, storage, 8);
    if (vsizeSize)
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
  loadEbmlId(IStorage & storage)
  {
    unsigned char v[4];
    unsigned int numBytes = vsizeLoad(v, storage, 4);
    if (numBytes)
    {
      return uintDecode(v, numBytes);
    }

    // invalid EBML ID or insufficient storage:
    return 0;
  }

  //----------------------------------------------------------------
  // uintDecode
  //
  uint64
  uintDecode(const unsigned char * v, uint64 numBytes)
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
  void
  uintEncode(uint64 ui, unsigned char * v, uint64 numBytes)
  {
    for (uint64 j = 0, k = numBytes - 1; j < numBytes; j++, k--)
    {
      unsigned char n = 0xFF & ui;
      ui >>= 8;
      v[(std::size_t)k] = n;
    }
  }

  //----------------------------------------------------------------
  // uintNumBytes
  //
  unsigned int
  uintNumBytes(uint64 ui)
  {
    if (ui <= YamkaUnsignedInt64(0xFF))
    {
      return 1;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFF))
    {
      return 2;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFFFF))
    {
      return 3;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFFFFFF))
    {
      return 4;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFFFFFFFF))
    {
      return 5;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFFFFFFFFFF))
    {
      return 6;
    }
    else if (ui <= YamkaUnsignedInt64(0xFFFFFFFFFFFFFF))
    {
      return 7;
    }

    return 8;
  }

  //----------------------------------------------------------------
  // intNumBytes
  //
  unsigned int
  intNumBytes(int64 si)
  {
    if (si >= -YamkaSignedInt64(0x80) &&
        si <= YamkaSignedInt64(0x7F))
    {
      return 1;
    }
    else if (si >= -YamkaSignedInt64(0x8000) &&
             si <= YamkaSignedInt64(0x7FFF))
    {
      return 2;
    }
    else if (si >= -YamkaSignedInt64(0x800000) &&
             si <= YamkaSignedInt64(0x7FFFFF))
    {
      return 3;
    }
    else if (si >= -YamkaSignedInt64(0x80000000) &&
             si <= YamkaSignedInt64(0x7FFFFFFF))
    {
      return 4;
    }
    else if (si >= -YamkaSignedInt64(0x8000000000) &&
             si <= YamkaSignedInt64(0x7FFFFFFFFF))
    {
      return 5;
    }
    else if (si >= -YamkaSignedInt64(0x800000000000) &&
             si <= YamkaSignedInt64(0x7FFFFFFFFFFF))
    {
      return 6;
    }
    else if (si >= -YamkaSignedInt64(0x80000000000000) &&
             si <= YamkaSignedInt64(0x7FFFFFFFFFFFFF))
    {
      return 7;
    }

    return 8;
  }


  //----------------------------------------------------------------
  // createUID
  //
  void
  createUID(unsigned char * v, std::size_t numBytes)
  {
    static bool seeded = false;
    if (!seeded)
    {
      std::time_t currentTime = std::time(NULL);
      unsigned int seed =
        (currentTime - kDateMilleniumUTC) %
        std::numeric_limits<unsigned int>::max();

      srand(seed);
      seeded = true;
    }

    for (std::size_t i = 0; i < numBytes; i++)
    {
      double t = double(rand()) / double(RAND_MAX);
      v[i] = (unsigned char)(t * 255.0);
    }
  }

  //----------------------------------------------------------------
  // createUID
  //
  uint64
  createUID()
  {
    unsigned char v[8];
    createUID(v, 8);

    uint64 id = uintDecode(v, 8) >> 7;
    return id;
  }

}
