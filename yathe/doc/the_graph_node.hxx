// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_graph_node.hxx
// Author       : Pavel A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Dependency graph node.

#ifndef THE_GRAPH_NODE_HXX_
#define THE_GRAPH_NODE_HXX_

// system includes:
#include <list>

// local includes:
#include "doc/the_registry.hxx"
#include "utils/the_unique_list.hxx"
#include "utils/the_utils.hxx"
#include "io/the_file_io.hxx"



//----------------------------------------------------------------
// the_graph_node_t
//
// The base class for all model graph_nodes:
class the_graph_node_t
{
  friend class the_registry_t;
  friend bool save(std::ostream & stream, const the_graph_node_t * graph_node);
  friend bool load(std::istream & stream, the_graph_node_t *& graph_node);
  friend bool load(std::istream & stream, the_registry_t & registry);

public:
  the_graph_node_t();
  the_graph_node_t(const the_graph_node_t & graph_node);
  virtual ~the_graph_node_t();

  // every graph_node must be able to make a duplicate of itself:
  virtual the_graph_node_t * clone() const = 0;

  // a human-readable name, should be provided by each instantiable graph_node:
  virtual const char * name() const = 0;

  // The registry will call these functions whenever this graph_node is
  // added to or removed from the registry. These functions should be
  // overridden by the classes that require dependence graph updates:
  virtual void added_to_the_registry(the_registry_t * registry,
				     const unsigned int & id);
  virtual void removed_from_the_registry();

  // Our means of identification:
  inline the_registry_t * registry() const
  { return registry_; }

  inline const unsigned int & id() const
  { return id_; }

  inline the_graph_node_t * graph_node(const unsigned int & id) const
  { return registry_->elem(id); }

  // these can be overridden if necessary:
  virtual void add_supporter(const unsigned int & supporter_id)
  { direct_supporters_.push_back(supporter_id); }

  virtual void del_supporter(const unsigned int & supporter_id)
  { direct_supporters_.remove(supporter_id); }

  virtual void add_dependent(const unsigned int & dependent_id)
  { direct_dependents_.push_back(dependent_id); }

  virtual void del_dependent(const unsigned int & dependent_id)
  { direct_dependents_.remove(dependent_id); }

  // accessors to the supporters/dependents of this graph_node:
  inline const the::unique_list<unsigned int> & direct_dependents() const
  { return direct_dependents_; }

  inline const the::unique_list<unsigned int> & direct_supporters() const
  { return direct_supporters_; }

  // check whether this graph_node is a direct supporter of a given graph_node:
  inline bool direct_supporter_of(const unsigned int & id) const
  { return direct_dependents_.has(id); }

  // check whether this graph_node is a direct dependent of a given graph_node:
  inline bool direct_dependent_of(const unsigned int & id) const
  { return direct_supporters_.has(id); }

  // check whether this graph_node depends on a given graph_node:
  inline bool depends(const unsigned int & id) const
  { return registry_->elem(id)->supports(id_); }

  // check whether this graph_node supports a given graph_node:
  bool supports(const unsigned int & id) const;

  // collect all dependents of this graph_node:
  void dependents(std::list<unsigned int> & dependents) const;

  // collect all supporters of this graph_node:
  void supporters(std::list<unsigned int> & supporters) const;

  // regeneration state of this node:
  typedef enum {
    REGENERATION_SUCCEEDED_E = 0,
    REGENERATION_REQUESTED_E = 1,
    REGENERATION_FAILED_E = 2,
  } regeneration_state_t;

  inline regeneration_state_t regeneration_state() const
  { return regeneration_state_; }

  // check whether this graph_node is up-to-date:
  inline bool regenerated() const
  { return regeneration_state_ == REGENERATION_SUCCEEDED_E; }

  // check whether all supporters of this node are regenerated:
  bool verify_supporters_regenerated() const;

  // check whether all supporters of this node have been visited:
  bool verify_supporters_visited() const;

  // regenerate a given node only, do not regenerate its dependents:
  static bool regenerate(the_graph_node_t * node);

  // request regeneration of a given node and its descendants:
  static void request_regeneration(the_graph_node_t * start_here);

  // regenerate a given node and its dependents:
  static bool regenerate_recursive(the_graph_node_t * start_here);

  // regenerate the supporters of a given node, and the node itself:
  static bool regenerate_supporters(the_graph_node_t * stop_here);

  // NOTE: this function only works if the starting node is a root node:
  // collect graph nodes into a list
  // sorted in order from supporters to dependents:
  static void collect_nodes_sorted(the_graph_node_t * start_here,
				   std::list<unsigned int> & nodes);

private:
  void request_regeneration_helper();
  bool regenerate_recursive_helper();
  bool regenerate_supporters_helper();
  void collect_nodes_sorted_helper(std::list<unsigned int> & sorted);

protected:
  // every graph node should be able to regenerate itself;
  // in general this function should not regenerate dependents of this node,
  // use regenerate_recursive to regenerate a given node and its dependents:
  virtual bool regenerate() = 0;

public:
  // file io:
  virtual bool save(std::ostream & stream) const;
  virtual bool load(std::istream & stream);

