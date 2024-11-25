// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 24 16:18:33 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_FOCUS_H_
#define YAE_ITEM_FOCUS_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"

// standard:
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <utility>

// yaeui:
#include "yaeExpression.h"
#include "yaeInputArea.h"
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // ItemFocus
  //
  struct ItemFocus
  {
    static ItemFocus & singleton();

    //----------------------------------------------------------------
    // TIndex
    //
    typedef std::pair<std::string, int> TIndex;

    //----------------------------------------------------------------
    // Target
    //
    struct Target
    {
      Target(Canvas::ILayer * view = NULL,
             Item * item = NULL,
             const char * focusGroup = "",
             int index = std::numeric_limits<int>::max());

      Canvas::ILayer * view_;
      yae::weak_ptr<Item> item_;
      TIndex index_;
    };

    ItemFocus();

    // unregister focusable item so it would no longer receive focus:
    void removeFocusable(const Item * item);

    // register item that will be allowed to receive focus:
    void setFocusable(Canvas::ILayer & view,
                      Item & item,
                      const char * focusGroup,
                      int index);

    int getGroupOffset(const char * focusGroup) const;

    // used to prevent passing focus to items in disabled focus groups:
    void enable(const char * focusGroup, bool enable = true);

    // clears focus from a given item, or any item if the id is empty:
    bool clearFocus(const Item * item = NULL);

    // NOTE: these three will return true even if focus doesn't change
    // in case there is no other suitable focusable item available;
    // false is returned when no focusable items are available at all:
    bool setFocus(const Item * item);
    bool setFocus(const Target & target);
    bool focusNext();
    bool focusPrevious();

    // check whether focus belongs to a given item:
    bool hasFocus(const Item * item) const;

    // retrieve focused item:
    ItemPtr focusedItem() const;

    // retrieve focused item:
    inline const Target * focus() const
    { return focus_; }

  protected:
    // keep track of disabled focus groups, do not move focus
    // to items that are in a disabled focus group:
    std::set<std::string> disabled_;
    std::map<TIndex, Target> index_;
    std::map<const Item *, const Target *> items_;
    std::map<std::string, std::set<int> > group_;

    const Target * focus_;
  };

  //----------------------------------------------------------------
  // ShowWhenFocused
  //
  struct ShowWhenFocused : public TBoolExpr
  {
    ShowWhenFocused(Item & focusProxy, bool show):
      focusProxy_(focusProxy),
      show_(show)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool hasFocus = ItemFocus::singleton().hasFocus(&focusProxy_);
      result = hasFocus ? show_ : !show_;
    }

    Item & focusProxy_;
    bool show_;
  };

  //----------------------------------------------------------------
  // ColorWhenFocused
  //
  struct ColorWhenFocused : public TColorExpr
  {
    ColorWhenFocused(Item & focusProxy):
      focusProxy_(focusProxy)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool hasFocus = ItemFocus::singleton().hasFocus(&focusProxy_);
      if (hasFocus)
      {
        focusProxy_.get(kPropertyColorOnFocusBg, result);
      }
      else
      {
        focusProxy_.get(kPropertyColorNoFocusBg, result);
      }
    }

    Item & focusProxy_;
  };

}


#endif // YAE_ITEM_FOCUS_H_
