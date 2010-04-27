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
#include <yamkaFileStorage.h>

// system includes:
#include <list>

  
namespace Yamka
{
  
  //----------------------------------------------------------------
  // eltsEval
  // 
  // A helper function for evaluating an element tree crawler
  // over a set of elements
  // 
  template <typename elts_t>
  bool
  eltsEval(elts_t & elts, IElementCrawler & crawler)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::iterator elt_iter_t;
    
    for (elt_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      elt_t & elt = *i;
      if (elt.eval(crawler))
      {
        return true;
      }
    }
    
    return false;
  }
  
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
  eltsSave(const elts_t & elts, IStorage & storage)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::const_iterator const_iter_t;
    
    IStorage::IReceiptPtr receipt = storage.receipt();
    for (const_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      const elt_t & elt = *i;
      *receipt += elt.save(storage);
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
  eltsLoad(elts_t & elts, FileStorage & storage, uint64 bytesToRead)
  {
    typedef typename elts_t::value_type elt_t;
    
    uint64 bytesRead = 0;
    while (bytesToRead)
    {
      elt_t elt;
      uint64 eltSize = elt.load(storage, bytesToRead);
      if (!eltSize)
      {
        break;
      }
      
      elts.push_back(elt);
      bytesRead += eltSize;
      bytesToRead -= eltSize;
    }
    
    return bytesRead;
  }

  //----------------------------------------------------------------
  // eltsFind
  // 
  // Find an element with storage receipt position
  // matching a given position:
  // 
  template <typename elts_t>
  typename elts_t::value_type *
  eltsFind(elts_t & elts, uint64 position)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::iterator elt_iter_t;
    
    for (elt_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      elt_t & elt = *i;
      IStorage::IReceiptPtr receipt = elt.storageReceipt();
      if (receipt)
      {
        uint64 eltPosition = receipt->position();
        if (eltPosition == position)
        {
          return &elt;
        }
      }
    }
    
    return NULL;
  }
  
  //----------------------------------------------------------------
  // eltsSetCrc32
  // 
  // Enable saving element payload CRC-32 checksum
  // 
  template <typename elts_t>
  void
  eltsSetCrc32(elts_t & elts, bool enableCrc32)
  {
    typedef typename elts_t::value_type elt_t;
    typedef typename elts_t::iterator elt_iter_t;
    
    for (elt_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      elt_t & elt = *i;
      elt.setCrc32(enableCrc32);
    }
  }
  
  
  //----------------------------------------------------------------
  // EbmlMaster
  // 
  // A base class for all container elements
  // 
  struct EbmlMaster : public IPayload
  {
    TypedefYamkaElt(VBinary, kIdVoid, "Void") TVoid;
    std::list<TVoid> voids_;
    
    // virtual:
    uint64 loadVoid(FileStorage & storage, uint64 bytesToRead);
    
    // virtual:
    IStorage::IReceiptPtr saveVoid(IStorage & storage) const;
    
    // virtual:
    bool hasVoid() const;
    
    // virtual:
    uint64 calcVoidSize() const;
  };
  
  //----------------------------------------------------------------
  // EbmlHead
  // 
  struct EbmlHead : public EbmlMaster
  {
    EbmlHead();
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(VUInt, 0x4286, "EBMLVersion") TVersion;
    TVersion version_;
    
    TypedefYamkaElt(VUInt, 0x42F7, "EBMLReadVersion") TReadVersion;
    TReadVersion readVersion_;
    
    TypedefYamkaElt(VUInt, 0x42F2, "EBMLMaxIDLength") TMaxIdLength;
    TMaxIdLength maxIdLength_;
    
    TypedefYamkaElt(VUInt, 0x42F3, "EBMLMaxSizeLength") TMaxSizeLength;
    TMaxSizeLength maxSizeLength_;
    
    TypedefYamkaElt(VString, 0x4282, "DocType") TDocType;
    TDocType docType_;
    
    TypedefYamkaElt(VUInt, 0x4287, "DocTypeVersion") TDocTypeVersion;
    TDocTypeVersion docTypeVersion_;
    
    TypedefYamkaElt(VUInt, 0x4285, "DocTypeReadVersion") TDocTypeReadVersion;
    TDocTypeReadVersion docTypeReadVersion_;
  };
  
  //----------------------------------------------------------------
  // EbmlDoc
  // 
  struct EbmlDoc : public EbmlMaster
  {
    EbmlDoc(const char * docType = "",
            uint64 docTypeVersion = 1,
            uint64 docTypeReadVersion = 1);
    
    ImplementsYamkaPayloadAPI();
    
    TypedefYamkaElt(EbmlHead, 0x1A45DFA3, "EBML") THead;
    THead head_;
  };
  
}


#endif // YAMKA_EBML_H_
