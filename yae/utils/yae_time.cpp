// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Dec 21 13:06:20 MST 2017
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <ctype.h>
#include <iomanip>
#include <iterator>
#include <limits>
#include <math.h>
#include <numeric>
#include <sstream>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

// yae includes:
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // unix_epoch_time_at_utc_time
  //
  int64_t
  unix_epoch_time_at_utc_time(int year, int mon, int day,
                              int hour, int min, int sec)
  {
    struct tm t = { 0 };
    t.tm_year = year - 1900;
    t.tm_mon  = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;
#ifdef _WIN32
    int64_t t_epoch = _mkgmtime64(&t);
#else
    int64_t t_epoch = timegm(&t);
#endif
    return t_epoch;
  }

  //----------------------------------------------------------------
  // unix_epoch_gps_offset
  //
  const int64_t unix_epoch_gps_offset =
    yae::unix_epoch_time_at_utc_time(1980, 01, 06, 00, 00, 00);

  //----------------------------------------------------------------
  // unix_epoch_time_to_localtime
  //
  void
  unix_epoch_time_to_localtime(int64_t ts, struct tm & t)
  {
    memset(&t, 0, sizeof(t));
#ifdef _WIN32
    _localtime64_s(&t, &ts);
#else
    time_t tt = time_t(ts);
    localtime_r(&tt, &t);
#endif
  }

  //----------------------------------------------------------------
  // localtime_to_unix_epoch_time
  //
  int64_t
  localtime_to_unix_epoch_time(const struct tm & t)
  {
    struct tm tm = t;
#ifdef _WIN32
    int64_t t_epoch = _mktime64(&tm);
#else
    int64_t t_epoch = timelocal(&tm);
#endif
    return t_epoch;
  }

  //----------------------------------------------------------------
  // unix_epoch_time_to_utc
  //
  void
  unix_epoch_time_to_utc(int64_t ts, struct tm & t)
  {
    memset(&t, 0, sizeof(t));
#ifdef _WIN32
    _gmtime64_s(&t, &ts);
#else
    time_t tt = time_t(ts);
    gmtime_r(&tt, &t);
#endif
  }

  //----------------------------------------------------------------
  // utc_to_unix_epoch_time
  //
  int64_t
  utc_to_unix_epoch_time(const struct tm & t)
  {
    struct tm tm = t;
#ifdef _WIN32
    int64_t t_epoch = _mkgmtime64(&tm);
#else
    int64_t t_epoch = timegm(&tm);
#endif
    return t_epoch;
  }

  //----------------------------------------------------------------
  // to_yyyymmdd
  //
  std::string
  to_yyyymmdd(const struct tm & t, const char * date_sep)
  {
    std::string txt = yae::strfmt("%04i%s%02i%s%02i",
                                  t.tm_year + 1900,
                                  date_sep,
                                  t.tm_mon + 1,
                                  date_sep,
                                  t.tm_mday);
    return txt;
  }

  //----------------------------------------------------------------
  // to_hhmmss
  //
  std::string
  to_hhmmss(const struct tm & t, const char * time_sep)
  {
    std::string txt = yae::strfmt("%02i%s%02i%s%02i",
                                  t.tm_hour,
                                  time_sep,
                                  t.tm_min,
                                  time_sep,
                                  t.tm_sec);
    return txt;
  }

  //----------------------------------------------------------------
  // to_yyyymmdd_hhmmss
  //
  std::string
  to_yyyymmdd_hhmmss(const struct tm & t,
                     const char * date_sep,
                     const char * separator,
                     const char * time_sep)
  {
    std::string txt = yae::strfmt("%04i%s%02i%s%02i%s%02i%s%02i%s%02i",
                                  t.tm_year + 1900,
                                  date_sep,
                                  t.tm_mon + 1,
                                  date_sep,
                                  t.tm_mday,
                                  separator,
                                  t.tm_hour,
                                  time_sep,
                                  t.tm_min,
                                  time_sep,
                                  t.tm_sec);
    return txt;
  }

  //----------------------------------------------------------------
  // to_yyyymmdd_hhmm
  //
  std::string
  to_yyyymmdd_hhmm(const struct tm & t,
                   const char * date_sep,
                   const char * separator,
                   const char * time_sep)
  {
    std::string txt = yae::strfmt("%04i%s%02i%s%02i%s%02i%s%02i",
                                  t.tm_year + 1900,
                                  date_sep,
                                  t.tm_mon + 1,
                                  date_sep,
                                  t.tm_mday,
                                  separator,
                                  t.tm_hour,
                                  time_sep,
                                  t.tm_min);
    return txt;
  }

  //----------------------------------------------------------------
  // unix_epoch_time_to_localtime_str
  //
  std::string
  unix_epoch_time_to_localtime_str(int64_t ts,
                                   const char * date_sep,
                                   const char * separator,
                                   const char * time_sep)
  {
    struct tm t = { 0 };
    unix_epoch_time_to_localtime(ts, t);
    return to_yyyymmdd_hhmmss(t, date_sep, separator, time_sep);
  }

  //----------------------------------------------------------------
  // unix_epoch_time_to_utc_str
  //
  std::string
  unix_epoch_time_to_utc_str(int64_t ts,
                             const char * date_sep,
                             const char * separator,
                             const char * time_sep)
  {
    struct tm t = { 0 };
    unix_epoch_time_to_utc(ts, t);
    return to_yyyymmdd_hhmmss(t, date_sep, separator, time_sep);
  }


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

    subsec_t(int64_t tsec, int64_t tsub, int64_t base):
      tsec_(tsec),
      tsub_(tsub),
      base_(base)
    {
      if (tsub_ < 0)
      {
        tsec_ -= 1;
        tsub_ += base_;
      }
    }

    inline int64_t tsec_abs() const
    { return tsec_ < 0 ? ~tsec_ : tsec_; }

    inline int64_t max_base(const subsec_t & other) const
    {
      int64_t base = std::max<int64_t>(base_, other.base_);
      int64_t nsec = std::numeric_limits<int64_t>::max() / base;

      if (nsec < tsec_abs() || nsec < other.tsec_abs())
      {
        // try to avoid an overflow:
        base = std::min<int64_t>(base_, other.base_);
      }

      return base;
    }

    inline subsec_t add(const subsec_t & other) const
    {
      int64_t base = max_base(other);
      int64_t sec = (tsec_ + other.tsec_);
      int64_t ss = (tsub_ * base) / base_ + (other.tsub_ * base) / other.base_;
      return subsec_t(sec, ss, base);
    }

    inline subsec_t sub(const subsec_t & other) const
    {
      int64_t base = max_base(other);
      int64_t sec = (tsec_ - other.tsec_);
      int64_t a = (tsub_ * base) / base_;
      int64_t b = (other.tsub_ * base) / other.base_;
      int64_t d = (a < b) ? 1 : 0;
      int64_t ss = base * d + a - b;
      return subsec_t(sec - d, ss, base);
    }

    inline bool eq(const subsec_t & other) const
    {
      return (tsec_ != other.tsec_) ? false : subsec_eq(other);
    }

    inline bool subsec_eq(const subsec_t & other) const
    {
      int64_t base = max_base(other);
      int64_t a = (tsub_ * base) / base_;
      int64_t b = (other.tsub_ * base) / other.base_;
      return a == b;
    }

    inline bool lt(const subsec_t & other) const
    {
      return
        (other.tsec_ < tsec_) ? false :
        (tsec_ < other.tsec_) ? true :
        subsec_lt(other);
    }

    inline bool subsec_lt(const subsec_t & other) const
    {
      int64_t base = max_base(other);
      int64_t a = (tsub_ * base) / base_;
      int64_t b = (other.tsub_ * base) / other.base_;
      return a < b;
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
  // TTime::max_flicks
  //
  const TTime &
  TTime::max_flicks()
  {
    static const TTime t(std::numeric_limits<int64_t>::max(), TTime::Flicks);
    return t;
  }

  //----------------------------------------------------------------
  // TTime::min_flicks
  //
  const TTime &
  TTime::min_flicks()
  {
    static const TTime t(std::numeric_limits<int64_t>::min(), TTime::Flicks);
    return t;
  }

  //----------------------------------------------------------------
  // TTime::now
  //
  TTime
  TTime::now()
  {
#ifdef _WIN32
    // Windows epoch starts 1601-01-01T00:00:00Z
    // UNIX/Linux epoch starts 1970-01-01T00:00:00Z
    // elapsed time from windows epoch to UNIX epoch is 11644473600 sec

    // get 100-nanosecond intervals that have elapsed since
    // 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC).
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER tmp;
    tmp.HighPart = ft.dwHighDateTime;
    tmp.LowPart = ft.dwLowDateTime;

    static const int64_t unix_epoch_offset_usec = 11644473600000000LL;
    int64_t usec = tmp.QuadPart / 10;
    TTime t(usec - unix_epoch_offset_usec, 1000000);
#else
    struct timeval now;
    gettimeofday(&now, NULL);

    TTime t(int64_t(now.tv_sec) * 1000000 + int64_t(now.tv_usec), 1000000);
#endif

    return t;
  }

  //----------------------------------------------------------------
  // TTime::gps_now
  //
  TTime
  TTime::gps_now()
  {
    TTime t = TTime::now();
    t -= TTime(unix_epoch_gps_offset, 1);
    return t;
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
  TTime::TTime(int64_t time, uint64_t base):
    time_(time),
    base_(base)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime(double seconds):
    time_((int64_t)(1000000.0 * seconds)),
    base_(1000000)
  {}

  //----------------------------------------------------------------
  // TTime::rebased
  //
  TTime
  TTime::rebased(uint64_t base) const
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
  // TTime::operator *=
  //
  TTime &
  TTime::operator *= (double s)
  {
    time_ = int64_t(yae::round(time_ * s));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator *
  //
  TTime
  TTime::operator * (double s) const
  {
    return TTime(int64_t(yae::round(time_ * s)), base_);
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
  TTime::reset(int64_t time, uint64_t base)
  {
    time_ = time;
    base_ = base;
  }

  //----------------------------------------------------------------
  // TTime::get
  //
  int64_t
  TTime::get(uint64_t base) const
  {
    if (base_ == base ||
        time_ == std::numeric_limits<int64_t>::min() ||
        time_ == std::numeric_limits<int64_t>::max())
    {
      return time_;
    }

    subsec_t ss(*this);
    int64_t t = ss.tsec_ * base + (ss.tsub_ * base) / ss.base_;
    return t;
  }

  //----------------------------------------------------------------
  // to_hhmmss
  //
  static bool
  to_hhmmss(int64_t time,
            uint64_t base,
            std::string & ts,
            const char * separator,
            bool includeNegativeSign = false)
  {
    bool negative = (time < 0);

    int64_t t = negative ? -time : time;
    t /= base;

    int64_t seconds = t % 60;
    t /= 60;

    int64_t minutes = t % 60;
    int64_t hours = t / 60;

    std::ostringstream os;

    if (negative && includeNegativeSign && (seconds || minutes || hours))
    {
      os << '-';
    }

    os << std::setw(2) << std::setfill('0') << (int64_t)(hours) << separator
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

    uint64_t t = negative ? -time_ : time_;
    uint64_t remainder = t % base_;
    uint64_t frac = (precision * remainder) / base_;

    // count number of digits required for given precision:
    uint64_t digits = 0;
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

    uint64_t t = negative ? -time_ : time_;
    uint64_t ff = uint64_t(ceil_fps * double(t % base_) / double(base_));
    t /= base_;

    uint64_t ss = t % 60;
    t /= 60;

    uint64_t mm = t % 60;
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
      std::size_t(::ceil(::log10(ceil_fps)));

    os << std::setw(2) << std::setfill('0') << t << separator
       << std::setw(2) << std::setfill('0') << mm << separator
       << std::setw(2) << std::setfill('0') << ss << framenum_separator
       << std::setw(w) << std::setfill('0') << ff;

    ts = os.str();
  }

  //----------------------------------------------------------------
  // to_short_txt
  //
  std::string
  TTime::to_short_txt() const
  {
    return yae::to_short_txt(this->sec());
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
  // to_short_txt
  //
  std::string
  to_short_txt(double seconds)
  {
    std::ostringstream oss;

    bool negative = (seconds < 0);
    int64_t t = int64_t(1000 * (negative ? -seconds : seconds));

    int64_t ms = t % 1000;
    t /= 1000;

    int64_t ss = t % 60;
    t /= 60;

    int64_t mm = t % 60;
    t /= 60;

    int64_t hh = t % 24;

    if (negative)
    {
      oss << '-';
    }

    if (hh)
    {
      oss << hh << ':'
          << std::setw(2) << std::setfill('0') << mm << ':'
          << std::setw(2) << std::setfill('0') << ss;
    }
    else if (mm)
    {
      oss << mm << ':'
          << std::setw(2) << std::setfill('0') << ss;
    }
    else
    {
      oss << ss;
    }

    if (ms)
    {
      oss << '.' << std::setw(3) << std::setfill('0') << ms;
    }

    std::string txt = oss.str();
    return txt;
  }

  //----------------------------------------------------------------
  // is_one_of
  //
  static const char *
  is_one_of(const std::string & text, const char ** options)
  {
    for (const char ** i = options; *i != NULL; ++i)
    {
      const char * option = *i;
      if (text == option)
      {
        return option;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // subsec_base_for
  //
  static int64_t
  subsec_base_for(const std::string & number)
  {
    int64_t base = 1;

    for (std::size_t i = 0, n = number.size(); i < n; i++)
    {
      base *= 10;
    }

    return base;
  }

  //----------------------------------------------------------------
  // TimeParser
  //
  struct TimeParser
  {
    TimeParser():
      sign_(0),
      hh_(-1),
      mm_(-1),
      ss_(-1),
      xx_(-1),
      xx_base_(-1),
      xx_sep_(NULL)
    {}

    //----------------------------------------------------------------
    // Token
    //
    struct Token
    {
      enum type_t { kUndefined, kNumber, kOther };
      Token(): type_(kUndefined) {}

      inline bool is_empty() const
      { return text_.empty(); }

      inline bool is_number() const
      { return type_ == kNumber; }

      inline bool is_other() const
      { return type_ == kOther; }

      inline void clear()
      {
        type_ = kUndefined;
        text_.clear();
      }

      type_t type_;
      std::string text_;
    };

    bool parse_time(TTime & result,
                    const char * hhmmss_xx,
                    const char * mm_separator,
                    const char * xx_separator,
                    const double frame_rate)
    {
      std::vector<Token> tokens;
      Token token;

      std::size_t num_numbers = 0;
      std::size_t num_others = 0;

      const std::size_t sz = strlen(hhmmss_xx);
      for (const char * i = hhmmss_xx, * end = hhmmss_xx + sz; i < end; i++)
      {
        char c = *i;
        if (isspace(c))
        {
          if (!token.is_empty())
          {
            num_numbers += (token.is_number() ? 1 : 0);
            num_others += (token.is_other() ? 1 : 0);
            tokens.push_back(token);
            token.clear();
          }

          continue;
        }

        if ('0' <= c && c <= '9')
        {
          if (!token.is_empty() && !token.is_number())
          {
            tokens.push_back(token);
            token.clear();
            num_others++;
          }

          token.type_ = Token::kNumber;
          token.text_ += c;
        }
        else
        {
          if (!token.is_empty() && !token.is_other())
          {
            tokens.push_back(token);
            token.clear();
            num_numbers++;
          }

          token.type_ = Token::kOther;
          token.text_ += c;
        }
      }

      if (!token.is_empty())
      {
        num_numbers += (token.is_number() ? 1 : 0);
        num_others += (token.is_other() ? 1 : 0);
        tokens.push_back(token);
        token.clear();
      }

      if (tokens.empty())
      {
        return false;
      }

      static const char * mm_separators[] = { ":", NULL };

      sign_ = 0;
      hh_ = -1;
      mm_ = -1;
      ss_ = -1;
      xx_ = -1;
      xx_base_ = -1;
      xx_sep_ = NULL;

      // handle (optional) sign first:
      if (tokens.front().is_other())
      {
        const std::string & text = tokens.front().text_;
        if (text == "-")
        {
          sign_ = -1;
        }
        else if (text == "+")
        {
          sign_ = 1;
        }
      }

      if (sign_)
      {
        tokens.erase(tokens.begin());
        num_others--;
      }

      if (!parse(tokens, num_numbers, num_others, mm_separator, xx_separator))
      {
        return false;
      }

      subsec_t t;
      hh_ = std::max<int64_t>(0, hh_);
      mm_ = std::max<int64_t>(0, mm_);
      ss_ = std::max<int64_t>(0, ss_);
      xx_ = std::max<int64_t>(0, xx_);
      xx_base_ = std::max<int64_t>(1, xx_base_);

      t.tsec_ = ss_ + 60 * (mm_ + 60 * hh_);

      static const char * framenum_separators[] = { ":", ";", NULL };
      if (xx_sep_ && is_one_of(std::string(xx_sep_), framenum_separators))
      {
        if (!frame_rate || int64_t(ceil(frame_rate)) <= xx_)
        {
          return false;
        }

        TTime dt = frameDurationForFrameRate(frame_rate);
        t.tsub_ = dt.time_ * xx_;
        t.base_ = dt.base_;
      }
      else
      {
        t.tsub_ = std::max<int64_t>(0, xx_);
        t.base_ = std::max<int64_t>(1, xx_base_);
      }

      result = sign_ < 0 ? -TTime(t) : TTime(t);
      return true;
    }

  protected:
    bool parse(const std::vector<Token> & tokens,
               std::size_t num_numbers,
               std::size_t num_others,
               const char * mm_separator,
               const char * xx_separator)
    {
      if (num_numbers < 1)
      {
        return false;
      }

      std::size_t num_tokens = tokens.size();
      if (num_numbers == num_others && num_tokens % 2 == 0)
      {
        // parse XXu YYu ZZu WWu formatted time:
        for (std::size_t i = 0; i < num_tokens; i += 2)
        {
          const Token & a = tokens.at(i);
          const Token & b = tokens.at(i + 1);
          if (!parse_number_and_label(a, b))
          {
            return false;
          }
        }

        return true;
      }

      if (num_numbers > 4 || num_numbers != num_others + 1)
      {
        return false;
      }

      const Token * t0 = &(tokens.at(0));
      const Token * t = t0 + tokens.size();

      static const char * subsec_separators[] = { ".", ";", NULL };
      if (num_numbers == 4 ||
          (num_numbers >= 2 &&
           (xx_separator ?
            (t[-2].text_ == xx_separator) :
            (is_one_of(t[-2].text_, subsec_separators) != NULL))))
      {
        if (!parse_xx(t[-2], t[-1], xx_separator))
        {
          return false;
        }

        num_numbers--;
        num_tokens -= 2;
        t -= 2;
      }

      if (num_numbers == 4)
      {
        return false;
      }

      if (!num_tokens || !parse(ss_, t[-1]))
      {
        return false;
      }

      if (num_tokens >= 3 && !parse(mm_, t[-2], t[-3], mm_separator))
      {
        return false;
      }

      if (num_tokens >= 5 && !parse(hh_, t[-4], t[-5], mm_separator))
      {
        return false;
      }

      return true;
    }

    bool parse_number_and_label(const Token & a, const Token & b)
    {
      // accept a number followed by units label
      if (a.is_other() || b.is_number())
      {
        return false;
      }

      if (b.text_ == "h" && hh_ == -1)
      {
        hh_ = to_scalar<int64_t>(a.text_);
      }
      else if (b.text_ == "m" && mm_ == -1)
      {
        mm_ = to_scalar<int64_t>(a.text_);
      }
      else if (b.text_ == "s" && ss_ == -1)
      {
        ss_ = to_scalar<int64_t>(a.text_);
      }
      else if (b.text_ == "ms" && xx_ == -1)
      {
        xx_ = to_scalar<int64_t>(a.text_);
        xx_base_ = 1000;
      }
      else if (b.text_ == "us" && xx_ == -1)
      {
        xx_ = to_scalar<int64_t>(a.text_);
        xx_base_ = 1000000;
      }
      else
      {
        // unknown units label, or same field specified more than once:
        return false;
      }

      return true;
    }

    bool parse_xx(const Token & a,
                  const Token & b,
                  const char * xx_separator)
    {
      // accept a separator followed by a number:
      if (xx_sep_ || a.is_number() || b.is_other())
      {
        return false;
      }

      if (xx_separator && a.text_ == xx_separator)
      {
        xx_sep_ = xx_separator;
      }
      else if (!xx_separator)
      {
        static const char * xx_separators[] = { ".", ":", ";", NULL };
        xx_sep_ = is_one_of(a.text_, xx_separators);
      }

      if (!xx_sep_)
      {
        // same field parsed more than once:
        return false;
      }

      xx_ = to_scalar<int64_t>(b.text_);
      xx_base_ = subsec_base_for(b.text_);
      return true;
    }

    bool parse(int64_t & result,
               const Token & a,
               const Token & b,
               const char * separator)
    {
      // accept a separator followed by a number:
      if (result != -1 || a.is_number() || b.is_other())
      {
        return false;
      }

      const char * sep = NULL;
      if (separator && a.text_ == separator)
      {
        sep = separator;
      }
      else if (!separator)
      {
        static const char * separators[] = { ":", NULL };
        sep = is_one_of(a.text_, separators);
      }

      if (!sep)
      {
        return false;
      }

      result = to_scalar<int64_t>(b.text_);
      return true;
    }

    bool parse(int64_t & result, const Token & a)
    {
      if (result != -1 || !a.is_number())
      {
        return false;
      }

      result = to_scalar<int64_t>(a.text_);
      return true;
    }

    int64_t sign_;
    int64_t hh_;
    int64_t mm_;
    int64_t ss_;
    int64_t xx_;
    int64_t xx_base_;
    const char * xx_sep_;
  };

  //----------------------------------------------------------------
  // parse_time
  //
  bool parse_time(TTime & result,
                  const char * hhmmss_xx,
                  const char * mm_separator,
                  const char * xx_separator,
                  const double frame_rate)
  {
    TimeParser parser;
    return parser.parse_time(result,
                             hhmmss_xx,
                             mm_separator,
                             xx_separator,
                             frame_rate);
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

    if (keyframes_.empty())
    {
      // audio, subs... every frame is a keyframe:
      for (std::size_t i = 0, end = dts_.size(); i < end; i++)
      {
        const TTime & t0 = pts_[i];
        GOPs[t0] = i;
      }
    }
    else
    {
      // if the 1st packet is not a keyframe...
      // it's a malformed GOP that preceeds the 1st well formed GOP,
      // and it must be accounted for:
      if (!yae::has<std::size_t>(keyframes_, 0))
      {
        const TTime & t0 = pts_[0];
        GOPs[t0] = 0;
      }

      for (std::set<std::size_t>::const_iterator
             i = keyframes_.begin(); i != keyframes_.end(); ++i)
      {
        const std::size_t & x = *i;
        const TTime & t0 = pts_[x];
        GOPs[t0] = x;
      }
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
  // Timeline::Track::dts_sample_index
  //
  bool
  Timeline::Track::find_sample_by_dts(const TTime & dts, std::size_t & i) const
  {
    std::vector<TTime>::const_iterator found =
      std::upper_bound(dts_.begin(), dts_.end(), dts);

    if (found != dts_.end())
    {
      YAE_ASSERT(found != dts_.begin());
      i = (found - dts_.begin()) - 1;
      return found != dts_.begin();
    }

    i = dts_.size() - 1;
    TTime end = dts_.back() + dur_.back();
    return dts < end;
  }


  //----------------------------------------------------------------
  // Timeline::add_frame
  //
  void
  Timeline::add_packet(const std::string & track_id,
                       bool keyframe,
                       std::size_t size,
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

    track.size_.push_back(size);
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

      // append additional packet sizes:
      dst.size_.insert(dst.size_.end(), src.size_.begin(), src.size_.end());

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

      // simply append additional packet durations:
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
  // bitrate_str
  //
  static std::string
  bitrate_str(const Timeline::Track & track)
  {
    static const char * units[] = { "bps", "Kbps", "Mbps", "Gbps", "Tbps" };
    static const std::size_t num_units = sizeof(units) / sizeof(units[0]);

    if (track.dts_span_.empty())
    {
      return std::string("n/a bps");
    }

    const TTime & t0 = track.dts_span_.front().t0_;
    const TTime & t1 = track.dts_span_.back().t1_;
    double dt_sec = (t1 - t0).sec();
    if (dt_sec == 0.0)
    {
      return std::string("inf bps");
    }

    uint64_t num_bytes = std::accumulate(track.size_.begin(),
                                         track.size_.end(),
                                         uint64_t(0));

    uint64_t bps = uint64_t((double(num_bytes * 8) / dt_sec) + 0.5);
    uint64_t remainder = 0;

    std::size_t i = 0;
    for (; i < num_units && bps > 1000; i++)
    {
      remainder = bps % 1000;
      bps /= 1000;
    }

    std::ostringstream oss;
    oss << "size: " << num_bytes << " bytes, bitrate: " << bps;
    if (remainder)
    {
      oss << '.' << std::setw(3) << std::setfill('0') << remainder;
    }
    oss << ' ' << units[i];

    return oss.str();
  }

  //----------------------------------------------------------------
  // operator
  //
  std::ostream &
  operator << (std::ostream & oss, const Timeline & timeline)
  {
    oss << "DTS: " << timeline.bbox_dts_
        << ", PTS: " << timeline.bbox_pts_
        << "\n";

    for (Timeline::TTracks::const_iterator
           i = timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      // shortcuts:
      const std::string & track_id = i->first;
      const Timeline::Track & track = i->second;

      // bitrate:
      oss << "track " << track_id
          << " packets: " << track.dts_.size()
          << ", " << bitrate_str(track) << '\n';

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

      // keyframes, if any:
      if (!track.keyframes_.empty())
      {
        oss << "track " << track_id << " keyframes:";
#if 0
        for (std::set<std::size_t>::const_iterator
               j = track.keyframes_.begin(); j != track.keyframes_.end(); ++j)
        {
          const std::size_t & sample = *j;
          const TTime & dts = track.dts_[sample];
          const TTime & pts = track.pts_[sample];
          oss << ' ' << pts << "(cts " << (pts - dts).get(1000) << "ms)";
        }
#else
        // present it as a GOP:
        static const char * alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::map<TTime, std::size_t> GOPs;
        track.generate_gops(GOPs);

        std::map<TTime, std::size_t>::const_iterator jend = GOPs.end();
        std::map<TTime, std::size_t>::const_iterator j0 = GOPs.begin();
        std::map<TTime, std::size_t>::const_iterator j1 = j0;
        std::advance(j1, 1);
        for (; j1 != jend; ++j0, ++j1)
        {
          const TTime & t0 = j0->first;
          const TTime & t1 = j1->first;
          std::size_t i0 = j0->second;
          std::size_t i1 = j1->second;

          oss << '\n' << t0 << ' ';
          for (std::size_t ix = i0; ix < i1; ix++)
          {
            std::size_t x = (ix - i0) % 10;
            oss << ((x > 0) ? '.' : alphabet[((ix - i0) / 10) % 36]);
          }
        }
#endif
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
    if (fps == std::numeric_limits<double>::infinity())
    {
      return TTime(0, 1);
    }

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

    return TTime(int64_t(frameDuration), uint64_t(frameDuration * fps));
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
  // FramerateEstimator::add
  //
  void
  FramerateEstimator::add(const FramerateEstimator & src, const TTime & offset)
  {
    max_ = std::max(max_, src.max_);

    for (std::list<TTime>::const_iterator
           i = src.dts_.begin(); i != src.dts_.end(); ++i)
    {
      const TTime & dts = *i;
      dts_.push_back(dts + offset);

      if (num_ < max_)
      {
        num_++;
      }
      else
      {
        dts_.pop_front();
      }
    }

    for (std::map<TTime, uint64_t>::const_iterator
           i = src.dur_.begin(); i != src.dur_.end(); ++i)
    {
      const TTime & msec = i->first;
      const uint64_t & num = i->second;
      dur_[msec] += num;

      const TTime & src_sum = yae::get(src.sum_, msec);
      TTime & dst_sum = sum_[msec];
      dst_sum = TTime(src_sum.time_ + dst_sum.get(src_sum.base_),
                      src_sum.base_);
    }
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

      uint64_t & num = dur_[msec];
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

    std::map<uint64_t, TTime> occurrences;
    uint64_t num = 0;
    TTime sum;

    for (std::map<TTime, uint64_t>::const_iterator
           i = dur_.begin(); i != dur_.end(); ++i)
    {
      const uint64_t & n = i->second;
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
    uint64_t inlier_num = 0;

    // calculate an inlier average by including occurrences
    // that happen less than 25% of the most frequent occurrence:
    TTime outlier_sum;
    uint64_t outlier_num = 0;
    {
      uint64_t max_occurrences = occurrences.rbegin()->first;
      for (std::map<TTime, uint64_t>::const_iterator
             i = dur_.begin(); i != dur_.end(); ++i)
      {
        const uint64_t & n = i->second;
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
    const std::map<TTime, uint64_t> & durations = estimator.durations();

    uint64_t total_occurrences = 0;
    for (std::map<TTime, uint64_t>::const_iterator
           i = durations.begin(); i != durations.end(); ++i)
    {
      const uint64_t & occurrences = i->second;
      total_occurrences += occurrences;
    }

    for (std::map<TTime, uint64_t>::const_iterator
           i = durations.begin(); i != durations.end(); ++i)
    {
      const TTime & dt = i->first;
      const uint64_t & occurrences = i->second;

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
