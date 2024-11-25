// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Feb 20 22:25:28 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ARROW_ITEM_H_
#define YAE_ARROW_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // ArrowItem
  //
  class YAEUI_API ArrowItem : public Item
  {
    ArrowItem(const ArrowItem &);
    ArrowItem & operator = (const ArrowItem &);

  public:

    //----------------------------------------------------------------
    // Direction
    //
    enum Direction
    {
      kLeft,
      kRight,
      kUp,
      kDown
    };

    ArrowItem(const char * id, Direction direction);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    bool visible() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // the thickness of the arrow shape
    ItemRef weight_;
    ItemRef opacity_;
    ColorRef color_;

  protected:
    typedef void(*TPaintFunc)(const BBox & bbox,
                              double weight,
                              double opacity,
                              const Color & color);
    TPaintFunc paint_arrow_;
  };

}


#endif // YAE_ARROW_ITEM_H_
