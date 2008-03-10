/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : the_quadratic_polynomial.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Quadratic polynomial.

#ifndef THE_QUADRATIC_POLYNOMIAL_HXX_
#define THE_QUADRATIC_POLYNOMIAL_HXX_

// system includes:
#include <iostream>

// local includes:
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// the_quadratic_polynomial_t
//
// f(x) = axx + bx + c
// 
class the_quadratic_polynomial_t
{
public:
  the_quadratic_polynomial_t():
    a_(0.0),
    b_(0.0),
    c_(0.0)
  {}
  
  the_quadratic_polynomial_t(const float & a,
			     const float & b,
			     const float & c):
    a_(a),
    b_(b),
    c_(c)
  {}
  
  bool
  setup_two_values_and_slope(const float & i, const float & r, // r = f(i)
			     const float & j, const float & s, // s = f(j)
			     const float & k, const float & t) // t = f'(k)
  {
    float denom = j * j + 2 * k * (i - j) - i * i;
    if (denom == 0.0) return false;
    
    a_ = (s - r + t * (i - j)) / denom;
    b_ = t - 2 * a_ * k;
    c_ = r - i * (a_ * (i + 2 * k) - t);
    
    return true;
  }
  
  bool
  setup_three_values(const float & i, const float & r, // r = f(i)
		     const float & j, const float & s, // s = f(j)
		     const float & k, const float & t) // t = f(k)
  {
    // solve via gaussian elimination:
    the_quadruplet_t<float> m[3];
    m[0].assign(i * i, i, 1, r);
    m[1].assign(j * j, j, 1, s);
    m[2].assign(k * k, k, 1, t);
    
    gauss_sort(m);
    if (m[0][0] == 0) return false;
    
    // eliminate first column of the second row:
    if (m[1][0] != 0)
    {
      the_quadruplet_t<float> d(m[0]);
      d.scale(m[1][0] / m[0][0]);
      m[1].decrement(d);
    }
    
    // eliminate first column of the third row:
    if (m[2][0] != 0)
    {
      the_quadruplet_t<float> d(m[0]);
      d.scale(m[2][0] / m[0][0]);
      m[2].decrement(d);
    }
    
    gauss_sort(m);
    if (m[1][1] == 0) return false;
    
    // eliminate second column of the third row:
    if (m[2][1] != 0)
    {
      the_quadruplet_t<float> d(m[1]);
      d.scale(m[2][1] / m[1][1]);
      m[2].decrement(d);
    }
    
    if (m[2][2] == 0) return false;
    
    // eliminate third column of the second row:
    if (m[1][2] != 0)
    {
      the_quadruplet_t<float> d(m[2]);
      d.scale(m[1][2] / m[2][2]);
      m[1].decrement(d);
    }
    
    // eliminate third column of the first row:
    if (m[0][2] != 0)
    {
      the_quadruplet_t<float> d(m[2]);
      d.scale(m[0][2] / m[2][2]);
      m[0].decrement(d);
    }
    
    // eliminate second column of the first row:
    if (m[0][1] != 0)
    {
      the_quadruplet_t<float> d(m[1]);
      d.scale(m[0][1] / m[1][1]);
      m[0].decrement(d);
    }
    
    a_ = m[0][3] / m[0][0];
    b_ = m[1][3] / m[1][1];
    c_ = m[2][3] / m[2][2];
    
    return true;
  }
  
  // this is the parameter at which the parabola is either minimum or maximum:
  inline float stationary_point() const
  { return (-b_) / (2 * a_); }
  
  // avaluate the parabola:
  inline float eval(const float & x) const
  { return c_ + x * (b_ + x * a_); }
  
  inline float operator() (const float & x) const
  { return eval(x); }
  
  // accessors:
  inline const float & a() const { return a_; }
  inline const float & b() const { return b_; }
  inline const float & c() const { return c_; }
  
  // helper:
  inline bool concave_up() const
  { return a_ > 0.0; }
  
  inline bool concave_down() const
  { return a_ < 0.0; }
  
private:
  static bool gauss_a_less_than_b(const the_quadruplet_t<float> & a,
				  const the_quadruplet_t<float> & b)
  {
    if (fabs(a[0]) > fabs(b[0])) return false;
    if (fabs(a[1]) > fabs(b[1])) return false;
    if (fabs(a[2]) > fabs(b[2])) return false;
    return true;
  }
  
  static void gauss_sort(the_quadruplet_t<float> * m)
  {
    if (gauss_a_less_than_b(m[0], m[1])) std::swap(m[0], m[1]);
    if (gauss_a_less_than_b(m[0], m[2])) std::swap(m[0], m[2]);
    if (gauss_a_less_than_b(m[1], m[2])) std::swap(m[1], m[2]);
  }
  
  float a_;
  float b_;
  float c_;
};

//----------------------------------------------------------------
// operator <<
// 
inline std::ostream &
operator << (std::ostream & s, const the_quadratic_polynomial_t & parabola)
{
  s << "Y(X) = " << parabola.a() << " * X^2";
  
  if (parabola.b() < 0.0) s << " - ";
  else s << " + ";
  s << fabs(parabola.b()) << " * X";
  
  if (parabola.c() < 0.0) s << " - ";
  else s << " + ";
  s << fabs(parabola.c());
  
  return s;
}


#endif // THE_QUADRATIC_POLYNOMIAL_HXX_
