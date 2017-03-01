// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jun  6 12:58:17 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <cstring>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>

// boost library:
#include <boost/chrono/chrono.hpp>
#include <boost/thread.hpp>

// aeyae:
#include "yae_benchmark.h"


//----------------------------------------------------------------
// indent
//
// FIXME: this probably exists somewhere in the codebase already
//
static std::string
indent(const char * text, std::size_t depth)
{
  static const char * whitespace =
    "                                                                ";
  static const std::size_t whitespace_max = std::strlen(whitespace);

  std::ostringstream os;
  for (std::size_t i = 0, n = depth / whitespace_max; i < n; i++)
  {
    os << whitespace;
  }

  os << &whitespace[whitespace_max - depth % whitespace_max];
  os << text;

  return std::string(os.str().c_str());
}

namespace yae
{

  //----------------------------------------------------------------
  // TBenchmark::Private
  //
  struct TBenchmark::Private
  {
    Private(const char * description);
    ~Private();

    static void show(std::ostream & os);
    static void clear();

    //----------------------------------------------------------------
    // Timesheet
    //
    struct Timesheet
    {
      Timesheet():
        t0_(boost::chrono::steady_clock::now()),
        depth_(0),
        path_("/")
      {}

      //----------------------------------------------------------------
      // Entry
      //
      struct Entry
      {
        Entry(std::size_t depth = 0,
              const std::string & path = std::string("/"),
              const char * name = ""):
          depth_(depth),
          path_(path),
          name_(name),
          n_(0),
          t_(0),
          fresh_(true)
        {
          YAE_ASSERT(*name);
        }

        std::size_t depth_;
        std::string path_;
        std::string name_;
        uint64 n_; // total number of occurrances
        uint64 t_; // total time spent, measured in microseconds
        bool fresh_;
      };

      boost::chrono::steady_clock::time_point t0_;
      std::map<std::string, Entry> entries_;
      std::size_t depth_;
      std::string path_;
    };

  protected:
    static boost::mutex mutex_;

    // thread-specific timesheets
    static std::map<boost::thread::id, Timesheet> tss_;

    std::string key_;
    boost::chrono::steady_clock::time_point t0_;
  };

  //----------------------------------------------------------------
  // TBenchmark::Private::mutex_
  //
  boost::mutex TBenchmark::Private::mutex_;

  //----------------------------------------------------------------
  // TBenchmark::Private::tss_
  //
  std::map<boost::thread::id, TBenchmark::Private::Timesheet>
  TBenchmark::Private::tss_;


  //----------------------------------------------------------------
  // TBenchmark::Private::Private
  //
  TBenchmark::Private::Private(const char * description)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    // lookup the timesheet for the current thread:
    boost::thread::id threadId = boost::this_thread::get_id();
    Timesheet & ts = tss_[threadId];

    // shortcut to timesheet entries:
    std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

    std::string entryPath = ts.path_;
    ts.path_ += '/';
    ts.path_ += description;
    key_ = ts.path_;

    std::map<std::string, Timesheet::Entry>::iterator
      lower_bound = entries.lower_bound(key_);

    if (lower_bound == entries.end() ||
        entries.key_comp()(key_, lower_bound->first))
    {
      // initialize a new measurement
      Timesheet::Entry newEntry(ts.depth_, entryPath, description);
      entries.insert(lower_bound, std::make_pair(key_, newEntry));
    }
    else
    {
      lower_bound->second.fresh_ = true;
    }

    ts.depth_++;

