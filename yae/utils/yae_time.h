// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 13:06:20 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIME_H_
#define YAE_TIME_H_

// system includes:
#include <algorithm>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <string>
#include <time.h>
#include <vector>

// yae includes:
#include "yae/api/yae_api.h"


namespace yae
{

  //----------------------------------------------------------------
  // unix_epoch_time_at_utc_date
  //
  YAE_API int64_t
  unix_epoch_time_at_utc_time(int year, int mon, int day,
                              int hour, int min, int sec);

  //----------------------------------------------------------------
  // unix_epoch_gps_offset
  //
  // gps_time is approximately unix_time + gps_offset ... plus a small
  // gps_utc_error_offset (which is about 18s currently)
  //
  // GPS time origin is Jan 6th 1980, 00:00 UTC
  //
  // unix epoch gps offset is 315964800
  //
  extern const int64_t YAE_API unix_epoch_gps_offset;

  //----------------------------------------------------------------
  // unix_epoch_time_to_localtime
  //
  YAE_API void
  unix_epoch_time_to_localtime(int64_t ts, struct tm & t);

  //----------------------------------------------------------------
  // localtime_to_unix_epoch_time
  //
  YAE_API int64_t
  localtime_to_unix_epoch_time(const struct tm & t);

  //----------------------------------------------------------------
  // unix_epoch_time_to_utc
  //
  YAE_API void
  unix_epoch_time_to_utc(int64_t ts, struct tm & t);

  //----------------------------------------------------------------
  // utc_to_unix_epoch_time
  //
  YAE_API int64_t
  utc_to_unix_epoch_time(const struct tm & t);

 //----------------------------------------------------------------
  // to_yyyymmdd
  //
  YAE_API std::string
  to_yyyymmdd(const struct tm & t, const char * sep = "/");

  //----------------------------------------------------------------
  // to_hhmmss
  //
  YAE_API std::string
  to_hhmmss(const struct tm & t, const char * sep = ":");

  //----------------------------------------------------------------
  // to_yyyymmdd_hhmmss
  //
  YAE_API std::string
  to_yyyymmdd_hhmmss(const struct tm & t,
                     const char * date_sep = "/",
                     const char * separator = " ",
                     const char * time_sep = ":");

  //----------------------------------------------------------------
  // to_yyyymmdd_hhmm
  //
  YAE_API std::string
  to_yyyymmdd_hhmm(const struct tm & t,
                   const char * date_sep = "/",
                   const char * separator = " ",
                   const char * time_sep = ":");

  //----------------------------------------------------------------
  // unix_epoch_time_to_localtime_str
  //
  YAE_API std::string
  unix_epoch_time_to_localtime_str(int64_t ts,
                                   const char * date_sep = "/",
                                   const char * separator = " ",
                                   const char * time_sep = ":");

  //----------------------------------------------------------------
  // unix_epoch_time_to_utc_str
  //
  YAE_API std::string
  unix_epoch_time_to_utc_str(int64_t ts,
                             const char * date_sep = "/",
                             const char * separator = " ",
                             const char * time_sep = ":");

  //----------------------------------------------------------------
  // unix_epoch_time_to_gps_time
  //
  YAE_API uint32_t
  unix_epoch_time_to_gps_time(int64_t ts);

  //----------------------------------------------------------------
  // gps_time_to_unix_epoch_time
  //
  YAE_API int64_t
  gps_time_to_unix_epoch_time(uint32_t gps_time);

  //----------------------------------------------------------------
  // gps_time_to_localtime_str
  //
  YAE_API std::string
  gps_time_to_localtime_str(uint32_t gps_time);

  //----------------------------------------------------------------
  // gps_time_to_localtime
  //
  YAE_API void
  gps_time_to_localtime(uint32_t gps_time, struct tm & t);


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

    static const TTime & max_flicks();
    static const TTime & min_flicks();
    static double max_flicks_as_sec();
    static double min_flicks_as_sec();
    static TTime now();
    static TTime gps_now();

    TTime();
    TTime(int64_t time, uint64_t base);
    TTime(double seconds);

    inline bool valid() const
    { return base_ > 0; }

    inline bool invalid() const
    { return !valid(); }

    // return time expressed in a given time base:
    TTime rebased(uint64_t base) const;

    // NOTE: these round to seconds and drop sub-seconds:
    TTime ceil() const;
    TTime floor() const;
    TTime round() const;

    TTime & operator += (const TTime & dt);
    TTime operator + (const TTime & dt) const;

    TTime & operator -= (const TTime & dt);
    TTime operator - (const TTime & dt) const;

    TTime & operator *= (double s);
    TTime operator * (double s) const;

    bool operator < (const TTime & t) const;
    bool operator == (const TTime & t) const;

    inline bool operator != (const TTime & t) const
    { return !(this->operator==(t)); }

    inline bool operator > (const TTime & t) const
    { return t < *this; }

    inline bool operator <= (const TTime & t) const
    { return !(t < *this); }

    inline bool operator >= (const TTime & t) const
    { return !(*this < t); }

    void reset(int64_t time = 0, uint64_t base = 1001);

    int64_t get(uint64_t base) const;

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

    // return the shortest form of hh:mm:ss.msec
    // omitting leading zeros and trailing factional zeros
    std::string to_short_txt() const;

    // seconds.msec -- even shorter than to_short_txt:
    std::string sec_msec() const;

    int64_t time_;
    uint64_t base_;
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
  // parse_time
  //
  // parse time expressed as [[[-][hh]mm]ss[[.:;]xxx]]
  //
  YAE_API bool
  parse_time(TTime & t,
             const char * hh_mm_ss_xx,
             const char * mm_separator = NULL,
             const char * xx_separator = NULL,
             const double frame_rate = 0.0);

