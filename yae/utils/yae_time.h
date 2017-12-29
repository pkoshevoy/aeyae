// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 13:06:20 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIME_H_
#define YAE_TIME_H_

// system includes:
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>

// yae includes:
#include "yae/api/yae_api.h"


namespace yae
{

  //----------------------------------------------------------------
  // TTime
  //
  struct YAE_API TTime
  {
    TTime();
    TTime(int64 time, uint64 base);
    TTime(double seconds);

    TTime & operator += (const TTime & dt);
    TTime operator + (const TTime & dt) const;

    TTime & operator += (double dtSec);
    TTime operator + (double dtSec) const;

    TTime & operator -= (const TTime & dt);
    TTime operator - (const TTime & dt) const;

    TTime & operator -= (double dtSec);
    TTime operator - (double dtSec) const;

    bool operator < (const TTime & t) const;
    bool operator <= (const TTime & t) const;

    inline bool operator > (const TTime & t) const
    { return t < *this; }

    inline bool operator >= (const TTime & t) const
    { return t <= *this; }

    void reset(int64 time = 0, uint64 base = 1001);

    int64 getTime(uint64 base) const;

    void to_hhmmss(std::string & ts,
                   const char * separator = "") const;

    void to_hhmmss_frac(std::string & ts,
                        unsigned int precision = 100, // centiseconds
                        const char * separator = ":",
                        const char * remainder_separator = ".") const;

    void to_hhmmss_usec(std::string & ts,
                        const char * separator = "",
                        const char * usec_separator = ".") const;

    void to_hhmmss_frame(std::string & ts,
                         double frameRate = 29.97,
                         const char * separator = ":",
                         const char * framenum_separator = ":") const;

    inline std::string to_hhmmss(const char * separator = "") const
    {
      std::string ts;
      to_hhmmss(ts, separator);
      return ts;
    }

    // return timestamp in hhmmss.uuuuuu format
    inline std::string to_hhmmss_frac(unsigned int precision = 100,
                                      const char * separator = "",
                                      const char * frac_separator = ".") const
    {
      std::string ts;
      to_hhmmss_frac(ts, precision, separator, frac_separator);
      return ts;
    }

    inline std::string to_hhmmss_usec(const char * separator = "",
                                      const char * usec_separator = ".") const
    {
      return to_hhmmss_frac(1000000, separator, usec_separator);
    }

    inline std::string to_hhmmss_frame(double frameRate = 29.97,
                                       const char * separator = ":",
                                       const char * fnum_separator = ":") const
    {
      std::string ts;
      to_hhmmss_frame(ts, frameRate, separator, fnum_separator);
      return ts;
    }

    inline double toSeconds() const
    { return double(time_) / double(base_); }

    inline double perSecond() const
    { return double(base_) / double(time_); }

    inline bool operator == (const TTime & t) const
    { return time_ == t.time_ && base_ == t.base_; }

    inline bool operator != (const TTime & t) const
    { return time_ != t.time_ || base_ != t.base_; }

    int64 time_;
    uint64 base_;
  };

