// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 13:01:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_PAYLOAD_H_
#define YAMKA_PAYLOAD_H_

// yamka includes:
#include <yamkaFileStorage.h>
#include <yamkaHodgePodge.h>
#include <yamkaIStorage.h>
#include <yamkaStdInt.h>
#include <yamkaBytes.h>
#include <yamkaElt.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <algorithm>
#include <string>
#include <ctime>
#include <assert.h>
#include <list>


namespace Yamka
{
  
  // forward declarations:
  struct IElementCrawler;
  struct VVoid;
  
  //----------------------------------------------------------------
  // IPayload
  // 
  // EBML element payload interface
  // 
  struct IPayload
  {
    virtual ~IPayload() {}
    
    // perform crawler computation on this payload:
    virtual bool eval(IElementCrawler & crawler) = 0;
    
    // check whether payload holds default value:
    virtual bool isDefault() const = 0;
    
    // calculate payload size:
    virtual uint64 calcSize() const = 0;
    
    // save the payload and return storage receipt:
    virtual IStorage::IReceiptPtr
    save(IStorage & storage) const = 0;
    
    // attempt to load the payload, return number of bytes read successfully:
    virtual uint64
    load(FileStorage & storage,
         uint64 bytesToRead,
         IDelegateLoad * delegateLoader = NULL) = 0;
    
    // return true if this payload is a composite of one or more
    // EBML elements (such as an EBML Master Element payload)
    // 
    // NOTE: non-composite payload must not contain CRC-32 elements
    virtual bool isComposite() const = 0;
    
    // attach to this payload a Void element with given payload size:
    virtual void addVoid(uint64 voidPayloadSize);
    
    // return true if this element payload has a Void element attached to it:
    virtual bool hasVoid() const;
    
    TypedefYamkaElt(VVoid, kIdVoid, "Void") TVoid;
    std::list<TVoid> voids_;
  };
  
  //----------------------------------------------------------------
  // IElementCrawler
  // 
  // Interface for an element tree crawling functor.
  // 
  struct IElementCrawler
  {
    virtual ~IElementCrawler() {}
    
    // NOTE: the crawler should return true when it's done
    // in order to stop:
    virtual bool eval(IElement & elt)
    {
      bool done = evalPayload(elt.getPayload());
      return done;
    }
    
    // NOTE: the crawler should return true when it's done
    // in order to stop:
    virtual bool evalPayload(IPayload & payload)
    {
      bool done = payload.eval(*this);
      return done;
    }
  };
  
  //----------------------------------------------------------------
  // ImplementsYamkaPayloadAPI
  // 
  // A helper macro used to implement the payload interface API
  // 
# define ImplementsYamkaPayloadAPI()                                    \
  bool eval(Yamka::IElementCrawler & crawler);                          \
  bool isDefault() const;                                               \
  Yamka::uint64 calcSize() const;                                       \
  Yamka::IStorage::IReceiptPtr save(Yamka::IStorage & storage) const;   \
  Yamka::uint64 load(Yamka::FileStorage & storage,                      \
                     Yamka::uint64 bytesToRead,                         \
                     Yamka::IDelegateLoad * loader = NULL)
  
  
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
    
    // virtual:
    bool isComposite() const
    { return false; }
    
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
    
    const TData & getDefault() const
    { return dataDefault_; }
    
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
    
    ImplementsYamkaPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VUInt
  // 
  struct VUInt : public VPayload<uint64>
  {
    typedef VPayload<uint64> TSuper;
    
    VUInt();
    
    ImplementsYamkaPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VFloat
  // 
  struct VFloat : public VPayload<double>
  {
    typedef VPayload<double> TSuper;
    
    VFloat();
    
    ImplementsYamkaPayloadAPI();
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
    
    ImplementsYamkaPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VString
  // 
  // ASCII or UTF-8 encoded string (ASCII is a subset of UTF-8)
  // 
  struct VString : public VPayload<std::string>
  {
    typedef VPayload<std::string> TSuper;
    
    VString & set(const std::string & str);
    VString & set(const char * cstr);
    
