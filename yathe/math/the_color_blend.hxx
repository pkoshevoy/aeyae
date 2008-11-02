// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : the_color_blend.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Nov  8 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Linear interpolation between 2 colors.

#ifndef THE_COLOR_BLEND_HXX_
#define THE_COLOR_BLEND_HXX_

// system includes:
#include <vector>

// local includes:
#include "math/the_color.hxx"


//----------------------------------------------------------------
// the_color_blend_t
// 
// uniformly spaced color blend:
// 
class the_color_blend_t
{
public:
  the_color_blend_t() {}
  the_color_blend_t(unsigned int num_colors): color_(num_colors) {}
  ~the_color_blend_t() {}
  
  // find between which two colors the parameter "t" falls,
  // blend between them and return the result:
  const the_color_t eval(float t) const;
  
  // short-hand for "eval(..)":
  inline const the_color_t operator() (float t) const
  { return eval(t); }
  
  // low level accessors:
  inline const std::vector<the_color_t> & color() const
  { return color_; }
  
  inline std::vector<the_color_t> & color()
  { return color_; }
  
private:
  // an array of colors:
  std::vector<the_color_t> color_;
};


#endif // THE_COLOR_BLEND_HXX_
