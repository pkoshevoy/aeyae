// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_tensurf.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Dec 28 13:36:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A tensor product surface.

#ifndef THE_TENSURF_HXX_
#define THE_TENSURF_HXX_

// local includes:
#include "doc/the_primitive.hxx"
#include "math/v3x1p3x1.hxx"
#include "geom/the_bspline.hxx"
#include "opengl/the_vertex.hxx"
#include "opengl/the_disp_list.hxx"

// system includes:
#include <vector>

// forward declarations:
class the_grid_t;


//----------------------------------------------------------------
// the_tensurf_t
//
// A tensor b-spline surface defined by two uniform knot vectors
// and supported by a quadrilateral grid.
//
class the_tensurf_t : public the_primitive_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_tensurf_t(*this); }

  // virtual:
  const char * name() const
  { return "the_tensurf_t"; }

  // virtual:
  bool regenerate();

  // virtual: display a piecewise linear approximation of the surface:
  the_dl_elem_t * dl_elem() const;

  // the tensor surface is supported by a grid:
  the_grid_t * grid() const;

  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;

private:
  the_knot_vector_t knot_vector_u_;
  the_knot_vector_t knot_vector_v_;

  std::vector<std::vector<the_vertex_t> > tri_mesh_;
};


#endif // THE_TENSURF_HXX_
