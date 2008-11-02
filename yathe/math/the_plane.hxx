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


// File         : the_plane.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A 3D plane class.

#ifndef THE_PLANE_HXX_
#define THE_PLANE_HXX_

// local includes:
#include "math/the_coord_sys.hxx"

// forward declarations:
class the_ray_t;


//----------------------------------------------------------------
// the_plane_t
// 
class the_plane_t
{
public:
  the_plane_t(const the_coord_sys_t & cs): cs_(cs) {}
  
  // equality test operators:
  inline bool operator == (const the_plane_t & p) const
  { return (cs_ == p.cs_); }
  
  inline bool operator != (const the_plane_t & p) const
  { return !(*this == p); }
  
  // intersect this plane with a ray, return true if successful,
  // store the result as parameter t along ray:
  bool intersect(const the_ray_t & r, float & param) const;
  
  // intersect this plane with a ray and return the point of intersection:
  const p3x1_t operator * (const the_ray_t & r) const;
  
  // project a point onto this plane:
  const p2x1_t operator * (const p3x1_t & wcs_pt) const;
  
  // express a local (u, v) point in the world coordinate system:
  const p3x1_t operator * (const p2x1_t & lcs_pt) const;
  
  // find the normal vector from the plane to a given point:
  const v3x1_t operator - (const p3x1_t & wcs_pt) const;
  
  // accessor to the local coordinate system:
  inline const the_coord_sys_t & cs() const
  { return cs_; }
  
  // calculate the distance from a point to the plane along the plane normal:
  inline float dist(const p3x1_t & pt) const
  { return cs_.z_axis() * (pt - cs_.origin()); }
  
  // For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
private:
  // Coordinate system of this plane. Z is the normal, O is a point in plane:
  the_coord_sys_t cs_;
};

extern ostream &
operator << (ostream & stream, const the_plane_t & plane);


#endif // THE_PLANE_HXX_