  //----------------------------------------------------------------
  // to_short_txt
  //
  YAE_API std::string
  to_short_txt(double seconds);


  //----------------------------------------------------------------
  // Timespan
  //
  struct YAE_API Timespan
  {
    Timespan(const TTime & t0 = TTime::max_flicks(),
             const TTime & t1 = TTime::min_flicks());

    Timespan & operator += (const TTime & offset);

    inline Timespan operator + (const TTime & offset) const
    { return Timespan(t0_ + offset, t1_ + offset); }

    inline Timespan & operator -= (const TTime & offset)
    { return this->operator += (-offset); }

    inline Timespan operator - (const TTime & offset) const
    { return this->operator + (-offset); }

    inline Timespan overlap(const Timespan & s) const
    { return Timespan(std::max(t0_, s.t0_), std::min(t1_, s.t1_)); }

    inline TTime dt() const
    { return t1_ - t0_; }

    inline bool empty() const
    { return t1_ < t0_; }

    inline bool disjoint(const Timespan & s) const
    { return t0_ > s.t1_ || s.t0_ > t1_; }

    inline bool overlaps(const Timespan & s) const
    { return !disjoint(s); }

    inline bool contains(const Timespan & s) const
    { return t0_ <= s.t0_ && s.t1_ <= t1_; }

    // check whether t is contained in [t0, t1):
    inline bool contains(const TTime & t) const
    { return t0_ <= t && t < t1_; }

    inline double duration_sec() const
    { return empty() ? 0.0 : (t1_ - t0_).sec(); }

    inline void
    reset(const TTime & t0 = TTime::max_flicks(),
          const TTime & t1 = TTime::min_flicks())
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
    struct YAE_API Track
    {

      //----------------------------------------------------------------
      // Trim
      //
      // points of interest for clipping a region of a track timeline:
      //
      //   |.....[.....|..........|..........|....].......
      //   ka    ia    kb                    kc   ib      kd
      //          t0   tb                    tc    t1
      //
      // for a seamless artifact-free triming one has to:
      //   - decode [ka, kb), drop [..., t0), encode [t0, tb)
      //   - copy [kb, kc)
      //   - decode [kc, ib], encode [tc, t1)
      //
      // if decoding/encoding is not possible then:
      //   - drop [ka, ia)
      //   - copy [ia, ib]
      //   - expect decoding artifacts in the resulting output
      //
      struct YAE_API Trim
      {
        Trim():
          ka_(std::numeric_limits<std::size_t>::max()),
          kb_(std::numeric_limits<std::size_t>::max()),
          kc_(std::numeric_limits<std::size_t>::max()),
          kd_(std::numeric_limits<std::size_t>::max()),
          ia_(std::numeric_limits<std::size_t>::max()),
          ib_(std::numeric_limits<std::size_t>::max())
        {}

        std::size_t ka_;
        std::size_t kb_;
        std::size_t kc_;
        std::size_t kd_;
        std::size_t ia_;
        std::size_t ib_;
      };

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
      bool find_samples_for(const Timespan & pts_span,
                            std::size_t & ka,
                            std::size_t & kb,
                            std::size_t & kc,
                            std::size_t & kd,
                            std::size_t & ia,
                            std::size_t & ib) const;

      inline bool
      find_samples_for(const Timespan & pts_span, Trim & samples) const
      {
        return find_samples_for(pts_span,
                                samples.ka_,
                                samples.kb_,
                                samples.kc_,
                                samples.kd_,
                                samples.ia_,
                                samples.ib_);
      }

      inline TTime get_dts(std::size_t i) const
      {
        const std::size_t n = dts_.size();
        YAE_ASSERT(n);
        return i < n ? dts_[i] : n ? dts_.back() + dur_.back() : TTime();
      }

      inline TTime get_pts(std::size_t i) const
      {
        const std::size_t n = pts_.size();
        YAE_ASSERT(n);
        return i < n ? pts_[i] : n ? pts_.back() + dur_.back() : TTime();
      }

      inline TTime get_dur(std::size_t i) const
      {
        const std::size_t n = pts_.size();
        YAE_ASSERT(n);
        return i < n ? dur_[i] : TTime();
      }

      inline std::size_t get_size(std::size_t i) const
      {
        const std::size_t n = pts_.size();
        YAE_ASSERT(n);
        return i < n ? size_[i] : 0;
      }

      // find sample index of the sample that spans a given DTS time point:
      bool find_sample_by_dts(const TTime & dts, std::size_t & index) const;

      // timespan built-up in [dts, dts + dur) increments,
      // should be quick due to monotonically increasing dts:
      std::list<Timespan> dts_span_;

      // timespan built-up in [pts, pts + dur) increments,
      // potentially slower due to non-monotonically increasing pts:
      std::list<Timespan> pts_span_;

      // packet size, dts, pts, and duration in order of appearance:
      std::vector<std::size_t> size_;
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
    void add_packet(const std::string & track_id,
                    bool keyframe,
                    std::size_t size,
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

    void add(const FramerateEstimator & s, const TTime & offset);

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

    inline const std::map<TTime, uint64_t> & durations() const
    { return dur_; }

    inline const std::list<TTime> & dts() const
    { return dts_; }

  protected:
    // a sliding window, for calculating a window average:
    std::list<TTime> dts_;
    std::size_t max_;
    std::size_t num_;

    // keep count of occurrences of various frame durations, msec:
    std::map<TTime, uint64_t> dur_;

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
