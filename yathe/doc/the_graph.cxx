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


// File         : the_graph.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Wrapper class for a dependency-sorted list of primitives:

// local includes:
#include "doc/the_graph.hxx"
#include "doc/the_registry.hxx"
#include "doc/the_primitive.hxx"

#ifndef NOUI
#include "sel/the_pick_list.hxx"
#endif // NOUI


//----------------------------------------------------------------
// the_graph_t::the_graph_t
// 
the_graph_t::the_graph_t():
  registry_(NULL)
{}

//----------------------------------------------------------------
// the_graph_t::the_graph_t
// 
the_graph_t::the_graph_t(const the_registry_t * registry):
  registry_(registry)
{
  registry_->graph(*this);
  
  dependency_sort();
}

//----------------------------------------------------------------
// the_graph_t::the_graph_t
// 
the_graph_t::the_graph_t(const the_registry_t * registry,
			 const unsigned int & root_id):
  std::list<unsigned int>(),
  registry_(registry)
{
  push_back(root_id);
  registry->elem(root_id)->dependents(*this);
  
  dependency_sort();
}

//----------------------------------------------------------------
// the_graph_t::the_graph_t
// 
the_graph_t::the_graph_t(const the_registry_t * registry,
			 const std::list<unsigned int> & roots):
  std::list<unsigned int>(),
  registry_(NULL)
{
  set_roots(registry, roots);
}

#ifndef NOUI
//----------------------------------------------------------------
// the_graph_t::the_graph_t
// 
the_graph_t::the_graph_t(const the_registry_t * registry,
			 const the_pick_list_t & root_picks):
  std::list<unsigned int>(),
  registry_(NULL)
{
  std::list<unsigned int> roots;
  for (the_pick_list_t::const_iterator i = root_picks.begin();
       i != root_picks.end(); ++i)
  {
    const unsigned int & root_id = (*i).data().id();
    roots.push_back(root_id);
  }
  
  set_roots(registry, roots);
}
#endif // NOUI

//----------------------------------------------------------------
// the_graph_t::set_graph
// 
void
the_graph_t::set_graph(const the_registry_t * registry,
		       const the::unique_list<unsigned int> & graph)
{
  registry_ = registry;
  
  clear();
  insert(end(), graph.begin(), graph.end());
  
  dependency_sort();
}

//----------------------------------------------------------------
// the_graph_t::set_roots
// 
void
the_graph_t::set_roots(const the_registry_t * registry,
		       const std::list<unsigned int> & roots)
{
  registry_ = registry;
  
  clear();
  for (std::list<unsigned int>::const_iterator i = roots.begin();
       i != roots.end(); ++i)
  {
    const unsigned int & root_id = *i;
    push_back(root_id);
    registry->elem(root_id)->dependents(*this);
  }
  
  dependency_sort();
}

//----------------------------------------------------------------
// the_graph_t::dependency_sort
// 
void
the_graph_t::dependency_sort()
{
  for (std::list<unsigned int>::iterator i = begin(); i != end(); ++i)
  {
    for (std::list<unsigned int>::iterator j = next(i); j != end(); ++j)
    {
      bool swap_ij = registry_->elem(*j)->supports(*i);
      if (!swap_ij) continue;
      
      std::swap(*i, *j);
    }
  }
  
  remove_duplicates();
}

//----------------------------------------------------------------
// the_graph_t::regenerate
// 
bool
the_graph_t::regenerate() const
{
  for (std::list<unsigned int>::const_iterator i = begin(); i != end(); ++i)
  {
    the_primitive_t * prim = registry_->elem(*i);
    if (prim->regenerated()) continue;
    if (!prim->regenerate()) return false;
  }
  
  return true;
}

//----------------------------------------------------------------
// the_graph_t::dump
// 
void
the_graph_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_graph_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "dispatcher_ =" << endl;
  unsigned int index = 0;
  for (std::list<unsigned int>::const_iterator i = begin(); i != end(); ++i)
  {
    strm << INDSTR << "node " << index++ << ". id " << *i << ": ";
    the_primitive_t * prim = registry_->elem(*i);
    prim->dump(strm, INDNXT);
  }
  strm << INDSCP << "}" << endl << endl;
}
