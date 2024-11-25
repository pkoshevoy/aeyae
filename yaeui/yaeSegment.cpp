// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeSegment.h"


namespace yae
{

  //----------------------------------------------------------------
  // Segment::clear
  //
  void
  Segment::clear()
  {
    origin_ = 0.0;
    length_ = 0.0;
  }

  //----------------------------------------------------------------
  // Segment::isEmpty
  //
  bool
  Segment::isEmpty() const
  {
    return (length_ == 0.0);
  }

  //----------------------------------------------------------------
  // Segment::expand
  //
  void
  Segment::expand(const Segment & segment)
  {
    if (!segment.isEmpty())
    {
      if (isEmpty())
      {
        *this = segment;
      }
      else
      {
        double e = std::max<double>(end(), segment.end());
        origin_ = std::min<double>(origin_, segment.origin_);
        length_ = e - origin_;
      }
    }
  }

  //----------------------------------------------------------------
  // Segment::pixelOverlap
  //
  double
  Segment::pixelOverlap(double p) const
  {
    int64 s0 = int64(0.5 + 1000.0 * origin_);
    int64 s1 = int64(0.5 + 1000.0 * (origin_ + length_));
    int64 p0 = int64(0.5 + 1000.0 * p);
    int64 p1 = p0 + 1000;

    int64 a = std::max<int64>(p0, s0);
    int64 b = std::min<int64>(p1, s1);
    int64 o = std::max<int64>(0, b - a);
    YAE_ASSERT(o <= 1000);
    return double(o) * 1e-3;
  }
}
