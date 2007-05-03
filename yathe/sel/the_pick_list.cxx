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


// File         : the_pick_list.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A container for user selected document primitives.

// local includes:
#include "sel/the_pick_list.hxx"
#include "doc/the_document.hxx"
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
    registry->elem(id)->intersect(pick_vol, solution);
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
