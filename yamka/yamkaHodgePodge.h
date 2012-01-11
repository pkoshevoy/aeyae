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
    // shortcut:
    inline void set(const IStorage::IReceiptPtr & dataReceipt)
    {
      receipts_.clear();
      add(dataReceipt);
    }

    // shortcut:
    inline void set(const Bytes & data, IStorage & storage)
    {
      receipts_.clear();
      add(data, storage);
    }
    
    // append given data storage receipt:
    void add(const IStorage::IReceiptPtr & dataReceipt);
    
    // store data in given storage and append the data storage receipt:
    void add(const Bytes & data, IStorage & storage);
    
    // discard all receipts, then load data and store the receipt:
    uint64 load(FileStorage & storage, uint64 bytesToRead);
    
    // save the data and return storage receipt to the saved data:
    IStorage::IReceiptPtr save(IStorage & storage) const;
    
    // calculate the sum of receipt data sizes:
    uint64 numBytes() const;

    // load data from storage receipts and store in contiguous byte vector:
    bool get(Bytes & bytes) const;

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
    TByte operator [] (int64 offset) const;
    TByte operator * () const;
    
    IStorage::IReceiptPtr receipt(uint64 position, uint64 numBytes) const; 
    
  protected:
    bool load(uint64 pos) const;
    
    const HodgePodge & hodgePodge_;
    mutable IStorage::IReceiptPtr receipt_;
    mutable TByteVec cache_;
    mutable uint64 start_;
    mutable uint64 end_;
    uint64 pos_;
  };
  
}


#endif // YAMKA_HODGE_PODGE_H_
