// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Feb  7 23:03:01 MST 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaFileStorage.h>
#include <yamkaMixedElements.h>

// system includes:
#include <list>
#include <map>

  
namespace Yamka
{

  //----------------------------------------------------------------
  // MixedElements::~MixedElements
  // 
  MixedElements::~MixedElements()
  {
    clear();
  }
  
  //----------------------------------------------------------------
  // MixedElements::MixedElements
  // 
  MixedElements::MixedElements()
  {}
  
  //----------------------------------------------------------------
  // MixedElements::MixedElements
  // 
  MixedElements::MixedElements(const MixedElements & eltMix)
  {
    *this = eltMix;
  }
  
  //----------------------------------------------------------------
  // MixedElements::operator =
  // 
  MixedElements &
  MixedElements::operator = (const MixedElements & eltMix)
  {
    if (this == &eltMix)
    {
      return *this;
    }
    
    clear();
    
    create_ = eltMix.create_;
    createCopy_ = eltMix.createCopy_;
    
    for (std::list<IElement *>::const_iterator i = eltMix.elts_.begin();
         i != eltMix.elts_.end(); ++i)
    {
      const IElement * elt = *i;
      uint64 id = elt->getId();
      
      TElementCreateCopy createCopy = createCopy_[id];
      IElement * copy = createCopy(elt);
      
      if (!copy)
      {
        assert(false);
      }
      else
      {
        elts_.push_back(copy);
      }
    }
    
    return *this;
  }
  
  //----------------------------------------------------------------
  // MixedElements::clear
  // 
  void
  MixedElements::clear()
  {
    while (!elts_.empty())
    {
      IElement * elt = elts_.front();
      delete elt;
      elts_.pop_front();
    }
  }
  
  //----------------------------------------------------------------
  // MixedElements::eval
  // 
  bool
  MixedElements::eval(IElementCrawler & crawler) const
  {
    for (std::list<IElement *>::const_iterator i = elts_.begin();
         i != elts_.end(); ++i)
    {
      IElement & elt = *(*i);
      if (crawler.eval(elt))
      {
        return true;
      }
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // MixedElements::calcSize
  // 
  uint64
  MixedElements::calcSize() const
  {
    uint64 size = 0;
    for (std::list<IElement *>::const_iterator i = elts_.begin();
         i != elts_.end(); ++i)
    {
      IElement & elt = *(*i);
      size += elt.calcSize();
    }
    
    return size;
  }
  
  //----------------------------------------------------------------
  // MixedElements::save
  // 
  IStorage::IReceiptPtr
  MixedElements::save(IStorage & storage) const
  {
    IStorage::IReceiptPtr receipt = storage.receipt();
    for (std::list<IElement *>::const_iterator i = elts_.begin();
         i != elts_.end(); ++i)
    {
      IElement & elt = *(*i);
      *receipt += elt.save(storage);
    }
    
    return receipt;
  }
  
  //----------------------------------------------------------------
  // MixedElements::load
  // 
  uint64
  MixedElements::load(FileStorage & storage,
                      uint64 bytesToRead,
                      IDelegateLoad * loader)
  {
    uint64 bytesRead = 0;
    while (bytesToRead)
    {
      uint64 eltId = 0;
      {
        File::Seek autoRestorePosition(storage.file_);
        eltId = loadEbmlId(storage);
      }
      
      typedef std::map<uint64, TElementCreate>::const_iterator TFactoryIter;
      TFactoryIter found = create_.find(eltId);
      if (found == create_.end())
      {
        // done
        break;
      }
      
      // shortuct to the factory method:
      TElementCreate create = found->second;
      IElement * elt = create();
      
      uint64 eltSize = elt->load(storage, bytesToRead, loader);
      if (!eltSize)
      {
        delete elt;
        break;
      }
      
      elts_.push_back(elt);
      bytesRead += eltSize;
      bytesToRead -= eltSize;
    }
    
    return bytesRead;
  }
  
}
