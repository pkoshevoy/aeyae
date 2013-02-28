// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Jan 10 12:22:22 MST 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_HODGE_PODGE_H_
#define YAMKA_HODGE_PODGE_H_

// yamka includes:
#include <yamkaFileStorage.h>
#include <yamkaBytes.h>

// boost includes:
#include <boost/cstdint.hpp>

// system includes:
#include <algorithm>
#include <deque>
#include <iterator>


namespace Yamka
{

  //----------------------------------------------------------------
  // uint64
  //
  typedef boost::uint64_t uint64;

  //----------------------------------------------------------------
  // int64
  //
  typedef boost::int64_t int64;

  //----------------------------------------------------------------
  // HodgePodge
  //
  struct HodgePodge
  {
    inline void clear()
    {
      receipts_.clear();
    }

    // shortcut:
    inline void set(const IStorage::IReceiptPtr & dataReceipt)
    {
      receipts_.clear();
      add(dataReceipt);
    }

    // shortcut:
    inline void set(const unsigned char * b, std::size_t nb, IStorage & stor)
    {
      receipts_.clear();
      add(b, nb, stor);
    }

    // shortcut:
    inline void set(const TByteVec & data, IStorage & storage)
    {
      receipts_.clear();
      add(data, storage);
    }

    // shortcut:
    inline void add(const TByteVec & data, IStorage & storage)
    {
      if (!data.empty())
      {
        add(&data[0], data.size(), storage);
      }
    }

    // append given data storage receipt:
    void add(const IStorage::IReceiptPtr & dataReceipt);

    // store data in given storage and append the data storage receipt:
    void add(const unsigned char * data,
             std::size_t size,
             IStorage & storage);

    // discard all receipts, then load data and store the receipt:
    uint64 load(FileStorage & storage, uint64 bytesToRead);

    // save the data and return storage receipt to the saved data:
    IStorage::IReceiptPtr save(IStorage & storage) const;

    // calculate the sum of receipt data sizes:
    uint64 numBytes() const;

    // load data from storage receipts and store in contiguous byte vector:
    bool get(TByteVec & bytes) const;
    bool get(unsigned char * bytes) const;

    // equivalence test:
    bool operator == (const HodgePodge & other) const;

    // data receipts:
    std::deque<IStorage::IReceiptPtr> receipts_;
  };

  //----------------------------------------------------------------
  // HodgePodgeConstIter
  //
  // A trivial acceleration structure for quicker sequential access
  //
  struct HodgePodgeConstIter
  {
    HodgePodgeConstIter(const HodgePodge & src, uint64 pos = 0);
    HodgePodgeConstIter & setpos(uint64 pos);
    unsigned char operator [] (int64 offset) const;
    unsigned char operator * () const;

    IStorage::IReceiptPtr receipt(uint64 position, uint64 numBytes) const;

    // ++i, prefix increment operator:
    inline HodgePodgeConstIter & operator++ ()
    { return setpos(pos_ + 1); }

    // --i, prefix decrement operator:
    inline HodgePodgeConstIter & operator-- ()
    { return setpos(pos_ - 1); }

    inline HodgePodgeConstIter & operator += (int64 offset)
    { return setpos(pos_ + offset); }

    inline HodgePodgeConstIter & operator -= (int64 offset)
    { return setpos(pos_ - offset); }

    inline HodgePodgeConstIter operator + (int64 offset)
    { return HodgePodgeConstIter(hodgePodge_, pos_ + offset); }

    inline HodgePodgeConstIter operator - (int64 offset)
    { return HodgePodgeConstIter(hodgePodge_, pos_ - offset); }

    inline bool operator < (const HodgePodgeConstIter & iter) const
    { return &hodgePodge_ == &iter.hodgePodge_ && pos_ < iter.pos_; }

    inline bool operator == (const HodgePodgeConstIter & iter) const
    { return &hodgePodge_ == &iter.hodgePodge_ && pos_ == iter.pos_; }

    inline bool operator <= (const HodgePodgeConstIter & iter) const
    { return &hodgePodge_ == &iter.hodgePodge_ && pos_ <= iter.pos_; }

    inline bool operator >= (const HodgePodgeConstIter & iter) const
    { return &hodgePodge_ == &iter.hodgePodge_ && pos_ >= iter.pos_; }

    inline int64 operator - (const HodgePodgeConstIter & iter) const
    { return pos_ - iter.pos_; }

    template <typename TBytes>
    int memcmp(const TBytes & bytes, std::size_t numBytes)
    {
      for (std::size_t i = 0; i < numBytes; i++)
      {
        unsigned char a = this->operator[](i);
        unsigned char b = bytes[i];
        if (a == b) continue;

        return (a < b) ? -1 : 1;
      }

      return 0;
    }

  protected:
    bool updateReceipt(uint64 position) const;
    bool updateCache(uint64 position) const;

    const HodgePodge & hodgePodge_;
    uint64 pos_;

    mutable IStorage::IReceiptPtr receipt_;
    mutable uint64 receiptStart_;
    mutable uint64 receiptEnd_;

    enum { kCacheSize = 128 };

    mutable TByteVec cache_;
    mutable uint64 cacheStart_;
    mutable uint64 cacheEnd_;
  };

}


#endif // YAMKA_HODGE_PODGE_H_
