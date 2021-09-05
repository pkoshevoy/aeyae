// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Aug  3 18:59:07 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <sstream>

// boost library:
#include <boost/thread.hpp>

// aeyae:
#include "../api/yae_log.h"
#include "yae_timesheet.h"


namespace yae
{

  //----------------------------------------------------------------
  // Timesheet::Private
  //
  struct Timesheet::Private
  {
    Private():
      start_(yae::TTime::now())
    {}

    Timesheet::Log &
    get(const std::string & where, const std::string & what);

    void clear();

    std::string to_str() const;

    mutable boost::mutex mutex_;
    std::map<std::string, std::map<std::string, Timesheet::Log> > where_;
    yae::TTime start_;
  };

  //----------------------------------------------------------------
  // Timesheet::Private::get
  //
  Timesheet::Log &
  Timesheet::Private::get(const std::string & where, const std::string & what)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    std::map<std::string, Timesheet::Log> & where_log = where_[where];
    Timesheet::Log & log = where_log[what];
    return log;
  }

  //----------------------------------------------------------------
  // Timesheet::Private::clear
  //
  void
  Timesheet::Private::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    where_.clear();
    start_ = yae::TTime::now();
  }

  //----------------------------------------------------------------
  // Timesheet::Private::to_str
  //
  std::string
  Timesheet::Private::to_str() const
  {
    std::ostringstream oss;

    boost::lock_guard<boost::mutex> lock(mutex_);
    yae::TTime elapsed = yae::TTime::now() - start_;
    oss << "timesheet log, elapsed time: " << elapsed.to_hhmmss_ms() << '\n';

    for (std::map<std::string, std::map<std::string, Timesheet::Log> >::
           const_iterator i = where_.begin(); i != where_.end(); ++i)
    {
      const std::string & where = i->first;
      const std::map<std::string, Timesheet::Log> & where_log = i->second;

      oss << "  " << where << '\n';
      for (std::map<std::string, Timesheet::Log>::const_iterator
             j = where_log.begin(); j != where_log.end(); ++j)
      {
        const std::string & what = j->first;
        const Timesheet::Log & log = j->second;

        oss << "    " << std::right << std::setw(30) << what
            << ", count: " << std::setw(16) << std::left
            << log.n_
            << " avg work: " << std::setw(8) << std::left
            << log.avg_work().sec_msec()
            << " sum work: " << std::setw(10) << std::left
            << log.work_.sec_msec();

        double sec_work = log.work_.sec();
        double fps_work = (double(log.n_) + 1e-6) / (sec_work + 1e-6);

        if (log.wait_.time_)
        {
          oss << " fps: " << std::setw(10) << std::left << fps_work;

          double sec_wait_and_work = (log.work_ + log.wait_).sec();
          double fps_wait_and_work = ((double(log.n_) + 1e-6) /
                                      (sec_wait_and_work + 1e-6));

          oss << " avg wait: " << std::setw(8) << std::left
              << log.avg_wait().sec_msec()
              << " sum wait: " << std::setw(10) << std::left
              << log.wait_.sec_msec()
              << " fps: "
              << fps_wait_and_work;
        }
        else
        {
          oss << " fps: " << fps_work;
        }

        if (!log.too_slow_.empty())
        {
          const char * separator = " ";
          oss << "; too slow, ms:";
          for (std::list<yae::TTime>::const_iterator
                 k = log.too_slow_.begin(); k != log.too_slow_.end(); ++k)
          {
            double s = k->sec();
            oss << separator << int(s * 1000 + 0.5);
            separator = ", ";
          }
        }
        oss << '\n';
      }
      oss << '\n';
    }
    oss << '\n';
    return std::string(oss.str().c_str());
  }

  //----------------------------------------------------------------
  // Timesheet::Probe::~Probe
  //
  Timesheet::Probe::~Probe()
  {
    if (timesheet_)
    {
      yae::TTime finish = yae::TTime::now();
      yae::TTime elapsed = finish - when_;
      Log & log = timesheet_->get(where_, what_);
      log.n_++;
      log.work_ += elapsed;

      if (expected_lifetime_.valid())
      {
        yae::TTime expected_finish = when_ + expected_lifetime_;
        if (expected_finish < finish)
        {
          log.too_slow_.push_back(elapsed);
          yae_wlog("TOO SLOW: %s %s: %i msec",
                   where_.c_str(),
                   what_.c_str(),
                   int(elapsed.sec() * 1000 + 0.5));
        }
      }
    }
  }

  //----------------------------------------------------------------
  // Timesheet::Timesheet
  //
  Timesheet::Timesheet():
    private_(new Timesheet::Private())
  {}

  //----------------------------------------------------------------
  // Timesheet::~Timesheet
  //
  Timesheet::~Timesheet()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // Timesheet::get
  //
  Timesheet::Log &
  Timesheet::get(const std::string & where, const std::string & what)
  {
    return private_->get(where, what);
  }

  //----------------------------------------------------------------
  // Timesheet::clear
  //
  void
  Timesheet::clear()
  {
    private_->clear();
  }

  //----------------------------------------------------------------
  // Timesheet::to_str
  //
  std::string
  Timesheet::to_str() const
  {
    return private_->to_str();
  }

}
