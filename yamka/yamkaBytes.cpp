// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 17:24:08 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaBytes.h>

// boost includes:
#include <boost/shared_ptr.hpp>

// system includes:
#include <iomanip>
#include <assert.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // Bytes::Bytes
  // 
  Bytes::Bytes(std::size_t size):
    bytes_(new TByteVecDec(1, TByteVec(size)))
  {}
  
  //----------------------------------------------------------------
  // Bytes::Bytes
  // 
  Bytes::Bytes(const void * data, std::size_t size):
    bytes_(new TByteVecDec(1, TByteVec((const unsigned char *)data,
                                       (const unsigned char *)data + size)))
  {}
  
  //----------------------------------------------------------------
  // Bytes::Bytes
  // 
  Bytes::Bytes(const TByteVec & byteVec):
    bytes_(new TByteVecDec(1, byteVec))
  {}
  
  //----------------------------------------------------------------
  // Bytes::operator TByteVec
  // 
  Bytes::operator TByteVec() const
  {
    TByteVec dstVec;
    dstVec.reserve(size());
    
    const TByteVecDec & deq = *bytes_;
    for (TByteVecDec::const_iterator i = deq.begin(); i != deq.end(); ++i)
    {
      const TByteVec & vec = *i;
      dstVec.insert(dstVec.end(), vec.begin(), vec.end());
    }
    
    return dstVec;
  }
  
  //----------------------------------------------------------------
  // Bytes::deepCopy
  // 
  Bytes &
  Bytes::deepCopy(const Bytes & bytes)
  {
    TByteVecDec & deq = *bytes_;
    deq.resize(1);
    
    TByteVec & vec = deq.back();
    vec = TByteVec(bytes);
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // Bytes::operator +=
  // 
  // NOTE: this is a deep copy of data (not shared with source):
  Bytes &
  Bytes::operator += (const Bytes & from)
  {
    const TByteVecDec & src = *(from.bytes_);
    TByteVecDec & dst = *bytes_;
    
    dst.insert(dst.end(),
               src.begin(),
               src.end());
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // Bytes::operator <<
  // 
  Bytes &
  Bytes::operator << (const TByteVec & bytes)
  {
    TByteVecDec & deq = *bytes_;
    deq.push_back(bytes);
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // Bytes::operator <<
  // 
  Bytes &
  Bytes::operator << (const TByte & byte)
  {
    TByteVecDec & deq = *bytes_;
    deq.back().push_back(byte);
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // Bytes::operator <<
  // 
  Bytes &
  Bytes::operator << (const std::string & str)
  {
    TByteVec vec(str.data(), str.data() + str.size());
    return (*this) << vec;
  }
  
  //----------------------------------------------------------------
  // Bytes::empty
  // 
  bool
  Bytes::empty() const
  {
    const TByteVecDec & deq = *bytes_;
    for (TByteVecDec::const_iterator i = deq.begin(); i != deq.end(); ++i)
    {
      const TByteVec & byteVec = *i;
      if (!byteVec.empty())
      {
        return false;
      }
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // Bytes::size
  // 
  std::size_t
  Bytes::size() const
  {
    std::size_t total = 0;
    
    const TByteVecDec & deq = *bytes_;
    for (TByteVecDec::const_iterator i = deq.begin(); i != deq.end(); ++i)
    {
      const TByteVec & byteVec = *i;
      total += byteVec.size();
    }
    
    return total;
  }
  
  //----------------------------------------------------------------
  // Bytes::getByte
  // 
  const TByte &
  Bytes::getByte(std::size_t i) const
  {
    const TByteVecDec & deq = *bytes_;
    for (TByteVecDec::const_iterator j = deq.begin(); j != deq.end(); ++j)
    {
      const TByteVec & byteVec = *j;
      const std::size_t byteVecSize = byteVec.size();
      
      if (i < byteVecSize)
      {
        return byteVec[i];
      }
      
      i -= byteVecSize;
    }
    
    assert(false);
    return *(TByte *)NULL;
  }
  
  //----------------------------------------------------------------
  // operator +
  // 
  Bytes
  operator + (const Bytes & a, const Bytes & b)
  {
    Bytes ab(a);
    ab += b;
    return ab;
  }
  
  //----------------------------------------------------------------
  // operator <<
  // 
  std::ostream &
  operator << (std::ostream & os, const TByteVec & vec)
  {
    os << std::hex
       << std::uppercase;
    
    for (TByteVec::const_iterator j = vec.begin(); j != vec.end(); ++j)
    {
      const TByte & byte = *j;
      os << std::setw(2)
         << std::setfill('0')
         << int(byte);
    }
    
    os << std::dec;
    return os;
  }
  
  //----------------------------------------------------------------
  // operator <<
  // 
  std::ostream &
  operator << (std::ostream & os, const Bytes & bytes)
  {
    const TByteVecDec & deq = *(bytes.bytes_);
    for (TByteVecDec::const_iterator i = deq.begin(); i != deq.end(); ++i)
    {
      const TByteVec & vec = *i;
      os << vec;
    }
    return os;
  }
  
}
