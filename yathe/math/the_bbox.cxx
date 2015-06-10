// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_bbox.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A bounding box aligned to a local coordinate system.

// system includes:
#include <assert.h>

// local includes:
#include "math/the_bbox.hxx"


//----------------------------------------------------------------
// the_bbox_t::operator +=
//
the_bbox_t &
the_bbox_t::operator += (const the_bbox_t & bbox)
{
  if (bbox.is_empty()) return *this;

  p3x1_t corner[8];
  bbox.wcs_corners(corner);
  return (*this) << corner[0]
		 << corner[1]
		 << corner[2]
		 << corner[3]
		 << corner[4]
		 << corner[5]
		 << corner[6]
		 << corner[7];
}

/*----------------------------------------------------------------
 * the_bbox_t::wcs_min_max
 */
void
the_bbox_t::wcs_min_max(unsigned int axis,
			p3x1_t & min,
			p3x1_t & max) const
{
  const p3x1_t & wcs_origin = ref_cs_.origin();
  const v3x1_t & wcs_axis_vec = ref_cs_[axis];
  min = wcs_origin + wcs_axis_vec * aa_.min_[axis];
  max = wcs_origin + wcs_axis_vec * aa_.max_[axis];
}

/*----------------------------------------------------------------
 * the_bbox_t::wcs_length
 */
float
the_bbox_t::wcs_length(unsigned int axis) const
{
  if (is_empty()) return 0.0;

  p3x1_t min;
  p3x1_t max;
  wcs_min_max(axis, min, max);
  return (max - min).norm();
}

//----------------------------------------------------------------
// the_bbox_t::wcs_corners
//
void
the_bbox_t::wcs_corners(p3x1_t * corner) const
{
  aa_.corners(corner);

  corner[0] = ref_cs_.to_wcs(corner[0]);
  corner[1] = ref_cs_.to_wcs(corner[1]);
  corner[2] = ref_cs_.to_wcs(corner[2]);
  corner[3] = ref_cs_.to_wcs(corner[3]);
  corner[4] = ref_cs_.to_wcs(corner[4]);
  corner[5] = ref_cs_.to_wcs(corner[5]);
  corner[6] = ref_cs_.to_wcs(corner[6]);
  corner[7] = ref_cs_.to_wcs(corner[7]);
}

//----------------------------------------------------------------
// the_bbox_t::radius
//
float
the_bbox_t::wcs_radius(const p3x1_t & wcs_center) const
{
  if (is_empty()) return 0.0;

  p3x1_t wcs_corner[8];
  wcs_corners(wcs_corner);

  float max_dist = -FLT_MAX;
  for (unsigned int i = 0; i < 8; i++)
  {
    float dist = ~(wcs_center - wcs_corner[i]);
    if (dist > max_dist) max_dist = dist;
  }

  return max_dist;
}

//----------------------------------------------------------------
// the_bbox_t::wcs_radius
//
float
the_bbox_t::wcs_radius(const p3x1_t & wcs_center, unsigned int axis_w_id) const
{
  p3x1_t lcs_center = ref_cs_.to_lcs(wcs_center);

  assert(axis_w_id < 3);
  unsigned int axis_u_id = (axis_w_id + 1) % 3;
  unsigned int axis_v_id = (axis_u_id + 1) % 3;

  p2x1_t uv_center(lcs_center[axis_u_id],
		   lcs_center[axis_v_id]);

  p2x1_t uv_corner[] = {
    p2x1_t(aa_.min_[axis_u_id], aa_.min_[axis_v_id]),
    p2x1_t(aa_.min_[axis_u_id], aa_.max_[axis_v_id]),
    p2x1_t(aa_.max_[axis_u_id], aa_.min_[axis_v_id]),
    p2x1_t(aa_.max_[axis_u_id], aa_.max_[axis_v_id])
  };

  float max_dist = -FLT_MAX;
  for (unsigned int i = 0; i < 4; i++)
  {
    v2x1_t uv_vec = uv_center - uv_corner[i];
    v3x1_t lcs_vec;
    lcs_vec[axis_u_id] = uv_vec[0];
    lcs_vec[axis_v_id] = uv_vec[1];
    lcs_vec[axis_w_id] = float(0);

    v3x1_t wcs_vec = ref_cs_.to_wcs(lcs_vec);
    float dist = ~wcs_vec;
    if (dist > max_dist) max_dist = dist;
  }

  return max_dist;
}

