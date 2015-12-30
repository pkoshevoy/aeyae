// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_BBOX_H_
#define YAE_BBOX_H_

// local interfaces:
#include "yaeSegment.h"
#include "yaeVec.h"


namespace yae
{


  //----------------------------------------------------------------
  // BBox
  //
  // 2D bounding box
  //
  struct BBox
  {
    BBox():
      x_(0.0),
      y_(0.0),
      w_(0.0),
      h_(0.0)
    {}

    void clear();
    bool isEmpty() const;
    void expand(const BBox & bbox);

    inline bool disjoint(const BBox & b) const
    {
      return
        (this->left() > b.right() || b.left() > this->right()) ||
        (this->top() > b.bottom() || b.top() > this->bottom());
    }

    inline bool overlap(const BBox & b) const
    { return !this->disjoint(b); }

    inline double left() const
    { return x_; }

    inline double right() const
    { return x_ + w_; }

    inline double top() const
    { return y_; }

    inline double bottom() const
    { return y_ + h_; }

    inline Segment x() const
    { return Segment(x_, w_); }

    inline Segment y() const
    { return Segment(y_, h_); }

    inline BBox & operator *= (double scale)
    {
      w_ *= scale;
      h_ *= scale;
      return *this;
    }

    inline BBox & operator += (double translate)
    {
      x_ += translate;
      y_ += translate;
      return *this;
    }

    inline TVec2D center() const
    {
      TVec2D pt;
      pt.coord_[0] = x_ + 0.5 * w_;
      pt.coord_[1] = y_ + 0.5 * h_;
      return pt;
    }

    inline double radius() const
    { return 0.5 * (h_ < w_ ? h_ : w_); }

    inline double aspectRatio() const
    { return h_ == 0.0 ? 0.0 : (w_ / h_); }

    double x_;
    double y_;
    double w_;
    double h_;
  };
}


#endif // YAE_BBOX_H_
