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
    storage.loadAndCalcCrc32(bytes, crc);
    
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
      binReceipt_ = binStorage_->save(bytes);
      binSize_ = bytes.size();
    }
    else
    {
      assert(false);
    }
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // VBinary::setDefault
  // 
  VBinary &
  VBinary::setDefault(const Bytes & bytes)
  {
    if (binStorage_)
    {
      binReceiptDefault_ = binStorage_->save(bytes);
      binSizeDefault_ = bytes.size();
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
    
    Bytes bytes;
    bytes << vsizeEncode(size);
    bytes += data;
    
    return storage.saveAndCalcCrc32(bytes, crc);
  }
  
}
