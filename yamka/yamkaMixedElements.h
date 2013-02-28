// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Tue Feb  7 21:04:57 MST 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAMKA_MIXED_ELEMENTS_H_
#define YAMKA_MIXED_ELEMENTS_H_

// yamka includes:
#include <yamkaElt.h>
#include <yamkaPayload.h>
#include <yamkaFileStorage.h>

// system includes:
#include <list>
#include <map>


namespace Yamka
{

  //----------------------------------------------------------------
  // MixedElements
  //
  struct MixedElements
  {
    // register an element type that may occur in this mix:
    template <typename TElement>
    inline bool addType()
    {
      uint64 id = TElement::kId;
      if (create_[id])
      {
        assert(false);
        return false;
      }

      TElementCreate create = &(TElement::create);
      create_[id] = create;

      TElementCreateCopy createCopy = &(TElement::createCopy);
      createCopy_[id] = createCopy;

      return true;
    }

    // NOTE: destructor calls clear()
    ~MixedElements();

    // default constructor:
    MixedElements();

    // copy constructor:
    MixedElements(const MixedElements & eltMix);

    // assignment operator:
    MixedElements & operator = (const MixedElements & eltMix);

    // accessor:
    inline const std::list<IElement *> & elts() const
    { return elts_; }

    // remove and delete all elements stored here:
    void clear();

    // check whether there these elements must be stored:
    bool mustSave() const;

    // perform crawler computation on the elements stored here:
    bool eval(IElementCrawler & crawler) const;

    // calculate payload size:
    uint64 calcSize() const;

    // save all the elements stored here, return total receipt:
    IStorage::IReceiptPtr save(IStorage & storage) const;

    // attempt to load as many elements as possible,
    // return total number of bytes consumed:
    uint64 load(FileStorage & storage,
                uint64 bytesToRead,
                IDelegateLoad * loader);

    // try to load one element, but do not add it to the mix.
    // pass back a pointer to the loaded element,
    // return number of bytes consumed to load the element,
    // return 0 if an element could not be loaded:
    uint64 loadOneElement(IElement *& elt,
                          FileStorage & storage,
                          uint64 bytesToRead,
                          IDelegateLoad * loader);

    // attempt to add an element to this mix:
    bool push_back(const IElement & elt);

    // accessor to the last element in this mix:
    template <typename TElement>
    inline TElement & back() const
    {
      TElement * elt = (TElement *)(elts_.back());
      return *elt;
    }

    // accessor to the counter of elements stored here:
    std::size_t getCount() const;

  protected:
    // a map from element id to factory method:
    std::map<uint64, TElementCreate> create_;
    std::map<uint64, TElementCreateCopy> createCopy_;

    // an ordered list of elements:
    std::list<IElement *> elts_;
    std::size_t count_;
  };

}


#endif // YAMKA_MIXED_ELEMENTS_H_
