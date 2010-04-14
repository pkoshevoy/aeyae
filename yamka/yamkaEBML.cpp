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
  // EbmlPayload::isDefault
  // 
  bool
  EbmlPayload::isDefault() const
  {
    uint64 size = calcSize();
    return size == 0;
  }
  
  //----------------------------------------------------------------
  // EbmlPayload::calcSize
  // 
  // calculate payload size:
  uint64
  EbmlPayload::calcSize() const
  {
    // shortcuts -- I wish I could use typeof(..) or decltype(..) instead
    typedef TypeOfElts(VBinary, kIdVoid, "Void") TVoids;
    typedef TVoids::value_type TVoid;
    
    return eltsCalcSize(voids_);
  }
  
  //----------------------------------------------------------------
  // EbmlPayload::save
  // 
  IStorage::IReceiptPtr
  EbmlPayload::save(IStorage & storage, Crc32 * crc) const
  {
    return eltsSave(voids_, storage, crc);
  }
  
  //----------------------------------------------------------------
  // EbmlPayload::load
  // 
  uint64
  EbmlPayload::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    // shortcut:
    typedef TypeOfElt(VBinary, kIdVoid, "Void") TVoid;
    
    voids_.clear();
    aliens_.clear();
    
    uint64 bytesRead = 0;
    while (storageSize)
    {
      TVoid eltVoid;
      uint64 voidSize = eltVoid.load(storage, storageSize, crc);
      
      if (voidSize)
      {
        voids_.push_back(eltVoid);
        bytesRead += voidSize;
        storageSize -= voidSize;
      }
      else
      {
        VBinary alien;
        alien.load(storage, storageSize, crc);
        bytesRead += storageSize;
        storageSize = 0;
      }
    }
    
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // EbmlPayload::loadVoid
  // 
  uint64
  EbmlPayload::loadVoid(FileStorage & storage, uint64 storageSize, Crc32 * crc)
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
    maxIdLength_.payload_.setDefault(4);
    maxSizeLength_.payload_.setDefault(8);
    version_.alwaysSave();
    docTypeVersion_.alwaysSave();
    docTypeReadVersion_.alwaysSave();
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
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // EbmlHead::load
  // 
  uint64
  EbmlHead::load(FileStorage & storage, uint64 storageSize, Crc32 * crc)
  {
    uint64 bytesToRead = vsizeDecode(storage, crc);
    
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
      
      uint64 bytesRead = prevStorageSize - bytesToRead;
      if (!bytesRead)
      {
        break;
      }
      
      bytesReadTotal += bytesRead;
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
  // EbmlDoc::calcSize
  // 
  uint64
  EbmlDoc::calcSize() const
  {
    uint64 size = head_.calcSize() + EbmlPayload::calcSize();
    
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
  EbmlDoc::save(IStorage & storage, Crc32 * crc) const
  {
    IStorage::IReceiptPtr receipt = head_.save(storage, crc);
    return receipt;
  }
  
}
