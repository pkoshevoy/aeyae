// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 10 12:54:55 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_ELT_H_
#define YAMKA_ELT_H_

// yamka includes:
#include <yamkaIStorage.h>
#include <yamkaStdInt.h>

// system includes:
#include <deque>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // TElt
  // 
  template <typename payload_t,
            unsigned int EltId,
            typename elt_name_t>
  struct TElt
  {
    typedef payload_t TPayload;
    typedef TElt<TPayload, EltId, elt_name_t> TSelf;
    
    //----------------------------------------------------------------
    // EbmlID
    // 
    enum EbmlID
    {
      EbmlIdCrc32 = 0xBF,
      EbmlIdVoid = 0xEC
    };
    
    static unsigned int id()
    { return EltId; }
    
    static const char * name()
    { return elt_name_t::getName(); }
    
    TElt():
      alwaysSave_(false),
      computeCrc32_(false)
    {}
    
    TSelf & enableCrc32(bool enable)
    {
      computeCrc32_ = true;
      return *this;
    }
    
    uint64 calcSize() const
    {
      if (!mustSave())
      {
        return 0;
      }
      
      // the payload size:
      uint64 payloadSize = payload_.calcSize();
      
      // the EBML ID, payload size descriptor, and payload size:
      uint64 size =
        uintNumBytes(EltId) +
        vsizeNumBytes(payloadSize) +
        payloadSize;
      
      if (computeCrc32_)
      {
        // CRC-32 element size:
        size +=
          uintNumBytes(EbmlIdCrc32) +
          vsizeNumBytes(4) +
          4;
      }
      
      return size;
    }
    
    IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * parentCrc32 = NULL) const
    {
      if (!mustSave())
      {
        return storage.receipt();
      }
      
      if (computeCrc32_)
      {
        Bytes bytes;
        bytes << uintEncode(EbmlIdCrc32)
              << vsizeEncode(4)
              << uintEncode(0, 4);
        
        receiptCrc32_ = storage.save(bytes);
        if (!receiptCrc32_)
        {
          return receiptCrc32_;
        }
      }
      
      Bytes bytes;
      bytes << uintEncode(EltId);
      
      Crc32 eltCrc32;
      Crc32 * crc32 = computeCrc32_ ? &eltCrc32 : parentCrc32;
      
      receipt_ = storage.saveAndCalcCrc32(bytes, crc32);
      if (!receipt_)
      {
        return receipt_;
      }
      
      receiptPayload_ = payload_.save(storage, crc32);
      if (!receiptPayload_)
      {
        return receiptPayload_;
      }
      
      if (computeCrc32_)
      {
        checksumCrc32_ = eltCrc32.checksum();
        return receiptCrc32_;
      }
      
      return receipt_;
    }
    
    bool load(IStorage & storage)
    {
      // FIXME: write me:
      // load the payload size, load the payload:
      return false;
    }
    
    // check whether this element payload holds a default value:
    bool mustSave() const
    {
      return alwaysSave_ || !payload_.isDefault();
    }
    
    // set the flag indicating that this element must be saved
    // even when it holds a default value:
    TSelf & alwaysSave()
    {
      alwaysSave_ = true;
      return *this;
    }
    
    // this flag indicates that this element must be saved
    // even when it holds a default value:
    bool alwaysSave_;
    
    // the contents of this element:
    TPayload payload_;
    
    // The CRC32 container can be placed around any EBML element or
    // elements. The value stored in CRC32Value is the result of the
    // CRC-32 [CRC32] checksum performed on the other child elements.
    // 
    // CRC32 := c3 container [ level:1..; card:*; ] {
    //   %children;
    //   CRC32Value := 42fe binary [ size:4; ]
    // }
    bool computeCrc32_;
    
    // loaded/computed CRC-32 checksum:
    mutable unsigned int checksumCrc32_;
    
    // storage receipt for crc32 and this element:
    mutable IStorage::IReceiptPtr receiptCrc32_;
    mutable IStorage::IReceiptPtr receipt_;
    mutable IStorage::IReceiptPtr receiptPayload_;
  };
  
  
  //----------------------------------------------------------------
  // TElts
  // 
  template <typename payload_t,
            unsigned int EltId,
            typename elt_name_t>
  struct TElts : public std::deque<TElt<payload_t, EltId, elt_name_t> >
  {};
  
  //----------------------------------------------------------------
  // Elt
  // 
  // Helper macro used to declare an element.
  // 
  // EXAMPLE: Elt(VUInt, 0x4286, "EBMLVersion") version_;
  //       
# define Elt(EltType, EbmlId, Name)                                    \
  struct EltName##EbmlId { static const char * getName() { return Name; } }; \
  TElt<EltType, EbmlId, EltName##EbmlId>
  
  //----------------------------------------------------------------
  // Elts
  //
  // Helper macro used to declare an element list
  // 
  // EXAMPLE: Elts(VUInt, 0x4286, "EBMLVersion") version_;
  //       
# define Elts(EltType, EbmlId, Name)                                   \
  struct EltType##EbmlId { static const char * getName() { return Name; } }; \
  TElts<EltType, EbmlId, EltType##EbmlId>
  
}


#endif // YAMKA_ELT_H_
