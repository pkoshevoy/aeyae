// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Apr 30 01:38:17 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaFileStorage.h>

// system includes:
#include <stdexcept>
#include <iostream>
#include <iomanip>


namespace Yamka
{
  
  //----------------------------------------------------------------
  // IElement::IElement
  // 
  IElement::IElement():
    alwaysSave_(false),
    computeCrc32_(false),
    checksumCrc32_(0)
  {}
  
  //----------------------------------------------------------------
  // IElement::mustSave
  // 
  bool
  IElement::mustSave() const
  {
    const IPayload & payload = getPayload();
    return (alwaysSave_ ||
            !payload.isDefault() ||
            (payload.isComposite() && payload.hasVoid()));
  }
  
  //----------------------------------------------------------------
  // IElement::alwaysSave
  // 
  IElement &
  IElement::alwaysSave()
  {
    alwaysSave_ = true;
    return *this;
  }
  
  //----------------------------------------------------------------
  // IElement::setCrc32
  // 
  IElement &
  IElement::setCrc32(bool enableCrc32)
  {
    computeCrc32_ = enableCrc32 && getPayload().isComposite();
    return *this;
  }
  
  //----------------------------------------------------------------
  // IElement::shouldComputeCrc32
  // 
  bool
  IElement::shouldComputeCrc32() const
  {
    return computeCrc32_ && getPayload().isComposite();
  }
  
  //----------------------------------------------------------------
  // IElement::calcSize
  // 
  uint64
  IElement::calcSize() const
  {
    if (!mustSave())
    {
      return 0;
    }
      
    // shortcut:
    const IPayload & payload = getPayload();
    
    // the payload size:
    uint64 payloadSize = payload.calcSize();
    if (payload.isComposite())
    {
      payloadSize += payload.calcVoidSize();
    }
    
    if (shouldComputeCrc32())
    {
      // CRC-32 element size:
      payloadSize +=
        uintNumBytes(kIdCrc32) +
        vsizeNumBytes(4) +
        4;
    }
    
    // the EBML ID, payload size descriptor, and payload size:
    uint64 size =
      uintNumBytes(getId()) +
      vsizeNumBytes(payloadSize) +
      payloadSize;
    
    return size;
  }
  
  //----------------------------------------------------------------
  // IElement::save
  // 
  IStorage::IReceiptPtr
  IElement::save(IStorage & storage) const
  {
    if (!mustSave())
    {
      return storage.receipt();
    }
    
    receipt_ = storage.save(Bytes(uintEncode(getId())));
    if (!receipt_)
    {
      return receipt_;
    }
    
    // shortcut:
    const IPayload & payload = getPayload();
    
    // NOTE: due to VEltPosition the payload size is not actually
    // known until the payload is written, however we know the
    // payload size upper limit.  Once the payload is saved it's
    // exact size will have to be saved again:
    uint64 payloadSize = payload.calcSize();
    if (payload.isComposite())
    {
      payloadSize += payload.calcVoidSize();
    }
    
    // must account for CRC-32 element as well:
    if (shouldComputeCrc32())
    {
      payloadSize += uintNumBytes(kIdCrc32) + vsizeNumBytes(4) + 4;
    }
    
    IStorage::IReceiptPtr payloadSizeReceipt =
      storage.save(Bytes(vsizeEncode(payloadSize)));
    *receipt_ += payloadSizeReceipt;
    
    // save payload receipt:
    receiptPayload_ = storage.receipt();
    receiptCrc32_ = IStorage::IReceiptPtr();
    
    // save CRC-32 element placeholder:
    if (shouldComputeCrc32())
    {
      receiptCrc32_ = storage.receipt();
      Bytes bytes;
      bytes << uintEncode(kIdCrc32)
            << vsizeEncode(4)
            << uintEncode(0, 4);
      
      receiptCrc32_ = storage.save(bytes);
      if (!receiptCrc32_)
      {
        return receiptCrc32_;
      }
      
      *receiptPayload_ += receiptCrc32_;
    }
    
    // save the payload:
    IStorage::IReceiptPtr payloadReceipt = payload.save(storage);
    if (!payloadReceipt)
    {
      return payloadReceipt;
    }
    
    *receiptPayload_ += payloadReceipt;
    
    if (payload.isComposite())
    {
      // NOTE: the Void elements always get saved last,
      // therefore they may shift from the original position
      // in the file they were loaded from.  However, this is
      // not considered a bug by me, because any Master element
      // should contain at most just one Void element,
      // as the last element.
      // 
      // That being said -- I have seen Matroska files with
      // Void elements scattered throughout the Segment.
      // Yamka will load such files, but will move all Void elements
      // to the end of the Segment when saving.
      // 
      *receiptPayload_ += payload.saveVoid(storage);
    }
    
    *receipt_ += receiptPayload_;
    
    uint64 payloadBytesSaved = receiptPayload_->numBytes();
    assert(payloadBytesSaved <= payloadSize);
    
    if (payloadBytesSaved < payloadSize)
    {
      // save exact payload size:
      uint64 vsizeBytesUsed = vsizeNumBytes(payloadSize);
      TByteVec v = vsizeEncode(payloadBytesSaved, vsizeBytesUsed);
      payloadSizeReceipt->save(Bytes(v));
    }
    
    return receipt_;
  }
  
