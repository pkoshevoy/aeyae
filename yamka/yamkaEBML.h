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
#include <yamkaEltNames.h>
#include <yamkaPayload.h>
#include <yamkaCrc32.h>

// system includes:
#include <deque>

  
namespace Yamka
{
  
  //----------------------------------------------------------------
  // EbmlHead
  // 
  struct EbmlHead
  {
    EbmlHead();
    
    uint64 calcSize() const;
    bool isDefault() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
    
    Elt<VUInt, 0x4286, kEBMLVersion> version_;
    Elt<VUInt, 0x42F7, kEBMLReadVersion> readVersion_;
    Elt<VUInt, 0x42F2, kEBMLMaxIDLength> maxIdLength_;
    Elt<VUInt, 0x42F3, kEBMLMaxSizeLength> maxSizeLength_;
    Elt<VString, 0x4282, kDocType> docType_;
    Elt<VUInt, 0x4287, kDocTypeVersion> docTypeVersion_;
    Elt<VUInt, 0x4285, kDocTypeReadVersion> docTypeReadVersion_;
  };
  
  //----------------------------------------------------------------
  // EbmlDoc
  // 
  struct EbmlDoc
  {
    EbmlDoc();
    
    uint64 calcSize() const;
    bool isDefault() const;
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * computeCrc32 = NULL) const;
    
    Elt<EbmlHead, 0x1A45DFA3, kEBML> head_;
    std::deque<Elt<VBinary, 0xEC, kVoid> > voids_;
  };
}


#endif // YAMKA_EBML_H_
