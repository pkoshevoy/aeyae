// File         : the_curve_selset.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun May 13 16:48:38 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A selection set class for interpolation curves.

// the includes:
#include <sel/the_curve_selset.hxx>


//----------------------------------------------------------------
// delete_point
// 
void
delete_point(the_registry_t * registry,
	     the_procedure_t * proc,
	     const unsigned int & id)
{
  the_point_t * point = registry->elem<the_point_t>(id);
  if (point == NULL) return;
  
  // delete the point:
  breakdown_supporter_dependent(registry, proc->id(), id);
  registry->del(point);
  delete point;
  
  proc->request_regeneration();
}

//----------------------------------------------------------------
// delete_curve
// 
void
delete_curve(the_registry_t * registry,
	     the_procedure_t * proc,
	     const unsigned int & id)
{
  the_intcurve_t * curve = registry->elem<the_intcurve_t>(id);
  if (curve == NULL) return;
  
  // delete all curve points:
  while (!curve->pts().empty())
  {
    unsigned int point_id = curve->pts().front().id_;
    curve->del(point_id);
    
    the_primitive_t * point = registry->elem(point_id);
    breakdown_supporter_dependent(registry, proc->id(), point_id);
    registry->del(point);
    delete point;
  }
  
  // delete the curve:
  breakdown_supporter_dependent(registry, proc->id(), id);
  registry->del(curve);
  delete curve;
  
  proc->request_regeneration();
}


//----------------------------------------------------------------
// the_curve_selrec_t::reset_anchors
// 
void
the_curve_selrec_t::reset_anchors() const
{
  for (std::list<unsigned int>::const_iterator iter(records().begin());
       iter != records().end(); ++iter)
  {
    the_point_t * pt = prim<the_point_t>(*iter);
    pt->set_anchor();
  }
}

//----------------------------------------------------------------
// the_curve_selrec_t::activate_all_points
// 
void
the_curve_selrec_t::activate_all_points()
{
  deactivate_all();
  
  const std::list<the_knot_point_t> & pts = get<the_intcurve_t>()->pts();
  for (std::list<the_knot_point_t>::const_iterator i = pts.begin();
       i != pts.end(); ++i)
  {
    activate((*i).id_);
  }
}


//----------------------------------------------------------------
// the_curve_selset_traits_t::clone
// 
the_curve_selset_traits_t::super_t *
the_curve_selset_traits_t::clone() const
{
  return new the_curve_selset_traits_t(*this);
}

//----------------------------------------------------------------
// the_curve_selset_traits_t::is_valid
// 
bool
the_curve_selset_traits_t::is_valid(const the_curve_selrec_t & rec) const
{
  return rec.get<the_intcurve_t>();
}

//----------------------------------------------------------------
// the_curve_selset_traits_t::activate
// 
bool
the_curve_selset_traits_t::activate(the_curve_selrec_t & rec) const
{
  rec.get<the_intcurve_t>()->set_current_state(THE_SELECTED_STATE_E);
  return true;
}

//----------------------------------------------------------------
// the_curve_selset_traits_t::deactivate
// 
bool
the_curve_selset_traits_t::deactivate(the_curve_selrec_t & rec) const
{
  rec.deactivate_all();
  rec.get<the_intcurve_t>()->clear_state(THE_SELECTED_STATE_E);
  
  // if the curve is invalid, get rid of it and its points permanently:
  if (!rec.get<the_intcurve_t>()->has_at_least_two_points())
  {
    delete_curve(rec.traits<the_selset_traits_t>()->registry(),
		 selset_.proc(),
		 rec.id());
  }
  
  return true;
}


//----------------------------------------------------------------
// the_curve_selset_t::the_curve_selset_t
// 
the_curve_selset_t::the_curve_selset_t():
  the_curve_selset_t::super_t(new the_curve_selset_traits_t(*this))
{}

//----------------------------------------------------------------
// the_curve_selset_t::the_curve_selset_t
// 
the_curve_selset_t::the_curve_selset_t(const the_curve_selset_t & selset):
  the_curve_selset_t::super_t(selset)
{}

