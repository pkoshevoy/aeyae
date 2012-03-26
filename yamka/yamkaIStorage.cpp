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
  // MemoryStorage::receipt
  // 
  IStorage::IReceiptPtr
  MemoryStorage::receipt() const
  {
    TStoragePtr bytes(new TStorage());
    return IStorage::IReceiptPtr(new Receipt(bytes));
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::save
  // 
  IStorage::IReceiptPtr
  MemoryStorage::save(const unsigned char * data, std::size_t size)
  {
    TStoragePtr bytes(new TStorage(data, data + size));
    return IStorage::IReceiptPtr(new Receipt(bytes, 0, size));
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::load
  // 
  IStorage::IReceiptPtr
  MemoryStorage::load(unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    assert(false);
    return IStorage::IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::peek
  // 
  std::size_t
  MemoryStorage::peek(unsigned char * data, std::size_t size)
  {
    (void) data;
    (void) size;
    assert(false);
    return 0;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::skip
  // 
  uint64
  MemoryStorage::skip(uint64 numBytes)
  {
    (void) numBytes;
    assert(false);
    return 0;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::Receipt
  // 
  MemoryStorage::Receipt::Receipt(const MemoryStorage::TStoragePtr & bytesPtr,
                                  std::size_t position,
                                  std::size_t numBytes):
    bytesPtr_(bytesPtr),
    position_(position),
    numBytes_(numBytes)
  {}
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::position
  // 
  uint64
  MemoryStorage::Receipt::position() const
  {
    return position_;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::numBytes
  // 
  uint64
  MemoryStorage::Receipt::numBytes() const
  {
    return numBytes_;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::setNumBytes
  // 
  MemoryStorage::Receipt &
  MemoryStorage::Receipt::setNumBytes(uint64 numBytes)
  {
    numBytes_ = (std::size_t)numBytes;
    assert(position_ + numBytes_ <= bytesPtr_->size());
    return *this;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::add
  // 
  MemoryStorage::Receipt &
  MemoryStorage::Receipt::add(uint64 numBytes)
  {
    numBytes_ += (std::size_t)numBytes;
    assert(position_ + numBytes_ <= bytesPtr_->size());
    return *this;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::save
  // 
  bool
  MemoryStorage::Receipt::save(const unsigned char * data, std::size_t size)
  {
    if (!size)
    {
      return true;
    }
    
    TStorage & bytes = *bytesPtr_;
    const std::size_t capacity = bytes.size();
    
    if (capacity < position_ + size)
    {
      assert(false);
      return false;
    }
    
    unsigned char * dst = &bytes[position_];
    memcpy(dst, data, size);
    numBytes_ = std::max<std::size_t>(numBytes_, size);
    
    return true;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::load
  // 
  bool
  MemoryStorage::Receipt::load(unsigned char * data)
  {
    const TStorage & bytes = *bytesPtr_;
    const std::size_t capacity = bytes.size();
    assert(position_ + numBytes_ <= capacity);
    
    if (numBytes_)
    {
      const unsigned char * src = &bytes[position_];
      memcpy(data, src, numBytes_);
    }
    
    return true;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::calcCrc32
  // 
  bool
  MemoryStorage::Receipt::calcCrc32(Crc32 & computeCrc32,
                                    const IStorage::IReceiptPtr & receiptSkip)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // MemoryStorage::Receipt::receipt
  // 
  IStorage::IReceiptPtr
  MemoryStorage::Receipt::receipt(uint64 offset, uint64 size) const
  {
    std::size_t position = position_ + (std::size_t)(offset + size);
    assert(position <= bytesPtr_->size());
    
    return IStorage::IReceiptPtr(new Receipt(bytesPtr_,
                                             position,
                                             (std::size_t)size));
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
