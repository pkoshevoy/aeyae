// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_graph.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Wrapper class for a dependency-sorted list of graph nodes:

// local includes:
#include "doc/the_graph.hxx"
#include "doc/the_registry.hxx"
#include "doc/the_graph_node.hxx"
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_graph_t::the_graph_t
//
the_graph_t::the_graph_t():
  std::list<unsigned int>(),
  registry_(NULL)
{}

//----------------------------------------------------------------
// the_graph_t::the_graph_t
//
the_graph_t::the_graph_t(const the_registry_t * registry):
  std::list<unsigned int>(),
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
  std::list<unsigned int>::push_back(root_id);
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

//----------------------------------------------------------------
// the_graph_t::set_graph
//
void
the_graph_t::set_graph(const the_registry_t * registry,
		       const std::list<unsigned int> & graph)
{
  registry_ = registry;

  clear();
  insert(end(), graph.begin(), graph.end());

  remove_duplicates();
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

  std::list<unsigned int>::clear();
  for (std::list<unsigned int>::const_iterator i = roots.begin();
       i != roots.end(); ++i)
  {
    const unsigned int & root_id = *i;
    std::list<unsigned int>::push_back(root_id);
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
  // FIXME: this is very inefficient:

  for (std::list<unsigned int>::iterator i = begin(); i != end(); ++i)
  {
    for (std::list<unsigned int>::iterator j = the::next(i); j != end(); ++j)
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
  if (!registry_)
  {
    assert(false);
    return false;
  }

  bool all_ok = true;
  std::list<unsigned int>::const_iterator i = std::list<unsigned int>::begin();
  for (; i != std::list<unsigned int>::end(); ++i)
  {
    unsigned int id = *i;
    the_graph_node_t * prim = registry_->elem(id);
    bool ok = the_graph_node_t::regenerate_recursive(prim);
    all_ok = all_ok && ok;
  }

  return all_ok;
}

//----------------------------------------------------------------
// the_graph_t::dump
//
void
the_graph_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_graph_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  unsigned int index = 0;
  std::list<unsigned int>::const_iterator i = std::list<unsigned int>::begin();
  for (; i != std::list<unsigned int>::end(); ++i)
  {
    strm << INDSTR << "root " << index++ << ", ";

    unsigned int id = *i;
    the_graph_node_t * node = registry_->elem(id);
    node->dump(strm, INDNXT);
  }
  strm << INDSCP << "}" << endl << endl;
}
