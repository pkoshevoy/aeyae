// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:31:46 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaIStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // IStorage::IReceipt::operator +=
  // 
  IStorage::IReceipt &
  IStorage::IReceipt::operator += (const IReceiptPtr & receipt)
  {
    if (!receipt)
    {
      return *this;
    }
    
    return add(receipt->numBytes());
  }
  
  
  //----------------------------------------------------------------
  // NullStorage::NullStorage
  // 
  NullStorage::NullStorage(uint64 currentPosition):
    currentPosition_(currentPosition)
  {}
  
  //----------------------------------------------------------------
  // NullStorage::receipt
  // 
  IStorage::IReceiptPtr
  NullStorage::receipt() const
  {
    return IStorage::IReceiptPtr(new Receipt(currentPosition_));
  }
  
  //----------------------------------------------------------------
  // NullStorage::isNullStorage
  // 
  bool
  NullStorage::isNullStorage() const
  {
    return true;
  }
  
  //----------------------------------------------------------------
  // NullStorage::save
  // 
  IStorage::IReceiptPtr
  NullStorage::save(const Bytes & data)
  {
    IStorage::IReceiptPtr receipt(new Receipt(currentPosition_));
    
    const std::size_t dataSize = data.size();
    currentPosition_ += dataSize;
    receipt->add(dataSize);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // NullStorage::load
  // 
  IStorage::IReceiptPtr
  NullStorage::load(Bytes & data)
  {
    return IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // NullStorage::skip
  // 
  IStorage::IReceiptPtr
  NullStorage::skip(uint64 numBytes)
  {
    IStorage::IReceiptPtr receipt(new Receipt(currentPosition_));
    
    currentPosition_ += numBytes;
    receipt->add(numBytes);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::Receipt
  // 
  NullStorage::Receipt::Receipt(uint64 addr):
    addr_(addr),
    numBytes_(0)
  {}
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::position
  // 
  uint64
  NullStorage::Receipt::position() const
  {
    return addr_;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::numBytes
  // 
  uint64
  NullStorage::Receipt::numBytes() const
  {
    return numBytes_;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::setNumBytes
  // 
  NullStorage::Receipt &
  NullStorage::Receipt::setNumBytes(uint64 numBytes)
  {
    numBytes_ = numBytes;
    return *this;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::add
  // 
  NullStorage::Receipt &
  NullStorage::Receipt::add(uint64 numBytes)
  {
    numBytes_ += numBytes;
    return *this;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::save
  // 
  bool
  NullStorage::Receipt::save(const Bytes & data)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::load
  // 
  bool
  NullStorage::Receipt::load(Bytes & data)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::calcCrc32
  // 
  bool
  NullStorage::Receipt::calcCrc32(Crc32 & computeCrc32,
                                  const IStorage::IReceiptPtr & receiptSkip)
  {
    return false;
  }
  
}
