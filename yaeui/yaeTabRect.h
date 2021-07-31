// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Sep  8 16:44:02 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TAB_RECT_H_
#define YAE_TAB_RECT_H_

// local interfaces:
#include "yaeInputArea.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // TabPosition
  //
  enum TabPosition
  {
    // vertical:
    kTabTop = 0,
    kTabBottom = 1,
    // horizontal:
    kTabLeft = 2,
    kTabRight = 3
  };

  inline static bool is_vertical(TabPosition tab)
  { return (int(tab) & 2) != 2; }

  inline static bool is_mirrored(TabPosition tab)
  { return (int(tab) & 1) != 1; }

  //----------------------------------------------------------------
  // TabRect
  //
  class YAEUI_API TabRect : public Item
  {
    TabRect(const TabRect &);
    TabRect & operator = (const TabRect &);

  public:
    TabRect(const char * id,
            ItemView & view,
            TabPosition orientation = kTabBottom);

    ~TabRect();

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // helpers:
    void animate_hover(bool force = false) const;

    // need view access for animation:
    ItemView & view_;

    // keep implementation details private:
    struct TPrivate;
    TPrivate * p_;

    // corner radius:
    ItemRef r1_; // exterior
    ItemRef r2_; // interior

    ItemRef opacity_;
    ColorRef color_;
    ColorRef background_;

    ItemView::TAnimatorPtr hover_;
    Item::TObserverPtr animate_;
  };

}


#endif // YAE_TAB_RECT_H_
