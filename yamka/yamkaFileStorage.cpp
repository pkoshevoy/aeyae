// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 20:02:27 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaFileStorage.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // FileStorage::FileStorage
  // 
  FileStorage::FileStorage(const std::string & pathUTF8,
                           File::AccessMode fileMode):
    file_(pathUTF8, fileMode)
  {}

  //----------------------------------------------------------------
  // FileStorage::receipt
  // 
  IStorage::IReceiptPtr
  FileStorage::receipt() const
  {
    return IStorage::IReceiptPtr(new Receipt(file_));
  }
    
  //----------------------------------------------------------------
  // FileStorage::save
  // 
  IStorage::IReceiptPtr
  FileStorage::save(const Bytes & data)
  {
    IStorage::IReceiptPtr receipt(new Receipt(file_));
    if (!file_.save(data))
    {
      return IStorage::IReceiptPtr();
    }
    
    receipt->add(data.size());
    return receipt;
  }
  
  //----------------------------------------------------------------
  // FileStorage::load
  // 
  IStorage::IReceiptPtr
  FileStorage::load(Bytes & data)
  {
    IStorage::IReceiptPtr receipt(new Receipt(file_));
    if (!file_.load(data))
    {
      return IStorage::IReceiptPtr();
    }
    
    receipt->add(data.size());
    return receipt;
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::Receipt
  // 
  FileStorage::Receipt::Receipt(const File & file):
    file_(file),
    addr_(file.absolutePosition()),
    numBytes_(0)
  {}
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::position
  // 
  uint64
  FileStorage::Receipt::position() const
  {
    return addr_;
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::numBytes
  // 
  uint64
  FileStorage::Receipt::numBytes() const
  {
    return numBytes_;
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::add
  // 
  FileStorage::Receipt &
  FileStorage::Receipt::add(uint64 numBytes)
  {
    numBytes_ += numBytes;
    return *this;
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::save
  // 
  bool
  FileStorage::Receipt::save(const Bytes & data)
  {
    try
    {
      File::Seek temp(file_, addr_);
      if (file_.save(data))
      {
        numBytes_ = std::max<uint64>(numBytes_, data.size());
        return true;
      }
    }
    catch (...)
    {}
    
    return false;
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::load
  // 
  bool
  FileStorage::Receipt::load(Bytes & data)
  {
    try
    {
      File::Seek temp(file_, addr_);
      if (file_.load(data))
      {
        numBytes_ = std::max<uint64>(numBytes_, data.size());
        return true;
      }
    }
    catch (...)
    {}
    
    return false;
  }
  
}
