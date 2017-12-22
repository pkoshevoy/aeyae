// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 13:06:20 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iomanip>
#include <limits>
#include <math.h>
#include <sstream>

// yae includes:
#include "yae/utils/yae_time.h"


namespace yae
{
  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime():
    time_(0),
    base_(1001)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime(int64 time, uint64 base):
    time_(time),
    base_(base)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime(double seconds):
    time_((int64)(1000000.0 * seconds)),
    base_(1000000)
  {}

  //----------------------------------------------------------------
  // TTime::operator +=
  //
  TTime &
  TTime::operator += (const TTime & dt)
  {
    if (base_ == dt.base_)
    {
      time_ += dt.time_;
      return *this;
    }

    return operator += (dt.toSeconds());
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime
  TTime::operator + (const TTime & dt) const
  {
    TTime t(*this);
    t += dt;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime &
  TTime::operator += (double dtSec)
  {
    time_ += int64(dtSec * double(base_));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime
  TTime::operator + (double dtSec) const
  {
    TTime t(*this);
    t += dtSec;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator -=
  //
  TTime &
  TTime::operator -= (const TTime & dt)
  {
    if (base_ == dt.base_)
    {
      time_ -= dt.time_;
      return *this;
    }

    return operator -= (dt.toSeconds());
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime
  TTime::operator - (const TTime & dt) const
  {
    TTime t(*this);
    t -= dt;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime &
  TTime::operator -= (double dtSec)
  {
    time_ -= int64(dtSec * double(base_));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime
  TTime::operator - (double dtSec) const
  {
    TTime t(*this);
    t -= dtSec;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator <
  //
  bool
  TTime::operator < (const TTime & t) const
  {
    if (t.base_ == base_)
    {
      return time_ < t.time_;
    }

    return toSeconds() < t.toSeconds();
  }

  //----------------------------------------------------------------
  // TTime::operator <=
  //
  bool
  TTime::operator <= (const TTime & t) const
  {
    if (t.base_ == base_)
    {
      return time_ <= t.time_;
    }

    double dt = toSeconds() - t.toSeconds();
    return dt <= 0;
  }

  //----------------------------------------------------------------
  // TTime::reset
  //
  void
  TTime::reset(int64 time, uint64 base)
  {
    time_ = time;
    base_ = base;
  }

  //----------------------------------------------------------------
  // TTime::getTime
  //
  int64
  TTime::getTime(uint64 base) const
  {
    if (base_ == base)
    {
      return time_;
    }

    TTime t(0, base);
    t += *this;
    return t.time_;
  }

  //----------------------------------------------------------------
  // to_hhmmss
  //
  static bool
  to_hhmmss(int64 time,
            uint64 base,
            std::string & ts,
            const char * separator,
            bool includeNegativeSign = false)
  {
    bool negative = (time < 0);

    int64 t = negative ? -time : time;
    t /= base;

    int64 seconds = t % 60;
    t /= 60;

    int64 minutes = t % 60;
    int64 hours = t / 60;

    std::ostringstream os;

    if (negative && includeNegativeSign && (seconds || minutes || hours))
    {
      os << '-';
    }

    os << std::setw(2) << std::setfill('0') << (int64)(hours) << separator
       << std::setw(2) << std::setfill('0') << (int)(minutes) << separator
       << std::setw(2) << std::setfill('0') << (int)(seconds);

    ts = std::string(os.str().c_str());

    return negative;
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss
  //
  void
  TTime::to_hhmmss(std::string & ts, const char * separator) const
  {
    yae::to_hhmmss(time_, base_, ts, separator, true);
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_frac
  //
  void
  TTime::to_hhmmss_frac(std::string & ts,
                        unsigned int precision,
                        const char * separator,
                        const char * remainder_separator) const
  {
    bool negative = yae::to_hhmmss(time_, base_, ts, separator);

    uint64 t = negative ? -time_ : time_;
    uint64 remainder = t % base_;
    uint64 frac = (precision * remainder) / base_;

    // count number of digits required for given precision:
    uint64 digits = 0;
    for (unsigned int i = precision - 1; precision && i; i /= 10, digits++) ;

    std::ostringstream os;

    if (negative && (frac || t >= base_))
    {
      os << '-';
    }

    os << ts;

    if (digits)
    {
      os << remainder_separator
         << std::setw(digits) << std::setfill('0') << (int)(frac);
    }

    ts = std::string(os.str().c_str());
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_usec
  //
  void
  TTime::to_hhmmss_usec(std::string & ts,
                        const char * separator,
                        const char * usec_separator) const
  {
    to_hhmmss_frac(ts, 1000000, separator, usec_separator);
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_frame
  //
  void
  TTime::to_hhmmss_frame(std::string & ts,
                         double frameRate,
                         const char * separator,
                         const char * framenum_separator) const
  {
    bool negative = (time_ < 0);

    // round to nearest frame:
    double seconds = toSeconds();

    if (negative)
    {
      seconds = -seconds;
    }

    double fpsWhole = ceil(frameRate);
    seconds = (seconds * fpsWhole + 0.5) / fpsWhole;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double frame = remainder * fpsWhole;
    uint64 frameNo = int(frame);

    TTime tmp(seconds);
    tmp.to_hhmmss(ts, separator);

    std::ostringstream os;

    if (negative && (frameNo || (uint64)tmp.time_ >= tmp.base_))
    {
      os << '-';
    }

    os << ts << framenum_separator
       << std::setw(2) << std::setfill('0') << frameNo;

    ts = std::string(os.str().c_str());
  }


  //----------------------------------------------------------------
  // Timespan::Timespan
  //
  Timespan::Timespan(const TTime & t0, const TTime & t1):
    t0_(t0),
    t1_(t1)
  {}

  //----------------------------------------------------------------
  // Timespan::extend
  //
  double
  Timespan::extend(const Timespan & s, double tolerance)
  {
    if (s.empty())
    {
      return 0.0;
    }

    if (empty())
    {
      t0_ = s.t0_;
      t1_ = s.t1_;
      return 0.0;
    }

    double gap_s_to_this = (t0_ - s.t1_).toSeconds();
    double gap_this_to_s = (s.t0_ - t1_).toSeconds();
    double gap =
      gap_s_to_this > 0.0 ? gap_s_to_this :
      gap_this_to_s > 0.0 ? gap_this_to_s :
      0.0;

    if (gap > tolerance)
    {
      return (gap == gap_s_to_this) ? -gap_s_to_this : gap_this_to_s;
    }

    if (s.t0_ < t0_)
    {
      t0_ = s.t0_;
    }

    if (t1_ < s.t1_)
    {
      t1_ = s.t1_;
    }

    return 0.0;
  }

  //----------------------------------------------------------------
  // Timespan::gap
  //
  // calculate the gap between t and this time interval:
  //
  // returns 0 if t is contained within the interval,
  // returns negative value if t is ahead of the interval,
  // returns positive value if t is behind the interval.
  //
  double
  Timespan::diff(const TTime & t) const
  {
    if (empty())
    {
      YAE_ASSERT(false);
      return std::numeric_limits<double>::max();
    }

    double gap_t_to_this = (t0_ - t).toSeconds();
    double gap_this_to_t = (t - t1_).toSeconds();
    double gap =
      (gap_t_to_this > 0.0) ? -gap_t_to_this :
      (gap_this_to_t > 0.0) ? gap_this_to_t :
      0.0;

    return gap;
  }

  //----------------------------------------------------------------
  // merge
  //
  static void
  merge(std::list<Timespan> & track, Timespan span, double tolerance)
  {
    std::list<Timespan> tmp;

    while (!track.empty())
    {
      const Timespan & s = track.front();

      // combine any overlapping intervals:
      double gap = span.extend(s, tolerance);

      if (gap < 0.0)
      {
        // s is ahead of span:
        tmp.push_back(s);
      }
      else if (gap > 0.0)
      {
        // span is ahead of s:
        break;
      }

      track.pop_front();
    }

    tmp.push_back(span);
    tmp.splice(tmp.end(), track);
    track.splice(track.end(), tmp);
  }

  //----------------------------------------------------------------
  // Timeline::extend
  //
  bool
  Timeline::extend(const std::string & track_id,
                   const Timespan & s,
                   double tolerance,
                   bool enforce_monotonically_increasing_time)
  {
    if (s.empty())
    {
      YAE_ASSERT(false);
      return false;
    }

    std::list<Timespan> & track = track_[track_id];
    if (track.empty())
    {
      track.push_back(s);
      return true;
    }

    Timespan & prev = track.back();
    double gap = prev.diff(s.t0_);
    if (gap == 0.0)
    {
      return prev.extend(s, tolerance);
    }
    else if (gap > tolerance)
    {
      track.push_back(s);
      return true;
    }

    if (enforce_monotonically_increasing_time)
    {
      // time should be monotonically increasing:
      return false;
    }

    // time should be monotonically increasing, this is sub-optimal:
    merge(track, s, tolerance);
    return true;
  }

}
