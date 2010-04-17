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
#include <deque>
#include <stdexcept>
#include <iostream>
#include <iomanip>


namespace Yamka
{
  
  // forward declarations:
  struct IElement;
  struct IPayload;

  //----------------------------------------------------------------
  // IElementCrawler
  // 
  // Interface for an element tree crawling functor.
  // 
  struct IElementCrawler
  {
    virtual ~IElementCrawler() {}
    
    // NOTE: the crawler should return true when it's done
    // in order to stop:
    virtual bool evalElement(IElement & elt) = 0;
    
    // NOTE: the crawler should return true when it's done
    // in order to stop:
    virtual bool evalPayload(IPayload & payload) = 0;
  };
  
  //----------------------------------------------------------------
  // EbmlGlobalID
  // 
  // Void and CRC-32 elements may occur at any level in an EBML
  // document, therefore every element should know these IDs.
  // 
  enum EbmlGlobalID
  {
    kIdCrc32 = 0xBF,
    kIdVoid = 0xEC
  };

  //----------------------------------------------------------------
  // IElement
  // 
  // EBML element interface
  // 
  struct IElement
  {
    virtual ~IElement() {}
    
    // element EBML ID accessor:
    virtual uint64 getId() const = 0;
    
    // element EBML ID accessor:
    virtual const char * getName() const = 0;
    
    // check whether this element payload must be saved (recursive):
    virtual bool mustSave() const = 0;
    
    // set the flag indicating that this element must be saved
    // even when it holds a default value:
    virtual IElement & alwaysSave() = 0;
    
    // turn on/off CRC-32 wrapper for this element:
    virtual IElement & enableCrc32() = 0;
    
    // accessor to total element size (recursive):
    virtual uint64 calcSize() const = 0;
    
    // save this element to a storage stream,
    // and return a storage receipt.
    // 
    // NOTE: if the element can not be saved due to invalid or
    // insufficient storage, then a NULL storage receipt is returned:
    virtual IStorage::IReceiptPtr
    save(IStorage & storage, Crc32 * parentCrc32 = NULL) const = 0;
    
    // attempt to load an instance of this element from a file,
    // return the number of bytes consumed successfully.
    // 
    // NOTE: the file position is not advanced unless some of the
    // element is loaded successfully:
    virtual uint64
    load(FileStorage & storage, uint64 storageSize, Crc32 * crc = NULL) = 0;
    
    // accessor to this elements storage receipt.
    // 
    // NOTE: storage receipt is set only after an element
    // is saved successfully
    virtual IStorage::IReceiptPtr storageReceipt() const = 0;
    
    // accessor to this elements payload storage receipt.
    // 
    // NOTE: payload storage receipt is set only after
    // an element payload is saved successfully
    virtual IStorage::IReceiptPtr payloadReceipt() const = 0;
    
    // dispose of storage receipts:
    virtual IElement & discardReceipts() = 0;
    
    // perform crawler computation on this element and its payload:
    virtual bool eval(IElementCrawler & crawler) = 0;
  };
  
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
    
    // by default CRC-32 is disabled:
    TElt():
      alwaysSave_(false),
      computeCrc32_(false),
      checksumCrc32_(0)
    {}
    
    // static constant for this element type EBML ID:
    enum EbmlEltID { kId =  EltId };
    
    // static accessor to descriptive name of this element:
    static const char * name()
    { return elt_name_t::getName(); }
    
    // virtual:
    uint64 getId() const
    { return kId; }
    
    // virtual:
    const char * getName() const
    { return elt_name_t::getName(); }
    
    // virtual:
    bool mustSave() const
    { return alwaysSave_ || !payload_.isDefault(); }
    
    // virtual:
    TSelf & alwaysSave()
    {
      alwaysSave_ = true;
      return *this;
    }
    
    // virtual:
    TSelf & enableCrc32()
    {
      computeCrc32_ = true;
      return *this;
    }
    
