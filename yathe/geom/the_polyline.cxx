// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_polyline.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Sep 06 20:32:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A polyline -- C0 continuous curve.

// local includes:
#include "geom/the_polyline.hxx"
#include "geom/the_point.hxx"
#include "doc/the_reference.hxx"
#include "math/the_bbox.hxx"
#include "opengl/the_view_mgr.hxx"
#include "opengl/the_appearance.hxx"

// system includes:
#include <list>
#include <vector>
#include <assert.h>


//----------------------------------------------------------------
// the_polyline_geom_t::reset
// 
void
the_polyline_geom_t::reset(const std::vector<p3x1_t> & pts,
			   const std::vector<float> & wts,
			   const std::vector<float> & kts)
{
  pt_ = pts;
  wt_ = wts;
  kt_ = kts;
}

//----------------------------------------------------------------
// the_polyline_geom_t::eval
// 
bool
the_polyline_geom_t::eval(p3x1_t & result,
			  float & weight,
			  const float & param) const
{
  // shortcut:
  const size_t & num_pts = pt_.size();
  
  if (num_pts == 0) return false;
  if ((param < kt_[0]) || (param > kt_[num_pts - 1])) return false;
  
  for (size_t i = 1; i < num_pts; i++)
  {
    if (param > kt_[i]) continue;
    
    float t = normalize((param - kt_[i - 1]), (kt_[i] - kt_[i - 1]));
    result = pt_[i - 1] + t * (pt_[i] - pt_[i - 1]);
    weight = wt_[i - 1] + t * (wt_[i] - wt_[i - 1]);
    return true;
  }
  
  result = pt_[0];
  weight = wt_[0];
  return true;
}

//----------------------------------------------------------------
// the_polyline_geom_t::segment
// 
size_t
the_polyline_geom_t::segment(const float & param) const
{
  if (param >= 0.0)
  {
    const size_t & num_pts = pt_.size();
    for (size_t i = 1; i < num_pts; i++)
    {
      if (param <= kt_[i]) return i - 1;
    }
  }
  
  return UINT_MAX;
}

//----------------------------------------------------------------
// the_polyline_geom_t::eval
// 
bool
the_polyline_geom_t::eval(const float & t,
			  p3x1_t & P0, // position
			  v3x1_t & P1, // first derivative
			  v3x1_t & P2, // second derivative
			  float & curvature,
			  float & torsion) const
{
  bool ok = (position(t, P0) && derivative(t, P1));
  P2.assign(0.0, 0.0, 0.0);
  curvature = 0.0;
  torsion = 0.0;
  return ok;
}

//----------------------------------------------------------------
// the_polyline_geom_t::position
// 
bool
the_polyline_geom_t::position(const float & t, p3x1_t & p) const
{
  float weight;
  return eval(p, weight, t);
}

//----------------------------------------------------------------
// the_polyline_geom_t::derivative
// 
bool
the_polyline_geom_t::derivative(const float & t, v3x1_t & d) const
{
  size_t s = segment(t);
  if (s >= pt_.size())
  {
    d.assign(0.0, 0.0, 0.0);
    return false;
  }
  
  d = pt_[s + 1] - pt_[s];
  return true;
}

//----------------------------------------------------------------
// the_polyline_geom_t::init_slope_signs
// 
size_t
the_polyline_geom_t::
init_slope_signs(const the_curve_deviation_t & deviation,
		 const size_t & steps_per_segment,
		 std::list<the_slope_sign_t> & slope_signs,
		 float & s0,
		 float & s1) const
{
  if (pt_.size() < 2) return 0;
  
  s0 = t_min();
  s1 = t_max();
  
  deviation.store_slope_sign(slope_signs, s0);
  
  size_t segments = pt_.size() - 1;
  for (size_t i = 0; i < segments; i++)
  {
    const float k[] =
    {
      kt_[i],
      kt_[i + 1]
    };
    
    for (size_t j = 0; j < steps_per_segment; j++)
    {
      float s =
	k[0] + (((0.5f + float(j)) / float(steps_per_segment)) *
		(k[1] - k[0]));
      deviation.store_slope_sign(slope_signs, s);
    }
  }
  
  deviation.store_slope_sign(slope_signs, s1);
  return segments;
}

//----------------------------------------------------------------
// the_polyline_geom_t::calc_bbox
// 
void
the_polyline_geom_t::calc_bbox(the_bbox_t & bbox) const
{
  const size_t & num_pts = pt_.size();
  for (size_t i = 0; i < num_pts; i++)
  {
    bbox << pt_[i];
  }
}

//----------------------------------------------------------------
// the_polyline_geom_dl_elem_t::the_polyline_geom_dl_elem_t
// 
the_polyline_geom_dl_elem_t::
the_polyline_geom_dl_elem_t(const the_polyline_geom_t & geom,
			    const the_color_t & color):
  the_dl_elem_t(),
  geom_(geom),
  color_(color)
{}

//----------------------------------------------------------------
// the_polyline_geom_dl_elem_t::draw
// 
void
the_polyline_geom_dl_elem_t::draw() const
{
  const std::vector<p3x1_t> & pts = geom_.pt();
  const size_t & num_pts = pts.size();
  if (num_pts < 2) return;
  
  for (size_t i = 1; i < num_pts; i++)
  {
    const p3x1_t & a = pts[i - 1];
    const p3x1_t & b = pts[i];
    
    the_line_dl_elem_t(a, b, color_).draw();
  }
}

//----------------------------------------------------------------
// the_polyline_geom_dl_elem_t::update_bbox
// 
void
the_polyline_geom_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  geom_.calc_bbox(bbox);
}


//----------------------------------------------------------------
// the_polyline_t::color
// 
the_color_t
the_polyline_t::color() const
{
  the_color_t c(THE_APPEARANCE.palette().curve()[current_state()]);
  c += the_color_t::WHITE;
  c *= 0.5;
  
  return c;
}

//----------------------------------------------------------------
// the_polyline_t::regenerate
// 
bool
the_polyline_t::regenerate()
{
  // the polyline must have at least two points to regenerate properly:
  if (pts_.size() < 1) return false;
  
  setup_parameterization();
  
  std::vector<p3x1_t> pts(pts_.size());
  std::vector<float> wts(pts_.size());
  std::vector<float> kts(pts_.size());
  point_values(pts, wts, kts);
  
  geom_.reset(pts, wts, kts);
  return true;
}

//----------------------------------------------------------------
// the_polyline_t::dump
// 
void
the_polyline_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_polyline_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_intcurve_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}
