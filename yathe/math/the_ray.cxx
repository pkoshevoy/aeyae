// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_ray.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 3D ray class.

// local includes:
#include "math/the_ray.hxx"


//----------------------------------------------------------------
// the_ray_t::dist
//
float
the_ray_t::dist(const p3x1_t & pt) const
{
  v3x1_t w = (pt - p_);
  float t = param(pt);
  float d = ~(w - t * v_);
  return d;
}

//----------------------------------------------------------------
// the_ray_t::param
//
float
the_ray_t::param(const p3x1_t & pt) const
{
  //        /
  //     w /
  //      /
  //     /
  //    /
  //   /
  //  *======----> v_

  v3x1_t w = (pt - p_);
  float wv = w * v_;
  float vv = v_ * v_;

  // FIXME: is the divide-by-zero test really necessary?
  // return (vv == 0) ? 0 : wv / vv;
  return wv / vv;
}

//----------------------------------------------------------------
// the_ray_t::intersect
//
bool
the_ray_t::intersect(const the_ray_t & rb,
		     float & ta,       // parameter along this ray
		     float & tb,       // parameter along given ray
		     float & ab) const // distance between closest points
{
  // NOTE: see http://astronomy.swin.edu.au/pbourke/geometry/lineline3d/
  // for details.

  const p3x1_t & p1 = p();
  p3x1_t p2 = p1 + v();

  const p3x1_t & p3 = rb.p();
  p3x1_t p4 = p3 + rb.v();

  v3x1_t v13 = p1 - p3;
  v3x1_t v43 = p4 - p3;
  if (~v43 < THE_EPSILON) return false;

  v3x1_t v21 = p2 - p1;
  if (~v21 < THE_EPSILON) return false;

  double d1343 = v13 * v43;
  double d4321 = v43 * v21;
  double d1321 = v13 * v21;
  double d4343 = v43 * v43;
  double d2121 = v21 * v21;

  double denom = d2121 * d4343 - d4321 * d4321;
  if (fabs(denom) < THE_EPSILON) return false;

  double numer = d1343 * d4321 - d1321 * d4343;

  ta = float(numer / denom);
  tb = float((d1343 + (d4321 * ta)) / d4343);

  p3x1_t pa = p1 + ta * v21;
  p3x1_t pb = p3 + tb * v43;
  ab = ~(pb - pa);
  return true;
}

//----------------------------------------------------------------
// the_ray_t::dump
//
void
the_ray_t::dump(ostream & strm) const
{
  strm << "the_ray_t(" << (void *)this << ")" << endl
       << "{" << endl
       << "  p_ = " << p_ << endl
       << "  v_ = " << v_ << endl
       << "}" << endl << endl;
}
