// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_pick_list.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A container for user selected document primitives.

// local includes:
#include "sel/the_pick_list.hxx"
#include "doc/the_document.hxx"
#include "doc/the_graph.hxx"
#include "math/the_view_volume.hxx"
#include "opengl/the_view.hxx"
#include "utils/the_indentation.hxx"

// system includes:
#include <assert.h>


//----------------------------------------------------------------
// the_pick_list_t::default_filter_
//
const the_pick_filter_t
the_pick_list_t::default_filter_;

//----------------------------------------------------------------
// the_pick_list_t::the_pick_list_t
//
the_pick_list_t::the_pick_list_t():
  the::unique_list<the_pick_rec_t>()
{}

//----------------------------------------------------------------
// the_pick_list_t::the_pick_list_t
//
the_pick_list_t::the_pick_list_t(const the::unique_list<the_pick_rec_t> & pl):
  the::unique_list<the_pick_rec_t>(pl)
{}

//----------------------------------------------------------------
// the_pick_list_t::pick
//
bool
the_pick_list_t::pick(the_view_t & view,
		      const p2x1_t & scs_pt,
		      const the_pick_filter_t & filter)
{
  clear();

  const the_document_t * doc = view.document();
  if (doc == NULL) return false;

  return pick(view, scs_pt, filter, &(doc->registry()));
}

//----------------------------------------------------------------
// the_pick_list_t::pick
//
bool
the_pick_list_t::pick(the_view_t & view,
		      const p2x1_t & scs_pt,
		      const the_pick_filter_t & filter,
		      const the_registry_t * registry)
{
  clear();

  const the_view_volume_t pick_vol(view.view_mgr().pick_volume(scs_pt));

  for (unsigned int id = 0; id < registry->size(); id++)
  {
    if (filter.allow(registry, id) == false) continue;

    std::list<the_pick_data_t> solution;
    the_primitive_t * prim = registry->elem<the_primitive_t>(id);
    prim->intersect(pick_vol, solution);

    for (std::list<the_pick_data_t>::iterator i = solution.begin();
	 i != solution.end(); ++i)
    {
      const the_pick_data_t & data = *i;
      push_back(the_pick_rec_t(&view, pick_vol, data));
    }
  }

  sort();
  return !empty();
}

//----------------------------------------------------------------
// the_pick_list_t::pick
//
bool
the_pick_list_t::pick(the_view_t & view,
		      const p2x1_t & scs_pt,
		      const the_pick_filter_t & filter,
		      const std::list<the_primitive_t *> & selectable)
{
  clear();

  const the_view_volume_t pick_vol(view.view_mgr().pick_volume(scs_pt));

  for (std::list<the_primitive_t *>::const_iterator iter = selectable.begin();
       iter != selectable.end(); ++iter)
  {
    the_primitive_t * primitive = *iter;
    if (!filter.allow(primitive->registry(), primitive->id())) continue;

    std::list<the_pick_data_t> solution;
    primitive->intersect(pick_vol, solution);
    for (std::list<the_pick_data_t>::iterator i = solution.begin();
	 i != solution.end(); ++i)
    {
      const the_pick_data_t & data = *i;
      push_back(the_pick_rec_t(&view, pick_vol, data));
    }
  }

  sort();
  return !empty();
}

//----------------------------------------------------------------
// the_pick_list_t::set_current_state
//
void
the_pick_list_t::set_current_state(the_registry_t * r,
				   the_primitive_state_t state) const
{
  for (the_pick_list_t::const_iterator i = begin(); i != end(); ++i)
  {
    (*i).set_current_state(r, state);
  }
}

//----------------------------------------------------------------
// the_pick_list_t::remove_current_state
//
void
the_pick_list_t::remove_current_state(the_registry_t * r,
				      the_primitive_state_t state) const
{
  for (the_pick_list_t::const_iterator i = begin(); i != end(); ++i)
  {
    (*i).remove_current_state(r, state);
  }
}

//----------------------------------------------------------------
// the_pick_list_t::clear_current_state
//
void
the_pick_list_t::clear_current_state(the_registry_t * r) const
{
  for (the_pick_list_t::const_iterator i = begin(); i != end(); ++i)
  {
    (*i).clear_current_state(r);
  }
}

//----------------------------------------------------------------
// the_pick_list_t::push_back
//
void
the_pick_list_t::push_back(const the_pick_rec_t & pick)
{
  if (replace(pick, pick)) return;
  the::unique_list<the_pick_rec_t>::push_back(pick);
}

//----------------------------------------------------------------
// the_pick_list_t::contains
//
bool
the_pick_list_t::contains(const unsigned int & id) const
{
  for (the_pick_list_t::const_iterator i = begin(); i != end(); ++i)
  {
    if ((*i).data().id() == id) return true;
  }

  return false;
}


//----------------------------------------------------------------
// setup_graph
//
void
setup_graph(const the_registry_t * registry,
	    const the_pick_list_t & root_picks,
	    the_graph_t & graph)
{
  std::list<unsigned int> roots;
  for (the_pick_list_t::const_iterator i = root_picks.begin();
       i != root_picks.end(); ++i)
  {
    const unsigned int & root_id = (*i).data().id();
    roots.push_back(root_id);
  }

  graph.set_roots(registry, roots);
}
