// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


// File         : the_graph_node_ref.cxx
// Author       : Pavel A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2004-2008 University of Utah
// License      : GPLv2
// Description  : A graph node ref object used to establish graph
//                dependencies between graph nodes.

// local includes:
#include "doc/the_graph_node_ref.hxx"
#include "utils/the_indentation.hxx"
#include "io/the_file_io.hxx"


//----------------------------------------------------------------
// the_graph_node_ref_t::the_graph_node_ref_t
// 
the_graph_node_ref_t::the_graph_node_ref_t(const unsigned int & id):
  id_(id)
{}

//----------------------------------------------------------------
// the_graph_node_ref_t::save
// 
bool
the_graph_node_ref_t::save(std::ostream & stream) const
{
  ::save(stream, id_);
  return true;
}

//----------------------------------------------------------------
// the_graph_node_ref_t::load
// 
bool
the_graph_node_ref_t::load(std::istream & stream)
{
  return ::load(stream, id_);
}

//----------------------------------------------------------------
// the_graph_node_ref_t::dump
// 
void
the_graph_node_ref_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_graph_node_ref_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "id_ = " << id_ << endl
       << INDSCP << "}" << endl << endl;
}
