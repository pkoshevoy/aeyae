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
