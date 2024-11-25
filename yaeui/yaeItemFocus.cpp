// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 24 16:18:33 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeItemFocus.h"


namespace yae
{

  //----------------------------------------------------------------
  // ItemFocus::Target::Target
  //
  ItemFocus::Target::Target(Canvas::ILayer * view,
                            Item * item,
                            const char * focusGroup,
                            int index):
    view_(view),
    item_(item->self_),
    index_(std::string(focusGroup), index)
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
  // ItemFocus::removeFocusable
  //
  void
  ItemFocus::removeFocusable(const Item * item)
  {
    const Target * target = yae::get(items_, item, (const Target *)0);
    if (!target)
    {
      return;
    }

    if (focus_ == target)
    {
      focus_ = NULL;
    }

    items_.erase(item);

    TIndex index = target->index_;
    index_.erase(index);
    group_[index.first].erase(index.second);
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocusable
  //
  void
  ItemFocus::setFocusable(Canvas::ILayer & view,
                          Item & item,
                          const char * focusGroup,
                          int index)
  {
    Target target(&view, &item, focusGroup, index);

    std::map<TIndex, Target>::iterator found =
      index_.lower_bound(target.index_);

    if (found == index_.end() || index_.key_comp()(target.index_, found->first))
    {
      // not found:
      found = index_.insert(found, std::make_pair(target.index_, target));
      group_[target.index_.first].insert(target.index_.second);
    }
    else
    {
      ItemPtr prev = found->second.item_.lock();
      if (prev && prev->id_ != item.id_)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("another item with same focus group index "
                                 "already exists");
      }
      else
      {
        found->second = target;
      }
    }

    items_[&item] = &(found->second);
  }

  //----------------------------------------------------------------
  // ItemFocus::getGroupOffset
  //
  int
  ItemFocus::getGroupOffset(const char * focusGroup) const
  {
    std::string name(focusGroup);
    std::map<std::string, std::set<int> >::const_iterator
      found = group_.find(name);

    if (found == group_.end())
    {
      return 0;
    }

    const std::set<int> & group = found->second;
    if (group.empty())
    {
      return 0;
    }

    int last = *(group.rbegin());
    return last + 1;
  }

  //----------------------------------------------------------------
  // ItemFocus::enable
  //
  void
  ItemFocus::enable(const char * focusGroup, bool enableFocusGroup)
  {
    std::string str(focusGroup);

    if (enableFocusGroup)
    {
      disabled_.erase(str);
    }
    else
    {
      disabled_.insert(str);
    }
  }

  //----------------------------------------------------------------
  // ItemFocus::clearFocus
  //
  bool
  ItemFocus::clearFocus(const Item * item)
  {
    if (item && !items_.empty() && !hasFocus(item))
    {
      return false;
    }

    if (focus_)
    {
      ItemPtr focused_ptr = focus_->item_.lock();
      focus_ = NULL;

      if (focused_ptr)
      {
        Item & focused = *focused_ptr;
        focused.onFocusOut();
        focused.uncache();
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocus
  //
  bool
  ItemFocus::setFocus(const Item * item)
  {
    if (hasFocus(item))
    {
      // already focused:
      return true;
    }

    const Target * target = yae::get(items_, item, (const Target *)0);
    if (!target)
    {
      YAE_ASSERT(false);
      throw std::runtime_error("can not give focus to unknown item");
    }

    return setFocus(*target);
  }

  //----------------------------------------------------------------
  // ItemFocus::setFocus
  //
  bool
  ItemFocus::setFocus(const Target & target)
  {
    // Hmm, not sure whether to allow setting focus to an item
    // in a disabled layer...
    //
    // So, allow it, but trigger an assertion in case it happens
    // unintentionally so this could be revisited then:
    YAE_ASSERT(target.view_->isEnabled());

    if (focus_)
    {
      ItemPtr itemPtr = focus_->item_.lock();
      focus_ = NULL;

      if (itemPtr)
      {
        Item & item = *itemPtr;
        item.onFocusOut();
      }
    }

    ItemPtr itemPtr = target.item_.lock();
    if (!itemPtr)
    {
      YAE_ASSERT(false);
      clearFocus();
      return false;
    }

    focus_ = &target;
    Item & item = *itemPtr;
    item.onFocus();

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
    std::map<TIndex, Target>::const_iterator iter =
      focus_ ? index_.find(focus_->index_) : index_.end();

    std::size_t numTargets = index_.size();
    for (std::size_t i = 0; i < numTargets; i++)
    {
      advance(index_, iter, 1);

      const Target & target = iter->second;
      if (!target.view_->isEnabled())
      {
        continue;
      }

      if (yae::has(disabled_, target.index_.first))
      {
        continue;
      }

      ItemPtr itemPtr = target.item_.lock();
      if (itemPtr)
      {
        return setFocus(target);
      }
    }

    clearFocus();
    return false;
  }

  //----------------------------------------------------------------
  // ItemFocus::focusPrevious
  //
  bool
  ItemFocus::focusPrevious()
  {
    std::map<TIndex, Target>::const_iterator iter =
      focus_ ? index_.find(focus_->index_) : index_.begin();

    const std::size_t numTargets = index_.size();
    for (std::size_t i = 0; i < numTargets; i++)
    {
      advance(index_, iter, -1);

      const Target & target = iter->second;
      if (!target.view_->isEnabled())
      {
        continue;
      }

      if (yae::has(disabled_, target.index_.first))
      {
        continue;
      }

      ItemPtr itemPtr = target.item_.lock();
      if (itemPtr)
      {
        return setFocus(target);
      }
    }

    clearFocus();
    return false;
  }

  //----------------------------------------------------------------
  // ItemFocus::hasFocus
  //
  bool
  ItemFocus::hasFocus(const Item * item) const
  {
    ItemPtr focused_item_ptr = focusedItem();
    return focused_item_ptr && (focused_item_ptr.get() == item);
  }

  //----------------------------------------------------------------
  // ItemFocus::focusedItem
  //
  ItemPtr
  ItemFocus::focusedItem() const
  {
    return focus_ ? focus_->item_.lock() : ItemPtr();
  }

}
