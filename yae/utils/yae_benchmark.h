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

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_time.h"


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
  // Timesheet
  //
  struct YAE_API Timesheet
  {
    //----------------------------------------------------------------
    // Log
    //
    struct YAE_API Log
    {
      Log(): n_(0) {}

      inline yae::TTime avg_wait() const
      { return TTime((n_ > 1) ? (wait_.time_ / (n_ - 1)) : 0, wait_.base_); }

      inline yae::TTime avg_work() const
      { return TTime(n_ ? (work_.time_ / n_) : 0, work_.base_); }

      yae::TTime last_;
      yae::TTime wait_;
      yae::TTime work_;
      uint64_t n_;
    };

    //----------------------------------------------------------------
    // Probe
    //
    struct YAE_API Probe
    {
      template <typename TWhere, typename TWhat>
      Probe(Timesheet & timesheet,
            const TWhere & where,
            const TWhat & what):
        timesheet_(&timesheet),
        where_(where),
        what_(what),
        when_(yae::TTime::now())
      {}

      template <typename TWhere, typename TWhat>
      Probe(Timesheet * timesheet,
            const TWhere & where,
            const TWhat & what):
        timesheet_(timesheet),
        where_(where),
        what_(what),
        when_(yae::TTime::now())
      {}

      ~Probe()
      {
        if (timesheet_)
        {
          yae::TTime finish = yae::TTime::now();
          Log & log = timesheet_->get(where_, what_);
          log.n_++;
          log.work_ += (finish - when_);
        }
      }

    private:
      // intentionally disabled:
      Probe(const Probe &);
      Probe & operator = (const Probe &);

      Timesheet * timesheet_;
      std::string where_;
      std::string what_;
      yae::TTime when_;
    };

    Timesheet();

    inline Log &
    get(const std::string & where, const std::string & what)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      std::map<std::string, Log> & where_log = where_[where];
      Log & log = where_log[what];
      return log;
    }

    inline Log &
    get(const char * where, const std::string & what)
    { return get(std::string(where), what); }

    void clear();

    std::string to_str() const;

  private:
    mutable boost::mutex mutex_;
    std::map<std::string, std::map<std::string, Log> > where_;
    yae::TTime start_;
  };
}

//----------------------------------------------------------------
// operator <<
//
inline std::ostream &
operator << (std::ostream & oss, const yae::Timesheet & timesheet)
{
    oss << timesheet.to_str();
    return oss;
}


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
