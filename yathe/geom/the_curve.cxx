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


// File         : the_curve.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Oct 31 16:45:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : An abstract curve class.

// system includes:
#include <vector>
#include <list>

// local includes:
#include "geom/the_curve.hxx"
#include "opengl/the_view_mgr.hxx"
#include "sel/the_pick_rec.hxx"
#include "utils/the_utils.hxx"


//----------------------------------------------------------------
// the_curve_geom_t::intersect
// 
bool
the_curve_geom_t::intersect(const p3x1_t & point,
			    std::list<the_deviation_min_t> & s_rs) const
{
  // construct a bounding box for the control points:
  the_bbox_t bbox;
  calc_bbox(bbox);
  
  // double the bbox just in case:
  bbox += bbox.wcs_radius();
  
  // early rejection test:
  if (bbox.contains(point) == false) return false;
  
  the_point_curve_deviation_t deviation(point, *this);
  if (deviation.find_local_minima(s_rs) == false) return false;
  
  s_rs.sort();
  return true;
}

//----------------------------------------------------------------
// the_curve_geom_t::intersect
// 
bool
the_curve_geom_t::intersect(const the_view_volume_t & volume,
			    std::list<the_deviation_min_t> & s_rs) const
{
  // deviation function:
  the_volume_curve_deviation_t deviation(volume, *this);
  
  // find the local minima:
  std::list<the_deviation_min_t> minima;
  deviation.find_local_minima(minima);
  minima.sort();
  
  for (std::list<the_deviation_min_t>::iterator i = minima.begin();
       i != minima.end(); ++i)
  {
    const float & rs = (*i).r_;
    if (rs > 1.0) continue;
    
    s_rs.push_back(*i);
  }
  
  return (s_rs.empty() == false);
}

//----------------------------------------------------------------
// the_curve_geom_dl_elem_t::the_curve_geom_dl_elem_t
// 
the_curve_geom_dl_elem_t::
the_curve_geom_dl_elem_t(const the_curve_geom_t & geom,
			 const the_color_t & color,
			 const unsigned int & segments):
  geom_(geom),
  segments_(segments)
{
  zebra_[0] = color;
  zebra_[1] = color;
}

//----------------------------------------------------------------
// the_curve_geom_dl_elem_t
// 
the_curve_geom_dl_elem_t::
the_curve_geom_dl_elem_t(const the_curve_geom_t & geom,
			 const the_color_t & zebra_a,
			 const the_color_t & zebra_b,
			 const unsigned int & segments):
  geom_(geom),
  segments_(segments)
{
  zebra_[0] = zebra_a;
  zebra_[1] = zebra_b;
}

//----------------------------------------------------------------
// the_curve_geom_dl_elem_t::draw
// 
void
the_curve_geom_dl_elem_t::draw() const
{
  const float t0 = geom_.t_min();
  const float t1 = geom_.t_max();
  const float dt = t1 - t0;
  
  std::vector<p3x1_t> p(segments_ + 1);
  if (!geom_.position(t0, p[0])) return;
  
  for (unsigned int i = 0; i <= segments_; i++)
  {
    geom_.position(t0 + dt * (float(i) / float(segments_)), p[i]);
  }
  
  for (unsigned int i = 1; i <= segments_; i++)
  {
    const p3x1_t & a = p[i - 1];
    const p3x1_t & b = p[i];
    
    the_line_dl_elem_t(a, b, zebra_[(i - 1) % 2]).draw();
  }
}

//----------------------------------------------------------------
// the_curve_geom_dl_elem_t::update_bbox
// 
void
the_curve_geom_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  geom_.calc_bbox(bbox);
}