    // virtual:
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
          uintNumBytes(kIdCrc32) +
          vsizeNumBytes(4) +
          4;
      }
      
      return size;
    }
    
    // virtual:
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
        bytes << uintEncode(kIdCrc32)
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
    
    // virtual:
    uint64
    load(FileStorage & storage, uint64 storageSize, Crc32 * crc = NULL)
    {
      // save a storage receipt so that element position references
      // can be resolved later:
      IStorage::IReceiptPtr storageReceipt = storage.receipt();
      IStorage::IReceiptPtr crc32Receipt;
      
      // save current seek position, so it can be restored if necessary:
      File::Seek storageStart(storage.file_);
      
      // keep track of the number of bytes read successfully:
      uint64 bytesRead = 0;
      Bytes bytesCrc32;
      
      uint64 eltId = loadEbmlId(storage, crc);
      if (eltId == kIdCrc32)
      {
        crc32Receipt = storageReceipt;
        
        uint64 vsizeSize = 0;
        uint64 vsize = vsizeDecode(storage, vsizeSize, crc);
        if (vsize != 4)
        {
          // wrong CRC-32 payload size:
          return 0;
        }
        
        bytesCrc32 = Bytes(4);
        if (!storage.loadAndCalcCrc32(bytesCrc32))
        {
          // failed to load CRC-32 checksum:
          return 0;
        }
        
        // move on to the real element:
        storageReceipt = storage.receipt();
        bytesRead += (uintNumBytes(eltId) +
                      vsizeSize +
                      4);
        
        eltId = loadEbmlId(storage, crc);
      }
      
      if (eltId != kId)
      {
        // element id wrong for my type:
        return 0;
      }
      
#if !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
      Indent::More indentMore;
      {
        File::Seek restore(storage.file_);
        uint64 vsizeSize = 0;
        uint64 vsize = vsizeDecode(storage, vsizeSize);
        std::cout << indent()
                  << std::setw(8) << uintEncode(kId) << " @ " << std::hex
                  << "0x" << storageStart.absolutePosition() << std::dec
                  << " -- " << name()
                  << ", payload " << vsize << " bytes" << std::endl;
      }
#endif
      
      // this appears to be a good payload:
      storageStart.doNotRestore();
      
      // store the storage receipt:
      receipt_ = storageReceipt;
      
      // store the checksum:
      if (!bytesCrc32.empty())
      {
        receiptCrc32_ = crc32Receipt;
        computeCrc32_ = true;
        checksumCrc32_ = (unsigned int)uintDecode(TByteVec(bytesCrc32), 4);
      }
      
      // save the payload storage receipt so that element position references
      // can be resolved later:
      IStorage::IReceiptPtr payloadReceipt = storage.receipt();
      
      bytesRead += uintNumBytes(eltId);
      uint64 payloadSize = payload_.load(storage,
                                         storageSize - bytesRead,
                                         crc);
      if (payloadSize)
      {
        receiptPayload_ = payloadReceipt;
      }
      
      bytesRead += payloadSize;
      
#if !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
      {
        uint64 newSize = calcSize();
        if (newSize != bytesRead)
        {
          std::cout << indent()
                    << std::setw(8) << uintEncode(kId) << " @ " << std::hex
                    << "0x" << storageStart.absolutePosition() << std::dec
                    << " -- WARNING: " << name()
                    << ", loaded size " << bytesRead
                    << ", stored size " << newSize
                    << std::endl;
          newSize = calcSize();
        }
      }
#endif
      
      return bytesRead;
    }
    
    // virtual:
    IStorage::IReceiptPtr storageReceipt() const
    { return receipt_; }
    
    // virtual:
    IStorage::IReceiptPtr payloadReceipt() const
    { return receiptPayload_; }
    
    // virtual:
    TSelf & discardReceipts()
    {
      receiptCrc32_ = IStorage::IReceiptPtr();
      receipt_ = IStorage::IReceiptPtr();
      receiptPayload_ = IStorage::IReceiptPtr();
      return *this;
    }
    
    // virtual:
    bool eval(IElementCrawler & crawler)
    {
      return
        crawler.evalElement(*this) ||
        crawler.evalPayload(payload_);
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
    
    // storage receipts for crc32 element, this element, and the payload:
    mutable IStorage::IReceiptPtr receiptCrc32_;
    mutable IStorage::IReceiptPtr receipt_;
    mutable IStorage::IReceiptPtr receiptPayload_;
  };
  
  
  //----------------------------------------------------------------
  // 
  // NOTE: Currently (2010) C++ templates can not be parameterized
  // with const string literal constants:
  // 
  //   TElt<int, 0x1, "One"> one; // does not compiles
  //
  // 
  // Therefore I added helper C preprocessor macro that
  // works around this limitation by creating a wrapper class
  // to return the string literal, and passing the wrapper
  // class as a template parameter to TElt:
  //
  //   struct EltName0x1 { static const char * getName() { return "One" } };
  //   TElt<int, 0x1, EltName0x1> one; // compiles just fine
  // 
  // 
  // The wrapper macros is used like this:
  // 
  //   TypedefElt(int, 0x1, "One") TOne;
  //   TOne one_;
  // 
  
  //----------------------------------------------------------------
  // TypedefYamkaElt
  // 
  // Helper macro used to declare an element type.
  // 
  // EXAMPLE: TypedefElt(VUInt, 0x4286, "EBMLVersion") TVersion;
  // 
# define TypedefYamkaElt(EltType, EbmlId, Name)                         \
  struct EltName##EbmlId { static const char * getName() { return Name; } }; \
  typedef Yamka::TElt<EltType, EbmlId, EltName##EbmlId>
  
}


#endif // YAMKA_ELT_H_
