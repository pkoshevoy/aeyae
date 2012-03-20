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
  // IStorage::skipWithReceipt
  // 
  IStorage::IReceiptPtr
  IStorage::skipWithReceipt(uint64 numBytes)
  {
    IReceiptPtr dataReceipt = this->receipt();
    if (!dataReceipt || !this->skip(numBytes))
    {
      return IReceiptPtr();
    }
    
    dataReceipt->add(numBytes);
    return dataReceipt;
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
  NullStorage::save(const unsigned char * data, std::size_t size)
  {
    (void) data;
    IStorage::IReceiptPtr receipt(new Receipt(currentPosition_));
    
    currentPosition_ += size;
    receipt->add(size);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // NullStorage::load
  // 
  IStorage::IReceiptPtr
  NullStorage::load(unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    return IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // NullStorage::peek
  // 
  std::size_t
  NullStorage::peek(unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    return 0;
  }
  
  //----------------------------------------------------------------
  // NullStorage::skip
  // 
  uint64
  NullStorage::skip(uint64 numBytes)
  {
    currentPosition_ += numBytes;
    return numBytes;
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
  NullStorage::Receipt::save(const unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    return false;
  }
  
  //----------------------------------------------------------------
  // NullStorage::Receipt::load
  // 
  bool
  NullStorage::Receipt::load(unsigned char * data)
  {
    (void) data;
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
  
  
  //----------------------------------------------------------------
  // MemReceipt::MemReceipt
  // 
  MemReceipt::MemReceipt(void * addr, std::size_t numBytes):
    addr_((unsigned char *)addr),
    numBytes_(numBytes)
  {}
  
  //----------------------------------------------------------------
  // MemReceipt::position
  // 
  uint64
  MemReceipt::position() const
  {
    return uint64(addr_);
  }
  
  //----------------------------------------------------------------
  // MemReceipt::numBytes
  // 
  uint64
  MemReceipt::numBytes() const
  {
    return uint64(numBytes_);
  }
  
  //----------------------------------------------------------------
  // MemReceipt::setNumBytes
  // 
  MemReceipt &
  MemReceipt::setNumBytes(uint64 numBytes)
  {
    numBytes_ = (std::size_t)numBytes;
    return *this;
  }
  
  //----------------------------------------------------------------
  // MemReceipt::add
  // 
  MemReceipt &
  MemReceipt::add(uint64 numBytes)
  {
    numBytes_ += (std::size_t)numBytes;
    return *this;
  }
  
  //----------------------------------------------------------------
  // MemReceipt::save
  // 
  bool
  MemReceipt::save(const unsigned char * data, std::size_t size)
  {
    std::size_t dstSize = numBytes_;
    if (dstSize < size)
    {
      return false;
    }
    
    memcpy(addr_, data, size);
    return true;
  }
  
  //----------------------------------------------------------------
  // MemReceipt::load
  // 
  bool
  MemReceipt::load(unsigned char * data)
  {
    memcpy(data, addr_, numBytes_);
    return true;
  }
  
  //----------------------------------------------------------------
  // MemReceipt::calcCrc32
  // 
  bool
  MemReceipt::calcCrc32(Crc32 & computeCrc32,
                        const IStorage::IReceiptPtr & receiptSkip)
  {
    try
    {
      const unsigned char * skipAddr = addr_;
      uint64 skipBytes = 0;
      
      if (receiptSkip)
      {
        skipAddr += receiptSkip->position() - position();
        skipBytes = receiptSkip->numBytes();
      }
      
      const unsigned char * p0 =
        std::min<const unsigned char *>(addr_, skipAddr);
      std::size_t n0 = (std::size_t)(skipAddr) - (std::size_t)(p0);
      
      const unsigned char * p1 =
        std::min<const unsigned char *>(skipAddr + skipBytes,
                                        addr_ + numBytes_);
      std::size_t n1 = (std::size_t)(addr_ + numBytes_) - (std::size_t)(p1);
      
      if (n0)
      {
        computeCrc32.compute(p0, (std::size_t)n0);
      }
      
      if (n1)
      {
        computeCrc32.compute(p1, (std::size_t)n1);
      }
      
      return true;
    }
    catch (...)
    {}
    
    return false;
  }
  
  //----------------------------------------------------------------
  // MemReceipt::receipt
  // 
  IStorage::IReceiptPtr
  MemReceipt::receipt(uint64 offset, uint64 size) const
  {
    unsigned char * addr = addr_ + (std::size_t)offset;
    return IStorage::IReceiptPtr(new MemReceipt(addr, (std::size_t)size));
  }
  
  
  //----------------------------------------------------------------
  // ConstMemReceipt::ConstMemReceipt
  // 
  ConstMemReceipt::ConstMemReceipt(const void * addr, std::size_t numBytes):
    MemReceipt(const_cast<void *>(addr), numBytes)
  {}
  
  //----------------------------------------------------------------
  // ConstMemReceipt::save
  // 
  bool
  ConstMemReceipt::save(const unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    return false;
  }
  
  //----------------------------------------------------------------
  // ConstMemReceipt::receipt
  // 
  IStorage::IReceiptPtr
  ConstMemReceipt::receipt(uint64 offset, uint64 size) const
  {
    const unsigned char * addr = addr_ + (std::size_t)offset;
    return IStorage::IReceiptPtr(new ConstMemReceipt(addr, (std::size_t)size));
  }
  
  
  //----------------------------------------------------------------
  // receiptForMemory
  // 
  IStorage::IReceiptPtr
  receiptForMemory(void * data, std::size_t size)
  {
    return IStorage::IReceiptPtr(new MemReceipt(data, size));
  }
  
  //----------------------------------------------------------------
  // receiptForConstMemory
  // 
  IStorage::IReceiptPtr
  receiptForConstMemory(const void * data, std::size_t size)
  {
    return IStorage::IReceiptPtr(new ConstMemReceipt(data, size));
  }
  
}
