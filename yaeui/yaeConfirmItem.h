// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 13 13:59:52 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CONFIRM_ITEM_H_
#define YAE_CONFIRM_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeItem.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // ConfirmItem
  //
  class YAEUI_API ConfirmItem : public Item
  {
  public:
    ConfirmItem(const char * id, ItemView & view);
    ~ConfirmItem();

    // call this after initializing the data references:
    void layout();

    // virtual:
    void uncache();

    //----------------------------------------------------------------
    // Action
    //
    struct YAEUI_API Action
    {
      virtual ~Action() {}
      virtual void execute() const {}

      inline void uncache()
      {
        bg_.uncache();
        fg_.uncache();
        message_.uncache();
      }

      ColorRef bg_;
      ColorRef fg_;
      TVarRef message_;
    };

    ItemView & view_;
    ColorRef bg_;
    ColorRef fg_;
    TVarRef message_;
    ItemRef font_size_;

    yae::shared_ptr<Action> affirmative_;
    yae::shared_ptr<Action> negative_;
  };

  //----------------------------------------------------------------
  // ConfirmItemPtr
  //
  typedef yae::shared_ptr<ConfirmItem, Item> ConfirmItemPtr;

}


#endif // YAE_CONFIRM_ITEM_H_
