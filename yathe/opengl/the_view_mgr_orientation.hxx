// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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

// io:
extern bool save(std::ostream & stream, the_view_mgr_orientation_t d);
extern bool load(std::istream & stream, the_view_mgr_orientation_t & d);


#endif // THE_VIEW_MGR_ORIENTATION_HXX_
