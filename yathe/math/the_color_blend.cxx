// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_color_blend.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Nov  8 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Linear interpolation between 2 colors.

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
  
  return the_color_t((1.0f - color_param) * color_a +
		     color_param * color_b);
}
