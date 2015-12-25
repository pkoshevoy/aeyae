// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 24 16:18:33 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_FOCUS_H_
#define YAE_ITEM_FOCUS_H_

// standard libraries:
#include <algorithm>
#include <map>

// boost includes:
#include <boost/shared_ptr.hpp>

// local interfaces:
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

    // register item that will be allowed to receive focus:
    void setFocusable(Item & item, int priority);

    void clearFocus(const std::string & id = std::string());
    bool setFocus(const std::string & id);
    bool focusNext();
    bool focusPrevious();

    // check whether focus belongs to an item with a given id:
    bool hasFocus(const std::string & id) const;

    // retrieve focused item:
    Item * focusedItem() const;

  protected:
    std::map<std::string, std::pair<Item *, int> > item_;
    std::map<int, std::string> priority_;
    std::string focus_;
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
      bool hasFocus = ItemFocus::singleton().hasFocus(focusProxy_.id_);
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
    ColorWhenFocused(Item & focusProxy,
                     const ColorRef & noFocus,
                     const ColorRef & focused):
      focusProxy_(focusProxy),
      noFocus_(noFocus),
      focused_(focused)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool hasFocus = ItemFocus::singleton().hasFocus(focusProxy_.id_);
      result = hasFocus ? focused_.get() : noFocus_.get();
    }

    Item & focusProxy_;
    ColorRef noFocus_;
    ColorRef focused_;
  };

}


#endif // YAE_ITEM_FOCUS_H_
