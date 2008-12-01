// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_benchmark.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 30 17:20:05 MST 2008
// Copyright    : Pavel Koshevoy (C) 2008
// License      : MIT
// Description  : Classed for keeping track of time spent in function calls.

// system includes:
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>

// local includes:
#include "utils/the_benchmark.hxx"

// Boost includes:
#include <boost/thread/tss.hpp>


//----------------------------------------------------------------
// SAVE_TO
// 
static std::string SAVE_TO;


//----------------------------------------------------------------
// the_walltime_t
// 
struct the_walltime_t
{
  the_walltime_t():
    sec_(0),
    usec_(0)
  {}
  
  inline void save()
  {
#if defined(_WIN32) || defined(_WIN64)
    // Windows:
    std::size_t msec = GetTickCount();
    sec_ = msec / 1000;
    usec_ = (msec % 1000) * 1000;
    
#else
    // UNIX:
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sec_ = tv.tv_sec + tv.tv_usec / 1000000;
    usec_ = tv.tv_usec % 1000000;
    
#endif
  }
  
  the_walltime_t & operator -= (const the_walltime_t & ref)
  {
    if (ref.usec_ > usec_)
    {
      usec_ = (usec_ + 1000000) - ref.usec_;
      sec_ -= (ref.sec_ + 1);
    }
    else
    {
      usec_ -= ref.usec_;
      sec_ -= ref.sec_;
    }
    
    return *this;
  }
  
  the_walltime_t & operator += (const the_walltime_t & ref)
  {
    usec_ += ref.usec_;
    sec_ += ref.sec_ + usec_ / 1000000;
    usec_ %= 1000000;
    
    return *this;
  }
  
  unsigned int sec_;
  unsigned int usec_;
};


//----------------------------------------------------------------
// the_benchmark_record_t
// 
class the_benchmark_record_t
{
public:
  the_benchmark_record_t(unsigned int level, const std::string & name):
    level_(level),
    name_(name),
    calls_(0)
  {}
  
  unsigned int level_;
  std::string name_;
  
  unsigned int calls_;
  the_walltime_t total_;
  the_walltime_t start_;
};


//----------------------------------------------------------------
// the_benchmarks_t
// 
class the_benchmarks_t
{
public:
  the_benchmarks_t();
  ~the_benchmarks_t();

  void clear();
  
  the_benchmark_record_t * add(const std::string & name);
  the_benchmark_record_t * lookup(const std::string & name) const;
  
  void dump(std::ostream & so) const;
  void save(const std::string & fn) const;
  
  // data members:
  std::vector<the_benchmark_record_t *> benchmarks_;
  unsigned int level_;
  std::string save_to_;
};


//----------------------------------------------------------------
// the_benchmarks_t::the_benchmarks_t
// 
the_benchmarks_t::the_benchmarks_t():
  level_(0),
  save_to_(SAVE_TO)
{}

//----------------------------------------------------------------
// the_benchmarks_t::~the_benchmarks_t
// 
the_benchmarks_t::~the_benchmarks_t()
{
  if (save_to_.empty())
  {
    dump(std::cout);
  }
  else
  {
    save(save_to_);
  }
  
  clear();
}

//----------------------------------------------------------------
// the_benchmarks_t::clear
// 
void
the_benchmarks_t::clear()
{
  const std::size_t num_benchmarks = benchmarks_.size();
  for (std::size_t i = 0; i < num_benchmarks; i++)
  {
    delete benchmarks_[i];
    benchmarks_[i] = NULL;
  }
  
  benchmarks_.clear();
}

//----------------------------------------------------------------
// the_benchmarks_t::add
// 
the_benchmark_record_t *
the_benchmarks_t::add(const std::string & name)
{
  the_benchmark_record_t * benchmark = new the_benchmark_record_t(level_,
								  name);
  benchmarks_.push_back(benchmark);
  return benchmark;
}

