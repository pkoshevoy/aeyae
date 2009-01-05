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


// File         : the_pick_list.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A container for user selected document primitives.

#ifndef THE_PICK_LIST_HXX_
#define THE_PICK_LIST_HXX_

// local includes:
#include "sel/the_pick_rec.hxx"
#include "sel/the_pick_filter.hxx"
#include "utils/the_unique_list.hxx"

// system includes:
#include <assert.h>

// forward declarations:
class the_graph_t;


//----------------------------------------------------------------
// the_pick_list_t
// 
class the_pick_list_t : public the::unique_list<the_pick_rec_t>
{
public:
  the_pick_list_t();
  the_pick_list_t(const the::unique_list<the_pick_rec_t> & pl);
  
  // go through the document associated with the view and pick all objects
  // that fall under the mouse (plus/minus 2 pixels):
  bool pick(the_view_t & view,
	    const p2x1_t & scs_pt,
	    const the_pick_filter_t & filter);
  
  bool pick(the_view_t & view,
	    const p2x1_t & scs_pt,
	    const the_pick_filter_t & filter,
	    const the_registry_t * registry);
  
  inline bool pick(the_view_t & view, const p2x1_t & scs_pt)
  { return pick(view, scs_pt, default_filter_); }
  
  // go through the given list of primitives and pick all objects that
  // fall under the mouse (plus/minus a small margin):
  bool pick(the_view_t & view,
	    const p2x1_t & scs_pt,
	    const the_pick_filter_t & filter,
	    const std::list<the_primitive_t *> & selectable);
  
  // selection appearance controls:
  void set_current_state(the_registry_t * r, the_primitive_state_t s) const;
  void remove_current_state(the_registry_t * r, the_primitive_state_t s) const;
  void clear_current_state(the_registry_t * r) const;
  
  // virtual:
  void push_back(const the_pick_rec_t & pick);
  
  // check whether this list has a pick record with a matching id:
  bool contains(const unsigned int & id) const;
  
  // an instance of the default filter:
  static const the_pick_filter_t default_filter_;
};


//----------------------------------------------------------------
// setup_graph
// 
extern void
setup_graph(const the_registry_t * registry,
	    const the_pick_list_t & roots,
	    the_graph_t & graph);
  


#endif // THE_PICK_LIST_HXX_
