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
#include <yamkaFileStorage.h>

// system includes:
#include <stdexcept>
#include <iostream>
#include <iomanip>


namespace Yamka
{
  
  // forward declarations:
  struct IPayload;
  struct IElement;
  
  //----------------------------------------------------------------
  // EbmlGlobalID
  // 
  // Void and CRC-32 elements may occur at almost any level in an EBML
  // document, therefore every element should know these IDs.
  // 
  enum EbmlGlobalID
  {
    kIdCrc32 = 0xBF,
    kIdVoid = 0xEC
  };

  //----------------------------------------------------------------
  // kMinVoidEltSize
  //
  // EC80 -- the smallest possible Void element (0 bytes payload)
  // 
  enum
  {
    kMinVoidEltSize = 2,
    kCrc32EltSize = 6,
  };

  //----------------------------------------------------------------
  // IDelegateLoad
  // 
  // NOTE: the delegate doesn't have to load the entire payload,
  // or even any part of it.
  // 
  // NOTE: The delegate should return the number of bytes it consumed
  // 
  struct IDelegateLoad
  {
    virtual ~IDelegateLoad() {}
    
    // return number of payload bytes consumed:
    virtual uint64 load(FileStorage & storage,
                        uint64 payloadBytesToRead,
                        uint64 eltId,
                        IPayload & payload) = 0;

    // allow the delegate to perform some post-processing
    // once the element has been successfully loaded:
    virtual void loaded(IElement &)
    {}
  };
  
  //----------------------------------------------------------------
  // IElement
  // 
  // EBML element interface
  // 
  struct IElement
  {
    // by default CRC-32 is disabled:
    IElement();
    
    virtual ~IElement() {}
    
    // element EBML ID accessor:
    virtual uint64 getId() const = 0;
    
    // element name accessor (for debugging):
    virtual const char * getName() const = 0;
    
    // element payload accessor:
    virtual const IPayload & getPayload() const = 0;
    virtual IPayload & getPayload() = 0;
    
    // check whether this element payload must be saved (recursive):
    virtual bool mustSave() const;
    
    // set the flag indicating that this element must be saved
    // even when it holds a default value:
    virtual IElement & alwaysSave();
    
    // turn on/off CRC-32 wrapper for this element:
    virtual IElement & setCrc32(bool enable);
    
    // accessor to total element size (recursive):
    virtual uint64 calcSize() const;

    // set the fixedSize_ variable to control whether this element will
    // be saved to storage with padded size, unknown size, or exact size:
    virtual void setFixedSize(uint64 fixedSize);

    // this is used to preallocate some storage for an element
    // where exact payload size is not yet known.
    // 
    // NOTE: the element will be saved using 8-byte payload size.
    // The combined element id size, payload size, the payload,
    // and padding void element will not exceed the
    // specified padded size.
    //
    // NOTE: CRC-32 will not be saved, existing voids will be discarded
    // and replaced with a padding void
    // 
    // paddedSize = 8 + sizeof(ebml-id) + sizeof(payload) + sizeof(void)
    //
    // NOTE: if paddedSize equals uintMax[8] -- saves the element
    // with "unknown size" per EBML spec, does not pad with a void element.
    // 
    virtual IStorage::IReceiptPtr
    savePaddedUpToSize(IStorage & storage, uint64 paddedSize) const;
    
    // save this element to a storage stream,
    // and return a storage receipt.
    // 
    // NOTE: if the element can not be saved due to invalid or
    // insufficient storage, then a NULL storage receipt is returned:
    virtual IStorage::IReceiptPtr
    save(IStorage & storage, uint64 vsizeBytesToUse = 0) const;
    
    // attempt to load an instance of this element from a file,
    // return the number of bytes consumed successfully.
    // 
    // NOTE: the file position is not advanced unless some of the
    // element is loaded successfully:
    virtual uint64
    load(FileStorage & storage,
         uint64 storageSize,
         IDelegateLoad * loader = NULL);
    
    // accessor to this elements storage receipt.
    // 
    // NOTE: storage receipt is set only after an element
    // is loaded/saved successfully
    virtual const IStorage::IReceiptPtr & storageReceipt() const;
    
    // accessor to this elements payload storage receipt.
    // 
    // NOTE: payload storage receipt is set only after
    // an element payload is loaded/saved successfully
    virtual IStorage::IReceiptPtr payloadReceipt() const;
    
    // accessor to this elements payload CRC-32 checksum storage receipt.
    // 
    // NOTE: payload CRC-32 checksum storage receipt is set only after
    // an element payload is loaded/saved successfully
    virtual IStorage::IReceiptPtr crc32Receipt() const;
    
