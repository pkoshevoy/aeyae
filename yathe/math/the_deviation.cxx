// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_deviation.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jun 14 11:54:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Helper classes used to find closest points between objects.

// system includes:
#include <algorithm>

// local includes:
#include "math/the_deviation.hxx"
#include "math/the_quadratic_polynomial.hxx"
#include "utils/the_dynamic_array.hxx"
#include "utils/the_utils.hxx"


//----------------------------------------------------------------
// the_deviation_t::find_local_minima
// 
bool
the_deviation_t::
find_local_minima(std::list<the_deviation_min_t> & solution,
		  const size_t & steps_per_segment) const
{
  std::list<the_slope_sign_t> slope_signs;
  float s0;
  float s1;
  size_t segments = init_slope_signs(steps_per_segment,
					   slope_signs,
					   s0,
					   s1);
  if (segments == 0) return false;
  
  // bracket the local minima:
  the_dynamic_array_t<the_slope_sign_t> brackets;
  
  for (std::list<the_slope_sign_t>::iterator i = slope_signs.begin();
       i != slope_signs.end() && the::next(i) != slope_signs.end(); ++i)
  {
    const the_slope_sign_t & a = *i;
    const the_slope_sign_t & b = *the::next(i);
    
    // if this interval starts with increase in function, then the solution
    // can only be a maxima, not minima, so skip it:
    if (a[1] > 0.0) continue;
    
    const float & ax = a[0];
    const float & cx = b[0];
    const float   bx = ax + 0.5f * (cx - ax);
    
    size_t pos = brackets.size();
    brackets[pos] = the_slope_sign_t(ax, R(ax));
    
    pos++;
    brackets[pos] = the_slope_sign_t(bx, R(bx));
    
    pos++;
    brackets[pos] = the_slope_sign_t(cx, R(cx));
  }
  
  // now that we have all the local minima bracketed, isolate them:
  const float tolerance = 1E-3f * ((s1 - s0) / float(segments));
  float minR = FLT_MAX;
  for (size_t i = 0; i < brackets.size(); i += 3)
  {
    the_slope_sign_t & a = brackets[i];
    the_slope_sign_t & b = brackets[i + 1];
    the_slope_sign_t & c = brackets[i + 2];
    
    if (isolate_minima(a, b, c, tolerance))
    {
      solution.push_back(the_deviation_min_t(b[0], b[1]));
      minR = std::min(minR, b[0]);
    }
  }
  
  // add boundary values if applicable:
  float R;
  float dR;
  
  eval_R(s0, R, dR);
  if (solution.empty() || R <= minR || dR >= 0)
  {
    // function is increasing, must be a boundary minima:
    solution.push_back(the_deviation_min_t(s0, R));
    minR = std::min(minR, R);
  }
  
  eval_R(s1, R, dR);
  if (solution.empty() || R <= minR || dR <= 0)
  {
    // function is decreasing, must be a boundary minima:
    solution.push_back(the_deviation_min_t(s1, R));
    minR = std::min(minR, R);
  }
  
  return (solution.empty() == false);
}

//----------------------------------------------------------------
// the_deviation_t::store_slope_sign
// 
void
the_deviation_t::
store_slope_sign(std::list<the_slope_sign_t> & slope_signs,
		 const float & s) const
{
  float Z;
  float dZ;
  eval_Z(s, Z, dZ);
  const float slope_sign = the_sign(dZ);
  
  // ignore stationary points:
  if (slope_sign == 0.0) return;
  
  // check for slope sign change:
  if (slope_signs.empty() == false)
  {
    the_slope_sign_t & prev = slope_signs.back();
    if (prev[1] == slope_sign)
    {
      if (slope_sign < 1.0) prev[0] = s;
      return;
    }
  }
  
  slope_signs.push_back(the_slope_sign_t(s, slope_sign));
}

//----------------------------------------------------------------
// the_deviation_t::isolate_minima
// 
bool
the_deviation_t::isolate_minima(the_slope_sign_t & a,
				the_slope_sign_t & b,
				the_slope_sign_t & c,
				const float & tolerance) const
{
  const float min_x = a[0];
  const float max_x = c[0];
  
  for (size_t i = 0; i < 20; i++)
  {
    float & ax = a[0];
    float & bx = b[0];
    float & cx = c[0];
    
    if (fabs(cx - ax) <= tolerance)
    {
      break;
    }
    
    float & fa = a[1];
    float & fb = b[1];
    float & fc = c[1];
    
    the_quadratic_polynomial_t parabola;
    if (parabola.setup_three_values(ax, fa, bx, fb, cx, fc) == false)
    {
      // could not fit a parabola to these values, most likely
      // the minimum is located at the min/max boundary:
      float R;
      float dR;
      eval_R(bx, R, dR);
      if (parabola.setup_two_values_and_slope(ax, fa,
					      cx, fc,
					      bx, dR) == false)
      {
#ifndef NDEBUG
	cout << "WARNING: isolate_minima: could not fit a parabola" << endl;
#endif
	return true;
      }
    }
    
    if (parabola.concave_up())
    {
      float x = std::min(max_x, std::max(min_x, parabola.stationary_point()));
      float f = R(x);
      
      if ((f <= fb) && (x > bx))
      {
	ax = bx;
	fa = fb;
	
	bx = x;
	fb = f;
      }
      else if ((f <= fb) && (x < bx))
      {
	cx = bx;
	fc = fb;
	
	bx = x;
	fb = f;
      }
      else if (x > bx)
      {
	cx = x;
	fc = f;
      }
      else if (x < bx)
      {
	ax = x;
	fa = f;
      }
      else
      {
#ifndef NDEBUG
	cout << "WARNING: isolate_minima: either stuck or done" << endl;
#endif
	break;
      }
    }
    else
    {
      // concave down parabola - pick a new midpoint:
      float ab = fb - fa;
      float bc = fb - fc;
      
      if (bc > ab)
      {
	ax = bx;
	fa = fb;
	
	bx = bx + 0.5f * (cx - bx);
	fb = R(bx);
      }
      else
      {
	cx = bx;
	fc = fb;
	
	bx = ax + 0.5f * (bx - ax);
	fb = R(bx);
      }
    }
  }
  
  return true;
}


//----------------------------------------------------------------
// the_volume_ray_deviation_t::init_slope_signs
// 
size_t
the_volume_ray_deviation_t::
init_slope_signs(const size_t & /* steps_per_segment */,
		 std::list<the_slope_sign_t> & slope_signs,
		 float & s0,
		 float & s1) const
{
  s0 = P_ * Q_.to_wcs(p3x1_t(0.0, 0.0, 0.0));
  s1 = P_ * Q_.to_wcs(p3x1_t(0.0, 0.0, 1.0));
  if (s0 > s1) std::swap(s0, s1);
  
  store_slope_sign(slope_signs, s0);
  store_slope_sign(slope_signs, s1);
  
  return 1;
}
