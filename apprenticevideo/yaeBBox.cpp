// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeBBox.h"


namespace yae
{

  //----------------------------------------------------------------
  // BBox::clear
  //
  void
  BBox::clear()
  {
    x_ = 0.0;
    y_ = 0.0;
    w_ = 0.0;
    h_ = 0.0;
  }

  //----------------------------------------------------------------
  // BBox::isEmpty
  //
  bool
  BBox::isEmpty() const
  {
    return (w_ == 0.0) && (h_ == 0.0);
  }

  //----------------------------------------------------------------
  // BBox::expand
  //
  void
  BBox::expand(const BBox & bbox)
  {
    if (!bbox.isEmpty())
    {
      if (isEmpty())
      {
        *this = bbox;
      }
      else
      {
        double r = std::max<double>(right(), bbox.right());
        double b = std::max<double>(bottom(), bbox.bottom());
        x_ = std::min<double>(x_, bbox.x_);
        y_ = std::min<double>(y_, bbox.y_);
        w_ = r - x_;
        h_ = b - y_;
      }
    }
  }

  //----------------------------------------------------------------
  // BBox::intersect
  //
  BBox
  BBox::intersect(const BBox & b) const
  {
    const BBox & a = *this;

    double x0 = (a.x0() < b.x0()) ? b.x0() : a.x0();
    double x1 = (a.x1() < b.x1()) ? a.x1() : b.x1();

    double y0 = (a.y0() < b.y0()) ? b.y0() : a.y0();
    double y1 = (a.y1() < b.y1()) ? a.y1() : b.y1();

    if (x0 < x1 && y0 < y1)
    {
      return BBox(x0, y0, x1 - x0, y1 - y0);
    }

    return BBox();
  }

}
