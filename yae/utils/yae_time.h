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
#include <vector>

// yae includes:
#include "yae/api/yae_api.h"


namespace yae
{

  //----------------------------------------------------------------
  // TTime
  //
  struct YAE_API TTime
  {

    //----------------------------------------------------------------
    // Flicks
    //
    // "universal" timebase, 705600000 ticks per second:
    //
    enum { Flicks = 705600000 };

    static TTime MaxFlicks;
    static TTime MinFlicks;

    TTime();
    TTime(int64 time, uint64 base);
    TTime(double seconds);

    // return time expressed in a given time base:
    TTime rebased(uint64 base) const;

    // NOTE: these round to seconds and drop sub-seconds:
    TTime ceil() const;
    TTime floor() const;
    TTime round() const;

    TTime & operator += (const TTime & dt);
    TTime operator + (const TTime & dt) const;

    TTime & operator -= (const TTime & dt);
    TTime operator - (const TTime & dt) const;

    bool operator < (const TTime & t) const;
    bool operator <= (const TTime & t) const;
    bool operator == (const TTime & t) const;

    inline bool operator != (const TTime & t) const
    { return !(this->operator==(t)); }

    inline bool operator > (const TTime & t) const
    { return t < *this; }

    inline bool operator >= (const TTime & t) const
    { return t <= *this; }

    void reset(int64 time = 0, uint64 base = 1001);

    int64 get(uint64 base) const;

    inline double sec() const
    {
      YAE_ASSERT(base_);
      return double(time_) / double(base_);
    }

    inline double hz() const
    {
      YAE_ASSERT(time_);
      return time_ ? double(base_) / double(time_) : 0.0;
    }

    void to_hhmmss(std::string & ts,
                   const char * separator = ":") const;

    void to_hhmmss_frac(std::string & ts,
                        unsigned int precision = 100, // centiseconds
                        const char * separator = ":",
                        const char * remainder_separator = ".") const;

    void to_hhmmss_frame(std::string & ts,
                         double frame_rate = 29.97,
                         const char * mm_separator = ":",
                         const char * ff_separator = ":") const;

    // return timestamp in hhmmss format
    inline std::string to_hhmmss(const char * mm_separator = ":") const
    {
      std::string ts;
      to_hhmmss(ts, mm_separator);
      return ts;
    }

    // return timestamp in hh:mm:ss:ff format
    inline std::string to_hhmmss_ff(double frame_rate = 29.97,
                                    const char * mm_separator = ":",
                                    const char * ff_separator = ":") const
    {
      std::string ts;
      to_hhmmss_frame(ts, frame_rate, mm_separator, ff_separator);
      return ts;
    }

    // return timestamp in hhmmss.xxxxxxxx format
    inline std::string to_hhmmss_frac(unsigned int precision = 100,
                                      const char * mm_separator = ":",
                                      const char * xx_separator = ".") const
    {
      std::string ts;
      to_hhmmss_frac(ts, precision, mm_separator, xx_separator);
      return ts;
    }

    // return timestamp in hhmmss.mmm format
    inline std::string to_hhmmss_ms(const char * mm_separator = ":",
                                    const char * ms_separator = ".") const
    {
      return to_hhmmss_frac(1000, mm_separator, ms_separator);
    }

    // return timestamp in hhmmss.uuuuuu format
    inline std::string to_hhmmss_us(const char * mm_separator = ":",
                                    const char * us_separator = ".") const
    {
      return to_hhmmss_frac(1000000, mm_separator, us_separator);
    }

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
    Timespan(const TTime & t0 = TTime::MaxFlicks,
             const TTime & t1 = TTime::MinFlicks);

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

    inline double duration_sec() const
    { return empty() ? 0.0 : (t1_ - t0_).sec(); }

    inline void
    reset(const TTime & t0 = TTime::MaxFlicks,
          const TTime & t1 = TTime::MinFlicks)
    {
      t0_ = t0;
      t1_ = t1;
    }

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
  // merge
  //
  YAE_API void
  merge(std::list<Timespan> & track, Timespan span, double tolerance = 0.0);

