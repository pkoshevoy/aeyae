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


// File         : the_view_mgr_orientation.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 30 19:55:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Predefined view orientations.

#ifndef THE_VIEW_MGR_ORIENTATION_HXX_
#define THE_VIEW_MGR_ORIENTATION_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"

// system includes:
#include <iostream>

// namespace access:
using std::ostream;


//----------------------------------------------------------------
// the_view_mgr_orientation_t
//
// A list of predefined view point orientations:
typedef enum
{
  THE_ISOMETRIC_VIEW_E,
  THE_TOP_VIEW_E,
  THE_BOTTOM_VIEW_E,
  THE_LEFT_VIEW_E,
  THE_RIGHT_VIEW_E,
  THE_FRONT_VIEW_E,
  THE_BACK_VIEW_E,
  THE_XY_VIEW_E
} the_view_mgr_orientation_t;

//----------------------------------------------------------------
// the_view_orientation_text
// 
extern const char *
the_view_orientation_text(const the_view_mgr_orientation_t & orientation);

//----------------------------------------------------------------
// operator <<
// 
inline ostream &
operator << (ostream & s, const the_view_mgr_orientation_t & orientation)
{
  return s << the_view_orientation_text(orientation);
}

// global orientation vectors (look from and up):
extern const v3x1_t THE_ORIENTATION_LF[];
extern const v3x1_t THE_ORIENTATION_UP[];


#endif // THE_VIEW_MGR_ORIENTATION_HXX_
