// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_transform.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu Aug  5 11:16:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : This class represents a transformation from one
//                frame of reference to another frame.

// local includes:
#include "math/the_transform.hxx"
#include "math/v3x1p3x1.hxx"
#include "math/m4x4.hxx"

// system includes:
#include <math.h>
#include <algorithm>


//----------------------------------------------------------------
// the_transform_t::the_transform_t
//
the_transform_t::the_transform_t()
{
  scale_[0] = 1.0;
  scale_[1] = 1.0;
  scale_[2] = 1.0;

  axis_[0].assign(1.0, 0.0, 0.0);
  axis_[1].assign(0.0, 1.0, 0.0);
  axis_[2].assign(0.0, 0.0, 1.0);

  translate_.assign(0.0, 0.0, 0.0);
}

//----------------------------------------------------------------
// the_transform_t::the_transform_t
//
the_transform_t::the_transform_t(const float * scale,
				 const float & rotation_axis_azimuth,
				 const float & rotation_axis_polar_angle,
				 const float & rotation_angle,
				 const float * translate)
{
  init(scale[0], scale[1], scale[2],
       rotation_axis_azimuth, rotation_axis_polar_angle, rotation_angle,
       translate[0], translate[1], translate[2]);
}

//----------------------------------------------------------------
// the_transform_t::the_transform_t
//
the_transform_t::the_transform_t(const float & scale_x,
				 const float & scale_y,
				 const float & scale_z,
				 const float & rotation_axis_azimuth,
				 const float & rotation_axis_polar_angle,
				 const float & rotation_angle,
				 const float & translate_x,
				 const float & translate_y,
				 const float & translate_z)
{
  init(scale_x, scale_y, scale_z,
       rotation_axis_azimuth, rotation_axis_polar_angle, rotation_angle,
       translate_x, translate_y, translate_z);
}

//----------------------------------------------------------------
// the_transform_t::the_transform_t
//
the_transform_t::
the_transform_t(const the_coord_sys_t & reference_frame,
		const the_coord_sys_t & transformed_frame)
{
  reference_frame.wcs_to_lcs(transformed_frame.x_axis(), axis_[0]);
  reference_frame.wcs_to_lcs(transformed_frame.y_axis(), axis_[1]);
  reference_frame.wcs_to_lcs(transformed_frame.z_axis(), axis_[2]);

  scale_[0] = axis_[0].norm();
  assert(scale_[0] != 0.0);

  scale_[1] = axis_[1].norm();
  assert(scale_[1] != 0.0);

  scale_[2] = axis_[2].norm();
  assert(scale_[2] != 0.0);

  axis_[0].normalize();
  axis_[1].normalize();
  axis_[2].normalize();

  p3x1_t transformed_frame_origin_lcs;
  reference_frame.wcs_to_lcs(transformed_frame.origin(),
			     transformed_frame_origin_lcs);
  translate_.assign(transformed_frame_origin_lcs.data());
}

//----------------------------------------------------------------
// the_transform_t::init
//
void
the_transform_t::init(const float & scale_x,
		      const float & scale_y,
		      const float & scale_z,
		      const float & rotation_axis_azimuth,
		      const float & rotation_axis_polar_angle,
		      const float & rotation_angle,
		      const float & translate_x,
		      const float & translate_y,
		      const float & translate_z)
{
  scale_[0] = scale_x;
  scale_[1] = scale_y;
  scale_[2] = scale_z;

  v3x1_t rotation_axis;
  the_coord_sys_t::sph_to_xyz(v3x1_t(1.0,
				     rotation_axis_azimuth,
				     rotation_axis_polar_angle),
			      rotation_axis);

  const m4x4_t r(rotation_axis, rotation_angle);
  axis_[0] = r * v3x1_t(1.0, 0.0, 0.0);
  axis_[1] = r * v3x1_t(0.0, 1.0, 0.0);
  axis_[2] = r * v3x1_t(0.0, 0.0, 1.0);

  translate_.assign(translate_x, translate_y, translate_z);
}

