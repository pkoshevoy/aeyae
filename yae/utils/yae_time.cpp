// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 13:06:20 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iomanip>
#include <iterator>
#include <limits>
#include <math.h>
#include <sstream>

// yae includes:
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // subsec_t
  //
  struct subsec_t
  {
    subsec_t(const TTime & a = TTime()):
      tsec_(a.time_ / int64_t(a.base_)),
      tsub_(a.time_ % int64_t(a.base_)),
      base_(int64_t(a.base_))
    {
      if (tsub_ < 0)
      {
        tsec_ -= 1;
        tsub_ += base_;
      }
    }

    subsec_t add(const subsec_t & other) const
    {
      int64_t base = std::max<int64_t>(base_, other.base_);
      int64_t sec = (tsec_ + other.tsec_);
      int64_t ss = (tsub_ * base) / base_ + (other.tsub_ * base) / other.base_;
      TTime t(sec * base + ss, base);
      return subsec_t(t);
    }

    subsec_t sub(const subsec_t & other) const
    {
      int64_t base = std::max<int64_t>(base_, other.base_);
      int64_t sec = (tsec_ - other.tsec_);
      int64_t a = (tsub_ * base) / base_;
      int64_t b = (other.tsub_ * base) / other.base_;
      int64_t d = (a < b) ? 1 : 0;
      int64_t ss = base * d + a - b;
      TTime t((sec - d) * base + ss, base);
      return subsec_t(t);
    }

    inline bool eq(const subsec_t & ss) const
    {
      subsec_t diff = sub(ss);
      return !(diff.tsec_ || diff.tsub_);
    }

    inline bool lt(const subsec_t & ss) const
    {
      subsec_t diff = sub(ss);
      return diff.tsec_ < 0;
    }

    inline bool le(const subsec_t & ss) const
    {
      subsec_t diff = ss.sub(*this);
      return diff.tsec_ >= 0;
    }

    inline operator TTime() const
    {
      return TTime(tsec_ * base_ + tsub_, base_);
    }

    int64_t tsec_; // whole seconds
    int64_t tsub_; // sub-seconds expressed in base_ timebase
    int64_t base_;
  };

  //----------------------------------------------------------------
  // operator <<
  //
  static std::ostream &
  operator << (std::ostream & os, const subsec_t & t)
  {
    os << t.tsec_ << ".(" << t.tsub_ << "/" << t.base_ << ")";
    return os;
  }


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
  // TTime::rebased
  //
  TTime
  TTime::rebased(uint64 base) const
  {
    subsec_t ss(*this);
    int64_t t = (ss.tsub_ * base) / ss.base_;
    return TTime(ss.tsec_ * base + t, base);
  }

  //----------------------------------------------------------------
  // TTime::ceil
  //
  TTime TTime::ceil() const
  {
    return TTime((time_ + base_ - 1) / base_, 1).rebased(base_);
  }

  //----------------------------------------------------------------
  // TTime::floor
  //
  TTime
  TTime::floor() const
  {
    return TTime(time_ / base_, 1).rebased(base_);
  }

  //----------------------------------------------------------------
  // TTime::round
  //
  TTime
  TTime::round() const
  {
    return TTime((time_ + base_ / 2) / base_, 1).rebased(base_);
  }

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

    *this = subsec_t(*this).add(subsec_t(dt));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime
  TTime::operator + (const TTime & dt) const
  {
    if (base_ == dt.base_)
    {
      return TTime(time_ + dt.time_, base_);
    }

    return subsec_t(*this).add(subsec_t(dt));
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

    *this = subsec_t(*this).sub(subsec_t(dt));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime
  TTime::operator - (const TTime & dt) const
  {
    if (base_ == dt.base_)
    {
      return TTime(time_ - dt.time_, base_);
    }

    return subsec_t(*this).sub(subsec_t(dt));
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

    return subsec_t(*this).lt(subsec_t(t));
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

    return subsec_t(*this).le(subsec_t(t));
  }

  //----------------------------------------------------------------
  // TTime::operator ==
  //
  bool
  TTime::operator == (const TTime & t) const
  {
    if (t.base_ == base_)
    {
      return time_ == t.time_;
    }

    return subsec_t(*this).eq(subsec_t(t));
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
  // TTime::get
  //
  int64
  TTime::get(uint64 base) const
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
  // TTime::to_hhmmss_frame
  //
  void
  TTime::to_hhmmss_frame(std::string & ts,
                         double frame_rate,
                         const char * separator,
                         const char * framenum_separator) const
  {
    const double ceil_fps = ::ceil(frame_rate);
    const bool negative = (time_ < 0);

    uint64 t = negative ? -time_ : time_;
    uint64 ff = uint64(ceil_fps * double(t % base_) / double(base_));
    t /= base_;

    uint64 ss = t % 60;
    t /= 60;

    uint64 mm = t % 60;
    t /= 60;

    std::ostringstream os;
    if (negative && (ff || ss || mm || t))
    {
      os << '-';
    }

    std::size_t w =
      ceil_fps <= 10.0 ? 1 :
      ceil_fps <= 100.0 ? 2 :
      ceil_fps <= 1000.0 ? 3 :
      ceil_fps <= 10000.0 ? 4 :
      ::ceil(::log10(ceil_fps));

    os << std::setw(2) << std::setfill('0') << t << separator
       << std::setw(2) << std::setfill('0') << mm << separator
       << std::setw(2) << std::setfill('0') << ss << framenum_separator
       << std::setw(w) << std::setfill('0') << ff;

    ts = os.str();
  }

  //----------------------------------------------------------------
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & oss, const TTime & t)
  {
    oss << t.to_hhmmss_ms(":", ".");
    return oss;
  }


  //----------------------------------------------------------------
  // Timespan::Timespan
  //
  Timespan::Timespan(const TTime & t0, const TTime & t1):
    t0_(t0),
    t1_(t1)
  {}

  //----------------------------------------------------------------
  // Timespan::operator +=
  //
  Timespan &
  Timespan::operator += (const TTime & offset)
  {
    t0_ += offset;
    t1_ += offset;
    return *this;
  }

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

    double gap_s_to_this = (t0_ - s.t1_).sec();
    double gap_this_to_s = (s.t0_ - t1_).sec();
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
  // Timespan::diff
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

    double gap_t_to_this = (t0_ - t).sec();
    double gap_this_to_t = (t - t1_).sec();
    double gap =
      (gap_t_to_this > 0.0) ? -gap_t_to_this :
      (gap_this_to_t > 0.0) ? gap_this_to_t :
      0.0;

    return gap;
  }

  //----------------------------------------------------------------
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & oss, const Timespan & s)
  {
    oss << '[' << s.t0_ << ", " << s.t1_ << ')';
    return oss;
  }


  //----------------------------------------------------------------
  // merge
  //
  void
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
  // extend
  //
  bool
  extend(std::list<Timespan> & track,
         const Timespan & s,
         double tolerance,
         bool fail_on_non_monotonically_increasing_time)
  {
    if (s.empty())
    {
      YAE_ASSERT(false);
      return false;
    }

    if (track.empty())
    {
      track.push_back(s);
      return true;
    }

    Timespan & prev = track.back();
    double gap = prev.diff(s.t0_);
    if (gap > tolerance)
    {
      track.push_back(s);
      return true;
    }
    else if (gap >= 0.0)
    {
      gap = prev.extend(s, tolerance);
      return gap == 0.0;
    }

    if (fail_on_non_monotonically_increasing_time)
    {
      // time should be monotonically increasing:
      return false;
    }

    // time should be monotonically increasing, this is sub-optimal:
    merge(track, s, tolerance);
    return true;
  }

  //----------------------------------------------------------------
  // bbox
  //
  Timespan
  bbox(const std::list<Timespan> & track)
  {
    const Timespan & head = track.front();
    const Timespan & tail = track.back();
    return Timespan(head.t0_, tail.t1_);
  }

  //----------------------------------------------------------------
  // expand_bbox
  //
  void
  expand_bbox(Timespan & bbox, const Timespan & s)
  {
    if (bbox.t0_ > s.t0_)
    {
      bbox.t0_ = s.t0_;
    }

    if (bbox.t1_ < s.t1_)
    {
      bbox.t1_ = s.t1_;
    }
  }

  //----------------------------------------------------------------
  // Timeline::Track::generate_gops
  //
  void
  Timeline::Track::generate_gops(std::map<TTime, std::size_t> & GOPs) const
  {
    GOPs.clear();

    for (std::set<std::size_t>::const_iterator
           i = keyframes_.begin(); i != keyframes_.end(); ++i)
    {
      const std::size_t & x = *i;
      const TTime & t0 = pts_[x];
      GOPs[t0] = x;
    }

    if (!pts_.empty())
    {
      // close the last GOP to simplify lookups:
      std::size_t i = GOPs.empty() ? 0 : GOPs.rbegin()->second;
      std::size_t n = pts_.size();

      // find max PTS in the last GOP (can't assume it's the last value
      // because PTS can be stored out-of-order due to B-frames):
      TTime t1 = pts_[i++];
      for (; i < n; i++)
      {
        TTime ti = pts_[i] + dur_[i];
        if (t1 < ti)
        {
          t1 = ti;
        }
      }

      GOPs[t1] = pts_.size();
    }
  }

  //----------------------------------------------------------------
  // Timeline::Track::find_bounding_samples
  //
  bool
  Timeline::Track::find_bounding_samples(const Timespan & pts_span,
                                         std::size_t & ka,
                                         std::size_t & kb,
                                         std::size_t & kc,
                                         std::size_t & kd) const
  {
    // generate a lookup map for GOPs, indexed by PTS value:
    std::map<TTime, std::size_t> GOPs;
    generate_gops(GOPs);

    std::map<TTime, std::size_t>::const_iterator
      found = GOPs.upper_bound(pts_span.t0_);
    if (found == GOPs.end())
    {
      // no overlap:
      YAE_ASSERT(false);
      return false;
    }

    kb = found->second;
    if (found != GOPs.begin())
    {
      std::advance(found, -1);
    }
    ka = found->second;

    found = GOPs.lower_bound(pts_span.t1_);
    if (found == GOPs.end())
    {
      kc = pts_.size();
      kd = kc;
    }
    else
    {
      kd = found->second;
      if (found != GOPs.begin())
      {
        std::advance(found, -1);
      }
      kc = found->second;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Timeline::Track::find_samples_for
  //
  bool
  Timeline::Track::find_samples_for(const Timespan & pts_span,
                                    std::size_t & ka,
                                    std::size_t & kb,
                                    std::size_t & kc,
                                    std::size_t & kd,
                                    std::size_t & ia,
                                    std::size_t & ib) const
  {
    if (!find_bounding_samples(pts_span, ka, kb, kc, kd))
    {
      return false;
    }

    ia = ka;
    for (; ia < kb; ia++)
    {
      const TTime & t0 = pts_[ia];
      const TTime & dt = dur_[ia];
      TTime t1 = t0 + dt;

      if (t0 <= pts_span.t0_ && pts_span.t0_ < t1)
      {
        break;
      }
    }

    ib = kd - 1;
    for (std::size_t j = 1; j <= kd - kc; j++)
    {
      ib = kd - j;
      const TTime & t0 = pts_[ib];
      const TTime & dt = dur_[ib];
      TTime t1 = t0 + dt;

      if (t1 < pts_span.t1_)
      {
        break;
      }
    }

    return true;
  }


  //----------------------------------------------------------------
  // Timeline::add_frame
  //
  void
  Timeline::add_frame(const std::string & track_id,
                      bool keyframe,
                      const TTime & dts,
                      const TTime & pts,
                      const TTime & dur,
                      double tolerance)
  {
    Track & track = tracks_[track_id];

    if (keyframe)
    {
      track.keyframes_.insert(track.dts_.size());
    }

    track.dts_.push_back(dts);
    track.pts_.push_back(pts);
    track.dur_.push_back(dur);

    Timespan s(dts, dts + dur);
    if (!yae::extend(track.dts_span_, s, tolerance))
    {
      // non-monotonically increasing DTS:
      YAE_ASSERT(false);

      if (!yae::extend(track.dts_span_, s, tolerance, false))
      {
        YAE_ASSERT(false);
      }
    }

    expand_bbox(bbox_dts_, s);

    // PTS may be non-monotonically increasing due to B-frames, allow it:
    Timespan t(pts, pts + dur);
    if (!yae::extend(track.pts_span_, t, tolerance, false))
    {
      YAE_ASSERT(false);
    }

    expand_bbox(bbox_pts_, t);
  }

  //----------------------------------------------------------------
  // translate
  //
  static void
  translate(std::list<Timespan> & tt, const TTime & offset)
  {
    for (std::list<Timespan>::iterator j = tt.begin(); j != tt.end(); ++j)
    {
      Timespan & t = *j;
      t += offset;
    }
  }

  //----------------------------------------------------------------
  // translate
  //
  static void
  translate(std::vector<TTime> & tt, const TTime & offset)
  {
    for (std::vector<TTime>::iterator i = tt.begin(); i != tt.end(); ++i)
    {
      TTime & t = *i;
      t += offset;
    }
  }

  //----------------------------------------------------------------
  // Timeline::operator +=
  //
  Timeline &
  Timeline::operator += (const TTime & offset)
  {
    for (std::map<std::string, Track>::iterator
           i = tracks_.begin(); i != tracks_.end(); ++i)
    {
      Track & track = i->second;
      translate(track.dts_span_, offset);
      translate(track.pts_span_, offset);
      translate(track.dts_, offset);
      translate(track.pts_, offset);
    }

    bbox_dts_ += offset;
    bbox_pts_ += offset;
    return *this;
  }


  //----------------------------------------------------------------
  // extend
  //
  static void
  extend(std::set<std::size_t> & dst,
         const std::set<std::size_t> & src,
         std::size_t sample_offset)
  {
    typedef std::pair<std::set<std::size_t>::iterator, bool> TResult;
    for (std::set<std::size_t>::const_iterator
           j = src.begin(); j != src.end(); ++j)
    {
      const std::size_t & sample = *j;
      TResult r = dst.insert(sample + sample_offset);
      YAE_ASSERT(r.second);
    }
  }

  //----------------------------------------------------------------
  // extend
  //
  static void
  extend(std::vector<TTime> & dst,
         const std::vector<TTime> & src,
         const TTime & offset)
  {
    for (std::vector<TTime>::const_iterator
           i = src.begin(); i != src.end(); ++i)
    {
      const TTime & t = *i;
      dst.push_back(t + offset);
    }
  }

  //----------------------------------------------------------------
  // Timeline::extend
  //
  void
  Timeline::extend(const Timeline & timeline,
                   const TTime & offset,
                   double tolerance)
  {
    for (std::map<std::string, Track>::const_iterator i =
           timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const Track & src = i->second;
      Track & dst = tracks_[track_id];

      // update keyframes:
      yae::extend(dst.keyframes_, src.keyframes_, dst.dts_.size());

      // update DTS timeline:
      for (std::list<Timespan>::const_iterator
             j = src.dts_span_.begin(); j != src.dts_span_.end(); ++j)
      {
        Timespan s = (*j + offset);
        yae::extend(dst.dts_span_, s, tolerance, false);
      }

      // update PTS timeline:
      for (std::list<Timespan>::const_iterator
             j = src.pts_span_.begin(); j != src.pts_span_.end(); ++j)
      {
        Timespan s = (*j + offset);
        yae::extend(dst.pts_span_, s, tolerance, false);
      }

      // must offset additional pts, dts:
      yae::extend(dst.dts_, src.dts_, offset);
      yae::extend(dst.pts_, src.pts_, offset);

      // simply append additional frame durations:
      dst.dur_.insert(dst.dur_.end(), src.dur_.begin(), src.dur_.end());
    }

    // update overall bounding box:
    expand_bbox(bbox_dts_, timeline.bbox_dts_ + offset);
    expand_bbox(bbox_pts_, timeline.bbox_pts_ + offset);
  }

  //----------------------------------------------------------------
  // Timeline::bbox_dts
  //
  Timespan
  Timeline::bbox_dts(const std::string & track_id) const
  {
    TTracks::const_iterator found = tracks_.find(track_id);
    if (found == tracks_.end())
    {
      return Timespan();
    }

    const Track & track = found->second;
    return yae::bbox(track.dts_span_);
  }

  //----------------------------------------------------------------
  // Timeline::bbox_pts
  //
  Timespan
  Timeline::bbox_pts(const std::string & track_id) const
  {
    TTracks::const_iterator found = tracks_.find(track_id);
    if (found == tracks_.end())
    {
      return Timespan();
    }

    const Track & track = found->second;
    return yae::bbox(track.pts_span_);
  }

  //----------------------------------------------------------------
  // operator
  //
  std::ostream &
  operator << (std::ostream & oss, const Timeline & timeline)
  {
    oss << "DTS: " << timeline.bbox_dts_ << ", PTS: " << timeline.bbox_pts_
        << "\n";

    for (Timeline::TTracks::const_iterator
           i = timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      // shortcuts:
      const std::string & track_id = i->first;
      const Timeline::Track & track = i->second;

      // DTS timeline:
      oss << "track " << track_id << " DTS:";
      std::size_t size = 0;
      for (std::list<Timespan>::const_iterator
             j = track.dts_span_.begin(); j != track.dts_span_.end(); ++j)
      {
        const Timespan & span = *j;
        oss << ' ' << span;
        size++;
      }

      if (size > 1)
      {
        oss << ", " << size << " segments";
      }
      oss << '\n';

      // PTS timeline:
      oss << "track " << track_id << " PTS:";
      size = 0;
      for (std::list<Timespan>::const_iterator
             j = track.pts_span_.begin(); j != track.pts_span_.end(); ++j)
      {
        const Timespan & span = *j;
        oss << ' ' << span;
        size++;
      }

      if (size > 1)
      {
        oss << ", " << size << " segments";
      }
      oss << '\n';

      oss << "frames " << track_id << ": " << track.dts_.size() << '\n';

      // keyframes, if any:
      if (!track.keyframes_.empty())
      {
        oss << "keyframes " << track_id << ':';

        for (std::set<std::size_t>::const_iterator
               j = track.keyframes_.begin(); j != track.keyframes_.end(); ++j)
        {
          const std::size_t & sample = *j;
          const TTime & dts = track.dts_[sample];
          const TTime & pts = track.pts_[sample];
          oss << ' ' << pts << "(cts " << (pts - dts).get(1000) << "ms)";
        }
        oss << '\n';
      }
    }

    return oss;
  }


  //----------------------------------------------------------------
  // frameDurationForFrameRate
  //
  TTime
  frameDurationForFrameRate(double fps)
  {
    double frameDuration = 1000000.0;
    double frac = ceil(fps) - fps;

    if (frac == 0.0)
    {
      frameDuration = 1000.0;
    }
    else
    {
      double stdFps = closestStandardFrameRate(fps);
      double fpsErr = fabs(stdFps - fps);

      if (fpsErr < 1e-3)
      {
        frac = ceil(stdFps) - stdFps;
        frameDuration = (frac > 0) ? 1001.0 : 1000.0;
        fps = stdFps;
      }
    }

    return TTime(int64(frameDuration), uint64(frameDuration * fps));
  }

  //----------------------------------------------------------------
  // kStandardFrameRate
  //
  static const double kStandardFrameRate[] = {
    24000.0 / 1001.0,
    24.0,
    25.0,
    30000.0 / 1001.0,
    30.0,
    50.0,
    60000.0 / 1001.0,
    60.0,
    120.0,
    120000.0 / 1001.0,
    240.0,
    240000.0 / 1001.0,
    480.0,
    480000.0 / 1001.0
  };

  //----------------------------------------------------------------
  // closeEnoughToStandardFrameRate
  //
  bool
  closeEnoughToStandardFrameRate(double fps,
                                 double & closest,
                                 double tolerance)
  {
    const std::size_t n = sizeof(kStandardFrameRate) / sizeof(double);
    double min_err = std::numeric_limits<double>::max();
    closest = fps;

    for (std::size_t i = 0; i < n; i++)
    {
      double err = fabs(fps - kStandardFrameRate[i]);
      if (err <= min_err)
      {
        min_err = err;
        closest = kStandardFrameRate[i];
      }
    }

    return !(min_err > tolerance);
  }

  //----------------------------------------------------------------
  // closestStandardFrameRate
  //
  double
  closestStandardFrameRate(double fps, double tolerance)
  {
    double closest = fps;
    bool found = closeEnoughToStandardFrameRate(fps, closest, tolerance);
    return found ? closest : fps;
  }


  //----------------------------------------------------------------
  // FramerateEstimator::FramerateEstimator
  //
  FramerateEstimator::FramerateEstimator(std::size_t buffer_size):
    max_(buffer_size),
    num_(0)
  {}

  //----------------------------------------------------------------
  // FramerateEstimator::operator +=
  //
  FramerateEstimator &
  FramerateEstimator::operator += (const FramerateEstimator & src)
  {
    max_ = std::max(max_, src.max_);

    for (std::list<TTime>::const_iterator
           i = src.dts_.begin(); i != src.dts_.end(); ++i)
    {
      const TTime & dts = *i;
      dts_.push_back(dts);

      if (num_ < max_)
      {
        num_++;
      }
      else
      {
        dts_.pop_front();
      }
    }

    for (std::map<TTime, uint64>::const_iterator
           i = src.dur_.begin(); i != src.dur_.end(); ++i)
    {
      const TTime & msec = i->first;
      const uint64 & num = i->second;
      dur_[msec] += num;

      const TTime & src_sum = yae::get(src.sum_, msec);
      TTime & dst_sum = sum_[msec];
      dst_sum = TTime(src_sum.time_ + dst_sum.get(src_sum.base_),
                      src_sum.base_);
    }

    return *this;
  }

  //----------------------------------------------------------------
  // FramerateEstimator::push
  //
  void
  FramerateEstimator::push(const TTime & dts)
  {
    if (!dts_.empty())
    {
      const TTime & prev = dts_.back();

      bool monotonically_increasing = !(prev > dts);
      YAE_ASSERT(monotonically_increasing);

      TTime dt = monotonically_increasing ? dts - prev : prev - dts;
      TTime msec(dt.get(1000), 1000);

      uint64 & num = dur_[msec];
      num++;

      TTime & sum = sum_[msec];
      dt.time_ += sum.get(dt.base_);
      sum = dt;
    }

    dts_.push_back(dts);
    if (num_ < max_)
    {
      num_++;
    }
    else
    {
      dts_.pop_front();
    }
  }

  //----------------------------------------------------------------
  // FramerateEstimator::window_avg
  //
  double
  FramerateEstimator::window_avg() const
  {
    if (num_ < 2)
    {
      return 0.0;
    }

    double dt = (dts_.back() - dts_.front()).sec();
    double fps = double(num_ - 1) / dt;
    return fps;
  }

  //----------------------------------------------------------------
  // FramerateEstimator::best_guess
  //
  double
  FramerateEstimator::best_guess() const
  {
    Framerate stats;
    double fps = get(stats);
    return fps;
  }

  //----------------------------------------------------------------
  // FramerateEstimator::Framerate::Framerate
  //
  FramerateEstimator::Framerate::Framerate():
    normal_(0.0),
    max_(0.0),
    min_(0.0),
    avg_(0.0),
    inlier_(0.0),
    outlier_(0.0)
  {}

  //----------------------------------------------------------------
  // FramerateEstimator::get
  //
  double
  FramerateEstimator::get(FramerateEstimator::Framerate & stats) const
  {
    if (dur_.empty())
    {
      return 0.0;
    }

    std::map<uint64, TTime> occurrences;
    uint64 num = 0;
    TTime sum;

    for (std::map<TTime, uint64>::const_iterator
           i = dur_.begin(); i != dur_.end(); ++i)
    {
      const uint64 & n = i->second;
      const TTime & dt = i->first;
      occurrences[i->second] = dt;

      TTime dur = yae::get(sum_, dt);
      sum = TTime(dur.time_ + sum.get(dur.base_), dur.base_);
      num += n;
    }

    const TTime & most_frequent = occurrences.rbegin()->second;
    const TTime & min_duration = dur_.begin()->first;
    const TTime & max_duration = dur_.rbegin()->first;

    // calculate a inlier average by excluding occurrences
    // that happen less than 25% of the most frequent occurrence:
    TTime inlier_sum;
    uint64 inlier_num = 0;

    // calculate an inlier average by including occurrences
    // that happen less than 25% of the most frequent occurrence:
    TTime outlier_sum;
    uint64 outlier_num = 0;
    {
      uint64 max_occurrences = occurrences.rbegin()->first;
      for (std::map<TTime, uint64>::const_iterator
             i = dur_.begin(); i != dur_.end(); ++i)
      {
        const uint64 & n = i->second;
        const TTime & dt = i->first;
        TTime dur = yae::get(sum_, dt);
        double r = double(n) / double(max_occurrences);

        if (r < 0.25)
        {
          outlier_sum = TTime(dur.time_ + outlier_sum.get(dur.base_),
                              dur.base_);
          outlier_num += n;
        }
        else
        {
          inlier_sum = TTime(dur.time_ + inlier_sum.get(dur.base_),
                             dur.base_);
          inlier_num += n;
        }
      }
    }

    stats.normal_ =
      double(yae::get(dur_, most_frequent)) /
      yae::get(sum_, most_frequent).sec();

    stats.min_ =
      double(yae::get(dur_, max_duration)) /
      yae::get(sum_, max_duration).sec();

    stats.max_ =
      double(yae::get(dur_, min_duration)) /
      yae::get(sum_, min_duration).sec();

    stats.outlier_ = outlier_num ?
      double(outlier_num) / outlier_sum.sec() :
      0.0;

    stats.inlier_ = double(inlier_num) / inlier_sum.sec();
    stats.avg_ = double(num) / sum.sec();

    std::set<double> std_fps;

    double avg = stats.avg_;
    if (closeEnoughToStandardFrameRate(avg, avg, 0.01))
    {
      std_fps.insert(avg);
    }

    double inlier = stats.inlier_;
    if (closeEnoughToStandardFrameRate(inlier, inlier, 0.01))
    {
      std_fps.insert(inlier);
    }

    double outlier = stats.outlier_;
    if (closeEnoughToStandardFrameRate(outlier, outlier, 0.01))
    {
      std_fps.insert(outlier);
    }

    double max = stats.max_;
    if (closeEnoughToStandardFrameRate(max, max, 0.01))
    {
      std_fps.insert(max);
    }

    double normal = stats.normal_;
    if (closeEnoughToStandardFrameRate(normal, normal, 0.01))
    {
      std_fps.insert(normal);
    }

    double fps = std_fps.empty() ? avg : *(std_fps.rbegin());
    return fps;
  }

  //----------------------------------------------------------------
  // operator
  //
  std::ostream &
  operator << (std::ostream & oss, const FramerateEstimator & estimator)
  {
    const std::map<TTime, uint64> & durations = estimator.durations();

    uint64 total_occurrences = 0;
    for (std::map<TTime, uint64>::const_iterator
           i = durations.begin(); i != durations.end(); ++i)
    {
      const uint64 & occurrences = i->second;
      total_occurrences += occurrences;
    }

    for (std::map<TTime, uint64>::const_iterator
           i = durations.begin(); i != durations.end(); ++i)
    {
      const TTime & dt = i->first;
      const uint64 & occurrences = i->second;

      oss << std::setw(6) << dt.time_ << " msec: " << occurrences
          << (occurrences == 1 ? " occurrence" : " occurrences")
          << ", " << double(occurrences) / double(total_occurrences)
          << std::endl;
    }

    FramerateEstimator::Framerate stats;
    double window_avg_fps = estimator.window_avg();
    double best_guess_fps = estimator.get(stats);

    oss << " normal fps: " << stats.normal_ << '\n'
        << "    min fps: " << stats.min_ << '\n'
        << "    max fps: " << stats.max_ << '\n'
        << "    avg fps: " << stats.avg_ << '\n'
        << " inlier fps: " << stats.inlier_ << '\n'
        << "outlier fps: " << stats.outlier_ << '\n'
        << " window avg: " << window_avg_fps << '\n'
        << " best guess: " << best_guess_fps << std::endl;

    return oss;
  }

}
