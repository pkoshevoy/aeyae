// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:58:17 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_BENCHMARK_H_
#define YAE_BENCHMARK_H_

#ifdef __GNUC__
#define YAE_GCC_VERSION (__GNUC__ * 10000           \
                         + __GNUC_MINOR__ * 100     \
                         + __GNUC_PATCHLEVEL__)
#else
#define YAE_GCC_VERSION 0
#endif

// standard:
#include <iostream>
#include <list>
#include <string>
#include <typeinfo>

#ifdef YAE_BACKTRACE_HEADER
#include <cxxabi.h>
#include <execinfo.h>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_assert.h"

//----------------------------------------------------------------
// YAE_ENABLE_BENCHMARK
//
// #define YAE_ENABLE_BENCHMARK

//----------------------------------------------------------------
// YAE_ENABLE_LIFETIME
//
// #define YAE_ENABLE_LIFETIME


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
# ifdef YAE_ENABLE_BENCHMARK
#  define YAE_BENCHMARK(varname, desc) yae::TBenchmark varname(desc)
# else
#  define YAE_BENCHMARK(varname, description)
# endif
#endif

#ifndef YAE_BENCHMARK_SHOW
# ifdef YAE_ENABLE_BENCHMARK
#  define YAE_BENCHMARK_SHOW(ostream) yae::TBenchmark::show(ostream)
# else
#  define YAE_BENCHMARK_SHOW(ostream)
# endif
#endif

#ifndef YAE_BENCHMARK_CLEAR
# ifdef YAE_ENABLE_BENCHMARK
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
# ifdef YAE_ENABLE_LIFETIME
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
# ifdef YAE_ENABLE_LIFETIME
#  define YAE_LIFETIME_SHOW(ostream) yae::TLifetime::show(ostream)
# else
#  define YAE_LIFETIME_SHOW(ostream)
# endif
#endif

#ifndef YAE_LIFETIME_CLEAR
# ifdef YAE_ENABLE_LIFETIME
#  define YAE_LIFETIME_CLEAR() yae::TLifetime::clear()
# else
#  define YAE_LIFETIME_CLEAR()
# endif
#endif


//----------------------------------------------------------------
// YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
//
#if 0 // !defined(NDEBUG) && defined(__GNUC__)
#define YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
#endif

namespace yae
{

#ifdef YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
  //----------------------------------------------------------------
  // TFootprint
  //
  struct YAE_API TFootprint
  {
    TFootprint(const char * name, std::size_t size);
    ~TFootprint();

    const std::string & name() const;

    void capture_backtrace();

    template <typename TData>
    static TFootprint * create()
    {
      int status = 0;
      const std::type_info & ti = typeid(TData);
      char * name = abi::__cxa_demangle(ti.name(), 0, 0, &status);
      TFootprint * footprint = new TFootprint(name, sizeof(TData));
      free(name);
      return footprint;
    }

    static void show(std::ostream & os);
    static void clear();

  private:
    void init(const char * name, std::size_t size);

    TFootprint(const TFootprint &);
    TFootprint & operator = (const TFootprint &);

    struct Private;
    Private * private_;
  };
#endif

}


#endif // YAE_BENCHMARK_H_
