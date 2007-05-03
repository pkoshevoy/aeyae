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


// File         : the_dlink.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Jul  5 11:40:00 MDT 2005
// Copyright    : (C) 2005
// License      : MIT
// Description  : Single element container with pointers to preceeding and
//                successive container, to be used in a double-linked list.

#ifndef THE_DLINK_HXX_
#define THE_DLINK_HXX_


//----------------------------------------------------------------
// the_dlink_t
// 
// the_dlink_t is a helper class used to implement a linked list,
// a stack, and a loop of objects:
template<class T>
class the_dlink_t
{
public:
  // constructor (an element is required, the next link pointer is optional):
  the_dlink_t(const T & ref_to_elem,
	      the_dlink_t * ptr_to_prev = 0,
	      the_dlink_t * ptr_to_next = 0):
    elem(ref_to_elem),
    prev(ptr_to_prev),
    next(ptr_to_next)
  {}
  
  // the element contained in this link:
  T elem;
  
  // pointers to the previous/next link:
  the_dlink_t<T> * prev;
  the_dlink_t<T> * next;
  
private:
  // disable the default constructor:
  the_dlink_t();
};


#endif // THE_DLINK_HXX_
