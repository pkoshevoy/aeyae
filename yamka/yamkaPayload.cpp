// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 23:49:04 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaPayload.h>


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
  VInt::save(IStorage & storage, Crc32 * computeCrc32) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
	  << intEncode(TSuper::data_);
    
    return storage.saveAndCalcCrc32(bytes, computeCrc32);
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
  VUInt::save(IStorage & storage, Crc32 * computeCrc32) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
	  << uintEncode(TSuper::data_);
    
    return storage.saveAndCalcCrc32(bytes, computeCrc32);
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
  VString::save(IStorage & storage, Crc32 * computeCrc32) const
  {
    uint64 size = calcSize();
    
    Bytes bytes;
    bytes << vsizeEncode(size)
	  << TSuper::data_;
    
    return storage.saveAndCalcCrc32(bytes, computeCrc32);
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
  VBinary::save(IStorage & storage, Crc32 * computeCrc32) const
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
    
    return storage.saveAndCalcCrc32(bytes, computeCrc32);
  }
  
}