//----------------------------------------------------------------
// the_curve_t::intersect
// 
bool
the_curve_t::
intersect(const the_view_volume_t & volume,
	  std::list<the_pick_data_t> & data) const
{
  // shortcut to the curve geometry:
  const the_curve_geom_t & curve_geom = geom();
  
  std::list<the_deviation_min_t> s_rs;
  curve_geom.intersect(volume, s_rs);
  
  for (std::list<the_deviation_min_t>::iterator i = s_rs.begin();
       i != s_rs.end(); ++i)
  {
    p3x1_t wcs_pt;
    curve_geom.position((*i).s_, wcs_pt);
    
    p3x1_t cyl_pt;
    volume.wcs_to_cyl(wcs_pt, cyl_pt);
    
    data.push_back(the_pick_data_t(cyl_pt,
				   new the_curve_ref_t(id(), (*i).s_)));
  }
  
  return (data.empty() == false);
}

//----------------------------------------------------------------
// the_curve_ref_t::the_curve_ref_t
// 
the_curve_ref_t::the_curve_ref_t(unsigned int id,
				 float param):
  the_reference_t(id),
  param_(param)
{}

//----------------------------------------------------------------
// the_curve_ref_t::eval
// 
bool
the_curve_ref_t::eval(the_registry_t * registry, p3x1_t & wcs_pt) const
{
  the_curve_t * curve = registry->elem<the_curve_t>(id());
  if (curve == NULL) return false;
  
  return curve->geom().position(param_, wcs_pt);
}

//----------------------------------------------------------------
// the_curve_ref_t::move
// 
bool
the_curve_ref_t::move(the_registry_t * registry,
		      const the_view_mgr_t & view_mgr,
		      const p3x1_t & wcs_pt)
{
  the_curve_t * curve = registry->elem<the_curve_t>(id());
  if (curve == NULL) return false;
  
  the_view_volume_t volume = view_mgr.view_volume();
  p2x1_t scs_pt = volume.to_scs(wcs_pt);
  view_mgr.setup_pick_volume(volume,
			     scs_pt,
			     std::max(view_mgr.width(),
				      view_mgr.height()) * 0.5);
  the_volume_curve_deviation_t deviation(volume, curve->geom());
  std::list<the_deviation_min_t> solution;
  if (deviation.find_local_minima(solution) == false) return false;
  
  solution.sort();
  const the_deviation_min_t & best = solution.front();
  param_ = best.s_;
  return true;
}

//----------------------------------------------------------------
// the_curve_ref_t::equal
// 
bool
the_curve_ref_t::equal(const the_reference_t * ref) const
{
  if (!the_reference_t::equal(ref)) return false;
  
  const the_curve_ref_t * curve_ref =
    dynamic_cast<const the_curve_ref_t *>(ref);
  if (curve_ref == NULL) return false;
  
  return (curve_ref->param_ == param_);
}

//----------------------------------------------------------------
// the_curve_ref_t::save
// 
bool
the_curve_ref_t::save(std::ostream & stream) const
{
  ::save(stream, param_);
  return the_reference_t::save(stream);
}

//----------------------------------------------------------------
// the_curve_ref_t::load
// 
bool
the_curve_ref_t::load(std::istream & stream)
{
  ::load(stream, param_);
  return the_reference_t::load(stream);
}

//----------------------------------------------------------------
// the_curve_ref_t::dump
// 
void
the_curve_ref_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_curve_ref_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_reference_t::dump(strm, INDNXT);
  strm << INDSTR << "param_ = " << param_ << ";" << endl
       << INDSCP << "}" << endl << endl;
}


//----------------------------------------------------------------
// the_knot_point_t::save
// 
bool
the_knot_point_t::save(std::ostream & stream) const
{
  return (::save(stream, id_) && ::save(stream, param_));
}

//----------------------------------------------------------------
// the_knot_point_t::load
// 
bool
the_knot_point_t::load(std::istream & stream)
{
  return (::load(stream, id_) && ::load(stream, param_));
}


//----------------------------------------------------------------
// the_intcurve_t::insert
// 
bool
the_intcurve_t::insert(const unsigned int & pt_id)
{
  // Go through the list of points, and find two adjacent
  // points closest to the new point. Insert the new
  // point in between of these two points.
  if (!has_at_least_two_points()) return add(pt_id);
  
  the_point_t * pt = point(pt_id);
  const p3x1_t & wcs_pt = pt->value();
  
  std::list<the_deviation_min_t> s_rs;
  if (!geom().intersect(wcs_pt, s_rs)) return false;
  
  float param = s_rs.front().s_;
  return insert(pt_id, param);
}