  //----------------------------------------------------------------
  // operator
  //
  inline TTime operator - (const TTime & t)
  { return TTime(-t.time_, t.base_); }

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const TTime & t);


  //----------------------------------------------------------------
  // Timespan
  //
  struct YAE_API Timespan
  {
    Timespan(const TTime & t0 = TTime(std::numeric_limits<int64>::max(), 1),
             const TTime & t1 = TTime(std::numeric_limits<int64>::min(), 1));

    Timespan & operator += (const TTime & offset);

    inline Timespan operator + (const TTime & offset) const
    { return Timespan(t0_ + offset, t1_ + offset); }

    inline bool empty() const
    { return t1_ < t0_; }

    inline bool disjoint(const Timespan & s) const
    { return t0_ > s.t1_ || s.t0_ > t1_; }

    inline bool overlaps(const Timespan & s) const
    { return !disjoint(s); }

    inline bool contains(const Timespan & s) const
    { return t0_ <= s.t0_ && s.t1_ <= t1_; }

    // returns non-zero value if interval s
    // and this interval are disjoint beyond given tolerance;
    //
    // returns negative value if s is ahead of this interval,
    // returns positive value if s is behind this interval.
    //
    double extend(const Timespan & s, double tolerance = 0.0);

    // calculate the gap between t and this time interval:
    //
    // returns 0 if t is contained within the interval,
    // returns negative value if t is ahead of the interval,
    // returns positive value if t is behind the interval.
    //
    double diff(const TTime & t) const;

    TTime t0_;
    TTime t1_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const Timespan & s);


  //----------------------------------------------------------------
  // Timeline
  //
  struct YAE_API Timeline
  {
    typedef std::map<std::string, std::list<Timespan> > TTracks;

    void add_keyframe(const std::string & track_id, const TTime & dts);

    // translate this timeline by a given offset:
    Timeline & operator += (const TTime & offset);

    // extend this timeline via a union with
    // a given timeline translated by a given offset:
    void extend(const Timeline & timeline,
                const TTime & offset,
                double tolerace);

    bool extend_track(const std::string & track_id,
                      const Timespan & s,
                      double tolerance,
                      bool fail_on_non_monotonically_increasing_time);

    bool extend(const std::string & track_id,
                const Timespan & s,
                double tolerance = 0.0,
                bool fail_on_non_monotonically_increasing_time = true);

    // calculate the timeline bounding box for a given track:
    Timespan bbox(const std::string & track_id) const;

    // bounding box for all tracks:
    Timespan bbox_;

    // time intervals for individual tracks:
    TTracks tracks_;

    // time stamps of keyframes, per track:
    std::map<std::string, std::set<TTime> > keyframes_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const Timeline & timeline);


  //----------------------------------------------------------------
  // frameDurationForFrameRate
  //
  // pick an "ideal" frame duration and time base to minimize
  // integer roundoff error buildup for a given framer rate:
  //
  YAE_API TTime
  frameDurationForFrameRate(double fps);

  //----------------------------------------------------------------
  // closeEnoughToStandardFrameRate
  //
  YAE_API bool
  closeEnoughToStandardFrameRate(double fps,
                                 double & std_fps,
                                 double tolerance = 1e-3);

  //----------------------------------------------------------------
  // closestStandardFrameRate
  //
  YAE_API double
  closestStandardFrameRate(double fps, double tolerance = 1e-3);


  //----------------------------------------------------------------
  // FramerateEstimator
  //
  struct YAE_API FramerateEstimator
  {
    FramerateEstimator(std::size_t buffer_size = 300);

    FramerateEstimator & operator += (const FramerateEstimator & s);

    void push(const TTime & dts);

    // average fps calculated from a sliding window buffer of DTS:
    double window_avg() const;

    // best guess of the closest standard framerate
    // that could represent the DTS that have occurred so far:
    double best_guess() const;

    struct Framerate
    {
      Framerate();

      // fps calculated from the most frequently occurring frame duration:
      double normal_;

      // fps calculated from the shortest frame duration observed:
      double max_;

      // fps calculated from the longest frame duration observed:
      double min_;

      // average fps:
      double avg_;

      // average fps calculated by excluding occurrences
      // that happen less than 25% of the most frequent occurrence:
      double inlier_;

      // average fps calculated by including occurrences
      // that happen less than 25% of the most frequent occurrence:
      double outlier_;
    };

    // summarize framerate statistics, return best guess of the framerate:
    double get(Framerate & stats) const;

    inline const std::map<TTime, uint64> & durations() const
    { return dur_; }

  protected:
    // a sliding window, for calculating a window average:
    std::list<TTime> dts_;
    std::size_t max_;
    std::size_t num_;

    // keep count of occurrences of various frame durations, msec:
    std::map<TTime, uint64> dur_;

    // keep an accurate sum of frame durations, per frame duration:
    std::map<TTime, TTime> sum_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  YAE_API std::ostream &
  operator << (std::ostream & oss, const FramerateEstimator & estimator);

}


#endif // YAE_TIME_H_
