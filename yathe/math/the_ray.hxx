// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_ray.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 3D ray class.

#ifndef THE_RAY_HXX_
#define THE_RAY_HXX_

// system includes:
#include <iostream>

// local includes:
#include "math/v3x1p3x1.hxx"

// namespace access:
using std::ostream;


//----------------------------------------------------------------
// the_ray_t
//
class the_ray_t
{
public:
  the_ray_t(): p_(0.0, 0.0, 0.0), v_(0.0, 0.0, 1.0) {}
  the_ray_t(const p3x1_t & P, const v3x1_t & V): p_(P), v_(V) {}

  // equality test operator:
  inline bool operator == (const the_ray_t & r) const
  { return p_ == r.p_ && v_ == r.v_; }

  // const accessors:
  inline const p3x1_t & p() const { return p_; }
  inline const v3x1_t & v() const { return v_; }

  // non-const accessors:
  inline p3x1_t & p() { return p_; }
  inline v3x1_t & v() { return v_; }

  // distance from a point to a ray along a normal to the ray:
  float dist(const p3x1_t & pt) const;

  // calculate parameter value of a point along the ray
  float param(const p3x1_t & pt) const;

  // find the closest points on two rays (when distance between closest
  // points is zero the rays intersect):
  bool intersect(const the_ray_t & rb,
		 float & ta,        // parameter along this ray
		 float & tb,        // parameter along given ray
		 float & ab) const; // distance between closest points

  // shortest distance between a point and this ray:
  inline float operator - (const p3x1_t & pt) const
  { return dist(pt); }

  // project a given point onto the ray:
  inline float operator * (const p3x1_t & pt) const
  { return param(pt); }

  // point at a given parameter along the ray:
  inline const p3x1_t operator * (const float & param) const
  { return eval(param); }

  // analogous to a curve:
  inline const p3x1_t eval(const float & param) const
  { return p_ + param * v_; }

  inline const v3x1_t & derivative(const float & /* param */) const
  { return v_; }

  inline void position_and_derivative(const float & s,
				      p3x1_t & P,
				      v3x1_t & dP_ds) const
  {
    P = eval(s);
    dP_ds = derivative(s);
  }

  // minor optimization:
  inline void eval(const float & param, p3x1_t & pt) const
  { pt = p_ + param * v_; }

  // For debugging:
  void dump(ostream & strm) const;

protected:
  p3x1_t p_;
  v3x1_t v_;
};

// For debugging purposes:
inline ostream &
operator << (ostream & stream, const the_ray_t & ray)
{
  ray.dump(stream);
  return stream;
}

// point at a given parameter along the ray:
inline const p3x1_t
operator * (const float & param, const the_ray_t & ray)
{
  return ray * param;
}


#endif // THE_RAY_HXX_
