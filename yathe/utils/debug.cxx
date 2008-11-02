// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : debug.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Helper functions used for debugging.

// local includes:
#include "utils/debug.hxx"

// system includes:
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <iostream>

// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// _FIXME
// 
extern void
_FIXME(const char * note, const char * file, int line)
{
  cerr << "FIXME: " << file << ',' << line << ": " << note << endl;
}

//----------------------------------------------------------------
// _the_failed_assertion
// 
extern void
_the_failed_assertion(const char * expr, const char * file, int line)
{
  cerr << "ABORT: " << file << ',' << line << ": " << expr << endl;
  assert(false);
}

//----------------------------------------------------------------
// put_breakpoint_here
// 
void
put_breakpoint_here()
{
  printf("BREAKPOINT\n");
}
