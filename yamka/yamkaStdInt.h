// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 09:03:05 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_STDINT_H_
#define YAMKA_STDINT_H_

// yamka includes:
#include <yamkaBytes.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <iostream>


namespace Yamka
{
  
  // forward declarations:
  struct IStorage;
  
  //----------------------------------------------------------------
  // uint64
  // 
  typedef boost::uint64_t uint64;
  
  //----------------------------------------------------------------
  // int64
  // 
  typedef boost::int64_t int64;
  
  
  //----------------------------------------------------------------
  // uintMax
  // 
  // Constant max unsigned int for each byte size:
  // 
  extern const uint64 uintMax[9];
  
  //----------------------------------------------------------------
  // vsizeNumBytes
  // 
  extern unsigned int
  vsizeNumBytes(uint64 i);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  extern uint64
  vsizeDecode(const Bytes & bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  extern uint64
  vsizeDecode(const TByteVec & v, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  extern uint64
  vsizeDecode(const TByte * v, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  extern TByteVec
  vsizeEncode(uint64 vsize, uint64 vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  extern TByteVec
  vsizeEncode(uint64 vsize);
  
  //----------------------------------------------------------------
  // vsizeSignedNumBytes
  // 
  extern unsigned int
  vsizeSignedNumBytes(int64 vsize);
  
  //----------------------------------------------------------------
  // vsizeSignedDecode
  // 
  extern int64
  vsizeSignedDecode(const Bytes & bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeSignedDecode
  // 
  extern int64
  vsizeSignedDecode(const TByteVec & bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeSignedDecode
  // 
  extern int64
  vsizeSignedDecode(const TByte * bytes, uint64 & vsizeSize);
  
  //----------------------------------------------------------------
  // vsizeSignedEncode
  // 
  extern TByteVec
  vsizeSignedEncode(int64 vsize);
  
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
  uintDecode(const TByteVec & v, uint64 nbytes);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  extern TByteVec
  uintEncode(uint64 ui, uint64 nbytes);
  
  //----------------------------------------------------------------
  // uintNumBytes
  // 
  extern unsigned int
  uintNumBytes(uint64 ui);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  extern TByteVec
  uintEncode(uint64 ui);
  
  //----------------------------------------------------------------
  // intDecode
  // 
  extern int64
  intDecode(const TByteVec & v, uint64 len);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  extern TByteVec
  intEncode(int64 si, uint64 nbytes);
  
  //----------------------------------------------------------------
  // intNumBytes
  // 
  extern unsigned int
  intNumBytes(int64 si);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  extern TByteVec
  intEncode(int64 si);

  //----------------------------------------------------------------
  // floatEncode
  // 
  extern TByteVec
  floatEncode(float f);

  //----------------------------------------------------------------
  // floatDecode
  // 
  extern float
  floatDecode(const TByteVec & v);
  
  //----------------------------------------------------------------
  // doubleEncode
  // 
  extern TByteVec
  doubleEncode(double d);
  
  //----------------------------------------------------------------
  // doubleDecode
  // 
  extern double
  doubleDecode(const TByteVec & v);
  
  //----------------------------------------------------------------
  // createUID
  // 
  extern TByteVec
  createUID(std::size_t numBytes);
  
  //----------------------------------------------------------------
  // Indent
  // 
  namespace Indent
  {
    struct More
    {
      More();
      ~More();
    };
    
    extern unsigned int depth_;
  }
  
  //----------------------------------------------------------------
  // indent
  // 
  struct indent
  {
    indent(unsigned int depth = 0);
    
    const unsigned int depth_;
  };
  
  //----------------------------------------------------------------
  // operator <<
  // 
  extern std::ostream &
  operator << (std::ostream & s, const indent & ind);
  
}


#endif // YAMKA_STDINT_H_
