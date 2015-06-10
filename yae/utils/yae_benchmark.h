// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:58:17 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_BENCHMARK_H_
#define YAE_BENCHMARK_H_

// aeyae:
#include "yae_log.h"
#include "../api/yae_api.h"

// standard C++ library:
#include <iostream>


namespace yae
{

  //----------------------------------------------------------------
  // TBenchmark
  //
  struct YAE_API TBenchmark
  {
    TBenchmark(const char * description);
    ~TBenchmark();

    static void show(std::ostream & os);
    static void clear();

  private:
    TBenchmark(const TBenchmark &);
    TBenchmark & operator = (const TBenchmark &);

    struct Private;
    Private * private_;
  };
}


#ifndef YAE_BENCHMARK
# ifndef NDEBUG
#  define YAE_BENCHMARK(varname, desc) yae::TBenchmark varname(desc)
# else
#  define YAE_BENCHMARK(varname, description)
# endif
#endif

#ifndef YAE_BENCHMARK_SHOW
# ifndef NDEBUG
#  define YAE_BENCHMARK_SHOW(ostream) yae::TBenchmark::show(ostream)
# else
#  define YAE_BENCHMARK_SHOW(ostream)
# endif
#endif

#ifndef YAE_BENCHMARK_CLEAR
# ifndef NDEBUG
#  define YAE_BENCHMARK_CLEAR() yae::TBenchmark::clear()
# else
#  define YAE_BENCHMARK_CLEAR()
# endif
#endif


#endif // YAE_BENCHMARK_H_
