// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 13:01:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_PAYLOAD_H_
#define YAMKA_PAYLOAD_H_

// yamka includes:
#include <yamkaIStorage.h>
#include <yamkaStdInt.h>
#include <yamkaCrc32.h>
#include <yamkaBytes.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <algorithm>
#include <string>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // Payload
  // 
  template <typename data_t>
  struct Payload
  {
    typedef data_t TData;
    typedef Payload<TData> TSelf;
    
    Payload(uint64 size = 0):
      size_(size)
    {}
    
    ~Payload() {}
    
    TSelf & setDefault(const TData & dataDefault)
    {
      dataDefault_ = dataDefault;
      data_ = dataDefault_;
      return *this;
    }
    
    TSelf & set(const TData & data)
    {
      data_ = data;
      return *this;
    }
    
    bool isDefault() const
    { return data_ == dataDefault_; }
    
    TData dataDefault_;
    TData data_;
    uint64 size_;
  };
  
  //----------------------------------------------------------------
  // VInt
  // 
  struct VInt : public Payload<int64>
  {
    typedef Payload<int64> TSuper;
    
    VInt();
    
    uint64 calcSize() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
  };
  
  //----------------------------------------------------------------
  // VUInt
  // 
  struct VUInt : public Payload<uint64>
  {
    typedef Payload<uint64> TSuper;
    
    VUInt();
    
    uint64 calcSize() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
  };

  //----------------------------------------------------------------
  // VString
  // 
  // ASCII or UTF-8 encoded string (ASCII is a subset of UTF-8)
  // 
  struct VString : public Payload<std::string>
  {
    typedef Payload<std::string> TSuper;
    
    uint64 calcSize() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
  };
  
  //----------------------------------------------------------------
  // VBinary
  // 
  // Binary data is stored and accessed via an abstract storage
  // interface. This allows a simple implementation for off-line
  // storage of binary blobs (which may be fairly large):
  //
  struct VBinary
  {
    VBinary();
    
    VBinary & setStorage(const IStoragePtr & binStorage);
    VBinary & set(const Bytes & bytes);
    
    uint64 calcSize() const;
    bool isDefault() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
    
    // data storage:
    IStoragePtr binStorage_;
    IStorage::IReceiptPtr binReceipt_;
    std::size_t binSize_;
    
    // IO storage:
    mutable IStorage::IReceiptPtr receipt_;
  };
  
}


#endif // YAMKA_PAYLOAD_H_
