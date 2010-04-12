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
  // IStorage::IReceipt::saveAndCalcCrc32
  // 
  bool
  IStorage::IReceipt::saveAndCalcCrc32(const Bytes & data,
                                       Crc32 * crc32)
  {
    if (save(data))
    {
      if (crc32)
      {
        crc32->compute(data);
      }
      
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // IStorage::IReceipt::loadAndCalcCrc32
  // 
  bool
  IStorage::IReceipt::loadAndCalcCrc32(Bytes & data,
                                       Crc32 * crc32)
  {
    if (load(data))
    {
      if (crc32)
      {
        crc32->compute(data);
      }
      
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // IStorage::saveAndCalcCrc32
  // 
  IStorage::IReceiptPtr
  IStorage::saveAndCalcCrc32(const Bytes & data,
                             Crc32 * crc32)
  {
    IStorage::IReceiptPtr receipt = save(data);
    if (receipt)
    {
      if (crc32)
      {
        crc32->compute(data);
      }
    }
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // IStorage::loadAndCalcCrc32
  // 
  IStorage::IReceiptPtr
  IStorage::loadAndCalcCrc32(Bytes & data,
                             Crc32 * crc32)
  {
    IStorage::IReceiptPtr receipt = load(data);
    if (receipt)
    {
      if (crc32)
      {
        crc32->compute(data);
      }
    }
    
    return receipt;
  }
  
}
