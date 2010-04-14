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
    return file_.save(data) ? receipt : IStorage::IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // FileStorage::load
  // 
  IStorage::IReceiptPtr
  FileStorage::load(Bytes & data)
  {
    IStorage::IReceiptPtr receipt(new Receipt(file_));
    return file_.load(data) ? receipt : IStorage::IReceiptPtr();
  }
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::Receipt
  // 
  FileStorage::Receipt::Receipt(const File & file):
    file_(file),
    addr_(file.absolutePosition())
  {}
  
  //----------------------------------------------------------------
  // FileStorage::Receipt::save
  // 
  bool
  FileStorage::Receipt::save(const Bytes & data)
  {
    try
    {
      File::Seek temp(file_, addr_);
      return file_.save(data);
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
      return file_.load(data);
    }
    catch (...)
    {}
    
    return false;
  }
  
}