//----------------------------------------------------------------
// the_curve_selset_t::append_active_curve
// 
bool
the_curve_selset_t::
append_active_curve(const unsigned int & curve_id)
{
  return activate(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::remove_active_curve
// 
bool
the_curve_selset_t::
remove_active_curve(const unsigned int & curve_id)
{
  return deactivate(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::set_active_curve
// 
bool
the_curve_selset_t::set_active_curve(const unsigned int & curve_id)
{
  return set_active(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::toggle_active_curve
// 
void
the_curve_selset_t::
toggle_active_curve(const unsigned int & curve_id)
{
  return toggle_active(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::remove_active_points
// 
void
the_curve_selset_t::remove_active_points()
{
  for (std::list<the_curve_selrec_t>::iterator iter = records_.begin();
       iter != records_.end(); ++iter)
  {
    the_curve_selrec_t & selrec = *iter;
    selrec.deactivate_all();
  }
}

//----------------------------------------------------------------
// the_curve_selset_t::active_curve_of_point
// 
unsigned int
the_curve_selset_t::
active_curve_of_point(const unsigned int & point_id) const
{
  the_knot_point_t kp(point_id);
  
  for (std::list<the_curve_selrec_t>::const_iterator
	 iter = records_.begin(); iter != records_.end(); ++iter)
  {
    const the_curve_selrec_t & selrec = *iter;
    the_intcurve_t * curve = selrec.get<the_intcurve_t>();
    
    if (::has(curve->pts(), kp))
    {
      return selrec.id();
    }
  }
  
  return UINT_MAX;
}

//----------------------------------------------------------------
// the_curve_selset_t::reset_anchors
// 
void
the_curve_selset_t::reset_anchors()
{
  for (std::list<the_curve_selrec_t>::iterator iter = records_.begin();
       iter != records_.end(); ++iter)
  {
    the_curve_selrec_t & selrec = *iter;
    selrec.reset_anchors();
  }
}

//----------------------------------------------------------------
// the_curve_selset_t::count_active_points
// 
unsigned int
the_curve_selset_t::count_active_points() const
{
  unsigned int num_pts = 0;
  for (std::list<the_curve_selrec_t>::const_iterator
	 iter = records_.begin(); iter != records_.end(); ++iter)
  {
    const the_curve_selrec_t & selrec = *iter;
    num_pts += selrec.records().size();
  }
  
  return num_pts;
}

//----------------------------------------------------------------
// the_curve_sel_set_t::record_with_points
// 
const the_curve_selrec_t *
the_curve_selset_t::
record_with_points(const unsigned int & num_points) const
{
  for (std::list<the_curve_selrec_t>::const_iterator i = records_.begin();
       i != records_.end(); ++i)
  {
    const the_curve_selrec_t & selrec = *i;
    if (selrec.records().size() != num_points) continue;
    
    return &selrec;
  }
  
  return NULL;
}

//----------------------------------------------------------------
// the_curve_selset_t::record
// 
const the_curve_selrec_t *
the_curve_selset_t::record(const unsigned int & curve_id) const
{
  return has(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::record
// 
the_curve_selrec_t *
the_curve_selset_t::record(const unsigned int & curve_id)
{
  return has(the_curve_selrec_t(curve_id));
}

//----------------------------------------------------------------
// the_curve_selset_t::has_active_point
// 
bool
the_curve_selset_t::has_active_point(const unsigned int & point_id,
				     the_intcurve_t *& curve,
				     the_point_t *& point) const
{
  curve = NULL;
  point = NULL;
  
  for (std::list<the_curve_selrec_t>::const_iterator
	 iter = records_.begin(); iter != records_.end(); ++iter)
  {
    const the_curve_selrec_t & selrec = *iter;
    if (!selrec.has(point_id))
    {
      continue;
    }
    
    curve = selrec.get<the_intcurve_t>();
    point = registry()->elem<the_point_t>(point_id);
    return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_curve_selset_t::graph
// 
void
the_curve_selset_t::graph(the_graph_t & graph) const
{
  std::list<unsigned int> curve_ids;
  
  for (std::list<the_curve_selrec_t>::const_iterator
	 iter = records_.begin(); iter != records_.end(); ++iter)
  {
    const the_curve_selrec_t & selrec = *iter;
    curve_ids.push_back(selrec.id());
  }
  
  graph.set_roots(registry(), curve_ids);
}
