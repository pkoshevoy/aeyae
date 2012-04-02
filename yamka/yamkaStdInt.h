// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 09:03:05 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_STDINT_H_
#define YAMKA_STDINT_H_

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <iostream>
#include <string.h>


namespace Yamka
{
  
  // forward declarations:
  struct IStorage;
  struct HodgePodgeConstIter;
  
  //----------------------------------------------------------------
  // uint64
  // 
  typedef boost::uint64_t uint64;
  
  //----------------------------------------------------------------
  // int64
  // 
  typedef boost::int64_t int64;
  
  //----------------------------------------------------------------
  // TUnixTimeContant
  // 
  enum TUnixTimeContant
  {
    // 2001/01/01 00:00:00 UTC:
    kDateMilleniumUTC = 978307200
  };
  
  //----------------------------------------------------------------
  // uintMax
  // 
  // Constant max unsigned int for each byte size:
  // 
  extern const uint64 uintMax[9];
  
  //----------------------------------------------------------------
  // vsizeUnknown
  // 
  // Reserved vsize value used to indicate that the payload size
  // is unknown, and payload boundary should be determined
  // by detecting an upper level EBML ID that could never be
  // part of the payload.
  // 
  extern const uint64 vsizeUnknown[9];
  
  //----------------------------------------------------------------
  // vsizeNumBytes
  // 
  // Return number of bytes required to encode a given integer.
  // 
  extern unsigned int
  vsizeNumBytes(uint64 i);
  
  //----------------------------------------------------------------
  // vsizeDecode
  //
  // NOTE: if the decoded value equals vsizeUnknown[vsizeSize]
  //       then uintMax[8] will be returned
  // 
  extern uint64
  vsizeDecode(const HodgePodgeConstIter & byteIter, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  // NOTE: if the decoded value equals vsizeUnknown[vsizeSize]
  //       then uintMax[8] will be returned
  // 
  extern uint64
  vsizeDecode(const unsigned char * bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  extern void
  vsizeEncode(uint64 vsize, unsigned char * v, uint64 vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  extern unsigned int
  vsizeEncode(uint64 vsize, unsigned char * v);
  
  //----------------------------------------------------------------
  // vsizeSignedNumBytes
  // 
  extern unsigned int
  vsizeSignedNumBytes(int64 vsize);
  
  //----------------------------------------------------------------
  // vsizeSignedDecode
  // 
  extern int64
  vsizeSignedDecode(const HodgePodgeConstIter & byteIter, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeSignedDecode
  // 
  extern int64
  vsizeSignedDecode(const unsigned char * bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeSignedEncode
  // 
  extern unsigned int
  vsizeSignedEncode(int64 vsize, unsigned char * v);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  // helper function for loading and decoding a payload size
  // descriptor from a storage stream
  // 
  extern uint64
  vsizeDecode(IStorage & storage,
              uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // loadEbmlId
  // 
  extern uint64
  loadEbmlId(IStorage & storage);
  
  //----------------------------------------------------------------
  // uintDecode
  // 
  extern uint64
  uintDecode(const unsigned char * v, uint64 nbytes);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  extern void
  uintEncode(uint64 ui, unsigned char * v, uint64 nbytes);
  
  //----------------------------------------------------------------
  // uintNumBytes
  // 
  extern unsigned int
  uintNumBytes(uint64 ui);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  inline unsigned int
  uintEncode(uint64 ui, unsigned char * v)
  {
    unsigned int numBytes = uintNumBytes(ui);
    uintEncode(ui, v, numBytes);
    return numBytes;
  }
  
  //----------------------------------------------------------------
  // intDecode
  // 
  inline int64
  intDecode(const unsigned char * v, uint64 nbytes)
  {
    uint64 ui = uintDecode(v, nbytes);
    uint64 mu = uintMax[nbytes];
    uint64 mi = mu >> 1;
    int64 i = (ui > mi) ? (ui - mu) - 1 : ui;
    return i;
  }
  
  //----------------------------------------------------------------
  // intEncode
  // 
  inline void
  intEncode(int64 si, unsigned char * v, uint64 nbytes)
  {
    for (uint64 j = 0, k = nbytes - 1; j < nbytes; j++, k--)
    {
      unsigned char n = 0xFF & si;
      si >>= 8;
      v[(std::size_t)k] = n;
    }
  }
  
  //----------------------------------------------------------------
  // intNumBytes
  // 
  extern unsigned int
  intNumBytes(int64 si);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  inline unsigned int
  intEncode(int64 si, unsigned char * v)
  {
    unsigned int numBytes = intNumBytes(si);
    intEncode(si, v, numBytes);
    return numBytes;
  }
  
  //----------------------------------------------------------------
  // floatEncode
  // 
  inline unsigned int
  floatEncode(float d, unsigned char * v)
  {
    const unsigned char * b = (const unsigned char *)&d;
    uint64 i = 0;
    memcpy(&i, b, 4);
    uintEncode(i, v, 4);
    return 4;
  }
  
  //----------------------------------------------------------------
  // floatDecode
  // 
  inline float
  floatDecode(const unsigned char * v)
  {
    float d = 0;
    uint64 i = uintDecode(v, 4);
    memcpy(&d, &i, 4);
    return d;
  }
  
  //----------------------------------------------------------------
  // doubleEncode
  // 
  inline unsigned int
  doubleEncode(double d, unsigned char * v)
  {
    const unsigned char * b = (const unsigned char *)&d;
    uint64 i = 0;
    memcpy(&i, b, 8);
    uintEncode(i, v, 8);
    return 8;
  }
  
  //----------------------------------------------------------------
  // doubleDecode
  // 
  inline double
  doubleDecode(const unsigned char * v)
  {
    double d = 0;
    uint64 i = uintDecode(v, 8);
    memcpy(&d, &i, 8);
    return d;
  }
  
  //----------------------------------------------------------------
  // createUID
  // 
  extern void
  createUID(unsigned char * v, std::size_t numBytes);
  
  //----------------------------------------------------------------
  // createUID
  // 
  extern uint64 createUID();
  
}


#endif // YAMKA_STDINT_H_
