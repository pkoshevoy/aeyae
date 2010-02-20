// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_procedure.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Apr 04 15:32:30 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A document procedure class -- a container for
//                inter related document primitives.

#ifndef THE_PROCEDURE_HXX_
#define THE_PROCEDURE_HXX_

// local includes:
#include "doc/the_primitive.hxx"
#include "utils/the_text.hxx"
#include "utils/the_indentation.hxx"

// forward declarations:
class the_bbox_t;
class the_view_t;


//----------------------------------------------------------------
// the_procedure_t
// 
class the_procedure_t : public the_primitive_t
{
public:
  the_procedure_t(): the_primitive_t() {}
  
  // a procedure may regenerate its dependents directly:
  virtual bool regenerate() = 0;
  
  // display the geometry:
  virtual void draw(const the_view_t & view) const = 0;
  
  // calculate the model bounding box:
  virtual void calc_bbox(const the_view_t & view, the_bbox_t & bbox) const = 0;
  
  // virtual: For debugging, dumps the value
  void dump(ostream & strm, unsigned int indent = 0) const
  {
    strm << INDSCP << "the_procedure_t(" << (void *)this << ")" << endl
	 << INDSCP << "{" << endl;
    the_primitive_t::dump(strm, INDNXT);
    strm << INDSCP << "}" << endl << endl;
  }
};


#endif // THE_PROCEDURE_HXX_
