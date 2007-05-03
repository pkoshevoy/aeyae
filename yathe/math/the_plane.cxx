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
