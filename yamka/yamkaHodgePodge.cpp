// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Jan 10 13:08:10 MST 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaHodgePodge.h>

// system includes:
#include <assert.h>
#include <string.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // TReceiptPtrCIter
  // 
  typedef std::deque<IStorage::IReceiptPtr>::const_iterator TReceiptPtrCIter;
  
  //----------------------------------------------------------------
  // HodgePodge::add
  // 
  void
  HodgePodge::add(const IStorage::IReceiptPtr & dataReceipt)
  {
    receipts_.push_back(dataReceipt);
  }

  //----------------------------------------------------------------
  // HodgePodge::add
  // 
  void
  HodgePodge::add(const Bytes & data, IStorage & storage)
  {
    IStorage::IReceiptPtr dataReceipt = storage.save(data);
    receipts_.push_back(dataReceipt);
  }

  //----------------------------------------------------------------
  // HodgePodge::load
  // 
  uint64
  HodgePodge::load(FileStorage & storage, uint64 bytesToRead)
  {
    IStorage::IReceiptPtr dataReceipt = storage.skip(bytesToRead);
    if (!dataReceipt)
    {
      return 0;
    }
    
    receipts_.clear();
    receipts_.push_back(dataReceipt);
    return dataReceipt->numBytes();
  }

  //----------------------------------------------------------------
  // HodgePodge::save
  // 
  IStorage::IReceiptPtr
  HodgePodge::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();

    // shortcut:
    bool isNullStorage = storage.isNullStorage();
    
    for (TReceiptPtrCIter i = receipts_.begin(); i != receipts_.end(); ++i)
    {
      IStorage::IReceiptPtr srcReceipt = *i;
      IStorage::IReceiptPtr dstReceipt;
      
      if (isNullStorage)
      {
        // don't bother copying the data when saving to NULL storage:
        dstReceipt = storage.skip(srcReceipt->numBytes());
      }
      else
      {
        Bytes data((std::size_t)srcReceipt->numBytes());
        if (!srcReceipt->load(data))
        {
          assert(false);
          return IStorage::IReceiptPtr();
        }
        
        dstReceipt = storage.save(data);
      }
      
      if (!dstReceipt)
      {
        assert(false);
        return IStorage::IReceiptPtr();
      }
      
      *receipt += dstReceipt;
    }

    if (!receipt->numBytes())
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    return receipt;
  }

  //----------------------------------------------------------------
  // HodgePodge::numBytes
  // 
  uint64
  HodgePodge::numBytes() const
  {
    uint64 total = 0;
    
    for (TReceiptPtrCIter i = receipts_.begin(); i != receipts_.end(); ++i)
    {
      const IStorage::IReceipt & dataReceipt = *(*i);
      total += dataReceipt.numBytes();
    }
    
    return total;
  }
  
  //----------------------------------------------------------------
  // HodgePodge::get
  // 
  bool
  HodgePodge::get(Bytes & bytes) const
  {
    if (receipts_.empty())
    {
      return false;
    }
    
    uint64 total = numBytes();
    bytes = Bytes((std::size_t)total);
    TByte * data = &(bytes[0]);
    
    for (TReceiptPtrCIter i = receipts_.begin(); i != receipts_.end(); ++i)
    {
      const IStorage::IReceiptPtr & dataReceipt = *i;
      if (!dataReceipt->load(data))
      {
        assert(false);
        return false;
      }
      
      data += dataReceipt->numBytes();
    }
    
    return true;
  }

  //----------------------------------------------------------------
  // HodgePodge::operator ==
  // 
  bool
  HodgePodge::operator == (const HodgePodge & other) const
  {
    std::size_t na = receipts_.size();
    std::size_t nb = other.receipts_.size();

    if (na != nb)
    {
      return false;
    }
    
    if (na == 0)
    {
      return true;
    }
    
    Bytes a(na);
    if (!get(a))
    {
      return false;
    }
    
    Bytes b(nb);
    if (!get(b))
    {
      return false;
    }

    bool same = memcmp(&(a[0]), &(b[0]), na) == 0;
    return same;
  }
  

  //----------------------------------------------------------------
  // HodgePodgeConstIter::HodgePodgeConstIter
  // 
  HodgePodgeConstIter::HodgePodgeConstIter(const HodgePodge & hodgePodge,
                                           uint64 pos):
    hodgePodge_(hodgePodge),
    start_(0),
    end_(0),
    pos_(pos)
  {}

  //----------------------------------------------------------------
  // HodgePodgeConstIter::setpos
  // 
  HodgePodgeConstIter &
  HodgePodgeConstIter::setpos(uint64 pos)
  {
    pos_ = pos;
    return *this;
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::operator []
  // 
  TByte
  HodgePodgeConstIter::operator [] (int64 offset) const
  {
    if (!load(pos_ + offset))
    {
      assert(false);
      return 0;
    }

    return cache_[(std::size_t)(pos_ + offset)];
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::operator
  // 
  TByte
  HodgePodgeConstIter::operator * () const
  {
    if (!load(pos_))
    {
      assert(false);
      return 0;
    }

    return cache_[(std::size_t)pos_];
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::receipt
  // 
  IStorage::IReceiptPtr
  HodgePodgeConstIter::receipt(uint64 position, uint64 numBytes) const
  {
    if (!load(position) || position + numBytes > end_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }
    
    return receipt_->receipt(position - start_, numBytes);
  }
  
  //----------------------------------------------------------------
  // HodgePodgeConstIter::load
  // 
  bool
  HodgePodgeConstIter::load(uint64 pos) const
  {
    if (pos >= start_ && pos < end_)
    {
      return true;
    }

    start_ = 0;
    for (TReceiptPtrCIter i = hodgePodge_.receipts_.begin();
         i != hodgePodge_.receipts_.end(); ++i)
    {
      const IStorage::IReceiptPtr & dataReceipt = *i;
      uint64 size = dataReceipt->numBytes();
      end_ = start_ + size;
      
      if (pos >= start_ && pos < end_)
      {
        cache_ = TByteVec((std::size_t)size);
        receipt_ = dataReceipt;
        return receipt_->load(&cache_[0]);
      }
    }
    
    return false;
  }

}
