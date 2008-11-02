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


// File         : the_registry.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The registry -- used to keep track of document primitives.

// local includes:
#include "doc/the_registry.hxx"
#include "doc/the_primitive.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_unique_list.hxx"


//----------------------------------------------------------------
// the_id_dispatcher_t::the_id_dispatcher_t
// 
the_id_dispatcher_t::
the_id_dispatcher_t(const the_id_dispatcher_t & dispatcher):
  reuse_(dispatcher.reuse_),
  id_(dispatcher.id_)
{}

//----------------------------------------------------------------
// the_id_dispatcher_t::dump
// 
void
the_id_dispatcher_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_id_dispatcher_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "id_ = " << id_ << ";" << endl
       << INDSTR << "reuse_ =" << reuse_ << endl
       << INDSCP << "}" << endl << endl;
}


//----------------------------------------------------------------
// the_registry_t::the_registry_t
// 
the_registry_t::the_registry_t():
  table_(512, 512, NULL)
{}

//----------------------------------------------------------------
// the_registry_t::the_registry_t
// 
the_registry_t::the_registry_t(const the_registry_t & registry):
  dispatcher_(registry.dispatcher_),
  table_(registry.table_)
{
  // make it a deep copy:
  const unsigned int size = the_registry_t::size();
  for (unsigned int i = 0; i < size; i++)
  {
    the_primitive_t *& prim = elem(i);
    if (prim == NULL) continue;
    
    prim = prim->clone();
    prim->registry_ = this;
  }
}

//----------------------------------------------------------------
// the_registry_t::~the_registry_t
// 
the_registry_t::~the_registry_t()
{
  clear();
}

//----------------------------------------------------------------
// the_registry_t::clear
// 
void
the_registry_t::clear()
{
  unsigned int size = the_registry_t::size();
  for (unsigned int i = 0; i < size; i++)
  {
    the_primitive_t * prim = table_[size - i - 1];
    if (prim == NULL) continue;
    
    del(prim);
    delete prim;
  }
  
  dispatcher_.reset();
}

//----------------------------------------------------------------
// the_registry_t::add
// 
void
the_registry_t::add(the_primitive_t * prim)
{
  unsigned int id = dispatcher_.dispatch_id();
  table_[id] = prim;
  prim->added_to_the_registry(this, id);
}

//----------------------------------------------------------------
// the_registry_t::del
// 
void
the_registry_t::del(the_primitive_t * prim)
{
  unsigned int id = prim->id();
  prim->removed_from_the_registry();
  dispatcher_.release(id);
  table_[id] = NULL;
}

//----------------------------------------------------------------
// graph_sorter_t
// 
class graph_sorter_t
{
public:
  graph_sorter_t(const the_registry_t * registry):
    registry_(registry)
  {}
  
  bool operator()(const unsigned int & id_a,
		  const unsigned int & id_b) const
  {
    return !registry_->elem(id_b)->supports(id_a);
  }
  
  const the_registry_t * registry_;
};

//----------------------------------------------------------------
// the_registry_t::graph
// 
void
the_registry_t::graph(std::list<unsigned int> & graph) const
{
  graph.clear();
  
  // first populate the graph:
  const unsigned int size = table_.size();
  for (unsigned int i = 0; i < size; i++)
  {
    if (table_[i] == NULL) continue;
    graph.push_back(i);
  }
  
  // sort it:
  graph.sort(graph_sorter_t(this));
}

//----------------------------------------------------------------
// the_registry_t::dump
// 
void
the_registry_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_registry_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "dispatcher_ =" << endl;
  dispatcher_.dump(strm, INDNXT);
  unsigned int size = table_.size();
  for (unsigned int i = 0; i < size; i++)
  {
    the_primitive_t * prim = table_[i];
    if (prim == NULL) continue;
    
    strm << INDSTR << "table_[" << i << "] =" << endl;
    prim->dump(strm, INDNXT);
  }
  strm << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// assert_sanity
// 
#ifndef NDEBUG
static void
assert_sanity(const the_registry_t * registry, const unsigned int & id)
{
  const the_primitive_t * p = registry->elem(id);
  if (p == NULL) return;
  
  // make sure the primitive knows it is in the registry:
  assert(p->id() == id);
  
  // make sure the direct dependents of this primitive are in the
  // registry and know that they are supported by this primitive:
  const the::unique_list<unsigned int> & deps = p->direct_dependents();
  for (std::list<unsigned int>::const_iterator
	 i = deps.begin(); i != deps.end(); ++i)
  {
    const unsigned int & dep_id = *i;
    const the_primitive_t * d = registry->elem(dep_id);
    assert(d != NULL);
    bool ok = d->direct_dependent_of(id);
    assert(ok);
  }
  
  // make sure the direct supporters of this primitive are in the
  // registry and know that they are supporting this primitive:
  const the::unique_list<unsigned int> & sups = p->direct_supporters();
  for (std::list<unsigned int>::const_iterator
	 i = sups.begin(); i != sups.end(); ++i)
  {
    const unsigned int & sup_id = *i;
    const the_primitive_t * s = registry->elem(sup_id);
    assert(s != NULL);
    bool ok = s->direct_supporter_of(id);
    assert(ok);
  }
}
#endif

//----------------------------------------------------------------
// the_registry_t::assert_sanity
// 
void
the_registry_t::assert_sanity() const
{
#ifndef NDEBUG
  dump(cout);
  
  // make sure dispatcher is sane:
  the_primitive_t * p = elem(dispatcher_.id());
  assert(p == NULL);
  const std::list<unsigned int> & reuse = dispatcher_.reuse();
  for (std::list<unsigned int>::const_iterator
	 i = reuse.begin(); i != reuse.end(); ++i)
  {
    const unsigned int & id = *i;
    assert(id != dispatcher_.id());
    
    p = elem(id);
    assert(p == NULL);
    
    // make sure the id is unique in the list:
    for (std::list<unsigned int>::const_iterator
	   j = next(i); j != reuse.end(); ++j)
    {
      assert(*j != id);
    }
  }
  
  // make sure every primitive in the registry knows it is in the registry,
  // and so are all of its dependents and supporters:
  const unsigned int size = table_.size();
  for (unsigned int i = 0; i < size; i++)
  {
    ::assert_sanity(this, i);
  }
#endif
}