    ImplementsYamkaPayloadAPI();
  };
  
  
  //----------------------------------------------------------------
  // VVoid
  // 
  // Void data is ignored
  //
  struct VVoid : public IPayload
  {
    VVoid();
    
    VVoid & set(uint64 size);
    uint64 get() const;
    
    ImplementsYamkaPayloadAPI();
    
    // virtual:
    bool isComposite() const
    { return false; }
    
    uint64 size_;
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
    VBinary();
    
    VBinary & setDefault(const unsigned char * b, std::size_t n, IStorage & s);
    VBinary & setDefault(const TByteVec & bytes, IStorage & storage);
    VBinary & setDefault(const IStorage::IReceiptPtr & dataReceipt);
    
    VBinary & set(const unsigned char * b, std::size_t n, IStorage & storage);
    VBinary & set(const TByteVec & bytes, IStorage & storage);
    VBinary & set(const IStorage::IReceiptPtr & dataReceipt);
    
    bool get(TByteVec & bytes) const;
    
    ImplementsYamkaPayloadAPI();
    
    // virtual:
    bool isComposite() const
    { return false; }
    
    // data storage:
    HodgePodge dataDefault_;
    HodgePodge data_;
  };
  
  
  //----------------------------------------------------------------
  // VBytes
  // 
  template <unsigned int fixedSize>
  struct VBytes : public IPayload
  {
    typedef VBytes<fixedSize> TSelf;
    
    TSelf & set(const unsigned char * bytes)
    {
      data_.assign(bytes, bytes + fixedSize);
      return *this;
    }
    
    // virtual:
    bool eval(IElementCrawler &)
    { return false; }
    
    // virtual:
    bool isComposite() const
    { return false; }
    
    // check whether payload holds default value:
    bool isDefault() const
    { return data_.empty(); }
    
    // calculate payload size:
    uint64 calcSize() const
    { return data_.size(); }
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage) const
    {
      return
        data_.empty() ?
        storage.receipt() :
        storage.save(&data_[0], data_.size());
    }
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
    {
      if (bytesToRead != fixedSize)
      {
        return 0;
      }
      
      TByteVec bytes(fixedSize);
      if (!storage.load(&bytes[0], fixedSize))
      {
        return 0;
      }
      
      data_ = bytes;
      return bytesToRead;
    }
    
    // data storage:
    TByteVec data_;
  };
  
  
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
    
    ImplementsYamkaPayloadAPI();
    
    // virtual:
    bool isComposite() const
    { return false; }
    
    // set maximum number of bytes to use when exact size of
    // the position is not known yet. The default is 8 bytes:
    void setMaxSize(uint64 vsize);
    
    // origin element accessors (position is relative to this element):
    void setOrigin(const IElement * origin);
    const IElement * getOrigin() const;
    
    // reference element accessors:
    void setElt(const IElement * elt);
    const IElement * getElt() const;
    
    // accessor to the reference element storage receipt position:
    uint64 position() const;

    // verify that an element position reference has in fact been loaded:
    bool hasPosition() const;
    
    // Rewrite reference element storage receipt position.
    // 
    // 1. This will fail if the position hasn't been saved yet.
    // 2. This will fail if the reference element is not set.
    // 3. This will fail if the new position doesn't fit within the
    //    number of bytes used to save the position previously.
    // 4. The size of the previously saved position will be preserved.
    // 
    bool rewrite() const;
    
    // discard position storage receipt:
    void discardReceipt();
    
  protected:
    // helper for getting the origin position:
    virtual uint64 getOriginPosition() const;
    
    // origin reference element:
    const IElement * origin_;
    
    // reference element:
    const IElement * elt_;
    
    // reference element storage receipt position:
    uint64 pos_;
    
    // max number of bytes used to save position:
    uint64 maxSize_;
    
    // position storage receipt is kept so that referenced element
    // storage receipt position can be rewritten once it is known:
    mutable IStorage::IReceiptPtr receipt_;
  };
  
}


#endif // YAMKA_PAYLOAD_H_
