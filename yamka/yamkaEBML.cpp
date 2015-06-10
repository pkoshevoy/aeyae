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
  // eltsFind
  //
  IElement *
  eltsFind(const std::list<IElement *> & elts, uint64 position)
  {
    typedef std::list<IElement *>::const_iterator elt_iter_t;

    for (elt_iter_t i = elts.begin(); i != elts.end(); ++i)
    {
      IElement * elt = *i;
      IStorage::IReceiptPtr receipt = elt->storageReceipt();
      if (receipt)
      {
        uint64 eltPosition = receipt->position();
        if (eltPosition == position)
        {
          return elt;
        }
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // EbmlMaster::isComposite
  //
  bool
  EbmlMaster::isComposite() const
  {
    return true;
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
    docType_.alwaysSave();
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
      crawler.eval(version_) ||
      crawler.eval(readVersion_) ||
      crawler.eval(maxIdLength_) ||
      crawler.eval(maxSizeLength_) ||
      crawler.eval(docType_) ||
      crawler.eval(docTypeVersion_) ||
      crawler.eval(docTypeReadVersion_);
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
      docTypeReadVersion_.calcSize();

    return size;
  }

  //----------------------------------------------------------------
  // EbmlHead::save
  //
  IStorage::IReceiptPtr
  EbmlHead::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();

    *receipt += version_.save(storage);
    *receipt += readVersion_.save(storage);
    *receipt += maxIdLength_.save(storage);
    *receipt += maxSizeLength_.save(storage);
    *receipt += docType_.save(storage);
    *receipt += docTypeVersion_.save(storage);
    *receipt += docTypeReadVersion_.save(storage);

    return receipt;
  }

  //----------------------------------------------------------------
  // EbmlHead::load
  //
  uint64
  EbmlHead::load(IStorage & storage,
                 uint64 bytesToRead,
                 IDelegateLoad * loader)
  {
    uint64 prevBytesToRead = bytesToRead;

    bytesToRead -= version_.load(storage, bytesToRead, loader);
    bytesToRead -= readVersion_.load(storage, bytesToRead, loader);
    bytesToRead -= maxIdLength_.load(storage, bytesToRead, loader);
    bytesToRead -= maxSizeLength_.load(storage, bytesToRead, loader);
    bytesToRead -= docType_.load(storage, bytesToRead, loader);
    bytesToRead -= docTypeVersion_.load(storage, bytesToRead, loader);
    bytesToRead -= docTypeReadVersion_.load(storage, bytesToRead, loader);

    uint64 bytesRead = prevBytesToRead - bytesToRead;
    return bytesRead;
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
    return crawler.eval(head_);
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
    uint64 size = head_.calcSize();
    return size;
  }

  //----------------------------------------------------------------
  // EbmlDoc::save
  //
  IStorage::IReceiptPtr
  EbmlDoc::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = head_.save(storage);
    return receipt;
  }

  //----------------------------------------------------------------
  // EbmlDoc::load
  //
  uint64
  EbmlDoc::load(IStorage & storage,
                uint64 bytesToRead,
                IDelegateLoad * loader)
  {
    unsigned char oneByte = 0;
    uint64 bytesReadTotal = 0;

    // skip forward until we load EBML head element:
    while (bytesToRead)
    {
      uint64 headSize = EbmlDoc::head_.load(storage, bytesToRead, loader);
      if (headSize)
      {
        bytesToRead -= headSize;
        bytesReadTotal += headSize;
        break;
      }

      storage.load(&oneByte, 1);
      bytesToRead--;
    }

    return bytesReadTotal;
  }

}
