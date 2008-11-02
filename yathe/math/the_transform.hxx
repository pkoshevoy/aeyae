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


// File         : the_transform.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu Aug  5 11:16:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : This class represents a transformation from one
//                frame of reference to another frame.

#ifndef THE_TRANSFORM_HXX_
#define THE_TRANSFORM_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_coord_sys.hxx"

// forward declarations:
class m4x4_t;


//----------------------------------------------------------------
// the_transform_t
// 
class the_transform_t
{
public:
  // identity transform:
  the_transform_t();
  
  the_transform_t(const float * scale,
		  const float & rotation_axis_azimuth,
		  const float & rotation_axis_polar_angle,
		  const float & rotation_angle,
		  const float * translate);
  
  the_transform_t(const float & scale_x,
		  const float & scale_y,
		  const float & scale_z,
		  const float & rotation_axis_azimuth,
		  const float & rotation_axis_polar_angle,
		  const float & rotation_angle,
		  const float & translate_x,
		  const float & translate_y,
		  const float & translate_z);
  
  the_transform_t(const the_coord_sys_t & reference_frame,
		  const the_coord_sys_t & transformed_frame);
  
  void init(const float & scale_x,
	    const float & scale_y,
	    const float & scale_z,
	    const float & rotation_axis_azimuth,
	    const float & rotation_axis_polar_angle,
	    const float & rotation_angle,
	    const float & translate_x,
	    const float & translate_y,
	    const float & translate_z);
  
  void eval(const the_coord_sys_t & reference_frame,
	    m4x4_t & lcs_to_wcs,
	    m4x4_t & wcs_to_lcs) const;
  
  void eval(const the_coord_sys_t & reference_frame,
	    the_coord_sys_t & transformed_frame) const;
  
  // return unit axis expressed in the world coordinate system:
  inline const v3x1_t unit_axis(const the_coord_sys_t & reference_frame,
				const unsigned int & axis_id) const
  { return !(reference_frame.to_wcs(axis_[axis_id])); }
  
  // translate the transformed frame by a vector specified in
  // the world coordinate system:
  inline void translate(const the_coord_sys_t & reference_frame,
			const v3x1_t & wcs_vec)
  { translate_ += reference_frame.to_lcs(wcs_vec); }
  
  // scale the specified axis to match the given axis (expressed in wcs):
  void scale(const the_coord_sys_t & reference_frame,
	     const unsigned int & axis_id,
	     const v3x1_t & wcs_axis)
  { scale_[axis_id] = axis_[axis_id] * reference_frame.to_lcs(wcs_axis); }
  
  // rotate the other two coordinate axis about a specified rotation axis
  // by a given angle expressed in radians:
  void rotate(const unsigned int & axis_id,
	      const float & radians);
  
  // extract rotation information from this transform:
  // 
  void get_rotation(// rotation axis azimuth (longitude):
		    float & azimuth,
		    // rotation axis polar angle (colatitude):
		    float & polar,
		    // the rotation angle:
		    float & alpha) const;
  
  // expressed in the local coordinate system:
  float scale_[3];
  
  // unit expressed in the reference coordinate system:
  v3x1_t axis_[3];
  
  // expressed in the reference coordinate system:
  v3x1_t translate_;
};


#endif // THE_TRANSFORM_HXX_
