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
#include <yamkaCrc32.h>
#include <yamkaIStorage.h>

// boost includes:
#include <boost/cstdint.hpp>


namespace Yamka
{

  //----------------------------------------------------------------
  // uint64
  // 
  typedef boost::uint64_t uint64;

  //----------------------------------------------------------------
  // int64
  // 
  typedef boost::int64_t int64;
  
  //----------------------------------------------------------------
  // vsizeNumBytes
  // 
  extern unsigned int
  vsizeNumBytes(uint64 i);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  extern uint64
  vsizeDecode(const TByteVec & v);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  extern TByteVec
  vsizeEncode(uint64 vsize);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  // helper function for loading and decoding a payload size
  // descriptor from a storage stream
  // 
  extern uint64
  vsizeDecode(IStorage & storage, Crc32 * computeCrc32 = NULL);
  
  //----------------------------------------------------------------
  // loadEbmlId
  // 
  extern uint64
  loadEbmlId(IStorage & storage, Crc32 * crc = NULL);
  
  //----------------------------------------------------------------
  // uintDecode
  // 
  extern uint64
  uintDecode(const TByteVec & v, unsigned int nbytes);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  extern TByteVec
  uintEncode(uint64 ui, unsigned int nbytes);
  
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
  intDecode(const TByteVec & v, unsigned int len);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  extern TByteVec
  intEncode(int64 si, unsigned int nbytes);
  
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
  
}


#endif // YAMKA_STDINT_H_
