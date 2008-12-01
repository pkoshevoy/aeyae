// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
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
  
  static void reset();
  static void dump(std::ostream & os);
  static void dump(const char * filename_utf8);
  
private:
  void * record_;
};


//----------------------------------------------------------------
// STR1
// 
// macro for converting its parameter to a string literal
// 
#define STR1(a) STR1_HIDDEN(a)
#define STR1_HIDDEN(a) #a

//----------------------------------------------------------------
// CAT2
//
// macro for concatenating 2 parameters together
// 
#define CAT2(a, b) CAT2_HIDDEN(a, b)
#define CAT2_HIDDEN(a, b) a ## b

//----------------------------------------------------------------
// BENCHMARK
// 
#define BENCHMARK(name)							\
  the_benchmark_t CAT2(benchmark_, CAT2(name, CAT2(_, __LINE__)))	\
    (#name ", " __FILE__ ":" STR1(__LINE__))


#endif // THE_BENCHMARK_HXX_
