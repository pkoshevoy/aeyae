// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_reference.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A reference object used to establish graph dependencies
//                between document primitives.

#ifndef THE_REFERENCE_HXX_
#define THE_REFERENCE_HXX_

// local includes:
#include "doc/the_graph_node_ref.hxx"
#include "doc/the_registry.hxx"
#include "math/v3x1p3x1.hxx"
#include "opengl/the_point_symbols.hxx"

// forward declarations:
class the_view_mgr_t;


//----------------------------------------------------------------
// the_reference_t
//
// Base class for the reference types:
//
class the_reference_t : public the_graph_node_ref_t
{
public:
  the_reference_t(const unsigned int & id);

  // a method for cloning references (potential memory leak):
  virtual the_reference_t * clone() const = 0;

  // a human-readable name, should be provided by each instantiable primitive:
  virtual const char * name() const = 0;

  // calculate the 3D value of this reference:
  virtual bool eval(the_registry_t * registry, p3x1_t & wcs_pt) const = 0;

  // if possible, adjust this reference:
  virtual bool move(the_registry_t * registry,
		    const the_view_mgr_t & view_mgr,
		    const p3x1_t & wcs_pt) = 0;

  // symbol id used to display this reference:
  virtual the_point_symbol_id_t symbol() const = 0;

  // For debugging, dumps the id:
  virtual void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  the_reference_t();
};


#endif // THE_REFERENCE_HXX_