  // For debugging, dumps this model graph_node table:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  // the registry to which this graph_node was added:
  the_registry_t * registry_;

  // id of this graph_node (can be used to lookup this object in the registry):
  unsigned int id_;

  // a list of graph_nodes that directly depend on this graph_node:
  the::unique_list<unsigned int> direct_dependents_;

  // a list of graph_nodes that directly support this graph_node:
  the::unique_list<unsigned int> direct_supporters_;

  // this flag will be set by regenerate_recursive:
  regeneration_state_t regeneration_state_;

private:
  // timestamp of the last time this node was visited:
  unsigned int timestamp_visited_;

  // a global regeneration request counter,
  // used to label successfully regenerated graph nodes:
  static unsigned int timestamp_;
};

//----------------------------------------------------------------
// operator <<
//
extern ostream &
operator << (ostream & s, const the_graph_node_t & p);

//----------------------------------------------------------------
// establish_supporter_dependent
//
// supporter/dependent graph management functions:
extern void
establish_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id);

//----------------------------------------------------------------
// breakdown_supporter_dependent
//
extern void
breakdown_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id);


//----------------------------------------------------------------
// nth_dependent
//
// Lookup the n-th direct dependent of type dependent_t for a
// given graph_node.
//
// NOTE: n is zero based, so to find the first occurence specify n = 0.
//
template <class dependent_t>
dependent_t *
nth_dependent(const the_graph_node_t * graph_node, unsigned int n)
{
  assert(graph_node != NULL);

  the_registry_t * registry = graph_node->registry();
  assert(registry != NULL);

  unsigned int index = 0;
  const the::unique_list<unsigned int> & deps = graph_node->direct_dependents();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    dependent_t * dep = registry->template elem<dependent_t>(*iter);
    if (dep != NULL)
    {
      if (n == index)
      {
	return dep;
      }
      else
      {
	index++;
      }
    }
  }

  return NULL;
}

//----------------------------------------------------------------
// first_dependent
//
// Lookup the first direct dependent of type dependent_t for a
// given graph_node:
//
template <class dependent_t>
dependent_t *
first_dependent(const the_graph_node_t * graph_node)
{
  return nth_dependent<dependent_t>(graph_node, 0);
}

//----------------------------------------------------------------
// nth_supporter
//
// Lookup the n-th direct supporter of type supporter_t for a
// given graph_node.
//
// NOTE: n is zero based, so to find the first occurence specify n = 0.
//
template <class supporter_t>
supporter_t *
nth_supporter(const the_graph_node_t * graph_node, unsigned int n)
{
  assert(graph_node != NULL);

  the_registry_t * registry = graph_node->registry();
  assert(registry != NULL);

  unsigned int index = 0;
  const the::unique_list<unsigned int> & sups = graph_node->direct_supporters();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = sups.begin(); iter != sups.end(); ++iter)
  {
    supporter_t * sup = registry->template elem<supporter_t>(*iter);
    if (sup != NULL)
    {
      if (n == index)
      {
	return sup;
      }
      else
      {
	index++;
      }
    }
  }

  return NULL;
}


//----------------------------------------------------------------
// first_supporter
//
// Lookup the first direct supporter of type supporter_t for a
// given graph_node:
//
template <class supporter_t>
supporter_t *
first_supporter(const the_graph_node_t * graph_node)
{
  return nth_supporter<supporter_t>(graph_node, 0);
}

//----------------------------------------------------------------
// direct_dependents
//
// Lookup the direct dependents of type dependent_t for a
// given graph_node, return the number of dependents found:
//
template <class dependent_t>
unsigned int
direct_dependents(const the_graph_node_t * graph_node,
		  std::list<dependent_t *> & dependents)
{
  assert(graph_node != NULL);

  the_registry_t * registry = graph_node->registry();
  assert(registry != NULL);

  unsigned int num_found = 0;

  const the::unique_list<unsigned int> & deps = graph_node->direct_dependents();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    dependent_t * dep = registry->template elem<dependent_t>(*iter);
    if (dep == NULL) continue;

    dependents.push_back(dep);
    num_found++;
  }

  return num_found;
}

//----------------------------------------------------------------
// direct_supporters
//
// Lookup the direct supporters of type supporter_t for a
// given graph_node, return the number of supporters found:
//
template <class supporter_t>
unsigned int
direct_supporters(const the_graph_node_t * graph_node,
		  std::list<supporter_t *> & supporters)
{
  assert(graph_node != NULL);

  the_registry_t * registry = graph_node->registry();
  assert(registry != NULL);

  unsigned int num_found = 0;

  const the::unique_list<unsigned int> & deps = graph_node->direct_supporters();
  the::unique_list<unsigned int>::const_iterator iter;
  for (iter = deps.begin(); iter != deps.end(); ++iter)
  {
    supporter_t * sup = registry->template elem<supporter_t>(*iter);
    if (sup == NULL) continue;

    supporters.push_back(sup);
    num_found++;
  }

  return num_found;
}


#endif // THE_GRAPH_NODE_HXX_
