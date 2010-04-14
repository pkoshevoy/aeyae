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
  // VInt::VInt
  // 
  VInt::VInt():
    TSuper()
  {
    setDefault(0);
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
  VInt::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 numBytes = vsizeDecode(storage, crc);
    
    Bytes bytes(numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = intDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeNumBytes(numBytes) + numBytes;
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
  VUInt::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 numBytes = vsizeDecode(storage, crc);
    
    Bytes bytes(numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = uintDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeNumBytes(numBytes) + numBytes;
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
  VFloat::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 numBytes = vsizeDecode(storage, crc);
    
    Bytes bytes(numBytes);
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
    
    uint64 bytesRead = vsizeNumBytes(numBytes) + numBytes;
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
  VDate::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 numBytes = vsizeDecode(storage, crc);
    
    Bytes bytes(numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TSuper::data_ = intDecode(bytes, numBytes);
    }
    
    uint64 bytesRead = vsizeNumBytes(numBytes) + numBytes;
    return bytesRead;
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
  VString::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 numBytes = vsizeDecode(storage, crc);
    
    Bytes bytes(numBytes);
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      TByteVec chars = TByteVec(bytes);
      TSuper::data_.assign((const char *)&chars[0], chars.size());
    }
    
    uint64 bytesRead = vsizeNumBytes(numBytes) + numBytes;
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
    binStorage_(defaultStorage_),
    binSize_(0),
    binSizeDefault_(0)
  {}
  
  //----------------------------------------------------------------
  // VBinary::setStorage
  // 
  VBinary &
  VBinary::setStorage(const IStoragePtr & binStorage)
  {
    binStorage_ = binStorage;
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::set
  // 
  VBinary &
  VBinary::set(const Bytes & bytes)
  {
    if (binStorage_)
    {
      uint64 numBytes = bytes.size();
      Bytes data = Bytes(vsizeEncode(numBytes)) + bytes;
      
      binReceipt_ = binStorage_->save(data);
      binSize_ = data.size();
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
    if (!binReceipt_)
    {
      return false;
    }
    
    uint64 size = calcSize();
    Bytes data(size);
    if (!binReceipt_->load(data))
    {
      return false;
    }
    
    uint64 numBytes = vsizeDecode(data);
    bytes = Bytes(numBytes);
    
    uint64 skipBytes = vsizeNumBytes(numBytes);
    memcpy(&bytes[0], &data[0] + skipBytes, numBytes);
    
    return true;
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const Bytes & bytes)
  {
    if (binStorage_)
    {
      uint64 numBytes = bytes.size();
      Bytes data = Bytes(vsizeEncode(numBytes)) + bytes;
      
      binReceiptDefault_ = binStorage_->save(data);
      binSizeDefault_ = data.size();
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
    if (binReceiptDefault_ && binReceipt_)
    {
      if (binSizeDefault_ != binSize_)
      {
        return false;
      }
      
      Bytes bytesDefault(binSizeDefault_);
      Bytes bytes(binSize_);
      
      if (binReceiptDefault_->load(bytesDefault))
      {
        // default payload can't be read:
        return false;
      }
      
      if (!binReceipt_->load(bytes))
      {
        // payload can't be read:
        return true;
      }
      
      // compare the bytes:
      bool same = (bytesDefault.bytes_->front() ==
                   bytes.bytes_->front());
      return same;
    }
    
    bool same = (binReceiptDefault_ == binReceipt_ &&
                 binSizeDefault_ == binSize_);
    return same;
  }
  
  //----------------------------------------------------------------
  // VBinary::calcSize
  // 
  uint64
  VBinary::calcSize() const
  {
    return binSize_;
  }
  
  //----------------------------------------------------------------
  // VBinary::save
  // 
  IStorage::IReceiptPtr
  VBinary::save(IStorage & storage, Crc32 * crc) const
  {
    if (!binReceipt_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    uint64 size = calcSize();
    Bytes data(size);
    if (!binReceipt_->load(data))
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    return storage.saveAndCalcCrc32(data, crc);
  }
  
  //----------------------------------------------------------------
  // VBinary::load
  // 
  uint64
  VBinary::load(IStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    uint64 numBytes = vsizeDecode(storage, crc);
    Bytes bytes(numBytes);
    
    binSize_ = 0;
    if (storage.loadAndCalcCrc32(bytes, crc))
    {
      binSize_ = vsizeNumBytes(numBytes) + numBytes;
      binReceipt_ = receipt;
    }
    
    return binSize_;
  }
}
