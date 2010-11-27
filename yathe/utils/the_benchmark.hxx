// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_benchmark.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 30 17:20:05 MST 2008
// Copyright    : Pavel Koshevoy (C) 2008
// License      : MIT
// Description  : A class for keeping track of time spent in function calls.

#ifndef THE_BENCHMARK_HXX_
#define THE_BENCHMARK_HXX_

// system includes:
#include <iostream>


//----------------------------------------------------------------
// the_benchmark_t
// 
class the_benchmark_t
{
public:
  the_benchmark_t(const char * name_utf8);
  ~the_benchmark_t();
  
  static void setup(const char * saveto_filename_utf8);
  static void reset();
  
  struct record_t;
  
private:
  record_t * record_;
};


//----------------------------------------------------------------
// THE_STR1
// 
// macro for converting its parameter to a string literal
// 
#define THE_STR1(a) THE_STR1_HIDDEN(a)
#define THE_STR1_HIDDEN(a) #a

//----------------------------------------------------------------
// THE_CAT2
//
// macro for concatenating 2 parameters together
// 
#define THE_CAT2(a, b) THE_CAT2_HIDDEN(a, b)
#define THE_CAT2_HIDDEN(a, b) a ## b

//----------------------------------------------------------------
// THE_BENCHMARK
// 
#define THE_BENCHMARK(name)				\
  the_benchmark_t THE_CAT2(benchmark_, __LINE__)	\
    (#name ", " __FILE__ ":" THE_STR1(__LINE__))


#endif // THE_BENCHMARK_HXX_
