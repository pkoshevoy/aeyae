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