  //----------------------------------------------------------------
  // IElement::load
  // 
  uint64
  IElement::load(FileStorage & storage, uint64 bytesToRead)
  {
    if (!bytesToRead)
    {
      return 0;
    }
    
    // save a storage receipt so that element position references
    // can be resolved later:
    IStorage::IReceiptPtr storageReceipt = storage.receipt();
    
    // save current seek position, so it can be restored if necessary:
    File::Seek storageStart(storage.file_);
    
    uint64 eltId = loadEbmlId(storage);
    if (eltId != getId())
    {
      // element id wrong for my type:
      return 0;
    }
    
#if 0 // !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
    Indent::More indentMore;
    {
      File::Seek restore(storage.file_);
      uint64 vsizeSize = 0;
      uint64 vsize = vsizeDecode(storage, vsizeSize);
      std::cout << indent()
                << std::setw(8) << uintEncode(getId()) << " @ "
                << std::hex
                << "0x" << storageStart.absolutePosition()
                << std::dec
                << " -- " << getName()
                << ", payload " << vsize << " bytes" << std::endl;
    }
#endif
    
    // this appears to be a good payload:
    storageStart.doNotRestore();
    
    // store the storage receipt:
    receipt_ = storageReceipt;
    
    // read payload size:
    uint64 vsizeSize = 0;
    uint64 payloadSize = vsizeDecode(storage, vsizeSize);
    
    // keep track of the number of bytes read successfully:
    receipt_->add(uintNumBytes(eltId));
    receipt_->add(vsizeSize);
    
    // clear the payload checksum:
    computeCrc32_ = false;
    checksumCrc32_ = 0;
    
    // save the payload storage receipt so that element position references
    // can be resolved later:
    receiptPayload_ = storage.receipt();
    receiptCrc32_ = IStorage::IReceiptPtr();
    
    // shortcut:
    IPayload & payload = getPayload();
    
    // container elements may be present in any order, therefore
    // not every load will succeed -- keep trying until all
    // load attempts fail:
    uint64 payloadBytesToRead = payloadSize;
    uint64 payloadBytesReadTotal = 0;
    while (payloadBytesToRead)
    {
      uint64 prevPayloadBytesToRead = payloadBytesToRead;
      
      // try to load some part of the payload:
      payloadBytesToRead -= payload.load(storage,
                                         payloadBytesToRead);
      
      if (payload.isComposite())
      {
        // consume any void elements that may exist:
        payloadBytesToRead -= payload.loadVoid(storage,
                                               payloadBytesToRead);
      }
      
      // consume the CRC-32 element if it exists:
      payloadBytesToRead -= loadCrc32(storage,
                                      payloadBytesToRead);
      
      uint64 payloadBytesRead = prevPayloadBytesToRead - payloadBytesToRead;
      payloadBytesReadTotal += payloadBytesRead;
      
      if (payloadBytesRead == 0)
      {
        break;
      }
    }
    
    if (payloadBytesReadTotal < payloadSize)
    {
      // skip unrecognized alien data:
      uint64 alienDataSize = payloadSize - payloadBytesReadTotal;
      
#if !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
      std::cerr << indent() << "WARNING: " << getName()
                << " 0x" << uintEncode(getId())
                << " -- skipping " << alienDataSize
                << " bytes of unrecognized alien data @ 0x"
                << std::hex
                << storage.file_.absolutePosition()
                << std::dec
                << std::endl;
#endif
      
      storage.file_.seek(alienDataSize, File::kRelativeToCurrent);
      payloadBytesReadTotal = payloadSize;
    }
    
    receiptPayload_->add(payloadBytesReadTotal);
    *receipt_ += receiptPayload_;
    
    // verify stored payload CRC-32 checksum:
    if (shouldComputeCrc32())
    {
      Crc32 crc32;
      receiptPayload_->calcCrc32(crc32, receiptCrc32_);
      unsigned int freshChecksum = crc32.checksum();
      
      if (freshChecksum != checksumCrc32_)
      {
#if !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
        std::cerr << indent() << "WARNING: " << getName()
                  << " 0x" << uintEncode(getId())
                  << " -- checksum mismatch, loaded "
                  << std::hex << checksumCrc32_
                  << ", computed " << freshChecksum
                  << ", CRC-32 @ 0x" << receiptCrc32_->position()
                  << ", payload @ 0x" << receiptPayload_->position()
                  << ":" << receiptPayload_->numBytes()
                  << std::dec
                  << std::endl;
        
        Crc32 doOverCrc32;
        receiptPayload_->calcCrc32(doOverCrc32, receiptCrc32_);
#endif
      }
    }
    
    return receipt_->numBytes();
  }
  
