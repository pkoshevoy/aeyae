// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SEGMENT_H_
#define YAE_SEGMENT_H_

// standard libraries:
#include <cmath>

// local interfaces:
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // Segment
  //
  // 1D bounding box
  //
  struct Segment
  {
    Segment(double origin = 0.0, double length = 0.0):
      origin_(origin),
      length_(length)
    {}

    inline bool operator == (const Segment & s) const
    { return origin_ == s.origin_ && length_ == s.length_; }

    void clear();
    bool isEmpty() const;
    void expand(const Segment & seg);

    // compute fractional pixel overlap over a segment
    // where 1 == full overlap, 0 == no overlap:
    double pixelOverlap(double p) const;

    inline bool disjoint(const Segment & b) const
    { return this->start() > b.end() || b.start() > this->end(); }

    inline bool overlap(const Segment & b) const
    { return !this->disjoint(b); }

    inline bool disjoint(double pt) const
    { return this->start() > pt || pt > this->end(); }

    inline bool overlap(double pt) const
    { return !this->disjoint(pt); }

    inline double start() const
    { return origin_; }

    inline double end() const
    { return origin_ + length_; }

    inline Segment & operator *= (double scale)
    {
      length_ *= scale;
      return *this;
    }

    inline Segment & operator += (double translate)
    {
      origin_ += translate;
      return *this;
    }

    inline double center() const
    { return origin_ + 0.5 * length_; }

    inline double radius() const
    { return 0.5 * length_; }

    // convert point from world coordinate system to local coordinate system:
    inline double to_lcs(double wcs_pt) const
    {
      YAE_ASSERT(length_);
      return length_ ? (wcs_pt - origin_) / length_ : 0.0;
    }

    // convert point from local coordinate system to world coordinate system:
    inline double to_wcs(double lcs_pt) const
    { return origin_ + length_ * lcs_ptr; }

    inline Segment rounded(double margin = 2e-1) const
    {
      double x = floor(log10(fabs(length_)));
      double granularity = pow(10, x - 1);
      double t0 = floor(origin_ / granularity - margin) * granularity;
      double t1 = ceil(end() / granularity + margin) * granularity;
      return Segment(t0, t1);
    }

    double origin_;
    double length_;
  };
}


#endif // YAE_SEGMENT_H_
