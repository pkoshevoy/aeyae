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
#include "yae/utils/yae_utils.h"


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
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & oss, const TTime & t)
  {
    oss << t.to_hhmmss_frac(1000, ":");
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

    double gap_t_to_this = (t0_ - t).toSeconds();
    double gap_this_to_t = (t - t1_).toSeconds();
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
  // Timeline::add_keyframe
  //
  void
  Timeline::add_keyframe(const std::string & track_id, const TTime & dts)
  {
    std::set<TTime> & track = keyframes_[track_id];
    track.insert(dts);
  }

  //----------------------------------------------------------------
  // Timeline::operator +=
  //
  Timeline &
  Timeline::operator += (const TTime & offset)
  {
    for (std::map<std::string, std::set<TTime> >::iterator
           i = keyframes_.begin(); i != keyframes_.end(); ++i)
    {
      std::set<TTime> & keyframes = i->second;
      std::set<TTime> tmp;

      for (std::set<TTime>::const_iterator j =
             keyframes.begin(); j != keyframes.end(); ++j)
      {
        tmp.insert(*j + offset);
      }

      keyframes.swap(tmp);
    }

    for (TTracks::iterator i = tracks_.begin(); i != tracks_.end(); ++i)
    {
      std::list<Timespan> & tt = i->second;
      for (std::list<Timespan>::iterator j = tt.begin(); j != tt.end(); ++j)
      {
        Timespan & t = *j;
        t += offset;
      }
    }

    bbox_ += offset;
    return *this;
  }

  //----------------------------------------------------------------
  // Timeline::extend
  //
  void
  Timeline::extend(const Timeline & timeline,
                   const TTime & offset,
                   double tolerance)
  {
    for (std::map<std::string, std::set<TTime> >::const_iterator i =
           timeline.keyframes_.begin(); i != timeline.keyframes_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const std::set<TTime> & keyframes = i->second;
      std::set<TTime> & track = keyframes_[track_id];

        for (std::set<TTime>::const_iterator
             j = keyframes.begin(); j != keyframes.end(); ++j)
      {
        TTime keyframe = (*j + offset);
        track.insert(keyframe);
      }
    }

    for (TTracks::const_iterator i =
           timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const std::list<Timespan> & track = i->second;

      for (std::list<Timespan>::const_iterator
             j = track.begin(); j != track.end(); ++j)
      {
        Timespan s = (*j + offset);
        extend(track_id, s, tolerance, false);
      }
    }
  }

  //----------------------------------------------------------------
  // Timeline::extend_track
  //
  bool
  Timeline::extend_track(const std::string & track_id,
                         const Timespan & s,
                         double tolerance,
                         bool fail_on_non_monotonically_increasing_time)
  {
    if (s.empty())
    {
      YAE_ASSERT(false);
      return false;
    }

    std::list<Timespan> & track = tracks_[track_id];
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
  // Timeline::extend
  //
  bool
  Timeline::extend(const std::string & track_id,
                   const Timespan & s,
                   double tolerance,
                   bool fail_on_non_monotonically_increasing_time)
  {
    if (!extend_track(track_id, s, tolerance,
                      fail_on_non_monotonically_increasing_time))
    {
      return false;
    }

    if (bbox_.t0_ > s.t0_)
    {
      bbox_.t0_ = s.t0_;
    }

    if (bbox_.t1_ < s.t1_)
    {
      bbox_.t1_ = s.t1_;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Timeline::bbox
  //
  Timespan
  Timeline::bbox(const std::string & track_id) const
  {
    TTracks::const_iterator found = tracks_.find(track_id);
    if (found == tracks_.end())
    {
      return Timespan();
    }

    // shortcuts:
    const std::list<Timespan> & track = found->second;
    const Timespan & head = track.front();
    const Timespan & tail = track.back();

    return Timespan(head.t0_, tail.t1_);
  }

  //----------------------------------------------------------------
  // operator
  //
  std::ostream &
  operator << (std::ostream & oss, const Timeline & timeline)
  {
    oss << timeline.bbox_ << "\n";

    for (Timeline::TTracks::const_iterator
           i = timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      // shortcuts:
      const std::string & track_id = i->first;
      const std::list<Timespan> & track = i->second;

      oss << "track " << track_id << ':';

      std::size_t size = 0;
      for (std::list<Timespan>::const_iterator j = track.begin();
           j != track.end(); ++j)
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
    }

    for (std::map<std::string, std::set<TTime> >::const_iterator i =
           timeline.keyframes_.begin(); i != timeline.keyframes_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const std::set<TTime> & keyframes = i->second;
      oss << "keyframes " << track_id << ':';

      for (std::set<TTime>::const_iterator j =
             keyframes.begin(); j != keyframes.end(); ++j)
      {
        const TTime & t = *j;
        oss << ' ' << t;
      }

      oss << '\n';
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
      dst_sum = TTime(src_sum.time_ + dst_sum.getTime(src_sum.base_),
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
      TTime msec(dt.getTime(1000), 1000);

      uint64 & num = dur_[msec];
      num++;

      TTime & sum = sum_[msec];
      dt.time_ += sum.getTime(dt.base_);
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

    double dt = (dts_.back() - dts_.front()).toSeconds();
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
      sum = TTime(dur.time_ + sum.getTime(dur.base_), dur.base_);
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
          outlier_sum = TTime(dur.time_ + outlier_sum.getTime(dur.base_),
                              dur.base_);
          outlier_num += n;
        }
        else
        {
          inlier_sum = TTime(dur.time_ + inlier_sum.getTime(dur.base_),
                             dur.base_);
          inlier_num += n;
        }
      }
    }

    stats.normal_ =
      double(yae::get(dur_, most_frequent)) /
      yae::get(sum_, most_frequent).toSeconds();

    stats.min_ =
      double(yae::get(dur_, max_duration)) /
      yae::get(sum_, max_duration).toSeconds();

    stats.max_ =
      double(yae::get(dur_, min_duration)) /
      yae::get(sum_, min_duration).toSeconds();

    stats.outlier_ = outlier_num ?
      double(outlier_num) / outlier_sum.toSeconds() :
      0.0;

    stats.inlier_ = double(inlier_num) / inlier_sum.toSeconds();
    stats.avg_ = double(num) / sum.toSeconds();

    double avg = stats.avg_;
    bool avg_ok = closeEnoughToStandardFrameRate(avg, avg, 0.01);

    double inlier = stats.inlier_;
    bool inlier_ok = closeEnoughToStandardFrameRate(inlier, inlier, 0.01);

    double max = stats.max_;
    bool max_ok = closeEnoughToStandardFrameRate(max, max, 0.01);

    double normal = stats.normal_;
    bool normal_ok = closeEnoughToStandardFrameRate(normal, normal, 0.01);

    double fps = (inlier_ok ? inlier :
                  (avg_ok && avg > normal) ? avg :
                  normal_ok ? normal :
                  max_ok ? max :
                  (max / avg < 2.5) ? max :
                  avg);
    return fps;
  }

  //----------------------------------------------------------------
  // operator
  //
  std::ostream &
  operator << (std::ostream & oss, const FramerateEstimator & estimator)
  {
    const std::map<TTime, uint64> & durations = estimator.durations();
    for (std::map<TTime, uint64>::const_iterator
           i = durations.begin(); i != durations.end(); ++i)
    {
      const TTime & dt = i->first;
      const uint64 & occurrences = i->second;

      oss << std::setw(6) << dt.time_ << " msec: " << occurrences
          << (occurrences == 1 ? " occurrence" : " occurrences")
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
