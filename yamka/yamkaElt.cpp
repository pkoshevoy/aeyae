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
  // IElement::kUndefinedOffset
  //
  const uint64 IElement::kUndefinedOffset = (uint64)(~0);

  //----------------------------------------------------------------
  // IElement::IElement
  //
  IElement::IElement():
    storageFlags_(0),
    checksumCrc32_(0),
    offsetToPayload_(IElement::kUndefinedOffset),
    offsetToCrc32_(IElement::kUndefinedOffset),
    fixedSize_(0)
  {}

  //----------------------------------------------------------------
  // IElement::mustSave
  //
  bool
  IElement::mustSave() const
  {
    const IPayload & payload = getPayload();
    return (fixedSize_ ||
            (storageFlags_ & kAlwaysSave) ||
            !payload.isDefault() ||
            (payload.isComposite() && payload.hasVoid()));
  }

  //----------------------------------------------------------------
  // IElement::alwaysSave
  //
  IElement &
  IElement::alwaysSave()
  {
    storageFlags_ |= kAlwaysSave;
    return *this;
  }

  //----------------------------------------------------------------
  // IElement::setCrc32
  //
  IElement &
  IElement::setCrc32(bool enableCrc32)
  {
    if (enableCrc32 && getPayload().isComposite())
    {
      storageFlags_ |= kComputeCrc32;
    }
    else
    {
      storageFlags_ &= ~kComputeCrc32;
    }

    return *this;
  }

  //----------------------------------------------------------------
  // IElement::shouldComputeCrc32
  //
  bool
  IElement::shouldComputeCrc32() const
  {
    return (storageFlags_ & kComputeCrc32) && getPayload().isComposite();
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

    // include attached Void elements:
    typedef std::list<IPayload::TVoid>::const_iterator TVoidIter;
    for (TVoidIter i = payload.voids_.begin();
         i != payload.voids_.end(); ++i)
    {
      const IPayload::TVoid & eltVoid = *i;
      uint64 voidSize = eltVoid.calcSize();
      size += voidSize;
    }

    return size;
  }

  //----------------------------------------------------------------
  // IElement::setFixedSize
  //
  void
  IElement::setFixedSize(uint64 fixedSize)
  {
    fixedSize_ = fixedSize;
  }

  //----------------------------------------------------------------
  // IElement::savePaddedUpToSize
  //
  IStorage::IReceiptPtr
  IElement::savePaddedUpToSize(IStorage & storage, uint64 paddedSize) const
  {
    // shortcut:
    const IPayload & payload = getPayload();

    const bool payloadSizeUnknown = (paddedSize == uintMax[8]);
    uint64 elementIdSize = uintNumBytes(getId());
    uint64 payloadSize = payload.calcSize();
    uint64 vsizeBytesToUse = vsizeNumBytes(payloadSize);

    // make sure the paddedSize is big enough to store the payload:
    uint64 elementSize = elementIdSize + vsizeBytesToUse + payloadSize;
    if (!payloadSizeUnknown &&
        elementSize != paddedSize &&
        elementSize + kMinVoidEltSize > paddedSize)
    {
      return IStorage::IReceiptPtr();
    }

    // save element id:
    unsigned char v[8];
    uintEncode(getId(), v, elementIdSize);
    receipt_ = storage.save(v, (std::size_t)elementIdSize);
    if (!receipt_)
    {
      return receipt_;
    }

    // save payload size:
    IStorage::IReceiptPtr payloadSizeReceipt;
    if (payloadSizeUnknown)
    {
      vsizeEncode(vsizeUnknown[8], v, 8);
      payloadSizeReceipt = storage.save(v, 8);
    }
    else
    {
      vsizeEncode(payloadSize, v, vsizeBytesToUse);
      payloadSizeReceipt = storage.save(v, (std::size_t)vsizeBytesToUse);
    }

    *receipt_ += payloadSizeReceipt;

    // save payload receipt:
    offsetToPayload_ = storage.receipt()->position() - receipt_->position();
    offsetToCrc32_ = kUndefinedOffset;

    // save the payload:
    IStorage::IReceiptPtr payloadReceipt = payload.save(storage);
    if (!payloadReceipt)
    {
      return payloadReceipt;
    }

    *receipt_ += payloadReceipt;

    if (payloadSizeUnknown)
    {
      // done:
      return receipt_;
    }

    uint64 payloadBytesSaved = payloadReceipt->numBytes();
    assert(payloadBytesSaved <= payloadSize);

    if (payloadBytesSaved < payloadSize)
    {
      // save exact payload size:
      vsizeEncode(payloadBytesSaved, v, vsizeBytesToUse);
      payloadSizeReceipt->save(v, (std::size_t)vsizeBytesToUse);
    }

    // add the padding:
    assert(payload.voids_.empty());

    elementSize = (elementIdSize + vsizeBytesToUse + payloadBytesSaved);
    uint64 voidSize = paddedSize - elementSize;

    if (voidSize >= kMinVoidEltSize)
    {
      IPayload::TVoid eltVoid;

      uint64 voidIdSize = uintNumBytes(eltVoid.getId());
      uint64 vsizeBytesToUse = vsizeNumBytes(voidSize - voidIdSize);

      eltVoid.payload_.set(voidSize - voidIdSize - vsizeBytesToUse);
      IStorage::IReceiptPtr receipt = eltVoid.save(storage, vsizeBytesToUse);

      if (receipt)
      {
        *receipt_ += receipt;
      }
    }

    return receipt_;
  }

  //----------------------------------------------------------------
  // IElement::save
  //
  IStorage::IReceiptPtr
  IElement::save(IStorage & storage, uint64 vsizeBytesToUse) const
  {
    if (fixedSize_)
    {
      return savePaddedUpToSize(storage, fixedSize_);
    }

    if (!mustSave())
    {
      return storage.receipt();
    }

    unsigned char v[8];
    uint64 elementIdSize = uintNumBytes(getId());
    uintEncode(getId(), v, elementIdSize);
    receipt_ = storage.save(v, (std::size_t)elementIdSize);
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

    // must account for CRC-32 element as well:
    if (shouldComputeCrc32())
    {
      payloadSize += uintNumBytes(kIdCrc32) + vsizeNumBytes(4) + 4;
    }

    // if necessary calculate number of bytes used to store the payload size:
    if (!vsizeBytesToUse)
    {
      vsizeBytesToUse = vsizeNumBytes(payloadSize);
    }

    vsizeEncode(payloadSize, v, vsizeBytesToUse);
    IStorage::IReceiptPtr payloadSizeReceipt =
      storage.save(v, (std::size_t)vsizeBytesToUse);
    *receipt_ += payloadSizeReceipt;

    // save payload receipt:
    offsetToPayload_ = elementIdSize + vsizeBytesToUse;
    offsetToCrc32_ = kUndefinedOffset;

    // save CRC-32 element placeholder:
    if (shouldComputeCrc32())
    {
      IStorage::IReceiptPtr receiptCrc32 = storage.receipt();
      TByteVec bytes;
      bytes << uintEncode(kIdCrc32)
            << vsizeEncode(4)
            << uintEncode(0, 4);

      receiptCrc32 = Yamka::save(storage, bytes);
      if (!receiptCrc32)
      {
        return receiptCrc32;
      }

      *receipt_ += receiptCrc32;
      offsetToCrc32_ = offsetToPayload_;
    }

    // save the payload:
    IStorage::IReceiptPtr receiptPayload = payload.save(storage);
    if (!receiptPayload)
    {
      return receiptPayload;
    }

    *receipt_ += receiptPayload;

    uint64 payloadBytesSaved = receipt_->numBytes() - offsetToPayload_;
    assert(payloadBytesSaved <= payloadSize);

    if (payloadBytesSaved < payloadSize)
    {
      // save exact payload size:
      vsizeEncode(payloadBytesSaved, v, vsizeBytesToUse);
      payloadSizeReceipt->save(v, (std::size_t)vsizeBytesToUse);
    }

    // save attached Void elements:
    typedef std::list<IPayload::TVoid>::const_iterator TVoidIter;
    for (TVoidIter i = payload.voids_.begin();
         i != payload.voids_.end(); ++i)
    {
      const IPayload::TVoid & eltVoid = *i;
      IStorage::IReceiptPtr voidReceipt = eltVoid.save(storage);
      *receipt_ += voidReceipt;
    }

    return receipt_;
  }

  //----------------------------------------------------------------
  // FindElement
  //
  struct FindElement : public IElementCrawler
  {
    FindElement(uint64 position):
      position_(position),
      eltFound_(NULL)
    {}

    // virtual:
    bool eval(IElement & elt)
    {
      IStorage::IReceiptPtr receipt = elt.storageReceipt();
      if (receipt)
      {
        uint64 eltStart = receipt->position();
        uint64 eltFinish = eltStart + receipt->numBytes();

        if ((eltStart <= position_) && (position_ < eltFinish))
        {
          eltFound_ = &elt;
          return true;
        }
      }

      return false;
    }

    const uint64 position_;
    IElement * eltFound_;
  };

  //----------------------------------------------------------------
  // IElement::load
  //
  uint64
  IElement::load(FileStorage & storage,
                 uint64 bytesToRead,
                 IDelegateLoad * loader)
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
    Indent::More indentMore(Indent::depth_);
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
    const bool payloadSizeUnknown = (payloadSize == uintMax[8]);

    // keep track of the number of bytes read successfully:
    receipt_->add(uintNumBytes(eltId));
    receipt_->add(vsizeSize);

    // clear the payload checksum:
    setCrc32(false);
    checksumCrc32_ = 0;

    // save the payload storage receipt so that element position references
    // can be resolved later:
    IStorage::IReceiptPtr receiptPayload = storage.receipt();
    offsetToPayload_ = receiptPayload->position() - receipt_->position();
    offsetToCrc32_ = kUndefinedOffset;

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
      uint64 partialPayloadSize = 0;
      if (loader)
      {
        uint64 bytesRead = loader->load(storage,
                                        payloadBytesToRead,
                                        eltId,
                                        payload);
        if (bytesRead == uintMax[8])
        {
          // special case, indicating that the loader doesn't
          // want to read any more data:
          storageStart.doRestore();
          loader->loaded(*this);
          return 0;
        }

        partialPayloadSize += bytesRead;
        payloadBytesToRead -= bytesRead;
      }

      if (!partialPayloadSize)
      {
        uint64 bytesRead = payload.load(storage,
                                        payloadBytesToRead,
                                        loader);
        partialPayloadSize += bytesRead;
        payloadBytesToRead -= bytesRead;
      }

      // consume any void elements that may exist:
      IPayload::TVoid eltVoid;
      uint64 voidPayloadSize = eltVoid.load(storage,
                                            payloadBytesToRead,
                                            loader);
      if (voidPayloadSize)
      {
        payloadBytesToRead -= voidPayloadSize;

        // find an element to store the Void element, so that
        // the relative element order would be preserved:
        IStorage::IReceiptPtr voidReceipt = eltVoid.storageReceipt();
        FindElement crawler(voidReceipt->position() - 2);

        if (partialPayloadSize)
        {
          crawler.evalPayload(payload);
          assert(crawler.eltFound_);
        }

        if (crawler.eltFound_)
        {
          IPayload & dstPayload = crawler.eltFound_->getPayload();
          dstPayload.voids_.push_back(eltVoid);
        }
        else
        {
          payload.voids_.push_back(eltVoid);
        }
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

    if (payloadBytesReadTotal < payloadSize && !payloadSizeUnknown)
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

    receiptPayload->add(payloadBytesReadTotal);
    *receipt_ += receiptPayload;

    // verify stored payload CRC-32 checksum:
    if (shouldComputeCrc32())
    {
      IStorage::IReceiptPtr receiptCrc32 = crc32Receipt();

      Crc32 crc32;
      receiptPayload->calcCrc32(crc32, receiptCrc32);
      unsigned int freshChecksum = crc32.checksum();

      if (freshChecksum != checksumCrc32_)
      {
#if 1 // !defined(NDEBUG) && (defined(DEBUG) || defined(_DEBUG))
        std::cerr << indent() << "WARNING: " << getName()
                  << " 0x" << uintEncode(getId())
                  << " -- checksum mismatch, loaded "
                  << std::hex << checksumCrc32_
                  << ", computed " << freshChecksum
                  << ", CRC-32 @ 0x" << receiptCrc32->position()
                  << ", payload @ 0x" << receiptPayload->position()
                  << ":" << receiptPayload->numBytes()
                  << std::dec
                  << std::endl;

        Crc32 doOverCrc32;
        receiptPayload->calcCrc32(doOverCrc32, receiptCrc32);
#endif
      }
    }

    if (loader && receipt_->numBytes())
    {
      // allow the delegate to perform post-processing on the loaded element:
      loader->loaded(*this);
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
    IStorage::IReceiptPtr receiptCrc32 = storage.receipt();

    uint64 eltId = loadEbmlId(storage);
    if (eltId != kIdCrc32)
    {
      return 0;
    }

    if (offsetToCrc32_ != kUndefinedOffset)
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

    // load CRC (it is stored in little endian layout)
    TEightByteBuffer crc32LittleEndian(4);

    if (!storage.load(crc32LittleEndian.v_, crc32LittleEndian.n_))
    {
      // failed to load CRC-32 checksum:
      return 0;
    }

    // convert to big-endian layout expected by uintDecode:
    TEightByteBuffer crc32BigEndian = crc32LittleEndian.reverseByteOrder();

    // update the receipt:
    receiptCrc32->add(uintNumBytes(kIdCrc32) + vsizeSize + 4);

    // successfully loaded the CRC-32 element:
    storageStart.doNotRestore();

    setCrc32(true);
    checksumCrc32_ = (unsigned int)uintDecode(crc32BigEndian.v_,
                                              crc32BigEndian.n_);
    offsetToCrc32_ = receiptCrc32->position() - receipt_->position();

    uint64 bytesRead = receiptCrc32->numBytes();
    assert(bytesRead == kCrc32EltSize);

    return bytesRead;
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
  IStorage::IReceiptPtr
  IElement::payloadReceipt() const
  {
    IStorage::IReceiptPtr receiptPayload;

    if (offsetToPayload_ != kUndefinedOffset)
    {
      uint64 elementSize = receipt_->numBytes();
      receiptPayload = receipt_->receipt(offsetToPayload_,
                                         elementSize - offsetToPayload_);
    }

    return receiptPayload;
  }

  //----------------------------------------------------------------
  // IElement::crc32Receipt
  //
  IStorage::IReceiptPtr
  IElement::crc32Receipt() const
  {
    IStorage::IReceiptPtr receiptCrc32;
    if (offsetToCrc32_ != kUndefinedOffset)
    {
      receiptCrc32 = receipt_->receipt(offsetToCrc32_, kCrc32EltSize);
    }

    return receiptCrc32;
  }

  //----------------------------------------------------------------
  // IElement::discardReceipts
  //
  IElement &
  IElement::discardReceipts()
  {
    receipt_ = IStorage::IReceiptPtr();
    offsetToPayload_ = kUndefinedOffset;
    offsetToCrc32_ = kUndefinedOffset;
    return *this;
  }

}
