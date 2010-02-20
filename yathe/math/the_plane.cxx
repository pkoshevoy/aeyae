// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_plane.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 3D plane class.

// local includes:
#include "math/the_plane.hxx"
#include "math/the_ray.hxx"
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_plane_t::intersect
// 
bool
the_plane_t::intersect(const the_ray_t & r, float & param) const
{
  // make sure the plane and ray are not parallel to each other:
  if (fabs(r.v() * cs_.z_axis()) == 0.0) return false;
  
  param = -((cs_.z_axis() * (r.p() - cs_.origin())) /
	    (cs_.z_axis() * r.v()));
  return true;
}

//----------------------------------------------------------------
// the_plane_t::operator *
// 
// NOTE: this function should only be called when you are
//       certain that a solution exists:
// 
const p3x1_t
the_plane_t::operator * (const the_ray_t & r) const
{
  float param = FLT_MAX;
  
#ifndef NDEBUG
  bool ok =
#endif
    intersect(r, param);
  assert(ok);
  
  return r * param;
}

//----------------------------------------------------------------
// the_plane_t::operator *
// 
const p2x1_t
the_plane_t::operator * (const p3x1_t & wcs_pt) const
{
  p3x1_t lcs_pt = cs_.to_lcs(wcs_pt);
  return p2x1_t(lcs_pt.x(), lcs_pt.y());
}

//----------------------------------------------------------------
// the_plane_t::operator *
// 
const p3x1_t
the_plane_t::operator * (const p2x1_t & lcs_pt) const
{
  return cs_.to_wcs(p3x1_t(lcs_pt.x(), lcs_pt.y(), 0.0));
}

//----------------------------------------------------------------
// the_plane_t::operator -
// 
const v3x1_t
the_plane_t::operator - (const p3x1_t & wcs_pt) const
{
  p3x1_t lcs_pt = cs_.to_lcs(wcs_pt);
  return cs_.z_axis() * lcs_pt.z();
}

//----------------------------------------------------------------
// the_plane_t::dump
// 
void
the_plane_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_plane_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "cs_ = ";
  cs_.dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & stream, const the_plane_t & plane)
{
  plane.dump(stream);
  return stream;
}
