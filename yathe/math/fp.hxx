// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : fp.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 27 01:00:09 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : fixed point number template wrapper class parameterized by
//                the number of bits allocated to the fraction

#ifndef FP_HXX_
#define FP_HXX_

// system includes:
#include <iostream>
#include <math.h>


//----------------------------------------------------------------
// fp_t
//
// A template class for fixed point number arithmetic.
//
template <unsigned char fraction_bits>
class fp_t
{
public:
  // typedefs:
  typedef fp_t<fraction_bits> self_t;
  typedef int fixed_t;

  // constants:
  static const fixed_t fixed_one = 1 << fraction_bits;
  static const fixed_t fixed_almost_one = fixed_one - 1;
  static const fixed_t fixed_one_half = fixed_one >> 1;

  // constructors/mutators:
  fp_t():
    fixed_(0)
  {}

  template <typename int_t>
  fp_t(const int_t & i):
    fixed_(fixed_t(i) << fraction_bits)
  {}

  fp_t(const float & f):
    fixed_(fixed_t(f * fixed_one))
  {}

  fp_t(const double & d):
    fixed_(fixed_t(float(d) * fixed_one))
  {}

  // conversion operators:
  inline operator int() const
  { return fixed_to_int<int>(fixed_); }

  inline operator float() const
  { return fixed_to_float(fixed_); }

  inline operator double() const
  { return fixed_to_double(fixed_); }

  // convenience functions:
  static inline fixed_t float_to_fixed(const float & f)
  { return fixed_t(f * fixed_one); }

  static inline float fixed_to_float(const fixed_t & x)
  { return float(x) / fixed_one; }

  static inline fixed_t double_to_fixed(const double & d)
  { return fixed_t(d * fixed_one); }

  static inline double fixed_to_double(const fixed_t & x)
  { return double(x) / fixed_one; }

  template <typename int_t>
  static inline fixed_t int_to_fixed(const int_t & i)
  { return fixed_t(i) << fraction_bits; }

  template <typename int_t>
  static inline int_t fixed_to_int(const fixed_t & x)
  { return  int_t(x >> fraction_bits); }

  template <typename int_t>
  static inline int_t fixed_round_int(const fixed_t & x)
  { return int_t((x + fixed_one_half) >> fraction_bits); }

  template <typename int_t>
  static inline int_t fixed_floor_int(const fixed_t & x)
  { return int_t(x >> fraction_bits); }

  template <typename int_t>
  static inline int_t fixed_ceil_int(const fixed_t & x)
  { return int_t((x + fixed_almost_one) >> fraction_bits); }

  // comparison operators:
  inline bool operator == (const self_t & b) const
  { return fixed_ == b.fixed_; }

  inline bool operator != (const self_t & b) const
  { return fixed_ != b.fixed_; }

  inline bool operator <= (const self_t & b) const
  { return fixed_ <= b.fixed_; }

  inline bool operator >= (const self_t & b) const
  { return fixed_ >= b.fixed_; }

  inline bool operator < (const self_t & b) const
  { return fixed_ < b.fixed_; }

  inline bool operator > (const self_t & b) const
  { return fixed_ > b.fixed_; }

  // bit shift operators:
  inline self_t operator << (int i) const
  {
    self_t s;
    s.fixed_ = fixed_ << i;
    return s;
  }

  inline self_t operator >> (int i) const
  {
    self_t s;
    s.fixed_ = fixed_ >> i;
    return s;
  }

  // modulo operator:
  inline self_t operator % (const self_t & modulo) const
  { return mod(modulo); }

  // in place arithmetic:
  inline self_t & operator += (const self_t & b)
  {
    fixed_ += b.fixed_;
    return *this;
  }

  inline self_t & operator -= (const self_t & b)
  {
    fixed_ -= b.fixed_;
    return *this;
  }

  inline self_t & operator *= (const self_t & b)
  {
    fixed_ = mul(fixed_, b.fixed_);
    return *this;
  }

  inline self_t & operator /= (const self_t & b)
  {
    fixed_ = div(fixed_, b.fixed_);
    return *this;
  }

  // out of place arithmetic:
  inline self_t operator + (const self_t & b) const
  {
    self_t c;
    c.fixed_ = fixed_ + b.fixed_;
    return c;
  }

  inline self_t operator - (const self_t & b) const
  {
    self_t c;
    c.fixed_ = fixed_ - b.fixed_;
    return c;
  }

  inline self_t operator * (const self_t & b) const
  {
    self_t c;
    c.fixed_ = mul(fixed_, b.fixed_);
    return c;
  }

  inline self_t operator / (const self_t & b) const
  {
    self_t c;
    c.fixed_ = div(fixed_, b.fixed_);
    return c;
  }

  // rounding functions:
  template <typename int_t>
  inline int_t floor() const
  { return fixed_floor_int<int_t>(fixed_); }

  template <typename int_t>
  inline int_t round() const
  { return fixed_round_int<int_t>(fixed_); }

  template <typename int_t>
  inline int_t ceil() const
  { return fixed_ceil_int<int_t>(fixed_); }

  // multiplication/division helpers:
  static inline fixed_t
  mul(const fixed_t & a, const fixed_t & b)
  {
    int64_t c = int64_t(a) * int64_t(b);
    return c >> fraction_bits;
  }

  static inline fixed_t
  div(const fixed_t & a, const fixed_t & b)
  {
    return (int64_t(a) << fraction_bits) / b;
  }

  // absolute value:
  inline self_t abs() const
  {
    self_t s;
    s.fixed_ = ::abs(fixed_);
    return s;
  }

  // return the remainder of the division by the modulo number:
  inline self_t mod(const self_t & modulo) const
  {
    self_t s;
    s.fixed_ = fixed_ % modulo.fixed_;
    return s;
  }

  // integer representation of the fixed point real number:
  fixed_t fixed_;
};

//----------------------------------------------------------------
// operator <<
//
template <unsigned char fraction_bits>
std::ostream &
operator << (std::ostream & so, const fp_t<fraction_bits> & fp)
{
  return so << float(fp);
}


#endif // FP_HXX_
