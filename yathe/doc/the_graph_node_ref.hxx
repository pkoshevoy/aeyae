// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_graph_node_ref.hxx
// Author       : Pavel A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A graph node ref object is used to establish graph
//                dependencies between graph nodes.

#ifndef THE_GRAPH_NODE_REF_HXX_
#define THE_GRAPH_NODE_REF_HXX_

// local includes:
#include "doc/the_registry.hxx"
#include "math/v3x1p3x1.hxx"


//----------------------------------------------------------------
// the_graph_node_ref_t
//
// Base class for the graph_node_ref types:
//
class the_graph_node_ref_t
{
public:
  the_graph_node_ref_t(const unsigned int & id);
  virtual ~the_graph_node_ref_t() {}

  // a method for cloning graph_node_refs (potential memory leak):
  virtual the_graph_node_ref_t * clone() const = 0;

  // a human-readable name, should be provided by each instantiable primitive:
  virtual const char * name() const = 0;

  // accessor to the graph_node_ref primitive:
  template <class T>
  T * references(the_registry_t * registry) const
  { return registry->template elem<T>(id_); }

  // equality test:
  virtual bool equal(const the_graph_node_ref_t * ref) const
  { return id_ == ref->id_; }

  // equality test operator:
  inline bool operator == (const the_graph_node_ref_t & ref) const
  { return equal(&ref); }

  // graph_node_ref model primitive id accessor:
  inline const unsigned int & id() const
  { return id_; }

  // file io:
  virtual bool save(std::ostream & stream) const;
  virtual bool load(std::istream & stream);

  // For debugging, dumps the id:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  the_graph_node_ref_t();

  // the primitive that is being graph_node_refd:
  unsigned int id_;
};


#endif // THE_GRAPH_NODE_REF_HXX_
