// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Aug  3 18:53:42 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMESHEET_H_
#define YAE_TIMESHEET_H_

// standard:
#include <iostream>
#include <string>

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_time.h"

//----------------------------------------------------------------
// YAE_TIMESHEET_ENABLED
//
#define YAE_TIMESHEET_ENABLED 0


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

      std::list<yae::TTime> too_slow_;
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
            const TWhat & what,
            const TTime & expected_lifetime = TTime(0, 0)):
        timesheet_(&timesheet),
        where_(where),
        what_(what),
        when_(yae::TTime::now()),
        expected_lifetime_(expected_lifetime)
      {}

      template <typename TWhere, typename TWhat>
      Probe(Timesheet * timesheet,
            const TWhere & where,
            const TWhat & what,
            const TTime & expected_lifetime = TTime(0, 0)):
        timesheet_(timesheet),
        where_(where),
        what_(what),
        when_(yae::TTime::now()),
        expected_lifetime_(expected_lifetime)
      {}

      ~Probe();

    private:
      // intentionally disabled:
      Probe(const Probe &);
      Probe & operator = (const Probe &);

      Timesheet * timesheet_;
      std::string where_;
      std::string what_;
      yae::TTime when_;
      yae::TTime expected_lifetime_;
    };

    Timesheet();
    ~Timesheet();

    Log & get(const std::string & where, const std::string & what);

    inline Log &
    get(const char * where, const std::string & what)
    { return get(std::string(where), what); }

    void clear();

    std::string to_str() const;

  private:
    Timesheet(const Timesheet &);
    Timesheet & operator = (const Timesheet &);

    struct Private;
    Private * private_;
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

#if YAE_TIMESHEET_ENABLED
#define YAE_TIMESHEET_PROBE(var_name, timesheet, where, what) \
  const yae::Timesheet::Probe var_name(timesheet, where, what)
#else
#define YAE_TIMESHEET_PROBE(var_name, timesheet, where, what)
#endif

#if YAE_TIMESHEET_ENABLED
#define YAE_TIMESHEET_PROBE_TOO_SLOW(var_name, timesheet, where, what, dur) \
  const yae::Timesheet::Probe var_name(timesheet, where, what, dur)
#else
#define YAE_TIMESHEET_PROBE_TOO_SLOW(var_name, timesheet, where, what, dur)
#endif


#endif // YAE_TIMESHEET_H_
