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


// File         : the_graph.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Wrapper class for a dependency-sorted list of root graph nodes

#ifndef THE_GRAPH_HXX_
#define THE_GRAPH_HXX_

// system includes:
#include <list>
#include <iostream>

// local includes:
#include "utils/the_unique_list.hxx"

// forward declarations:
class the_registry_t;


//----------------------------------------------------------------
// the_graph_t
// 
class the_graph_t : public std::list<unsigned int>
{
public:
  the_graph_t();
  
  the_graph_t(const the_registry_t * registry);
  
  the_graph_t(const the_registry_t * registry,
	      const unsigned int & root);
  
  the_graph_t(const the_registry_t * registry,
	      const std::list<unsigned int> & roots);
  
  void set_graph(const the_registry_t * registry,
		 const std::list<unsigned int> & graph);
  
  // set root nodes to regenerate:
  void set_roots(const the_registry_t * registry,
		 const std::list<unsigned int> & roots);
  
  void dependency_sort();
  
  // regenerate the graph nodes according to their pecking order
  // (supporter before dependent) if any of the graph nodes fail
  // to regenerate return false, but not before completing
  // regeneration of the remaining graph nodes:
  bool regenerate() const;
  
  // For debugging, dumps this model graph node id dispatcher:
  void dump(std::ostream & strm, unsigned int indent = 0) const;
  
private:
  // this will be called after sorting the graph:
  inline void remove_duplicates()
  { if (!empty()) unique(); }
  
  // the registry that owns the graph nodes stored in this graph:
  const the_registry_t * registry_;
};


#endif // THE_GRAPH_HXX_
