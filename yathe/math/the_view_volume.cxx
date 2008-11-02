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


// File         : the_view_volume.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 30 18:55:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : Helper class used to evaluate the view volume.

// local includes:
#include "math/the_view_volume.hxx"
#include "math/v3x1p3x1.hxx"
#include "math/the_ray.hxx"
#include "math/the_coord_sys.hxx"


//----------------------------------------------------------------
// the_view_volume_t::the_view_volume_t
// 
the_view_volume_t::the_view_volume_t():
  w_axis_orthogonal_(0, 0, 1)
{
  origin_[0] = p3x1_t(0, 0, 0);
  origin_[1] = p3x1_t(0, 0, 1);
  
  u_axis_[0] = v3x1_t(1, 0, 0);
  u_axis_[1] = v3x1_t(1, 0, 0);
  
  v_axis_[0] = v3x1_t(0, 1, 0);
  v_axis_[1] = v3x1_t(0, 1, 0);
}

//----------------------------------------------------------------
// the_view_volume_t::the_view_volume_t
// 
// the simplest way to setup a viewing volume:
the_view_volume_t::
the_view_volume_t(// look-from point, look-at point, and up vector
		  // are expressed in the world coordinate system,
		  // up vector is a unit vector:
		  const p3x1_t & wcs_lf,
		  const p3x1_t & wcs_la,
		  const v3x1_t & wcs_up,
		  
		  // near and far are parameters along unit vector
		  // from look-from point to look-at point:
		  const float & near_plane,
		  const float & far_plane,
		  
		  // dimensions of the near clipping face
		  // (expressed in the world coordinate system):
		  const float & wcs_width,
		  const float & wcs_height,
		  
		  // a flag indicating wether the viewing volume is
		  // orthogonal or perpsecive:
		  const bool & perspective)
{
  // setup the volume:
  setup(wcs_lf,
	wcs_la,
	wcs_up,
	near_plane,
	far_plane,
	wcs_width,
	wcs_height,
	perspective);
}

//----------------------------------------------------------------
// the_view_volume_t::operator
// 
the_view_volume_t &
the_view_volume_t::operator = (const the_view_volume_t & view_volume)
{
  origin_[0] = view_volume.origin_[0];
  origin_[1] = view_volume.origin_[1];
  
  u_axis_[0] = view_volume.u_axis_[0];
  u_axis_[1] = view_volume.u_axis_[1];
  
  v_axis_[0] = view_volume.v_axis_[0];
  v_axis_[1] = view_volume.v_axis_[1];
  
  w_axis_orthogonal_ = view_volume.w_axis_orthogonal_;
  
  return *this;
}

//----------------------------------------------------------------
// the_view_volume_t::setup
// 
void
the_view_volume_t::setup(const p3x1_t * near_face, const p3x1_t * far_face)
{
  origin_[0] = near_face[0];
  origin_[1] = far_face[0];
  
  u_axis_[0] = near_face[1] - near_face[0];
  u_axis_[1] = far_face[1] - far_face[0];
  
  v_axis_[0] = near_face[3] - near_face[0];
  v_axis_[1] = far_face[3] - far_face[0];
  
  v3x1_t u_unit = !u_axis_[0];
  v3x1_t v_unit = !v_axis_[0];
  v3x1_t w_unit = !(u_unit % v_unit);
  v3x1_t w_axis = origin_[1] - origin_[0];
  w_axis_orthogonal_ = w_unit * (w_axis * w_unit);
}