    // save timestamp last, so that the benchmark would
    // not be older than the timesheet that keeps its entry:
    t0_ = boost::chrono::steady_clock::now();
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::~Private
  //
  TBenchmark::Private::~Private()
  {
    boost::chrono::steady_clock::time_point
      t1 = boost::chrono::steady_clock::now();

    uint64 dt =
      boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
      count();

    boost::lock_guard<boost::mutex> lock(mutex_);

    // lookup the timesheet for the current thread:
    boost::thread::id threadId = boost::this_thread::get_id();
    Timesheet & ts = tss_[threadId];

    std::map<std::string, Timesheet::Entry>::iterator
      found = ts.entries_.find(key_);

    if (found == ts.entries_.end())
    {
      // this benchmark was created before timesheet was cleared, ignore it:
      return;
    }

    // lookup this benchmark timesheet entry:
    Timesheet::Entry & entry = found->second;
    YAE_ASSERT(entry.name_.size() && entry.name_[0]);

    entry.n_++;
    entry.t_ += dt;

    if (entry.fresh_)
    {
      YAE_ASSERT(ts.depth_ > 0);
      ts.depth_--;
      ts.path_ = entry.path_;
    }
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::show
  //
  void
  TBenchmark::Private::show(std::ostream & os)
  {
    static const uint64 timebase = 1000000;
    boost::lock_guard<boost::mutex> lock(mutex_);

    std::ostringstream oss;
    oss <<  "\nBenchmark timesheets per thread:\n";

    for (std::map<boost::thread::id, Timesheet>::const_iterator
           j = tss_.begin(); j != tss_.end(); ++j)
    {
      boost::thread::id threadId = j->first;
      oss << "\n Thread " << threadId << " Timesheet:\n";

      // shortcuts:
      const Timesheet & ts = j->second;
      const std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

      for (std::map<std::string, Timesheet::Entry>::const_iterator
             i = entries.begin(); i != entries.end(); ++i)
      {
        const Timesheet::Entry & entry = i->second;
        if (entry.n_ < 1)
        {
          continue;
        }

        oss
          << "  "
          << std::left << std::setw(40) << std::setfill(' ')
          << indent(entry.name_.c_str(), entry.depth_)
          << " : "

          << std::right << std::setw(8) << std::setfill(' ')
          << entry.n_
          << " "

          << std::right << std::setw(5) << std::setfill(' ')
          << (entry.n_ == 1 ? "call" : "calls")
          << ", "

          << std::fixed << std::setprecision(3) << std::setw(13)
          << double(entry.t_ * 1000) / double(timebase)
          << " msec total, "

          << std::fixed << std::setprecision(3) << std::setw(13)
          << double(entry.t_ * 1000000) / double(entry.n_ * timebase)
          << " usec avg\n";
      }
    }

    os << oss.str().c_str() << std::endl;
  }

  //----------------------------------------------------------------
  // TBenchmark::Private::clear
  //
  void
  TBenchmark::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    for (std::map<boost::thread::id, Timesheet>::iterator
           j = tss_.begin(), j1; j != tss_.end(); )
    {
      j1 = j; ++j1;

      // shortcuts:
      Timesheet & ts = j->second;
      std::map<std::string, Timesheet::Entry> & entries = ts.entries_;

      for (std::map<std::string, Timesheet::Entry>::iterator
             i = entries.begin(), i1; i != entries.end(); )
      {
        i1 = i; ++i1;

        Timesheet::Entry & entry = i->second;
        if (entry.n_ > 0)
        {
          entries.erase(i);
        }
        else
        {
          entry.fresh_ = false;
        }

        i = i1;
      }

      if (entries.empty())
      {
        tss_.erase(j);
      }
      else
      {
        ts.depth_ = 0;
        ts.path_ = "/";
        ts.t0_ = boost::chrono::steady_clock::now();
      }

      j = j1;
    }
  }


  //----------------------------------------------------------------
  // TBenchmark::TBenchmark
  //
  TBenchmark::TBenchmark(const char * description):
    private_(new TBenchmark::Private(description))
  {}

  //----------------------------------------------------------------
  // TBenchmark::~TBenchmark
  //
  TBenchmark::~TBenchmark()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TBenchmark::show
  //
  void
  TBenchmark::show(std::ostream & os)
  {
    Private::show(os);
  }

  //----------------------------------------------------------------
  // TBenchmark::clear
  //
  void
  TBenchmark::clear()
  {
    Private::clear();
  }


  //----------------------------------------------------------------
  // TLifetime::Private
  //
  struct TLifetime::Private
  {
    ~Private();

    void start(const char * description);
    void finish();

    static void show(std::ostream & os);
    static void clear();

    //----------------------------------------------------------------
    // Timesheet
    //
    struct Timesheet
    {
      //----------------------------------------------------------------
      // Entry
      //
      struct Entry
      {
        Entry():
          n_(0),
          t_(0)
        {}

        uint64 n_; // total number of occurrances
        uint64 t_; // total time spent, measured in microseconds
      };

      Timesheet():
        t0_(boost::chrono::steady_clock::now())
      {}

