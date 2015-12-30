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
  // lookup
  //
  static const ItemFocus::Target *
  lookup(const std::map<std::string, const ItemFocus::Target *> & items,
         const std::string & id)
  {
    std::map<std::string, const ItemFocus::Target *>::const_iterator
      found = items.find(id);

    return found != items.end() ? found->second : NULL;
  }

  //----------------------------------------------------------------
  // ItemFocus::Target::Target
  //
  ItemFocus::Target::Target(Canvas::ILayer * view, Item * item, int index):
    view_(view),
    item_(item),
    index_(index)
  {}

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
  // ItemFocus::ItemFocus
  //
  ItemFocus::ItemFocus():
    focus_(NULL)
  {}

  //----------------------------------------------------------------
  // ItemFocus::setFocusable
  //
  void
  ItemFocus::setFocusable(Canvas::ILayer & view, Item & item, int index)
  {
    Target target(&view, &item, index);

    std::map<int, Target>::iterator found = index_.lower_bound(index);
    if (found == index_.end() ||
        index_.key_comp()(index, found->first))
    {
      // not found:
      found = index_.insert(found, std::make_pair(index, target));
    }
    else if (found->second.item_->id_ != target.item_->id_)
    {
      YAE_ASSERT(false);
      throw std::runtime_error("another item with same index already exists");
    }

    idMap_[item.id_] = &(found->second);
  }

  //----------------------------------------------------------------
  // ItemFocus::clearFocus
  //
  bool
  ItemFocus::clearFocus(const std::string & id)
  {
    if (!(id.empty() || hasFocus(id)))
    {
      return false;
    }

    if (focus_)
    {
      focus_->item_->onFocusOut();
    }

    focus_ = NULL;
    return true;
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocus
  //
  bool
  ItemFocus::setFocus(const std::string & id)
  {
    if (hasFocus(id))
    {
      // already focused:
      return true;
    }

    const Target * target = lookup(idMap_, id);
    if (!target)
    {
      YAE_ASSERT(false);
      throw std::runtime_error("can not give focus to unknown item");
    }

    // Hmm, not sure whether to allow setting focus to an item
    // in a disabled layer...
    //
    // So, allow it, but trigger an assertion in case it happens
    // unintentionally so this could be revisited then:
    YAE_ASSERT(target->view_->isEnabled());

    if (focus_)
    {
      focus_->item_->onFocusOut();
    }

    focus_ = target;
    focus_->item_->onFocus();

    return true;
  }

  //----------------------------------------------------------------
  // advance
  //
  template <typename TKey, typename TData>
  static void
  advance(const std::map<TKey, TData> & index,
          typename std::map<TKey, TData>::const_iterator & iter,
          int n)
  {
    while (n > 0)
    {
      n--;

      if (iter != index.end())
      {
        ++iter;
      }

      if (iter == index.end())
      {
        iter = index.begin();
      }
    }

    while (n < 0)
    {
      n++;

      if (iter == index.begin())
      {
        iter = index.end();
      }

      --iter;
    }
  }

  //----------------------------------------------------------------
  // ItemFocus::focusNext
  //
  bool
  ItemFocus::focusNext()
  {
    std::map<int, Target>::const_iterator iter =
      focus_ ? index_.find(focus_->index_) : index_.end();

    const Target * nextTarget = NULL;
    std::size_t numTargets = index_.size();

    for (std::size_t i = 0; i < numTargets; i++)
    {
      advance(index_, iter, 1);

      const Target & target = iter->second;
      if (target.view_->isEnabled())
      {
        nextTarget = &target;
        break;
      }
    }

    if (nextTarget)
    {
      return setFocus(nextTarget->item_->id_);
    }

    focus_ = nextTarget;
    return false;
  }

  //----------------------------------------------------------------
  // ItemFocus::focusPrevious
  //
  bool
  ItemFocus::focusPrevious()
  {
    std::map<int, Target>::const_iterator iter =
      focus_ ? index_.find(focus_->index_) : index_.begin();

    const Target * nextTarget = NULL;
    std::size_t numTargets = index_.size();

    for (std::size_t i = 0; i < numTargets; i++)
    {
      advance(index_, iter, -1);

      const Target & target = iter->second;
      if (target.view_->isEnabled())
      {
        nextTarget = &target;
        break;
      }
    }

    if (nextTarget)
    {
      return setFocus(nextTarget->item_->id_);
    }

    focus_ = nextTarget;
    return false;
  }

  //----------------------------------------------------------------
  // ItemFocus::hasFocus
  //
  bool
  ItemFocus::hasFocus(const std::string & id) const
  {
    return focus_ && focus_->item_->id_ == id;
  }

  //----------------------------------------------------------------
  // ItemFocus::focusedItem
  //
  Item *
  ItemFocus::focusedItem() const
  {
    return focus_ ? focus_->item_ : NULL;
  }

}