//----------------------------------------------------------------
// the_intcurve_t::insert
// 
bool
the_intcurve_t::insert(const unsigned int & pt_id, float param)
{
  // Go through the list of points, and find two adjacent
  // points closest to the new point. Insert the new
  // point in between of these two points.
  if (!has_at_least_two_points()) return add(pt_id);
  
  unsigned int seg_id = segment(param);
  assert(seg_id != UINT_MAX);
  pts_.insert(iterator_at_index(pts_, seg_id), the_knot_point_t(pt_id));
  
  // update the graph:
  establish_supporter_dependent(registry(), pt_id, id());
  
  return true;
}

//----------------------------------------------------------------
// the_intcurve_t::add_head
// 
bool
the_intcurve_t::add_head(const unsigned int & pt_id)
{
  pts_.push_front(the_knot_point_t(pt_id));
  
  // update the graph:
  establish_supporter_dependent(registry(), pt_id, id());
  
  return true;
}

//----------------------------------------------------------------
// the_intcurve_t::add_tail
// 
bool
the_intcurve_t::add_tail(const unsigned int & pt_id)
{
  pts_.push_back(the_knot_point_t(pt_id));
  
  // update the graph:
  establish_supporter_dependent(registry(), pt_id, id());
  
  return true;
}

//----------------------------------------------------------------
// the_intcurve_t::add
// 
bool
the_intcurve_t::add(const unsigned int & pt_id)
{
  // add a point to the list (pick the closest end of the curve
  // and add either to the tail or the head):
  if (!has_at_least_two_points()) return add_tail(pt_id);
  
  // decide wether to add to the tail or head:
  the_point_t * point_0 = point(pts_.front().id_);
  the_point_t * point_1 = point(pts_.back().id_);
  
  const p3x1_t & p0 = point_0->value();
  const p3x1_t & p1 = point_1->value();
  
  the_point_t * point_n = point(pt_id);
  const p3x1_t & pt = point_n->value();
  
  if ((pt - p0).norm_sqrd() < (pt - p1).norm_sqrd())
  {
    // add at the head:
    pts_.push_front(the_knot_point_t(pt_id));
  }
  else
  {
    // add at the tail:
    pts_.push_back(the_knot_point_t(pt_id));
  }
  
  // update the graph:
  establish_supporter_dependent(registry(), pt_id, id());
  
  return true;
}

//----------------------------------------------------------------
// the_intcurve_t::del
// 
bool
the_intcurve_t::del(const unsigned int & pt_id)
{
  // Find the affected segments:
  unsigned int idx = index_of(pts_, the_knot_point_t(pt_id));
  if (idx == UINT_MAX) return false;
  
  // remove the point from this curve:
  pts_.remove(the_knot_point_t(pt_id));
  
  // update the graph:
  breakdown_supporter_dependent(registry(), pt_id, id());
  
  return true;
}

//----------------------------------------------------------------
// the_intcurve_t::remove_duplicates
// 
void
the_intcurve_t::remove_duplicates(const float & threshold)
{
  std::list<the_knot_point_t>::iterator ia = pts_.begin();
  if (ia == pts_.end()) return;
  
  p3x1_t pa = point((*ia).id_)->value();
  
  std::list<the_knot_point_t>::iterator ib = next(ia);
  while (ib != pts_.end())
  {
    std::list<the_knot_point_t>::iterator ic = next(ib);
    p3x1_t pb = point((*ib).id_)->value();
    float d2 = (pb - pa).norm_sqrd();
    
    if (d2 > threshold)
    {
      ia = ib;
      pa = pb;
    }
    else
    {
      ib = pts_.erase(ib);
    }
    
    ib = ic;
  }
}

