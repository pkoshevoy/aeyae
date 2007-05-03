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
  
  return the_color_t((1.0 - color_param) * color_a +
		     color_param * color_b);
}
