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
// THE_BENCHMARK
// 
#define THE_BENCHMARK(name) \
  the_benchmark_t(name ## " (" ## __FILE__ ## ":" ## __LINE__)


#endif // THE_BENCHMARK_HXX_
