// File         : the_reference.cxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

// local includes:
#include "doc/the_reference.hxx"
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_reference_t::the_reference_t
// 
the_reference_t::the_reference_t(const unsigned int & id):
  id_(id)
{}

//----------------------------------------------------------------
// the_reference_t::save
// 
bool
the_reference_t::save(std::ostream & stream) const
{
  stream << id_ << endl;
  return true;
}

//----------------------------------------------------------------
// the_reference_t::load
// 
bool
the_reference_t::load(std::istream & stream)
{
  stream >> id_;
  return true;
}

//----------------------------------------------------------------
// the_reference_t::dump
// 
void
the_reference_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_reference_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "id_ = " << id_ << endl
       << INDSCP << "}" << endl << endl;
}
