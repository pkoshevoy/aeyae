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
  // EbmlMaster::loadVoid
  // 
  uint64
  EbmlMaster::loadVoid(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // shortcut:
    typedef TypeOfElt(VBinary, kIdVoid, "Void") TVoid;
    
    TVoid eltVoid;
    uint64 bytesRead = eltVoid.load(storage, storageSize, crc);
    if (bytesRead)
    {
      voids_.push_back(eltVoid);
    }
    
    return bytesRead;
  }
  
  
  //----------------------------------------------------------------
  // EbmlHead::EbmlHead
  // 
  EbmlHead::EbmlHead()
  {
    version_.alwaysSave().payload_.setDefault(1);
    readVersion_.alwaysSave().payload_.setDefault(1);
    maxIdLength_.alwaysSave().payload_.setDefault(4);
    maxSizeLength_.alwaysSave().payload_.setDefault(8);
    version_.alwaysSave();
    docTypeVersion_.alwaysSave();
    docTypeReadVersion_.alwaysSave();
  }

  //----------------------------------------------------------------
  // EbmlHead::eval
  // 
  bool
  EbmlHead::eval(IElementCrawler & crawler)
  {
    return
      version_.eval(crawler) ||
      readVersion_.eval(crawler) ||
      maxIdLength_.eval(crawler) ||
      maxSizeLength_.eval(crawler) ||
      docType_.eval(crawler) ||
      docTypeVersion_.eval(crawler) ||
      docTypeReadVersion_.eval(crawler) ||
      eltsEval(voids_, crawler);
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
      docTypeReadVersion_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }

  //----------------------------------------------------------------
  // EbmlHead::save
  // 
  IStorage::IReceiptPtr
  EbmlHead::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt =
      storage.save(Bytes(vsizeEncode(calcSize())));
    
    version_.save(storage, crc);
    readVersion_.save(storage, crc);
    maxIdLength_.save(storage, crc);
    maxSizeLength_.save(storage, crc);
    docType_.save(storage, crc);
    docTypeVersion_.save(storage, crc);
    docTypeReadVersion_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // EbmlHead::load
  // 
  uint64
  EbmlHead::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 vsizeSize = 0;
    uint64 bytesToRead = vsizeDecode(storage, vsizeSize, crc);
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    
    uint64 bytesReadTotal = 0;
    while (true)
    {
      uint64 prevStorageSize = bytesToRead;

      bytesToRead -= version_.load(storage, bytesToRead, crc);
      bytesToRead -= readVersion_.load(storage, bytesToRead, crc);
      bytesToRead -= maxIdLength_.load(storage, bytesToRead, crc);
      bytesToRead -= maxSizeLength_.load(storage, bytesToRead, crc);
      bytesToRead -= docType_.load(storage, bytesToRead, crc);
      bytesToRead -= docTypeVersion_.load(storage, bytesToRead, crc);
      bytesToRead -= docTypeReadVersion_.load(storage, bytesToRead, crc);
      bytesToRead -= loadVoid(storage, bytesToRead, crc);
      
      uint64 bytesRead = prevStorageSize - bytesToRead;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
    }
    
    if (bytesReadTotal)
    {
      bytesReadTotal += vsizeSize;
    }
    
    return bytesReadTotal;
  }
  
  
  //----------------------------------------------------------------
  // EbmlDoc::EbmlDoc
  // 
  EbmlDoc::EbmlDoc(const char * docType,
                   uint64 docTypeVersion,
                   uint64 docTypeReadVersion)
  {
    head_.payload_.docType_.payload_.set(std::string(docType));
    head_.payload_.docTypeVersion_.payload_.set(docTypeVersion);
    head_.payload_.docTypeReadVersion_.payload_.set(docTypeReadVersion);
  }
  
  //----------------------------------------------------------------
  // EbmlDoc::eval
  // 
  bool
  EbmlDoc::eval(IElementCrawler & crawler)
  {
    return
      head_.eval(crawler) ||
      eltsEval(voids_, crawler);
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
  // EbmlDoc::calcSize
  // 
  uint64
  EbmlDoc::calcSize() const
  {
    uint64 size =
      head_.calcSize() +
      eltsCalcSize(voids_);
    
    return size;
  }
  
  //----------------------------------------------------------------
  // EbmlDoc::save
  // 
  IStorage::IReceiptPtr
  EbmlDoc::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = head_.save(storage, crc);
    eltsSave(voids_, storage, crc);
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // EbmlDoc::load
  // 
  uint64
  EbmlDoc::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 bytesToRead = storageSize;
    
    Bytes oneByte(1);
    uint64 bytesReadTotal = 0;
    
    // skip forward until we load EBML head element:
    while (bytesToRead)
    {
      uint64 headSize = EbmlDoc::head_.load(storage, bytesToRead, crc);
      if (headSize)
      {
        bytesToRead -= headSize;
        bytesReadTotal += headSize;
        break;
      }
      
      storage.load(oneByte);
      bytesToRead--;
    }
    
    return bytesReadTotal;
  }
  
}
