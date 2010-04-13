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
#include <ctime>
#include <assert.h>


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
    
    // check whether payload holds default value:
    bool isDefault() const
    { return data_ == dataDefault_; }

    // set default value and payload value:
    TSelf & setDefault(const TData & dataDefault)
    {
      dataDefault_ = dataDefault;
      data_ = dataDefault_;
      return *this;
    }

    // set payload value
    TSelf & set(const TData & data)
    {
      data_ = data;
      return *this;
    }
    
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
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
  };
  
  //----------------------------------------------------------------
  // VUInt
  // 
  struct VUInt : public Payload<uint64>
  {
    typedef Payload<uint64> TSuper;
    
    VUInt();
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
  };
  
  //----------------------------------------------------------------
  // VFloat
  // 
  struct VFloat : public Payload<double>
  {
    typedef Payload<double> TSuper;

    VFloat();
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
  };

  //----------------------------------------------------------------
  // VDate
  // 
  // 64-bit signed integer expressing elapsed time
  // in nanoseconds since 2001-01-01 T00:00:00,000000000 UTC
  // 
  struct VDate : public Payload<int64>
  {
    typedef Payload<int64> TSuper;
    
    // constructor stores current time:
    VDate();

    void setTime(std::time_t t);
    std::time_t getTime() const;
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
  };
  
  //----------------------------------------------------------------
  // VString
  // 
  // ASCII or UTF-8 encoded string (ASCII is a subset of UTF-8)
  // 
  struct VString : public Payload<std::string>
  {
    typedef Payload<std::string> TSuper;
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
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
    typedef Bytes TData;
    
    VBinary();
    
    VBinary & setStorage(const IStoragePtr & binStorage);
    VBinary & setDefault(const Bytes & bytes);
    VBinary & set(const Bytes & bytes);
    
    // check whether payload holds default value:
    bool isDefault() const;
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL);
    
    // data storage:
    static IStoragePtr defaultStorage_;
    IStoragePtr binStorage_;
    IStorage::IReceiptPtr binReceipt_;
    IStorage::IReceiptPtr binReceiptDefault_;
    std::size_t binSize_;
    std::size_t binSizeDefault_;
    
    // IO storage:
    mutable IStorage::IReceiptPtr receipt_;
  };
  
  //----------------------------------------------------------------
  // VBytes
  // 
  template <unsigned int fixedSize>
  struct VBytes
  {
    typedef VBytes<fixedSize> TSelf;
    typedef Bytes TData;
    
    TSelf & set(const Bytes & bytes)
    {
      if (bytes.size() == fixedSize)
      {
        data_.deepCopy(bytes);
      }
      else
      {
        assert(false);
      }
      
      return *this;
    }
    
    // check whether payload holds default value:
    bool isDefault() const
    { return data_.empty(); }
    
    // calculate payload size:
    uint64 calcSize() const
    { return fixedSize; }
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc = NULL) const
    {
      uint64 size = calcSize();
      
      Bytes bytes;
      bytes << vsizeEncode(size);
      bytes += data_;
      
      return storage.saveAndCalcCrc32(bytes, crc);
    }
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(IStorage & storage, uint64 storageSize, Crc32 * crc = NULL)
    {
      // FIXME:
      return 0;
    }
    
    // data storage:
    Bytes data_;
  };
  
}


#endif // YAMKA_PAYLOAD_H_
