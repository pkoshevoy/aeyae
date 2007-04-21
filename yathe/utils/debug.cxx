// File         : debug.cxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

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
