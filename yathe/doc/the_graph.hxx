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
// Description  : Wrapper class for a dependency-sorted list of primitives:

#ifndef THE_GRAPH_HXX_
#define THE_GRAPH_HXX_

// system includes:
#include <list>

// local includes:
#include "utils/the_unique_list.hxx"
#include "utils/the_indentation.hxx"

// forward declarations:
class the_registry_t;
class the_pick_list_t;


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
  
#ifndef NOUI
  the_graph_t(const the_registry_t * registry,
	      const the_pick_list_t & roots);
#endif // NOUI
  
  inline void set_context(const the_registry_t * registry)
  {
    registry_ = registry;
    dependency_sort();
  }
  
  void set_graph(const the_registry_t * registry,
		 const the::unique_list<unsigned int> & graph);
  
  void set_roots(const the_registry_t * registry,
		 const std::list<unsigned int> & roots);
  
  void dependency_sort();
  
  // regenerate the primitives according to their pecking order
  // (supporter before dependent) if any of the primitives fail
  // to regenerate return false and don't bother regenerating the
  // remaining primitives:
  bool regenerate() const;
  
  // For debugging, dumps this model primitive id dispatcher:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
private:
  // this will be called after sorting the graph:
  inline void remove_duplicates()
  { if (!empty()) unique(); }
  
  // the registry that owns the primitives stored in this graph:
  const the_registry_t * registry_;
};


#endif // THE_GRAPH_HXX_