  //----------------------------------------------------------------
  // extend
  //
  YAE_API bool
  extend(std::list<Timespan> & track,
         const Timespan & s,
         double tolerance = 0.0,
         bool fail_on_non_monotonically_increasing_time = true);

  //----------------------------------------------------------------
  // bbox
  //
  YAE_API Timespan
  bbox(const std::list<Timespan> & track);

  //----------------------------------------------------------------
  // expand_bbox
  //
  YAE_API void
  expand_bbox(Timespan & bbox, const Timespan & s);


  //----------------------------------------------------------------
  // Timeline
  //
  struct YAE_API Timeline
  {

    //----------------------------------------------------------------
    // Track
    //
    struct Track
    {
      // generate a lookup map for GOPs, indexed by PTS value:
      void generate_gops(std::map<TTime, std::size_t> & GOPs) const;

      // given PTS timespan [t0, t1)
      // find keyframe samples [ka, kb, kc, kd] such that:
      //
      //      [t0,         t1)
      //   [ka, [kb,... kc), kd)
      //      [ia,         ib]
      //
      // then [kb, kc) samples can be copied
      // and [ka, kb), [kc, kd) samples must be re-encoded
      // to represent trimmed region [t0, t1)... unless one
      // is willing to skip re-encoding and tolerate some
      // decoding artifacts in which case [ia, kb) and [kc, ib)
      // can be copied instead.
      //
      bool find_bounding_samples(const Timespan & pts_span,
                                 std::size_t & ka,
                                 std::size_t & kb,
                                 std::size_t & kc,
                                 std::size_t & kd) const;

      // same as above, additionally pass-back sample indices [ia, ib]
      // corresponding to PTS span [t0, t1):
      //
      // points of interest for clipping a region of a track timeline:
      //
      //   |.....[.....|..........|..........|....].......
      //   ka    ia    kb                    kc   ib      kd
      //         t0                                t1
      //
      // for a seamless artifact-free triming one has to:
      //   - decode [ka, kb), drop [ka, ia), encode [ia, kb)
      //   - copy [kb, kc)
      //   - decode [kc, ib], encode [kc, ib]
      //
      // if decoding/encoding is not possible then:
      //   - drop [ka, ia)
      //   - copy [ia, ib]
      //   - expect decoding artifacts in the resulting output
      //
      bool find_samples_for(const Timespan & pts_span,
                            std::size_t & ka,
                            std::size_t & kb,
                            std::size_t & kc,
                            std::size_t & kd,
                            std::size_t & ia,
                            std::size_t & ib) const;

      // timespan built-up in [dts, dts + dur) increments,
      // should be quick due to monotonically increasing dts:
      std::list<Timespan> dts_span_;

      // timespan built-up in [pts, pts + dur) increments,
      // potentially slower due to non-monotonically increasing pts:
      std::list<Timespan> pts_span_;

      // frame dts, pts, and duration in order of appearance:
      std::vector<TTime> dts_;
      std::vector<TTime> pts_;
      std::vector<TTime> dur_;

      // sample indices of keyframes:
      std::set<std::size_t> keyframes_;
    };

    //----------------------------------------------------------------
    // TTracks
    //
    typedef std::map<std::string, Track> TTracks;

    // update time spans:
    void add_frame(const std::string & track_id,
                   bool keyframe,
                   const TTime & dts,
                   const TTime & pts,
                   const TTime & dur,
                   double tolerance);

    // translate this timeline by a given offset:
    Timeline & operator += (const TTime & offset);

    // extend this timeline via a union with
    // a given timeline translated by a given offset:
    void extend(const Timeline & timeline,
                const TTime & offset,
                double tolerace);

    // calculate the timeline bounding box for a given track:
    Timespan bbox_dts(const std::string & track_id) const;
    Timespan bbox_pts(const std::string & track_id) const;

    // bounding box for all tracks:
    Timespan bbox_dts_;
    Timespan bbox_pts_;

    // time data for individual tracks:
    TTracks tracks_;
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

    inline const std::list<TTime> & dts() const
    { return dts_; }

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
