// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 23:49:04 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaPayload.h>

// system includes:
#include <assert.h>
#include <string.h>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // IPayload::addVoid
  // 
  void
  IPayload::addVoid(uint64 voidPayloadSize)
  {
    TVoid eltVoid;
    eltVoid.payload_.set(voidPayloadSize);
    voids_.push_back(eltVoid);
  }
  
  //----------------------------------------------------------------
  // IPayload::hasVoid
  // 
  bool
  IPayload::hasVoid() const
  {
    return !voids_.empty();
  }
  
  
  //----------------------------------------------------------------
  // VInt::VInt
  // 
  VInt::VInt():
    TSuper()
  {
    setDefault(0);
  }
  
  //----------------------------------------------------------------
  // VInt::eval
  // 
  bool
  VInt::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VInt::isDefault
  // 
  bool
  VInt::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VInt::calcSize
  // 
  uint64
  VInt::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, intNumBytes(TSuper::data_));
  }

  //----------------------------------------------------------------
  // VInt::save
  // 
  IStorage::IReceiptPtr
  VInt::save(IStorage & storage) const
  {
    unsigned char bytes[8];
    unsigned int numBytes = intEncode(TSuper::data_, bytes);
    return storage.save(bytes, numBytes);
  }
  
  //----------------------------------------------------------------
  // VInt::load
  // 
  uint64
  VInt::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    assert(bytesToRead <= 8);
    unsigned char bytes[8];
    if (!storage.load(bytes, (std::size_t)bytesToRead))
    {
      return 0;
    }
    
    TSuper::data_ = intDecode(bytes, bytesToRead);
    return bytesToRead;
  }


  //----------------------------------------------------------------
  // VUInt::VUInt
  // 
  VUInt::VUInt():
    TSuper()
  {
    setDefault(0);
  }
  
  //----------------------------------------------------------------
  // VUInt::eval
  // 
  bool
  VUInt::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VUInt::isDefault
  // 
  bool
  VUInt::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VUInt::calcSize
  // 
  uint64
  VUInt::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, uintNumBytes(TSuper::data_));
  }

  //----------------------------------------------------------------
  // VUInt::save
  // 
  IStorage::IReceiptPtr
  VUInt::save(IStorage & storage) const
  {
    unsigned char bytes[8];
    unsigned int numBytes = uintEncode(TSuper::data_, bytes);
    return storage.save(bytes, numBytes);
  }
  
  //----------------------------------------------------------------
  // VUInt::load
  // 
  uint64
  VUInt::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    assert(bytesToRead <= 8);
    unsigned char bytes[8];
    if (!storage.load(bytes, (std::size_t)bytesToRead))
    {
      return 0;
    }
    
    TSuper::data_ = uintDecode(bytes, bytesToRead);
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // VFloat::VFloat
  // 
  VFloat::VFloat():
    TSuper(4)
  {
    setDefault(0.0);
  }
  
  //----------------------------------------------------------------
  // VFloat::eval
  // 
  bool
  VFloat::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VFloat::isDefault
  // 
  bool
  VFloat::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VFloat::calcSize
  // 
  uint64
  VFloat::calcSize() const
  {
    // only 32-bit floats and 64-bit doubles are allowed:
    return (TSuper::size_ > 4) ? 8 : 4;
  }
  
  //----------------------------------------------------------------
  // VFloat::save
  // 
  IStorage::IReceiptPtr
  VFloat::save(IStorage & storage) const
  {
    uint64 size = calcSize();
    unsigned char bytes[8];
    
    if (size == 4)
    {
      floatEncode(float(TSuper::data_), bytes);
    }
    else
    {
      doubleEncode(TSuper::data_, bytes);
    }
    
    return storage.save(bytes, (std::size_t)size);
  }
  
  //----------------------------------------------------------------
  // VFloat::load
  // 
  uint64
  VFloat::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    assert(bytesToRead <= 8);
    unsigned char bytes[8];
    
    if (!storage.load(bytes, (std::size_t)bytesToRead))
    {
      return 0;
    }
    
    if (bytesToRead > 4)
    {
      TSuper::data_ = doubleDecode(bytes);
      TSuper::size_ = 8;
    }
    else
    {
      TSuper::data_ = double(floatDecode(bytes));
      TSuper::size_ = 4;
    }
    
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // VDate::VDate
  // 
  VDate::VDate():
    TSuper(8)
  {
    setDefault(0);
    
    std::time_t currentTime = std::time(NULL);
    setTime(currentTime);
  }
  
  //----------------------------------------------------------------
  // VDate::eval
  // 
  bool
  VDate::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VDate::isDefault
  // 
  bool
  VDate::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VDate::setTime
  // 
  void
  VDate::setTime(std::time_t t)
  {
    TSuper::data_ = int64(t - kDateMilleniumUTC) * 1000000000;
  }
  
  //----------------------------------------------------------------
  // getTime
  // 
  std::time_t
  VDate::getTime() const
  {
    std::time_t t = kDateMilleniumUTC + TSuper::data_ / 1000000000;
    return t;
  }
  
  //----------------------------------------------------------------
  // TSuper::calcSize
  // 
  uint64
  VDate::calcSize() const
  {
    return 8;
  }
  
  //----------------------------------------------------------------
  // VDate::save
  // 
  IStorage::IReceiptPtr
  VDate::save(IStorage & storage) const
  {
    uint64 size = calcSize();
    
    unsigned char bytes[8];
    intEncode(TSuper::data_, bytes, size);
    
    return storage.save(bytes, (std::size_t)size);
  }
  
  //----------------------------------------------------------------
  // VDate::load
  // 
  uint64
  VDate::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    assert(bytesToRead <= 8);
    unsigned char bytes[8];
    
    if (!storage.load(bytes, (std::size_t)bytesToRead))
    {
      return 0;
    }
    
    TSuper::data_ = intDecode(bytes, bytesToRead);
    return bytesToRead;
  }
  
  
  //----------------------------------------------------------------
  // VString::eval
  // 
  bool
  VString::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VString::isDefault
  // 
  bool
  VString::isDefault() const
  {
    bool allDefault =
      TSuper::data_ == TSuper::dataDefault_;
    
    return allDefault;
  }
  
  //----------------------------------------------------------------
  // VString::calcSize
  // 
  uint64
  VString::calcSize() const
  {
    return std::max<uint64>(TSuper::size_, TSuper::data_.size());
  }

  //----------------------------------------------------------------
  // VString::save
  // 
  IStorage::IReceiptPtr
  VString::save(IStorage & storage) const
  {
    const unsigned char * text = (const unsigned char *)TSuper::data_.data();
    std::size_t size = TSuper::data_.size();
    
    return storage.save(text, size);
  }
  
  //----------------------------------------------------------------
  // VString::load
  // 
  uint64
  VString::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    if (bytesToRead)
    {
      TByteVec chars((std::size_t)bytesToRead);
      if (!Yamka::load(storage, chars))
      {
        return 0;
      }
      
      const char * text = (const char *)&chars[0];
      std::size_t size = chars.size();
      TSuper::data_.assign(text, text + size);
    }
    else
    {
      TSuper::data_ = std::string();
    }
    
    return bytesToRead;
  }
  
  //----------------------------------------------------------------
  // VString::set
  // 
  VString &
  VString::set(const std::string & str)
  {
    TSuper::set(str);
    return *this;
  }
  
  //----------------------------------------------------------------
  // VString::set
  // 
  VString &
  VString::set(const char * cstr)
  {
    std::string str;
    if (cstr)
    {
      str = std::string(cstr);
    }
    
    TSuper::set(str);
    return *this;
  }
  
  
  //----------------------------------------------------------------
  // VVoid::VVoid
  // 
  VVoid::VVoid():
    size_(0)
  {}
  
  //----------------------------------------------------------------
  // VVoid::set
  // 
  VVoid &
  VVoid::set(uint64 size)
  {
    size_ = size;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VVoid::get
  // 
  uint64
  VVoid::get() const
  {
    return size_;
  }
  
  //----------------------------------------------------------------
  // VVoid::eval
  // 
  bool
  VVoid::eval(Yamka::IElementCrawler & crawler)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VVoid::isDefault
  // 
  bool
  VVoid::isDefault() const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VVoid::calcSize
  // 
  uint64
  VVoid::calcSize() const
  {
    return size_;
  }
  
  //----------------------------------------------------------------
  // VVoid::save
  // 
  IStorage::IReceiptPtr
  VVoid::save(Yamka::IStorage & storage) const
  {
    unsigned char zeros[1024] = { 0 };
    
    IStorage::IReceiptPtr receipt = storage.receipt();
    uint64 numSteps = size_ / 1024;
    for (uint64 i = 0; i < numSteps; i++)
    {
      storage.save(zeros, 1024);
      receipt->add(1024);
    }
    
    std::size_t remainder = (std::size_t)(size_ % 1024);
    if (remainder)
    {
      storage.save(&zeros[1024 - remainder], remainder);
      receipt->add(remainder);
    }
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // VVoid::load
  // 
  uint64
  VVoid::load(Yamka::FileStorage & storage,
              uint64 bytesToRead,
              IDelegateLoad *)
  {
    // Void payload is ignored, just skip over it:
    if (storage.file_.seek(bytesToRead, File::kRelativeToCurrent))
    {
      size_ = bytesToRead;
      return bytesToRead;
    }
    
    return 0;
  }
  
  
  //----------------------------------------------------------------
  // VBinary::VBinary
  // 
  VBinary::VBinary()
  {}
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const unsigned char * b, std::size_t nb, IStorage & storage)
  {
    data_.set(b, nb, storage);
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const TByteVec & bytes, IStorage & storage)
  {
    data_.set(bytes, storage);
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const IStorage::IReceiptPtr & dataReceipt)
  {
    data_.set(dataReceipt);
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::get
  // 
  bool
  VBinary::get(TByteVec & bytes) const
  {
    return data_.get(bytes);
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const unsigned char * b, std::size_t nb, IStorage & s)
  {
    dataDefault_.set(b, nb, s);
    data_ = dataDefault_;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const TByteVec & bytes, IStorage & storage)
  {
    dataDefault_.set(bytes, storage);
    data_ = dataDefault_;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const IStorage::IReceiptPtr & dataReceipt)
  {
    dataDefault_.set(dataReceipt);
    data_ = dataDefault_;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::eval
  // 
  bool
  VBinary::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VBinary::isDefault
  // 
  bool
  VBinary::isDefault() const
  {
    return data_ == dataDefault_;
  }
  
  //----------------------------------------------------------------
  // VBinary::calcSize
  // 
  uint64
  VBinary::calcSize() const
  {
    return data_.numBytes();
  }
  
  //----------------------------------------------------------------
  // VBinary::save
  // 
  IStorage::IReceiptPtr
  VBinary::save(IStorage & storage) const
  {
    return data_.save(storage);
  }
  
  //----------------------------------------------------------------
  // VBinary::load
  // 
  uint64
  VBinary::load(FileStorage & storage, uint64 bytesToRead, IDelegateLoad *)
  {
    return data_.load(storage, bytesToRead);
  }
  
  
  //----------------------------------------------------------------
  // VEltPosition::VEltPosition
  // 
  VEltPosition::VEltPosition():
    origin_(NULL),
    elt_(NULL),
    pos_(uintMax[8]),
    maxSize_(8)
  {}
  
  //----------------------------------------------------------------
  // VEltPosition::setMaxSize
  // 
  void
  VEltPosition::setMaxSize(uint64 vsize)
  {
    if (vsize > 8)
    {
      assert(false);
      vsize = 8;
    }
    
    if (pos_ == uintMax[maxSize_])
    {
      // adjust the unknown position:
      pos_ = uintMax[vsize];
    }
    
    maxSize_ = vsize;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::eval
  // 
  bool
  VEltPosition::eval(IElementCrawler &)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::isDefault
  // 
  bool
  VEltPosition::isDefault() const
  {
    // no point is saving an unresolved invalid reference:
    return !elt_ && pos_ == uintMax[maxSize_];
  }
  
  //----------------------------------------------------------------
  // VEltPosition::calcSize
  // 
  uint64
  VEltPosition::calcSize() const
  {
    if (!elt_)
    {
      return maxSize_;
    }
    
    IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
    if (!eltReceipt)
    {
      return maxSize_;
    }
    
    if (receipt_)
    {
      // must use the same number of bytes as before:
      return receipt_->numBytes();
    }
    
    // NOTE:
    // 1. The origin position may not be known when this is called,
    // 2. We can assume that the relative position will not require
    //    any more bytes than the absolute position would.
    // 3. We'll use the absolute position to calculate the required
    //    number of bytes
    // 
    uint64 absolutePosition = eltReceipt->position();
    uint64 bytesNeeded = uintNumBytes(absolutePosition);
    return bytesNeeded;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::save
  // 
  IStorage::IReceiptPtr
  VEltPosition::save(IStorage & storage) const
  {
    uint64 bytesNeeded = calcSize();
    uint64 eltPosition = pos_;
    uint64 originPosition = getOriginPosition();
    
    if (elt_)
    {
      IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
      if (eltReceipt)
      {
        eltPosition = eltReceipt->position();
      }
    }
    
    // let VUInt do the rest:
    uint64 relativePosition = eltPosition - originPosition;
    VUInt data;
    data.setSize(bytesNeeded);
    data.set(relativePosition);
    
    receipt_ = data.save(storage);
    return receipt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::load
  // 
  uint64
  VEltPosition::load(FileStorage & storage,
                     uint64 bytesToRead,
                     IDelegateLoad *)
  {
    VUInt data;
    uint64 bytesRead = data.load(storage, bytesToRead);
    if (bytesRead)
    {
      origin_ = NULL;
      elt_ = NULL;
      pos_ = data.get();
    }
    
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::setOrigin
  // 
  void
  VEltPosition::setOrigin(const IElement * origin)
  {
    origin_ = origin;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getOrigin
  // 
  const IElement *
  VEltPosition::getOrigin() const
  {
    return origin_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::setElt
  // 
  void
  VEltPosition::setElt(const IElement * elt)
  {
    elt_ = elt;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getElt
  // 
  const IElement *
  VEltPosition::getElt() const
  {
    return elt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::setPosition
  // 
  void
  VEltPosition::setPosition(uint64 position)
  {
    origin_ = NULL;
    elt_ = NULL;
    pos_ = position;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::hasPosition
  // 
  bool
  VEltPosition::hasPosition() const
  {
    return pos_ < uintMax[maxSize_];
  }
  
  //----------------------------------------------------------------
  // VEltPosition::position
  // 
  uint64
  VEltPosition::position() const
  {
    return pos_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::rewrite
  // 
  bool
  VEltPosition::rewrite() const
  {
    if (!receipt_)
    {
      return false;
    }
    
    if (!elt_)
    {
      return false;
    }
    
    IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
    if (!eltReceipt)
    {
      return false;
    }
    
    uint64 originPosition = getOriginPosition();
    uint64 eltPosition = eltReceipt->position();
    uint64 relativePosition = eltPosition - originPosition;
    
    uint64 bytesNeeded = uintNumBytes(relativePosition);
    uint64 bytesUsed = receipt_->numBytes();
    
    if (bytesNeeded > bytesUsed)
    {
      // must use the same size as before:
      return false;
    }
    
    unsigned char bytes[8];
    uintEncode(relativePosition, bytes, bytesUsed);
    
    bool saved = receipt_->save(bytes, (std::size_t)bytesUsed);
    return saved;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::discardReceipt
  // 
  void
  VEltPosition::discardReceipt()
  {
    receipt_ = IStorage::IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // VEltPosition::getOriginPosition
  // 
  uint64
  VEltPosition::getOriginPosition() const
  {
    if (!origin_)
    {
      return 0;
    }
    
    IStorage::IReceiptPtr originReceipt = origin_->payloadReceipt();
    if (!originReceipt)
    {
      return 0;
    }
    
    // get the payload position:
    uint64 originPosition = originReceipt->position();
    return originPosition;
  }
  
}
