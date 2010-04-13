// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Apr 11 23:44:58 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaEBML.h>


namespace Yamka
{

  //----------------------------------------------------------------
  // EbmlHead::EbmlHead
  // 
  EbmlHead::EbmlHead()
  {
    version_.alwaysSave().payload_.setDefault(1);
    readVersion_.alwaysSave().payload_.setDefault(1);
    maxIdLength_.payload_.setDefault(4);
    maxSizeLength_.payload_.setDefault(8);
  }
  
  //----------------------------------------------------------------
  // EbmlHead::calcSize
  // 
  uint64
  EbmlHead::calcSize() const
  {
    uint64 size =
      version_.calcSize() +
      readVersion_.calcSize() +
      maxIdLength_.calcSize() +
      maxSizeLength_.calcSize() +
      docType_.calcSize() +
      docTypeVersion_.calcSize() +
      docTypeReadVersion_.calcSize();
    
    return size;
  }

  //----------------------------------------------------------------
  // EbmlHead::isDefault
  // 
  bool
  EbmlHead::isDefault() const
  {
    return false;
  }

  //----------------------------------------------------------------
  // EbmlHead::save
  // 
  IStorage::IReceiptPtr
  EbmlHead::save(IStorage & storage, Crc32 * computeCrc32) const
  {
    IStorage::IReceiptPtr receipt =
      storage.save(Bytes(vsizeEncode(calcSize())));
    
    version_.save(storage, computeCrc32);
    readVersion_.save(storage, computeCrc32);
    maxIdLength_.save(storage, computeCrc32);
    maxSizeLength_.save(storage, computeCrc32);
    docType_.save(storage, computeCrc32);
    docTypeVersion_.save(storage, computeCrc32);
    docTypeReadVersion_.save(storage, computeCrc32);
    
    return receipt;
  }
  

  //----------------------------------------------------------------
  // EbmlDoc::EbmlDoc
  // 
  EbmlDoc::EbmlDoc()
  {}

  //----------------------------------------------------------------
  // EbmlDoc::calcSize
  // 
  uint64
  EbmlDoc::calcSize() const
  {
    uint64 size = head_.calcSize();
    return size;
  }
  
  //----------------------------------------------------------------
  // EbmlDoc::isDefault
  // 
  bool
  EbmlDoc::isDefault() const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // EbmlDoc::save
  // 
  IStorage::IReceiptPtr
  EbmlDoc::save(IStorage & storage, Crc32 * computeCrc32) const
  {
    IStorage::IReceiptPtr receipt = head_.save(storage, computeCrc32);
    return receipt;
  }
  
}
