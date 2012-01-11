// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 15:31:46 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaIStorage.h>
#include <yamkaHodgePodge.h>


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
  // IStorage::loadHodgePodge
  // 
  IStorage::IReceiptPtr
  IStorage::loadHodgePodge(HodgePodge & hodgePodge, uint64 numBytes)
  {
    IReceiptPtr receipt = skip(numBytes);
    hodgePodge.set(receipt);
    return receipt;
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
  NullStorage::Receipt::Receipt(uint64 addr, uint64 numBytes):
    addr_(addr),
    numBytes_(numBytes)
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
  // NullStorage::Receipt::load
  // 
  bool
  NullStorage::Receipt::load(TByte * data)
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

  //----------------------------------------------------------------
  // NullStorage::Receipt::receipt
  // 
  IStorage::IReceiptPtr
  NullStorage::Receipt::receipt(uint64 offset, uint64 size) const
  {
    if (offset + size > numBytes_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    NullStorage::Receipt * r = new NullStorage::Receipt(addr_ + offset, size);
    return IStorage::IReceiptPtr(r);
  }
  
}
