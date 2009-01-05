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


// File         : the_polyline.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Sep 06 20:25:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A polyline -- C0 continuous curve.

#ifndef THE_POLYLINE_HXX_
#define THE_POLYLINE_HXX_

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
class the_bbox_t;


//----------------------------------------------------------------
// the_polyline_geom_t
// 
class the_polyline_geom_t : public the_curve_geom_t
{
public:
  void reset(const std::vector<p3x1_t> & pts,
	     const std::vector<float> & wts,
	     const std::vector<float> & kts);
  
  // evaluate this polyline at a given parameter:
  bool eval(p3x1_t & result, float & weight, const float & param) const;
  
  // lookup the segment corresponding to a given parameter,
  // return UINT_MAX on failure:
  unsigned int segment(const float & param) const;
  
  // virtual:
  bool eval(const float & t,
	    p3x1_t & P0, // position
	    v3x1_t & P1, // first derivative
	    v3x1_t & P2, // second derivative
	    float & curvature,
	    float & torsion) const;
  
  // virtual:
  bool position(const float & t, p3x1_t & p) const;
  
  // virtual:
  bool derivative(const float & t, v3x1_t & d) const;
  
  // virtual: returns number of segments:
  unsigned int
  init_slope_signs(const the_curve_deviation_t & deviation,
		   const unsigned int & steps_per_segment,
		   std::list<the_slope_sign_t> & slope_signs,
		   float & s0,
		   float & s1) const;
  
  // virtual: calculate the bounding box of the polyline:
  void calc_bbox(the_bbox_t & bbox) const;
  
  // virtual:
  float t_min() const { return kt_[0]; }
  float t_max() const { return kt_[kt_.size() - 1]; }
  
  // accessors:
  inline const std::vector<p3x1_t> & pt() const { return pt_; }
  inline const std::vector<float> & wt() const { return wt_; }
  inline const std::vector<float> & kt() const { return kt_; }
  
private:
  // points and their corresponding weights:
  std::vector<p3x1_t> pt_;
  std::vector<float> wt_;
  
  // parameter values mapped to each point:
  std::vector<float> kt_;
};


//----------------------------------------------------------------
// the_polyline_geom_dl_elem_t
// 
class the_polyline_geom_dl_elem_t : public the_dl_elem_t
{
public:
  the_polyline_geom_dl_elem_t(const the_polyline_geom_t & geom,
			      const the_color_t & color);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
private:
  the_polyline_geom_dl_elem_t();
  
  const the_polyline_geom_t & geom_;
  the_color_t color_;
};


//----------------------------------------------------------------
// the_polyline_t
// 
// Polyline primitive (supported by two or more points):
class the_polyline_t : public the_intcurve_t
{
public:
  // virtual:
  the_primitive_t * clone() const
  { return new the_polyline_t(*this); }
  
  // virtual: color of the model primitive:
  the_color_t color() const;
  
  // rebuild the display list according to the current point list:
  bool regenerate();
  
  // virtual: For debugging, dumps all segments
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // virtual:
  the_dl_elem_t * dl_elem() const
  { return new the_polyline_geom_dl_elem_t(geom_, color()); }
  
  // virtual:
  const the_polyline_geom_t & geom() const
  { return geom_; }
  
private:
  the_polyline_geom_t geom_;
};

//----------------------------------------------------------------
// the_polyline_pick_filter_t
// 
class the_polyline_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_polyline_t * polyline = registry->elem<the_polyline_t>(id);
    return (polyline != NULL);
  }
};


#endif // THE_POLYLINE_HXX_
