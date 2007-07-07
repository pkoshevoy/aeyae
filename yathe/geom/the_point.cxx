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


// File         : the_point.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The 3D point primitive.

// local includes:
#include "geom/the_point.hxx"
#include "doc/the_reference.hxx"
#include "math/the_view_volume.hxx"
#include "sel/the_pick_rec.hxx"

// system includes:
#include <assert.h>

//----------------------------------------------------------------
// USE_SMART_SOFT_POINTS
// 
#define USE_SMART_SOFT_POINTS

#ifdef USE_SMART_SOFT_POINTS
#include <geom/the_curve.hxx>
#include <math/the_deviation.hxx>
#endif


//----------------------------------------------------------------
// the_point_t::intersect
// 
bool
the_point_t::
intersect(const the_view_volume_t & volume,
	  std::list<the_pick_data_t> & data) const
{
  const p3x1_t wcs_pt = value();
  
  // shortcuts:
  p3x1_t cyl_pt;
  float & radius = cyl_pt.x();
  float & angle  = cyl_pt.y();
  float & depth  = cyl_pt.z();
  
  // find the depth of this point within the volume:
  depth = volume.depth_of_wcs_pt(wcs_pt);
  
  // find the UV frame within the volume at the specified depth:
  the_cyl_uv_csys_t uv_frame;
  volume.uv_frame_at_depth(depth, uv_frame);
  
  // calculate the polar coordinates of this point within the UV frame:
  uv_frame.wcs_to_lcs(wcs_pt, radius, angle);
  
  // if the radius of this point within the UV frame exceeds 1.0 then
  // this point is outside the volume - does not intersect:
  if (radius > 1.0)
  {
    return false;
  }
  
  // this point falls within the volume boundaries:
  data.push_back(the_pick_data_t(cyl_pt, new the_point_ref_t(id())));
  return true;
}

//----------------------------------------------------------------
// the_point_t::save
// 
bool
the_point_t::save(std::ostream & stream) const
{
  ::save(stream, anchor_);
  ::save(stream, weight_);
  return the_primitive_t::save(stream);
}

//----------------------------------------------------------------
// the_point_t::load
// 
bool
the_point_t::load(std::istream & stream)
{
  ::load(stream, anchor_);
  ::load(stream, weight_);
  return the_primitive_t::load(stream);
}

//----------------------------------------------------------------
// the_point_t::dump
// 
void
the_point_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_point_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_primitive_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}


//----------------------------------------------------------------
// the_hard_point_t::set_value
// 
bool
the_hard_point_t::set_value(const the_view_mgr_t & /* view_mgr */,
			    const p3x1_t & wcs_pt)
{
  value_ = wcs_pt;
  request_regeneration();
  return true;
}

//----------------------------------------------------------------
// the_hard_point_t::save
// 
bool
the_hard_point_t::save(std::ostream & stream) const
{
  ::save(stream, value_);
  return the_point_t::save(stream);
}

//----------------------------------------------------------------
// the_hard_point_t::load
// 
bool
the_hard_point_t::load(std::istream & stream)
{
  ::load(stream, value_);
  return the_point_t::load(stream);
}

//----------------------------------------------------------------
// the_hard_point_t::dump
// 
void
the_hard_point_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_hard_point_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_point_t::dump(strm, INDNXT);
  strm << INDSTR << "value_ = " << value_ << ";" << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_soft_point_t::the_soft_point_t
// 
the_soft_point_t::the_soft_point_t(const the_reference_t & ref):
  the_point_t()
{
  ref_ = ref.clone();
}

//----------------------------------------------------------------
// the_soft_point_t::the_soft_point_t
// 
the_soft_point_t::the_soft_point_t(const the_soft_point_t & point):
  the_point_t(point),
  ref_(point.ref_->clone())
{}

//----------------------------------------------------------------
// the_soft_point_t::~the_soft_point_t
// 
the_soft_point_t::~the_soft_point_t()
{
  delete ref_;
  ref_ = NULL;
}

