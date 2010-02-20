// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_reference.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A reference object used to establish graph dependencies
//                between document primitives.

// local includes:
#include "doc/the_reference.hxx"
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_reference_t::the_reference_t
// 
the_reference_t::the_reference_t(const unsigned int & id):
  the_graph_node_ref_t(id)
{}

//----------------------------------------------------------------
// the_reference_t::dump
// 
void
the_reference_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_reference_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_graph_node_ref_t::dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}
