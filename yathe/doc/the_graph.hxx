// File         : the_graph.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
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
