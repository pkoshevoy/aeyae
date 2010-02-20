// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_primitive.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Implementation of the document primitive object.

// local includes:
#include "doc/the_primitive.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_utils.hxx"

#ifndef NOUI
#include "opengl/the_appearance.hxx"
#endif // NOUI 


//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & strm, the_primitive_state_t state)
{
  switch (state)
  {
    case THE_REGULAR_STATE_E:	return strm << "THE_REGULAR_STATE_E";
    case THE_HILITED_STATE_E:	return strm << "THE_HILITED_STATE_E";
    case THE_SELECTED_STATE_E:	return strm << "THE_SELECTED_STATE_E";
      
    default:
      assert(false);
  }
  
  return strm;
}


//----------------------------------------------------------------
// the_primitive_t::the_primitive_t
// 
the_primitive_t::the_primitive_t():
  the_graph_node_t()
{}

//----------------------------------------------------------------
// the_primitive_t::the_primitive_t
// 
the_primitive_t::the_primitive_t(const the_primitive_t & primitive):
  the_graph_node_t(primitive),
  current_state_(primitive.current_state_)
{}

//----------------------------------------------------------------
// the_primitive_t::~the_primitive_t
// 
the_primitive_t::~the_primitive_t()
{}

//----------------------------------------------------------------
// the_primitive_t::color
// 
the_color_t
the_primitive_t::color() const
{
  return THE_APPEARANCE.palette().point()[current_state()];
}

//----------------------------------------------------------------
// the_primitive_t::dump
// 
void
the_primitive_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_primitive_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_graph_node_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}
