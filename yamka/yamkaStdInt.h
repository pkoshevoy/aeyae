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
  unsigned int
  vsizeNumBytes(uint64 i);
  
  //----------------------------------------------------------------
  // vsizeDecode
  // 
  uint64
  vsizeDecode(const TByteVec & v);
  
  //----------------------------------------------------------------
  // vsizeEncode
  // 
  TByteVec
  vsizeEncode(uint64 vsize);
  
  //----------------------------------------------------------------
  // uintDecode
  // 
  uint64
  uintDecode(const TByteVec & v, unsigned int nbytes);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  TByteVec
  uintEncode(uint64 ui, unsigned int nbytes);
  
  //----------------------------------------------------------------
  // uintNumBytes
  // 
  unsigned int
  uintNumBytes(uint64 ui);
  
  //----------------------------------------------------------------
  // uintEncode
  // 
  TByteVec
  uintEncode(uint64 ui);
  
  //----------------------------------------------------------------
  // intDecode
  // 
  int64
  intDecode(const TByteVec & v, unsigned int len);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  TByteVec
  intEncode(int64 si, unsigned int nbytes);
  
  //----------------------------------------------------------------
  // intNumBytes
  // 
  unsigned int
  intNumBytes(int64 si);
  
  //----------------------------------------------------------------
  // intEncode
  // 
  TByteVec
  intEncode(int64 si);
  
}


#endif // YAMKA_STDINT_H_