  //----------------------------------------------------------------
  // IElement::loadCrc32
  // 
  uint64
  IElement::loadCrc32(FileStorage & storage,
                      uint64 bytesToRead)
  {
    if (!bytesToRead)
    {
      return 0;
    }
    
    // save current seek position, so it can be restored if necessary:
    File::Seek storageStart(storage.file_);
    IStorage::IReceiptPtr receipt = storage.receipt();
    
    uint64 eltId = loadEbmlId(storage);
    if (eltId != kIdCrc32)
    {
      return 0;
    }
    
    if (receiptCrc32_)
    {
      // only one CRC-32 element is allowed per master element:
      assert(false);
      return 0;
    }
    
    uint64 vsizeSize = 0;
    uint64 vsize = vsizeDecode(storage, vsizeSize);
    if (vsize != 4)
    {
      // wrong CRC-32 payload size:
      return 0;
    }
    
    Bytes bytesCrc32(4);
    if (!storage.load(bytesCrc32))
    {
      // failed to load CRC-32 checksum:
      return 0;
    }
    
    // update the receipt:
    receipt->add(uintNumBytes(kIdCrc32) + vsizeSize + 4);
    
    // successfully loaded the CRC-32 element:
    storageStart.doNotRestore();
    
    computeCrc32_ = true;
    checksumCrc32_ = (unsigned int)uintDecode(TByteVec(bytesCrc32), 4);
    receiptCrc32_ = receipt;
    
    return receipt->numBytes();
  }

  //----------------------------------------------------------------
  // IElement::storageReceipt
  // 
  const IStorage::IReceiptPtr &
  IElement::storageReceipt() const
  { return receipt_; }
  
  //----------------------------------------------------------------
  // IElement::payloadReceipt
  // 
  const IStorage::IReceiptPtr &
  IElement::payloadReceipt() const
  { return receiptPayload_; }
  
  //----------------------------------------------------------------
  // IElement::crc32Receipt
  // 
  const IStorage::IReceiptPtr &
  IElement::crc32Receipt() const
  { return receiptCrc32_; }
  
  //----------------------------------------------------------------
  // IElement::discardReceipts
  // 
  IElement &
  IElement::discardReceipts()
  {
    receipt_ = IStorage::IReceiptPtr();
    receiptPayload_ = IStorage::IReceiptPtr();
    return *this;
  }
  
}
