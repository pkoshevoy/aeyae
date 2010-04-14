// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 12:58:26 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_EBML_H_
#define YAMKA_EBML_H_

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaCrc32.h>
#include <yamkaFileStorage.h>

// system includes:
#include <deque>

  
namespace Yamka
{
  
  //----------------------------------------------------------------
  // declareEbmlPayloadAPI
  /*
    
    // check whether payload holds default value:
    bool isDefault() const;
    
    // calculate payload size:
    uint64 calcSize() const;
    
    // save the payload and return storage receipt:
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * crc32 = NULL) const;
    
    // attempt to load the payload, return number of bytes read successfully:
    uint64
    load(FileStorage & storage, uint64 storageSize, Crc32 * crc32 = NULL);
    
  */
# define declareEbmlPayloadAPI()                                        \
  bool isDefault() const;                                               \
  uint64 calcSize() const;                                              \
  IStorage::IReceiptPtr save(IStorage & storage,                        \
                             Crc32 * computeCrc32 = NULL) const;        \
  uint64 load(FileStorage & storage,                                    \
              uint64 storageSize,                                       \
              Crc32 * computeCrc32 = NULL)
  
  //----------------------------------------------------------------
  // eltsCalcSize
  // 
  // A helper function for calculating payload size of a set of elements
  // 
  template <typename elts_t>
  uint64
  eltsCalcSize(const elts_t & elts)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::const_iterator const_iter_t;
    
    uint64 size = 0;
    for (const_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      const elt_t & elt = *i;
      size += elt.calcSize();
    }
    
    return size;
  }
  
  //----------------------------------------------------------------
  // eltsSave
  // 
  // A helper function for saving a set of elements
  // 
  template <typename elts_t>
  IStorage::IReceiptPtr
  eltsSave(const elts_t & elts,
           IStorage & storage,
           Crc32 * crc)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::const_iterator const_iter_t;
    
    IStorage::IReceiptPtr receipt = storage.receipt();
    for (const_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      const elt_t & elt = *i;
      elt.save(storage, crc);
    }
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // eltsLoad
  // 
  // A helper function for loading a set of elements
  // 
  template <typename elts_t>
  uint64
  eltsLoad(elts_t & elts,
           FileStorage & storage,
           uint64 storageSize,
           Crc32 * crc)
  {
    typedef typename elts_t::value_type elt_t;
    
    elts.clear();
    uint64 bytesRead = 0;
    while (storageSize)
    {
      elt_t elt;
      uint64 eltSize = elt.load(storage, storageSize, crc);
      if (!eltSize)
      {
        break;
      }
      
      elts.push_back(elt);
      bytesRead += eltSize;
      storageSize -= eltSize;
    }
    
    return bytesRead;
  }
  
  //----------------------------------------------------------------
  // EbmlPayload
  // 
  // A helper base class used by all container elements
  // to store Void elements and unrecognized alien data
  // 
  struct EbmlPayload
  {
    Elts(VBinary, kIdVoid, "Void") voids_;
    std::deque<VBinary> aliens_;
    
  protected:
    declareEbmlPayloadAPI();
    
    // attempt to load a void element:
    uint64 loadVoid(FileStorage & storage, uint64 storageSize, Crc32 * crc);
  };
  
  //----------------------------------------------------------------
  // EbmlHead
  // 
  struct EbmlHead : public EbmlPayload
  {
    EbmlHead();
    
    declareEbmlPayloadAPI();
    
    Elt(VUInt, 0x4286, "EBMLVersion") version_;
    Elt(VUInt, 0x42F7, "EBMLReadVersion") readVersion_;
    Elt(VUInt, 0x42F2, "EBMLMaxIDLength") maxIdLength_;
    Elt(VUInt, 0x42F3, "EBMLMaxSizeLength") maxSizeLength_;
    Elt(VString, 0x4282, "DocType") docType_;
    Elt(VUInt, 0x4287, "DocTypeVersion") docTypeVersion_;
    Elt(VUInt, 0x4285, "DocTypeReadVersion") docTypeReadVersion_;
  };
  
  //----------------------------------------------------------------
  // EbmlDoc
  // 
  struct EbmlDoc : public EbmlPayload
  {
    EbmlDoc(const char * docType = "",
            uint64 docTypeVersion = 1,
            uint64 docTypeReadVersion = 1);
    
    declareEbmlPayloadAPI();
    
    Elt(EbmlHead, 0x1A45DFA3, "EBML") head_;
  };
  
}


#endif // YAMKA_EBML_H_
