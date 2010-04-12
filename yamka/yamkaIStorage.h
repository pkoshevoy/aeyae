// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 12:47:08 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_ISTORAGE_H_
#define YAMKA_ISTORAGE_H_

// yamka includes:
#include <yamkaBytes.h>
#include <yamkaCrc32.h>

// boost includes:
#include <boost/shared_ptr.hpp>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // IStorage
  // 
  struct IStorage
  {
    virtual ~IStorage() {}

    //----------------------------------------------------------------
    // IReceipt
    // 
    struct IReceipt
    {
      virtual ~IReceipt() {}

      // return false if load/save fails:
      virtual bool save(const Bytes & data) = 0;
      virtual bool load(Bytes & data) = 0;
      
      // helpers:
      // load/save and optionally compute CRC-32 checksum on the fly;
      // checksum is calculated only if save/load succeeded
      bool saveAndCalcCrc32(const Bytes & data,
                            Crc32 * computeCrc32 = NULL);
      
      bool loadAndCalcCrc32(Bytes & data,
                            Crc32 * computeCrc32 = NULL);
    };
    
    //----------------------------------------------------------------
    // IReceiptPtr
    // 
    typedef boost::shared_ptr<IReceipt> IReceiptPtr;
    
    // get a receipt for the current storage state:
    virtual IReceiptPtr receipt() const = 0;
    
    // return NULL receipt if load/save fails:
    virtual IReceiptPtr save(const Bytes & data) = 0;
    virtual IReceiptPtr load(Bytes & data) = 0;
    
    // helpers:
    // load/save and optionally compute CRC-32 checksum on the fly;
    // checksum is calculated only if save/load succeeded
    IReceiptPtr saveAndCalcCrc32(const Bytes & data,
                                 Crc32 * computeCrc32 = NULL);
    
    IReceiptPtr loadAndCalcCrc32(Bytes & data,
                                 Crc32 * computeCrc32 = NULL);
  };
  
  //----------------------------------------------------------------
  // IStoragePtr
  // 
  typedef boost::shared_ptr<IStorage> IStoragePtr;
  
}


#endif // YAMKA_ISTORAGE_H_
