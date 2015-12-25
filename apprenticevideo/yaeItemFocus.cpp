// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 24 16:18:33 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// local interfaces:
#include "yaeItemFocus.h"


namespace yae
{

  //----------------------------------------------------------------
  // ItemFocus::singleton
  //
  ItemFocus &
  ItemFocus::singleton()
  {
    static ItemFocus focusManager;
    return focusManager;
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocusable
  //
  void
  ItemFocus::setFocusable(Item & item, int priority)
  {
    std::map<int, std::string>::iterator
      found = priority_.lower_bound(priority);

    if (found == priority_.end() ||
        priority_.key_comp()(priority, found->first))
    {
      // not found:
      found = priority_.insert(found, std::make_pair(priority, item.id_));
    }
    else if (found->second != item.id_)
    {
      YAE_ASSERT(false);
      throw std::runtime_error("another item with equal priority exists");
    }

    item_[item.id_] = std::make_pair(&item, priority);
  }

  //----------------------------------------------------------------
  // lookup
  //
  static const std::pair<Item *, int> *
  lookup(const std::map<std::string, std::pair<Item *, int> > & items,
         const std::string & id)
  {
    std::map<std::string, std::pair<Item *, int> >::const_iterator
      found = items.find(id);

    return (found == items.end()) ? NULL : &(found->second);
  }

  //----------------------------------------------------------------
  // lookupItem
  //
  static Item *
  lookupItem(const std::map<std::string, std::pair<Item *, int> > & items,
             const std::string & id)
  {
    const std::pair<Item *, int> * found = lookup(items, id);
    return found ? found->first : NULL;
  }

  //----------------------------------------------------------------
  // ItemFocus::clearFocus
  //
  void
  ItemFocus::clearFocus(const std::string & id)
  {
    if (id != focus_ && !id.empty())
    {
      return;
    }

    Item * prev = lookupItem(item_, focus_);
    if (prev)
    {
      prev->onFocusOut();
    }

    focus_.clear();
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocus
  //
  bool
  ItemFocus::setFocus(const std::string & id)
  {
    if (id == focus_)
    {
      return true;
    }

    Item * prev = lookupItem(item_, focus_);
    if (prev)
    {
      prev->onFocusOut();
    }

    Item * item = lookupItem(item_, id);
    if (!item)
    {
      YAE_ASSERT(false);
      throw std::runtime_error("can not set focus to unknown item");
    }

    focus_ = id;
    item->onFocus();
    return true;
  }

  //----------------------------------------------------------------
  // ItemFocus::focusNext
  //
  bool
  ItemFocus::focusNext()
  {
    if (priority_.empty())
    {
      return false;
    }

    bool unfocused = focus_.empty();
    if (!unfocused && priority_.size() < 2)
    {
      return !unfocused;
    }

    std::string id = priority_.begin()->second;
    const std::pair<Item *, int> * found = lookup(item_, focus_);

    if (found)
    {
      int priority = found->second;

      std::map<int, std::string>::iterator
        next = priority_.upper_bound(priority);

      if (next != priority_.end())
      {
        id = next->second;
      }
    }

    return setFocus(id);
  }

  //----------------------------------------------------------------
  // ItemFocus::focusPrevious
  //
  bool
  ItemFocus::focusPrevious()
  {
    if (priority_.empty())
    {
      return false;
    }

    bool unfocused = focus_.empty();
    if (!unfocused && priority_.size() < 2)
    {
      return !unfocused;
    }

    std::string id = priority_.begin()->second;
    const std::pair<Item *, int> * found = lookup(item_, focus_);

    if (found)
    {
      int priority = found->second;

      std::map<int, std::string>::iterator next = priority_.find(priority);

      if (next != priority_.begin())
      {
        --next;
        id = next->second;
      }
      else
      {
        id = priority_.rbegin()->second;
      }
    }

    return setFocus(id);
  }

  //----------------------------------------------------------------
  // ItemFocus::hasFocus
  //
  bool
  ItemFocus::hasFocus(const std::string & id) const
  {
    return focus_ == id;
  }

  //----------------------------------------------------------------
  // ItemFocus::focusedItem
  //
  Item *
  ItemFocus::focusedItem() const
  {
    return lookupItem(item_, focus_);
  }

}
