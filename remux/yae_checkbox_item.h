// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 21:32:22 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CHECKBOX_ITEM_H_
#define YAE_CHECKBOX_ITEM_H_

// local:
#include "yaeInputArea.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // CheckboxItem
  //
  class CheckboxItem : public ClickableItem
  {
    CheckboxItem(const CheckboxItem &);
    CheckboxItem & operator = (const CheckboxItem &);

  public:
    CheckboxItem(const char * id, ItemView & view);

    // virtual:
    void uncache();

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    bool onMouseOver(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint);

    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    void onFocus();
    void onFocusOut();

    bool processEvent(Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * event);

    ItemView & view_;
    BoolRef enabled_;
    BoolRef checked_;
    ColorRef color_;

    ContextCallback toggle_checked_;

    ItemView::TAnimatorPtr spotlight_animator_;
    Item::TObserverPtr animate_spotlight_;
  };

}


#endif // YAE_CHECKBOX_ITEM_H_
