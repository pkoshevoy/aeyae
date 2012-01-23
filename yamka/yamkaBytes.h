// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 17:10:52 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_BYTES_H_
#define YAMKA_BYTES_H_

// yamka includes:
#include <yamkaSharedPtr.h>

// system includes:
#include <deque>
#include <vector>
#include <iostream>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // TByte
  // 
  typedef unsigned char TByte;
  
  //----------------------------------------------------------------
  // TByteVec
  // 
  typedef std::vector<TByte> TByteVec;
  
  //----------------------------------------------------------------
  // TByteVecDec
  // 
  typedef std::deque<TByteVec> TByteVecDec;
  
  //----------------------------------------------------------------
  // TByteVecDecPtr
  // 
  typedef TSharedPtr<TByteVecDec> TByteVecDecPtr;
  
  //----------------------------------------------------------------
  // Bytes
  // 
  struct Bytes
  {
    Bytes(std::size_t size = 0);
    Bytes(const void * data, std::size_t size);
    explicit Bytes(const TByteVec & byteVec);
    
    // convert to a byte vector (deep copy of data):
    operator TByteVec() const;
    
    // reset the size and copy all given data:
    Bytes & deepCopy(const Bytes & bytes);
    Bytes & deepCopy(const TByte * bytes, std::size_t numBytes);
    
    // append via shallow copy of given bytes:
    Bytes & operator += (const Bytes & bytes);
    
    // append via deep copy of given byte(s):
    Bytes & operator << (const TByteVec & bytes);
    Bytes & operator << (const TByte & byte);
    Bytes & operator << (const std::string & str);
    
    // size accessors:
    bool empty() const;
    std::size_t size() const;
    
    // byte accessors:
    const TByte & getByte(std::size_t i) const;
    
    inline const TByte & operator [] (std::size_t i) const
    { return getByte(i); }
    
    inline TByte & operator [] (std::size_t i)
    { return const_cast<TByte &>(this->getByte(i)); }
    
    // using a shared pointer to avoid expensive deep copies:
    TByteVecDecPtr bytes_;
  };
  
  //----------------------------------------------------------------
  // operator +
  // 
  extern Bytes operator + (const Bytes & a, const Bytes & b);
  
  //----------------------------------------------------------------
  // operator <<
  // 
  extern std::ostream &
  operator << (std::ostream & os, const TByteVec & byteVec);
  
  //----------------------------------------------------------------
  // operator <<
  // 
  extern std::ostream &
  operator << (std::ostream & os, const Bytes & bytes);
  
}


#endif // YAMKA_BYTES_H_