//----------------------------------------------------------------
// the_transform_t::eval
//
void
the_transform_t::eval(const the_coord_sys_t & reference_frame,
		      m4x4_t & transform,
		      m4x4_t & transform_inverse) const
{
  m4x4_t wcs_to_ref;
  m4x4_t ref_to_wcs;
  reference_frame.wcs_to_lcs(wcs_to_ref);
  reference_frame.lcs_to_wcs(ref_to_wcs);

  the_coord_sys_t transform_lcs(axis_[0] * scale_[0],
				axis_[1] * scale_[1],
				axis_[2] * scale_[2],
				p3x1_t(translate_.data()));
  m4x4_t ref_to_lcs;
  m4x4_t lcs_to_ref;
  transform_lcs.wcs_to_lcs(ref_to_lcs);
  transform_lcs.lcs_to_wcs(lcs_to_ref);

  transform =
    ref_to_wcs *
    lcs_to_ref *
    wcs_to_ref;

  transform_inverse =
    ref_to_wcs *
    ref_to_lcs *
    wcs_to_ref;
}

//----------------------------------------------------------------
// the_transform_t::eval
//
void
the_transform_t::eval(const the_coord_sys_t & reference_frame,
		      the_coord_sys_t & transformed_frame) const
{
  reference_frame.lcs_to_wcs(axis_[0], transformed_frame.x_axis());
  reference_frame.lcs_to_wcs(axis_[1], transformed_frame.y_axis());
  reference_frame.lcs_to_wcs(axis_[2], transformed_frame.z_axis());

  transformed_frame.x_axis().scale(scale_[0]);
  transformed_frame.y_axis().scale(scale_[1]);
  transformed_frame.z_axis().scale(scale_[2]);

  reference_frame.lcs_to_wcs(p3x1_t(translate_.data()),
			     transformed_frame.origin());
}

//----------------------------------------------------------------
// the_transform_t::rotate
//
void
the_transform_t::rotate(const unsigned int & axis_id,
			const float & radians)
{
  const m4x4_t rotate(axis_[axis_id], radians);

  v3x1_t & axis_u = axis_[(axis_id + 1) % 3];
  v3x1_t & axis_v = axis_[(axis_id + 2) % 3];

  axis_u = rotate * axis_u;
  axis_v = rotate * axis_v;
}

//----------------------------------------------------------------
// the_transform_t::get_rotation
//
void
the_transform_t::get_rotation(// rotation axis azimuth (longitude):
			      float & azimuth,
			      // rotation axis polar angle (colatitude):
			      float & polar,
			      // the rotation angle:
			      float & alpha) const
{
  static const float angle_eps = 1e-7f;

  const float r[3][3] =
  {
    {
      axis_[0].x(),
      axis_[1].x(),
      axis_[2].x(),
    },

    {
      axis_[0].y(),
      axis_[1].y(),
      axis_[2].y(),
    },

    {
      axis_[0].z(),
      axis_[1].z(),
      axis_[2].z(),
    }
  };

  const float trace_r = r[0][0] + r[1][1] + r[2][2];
  const float cos_alpha = (trace_r - 1) / 2;

  const float r3223 = r[2][1] - r[1][2];
  const float r1331 = r[0][2] - r[2][0];
  const float r2112 = r[1][0] - r[0][1];
  const float sin_alpha = 0.5f * sqrt(r3223 * r3223 +
				      r1331 * r1331 +
				      r2112 * r2112);

  alpha = atan2(sin_alpha, cos_alpha);

  if (fabs(alpha) < angle_eps)
  {
    alpha = 0;
    azimuth = 0;
    polar = 0;
  }
  else
  {
    v3x1_t rot_axis(r3223, r1331, r2112);
    rot_axis /= (2 * sin_alpha);

    v3x1_t sph_axis;
    the_coord_sys_t::xyz_to_sph(rot_axis, sph_axis);

    polar = float(std::min(M_PI, std::max(0.0, double(sph_axis.z()))));
    if (polar < angle_eps || fabs(polar - M_PI) < angle_eps)
    {
      azimuth = 0;
    }
    else
    {
      azimuth = sph_axis.y();
    }
  }
}
