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


// File         : the_log.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Mar 23 11:04:53 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A text log object -- behaves almost like a std::ostream.

// local includes:
#include "the_log.hxx"


//----------------------------------------------------------------
// null_log
// 
the_null_log_t *
null_log()
{
  static the_null_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_null_log_t;
  }
  
  return log;
}


//----------------------------------------------------------------
// cerr_log
// 
the_stream_log_t *
cerr_log()
{
  static the_stream_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_stream_log_t(std::cerr);
  }
  
  return log;
}
