// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
