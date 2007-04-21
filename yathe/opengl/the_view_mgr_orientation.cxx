// File         : the_view_mgr_orientation.cxx
// Author       : Paul A. Koshevoy
// Created      : Sun May 30 19:58:00 MDT 2004
// Copyright    : (C) 2004
// License      : GPL.
// Description  : 

// system includes:
#include <assert.h>

// local includes:
#include "opengl/the_view_mgr_orientation.hxx"


//----------------------------------------------------------------
// the_view_name
// 
const char *
the_view_orientation_text(const the_view_mgr_orientation_t & orientation)
{
  switch (orientation)
  {
    case THE_ISOMETRIC_VIEW_E:	return "Isometric";
    case THE_TOP_VIEW_E:	return "Top";
    case THE_BOTTOM_VIEW_E:	return "Bottom";
    case THE_LEFT_VIEW_E:	return "Left";
    case THE_RIGHT_VIEW_E:	return "Right";
    case THE_FRONT_VIEW_E:	return "Front";
    case THE_BACK_VIEW_E:	return "Back";
    case THE_XY_VIEW_E:		return "XY";
  }
  
  assert(false);
  return NULL;
}

//----------------------------------------------------------------
// THE_ORIENTATION_LF
// 
// this array should be accessed via the_view_type_t:
// 
const v3x1_t
THE_ORIENTATION_LF[] =
{
  !v3x1_t( 1.0,  1.0,  1.0), // isometric
  v3x1_t(  0.0,  0.0,  1.0), // top
  v3x1_t(  0.0,  0.0, -1.0), // bottom
  v3x1_t(  0.0,  1.0,  0.0), // left
  v3x1_t(  0.0, -1.0,  0.0), // right
  v3x1_t(  1.0,  0.0,  0.0), // front
  v3x1_t( -1.0,  0.0,  0.0), // back
  v3x1_t(  0.0,  0.0,  1.0)  // bottom
};

//----------------------------------------------------------------
// THE_ORIENTATION_UP
// 
// this array should be accessed via the_view_type_t:
// 
const v3x1_t
THE_ORIENTATION_UP[] =
{
  !v3x1_t(-1.0, -1.0, 2.0), // isometric
  v3x1_t(  0.0, -1.0, 0.0), // top
  v3x1_t(  0.0, -1.0, 0.0), // bottom
  v3x1_t(  0.0,  0.0, 1.0), // left
  v3x1_t(  0.0,  0.0, 1.0), // right
  v3x1_t(  0.0,  0.0, 1.0), // front
  v3x1_t(  0.0,  0.0, 1.0), // back
  v3x1_t(  0.0,  1.0, 0.0)  // xy
};