      boost::chrono::steady_clock::time_point t0_;
      std::map<std::string, Entry> entries_;
    };

  protected:
    static boost::mutex mutex_;

    // since lifetime may start on one thread and end on another
    // all lifetimes are tracked on one timesheet:
    static Timesheet tss_;

    std::string key_;
    boost::chrono::steady_clock::time_point t0_;
  };

  //----------------------------------------------------------------
  // TLifetime::Private::mutex_
  //
  boost::mutex TLifetime::Private::mutex_;

  //----------------------------------------------------------------
  // TLifetime::Private::tss_
  //
  TLifetime::Private::Timesheet TLifetime::Private::tss_;


  //----------------------------------------------------------------
  // TLifetime::Private::~Private
  //
  TLifetime::Private::~Private()
  {
    finish();
  }

  //----------------------------------------------------------------
  // TLifetime::Private::start
  //
  void
  TLifetime::Private::start(const char * description)
  {
    YAE_ASSERT(description && *description);

    t0_ = boost::chrono::steady_clock::now();
    key_.assign(description);
  }

  //----------------------------------------------------------------
  // TLifetime::Private::finish
  //
  void
  TLifetime::Private::finish()
  {
    YAE_ASSERT(key_.size() && key_[0]);

    boost::chrono::steady_clock::time_point
      t1 = boost::chrono::steady_clock::now();

    uint64 dt =
      boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
      count();

    // lookup/create timesheet entry for this lifetime:
    boost::lock_guard<boost::mutex> lock(mutex_);
    Timesheet::Entry & entry = tss_.entries_[key_];
    entry.n_++;
    entry.t_ += dt;
  }

  //----------------------------------------------------------------
  // TLifetime::Private::show
  //
  void
  TLifetime::Private::show(std::ostream & os)
  {
    static const uint64 timebase = 1000000;

    boost::lock_guard<boost::mutex> lock(mutex_);

    boost::chrono::steady_clock::time_point
      t_now = boost::chrono::steady_clock::now();

    uint64 dt_usec =
      boost::chrono::duration_cast<boost::chrono::microseconds>
      (t_now - tss_.t0_).count();

    std::ostringstream oss;
    oss <<  "\nLifetime timesheet over the last "
        << double(dt_usec) * 1e-6 << " seconds:\n";

    for (std::map<std::string, Timesheet::Entry>::const_iterator
           i = tss_.entries_.begin(); i != tss_.entries_.end(); ++i)
    {
      const std::string & key = i->first;
      const Timesheet::Entry & entry = i->second;

      oss
        << "  "
        << std::left << std::setw(40) << std::setfill(' ')
        << key.c_str()
        << " : "

        << std::right << std::setw(8) << std::setfill(' ')
        << entry.n_
        << ", "

        << std::fixed << std::setprecision(3) << std::setw(13)
        << double(entry.t_ * 1000) / double(timebase)
        << " msec total, "

        << std::fixed << std::setprecision(3) << std::setw(13)
        << double(entry.t_ * 1000000) / double(entry.n_ * timebase)
        << " usec avg\n";
    }

    os << oss.str().c_str() << std::endl;
  }

  //----------------------------------------------------------------
  // TLifetime::Private::clear
  //
  void
  TLifetime::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    tss_.t0_ = boost::chrono::steady_clock::now();
    tss_.entries_.clear();
  }


  //----------------------------------------------------------------
  // TLifetime::TLifetime
  //
  TLifetime::TLifetime():
    private_(new TLifetime::Private())
  {}

  //----------------------------------------------------------------
  // TLifetime::~TLifetime
  //
  TLifetime::~TLifetime()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TLifetime::start
  //
  void
  TLifetime::start(const char * description)
  {
    if (private_)
    {
      private_->start(description);
    }
  }

  //----------------------------------------------------------------
  // TLifetime::finish
  //
  void
  TLifetime::finish()
  {
    delete private_;
    private_ = NULL;
  }

  //----------------------------------------------------------------
  // TLifetime::show
  //
  void
  TLifetime::show(std::ostream & os)
  {
    Private::show(os);
  }

  //----------------------------------------------------------------
  // TLifetime::clear
  //
  void
  TLifetime::clear()
  {
    Private::clear();
  }

}