//----------------------------------------------------------------
// the_benchmarks_t::lookup
// 
the_benchmark_record_t *
the_benchmarks_t::lookup(const std::string & name) const
{
  const std::size_t num_benchmarks = benchmarks_.size();
  for (std::size_t i = 0; i < num_benchmarks; i++)
  {
    the_benchmark_record_t * benchmark = benchmarks_[i];
    if (benchmark->level_ == level_ && benchmark->name_ == name)
    {
      return benchmark;
    }
  }
  
  return NULL;
}

//----------------------------------------------------------------
// the_benchmarks_t::dump
// 
void
the_benchmarks_t::dump(std::ostream & so) const
{
  std::ios::fmtflags old_flags = so.setf(std::ios::dec | std::ios::scientific);
  int old_precision = so.precision();
  so.precision(6);
  
  so << "\n------------------------------- "
     << this
     << " -------------------------------\n";
  
  const std::size_t num_benchmarks = benchmarks_.size();
  for (std::size_t i = 0; i < num_benchmarks; i++)
  {
    const the_benchmark_record_t * benchmark = benchmarks_[i];
    double elapsed = (double(benchmark->total_.sec_ +
			     benchmark->total_.usec_ / 1000000) +
		      double(benchmark->total_.usec_ % 1000000) * 1e-6);
    
    so << std::setw(13) << std::fixed
       << elapsed << " ("
       << std::setw(6)
       << benchmark->calls_ << " x "
       << std::setw(10) << std::scientific
       << elapsed / double(benchmark->calls_)
       << ")\t";
    
    // indent for readability:
    for (std::size_t j = 0; j < benchmark->level_; j++)
    {
      so << "  ";
    }
    
    so << benchmark->name_ << std::endl;
  }
  
  so.setf(old_flags);
  so.precision(old_precision);
}

//----------------------------------------------------------------
// the_benchmarks_t::save
// 
void
the_benchmarks_t::save(const std::string & fn) const
{
  std::fstream fo(fn.c_str(), (std::ios::out |
			       std::ios::app |
			       std::ios::binary));
  
  if (fo.is_open())
  {
    dump(fo);
    fo.close();
  }
}


//----------------------------------------------------------------
// TSS
// 
static boost::thread_specific_ptr<the_benchmarks_t> TSS;

//----------------------------------------------------------------
// the_benchmark_t::the_benchmark_t
// 
the_benchmark_t::the_benchmark_t(const char * name_utf8)
{
  the_benchmarks_t * tss = TSS.get();
  if (!tss)
  {
    tss = new the_benchmarks_t();
    TSS.reset(tss);
  }

  // lookup a record by the given name at the current level:
  std::string name(name_utf8);
  the_benchmark_record_t * benchmark = tss->lookup(name);
  if (!benchmark)
  {
    benchmark = tss->add(name);
  }
  
  // save entrance time:
  benchmark->start_.save();
  
  // increment the call counter:
  benchmark->calls_++;
  
  // save benchmark record pointer for quicker lookup on exit:
  record_ = benchmark;
  
  // increment scope level at entrance:
  tss->level_++;
}

//----------------------------------------------------------------
// the_benchmark_t::~the_benchmark_t
// 
the_benchmark_t::~the_benchmark_t()
{
  the_walltime_t finish;
  finish.save();
  
  the_benchmark_record_t * benchmark = (the_benchmark_record_t *)(record_);
  finish -= benchmark->start_;
  benchmark->total_ += finish;
  
  // decrement scope level at exit:
  the_benchmarks_t * tss = TSS.get();
  tss->level_--;
}

//----------------------------------------------------------------
// the_benchmark_t::setup
// 
void
the_benchmark_t::setup(const char * saveto_filename_utf8)
{
  SAVE_TO.assign(saveto_filename_utf8);
  
  the_benchmarks_t * tss = TSS.get();
  if (tss)
  {
    tss->save_to_ = SAVE_TO;
  }
}

//----------------------------------------------------------------
// the_benchmark_t::reset
// 
void
the_benchmark_t::reset()
{
  the_benchmarks_t * tss = TSS.get();
  if (!tss)
  {
    tss->clear();
  }
}
