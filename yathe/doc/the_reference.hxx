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
