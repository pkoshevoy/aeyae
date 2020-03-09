// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jul  2 11:31:34 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DASHED_RECT_H_
#define YAE_DASHED_RECT_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // DashedRect
  //
  class DashedRect : public Item
  {
    DashedRect(const DashedRect &);
    DashedRect & operator = (const DashedRect &);

  public:
    DashedRect(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // line width:
    ItemRef border_;

    ItemRef opacity_;
    ColorRef fg_;
    ColorRef bg_;
  };

}


#endif // YAE_DASHED_RECT_H_
