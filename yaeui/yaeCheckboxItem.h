// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 21:32:22 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CHECKBOX_ITEM_H_
#define YAE_CHECKBOX_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeInputArea.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // CheckboxItem
  //
  class YAEUI_API CheckboxItem : public ClickableItem
  {
    CheckboxItem(const CheckboxItem &);
    CheckboxItem & operator = (const CheckboxItem &);

  public:
    CheckboxItem(const char * id, ItemView & view);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    bool onMouseOver(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint);

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    // virtual:
    void onFocus();

    // virtual:
    bool processEvent(Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * event);

    // helpers:
    void set_checked(bool checked);
    void toggle();

    void animate_hover(bool force = false) const;
    void animate_click() const;

    //----------------------------------------------------------------
    // Action
    //
    struct YAEUI_API Action
    {
      virtual ~Action() {}
      virtual void operator()(const CheckboxItem &) const = 0;
    };

    ItemView & view_;
    BoolRef enabled_;
    BoolRef checked_;
    BoolRef animate_;
    ColorRef color_;

    yae::shared_ptr<Action> on_toggle_;

    ItemView::TAnimatorPtr hover_;
    ItemView::TAnimatorPtr click_;
    Item::TObserverPtr animate_spotlight_;
  };

}


#endif // YAE_CHECKBOX_ITEM_H_
