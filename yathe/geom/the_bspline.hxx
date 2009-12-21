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


// File         : the_bspline.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Nov 17 17:50:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A bspline curve class.

#ifndef THE_BSPLINE_HXX_
#define THE_BSPLINE_HXX_

// system includes:
#include <list>
#include <vector>

// local includes:
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "math/v3x1p3x1.hxx"
#include "opengl/the_disp_list.hxx"
#include "sel/the_pick_filter.hxx"
#include "geom/the_curve.hxx"

// forward declarations:
class the_polyline_t;
class the_knot_vector_t;


//----------------------------------------------------------------
// the_bspline_geom_t
// 
class the_bspline_geom_t : public the_curve_geom_t
{
public:
  void reset(const std::vector<p3x1_t> & pts,  // control points
	     const std::vector<float> & kts); // knots
  
  // virtual: evaluate this curve at a given parameter:
  bool eval(const float & t,
	    p3x1_t & P0, // position
	    v3x1_t & P1, // first derivative
	    v3x1_t & P2, // second derivative
	    float & curvature,
	    float & torsion) const;
  
  // virtual: a stripped down version of eval:
  bool position(const float & t, p3x1_t & position) const;
  
  // virtual: a stripped down version of eval:
  bool derivative(const float & t, v3x1_t & derivative) const;
  
  // virtual: a stripped down version of eval:
  bool position_and_derivative(const float & t,
			       p3x1_t & position,
			       v3x1_t & derivative) const;
  
  // virtual:
  size_t
  init_slope_signs(const the_curve_deviation_t & deviation,
		   const size_t & steps_per_segment,
		   std::list<the_slope_sign_t> & slope_signs,
		   float & s0,
		   float & s1) const;
  
  // virtual: calculate the bounding box of the curve:
  void calc_bbox(the_bbox_t & bbox) const;
  
  // virtual:
  float t_min() const;
  float t_max() const;
  
  // order of this curve:
  inline size_t order() const  { return kt_.size() - pt_.size(); }
  
  // degree of this curve:
  inline size_t degree() const { return kt_.size() - pt_.size() - 1; }
  
  // accessors:
  inline const std::vector<p3x1_t> & pt() const { return pt_; }
  inline const std::vector<float> & kt() const { return kt_; }
  
private:
  // find the first knot vector that bounds the parameter t on the
  // interval [tau i, tau i+1) - J in Elane Cohens' book:
  size_t find_segment_index(const float & t) const;
  
  std::vector<p3x1_t> pt_; // control points
  std::vector<float> kt_; // knot vector
};


//----------------------------------------------------------------
// the_bspline_geom_dl_elem_t
// 
class the_bspline_geom_dl_elem_t : public the_dl_elem_t
{
public:
  the_bspline_geom_dl_elem_t(const the_bspline_geom_t & geom,
			     const the_color_t & color);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
private:
  the_bspline_geom_dl_elem_t();
  
  const the_bspline_geom_t & geom_;
  the_color_t color_;
};


//----------------------------------------------------------------
// the_knot_vector_t
// 
class the_knot_vector_t : public the_primitive_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_knot_vector_t(*this); }
  
  // virtual:
  const char * name() const
  { return "knot vector"; }
  
  // initialize the knot vector:
  bool init(const size_t & degree,
	    const std::vector<float> & knots);
  
  bool init(const size_t & degree,
	    const size_t & num_pt,
	    const float & t0 = 0.0,
	    const float & t1 = 1.0,
	    const bool & a_floating = false,
	    const bool & b_floating = false);
  
  void set_target_degree(const size_t & target_degree);
  
  // virtual:
  bool regenerate();
  
  // helper:
  bool update(const size_t & polyline_pts);
  
  // accessor to the polyline that supports (defines) this curve:
  the_polyline_t * polyline() const;
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // accessors:
  inline const size_t & target_degree() const
  { return target_degree_; }
  
  inline const size_t & degree() const
  { return degree_; }
  
  inline const std::vector<float> & knots() const
  { return knots_; }
  
private:
  // adjust the knot vector while trying to maintain the end conditions:
  bool raise_degree();
  bool lower_degree();
  
  bool insert_point();
  bool remove_point();
  
  // the highest degree of the bspline curve:
  size_t target_degree_;
  
  // the degree of the bspline curve:
  size_t degree_;
  
  // the knots:
  std::vector<float> knots_;
};


//----------------------------------------------------------------
// the_bspline_t
// 
class the_bspline_t : public the_curve_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_bspline_t(*this); }
  
  // virtual:
  const char * name() const
  { return "bspline curve"; }
  
  // virtual: rebuild the display list according to the current point list:
  bool regenerate();
  
  // virtual: display a piecewise linear approximation of the curve:
#if 0
  the_dl_elem_t * dl_elem() const
  { return new the_bspline_geom_dl_elem_t(geom(), color()); }
#else
  the_dl_elem_t * dl_elem() const
  {
    return new the_curve_geom_dl_elem_t(geom(),
					color(),
					100 + geom().kt().size());
  }
#endif
  
  // accessor to the knot vector that supports (defines) this curve:
  the_knot_vector_t * knot_vector() const;
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // virtual:
  const the_bspline_geom_t & geom() const
  { return geom_; }
  
private:
  the_bspline_geom_t geom_;
};


//----------------------------------------------------------------
// the_bspline_pick_filter_t
// 
class the_bspline_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_bspline_t * bspline = registry->elem<the_bspline_t>(id);
    return (bspline != NULL);
  }
};

//----------------------------------------------------------------
// the_tangent_id_t
//
// THE_HEAD_TANGENT_E corresponds to the first end point of the
// interpolation points.
// 
// THE_TAIL_TANGENT_E corresponds to the last end point of the
// interpolation points.
// 
typedef enum
{
  THE_HEAD_TANGENT_E,
  THE_TAIL_TANGENT_E
} the_tangent_id_t;


//----------------------------------------------------------------
// the_interpolation_bspline_t
// 
class the_interpolation_bspline_t : public the_intcurve_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_interpolation_bspline_t(*this); }
  
  // virtual:
  const char * name() const
  { return "interpolation bspline"; }
  
  // virtual: color of the model primitive:
  the_color_t color() const;
  
  // rebuild the display list according to the current point list:
  bool regenerate();
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // virtual:
  the_dl_elem_t * dl_elem() const
  { return new the_bspline_geom_dl_elem_t(geom(), color()); }
  
  // virtual:
  const the_bspline_geom_t & geom() const
  { return geom_; }

  // helper:
  bool update_geom(const std::vector<p3x1_t> & pts,
		   const std::vector<float> & wts,
		   const std::vector<float> & kts);
  
protected:
  // virtual:
  void setup_parameterization();
  
private:
  // helper:
  const p3x1_t bessel_pt(const std::vector<p3x1_t> & pts,
			 const std::vector<float> & kts,
			 const the_tangent_id_t & tan_id) const;
  
  the_bspline_geom_t geom_;
};


#endif // THE_BSPLINE_HXX_