//----------------------------------------------------------------
// the_soft_point_t::added_to_the_registry
// 
void
the_soft_point_t::added_to_the_registry(the_registry_t * registry,
					const unsigned int & id)
{
  the_point_t::added_to_the_registry(registry, id);
  
  // update the graph:
  establish_supporter_dependent(registry, ref_->id(), id);
  
#ifdef USE_SMART_SOFT_POINTS
  ref_->eval(registry, value_);
  anchor_ = value_;
#endif
}

//----------------------------------------------------------------
// the_soft_point_t::regenerate
// 
bool
the_soft_point_t::regenerate()
{
  the_registry_t * r = registry();
  
#ifdef USE_SMART_SOFT_POINTS
  the_curve_t * curve = ref_->references<the_curve_t>(r);
  if (curve)
  {
    /*
    if (has_state(THE_SELECTED_STATE_E))
    {
      cout << "regenerating an active soft point" << endl;
    }
    */
    the_point_curve_deviation_t deviation(value_, curve->geom());
    std::list<the_deviation_min_t> solution;
    if (deviation.find_local_minima(solution))
    {
      solution.sort();
      /*
      if (has_state(THE_SELECTED_STATE_E))
      {
	for (std::list<the_deviation_min_t>::const_iterator
	       i = solution.begin(); i != solution.end(); ++i)
	{
	  const the_deviation_min_t & sr = *i;
	  cout << sr.s_ << '\t' << sr.r_ << endl;
	}
	cout << endl;
      }
      */
      
      const the_deviation_min_t & srs = solution.front();
      the_curve_ref_t * crv_ref = dynamic_cast<the_curve_ref_t *>(ref_);
      assert(crv_ref);
      /*
      float delta = fabs(srs.s_ - crv_ref->param());
      if (has_state(THE_SELECTED_STATE_E))
      {
	cout << srs.s_ << " - " << crv_ref->param() << " = " << delta << endl;
	if (delta > 0.9)
	{
	  std::list<the_deviation_min_t> tmp;
	  deviation.find_local_minima(tmp);
	}
      }
      */
      crv_ref->set_param(srs.s_);
    }
  }
#endif
  
  // look up the reference, evaluate the paramater with
  // respect to the reference, return the value:
  regenerated_ = ref_->eval(r, value_);
  return regenerated_;
}

//----------------------------------------------------------------
// the_soft_point_t::set_value
// 
bool
the_soft_point_t::set_value(const the_view_mgr_t & view_mgr,
			    const p3x1_t & wcs_pt)
{
  the_registry_t * r = registry();
  bool ok = ref_->move(r, view_mgr, wcs_pt);
  ref_->eval(r, value_);
  
  if (ok) request_regeneration();
  return ok;
}

//----------------------------------------------------------------
// the_soft_point_t::symbol
// 
the_point_symbol_id_t
the_soft_point_t::symbol() const
{
  return ref_->symbol();
}

//----------------------------------------------------------------
// the_soft_point_t::save
// 
bool
the_soft_point_t::save(std::ostream & stream) const
{
  ::save(stream, ref_);
  ::save(stream, value_);
  return the_point_t::save(stream);
}

//----------------------------------------------------------------
// the_soft_point_t::load
// 
bool
the_soft_point_t::load(std::istream & stream)
{
  ::load(stream, ref_);
  ::load(stream, value_);
  return the_point_t::load(stream);
}

//----------------------------------------------------------------
// the_soft_point_t::dump
// 
void
the_soft_point_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_soft_point_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_point_t::dump(strm, INDNXT);
  strm << INDSTR << "ref_ =" << endl;
  ref_->dump(strm, INDNXT);
  strm << INDSTR << "value_ = " << value_ << ";" << endl
       << INDSCP << "}" << endl << endl;
}


//----------------------------------------------------------------
// the_point_ref_t::the_point_ref_t
// 
the_point_ref_t::the_point_ref_t(const unsigned int & id):
  the_reference_t(id)
{}

//----------------------------------------------------------------
// the_point_ref_t::eval
// 
bool
the_point_ref_t::eval(the_registry_t * r, p3x1_t & pt) const
{
  the_point_t * vert = r->elem<the_point_t>(id());
  if (vert == NULL) return false;
  
  pt = vert->value();
  return true;
}

//----------------------------------------------------------------
// the_point_ref_t::dump
// 
void
the_point_ref_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_point_ref_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_reference_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}
