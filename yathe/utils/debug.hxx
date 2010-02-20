// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : debug.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Helper functions used for debugging.

#ifndef DEBUG_HXX_
#define DEBUG_HXX_

// system includes:
#include <iostream>
#include <iomanip>
#include <exception>
#include <new>
#include <stdlib.h>

//----------------------------------------------------------------
// FIXME
// 
#define FIXME( note ) \
_FIXME( note, __FILE__, __LINE__ )

//----------------------------------------------------------------
// _FIXME
// 
extern void
_FIXME(const char * note, const char * file_name, int line_num);


//----------------------------------------------------------------
// the_assertion
// 
#define the_assertion( expr ) ((expr) ? ((void) 0) : \
_the_failed_assertion(#expr, __FILE__, __LINE__ )

//----------------------------------------------------------------
// _the_failed_assertion
// 
extern void
_the_failed_assertion(const char * expr, const char * file_name, int line_num);

// for memory leak debugging:
#ifdef DEBUG_MEMORY
extern void * operator new (std::size_t size) throw(std::bad_alloc);
extern void operator delete (void * ptr) throw();
extern void dump_new_delete_stats();
#endif // DEBUG_MEMORY

//----------------------------------------------------------------
// put_breakpoint_here
// 
extern void put_breakpoint_here();


#endif // DEBUG_HXX_
