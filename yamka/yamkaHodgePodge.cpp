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
    if (dataReceipt->numBytes())
    {
      receipts_.push_back(dataReceipt);
    }
  }

  //----------------------------------------------------------------
  // HodgePodge::add
  //
  void
  HodgePodge::add(const unsigned char * data,
                  std::size_t size,
                  IStorage & storage)
  {
    if (size)
    {
      IStorage::IReceiptPtr dataReceipt = storage.save(data, size);
      receipts_.push_back(dataReceipt);
    }
  }

  //----------------------------------------------------------------
  // HodgePodge::load
  //
  uint64
  HodgePodge::load(FileStorage & storage, uint64 bytesToRead)
  {
    IStorage::IReceiptPtr dataReceipt = storage.skipWithReceipt(bytesToRead);
    if (!dataReceipt)
    {
      return 0;
    }

    receipts_.clear();
    receipts_.push_back(dataReceipt);
    return bytesToRead;
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
        dstReceipt = storage.skipWithReceipt(srcReceipt->numBytes());
      }
      else
      {
        TByteVec data;
        if (!Yamka::load(srcReceipt, data))
        {
          assert(false);
          return IStorage::IReceiptPtr();
        }

        dstReceipt = Yamka::save(storage, data);
      }

      if (!dstReceipt)
      {
        assert(false);
        return IStorage::IReceiptPtr();
      }

      *receipt += dstReceipt;
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
  HodgePodge::get(TByteVec & bytes) const
  {
    uint64 total = numBytes();
    bytes.resize((std::size_t)total);
    return total ? get(&(bytes[0])) : true;
  }

  //----------------------------------------------------------------
  // HodgePodge::get
  //
  bool
  HodgePodge::get(unsigned char * data) const
  {
    if (receipts_.empty())
    {
      return false;
    }

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

    TByteVec a;
    if (!this->get(a))
    {
      return false;
    }

    TByteVec b;
    if (!this->get(b))
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
    pos_(pos),
    receiptStart_(0),
    receiptEnd_(0),
    cacheStart_(0),
    cacheEnd_(0)
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
  unsigned char
  HodgePodgeConstIter::operator [] (int64 offset) const
  {
    if (!updateCache(pos_ + offset))
    {
      assert(false);
      return 0;
    }

    return cache_[(std::size_t)(pos_ + offset - receiptStart_ - cacheStart_)];
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::operator
  //
  unsigned char
  HodgePodgeConstIter::operator * () const
  {
    if (!updateCache(pos_))
    {
      assert(false);
      return 0;
    }

    return cache_[(std::size_t)(pos_ - receiptStart_ - cacheStart_)];
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::receipt
  //
  IStorage::IReceiptPtr
  HodgePodgeConstIter::receipt(uint64 position, uint64 numBytes) const
  {
    if (!updateReceipt(position) || position + numBytes > receiptEnd_)
    {
      assert(false);
      return IStorage::IReceiptPtr();
    }

    return receipt_->receipt(position - receiptStart_, numBytes);
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::updateReceipt
  //
  bool
  HodgePodgeConstIter::updateReceipt(uint64 position) const
  {
    if (position >= receiptStart_ && position < receiptEnd_)
    {
      return true;
    }

    receiptStart_ = 0;
    cacheStart_ = 0;
    cacheEnd_ = 0;

    for (TReceiptPtrCIter i = hodgePodge_.receipts_.begin();
         i != hodgePodge_.receipts_.end(); ++i)
    {
      const IStorage::IReceiptPtr & dataReceipt = *i;
      uint64 size = dataReceipt->numBytes();

      if (!size)
      {
        // there should be no empty receipts in the quilt:
        assert(false);
        continue;
      }

      receiptEnd_ = receiptStart_ + size;

      if (position >= receiptStart_ && position < receiptEnd_)
      {
        receipt_ = dataReceipt;
        return true;
      }

      receiptStart_ += size;
    }

    receipt_ = IStorage::IReceiptPtr();
    receiptEnd_ = 0;
    return false;
  }

  //----------------------------------------------------------------
  // HodgePodgeConstIter::updateCache
  //
  bool
  HodgePodgeConstIter::updateCache(uint64 position) const
  {
    if (position >= cacheStart_ && position < cacheEnd_)
    {
      return true;
    }

    if (updateReceipt(position))
    {
      uint64 receiptSize = receiptEnd_ - receiptStart_;

      cacheStart_ = position - (position % kCacheSize);
      cacheEnd_ = std::min<uint64>(cacheStart_ + kCacheSize, receiptSize);

      std::size_t chunkSize = (std::size_t)(cacheEnd_ - cacheStart_);
      cache_.resize(kCacheSize);

      IStorage::IReceiptPtr chunk =
        cacheStart_ || chunkSize != receiptSize ?
        receipt_->receipt(cacheStart_, chunkSize) :
        receipt_;

      return chunk->load(&cache_[0]);
    }

    return false;
  }

}
