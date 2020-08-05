// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:58:17 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_BENCHMARK_H_
#define YAE_BENCHMARK_H_

// standard:
#ifdef __GNUC__
#include <cxxabi.h>
#include <execinfo.h>
#endif
#include <iostream>
#include <list>
#include <string>
#include <typeinfo>

// aeyae:
#include "yae/api/yae_api.h"


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


//----------------------------------------------------------------
// YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
//
#if 0 // !defined(NDEBUG) && defined(__GNUC__)
#define YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
#endif

namespace yae
{

  //----------------------------------------------------------------
  // StackFrame
  //
  struct YAE_API StackFrame
  {
    std::string module_;
    std::string func_;
    std::string offset_;
    std::string address_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & os, const StackFrame & f);

  //----------------------------------------------------------------
  // demangle
  //
  YAE_API void
  demangle(StackFrame & frame, const char * line);

  //----------------------------------------------------------------
  // capture_backtrace
  //
  YAE_API void
  capture_backtrace(std::list<StackFrame> & backtrace, std::size_t offset = 2);

  //----------------------------------------------------------------
  // dump
  //
  YAE_API std::ostream &
  dump(std::ostream & os, const std::list<StackFrame> & traceback);

  //----------------------------------------------------------------
  // dump_stacktrace
  //
  YAE_API std::ostream &
  dump_stacktrace(std::ostream & os);

  //----------------------------------------------------------------
  // get_stacktrace_str
  //
  YAE_API std::string get_stacktrace_str();


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
