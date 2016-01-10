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


namespace yae
{

  //----------------------------------------------------------------
  // TLifetime
  //
  struct YAE_API TLifetime
  {
    TLifetime();
    ~TLifetime();

    void start(const char * description);
    void finish();

    static void show(std::ostream & os);
    static void clear();

  private:
    TLifetime(const TLifetime &);
    TLifetime & operator = (const TLifetime &);

    struct Private;
    Private * private_;
  };
}

#ifndef YAE_LIFETIME
# ifndef NDEBUG
#  define YAE_LIFETIME(varname) yae::TLifetime varname
#  define YAE_LIFETIME_START(varname, desc) varname.start(desc)
#  define YAE_LIFETIME_FINISH(varname) varname.finish()
# else
#  define YAE_LIFETIME(varname)
#  define YAE_LIFETIME_START(varname, desc)
#  define YAE_LIFETIME_FINISH(varname)
# endif
#endif

#ifndef YAE_LIFETIME_SHOW
# ifndef NDEBUG
#  define YAE_LIFETIME_SHOW(ostream) yae::TLifetime::show(ostream)
# else
#  define YAE_LIFETIME_SHOW(ostream)
# endif
#endif

#ifndef YAE_LIFETIME_CLEAR
# ifndef NDEBUG
#  define YAE_LIFETIME_CLEAR() yae::TLifetime::clear()
# else
#  define YAE_LIFETIME_CLEAR()
# endif
#endif


#endif // YAE_BENCHMARK_H_
