// File         : the_color_blend.cxx
// Author       : Paul A. Koshevoy
// Created      : Fri Nov  8 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

// local includes:
#include "math/the_color_blend.hxx"


//----------------------------------------------------------------
// the_color_blend_t::eval
// 
const the_color_t
the_color_blend_t::eval(float t) const
{
  unsigned int color_a_index = (unsigned int)((float)(color_.size() - 1) * t);
  unsigned int color_b_index = color_a_index + 1;
  
  float color_param = t;
  if (color_.size() > 1)
  {
    color_param = (((float)(color_.size() - 1) * t -
		    (float)(color_a_index)) /
		   ((float)(color_b_index) -
		    (float)(color_a_index)));
  }
  
  if (color_b_index == color_.size()) color_b_index--;
  
  const the_color_t & color_a = color_[color_a_index];
  const the_color_t & color_b = color_[color_b_index];
  
  return the_color_t((1.0 - color_param) * color_a +
		     color_param * color_b);
}