//----------------------------------------------------------------
// the_view_volume_t::setup
// 
// an intuitive way to set up a view volume:
void
the_view_volume_t::setup(// look-from point, look-at point, and up vector
			 // are expressed in the world coordinate system,
			 // up vector is a unit vector:
			 const p3x1_t & wcs_lf,
			 const p3x1_t & wcs_la,
			 const v3x1_t & wcs_up,
			 
			 // near and far are parameters along unit vector
			 // from look-from point to look-at point:
			 const float & near_plane,
			 const float & far_plane,
			 
			 // dimensions of the near clipping face
			 // (expressed in the world coordinate system):
			 const float & wcs_width,
			 const float & wcs_height,
			 
			 // a flag indicating wether the viewing volume is
			 // orthogonal or perpsecive:
			 const bool & perspective)
{
  v3x1_t unit_z = !(wcs_la - wcs_lf); // unit vector from LF to LA.
  v3x1_t unit_y = !(-wcs_up);         // unit vector pointing down.
  v3x1_t unit_x = !(unit_y % unit_z); // unit vector pointing to the right.
  
  // center points of the near/far clipping faces:
  p3x1_t center[] =
  {
    wcs_lf + unit_z * near_plane,
    wcs_lf + unit_z * far_plane
  };
  
  // find corners of the near/far clipping faces:
  p3x1_t face[2][4];
  
  // near face:
  face[0][0] = (center[0] -
		unit_x * (wcs_width / 2) -
		unit_y * (wcs_height / 2));
  
  face[0][1] = (center[0] +
		unit_x * (wcs_width / 2) -
		unit_y * (wcs_height / 2));
  
  face[0][2] = (center[0] +
		unit_x * (wcs_width / 2) +
		unit_y * (wcs_height / 2));
  
  face[0][3] = (center[0] -
		unit_x * (wcs_width / 2) +
		unit_y * (wcs_height / 2));
  
  // far face:
  float scale = perspective ? (far_plane / near_plane) : 1;
  
  face[1][0] = (center[1] -
		unit_x * scale * (wcs_width / 2) -
		unit_y * scale * (wcs_height / 2));
  
  face[1][1] = (center[1] +
		unit_x * scale * (wcs_width / 2) -
		unit_y * scale * (wcs_height / 2));
  
  face[1][2] = (center[1] +
		unit_x * scale * (wcs_width / 2) +
		unit_y * scale * (wcs_height / 2));
  
  face[1][3] = (center[1] -
		unit_x * scale * (wcs_width / 2) +
		unit_y * scale * (wcs_height / 2));
  
  // setup the volume:
  setup(face[0], face[1]);
}

//----------------------------------------------------------------
// the_view_volume_t::sub_volume
// 
void
the_view_volume_t::sub_volume(const p2x1_t & scs_min,
			      const p2x1_t & scs_max,
			      the_view_volume_t & volume) const
{
  p3x1_t face[2][4] =
  {
    {
      origin_[0] + u_axis_[0] * scs_min.x() + v_axis_[0] * scs_min.y(),
      origin_[0] + u_axis_[0] * scs_max.x() + v_axis_[0] * scs_min.y(),
      origin_[0] + u_axis_[0] * scs_max.x() + v_axis_[0] * scs_max.y(),
      origin_[0] + u_axis_[0] * scs_min.x() + v_axis_[0] * scs_max.y()
    },
    
    {
      origin_[1] + u_axis_[1] * scs_min.x() + v_axis_[1] * scs_min.y(),
      origin_[1] + u_axis_[1] * scs_max.x() + v_axis_[1] * scs_min.y(),
      origin_[1] + u_axis_[1] * scs_max.x() + v_axis_[1] * scs_max.y(),
      origin_[1] + u_axis_[1] * scs_min.x() + v_axis_[1] * scs_max.y()
    }
  };
  
  volume.setup(face[0], face[1]);
}

//----------------------------------------------------------------
// the_view_volume_t::scs_pt_to_ray
// 
void
the_view_volume_t::scs_pt_to_ray(const p2x1_t & scs_pt, the_ray_t & ray) const
{
  p3x1_t near_pt = (origin_[0] +
		    u_axis_[0] * scs_pt.x() +
		    v_axis_[0] * scs_pt.y());
  
  p3x1_t far_pt = (origin_[1] +
		   u_axis_[1] * scs_pt.x() +
		   v_axis_[1] * scs_pt.y());
  
  ray.p() = near_pt;
  ray.v() = far_pt - near_pt;
}

//----------------------------------------------------------------
// the_view_volume_t::dump
// 
void
the_view_volume_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_view_volume_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "origin_[0] = " << origin_[0] << endl
       << INDSTR << "origin_[1] = " << origin_[1] << endl
       << INDSTR << "u_axis_[0] = " << u_axis_[0] << endl
       << INDSTR << "u_axis_[1] = " << u_axis_[1] << endl
       << INDSTR << "v_axis_[0] = " << v_axis_[0] << endl
       << INDSTR << "v_axis_[1] = " << v_axis_[1] << endl
       << INDSTR << "w_axis_orthogonal_ = " << w_axis_orthogonal_ << endl
       << INDSCP << "}" << endl << endl;
}