//----------------------------------------------------------------
// the_bbox_t::intersects
//
bool
the_bbox_t::intersects(const the_bbox_t & bbox) const
{
  p3x1_t corner[8];
  bbox.wcs_corners(corner);

  bool contained =
    contains(corner[0]) ||
    contains(corner[1]) ||
    contains(corner[2]) ||
    contains(corner[3]) ||
    contains(corner[4]) ||
    contains(corner[5]) ||
    contains(corner[6]) ||
    contains(corner[7]);
  if (contained) return true;

  wcs_corners(corner);
  contained =
    bbox.contains(corner[0]) ||
    bbox.contains(corner[1]) ||
    bbox.contains(corner[2]) ||
    bbox.contains(corner[3]) ||
    bbox.contains(corner[4]) ||
    bbox.contains(corner[5]) ||
    bbox.contains(corner[6]) ||
    bbox.contains(corner[7]);

  return contained;
}

//----------------------------------------------------------------
// the_bbox_t::dump
//
void
the_bbox_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_bbox_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "aa_  = ";
  aa_.dump(strm);
  strm << INDSTR << "ref_cs_  = ";
  ref_cs_.dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}


//----------------------------------------------------------------
// the_bbox_face_t::the_bbox_face_t
//
the_bbox_face_t::the_bbox_face_t(the_bbox_face_id_t face_id,
				 const the_bbox_t & bbox):
  corner_(4)
{
  const the_coord_sys_t & lcs = bbox.ref_cs();
  const p3x1_t & min = bbox.lcs_min();
  const p3x1_t & max = bbox.lcs_max();

  switch (face_id)
  {
    case THE_TOP_FACE_E: // F0
    {
      corner_[0] = lcs.to_wcs(p3x1_t(max.x(), min.y(), max.z())); // C0
      corner_[1] = lcs.to_wcs(p3x1_t(min.x(), min.y(), max.z())); // C1
      corner_[2] = lcs.to_wcs(p3x1_t(min.x(), max.y(), max.z())); // C5
      corner_[3] = lcs.to_wcs(p3x1_t(max.x(), max.y(), max.z())); // C4
    }
    break;

    case THE_BACK_FACE_E: // f1
    {
      corner_[0] = lcs.to_wcs(p3x1_t(min.x(), min.y(), max.z())); // C1
      corner_[1] = lcs.to_wcs(p3x1_t(min.x(), max.y(), max.z())); // C5
      corner_[2] = lcs.to_wcs(p3x1_t(min.x(), max.y(), min.z())); // C6
      corner_[3] = lcs.to_wcs(p3x1_t(min.x(), min.y(), min.z())); // c2
    }
    break;

    case THE_BOTTOM_FACE_E: // f2
    {
      corner_[0] = lcs.to_wcs(p3x1_t(max.x(), min.y(), min.z())); // C3
      corner_[1] = lcs.to_wcs(p3x1_t(min.x(), min.y(), min.z())); // c2
      corner_[2] = lcs.to_wcs(p3x1_t(min.x(), max.y(), min.z())); // C6
      corner_[3] = lcs.to_wcs(p3x1_t(max.x(), max.y(), min.z())); // C7
    }

    case THE_FRONT_FACE_E: // F3
    {
      corner_[0] = lcs.to_wcs(p3x1_t(max.x(), min.y(), max.z())); // C0
      corner_[1] = lcs.to_wcs(p3x1_t(max.x(), max.y(), max.z())); // C4
      corner_[2] = lcs.to_wcs(p3x1_t(max.x(), max.y(), min.z())); // C7
      corner_[3] = lcs.to_wcs(p3x1_t(max.x(), min.y(), min.z())); // C3
    }
    break;

    case THE_RIGHT_FACE_E: // f4
    {
      corner_[0] = lcs.to_wcs(p3x1_t(max.x(), min.y(), max.z())); // C0
      corner_[1] = lcs.to_wcs(p3x1_t(min.x(), min.y(), max.z())); // C1
      corner_[2] = lcs.to_wcs(p3x1_t(min.x(), min.y(), min.z())); // c2
      corner_[3] = lcs.to_wcs(p3x1_t(max.x(), min.y(), min.z())); // C3
    }
    break;

    case THE_LEFT_FACE_E: // F5
    {
      corner_[0] = lcs.to_wcs(p3x1_t(max.x(), max.y(), max.z())); // C4
      corner_[1] = lcs.to_wcs(p3x1_t(min.x(), max.y(), max.z())); // C5
      corner_[2] = lcs.to_wcs(p3x1_t(min.x(), max.y(), min.z())); // C6
      corner_[3] = lcs.to_wcs(p3x1_t(max.x(), max.y(), min.z())); // C7
    }

    default: assert(false);
  }
}
