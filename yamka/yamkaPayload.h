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
#include <yamkaElt.h>

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
  // IPayload
  // 
  // EBML element payload interface
  // 
  struct IPayload
  {
    virtual ~IPayload() {}
    
    // check whether payload holds default value:
    virtual bool isDefault() const = 0;
    
    // calculate payload size:
    virtual uint64 calcSize() const = 0;
    
    // save the payload and return storage receipt:
    virtual IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc32 = NULL) const = 0;
    
    // attempt to load the payload, return number of bytes read successfully:
    virtual uint64
    load(FileStorage & storage, uint64 storageSize, Crc32 * crc32 = NULL) = 0;
  };
  
  
  //----------------------------------------------------------------
  // ImplementsPayloadAPI
  // 
  // A helper macro used to implement the payload interface API
  // 
# define ImplementsPayloadAPI()                                         \
  bool isDefault() const;                                               \
  uint64 calcSize() const;                                              \
  IStorage::IReceiptPtr save(IStorage & storage,                        \
                             Crc32 * computeCrc32 = NULL) const;        \
  uint64 load(FileStorage & storage,                                    \
              uint64 storageSize,                                       \
              Crc32 * computeCrc32 = NULL)
  

  //----------------------------------------------------------------
  // vsizeNumBytes
  // 
  // Returns the number of bytes used to store a payload size.
  // 
  // NOTE: This requires loading one byte.
  // 
  extern uint64
  vsizeNumBytes(IStorage::IReceiptPtr payloadReceipt);
  
  
  //----------------------------------------------------------------
  // VEltPosition
  // 
  // NOTE: when this element is saved the reference element
  // storage receipt position is used.  If the
  // referenced element doesn't have a storage receipt, then
  // a default position (max uint64 for a given vsize) will be saved.
  // Once all elements are saved this position will have
  // to be corrected (saved again with correct storage receipt):
  // 
  // NOTE: once this element and all other elements are loaded,
  // a second pass is required to map loaded storage receipt
  // position to an actual element pointer.
  // 
  struct VEltPosition : public IPayload
  {
    VEltPosition();
    
    ImplementsPayloadAPI();
    
    // set default number of bytes to use when exact size of
    // the position is not known yet. The default is 8 bytes
    void setUnknownPositionSize(uint64 vsize);
    
    // origin element accessors (position is relative to this element):
    void setOrigin(const IElement * origin);
    const IElement * getOrigin() const;
    
    // reference element accessors:
    void setElt(const IElement * elt);
    const IElement * getElt() const;
    
    // accessor to the reference element storage receipt position:
    uint64 position() const;
    
    // Rewrite reference element storage receipt position.
    // 
    // 1. This will fail if the position hasn't been saved yet.
    // 2. This will fail if the reference element is not set.
    // 3. This will fail if the new position doesn't fit within the
    //    number of bytes used to save the position previously.
    // 4. The size of the previously saved position will be preserved.
    // 
    bool rewrite() const;
    
  protected:
    // helper for getting the origin position:
    virtual uint64 getOriginPosition() const;
    
    // origin reference element:
    const IElement * origin_;
    
    // reference element:
    const IElement * elt_;
    
    // reference element storage receipt position:
    uint64 pos_;
    
    // number of bytes used to save unknown position:
    uint64 unknownPositionSize_;
    
    // position storage receipt is kept so that referenced element
    // storage receipt position can be rewritten once it is known:
    mutable IStorage::IReceiptPtr receipt_;
  };
  
  
  //----------------------------------------------------------------
  // VPayload
  // 
  // Helper base class for simple payload types
  // such as int, float, date, string...
  // 
  template <typename data_t>
  struct VPayload : public IPayload
  {
    typedef data_t TData;
    typedef VPayload<TData> TSelf;
    
    VPayload(uint64 size = 0):
      size_(size)
    {}
    
    // set payload size:
    TSelf & setSize(uint64 size)
    {
      size_ = size;
      return *this;
    }
    
    // set default value and payload data:
    TSelf & setDefault(const TData & dataDefault)
    {
      dataDefault_ = dataDefault;
      data_ = dataDefault_;
      return *this;
    }
    
    // set payload data:
    TSelf & set(const TData & data)
    {
      data_ = data;
      return *this;
    }
    
    // payload data accessor:
    const TData & get() const
    { return data_; }
    
  protected:
    uint64 size_;
    
    TData dataDefault_;
    TData data_;
  };
  
  
  //----------------------------------------------------------------
  // VInt
  // 
  struct VInt : public VPayload<int64>
  {
    typedef VPayload<int64> TSuper;
    
    VInt();
    
    ImplementsPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VUInt
  // 
  struct VUInt : public VPayload<uint64>
  {
    typedef VPayload<uint64> TSuper;
    
    VUInt();
    
    ImplementsPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VFloat
  // 
  struct VFloat : public VPayload<double>
  {
    typedef VPayload<double> TSuper;
    
    VFloat();
    
    ImplementsPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VDate
  // 
  // 64-bit signed integer expressing elapsed time
  // in nanoseconds since 2001-01-01 T00:00:00,000000000 UTC
  // 
  struct VDate : public VPayload<int64>
  {
    typedef VPayload<int64> TSuper;
    
    // constructor stores current time:
    VDate();
    
    void setTime(std::time_t t);
    std::time_t getTime() const;
    
    ImplementsPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VString
  // 
  // ASCII or UTF-8 encoded string (ASCII is a subset of UTF-8)
  // 
  struct VString : public VPayload<std::string>
  {
    typedef VPayload<std::string> TSuper;
    
    ImplementsPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VBinary
  // 
  // Binary data is stored and accessed via an abstract storage
  // interface. This allows a simple implementation for off-line
  // storage of binary blobs (which may be fairly large):
  //
  struct VBinary : public IPayload
  {
    typedef Bytes TData;
    
    VBinary();
    
    VBinary & setStorage(const IStoragePtr & binStorage);
    VBinary & setDefault(const Bytes & bytes);
    VBinary & set(const Bytes & bytes);
    bool get(Bytes & bytes) const;
    
    ImplementsPayloadAPI();
    
    // data storage:
    static IStoragePtr defaultStorage_;
    IStoragePtr storage_;
    IStorage::IReceiptPtr receipt_;
    IStorage::IReceiptPtr receiptDefault_;
    std::size_t size_;
    std::size_t sizeDefault_;
  };
  
  
  //----------------------------------------------------------------
  // VBytes
  // 
  template <unsigned int fixedSize>
  struct VBytes : public IPayload
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
    load(FileStorage & storage, uint64 storageSize, Crc32 * crc = NULL)
    {
      uint64 vsizeSize = 0;
      uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
      if (numBytes != fixedSize)
      {
        return 0;
      }
      
      Bytes bytes(fixedSize);
      if (storage.loadAndCalcCrc32(bytes, crc))
      {
        data_ = bytes;
      };
      
      uint64 bytesRead = vsizeSize + fixedSize;
      return bytesRead;
    }
    
    // data storage:
    Bytes data_;
  };
  
}


#endif // YAMKA_PAYLOAD_H_