    // dispose of storage receipts:
    virtual IElement & discardReceipts();
    
    // helper: only composite elements (EBML Master) may hold a CRC-32 element:
    bool shouldComputeCrc32() const;
    
    // helper for loading CRC-32 checksum element:
    uint64 loadCrc32(FileStorage & storage, uint64 bytesToRead);

    // special value used to indcate that payload (or CRC-32 element)
    // position is unknown relative to this elements storage position:
    static const uint64 kUndefinedOffset;
    
    enum
    {
      // this flag indicates that this element must be saved
      // even when it holds a default value:
      kAlwaysSave   = 1 << 0,
      
      // The CRC-32 value represents all the data inside the
      // EBML Master it's contained in, except the CRC32 element itself.
      // It should be placed as the first element in a Master
      // so it applies to all the following elements at that level.
      kComputeCrc32 = 1 << 1
    };
    
    // kAlwaysSave, kComputeCrc32
    unsigned char storageFlags_;
    
    // loaded/computed CRC-32 checksum:
    mutable unsigned int checksumCrc32_;
    
    // storage receipts for this element and the payload (including CRC-32):
    mutable IStorage::IReceiptPtr receipt_;
    mutable uint64 offsetToPayload_;
    mutable uint64 offsetToCrc32_;

    // when fixedSize_ != 0 IElement::savePaddedUpToSize(...) is used
    // when fixedSize_ == 0 IElement::save(...) is used (default)
    uint64 fixedSize_;
  };

  //----------------------------------------------------------------
  // TElementMake
  // 
  typedef IElement * (*TElementCreate)();
  
  //----------------------------------------------------------------
  // TElementCopy
  // 
  typedef IElement * (*TElementCreateCopy)(const IElement *);
  
  //----------------------------------------------------------------
  // TElt
  //
  // This is a template class representing an EBML element
  // The template is parametarized by
  // 
  //   element payload type,
  //   unique element ID
  //   element name type that provides T::getName() API
  // 
  template <typename payload_t,
            unsigned int EltId,
            typename elt_name_t>
  struct TElt : public IElement
  {
    // type accessors:
    typedef payload_t TPayload;
    typedef elt_name_t TName;
    typedef TElt<TPayload, EltId, elt_name_t> TSelf;
    
    // static constant for this element type EBML ID:
    enum EbmlEltID { kId =  EltId };
    
    // static accessor to descriptive name of this element:
    static const char * name()
    { return elt_name_t::getName(); }
    
    // factory method for creating new element instances:
    static TSelf * create()
    { return new TSelf(); }
    
    // factory method for creating copy element instances:
    static TSelf * createCopy(const IElement * elt)
    {
      if (elt->getId() == kId)
      {
        const TSelf * original = (const TSelf *)elt;
        return new TSelf(*original);
      }
      
      assert(false);
      return NULL;
    }
    
    // virtual:
    uint64 getId() const
    { return kId; }
    
    // virtual:
    const char * getName() const
    { return elt_name_t::getName(); }
    
    // virtual:
    const TPayload & getPayload() const
    { return payload_; }
    
    // virtual:
    TPayload & getPayload()
    { return payload_; }
    
    // virtual:
    TSelf & alwaysSave()
    {
      IElement::alwaysSave();
      return *this;
    }
    
    // the contents of this element:
    TPayload payload_;
  };
  
  
  //----------------------------------------------------------------
  // TypedefYamkaElt
  // 
  // This is a helper macro used to declare an element type.
  // 
  // NOTE: Currently (2010) C++ templates can not be parameterized
  // with string literal constants:
  // 
  //   TElt<int, 0x1, "One"> one; // does not compile
  // 
  // 
  // Therefore I added a preprocessor macro that works
  // around this limitation by creating a wrapper class
  // to return the string literal, and passing the wrapper
  // class as a template parameter to TElt:
  //
  //   struct EltName0x1 { static const char * getName() { return "One"; } };
  //   TElt<int, 0x1, EltName0x1> one; // compiles just fine
  // 
  // 
  // The wrapper macro is used like this:
  // 
  //   TypedefYamkaElt(int, 0x1, "One") TOne;
  //   TOne one_;
  // 
# define TypedefYamkaElt(EltType, EbmlId, Name)                         \
  struct EltName##EbmlId { static const char * getName() { return Name; } }; \
  typedef Yamka::TElt<EltType, EbmlId, EltName##EbmlId>
  
}


#endif // YAMKA_ELT_H_
