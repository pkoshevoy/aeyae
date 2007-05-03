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


// File         : the_pick_filter.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Basic selection filters for document primitives.

#ifndef THE_PICK_FILTER_HXX_
#define THE_PICK_FILTER_HXX_

// local includes:
#include "doc/the_registry.hxx"


//----------------------------------------------------------------
// the_pick_filter_t
//
class the_pick_filter_t
{
public:
  virtual ~the_pick_filter_t() {}
  
  virtual bool allow(const the_registry_t * registry,
		     const unsigned int & id) const
  { return registry->elem(id) != NULL; }
};

//----------------------------------------------------------------
// the_pick_filter_t<prim_t>
// 
template <class prim_t>
class the_pick_filter : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry,
	     const unsigned int & id) const
  { return dynamic_cast<prim_t *>(registry->elem(id)) != NULL; }
};


#endif // THE_PICK_FILTER_HXX_
