// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jun 29 21:16:03 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DONUT_RECT_H_
#define YAE_DONUT_RECT_H_

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // DonutRect
  //
  class DonutRect : public Item
  {
    DonutRect(const DonutRect &);
    DonutRect & operator = (const DonutRect &);

  public:
    DonutRect(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // 1D bounding segments of the donut hole:
    Segment xHole_;
    Segment yHole_;

    ItemRef opacity_;
    ColorRef color_;
  };

}


#endif // YAE_DONUT_RECT_H_