//----------------------------------------------------------------
// the_intcurve_t::save
// 
bool
the_intcurve_t::save(std::ostream & stream) const
{
  return (::save<the_knot_point_t>(stream, pts_) &&
	  the_curve_t::save(stream));
}

//----------------------------------------------------------------
// the_intcurve_t::load
// 
bool
the_intcurve_t::load(std::istream & stream)
{
  return (::load<the_knot_point_t>(stream, pts_) &&
	  the_curve_t::load(stream));
}

//----------------------------------------------------------------
// the_intcurve_t::dump
// 
void
the_intcurve_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_intcurve_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_curve_t::dump(strm, INDNXT);
  strm << INDSTR << "pts_ =" << endl;
  ::dump(strm, pts_);
  strm << INDSTR << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_intcurve_t::setup_parameterization
// 
void
the_intcurve_t::setup_parameterization()
{
  if (!has_at_least_two_points()) return;
  float length = 0.0;
  
  the_knot_point_t & ka = pts_.front();
  p3x1_t pa = point(ka.id_)->value();
  ka.param_ = 0;
  
  for (std::list<the_knot_point_t>::iterator ib = ++(pts_.begin());
       ib != pts_.end(); ++ib)
  {
    the_knot_point_t & kb = *ib;
    p3x1_t pb = point(kb.id_)->value();
    length += (pb - pa).norm();
    kb.param_ = length;
    
    pa = pb;
  }
  
  if (length == 0.0)
  {
    length = 1.0;
    pts_.back().param_ = length;
    return;
  }
  
  for (std::list<the_knot_point_t>::iterator i = ++(pts_.begin());
       i != pts_.end(); ++i)
  {
    the_knot_point_t & kb = *i;
    kb.param_ /= length;
  }
}

//----------------------------------------------------------------
// the_intcurve_t::segment1
// 
unsigned int
the_intcurve_t::segment(const float & param) const
{
  if (pts_.empty())
  {
    return UINT_MAX;
  }
  
  if (param > pts_.back().param_)
  {
    return pts_.size();
  }
  
  unsigned int index = 0;
  for (std::list<the_knot_point_t>::const_iterator i = pts_.begin();
       i != pts_.end(); ++i)
  {
    if (param <= (*i).param_) return index;
    index++;
  }
  
  return UINT_MAX;
}

//----------------------------------------------------------------
// the_intcurve_t::point_values
// 
void
the_intcurve_t::point_values(std::vector<p3x1_t> & wcs_pts,
			     std::vector<float> & weights,
			     std::vector<float> & params) const
{
  if (pts_.empty()) return;
  
  std::list<p3x1_t> pt_list;
  std::list<float> wt_list;
  std::list<float> kt_list;

  std::list<the_knot_point_t>::const_iterator i = pts_.begin();
  pt_list.push_back(point((*i).id_)->value());
  wt_list.push_back(point((*i).id_)->weight());
  kt_list.push_back((*i).param_);
  
  for (i = next(i); i != pts_.end(); ++i)
  {
    if (fabs(kt_list.back() - (*i).param_) <= THE_EPSILON) continue;
    
    the_point_t * pt = point((*i).id_);
    assert(pt != NULL);
    
    pt_list.push_back(pt->value());
    wt_list.push_back(pt->weight());
    kt_list.push_back((*i).param_);
  }
  
  copy_a_to_b(pt_list, wcs_pts);
  copy_a_to_b(wt_list, weights);
  copy_a_to_b(kt_list, params);
}

//----------------------------------------------------------------
// the_intcurve_t::has_point_value
// 
bool
the_intcurve_t::has_point_value(const p3x1_t & pt_a) const
{
  for (std::list<the_knot_point_t>::const_iterator i = pts_.begin();
       i != pts_.end(); ++i)
  {
    the_point_t * pt = point((*i).id_);
    assert(pt != NULL);
    
    const p3x1_t & pt_b = pt->value();
    
    v3x1_t ab = pt_b - pt_a;
    if (ab.norm_sqrd() < THE_EPSILON) return true;
  }
  
  return false;
}
