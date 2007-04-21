// File         : the_selset.cxx
// Author       : Paul A. Koshevoy
// Created      : Sat Jan 22 13:03:00 MDT 2005
// Copyright    : (C) 2005
// License      : GPL.
// Description  : 

// local includes:
#include "sel/the_selset.hxx"
#include "doc/the_graph.hxx"


//----------------------------------------------------------------
// the_selset_t::may_activate
// 
bool
the_selset_t::may_activate(const unsigned int & id) const
{
  if (registry()->elem(id) == NULL) return false;
  return !::has(records_, id);
}

//----------------------------------------------------------------
// the_selset_t::activate
// 
bool
the_selset_t::activate(const unsigned int & id)
{
  if (may_activate(id) == false) return false;
  
  records_.push_back(id);
  registry()->elem(id)->set_current_state(THE_SELECTED_STATE_E);
  return true;
}

//----------------------------------------------------------------
// the_selset_t::deactivate
// 
bool
the_selset_t::deactivate(const unsigned int & id)
{
  if (!::has(records_, id)) return false;
  
  records_.remove(id);
  registry()->elem(id)->clear_state(THE_SELECTED_STATE_E);
  return true;
}

//----------------------------------------------------------------
// the_selset_t::deactivate_all
// 
void
the_selset_t::deactivate_all()
{
  while (!records_.empty())
  {
    const unsigned int id = records_.front();
    deactivate(id);
  }
}

//----------------------------------------------------------------
// the_selset_t::set_active
// 
bool
the_selset_t::set_active(const unsigned int & id)
{
  deactivate_all();
  return activate(id);
}

//----------------------------------------------------------------
// the_selset_t::toggle_active
// 
void
the_selset_t::toggle_active(const unsigned int & id)
{
  if (!deactivate(id))
  {
    activate(id);
  }
}

//----------------------------------------------------------------
// the_selset_t::graph
// 
void
the_selset_t::graph(the_graph_t & graph) const
{
  graph.set_roots(registry(), records_);
}
