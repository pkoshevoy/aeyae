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
