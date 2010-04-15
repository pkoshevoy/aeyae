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
  // VEltPosition::VEltPosition
  // 
  VEltPosition::VEltPosition():
    elt_(NULL),
    pos_(uintMax[8]),
    unknownPositionSize_(8)
  {}
  
  //----------------------------------------------------------------
  // VEltPosition::setUnknownPositionSize
  // 
  void
  VEltPosition::setUnknownPositionSize(uint64 vsize)
  {
    if (vsize > 8)
    {
      assert(false);
      vsize = 8;
    }
    
    if (pos_ == uintMax[unknownPositionSize_])
    {
      // adjust the unknown position:
      pos_ = uintMax[vsize];
    }
    
    unknownPositionSize_ = vsize;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::isDefault
  // 
  bool
  VEltPosition::isDefault() const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::calcSize
  // 
  uint64
  VEltPosition::calcSize() const
  {
    if (!elt_)
    {
      return unknownPositionSize_;
    }
    
    IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
    if (!eltReceipt)
    {
      return unknownPositionSize_;
    }
    
    if (receipt_)
    {
      // use previous size:
      Bytes savedPosition;
      if (!receipt_->load(savedPosition))
      {
        assert(false);
        return unknownPositionSize_;
      }
      
      uint64 vsizeSize = 0;
      uint64 bytesUsed = vsizeDecode(savedPosition, vsizeSize);
      return bytesUsed;
    }
    
    uint64 storagePosition = eltReceipt->position();
    uint64 bytesNeeded = uintNumBytes(storagePosition);
    return bytesNeeded;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::save
  // 
  IStorage::IReceiptPtr
  VEltPosition::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 numBytes = calcSize();
    uint64 storagePosition = uintMax[numBytes];
    
    if (elt_)
    {
      IStorage::IReceiptPtr eltReceipt = elt_->storageReceipt();
      if (eltReceipt)
      {
        storagePosition = eltReceipt->position();
      }
    }
    
    // let VUInt do the rest:
    VUInt data;
    data.setSize(numBytes);
    data.set(storagePosition);
    
    receipt_ = data.save(storage, crc);
    return receipt_;
  }
  
  //----------------------------------------------------------------
  // VEltPosition::load
  // 
  uint64
  VEltPosition::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    VUInt data;
    uint64 bytesRead = data.load(storage, storageSize, crc);
    if (bytesRead)
    {
      elt_ = NULL;
      pos_ = data.get();
    }
    
    return bytesRead;
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
    
    // must use previous size:
    Bytes savedPosition;
    if (!receipt_->load(savedPosition))
    {
      return false;
    }
    
    uint64 vsizeSize = 0;
    uint64 bytesUsed = vsizeDecode(savedPosition, vsizeSize);
    
    uint64 storagePosition = eltReceipt->position();
    uint64 bytesNeeded = uintNumBytes(storagePosition);
    
    if (bytesUsed < bytesNeeded)
    {
      return false;
    }
    
    Bytes bytes;
    bytes << vsizeEncode(bytesUsed)
          << uintEncode(storagePosition);
    
    bool saved = receipt_->save(bytes);
    return saved;
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
  VInt::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
          << intEncode(TSuper::data_);
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VInt::load
  // 
  uint64
  VInt::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes((std::size_t)numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = intDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeSize + numBytes;
    return bytesRead;
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
  VUInt::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
          << uintEncode(TSuper::data_);
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VUInt::load
  // 
  uint64
  VUInt::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes((std::size_t)numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = uintDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeSize + numBytes;
    return bytesRead;
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
  VFloat::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size);
    
    if (size == 4)
    {
      bytes << floatEncode(float(TSuper::data_));
    }
    else
    {
      bytes << doubleEncode(TSuper::data_);
    }
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VFloat::load
  // 
  uint64
  VFloat::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes((std::size_t)numBytes);
    storage.loadAndCalcCrc32(bytes, crc);
    if (numBytes > 4)
    {
      TSuper::data_ = doubleDecode(bytes);
      TSuper::size_ = 8;
    }
    else
    {
      TSuper::data_ = double(floatDecode(bytes));
      TSuper::size_ = 4;
    }
    
    uint64 bytesRead = vsizeSize + numBytes;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // kVDateMilleniumUTC
  // 
  // 2001/01/01 00:00:00 UTC
  static const std::time_t kVDateMilleniumUTC = 978307200;
  
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
    TSuper::data_ = int64(t - kVDateMilleniumUTC) * 1000000000;
  }
  
  //----------------------------------------------------------------
  // getTime
  // 
  std::time_t
  VDate::getTime() const
  {
    std::time_t t = kVDateMilleniumUTC + TSuper::data_ / 1000000000;
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
  VDate::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
          << intEncode(TSuper::data_, size);
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VDate::load
  // 
  uint64
  VDate::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes((std::size_t)numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = intDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeSize + numBytes;
    return bytesRead;
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
  VString::save(IStorage & storage, Crc32 * crc) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
          << TSuper::data_;
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VString::load
  // 
  uint64
  VString::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 numBytes = vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes((std::size_t)numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TByteVec chars = TByteVec(bytes);
      TSuper::data_.assign((const char *)&chars[0], chars.size());
    }
    
    uint64 bytesRead = vsizeSize + numBytes;
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // VBinary::defaultStorage_
  // 
  IStoragePtr
  VBinary::defaultStorage_;
  
  //----------------------------------------------------------------
  // VBinary::VBinary
  // 
  VBinary::VBinary():
    storage_(defaultStorage_),
    size_(0),
    sizeDefault_(0)
  {}
  
  //----------------------------------------------------------------
  // VBinary::setStorage
  // 
  VBinary &
  VBinary::setStorage(const IStoragePtr & storage)
  {
    storage_ = storage;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const Bytes & bytes)
  {
    if (storage_)
    {
      size_ = bytes.size();
      receipt_ = storage_->save(bytes);
    }
    else
    {
      assert(false);
    }
    
    return *this;
  }

  //----------------------------------------------------------------
  // VBinary::get
  // 
  bool
  VBinary::get(Bytes & bytes) const
  {
    if (!receipt_)
    {
      return false;
    }
    
    bytes = Bytes(size_);
    return receipt_->load(bytes);
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const Bytes & bytes)
  {
    if (storage_)
    {
      sizeDefault_ = bytes.size();
      receiptDefault_ = storage_->save(bytes);
    }
    else
    {
      assert(false);
    }
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::isDefault
  // 
  bool
  VBinary::isDefault() const
  {
    if (receiptDefault_ && receipt_)
    {
      if (sizeDefault_ != size_)
      {
        return false;
      }
      
      Bytes bytesDefault(sizeDefault_);
      Bytes bytes(size_);
      
      if (receiptDefault_->load(bytesDefault))
      {
        // default payload can't be read:
        return false;
      }
      
      if (!receipt_->load(bytes))
      {
        // payload can't be read:
        return true;
      }
      
      // compare the bytes:
      bool same = (bytesDefault.bytes_->front() ==
                   bytes.bytes_->front());
      return same;
    }
    
    bool same = (receiptDefault_ == receipt_ &&
                 sizeDefault_ == size_);
    return same;
  }
  
  //----------------------------------------------------------------
  // VBinary::calcSize
  // 
  uint64
  VBinary::calcSize() const
  {
    return size_;
  }
  
  //----------------------------------------------------------------
  // VBinary::save
  // 
  IStorage::IReceiptPtr
  VBinary::save(IStorage & storage, Crc32 * crc) const
  {
    if (!receipt_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    Bytes data(size_);
    if (!receipt_->load(data))
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    Bytes bytes;
    bytes << vsizeEncode(size_);
    bytes += data;
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
  //----------------------------------------------------------------
  // VBinary::load
  // 
  uint64
  VBinary::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    uint64 vsizeSize = 0;
    size_ = (std::size_t)vsizeDecode(storage, vsizeSize, crc);
    
    Bytes bytes(size_);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      receipt_ = receipt;
    }
    else
    {
      receipt_ = IStorage::IReceiptPtr();
      size_ = 0;
    }
    
    uint64 bytesRead = vsizeSize + size_;
    return bytesRead;
  }
}
