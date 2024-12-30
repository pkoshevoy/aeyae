// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Dec  1 12:38:37 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_log.h"
#include "yae/utils/yae_benchmark.h"

// standard:
#include <ctype.h>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// yaeui:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif

// local:
#include "yae_dvr.h"


namespace yae
{

  //----------------------------------------------------------------
  // yaetv_log_rx
  //
  static const char * yaetv_log_rx =
    "^yaetv-\\d{8}-\\d{6}\\.log$";

  //----------------------------------------------------------------
  // rec_sched_rx
  //
  static const char * rec_sched_rx =
    "^rec-\\d{1,2}\\.\\d{1,2}-\\d{8}-\\d{4}\\.json.*$";

  //----------------------------------------------------------------
  // recording_rx
  //
  static const char * recording_rx =
    "^\\d{8}-\\d{4} \\d{1,2}\\.\\d{1,2} .+\\.mpg$";

  //----------------------------------------------------------------
  // epg_by_freq_rx
  //
  static const char * epg_by_freq_rx =
    "^epg-\\d{8,9}\\.json";

  //----------------------------------------------------------------
  // heartbeat_rx
  //
  static const char * heartbeat_rx =
    "^heartbeat-[a-f0-9]{8}"
    "-[a-f0-9]{4}"
    "-4[a-f0-9]{3}"
    "-[89aAbB][a-f0-9]{3}"
    "-[a-f0-9]{12}\\.json";

  //----------------------------------------------------------------
  // CollectFiles
  //
  struct CollectFiles
  {
    CollectFiles(std::map<std::string, std::string> & dst,
                 const char * filename_rx,
                 boost::regex_constants::syntax_option_type opts =
                 boost::regex::icase):
      pattern_(filename_rx, opts),
      files_(dst)
    {}

    bool operator()(bool is_folder,
                    const std::string & name,
                    const std::string & path)
    {
      if (!is_folder && boost::regex_match(name, pattern_))
      {
        files_[name] = path;
      }

      return true;
    }

  protected:
    boost::regex pattern_;

    // files, indexed by filename:
    std::map<std::string, std::string> & files_;
  };

  //----------------------------------------------------------------
  // CollectRecordings
  //
  struct CollectRecordings : CollectFiles
  {
    CollectRecordings(std::map<std::string, std::string> & dst):
      CollectFiles(dst, recording_rx, boost::regex::icase)
    {}
  };


  //----------------------------------------------------------------
  // Wishlist::Wishlist
  //
  Wishlist::Wishlist(int64_t lastmod):
    lastmod_(lastmod)
  {}

  //----------------------------------------------------------------
  // Wishlist::Item::set_title
  //
  void
  Wishlist::Item::set_title(const std::string & title_rx)
  {
    title_ = title_rx;
    rx_title_.reset();
  }

  //----------------------------------------------------------------
  // Wishlist::Item::set_description
  //
  void
  Wishlist::Item::set_description(const std::string & desc_rx)
  {
    description_ = desc_rx;
    rx_description_.reset();
  }

  //----------------------------------------------------------------
  // Wishlist::Item::ch_txt
  //
  std::string
  Wishlist::Item::ch_txt() const
  {
    std::ostringstream oss;

    if (channel_)
    {
      const std::pair<uint16_t, uint16_t> & ch_num = *channel_;
      oss << ch_num.first << '-' << ch_num.second;
    }
    else
    {
      oss << "*";
    }

    return std::string(oss.str().c_str());
  }

  //----------------------------------------------------------------
  // deregex_and_tolower
  //
  std::string
  deregex_and_tolower(const std::string & text_or_regex)
  {
    std::ostringstream oss;
    const char * str = &text_or_regex[0];
    const char * end = str + text_or_regex.size();
    const char * iter = str;
    uint32_t prev_uc = 0;
    uint32_t uc = 0;

    while (iter < end)
    {
      prev_uc = uc;
      const char * prev = iter;
      if (!yae::utf8_to_unicode(iter, end, uc))
      {
        oss << *iter;
        iter++;
        continue;
      }

      // if not plain printable ASCII:
      if (uc >= 0x80)
      {
        oss << std::string(prev, iter);
        continue;
      }

      // strip all non
      uint8_t c = uint8_t(uc);
      if (::isalnum(c))
      {
        c = char(::tolower(uc));
        oss << c;
      }
      else if (::isspace(c) && prev_uc != ' ')
      {
        uc = ' ';
        oss << ' ';
      }
    }

    return oss.str();
  }

  //----------------------------------------------------------------
  // Wishlist::Item::to_txt
  //
  std::string
  Wishlist::Item::to_txt() const
  {
    const char * sep = "";
    std::ostringstream oss;

    if (!title_.empty())
    {
      oss << sep << title_;
      sep = ", ";
    }

    if (!description_.empty())
    {
      oss << sep << description_;
      sep = ", ";
    }

    if (date_)
    {
      const struct tm & tm = *date_;
      int64_t ts = yae::localtime_to_unix_epoch_time(tm);
      oss << sep << yae::unix_epoch_time_to_localdate(ts);
      sep = ", ";
    }

    if (weekday_mask_ && *weekday_mask_)
    {
      const char * s = sep;
      uint16_t weekdays = *weekday_mask_;
      for (uint8_t i = 0; i < 7; i++)
      {
        uint8_t j = (i + 1) % 7;
        uint16_t wday = 1 << j;
        if ((weekdays & wday) == wday)
        {
          oss << s << kWeekdays[j];
          s = " ";
        }
      }

      sep = ", ";
    }

    if (when_)
    {
      const Timespan & when = *when_;
      oss << sep << when.t0_.to_hhmm() << " - " << when.t1_.to_hhmm();
      sep = ", ";
    }

    if (min_minutes_ && *min_minutes_)
    {
      uint16_t min_minutes = *min_minutes_;
      oss << sep << "GEQ " << min_minutes << "min";
      sep = ", ";
    }

    if (max_minutes_ && *max_minutes_)
    {
      uint16_t max_minutes = *max_minutes_;
      oss << sep << "LEQ " << max_minutes << "min";
      sep = ", ";
    }

    if (this->do_not_record())
    {
      oss << sep << "DNR";
    }

    return std::string(oss.str().c_str());
  }

  //----------------------------------------------------------------
  // Wishlist::Item::to_key
  //
  std::string
  Wishlist::Item::to_key() const
  {
    const char * sep = "";
    std::ostringstream oss;

    if (channel_)
    {
      const std::pair<uint16_t, uint16_t> & ch_num = *channel_;
      oss << sep << strfmt("%02i.%02i", ch_num.first, ch_num.second);
      sep = ", ";
    }
    else
    {
      oss << sep << "00.00";
      sep = ", ";
    }

    if (!title_.empty())
    {
      // YAE_BREAKPOINT_IF(al::ends_with(title_, "13 News.*"));
      std::string title = deregex_and_tolower(title_);
      oss << sep << title;
      sep = ", ";
    }

    if (!description_.empty())
    {
      std::string desc = deregex_and_tolower(description_);
      oss << sep << desc;
      sep = ", ";
    }

    if (date_)
    {
      const struct tm & tm = *date_;
      int64_t ts = yae::localtime_to_unix_epoch_time(tm);
      oss << sep << yae::unix_epoch_time_to_localdate(ts);
      sep = ", ";
    }

    if (weekday_mask_ && *weekday_mask_)
    {
      const char * s = sep;
      uint16_t weekdays = *weekday_mask_;
      uint16_t wday = 1;
      for (int i = 0; i < 7; i++)
      {
        if ((weekdays & wday) == wday)
        {
          oss << s << "wd_" << int(i);
          s = " ";
        }

        wday <<= 1;
      }

      sep = ", ";
    }

    if (when_)
    {
      const Timespan & when = *when_;
      oss << sep << when.t0_.to_hhmm() << " - " << when.t1_.to_hhmm();
      sep = ", ";
    }

    if (min_minutes_ && *min_minutes_)
    {
      uint16_t min_minutes = *min_minutes_;
      oss << sep << "GEQ " << min_minutes << " min";
      sep = ", ";
    }

    if (max_minutes_ && *max_minutes_)
    {
      uint16_t max_minutes = *max_minutes_;
      oss << sep << "LEQ " << max_minutes << " min";
      sep = ", ";
    }

    if (disabled_ && *disabled_)
    {
      oss << sep << "disabled";
      sep = ", ";
    }
    else
    {
      oss << sep << "enabled";
      sep = ", ";
    }

    return std::string(oss.str().c_str());
  }

  //----------------------------------------------------------------
  // Wishlist::Item::matches
  //
  bool
  Wishlist::Item::matches(const yae::mpeg_ts::EPG::Channel & channel,
                          const yae::mpeg_ts::EPG::Program & program) const
  {
    if (disabled_ && *disabled_)
    {
      return false;
    }

    if (channel_)
    {
      const std::pair<uint16_t, uint16_t> & require_channel = *channel_;
      if (channel.major_ != require_channel.first ||
          channel.minor_ != require_channel.second)
      {
        return false;
      }
    }

    if (date_)
    {
      const struct tm & tm = *date_;
#ifndef _WIN32
      YAE_EXPECT(tm.tm_gmtoff == program.tm_.tm_gmtoff);
#endif
      if (tm.tm_year != program.tm_.tm_year ||
          tm.tm_mon  != program.tm_.tm_mon  ||
          tm.tm_mday != program.tm_.tm_mday)
      {
        return false;
      }
    }

    if (min_minutes_ && *min_minutes_)
    {
      uint32_t min_duration = 60 * (*min_minutes_);
      if (program.duration_ < min_duration)
      {
        return false;
      }
    }

    if (max_minutes_ && *max_minutes_)
    {
      uint32_t max_duration = 60 * (*max_minutes_);
      if (program.duration_ > max_duration)
      {
        return false;
      }
    }

    Timespan timespan = when_ ? *when_ : Timespan();
    bool match_timespan = !timespan.empty();
    if (match_timespan)
    {
      TTime t0((program.tm_.tm_hour * 60 +
                program.tm_.tm_min) * 60 +
               program.tm_.tm_sec, 1);

      Timespan program_timespan(t0, t0 + TTime(program.duration_, 1));
      Timespan overlap = timespan.overlap(program_timespan);
      if (overlap.empty())
      {
        return false;
      }

      double overlap_sec = overlap.dt().sec();
      double overlap_ratio = overlap_sec / double(program.duration_);
      if (overlap_ratio < 0.5)
      {
        return false;
      }
    }

    uint8_t weekday_mask = weekday_mask_ ? *weekday_mask_ : 0;
    if (weekday_mask)
    {
      uint8_t program_wday = (1 << program.tm_.tm_wday);
      if ((weekday_mask & program_wday) != program_wday)
      {
        return false;
      }
    }

    bool match_title = !title_.empty();
    if (match_title &&
        !boost::iequals(program.title_, title_))
    {
      if (!rx_title_)
      {
        rx_title_.reset(boost::regex(title_, boost::regex::icase));
      }

      if (!boost::regex_match(program.title_, *rx_title_))
      {
        return false;
      }
    }

    bool match_description = !description_.empty();
    if (match_description &&
        !boost::iequals(program.description_, description_))
    {
      if (!rx_description_)
      {
        rx_description_.reset(boost::regex(description_,
                                           boost::regex::icase));
      }

      if (!boost::regex_match(program.description_, *rx_description_))
      {
        return false;
      }
    }

    if (channel_ && (date_ ||
                     weekday_mask ||
                     match_timespan))
    {
      return true;
    }

    return (match_title || match_description);
  }

  //----------------------------------------------------------------
  // Wishlist::Item::save
  //
  void
  Wishlist::Item::save(Json::Value & json) const
  {
    if (channel_)
    {
      const std::pair<uint16_t, uint16_t> & channel = *channel_;
      std::string major_minor = strfmt("%i.%i",
                                       int(channel.first),
                                       int(channel.second));
      json["channel"] = major_minor;
    }

    if (when_ && !when_->empty())
    {
      Json::Value & when = json["when"];
      when["t0"] = when_->t0_.to_hhmmss();
      when["t1"] = when_->t1_.to_hhmmss();
    }

    const uint16_t weekday_mask = weekday_mask_ ? *weekday_mask_ : 0;
    if (weekday_mask)
    {
      const char * separator = "";
      std::ostringstream oss;
      for (int i = 0; i < 7; i++)
      {
        int j = (i + 1) % 7;
        uint16_t required = (1 << j);
        if ((weekday_mask & required) == required)
        {
          oss << separator << kWeekdays[j];
          separator = " ";
        }
      }

      json["weekdays"] = oss.str();
    }

    yae::save(json, "date", date_);
    yae::save(json, "title", title_);
    yae::save(json, "description", description_);
    yae::save(json, "min_minutes", min_minutes_);
    yae::save(json, "max_minutes", max_minutes_);
    yae::save(json, "max_recordings", max_recordings_);
    yae::save(json, "skip_duplicates", skip_duplicates_);
    yae::save(json, "do_not_record", do_not_record_);
    yae::save(json, "disabled", disabled_);
  }

  //----------------------------------------------------------------
  // parse_channel_str
  //
  static bool
  parse_channel_str(const std::string & major_minor,
                    uint16_t & major,
                    uint16_t & minor)
  {
    bool ok = true;
    std::vector<std::string> tokens;
    YAE_ASSERT(ok = (yae::split(tokens, ".", major_minor.c_str()) == 2));
    if (ok)
    {
      major = boost::lexical_cast<uint16_t>(tokens[0]);
      minor = boost::lexical_cast<uint16_t>(tokens[1]);
    }
    return ok;
  }

  //----------------------------------------------------------------
  // parse_channel_str
  //
  static uint32_t
  parse_channel_str(const std::string & major_minor)
  {
    uint16_t major = 0;
    uint16_t minor = 0;
    if (!parse_channel_str(major_minor, major, minor))
    {
      return 0;
    }

    return yae::mpeg_ts::channel_number(major, minor);
  }

  //----------------------------------------------------------------
  // Wishlist::Item::load
  //
  void
  Wishlist::Item::load(const Json::Value & json)
  {
    if (json.isMember("channel"))
    {
      std::string major_minor;
      yae::load(json["channel"], major_minor);
      std::pair<uint16_t, uint16_t> channel;
      parse_channel_str(major_minor, channel.first, channel.second);
      channel_.reset(channel);
    }

    if (json.isMember("when"))
    {
      const Json::Value & when = json["when"];
      std::string t0 = when["t0"].asString();
      std::string t1 = when["t1"].asString();

      Timespan ts;
      YAE_EXPECT(yae::parse_time(ts.t0_, t0.c_str(), ":", "."));
      YAE_EXPECT(yae::parse_time(ts.t1_, t1.c_str(), ":", "."));
      if (ts.empty())
      {
        when_.reset();
      }
      else
      {
        when_.reset(ts);
      }
    }

    if (json.isMember("weekdays"))
    {
      uint16_t weekday_mask = 0;

      std::istringstream iss(json["weekdays"].asString());
      while (!iss.eof())
      {
        std::string day;
        iss >> day;

        if (day == "Sun")
        {
          weekday_mask |= Sun;
        }
        else if (day == "Mon")
        {
          weekday_mask |= Mon;
        }
        else if (day == "Tue")
        {
          weekday_mask |= Tue;
        }
        else if (day == "Wed")
        {
          weekday_mask |= Wed;
        }
        else if (day == "Thu")
        {
          weekday_mask |= Thu;
        }
        else if (day == "Fri")
        {
          weekday_mask |= Fri;
        }
        else if (day == "Sat")
        {
          weekday_mask |= Sat;
        }
      }

      if (weekday_mask)
      {
        weekday_mask_.reset(weekday_mask);
      }
      else
      {
        weekday_mask_.reset();
      }
    }

    yae::load(json, "date", date_);
    yae::load(json, "title", title_);
    yae::load(json, "description", description_);
    yae::load(json, "min_minutes", min_minutes_);
    yae::load(json, "max_minutes", max_minutes_);
    yae::load(json, "max_recordings", max_recordings_);
    yae::load(json, "skip_duplicates", skip_duplicates_);
    yae::load(json, "do_not_record", do_not_record_);
    yae::load(json, "disabled", disabled_);
  }

  //----------------------------------------------------------------
  // Wishlist::get
  //
  void
  Wishlist::get(std::map<std::string, Item> & wishlist) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    unsigned int index = 0;

    for (std::list<Item>::const_iterator
           i = items_.begin(); i != items_.end(); ++i, ++index)
    {
      const Item & item = *i;
      std::string key = item.to_key() + strfmt(", index %03u", index);
      wishlist[key] = item;
    }
  }

  //----------------------------------------------------------------
  // Wishlist::remove
  //
  bool
  Wishlist::remove(const std::string & wi_key)
  {
    if (!wi_key.empty())
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      unsigned int index = 0;

      for (std::list<Item>::iterator
             i = items_.begin(); i != items_.end(); ++i, ++index)
      {
        Item & item = *i;
        std::string item_key = item.to_key() + strfmt(", index %03u", index);
        if (item_key == wi_key)
        {
          i = items_.erase(i);
          return true;
        }
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Wishlist::update
  //
  void
  Wishlist::update(const std::string & wi_key, const Wishlist::Item & new_item)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (!wi_key.empty())
    {
      unsigned int index = 0;
      for (std::list<Item>::iterator
             i = items_.begin(); i != items_.end(); ++i, ++index)
      {
        Item & item = *i;
        std::string item_key = item.to_key() + strfmt(", index %03u", index);
        if (item_key == wi_key)
        {
          item = new_item;
          return;
        }
      }
    }

    items_.push_back(new_item);
  }

  //----------------------------------------------------------------
  // Wishlist::matches
  //
  yae::shared_ptr<Wishlist::Item>
  Wishlist::matches(const yae::mpeg_ts::EPG::Channel & channel,
                    const yae::mpeg_ts::EPG::Program & program) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::list<yae::shared_ptr<Wishlist::Item> > wanted;
    std::list<yae::shared_ptr<Wishlist::Item> > unwanted;

    for (std::list<Item>::const_iterator
           i = items_.begin(); i != items_.end(); ++i)
    {
      const Item & item = *i;
      if (item.matches(channel, program))
      {
        yae::shared_ptr<Item> item_ptr(new Item(item));

        if (item.do_not_record())
        {
          unwanted.push_back(item_ptr);
        }
        else
        {
          wanted.push_back(item_ptr);
        }
      }
    }

    // Do Not Record should have higher precedence:
    if (!unwanted.empty())
    {
      return unwanted.front();
    }
    else if (!wanted.empty())
    {
      return wanted.front();
    }

    return yae::shared_ptr<Item>();
  }

  //----------------------------------------------------------------
  // Wishlist::swap_to_latest
  //
  bool
  Wishlist::swap_to_latest(Wishlist & wishlist)
  {
    boost::unique_lock<boost::mutex> lock_this(mutex_);
    boost::unique_lock<boost::mutex> lock_that(wishlist.mutex_);

    if (lastmod_ >= wishlist.lastmod_)
    {
      return false;
    }

    std::swap(lastmod_, wishlist.lastmod_);
    items_.swap(wishlist.items_);
    return true;
  }

  //----------------------------------------------------------------
  // Wishlist::save
  //
  void
  Wishlist::save(Json::Value & json) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    yae::save(json["items"], items_);
  }

  //----------------------------------------------------------------
  // Wishlist::load
  //
  void
  Wishlist::load(const Json::Value & json)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    yae::load(json["items"], items_);
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const Wishlist::Item & item)
  {
    item.save(json);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, Wishlist::Item & item)
  {
    item.load(json);
  }


  //----------------------------------------------------------------
  // same
  //
  bool same(const TRecs & a, const TRecs & b)
  {
    TRecs::const_iterator ia = a.begin();
    TRecs::const_iterator ib = b.begin();
    for (; ia != a.end() && ib != b.end(); ++ia, ++ib)
    {
      const std::string & ka = ia->first;
      const std::string & kb = ib->first;
      if (ka != kb)
      {
        return false;
      }

      const Recording::Rec & ra = *(ia->second);
      const Recording::Rec & rb = *(ib->second);
      if (ra != rb)
      {
        return false;
      }
    }

    return ia == a.end() && ib == b.end();
  }


  //----------------------------------------------------------------
  // Schedule::is_recording_now
  //
  bool
  Schedule::is_recording_now(const TRecPtr & rec_ptr) const
  {
    if (!rec_ptr)
    {
      return false;
    }

    const Recording::Rec & rec = *rec_ptr;
    TRecordingPtr recording_ptr = get(rec.ch_num(), rec.gps_t0_);
    if (!recording_ptr)
    {
      return false;
    }

    const Recording & recording = *recording_ptr;
    return recording.is_recording();
  }

  //----------------------------------------------------------------
  // Schedule::enable_live
  //
  uint32_t
  Schedule::enable_live(uint32_t ch_num)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    uint32_t prev_ch = live_ch_ ? *live_ch_ : 0;
    live_ch_ = ch_num;
    return prev_ch;
  }

  //----------------------------------------------------------------
  // Schedule::disable_live
  //
  uint32_t
  Schedule::disable_live()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    uint32_t prev_ch = live_ch_ ? *live_ch_ : 0;
    live_ch_.reset();
    return prev_ch;
  }

  //----------------------------------------------------------------
  // Schedule::get_live_channel
  //
  uint32_t
  Schedule::get_live_channel() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    uint32_t live_ch = live_ch_ ? *live_ch_ : 0;
    return live_ch;
  }

  //----------------------------------------------------------------
  // find_program
  //
  static const yae::mpeg_ts::EPG::Program *
  find_program(const std::list<yae::mpeg_ts::EPG::Program> & programs,
               uint32_t gps_time)
  {
    for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
           i = programs.begin(); i != programs.end(); ++i)
    {
      const yae::mpeg_ts::EPG::Program & program = *i;
      uint32_t program_end = program.gps_time_ + program.duration_;
      if (program.gps_time_ <= gps_time && gps_time < program_end)
      {
        return &program;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Schedule::update
  //
  void
  Schedule::update(DVR & dvr, const yae::mpeg_ts::EPG & epg)
  {
    uint64_t gps_now = TTime::gps_now().get(1);

    std::set<uint32_t> blocked_channels;
    dvr.blocklist_.get(blocked_channels);

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      if (yae::has(blocked_channels, ch_num))
      {
        continue;
      }

      const yae::mpeg_ts::EPG::Channel & channel = i->second;
      std::list<yae::mpeg_ts::EPG::Program> programs = channel.programs_;

      // handle the situation when there is no EPG for the current bucket:
      uint32_t gps_time = uint32_t(TTime::gps_now().get(1));
      if (!yae::find_program(programs, gps_time))
      {
        programs.push_back(yae::mpeg_ts::EPG::Program());
        yae::mpeg_ts::EPG::Program & prog = programs.back();
        prog.title_ = yae::strfmt("Program Guide Not Available",
                                  int(channel.major_),
                                  int(channel.minor_));

        uint32_t bucket_index = (gps_time / 10800);
        prog.gps_time_ = bucket_index * 10800;
        prog.duration_ = 10800;
        yae::gps_time_to_localtime(prog.gps_time_, prog.tm_);
      }

      for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
             j = programs.begin(); j != programs.end(); ++j)
      {
        const yae::mpeg_ts::EPG::Program & program = *j;

        uint32_t gps_t1 = program.gps_time_ + program.duration_;
        if (gps_t1 <= gps_now)
        {
          // it's in the past:
          continue;
        }

        yae::shared_ptr<Wishlist::Item> want =
          dvr.explicitly_scheduled(channel, program);

        Recording::MadeBy rec_cause =
          want ? Recording::kExplicitlyScheduled : Recording::kUnspecified;

        if (!want && live_ch_ && *live_ch_ == ch_num)
        {
          want.reset(new Wishlist::Item());
          rec_cause = Recording::kLiveChannel;
        }

        if (!want)
        {
          want = dvr.wishlist_.matches(channel, program);
          rec_cause = Recording::kWishlistItem;
        }

        if (!want)
        {
          continue;
        }

        if (want->do_not_record())
        {
          yae_ilog("skipping, don't want to record: %s: %s",
                   channel.name_.c_str(),
                   program.description_.c_str());
          continue;
        }

        if (want->skip_duplicates())
        {
          TRecPtr rec_ptr = dvr.already_recorded(channel, program);
          if (rec_ptr)
          {
            // this program has already been recorded:
            yae_ilog("skipping, already recorded: %s, %s",
                     rec_ptr->get_basename().c_str(),
                     program.description_.c_str());
            continue;
          }
        }

        boost::unique_lock<boost::mutex> lock(mutex_);
        TRecordingPtr & recording_ptr = recordings_[ch_num][program.gps_time_];

        if (!recording_ptr)
        {
          recording_ptr.reset(new Recording());
        }

        Recording & recording = *recording_ptr;
        bool is_cancelled = recording.is_cancelled();

        uint16_t max_recordings = want->max_recordings();
        TRecPtr rec_ptr(new Recording::Rec(channel,
                                           program,
                                           rec_cause,
                                           max_recordings));
        rec_ptr->cancelled_ = is_cancelled;
        recording.set_rec(rec_ptr);
      }
    }

    // remove past recordings from schedule:
    std::map<uint32_t, TScheduledRecordings> current_schedule;
    std::map<uint32_t, TScheduledRecordings> updated_schedule;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      current_schedule = recordings_;
    }

    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = current_schedule.begin(); i != current_schedule.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      if (yae::has(blocked_channels, ch_num))
      {
        continue;
      }

      std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
        ch_found = epg.channels_.find(ch_num);
      if (ch_found == epg.channels_.end())
      {
        continue;
      }

      const TScheduledRecordings & schedule = i->second;
      for (TScheduledRecordings::const_iterator
             j = schedule.begin(); j != schedule.end(); ++j)
      {
        const uint32_t gps_t0 = j->first;
        const TRecordingPtr & recording_ptr = j->second;
        const Recording & recording = *recording_ptr;
        const TRecPtr rec_ptr = recording.get_rec();
        const Recording::Rec & rec = *rec_ptr;

        if (rec.gps_t1_ < gps_now)
        {
          // it's in the past:
          continue;
        }

        if (rec.made_by_ == Recording::kLiveChannel &&
            !(live_ch_ && *live_ch_ == ch_num))
        {
          // schedule for a different live channel:
          continue;
        }

        // confirm the recording is still wanted:
        yae::mpeg_ts::EPG::Channel channel = rec.to_epg_channel();
        yae::mpeg_ts::EPG::Program program = rec.to_epg_program();

        if (rec.made_by_ != Recording::kLiveChannel &&
            !dvr.explicitly_scheduled(channel, program))
        {
          yae::shared_ptr<Wishlist::Item> wanted =
            dvr.wishlist_.matches(channel, program);
          if (!wanted)
          {
            yae_dlog("skipping cancelled recording: %s: %s",
                     channel.name_.c_str(),
                     program.title_.c_str());
            continue;
          }

          if (wanted->do_not_record())
          {
            yae_ilog("cancelling, don't want to record: %s: %s",
                     channel.name_.c_str(),
                     program.description_.c_str());
            continue;
          }

          if (wanted->skip_duplicates())
          {
            TRecPtr found = dvr.already_recorded(channel, program);
            if (found)
            {
              // check when the recording was made:
              const Recording::Rec & prev = *found;
              if (prev.gps_t0_ < rec.gps_t0_)
              {
                // this program has already been recorded:
                yae_ilog("cancelling duplicate recording: %s, %s",
                         found->get_basename().c_str(),
                         program.description_.c_str());
                continue;
              }
            }
          }
        }

        // keep it:
        updated_schedule[ch_num][gps_t0] = recording_ptr;
      }
    }

    // update the schedule:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      recordings_.swap(updated_schedule);
    }
  }

  //----------------------------------------------------------------
  // Schedule::get
  //
  void
  Schedule::get(std::map<uint32_t, TScheduledRecordings> & recordings) const
  {
    YAE_BENCHMARK(probe, "Schedule::get");
    boost::unique_lock<boost::mutex> lock(mutex_);
    recordings = recordings_;
  }

  //----------------------------------------------------------------
  // Schedule::get
  //
  TRecordingPtr
  Schedule::get(uint32_t ch_num, uint32_t gps_time) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return yae::next<Recording>(recordings_, ch_num, gps_time);
  }

  //----------------------------------------------------------------
  // Schedule::get
  //
  void
  Schedule::get(std::set<TRecordingPtr> & recordings,
                uint32_t ch_num,
                uint32_t gps_time,
                uint32_t margin_sec) const
  {
    TRecordingPtr leading = get(ch_num, gps_time + margin_sec);
    TRecordingPtr trailing = get(ch_num, gps_time - margin_sec);

    if (leading && leading->is_cancelled())
    {
      leading.reset();
    }

    if (trailing && trailing->is_cancelled())
    {
      trailing.reset();
    }

    if (leading && trailing && leading != trailing)
    {
      recordings.insert(leading);
      recordings.insert(trailing);
    }
    else if (leading)
    {
      recordings.insert(leading);
    }
    else if (trailing)
    {
      recordings.insert(trailing);
    }
  }

  //----------------------------------------------------------------
  // Schedule::toggle
  //
  bool
  Schedule::toggle(uint32_t ch_num, uint32_t gps_time)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    std::map<uint32_t, TScheduledRecordings>::iterator
      found_sched = recordings_.find(ch_num);
    if (found_sched == recordings_.end())
    {
      return false;
    }

    TScheduledRecordings & schedule = found_sched->second;
    TScheduledRecordings::iterator found_rec = schedule.find(gps_time);
    if (found_rec == schedule.end())
    {
      return false;
    }

    Recording & recording = *(found_rec->second);
    TRecPtr rec_ptr(new Recording::Rec(*recording.get_rec()));
    Recording::Rec & rec = *rec_ptr;
    rec.cancelled_ = !(rec.cancelled_);
    recording.set_rec(rec_ptr);

    yae_ilog("%s wishlist recording: %s",
             rec.cancelled_ ? "cancel" : "schedule",
             rec.get_basename().c_str());
    return true;
  }

  //----------------------------------------------------------------
  // Schedule::remove
  //
  void
  Schedule::remove(uint32_t ch_num, uint32_t gps_time)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    std::map<uint32_t, TScheduledRecordings>::iterator
      found_sched = recordings_.find(ch_num);
    if (found_sched == recordings_.end())
    {
      return;
    }

    TScheduledRecordings & schedule = found_sched->second;
    TScheduledRecordings::iterator found_rec = schedule.find(gps_time);
    if (found_rec == schedule.end())
    {
      return;
    }

    Recording & recording = *(found_rec->second);
    TRecPtr rec_ptr(new Recording::Rec(*recording.get_rec()));
    rec_ptr->cancelled_ = true;
    recording.set_rec(rec_ptr);

    schedule.erase(found_rec);

    if (schedule.empty())
    {
      recordings_.erase(found_sched);
    }
  }

  //----------------------------------------------------------------
  // Schedule::save
  //
  void
  Schedule::save(Json::Value & json) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    Json::Value & recordings = json["recordings"];
    recordings = Json::Value(Json::objectValue);

    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = recordings_.begin(); i != recordings_.end(); ++i)
    {
      std::string ch_num = boost::lexical_cast<std::string>(i->first);
      const TScheduledRecordings & scheduled = i->second;

      Json::Value & channel = json[ch_num];
      channel = Json::Value(Json::objectValue);

      for (TScheduledRecordings::const_iterator
             j = scheduled.begin(); j != scheduled.end(); ++j)
      {
        std::string gps_start = boost::lexical_cast<std::string>(j->first);
        const TRecordingPtr & rec_ptr = j->second;

        // skip explicitly scheduled recordings and live recordings:
        if (rec_ptr && rec_ptr->made_by_wishlist())
        {
          yae::save(channel[gps_start], *rec_ptr);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // Schedule::load
  //
  void
  Schedule::load(const Json::Value & json)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    yae::load(json["recordings"], recordings_);
  }

  //----------------------------------------------------------------
  // Schedule::clear
  //
  void
  Schedule::clear()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    recordings_.clear();
  }


  //----------------------------------------------------------------
  // ParseStream
  //
  struct ParseStream : yae::Worker::Task
  {
    ParseStream(DVR::PacketHandler & packet_handler);
    ~ParseStream();

    // virtual:
    void execute(const yae::Worker & worker);

    DVR::PacketHandler & packet_handler_;

#ifdef __APPLE__
    yae::PreventAppNap prevent_app_nap_;
#endif
  };

  //----------------------------------------------------------------
  // ParseStream::ParseStream
  //
  ParseStream::ParseStream(DVR::PacketHandler & packet_handler):
    packet_handler_(packet_handler)
  {}

  //----------------------------------------------------------------
  // ParseStream::~ParseStream
  //
  ParseStream::~ParseStream()
  {
    yae::mpeg_ts::Context & ctx = packet_handler_.ctx_;
    ctx.clear_buffers();
  }

  //----------------------------------------------------------------
  // ParseStream::execute
  //
  void
  ParseStream::execute(const yae::Worker & worker)
  {
    (void)worker;

    DVR::PacketHandler::TSessionPtr ph_session = packet_handler_.session_;
    yae::RingBuffer & ring_buffer = ph_session->ring_buffer_;
    yae::mpeg_ts::Context & ctx = packet_handler_.ctx_;
    yae::Data data(188 * 4096);

    while (true)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (yae::Worker::Task::cancelled_)
      {
        return;
      }

      data.resize(188 * 4096);
      std::size_t size = ring_buffer.pull(data.get(), data.size());

      if (!size)
      {
        if (!ring_buffer.is_open())
        {
          break;
        }

        continue;
      }

      data.truncate(size);

      // parse the transport stream:
      yae::Bitstream bitstream(data);
      while (!bitstream.exhausted())
      {
        try
        {
          TBufferPtr pkt_data = bitstream.read_bytes(188);
          yae::Bitstream bin(pkt_data);

          yae::mpeg_ts::TSPacket pkt;
          pkt.load(bin);

          std::size_t end_pos = bin.position();
          std::size_t bytes_consumed = end_pos >> 3;

          if (bytes_consumed != 188)
          {
#ifndef NDEBUG
            yae_wlog("%sTSPacket too short (%i bytes), %s ...",
                     ctx.log_prefix_.c_str(),
                     bytes_consumed,
                     yae::to_hex(pkt_data->get(), 32, 4).c_str());
#endif
            continue;
          }

          if (pkt.is_null_packet())
          {
            continue;
          }

          ctx.push(pkt);

          yae::mpeg_ts::IPacketHandler::Packet packet(pkt.pid_, pkt_data);
          ctx.handle(packet, packet_handler_);
        }
        catch (const std::exception & e)
        {
#ifndef NDEBUG
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("%sfailed to parse %s: %s",
                   ctx.log_prefix_.c_str(),
                   data_hex.c_str(),
                   e.what());
#else
          (void)e;
#endif
        }
        catch (...)
        {
#ifndef NDEBUG
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("%sfailed to parse %s: unexpected exception",
                   ctx.log_prefix_.c_str(),
                   data_hex.c_str());
#endif
        }
      }

      // check if Channel Guide extends to 9 hours from now
      {
        static const TTime nine_hours(9 * 60 * 60, 1);
        int64_t t = (TTime::now() + nine_hours).get(1);
        if (ctx.channel_guide_overlaps(t))
        {
          packet_handler_.epg_ready_.notify_all();
        }
      }
    }
  }


  //----------------------------------------------------------------
  // DVR::PacketHandler::PacketHandler
  //
  DVR::PacketHandler::PacketHandler(DVR & dvr, const std::string & frequency):
    dvr_(dvr),
    ctx_(frequency),
    recordings_update_gps_time_(0)
  {
    worker_.set_queue_size_limit(1);
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::~PacketHandler
  //
  DVR::PacketHandler::~PacketHandler()
  {
    session_.reset();
    worker_.stop();
    worker_.wait_until_finished();
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::handle
  //
  void
  DVR::PacketHandler::handle(const yae::mpeg_ts::IPacketHandler::Packet & pkt,
                             const yae::mpeg_ts::Bucket & bucket,
                             uint32_t gps_time)
  {
    // keep alive:
    PacketHandler::TSessionPtr ph_session = session_;
    if (!ph_session)
    {
      return;
    }

    // shortcut:
    yae::fifo<Packet> & packets = ph_session->packets_;
    packets.push(pkt);

    if (bucket.guide_.empty() ||
        (!packets.full() && !bucket.vct_table_set_.is_complete()))
    {
      return;
    }

    // consume the backlog:
    handle_backlog(bucket, gps_time);
  }

  //----------------------------------------------------------------
  // write
  //
  static void
  write(const DVR & dvr,
        const std::set<TRecordingPtr> & recs,
        const yae::Data & data)
  {
    for (std::set<TRecordingPtr>::const_iterator
           i = recs.begin(); i != recs.end(); ++i)
    {
      Recording & recording = *(*i);

      if (!recording.has_writer())
      {
        yae::shared_ptr<Recording::Rec> rec_ptr = recording.get_rec();
        Recording::Rec & rec = *rec_ptr;
        uint32_t num_sec = rec.get_duration();
        yae::make_room_for(dvr.basedir_, rec, num_sec);
      }

      recording.write(dvr.basedir_, data);
    }
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::handle_backlog
  //
  void
  DVR::PacketHandler::handle_backlog(const yae::mpeg_ts::Bucket & bucket,
                                     uint32_t gps_time)
  {
    static const double prng_max =
      std::numeric_limits<boost::random::mt11213b::result_type>::max();

    boost::unique_lock<boost::mutex> lock(mutex_);
    if (recordings_update_gps_time_ < gps_time)
    {
      YAE_TIMESHEET_PROBE(probe, ctx_.timesheet_,
                          "PacketHandler::handle_backlog",
                          "dvr_.schedule_.get");

      // wait 8..15s before re-caching scheduled recordings:
      uint32_t r = uint32_t(8.0 * (double(prng_()) / prng_max));
      recordings_update_gps_time_ = gps_time + 8 + r;
      recordings_.clear();

      uint32_t margin = dvr_.margin_.get(1);
      for (std::map<uint32_t, yae::mpeg_ts::ChannelGuide>::const_iterator
             i = bucket.guide_.begin(); i != bucket.guide_.end(); ++i)
      {
        const uint32_t ch_num = i->first;
        std::set<TRecordingPtr> recs;
        dvr_.schedule_.get(recs, ch_num, gps_time, margin);
        if (!recs.empty())
        {
          recordings_[ch_num].swap(recs);
        }
      }
    }

    // keep alive:
    PacketHandler::TSessionPtr ph_session = session_;
    if (!ph_session)
    {
      return;
    }

    // shortcut:
    yae::fifo<Packet> & packets = ph_session->packets_;

    YAE_TIMESHEET_PROBE(probe, ctx_.timesheet_,
                        "PacketHandler::handle_backlog", "write");

    yae::mpeg_ts::IPacketHandler::Packet pkt;
    while (packets.pop(pkt))
    {
      const yae::Data & data = pkt.data_;

      std::map<uint16_t, uint32_t>::const_iterator found =
        bucket.pid_to_ch_num_.find(pkt.pid_);

      if (found == bucket.pid_to_ch_num_.end())
      {
        for (std::map<uint32_t, std::set<TRecordingPtr> >::const_iterator
               it = recordings_.begin(); it != recordings_.end(); ++it)
        {
          const std::set<TRecordingPtr> & recs = it->second;
          write(dvr_, recs, data);
        }
      }
      else
      {
        const uint32_t ch_num = found->second;
        std::map<uint32_t, std::set<TRecordingPtr> >::const_iterator
          it = recordings_.find(ch_num);
        if (it != recordings_.end())
        {
          const std::set<TRecordingPtr> & recs = it->second;
          write(dvr_, recs, data);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::refresh_cached_recordings
  //
  void
  DVR::PacketHandler::refresh_cached_recordings()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    recordings_update_gps_time_ = 0;
  }


  //----------------------------------------------------------------
  // CaptureStream
  //
  struct CaptureStream : yae::Worker::Task
  {
    CaptureStream(const DVR::TStreamPtr & stream_ptr);

    // virtual:
    void execute(const yae::Worker & worker);
    void cancel();

    yae::weak_ptr<IStream> stream_;
  };

  //----------------------------------------------------------------
  // CaptureStream::CaptureStream
  //
  CaptureStream::CaptureStream(const DVR::TStreamPtr & stream_ptr):
    stream_(stream_ptr)
  {}

  //----------------------------------------------------------------
  // CaptureStream::execute
  //
  void
  CaptureStream::execute(const yae::Worker & worker)
  {
    (void)worker;

    DVR::TStreamPtr stream_ptr = stream_.lock();
    if (stream_ptr)
    {
      HDHomeRun::TSessionPtr session_ptr = stream_ptr->session_;

      YAE_EXPECT(session_ptr);
      if (!session_ptr)
      {
        return;
      }

      HDHomeRun & hdhr = stream_ptr->dvr_.hdhr_;
      std::string frequency = stream_ptr->frequency_;
      stream_ptr.reset();
      hdhr.capture(session_ptr, stream_, frequency);
    }
  }

  //----------------------------------------------------------------
  // CaptureStream::cancel
  //
  void
  CaptureStream::cancel()
  {
    yae::Worker::Task::cancel();

    boost::unique_lock<boost::mutex> lock(mutex_);
    DVR::TStreamPtr stream_ptr = stream_.lock();
    if (stream_ptr)
    {
      stream_ptr->close();
    }
  }

  //----------------------------------------------------------------
  // DVR::Stream::Stream
  //
  DVR::Stream::Stream(DVR & dvr,
                      const yae::HDHomeRun::TSessionPtr & session_ptr,
                      const std::string & frequency):
    dvr_(dvr),
    session_(session_ptr),
    frequency_(frequency)
  {
    packet_handler_ = dvr_.packet_handler_[frequency_];

    if (!packet_handler_)
    {
      packet_handler_.reset(new PacketHandler(dvr_, frequency));
      dvr_.packet_handler_[frequency] = packet_handler_;
    }

    PacketHandler & packet_handler = *packet_handler_;
    packet_handler.session_.reset(new PacketHandler::Session());

    if (session_)
    {
      std::ostringstream oss;
      std::string tuner_name = session_->tuner_name();
      oss << tuner_name << " " << frequency << " Hz";

      std::string channels = dvr_.get_channels_str(frequency_);
      if (!channels.empty())
      {
        oss << ", channels: " << channels;
      }

      oss << "; ";

      packet_handler.ctx_.log_prefix_ = oss.str().c_str();
      yae_ilog("%p stream start: %s",
               this,
               packet_handler.ctx_.log_prefix_.c_str());
    }
  }

  //----------------------------------------------------------------
  // DVR::Stream::~Stream
  //
  DVR::Stream::~Stream()
  {
    close();
  }

  //----------------------------------------------------------------
  // DVR::Stream::open
  //
  void
  DVR::Stream::open(const DVR::TStreamPtr & stream_ptr,
                    const yae::TWorkerPtr & worker_ptr)
  {
    yae::shared_ptr<CaptureStream, yae::Worker::Task> task;
    task.reset(new CaptureStream(stream_ptr));

    worker_ = worker_ptr;
    worker_->add(task);
    worker_->start();
  }

  //----------------------------------------------------------------
  // DVR::Stream::close
  //
  void
  DVR::Stream::close()
  {
    PacketHandler & packet_handler = *packet_handler_;

    yae_ilog("%p stream stop: %s",
             this,
             packet_handler.ctx_.log_prefix_.c_str());
    PacketHandler::TSessionPtr ph_session = packet_handler.session_;
    if (ph_session)
    {
      ph_session->ring_buffer_.close();
      packet_handler.session_.reset();
      ph_session.reset();
    }

    // it's as ready as it's going to be:
    packet_handler.epg_ready_.notify_all();

    // get rid of the session so we don't end up with a stale tuner lock:
    session_.reset();
  }

  //----------------------------------------------------------------
  // DVR::Stream::is_open
  //
  bool
  DVR::Stream::is_open() const
  {
    const PacketHandler & packet_handler = *packet_handler_;
    PacketHandler::TSessionPtr ph_session = packet_handler.session_;
    if (!(ph_session && ph_session->ring_buffer_.is_open()))
    {
      return false;
    }

    return worker_ ? worker_->is_busy() : false;
  }

  //----------------------------------------------------------------
  // DVR::Stream::push
  //
  bool
  DVR::Stream::push(const void * data, std::size_t size)
  {
    YAE_ASSERT(data && size);
    if (!(data && size))
    {
      // nothing to do:
      return true;
    }

    if (!this->is_open())
    {
      return false;
    }

    PacketHandler & packet_handler = *packet_handler_;
    PacketHandler::TSessionPtr ph_session = packet_handler.session_;
    yae::RingBuffer & ring_buffer = ph_session->ring_buffer_;
    yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
    YAE_TIMESHEET_PROBE_TOO_SLOW(probe1, ctx.timesheet_,
                                 "DVR::Stream", "push",
                                 TTime(30, 1000));

#if 0
    std::string data_hex =
      yae::to_hex(data, std::min<std::size_t>(size, 32), 4);

    yae_dlog("%s%5i %s...",
             packet_handler.ctx_.log_prefix_.c_str(),
             int(size),
             data_hex.c_str());
#endif

    double ring_buffer_occupancy = ring_buffer.occupancy();
    if (ring_buffer_occupancy > 0.7)
    {
      yae_wlog("%sring buffer occupancy: %f",
               ctx.log_prefix_.c_str(),
               ring_buffer_occupancy);
    }

    if (packet_handler.worker_.is_idle())
    {
      yae::shared_ptr<ParseStream, yae::Worker::Task> task;
      task.reset(new ParseStream(packet_handler));
      packet_handler.worker_.add(task);
    }

    if (ring_buffer.push(data, size) != size)
    {
      return false;
    }

    return true;
  }


  //----------------------------------------------------------------
  // DVR::ServiceLoop::ServiceLoop
  //
  DVR::ServiceLoop::ServiceLoop(DVR & dvr):
    dvr_(dvr)
  {}

  //----------------------------------------------------------------
  // DVR::ServiceLoop::execute
  //
  void
  DVR::ServiceLoop::execute(const yae::Worker & worker)
  {
    (void)worker;
    dvr_.init_packet_handlers();

    TTime now = TTime::now().rebased(1);
    dvr_.set_next_channel_scan(now);
    dvr_.set_next_epg_refresh(now);
    dvr_.set_next_schedule_refresh(now);
    dvr_.set_next_storage_cleanup(now);
    dvr_.set_next_log_cleanup(now + dvr_.log_cleanup_period_);
    dvr_.set_next_heartbeat(now);

    yae::mpeg_ts::EPG epg;
    dvr_.get_epg(epg);
    dvr_.cache_epg(epg);
    dvr_.update_epg();

    // pull EPG, evaluate wishlist, start captures, etc...
    while (!keep_going_.stop_)
    {
      now = TTime::now().rebased(1);

      if (dvr_.next_log_cleanup() <= now)
      {
        dvr_.set_next_log_cleanup(now + dvr_.log_cleanup_period_);
        dvr_.cleanup_logs();
      }

      if (dvr_.load_wishlist())
      {
        dvr_.save_schedule();
      }

      if (dvr_.next_heartbeat() <= now)
      {
        dvr_.set_next_heartbeat(now + dvr_.heartbeat_period_);
        dvr_.save_heartbeat();
      }

      if (dvr_.next_schedule_refresh() <= now)
      {
        dvr_.set_next_schedule_refresh(now + dvr_.schedule_refresh_period_);

        yae::mpeg_ts::EPG curr_epg;
        dvr_.get_epg(curr_epg);

        bool epg_changed = (epg.channels_ != curr_epg.channels_);
        if (epg_changed)
        {
          epg.channels_.swap(curr_epg.channels_);
          dvr_.cache_epg(epg);
        }

#if 0 // ndef NDEBUG
        for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
               i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
        {
          const yae::mpeg_ts::EPG::Channel & channel = i->second;
          std::string fn = strfmt("epg-%02i.%02i.json",
                                  channel.major_,
                                  channel.minor_);
          Json::Value json;
          yae::mpeg_ts::save(json, channel);
          yae::TOpenFile((dvr_.yaetv_ / fn).string(), "wb").save(json);
        }
#endif

        dvr_.evaluate(epg);
      }

      if (dvr_.worker_.is_idle())
      {
        bool blocklist_changed = dvr_.load_blocklist();

        if (blocklist_changed || dvr_.next_epg_refresh() <= now)
        {
          dvr_.set_next_epg_refresh(now + dvr_.schedule_refresh_period_ * 2.0);
          dvr_.update_epg();
        }

        std::set<std::string> enabled_tuners;
        bool has_enabled_tuners = dvr_.discover_enabled_tuners(enabled_tuners);

        if (has_enabled_tuners &&
            dvr_.next_channel_scan() <= now)
        {
          dvr_.set_next_channel_scan(now + dvr_.channel_scan_period_);
          dvr_.scan_channels();
        }

        if (has_enabled_tuners &&
            dvr_.next_storage_cleanup() <= now)
        {
          dvr_.set_next_storage_cleanup(now + dvr_.storage_cleanup_period_);
          dvr_.cleanup_storage();
        }
      }

      try
      {
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
      }
      catch (...)
      {}
    }
  }

  //----------------------------------------------------------------
  // DVR::ServiceLoop::cancel
  //
  void
  DVR::ServiceLoop::cancel()
  {
    yae::Worker::Task::cancel();
    keep_going_.stop_ = true;
  }


  //----------------------------------------------------------------
  // DVR::DVR
  //
  DVR::DVR(const std::string & yaetv_dir,
           const std::string & basedir):
    hdhr_(yaetv_dir),
    worker_(0, 1, "DVR"),
    yaetv_(yaetv_dir),
    basedir_(basedir.empty() ? yae::get_temp_dir_utf8() : basedir),
    heartbeat_period_(5, 1),
    channel_scan_period_(24 * 60 * 60, 1),
    epg_refresh_period_(30 * 60, 1),
    schedule_refresh_period_(30, 1),
    storage_cleanup_period_(300, 1),
    log_cleanup_period_(24 * 60 * 60, 1),
    margin_(60, 1)
  {
    YAE_THROW_IF(!yae::mkdir_p(yaetv_.string()));


    // load or generate a UUID for this DVR instance:
    {
      Json::Value json;
      std::string uuid_path = (yaetv_ / "uuid.json").string();
      if (yae::TOpenFile(uuid_path, "rb").load(json))
      {
        local_uuid_ = json["uuid"].asString();
        YAE_ASSERT(!local_uuid_.empty());
      }

      if (local_uuid_.empty())
      {
        local_uuid_ = yae::generate_uuid();
        json["uuid"] = local_uuid_;

        if (!yae::TOpenFile(uuid_path, "wb").save(json))
        {
          local_uuid_.clear();
        }
      }
    }

    restart(basedir_.string());
  }

  //----------------------------------------------------------------
  // DVR::restart
  //
  void
  DVR::restart(const std::string & basedir)
  {
    shutdown();

    cleanup_yaetv_dir();

    // load preferences:
    {
      std::string path = (fs::path(yaetv_) / "settings.json").string();
      yae::TOpenFile(path, "rb").load(preferences_);
    }

    // load the tuner cache:
    {
      std::string path = (yaetv_ / "tuners.json").string();
      yae::TOpenFile(path, "rb").load(tuner_cache_);
      update_channel_frequency_luts();
    }

    // load the blocklist:
    load_blocklist();

    // load the wishlist:
    load_wishlist();

    // load the schedule:
    load_schedule();

    basedir_ = basedir.empty() ? yae::get_temp_dir_utf8() : basedir;
    yae_ilog("DVR start, recordings storage: %s", basedir.c_str());
    if (!yae::mkdir_p(basedir_.string()))
    {
      yae_elog("mkdir failed for %s", basedir_.string().c_str());
    }

    uint64_t filesystem_bytes = 0;
    uint64_t filesystem_bytes_free = 0;
    uint64_t available_bytes = 0;

    if (yae::stat_diskspace(basedir_.string().c_str(),
                            filesystem_bytes,
                            filesystem_bytes_free,
                            available_bytes))
    {
      yae_ilog("will write to %s: "
               "%" PRIu64 " GB total, "
               "%" PRIu64 " GB free, "
               "%" PRIu64 " GB available",
               basedir_.string().c_str(),
               filesystem_bytes / 1000000000,
               filesystem_bytes_free / 1000000000,
               available_bytes / 1000000000);
    }
    else
    {
      yae_elog("failed to query available disk space: %s",
               basedir_.string().c_str());
    }

    std::map<std::string, std::string> recordings;
    {
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(basedir_.string(), collect_recordings);
    }

    uint64_t recordings_bytes = 0;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & name = i->first;
      const std::string & path = i->second;
      uint64_t num_bytes = yae::stat_filesize(path.c_str());
      recordings_bytes += num_bytes;

      yae_ilog("recorded %.3f GB, %s",
               double(num_bytes) / 1000000000.0,
               name.c_str());
    }

    yae_ilog("total size of recordings: %.3F GB",
             double(recordings_bytes) / 1000000000.0);

    // (re)start the service loop:
    worker_.start();

    if (!service_loop_worker_)
    {
      service_loop_worker_.reset(new yae::Worker());
    }

    TWorkerPtr service_loop_worker_ptr = service_loop_worker_;
    Worker & service_loop_worker = *service_loop_worker_ptr;
    yae::shared_ptr<DVR::ServiceLoop, yae::Worker::Task> task;
    task.reset(new DVR::ServiceLoop(*this));
    service_loop_worker.add(task);
  }

  //----------------------------------------------------------------
  // DVR::~DVR
  //
  DVR::~DVR()
  {
    shutdown();
  }

  //----------------------------------------------------------------
  // DVR::cleanup_yaetv_dir
  //
  void
  DVR::cleanup_yaetv_dir()
  {
    // cleanup logs and expired/cancelled explicitly scheduled items:
    cleanup_yaetv_logs(yaetv_.string());
    cleanup_explicitly_scheduled_items();
  }

  //----------------------------------------------------------------
  // DVR::cleanup_explicitly_scheduled_items
  //
  void
  DVR::cleanup_explicitly_scheduled_items()
  {
    std::map<std::string, std::string> recs;
    {
      CollectFiles collect_files(recs, rec_sched_rx);
      for_each_file_at((basedir_ / ".yaetv").string(), collect_files);
    }

    // remove cancelled/expired explicitly scheduled items:
    int64_t now = TTime::now().get(1);

    for (std::map<std::string, std::string>::const_iterator
           i = recs.begin(); i != recs.end(); ++i)
    {
      const std::string & name = i->first;
      const std::string & path = i->second;

      const char * desc = "cancelled";
      if (!al::ends_with(name, ".cancelled"))
      {
        desc = "expired";

        Json::Value json;
        yae::TOpenFile file;
        if (!(file.open(path, "rb") && file.load(json)))
        {
          continue;
        }

        Wishlist::Item item;
        yae::load(json, item);
        file.close();

        const struct tm & date = *(item.date_);
        if (item.when_)
        {
          const Timespan & when = *(item.when_);

          int64_t t1 = localtime_to_unix_epoch_time(date) + when.dt().get(1);
          if (now < t1)
          {
            continue;
          }
        }
      }

      if (!yae::remove_utf8(path))
      {
        yae_wlog("failed to remove %s schedule: %s", desc, path.c_str());
      }
      else
      {
        yae_ilog("removed %s schedule: %s", desc, path.c_str());
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::init_packet_handlers
  //
  void
  DVR::init_packet_handlers()
  {
    // load the frequencies:
    std::map<std::string, yae::TChannels> frequencies;
    try
    {
      std::string path = (yaetv_ / "frequencies.json").string();
      Json::Value json;
      yae::TOpenFile(path, "rb").load(json);
      yae::load(json, frequencies);
    }
    catch (...)
    {}

    if (frequencies.empty())
    {
      // NOTE: this assumes hdhr_.discover_tuners(..), hdhr_.init(tuner)
      // has already happened:
      DVR::get_channels(frequencies);
    }

    // load the EPG:
    load_epg();
  }

  //----------------------------------------------------------------
  // DVR::shutdown
  //
  void
  DVR::shutdown()
  {
    yae_ilog("DVR shutdown");
    {
      TWorkerPtr service_loop_worker;
      std::swap(service_loop_worker, service_loop_worker_);

      if (service_loop_worker)
      {
        service_loop_worker->stop();
        service_loop_worker->wait_until_finished();
      }
    }

    worker_.stop();
    worker_.wait_until_finished();

    // clear the schedule:
    boost::unique_lock<boost::mutex> lock(mutex_);
    schedule_.clear();

    for (std::map<std::string, TPacketHandlerPtr>::const_iterator
           i = packet_handler_.begin(); i != packet_handler_.end(); ++i)
    {
      std::string frequency = i->first;
      TPacketHandlerPtr packet_handler_ptr = i->second;
      if (packet_handler_ptr)
      {
        PacketHandler & ph = *packet_handler_ptr;
        PacketHandler::TSessionPtr ph_session = ph.session_;
        if (ph_session)
        {
          ph_session->ring_buffer_.close();
        }

        ph.worker_.stop();
        ph.worker_.wait_until_finished();

#if YAE_TIMESHEET_ENABLED
        // save timesheet to disk:
        yae::mpeg_ts::Context & ctx = ph.ctx_;
        std::string timesheet = ctx.timesheet_.to_str();
        std::string fn = strfmt("timesheet-dvr.%s.log",
                                ctx.log_prefix_.c_str());
        fn = sanitize_filename_utf8(fn);
        fn = (fs::path(yae::get_temp_dir_utf8()) / fn).string();
        yae::TOpenFile(fn, "ab").write(timesheet);
#endif
      }

      TStreamPtr stream_ptr = stream_[frequency].lock();
      if (stream_ptr)
      {
        Stream & stream = *stream_ptr;
        stream.close();
      }

      TWorkerPtr worker_ptr = stream_worker_[frequency];
      if (worker_ptr)
      {
        worker_ptr->stop();
        worker_ptr->wait_until_finished();
      }
    }

    packet_handler_.clear();
    stream_worker_.clear();
    stream_.clear();
  }


  //----------------------------------------------------------------
  // save_epg
  //
  void
  save_epg(const DVR & dvr,
           const std::string & frequency,
           const yae::mpeg_ts::Context & ctx)
  {
    dvr.save_epg(frequency, ctx);
    dvr.save_frequencies();

    // ctx.dump();

#if 0
    // also store it to disk, to help with post-mortem debugging:
    yae::mpeg_ts::EPG epg;
    ctx.get_epg_now(epg);

    int64_t t = yae::TTime::now().get(1);
    t -= t % 1800; // round-down to half-hour:

    struct tm tm;
    yae::unix_epoch_time_to_localtime(t, tm);

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const yae::mpeg_ts::EPG::Channel & channel = i->second;
      std::string fn = strfmt("epg-%02i.%02i-%02u%02u.json",
                              channel.major_,
                              channel.minor_,
                              tm.tm_hour,
                              tm.tm_min);

      Json::Value json;
      yae::mpeg_ts::save(json, channel);
      yae::TOpenFile((dvr.yaetv_ / fn).string(), "wb").save(json);
    }
#endif
  }


  //----------------------------------------------------------------
  // ScanChannels
  //
  struct ScanChannels : yae::Worker::Task
  {
    ScanChannels(DVR & dvr);

    // helper:
    bool tune_and_scan(HDHomeRun::TSessionPtr session_ptr,
                       const TunerChannel & tuner_channel,
                       TunerStatus & tuner_status);
    // virtual:
    void execute(const yae::Worker & worker);
    void cancel();

    DVR & dvr_;
    DontStop keep_going_;
  };

  //----------------------------------------------------------------
  // ScanChannels::ScanChannels
  //
  ScanChannels::ScanChannels(DVR & dvr):
    dvr_(dvr)
  {}

  //----------------------------------------------------------------
  // ScanChannels::tune_and_scan
  //
  bool
  ScanChannels::tune_and_scan(HDHomeRun::TSessionPtr session_ptr,
                              const TunerChannel & tuner_channel,
                              TunerStatus & tuner_status)
  {
    std::string frequency = tuner_channel.frequency_str();
    DVR::TPacketHandlerPtr & handler_ptr = dvr_.packet_handler_[frequency];
    if (!handler_ptr)
    {
      handler_ptr.reset(new DVR::PacketHandler(dvr_, frequency));
    }

    const DVR::PacketHandler & packet_handler = *handler_ptr;
    const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
#if 0
    TTime elapsed_time_since_mgt = ctx.elapsed_time_since_mgt();
    YAE_ASSERT(elapsed_time_since_mgt.time_ >= 0);

    if (elapsed_time_since_mgt < dvr_.epg_refresh_period_)
    {
      yae_ilog("%s skipping channel scan update for %s",
               session_ptr->device_name().c_str(),
               frequency.c_str());
      return false;
    }
#endif
    if (!dvr_.hdhr_.tune_to(session_ptr,
                            tuner_channel.frequency_,
                            tuner_status))
    {
      yae_ilog("%s channel scan failed to tune to %u, "
               "signal present: %s, "
               "signal strength: %u, "
               "signal to noise quality: %u, "
               "signal error quality: %u",
               session_ptr->device_name().c_str(),
               tuner_channel.frequency_,
               tuner_status.signal_present_ ? "yes" : "no",
               tuner_status.signal_strength_,
               tuner_status.symbol_error_quality_,
               tuner_status.signal_to_noise_quality_);
      return true;
    }

    static const TTime sample_dur(30, 1);
    DVR::TStreamPtr stream_ptr = dvr_.capture_stream(session_ptr,
                                                     frequency,
                                                     sample_dur);
    if (!stream_ptr)
    {
      yae_wlog("%s channel scan failed for %s",
               session_ptr->device_name().c_str(),
               frequency.c_str());
      return false;
    }

    // wait until EPG is ready:
    DVR::Stream & stream = *stream_ptr;
    boost::system_time giveup_at(boost::get_system_time());
    giveup_at += boost::posix_time::seconds(sample_dur.get(1));

    yae_ilog("%sstarted channel scan for %s",
             packet_handler.ctx_.log_prefix_.c_str(),
             frequency.c_str());

    bool done = false;
    while (!done)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (yae::Worker::Task::cancelled_)
      {
        yae_wlog("%s channel scan cancelled for %s",
                 session_ptr->device_name().c_str(),
                 frequency.c_str());
        break;
      }

      try
      {
        if (packet_handler.epg_ready_.timed_wait(lock, giveup_at))
        {
          done = true;
          break;
        }
      }
      catch (...)
      {}

      boost::system_time now(boost::get_system_time());
      if (giveup_at <= now)
      {
        done = true;
        break;
      }
    }

    session_ptr->get_tuner_status(tuner_status);
    stream.close();
    stream.worker_->wait_until_finished();

    return done;
  }

  //----------------------------------------------------------------
  // update_tuner_cache
  //
  static void
  update_tuner_cache(Json::Value & cache,
                     const TunerStatus & tuner_status,
                     const yae::mpeg_ts::Context & ctx)
  {
    Json::Value & status = cache["status"];
    status["signal_present"] = tuner_status.signal_present_;
    status["signal_strength"] = tuner_status.signal_strength_;
    status["symbol_error_quality"] = tuner_status.symbol_error_quality_;
    status["signal_to_noise_quality"] =
      tuner_status.signal_to_noise_quality_;

    std::map<uint32_t, yae::mpeg_ts::EPG::Channel> channels;
    ctx.get_channels(channels);

    Json::Value & programs = cache["programs"];
    programs = Json::Value(Json::arrayValue);

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           x = channels.begin(); x != channels.end(); ++x)
    {
      const yae::mpeg_ts::EPG::Channel & channel = x->second;
      Json::Value p;
      p["major"] = channel.major_;
      p["minor"] = channel.minor_;
      p["name"] = channel.name_;

      if (!channel.description_.empty())
      {
        p["description"] = channel.description_;
      }

      programs.append(p);
    }
  }

  //----------------------------------------------------------------
  // ScanChannels::execute
  //
  void
  ScanChannels::execute(const yae::Worker & worker)
  {
    (void)worker;

    std::set<std::string> enabled_tuners;
    if (!dvr_.discover_enabled_tuners(enabled_tuners))
    {
      // there are no enabled tuners, nothing to do here:
      return;
    }

    std::string channelmap = dvr_.get_channelmap();
    std::list<TunerChannel> channels;
    dvr_.hdhr_.get_channel_list(channels, channelmap.c_str());

    std::list<TunerDevicePtr> devices;
    dvr_.hdhr_.discover_devices(devices);

    for (std::list<TunerDevicePtr>::const_iterator
           i = devices.begin(); i != devices.end(); ++i)
    {
      if (yae::Worker::Task::cancelled_)
      {
        return;
      }

      const TunerDevicePtr & device_ptr = *i;
      const TunerDevice & device = *device_ptr;

      Json::Value tuner_cache;
      dvr_.get_tuner_cache(device.name(), tuner_cache);
      bool rescan = false;

      int64_t timestamp = tuner_cache.get("timestamp", 0).asInt64();
      int64_t now = yae::TTime::now().get(1);
      int64_t elapsed = now - timestamp;
      int64_t threshold = 24 * 60 * 60;

      if (threshold < elapsed)
      {
        // cache is too old, purge it:
        yae_ilog("%s channel scan cache expired", device.name().c_str());
        rescan = true;
      }

      if (!rescan)
      {
        struct tm localtime;
        yae::unix_epoch_time_to_localtime(timestamp, localtime);
        std::string date = yae::to_yyyymmdd_hhmmss(localtime);

        yae_ilog("%s skipping channel scan, using cache from %s",
                 device.name().c_str(),
                 date.c_str());
        continue;
      }

      bool done = false;
      int num_tuners = device.num_tuners();
      for (int tuner = 0; !done && tuner < num_tuners; tuner++)
      {
        // check whether device is enabled in settings:
        std::string tuner_name = device.tuner_name(tuner);
        if (!yae::has(enabled_tuners, tuner_name))
        {
          continue;
        }

        bool exclusive_session = true;
        HDHomeRun::TSessionPtr session_ptr =
          dvr_.hdhr_.open_session(tuner_name, exclusive_session);
        if (!session_ptr)
        {
          continue;
        }

        for (std::list<TunerChannel>::const_iterator
               k = channels.begin(); !done && k != channels.end(); ++k)
        {
          // check for cancellation:
          if (yae::Worker::Task::cancelled_)
          {
            return;
          }

          const TunerChannel & tuner_channel = *k;
          std::string frequency = tuner_channel.frequency_str();

          if (dvr_.maybe_skip(frequency))
          {
            yae_dlog("%s skipping channel scan for %s",
                     device.name().c_str(),
                     frequency.c_str());
            continue;
          }

          Json::Value & cache = tuner_cache["frequencies"][frequency];
          int64_t timestamp = cache.get("timestamp", 0).asInt64();
          int64_t elapsed = now - timestamp;
          if (elapsed < threshold)
          {
            // use the cached value:
            struct tm localtime;
            yae::unix_epoch_time_to_localtime(timestamp, localtime);
            std::string date = yae::to_yyyymmdd_hhmmss(localtime);

            yae_ilog("%s skipping channel scan for %s, using cache from %s",
                     device.name().c_str(),
                     frequency.c_str(),
                     date.c_str());
            continue;
          }

          TunerStatus tuner_status;
          if (!tune_and_scan(session_ptr, tuner_channel, tuner_status))
          {
            if (!tuner_status.signal_present_)
            {
              cache["programs"].clear();
              cache["timestamp"] = (Json::Value::Int64)now;
              dvr_.update_tuner_cache(device.name(), tuner_cache);
              dvr_.no_signal(frequency);
            }
            continue;
          }

          DVR::TPacketHandlerPtr & handler_ptr =
            dvr_.packet_handler_[frequency];
          YAE_ASSERT(handler_ptr);

          const DVR::PacketHandler & packet_handler = *handler_ptr;
          const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;

          cache["programs"].clear();
          yae::update_tuner_cache(cache, tuner_status, ctx);

          cache["timestamp"] = (Json::Value::Int64)now;
          dvr_.update_tuner_cache(device.name(), tuner_cache);
          yae::save_epg(dvr_, frequency, ctx);
        }

        done = true;
      }

      if (done)
      {
        tuner_cache["timestamp"] = (Json::Value::Int64)now;
        dvr_.update_tuner_cache(device.name(), tuner_cache);
        dvr_.save_epg();
      }
    }
  }

  //----------------------------------------------------------------
  // ScanChannels::cancel
  //
  void
  ScanChannels::cancel()
  {
    yae::Worker::Task::cancel();
    keep_going_.stop_ = true;
  }

  //----------------------------------------------------------------
  // DVR::scan_channels
  //
  void
  DVR::scan_channels()
  {
    yae::shared_ptr<ScanChannels, yae::Worker::Task> task;
    task.reset(new ScanChannels(*this));
    worker_.add(task);
  }


  //----------------------------------------------------------------
  // UpdateProgramGuide
  //
  struct UpdateProgramGuide : yae::Worker::Task
  {
    DVR & dvr_;

    UpdateProgramGuide(DVR & dvr);

    // virtual:
    void execute(const yae::Worker & worker);
  };

  //----------------------------------------------------------------
  // UpdateProgramGuide::UpdateProgramGuide
  //
  UpdateProgramGuide::UpdateProgramGuide(DVR & dvr):
    dvr_(dvr)
  {}

  //----------------------------------------------------------------
  // UpdateProgramGuide::execute
  //
  void
  UpdateProgramGuide::execute(const yae::Worker & worker)
  {
    // unused:
    (void)worker;

    std::set<std::string> enabled_tuners;
    if (!dvr_.discover_enabled_tuners(enabled_tuners))
    {
      // there are no enabled tuners, use the remote EPG, if any:
      dvr_.load_epg();
      return;
    }

    static const TTime sample_dur(30, 1);
    std::map<uint32_t, std::string> channels;
    dvr_.get_channels(channels);

    for (std::map<uint32_t, std::string>::const_iterator
           i = channels.begin(); i != channels.end(); ++i)
    {
      if (yae::Worker::Task::cancelled_)
      {
        return;
      }

      // shortucts:
      const uint32_t ch_num = i->first;
      const std::string & frequency = i->second;

      if (dvr_.maybe_skip(frequency))
      {
        yae_dlog("skipping EPG update for %s", frequency.c_str());
        continue;
      }

      const uint16_t major = yae::mpeg_ts::channel_major(ch_num);
      const uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
      const std::string channels_str = dvr_.get_channels_str(frequency);

      DVR::TPacketHandlerPtr & handler_ptr = dvr_.packet_handler_[frequency];
      if (!handler_ptr)
      {
        handler_ptr.reset(new DVR::PacketHandler(dvr_, frequency));
      }

      const DVR::PacketHandler & packet_handler = *handler_ptr;
      const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;

      TTime elapsed_time_since_mgt = ctx.elapsed_time_since_mgt();
      YAE_ASSERT(elapsed_time_since_mgt.time_ >= 0);

      bool update_mgt = dvr_.epg_refresh_period_ <= elapsed_time_since_mgt;

      DVR::TStreamPtr stream_ptr =
        update_mgt ?
        dvr_.capture_stream(frequency, sample_dur) :
        dvr_.get_existing_stream(frequency);

      HDHomeRun::TSessionPtr session_ptr =
        stream_ptr ? stream_ptr->session_ : HDHomeRun::TSessionPtr();

      if (stream_ptr && session_ptr)
      {
        std::string device_name = session_ptr->device_name();
        Json::Value tuner_cache;
        dvr_.get_tuner_cache(device_name, tuner_cache);
        Json::Value & cache = tuner_cache["frequencies"][frequency];

        if (update_mgt)
        {
          // wait until EPG is ready:
          DVR::Stream & stream = *stream_ptr;
          boost::system_time giveup_at(boost::get_system_time());
          giveup_at += boost::posix_time::seconds(sample_dur.get(1));

          yae_ilog("%sstarted EPG update for %s, channels %s",
                   ctx.log_prefix_.c_str(),
                   frequency.c_str(),
                   channels_str.c_str());

          while (true)
          {
            boost::unique_lock<boost::mutex> lock(mutex_);
            if (yae::Worker::Task::cancelled_)
            {
              return;
            }

            try
            {
              if (packet_handler.epg_ready_.timed_wait(lock, giveup_at))
              {
                break;
              }
            }
            catch (...)
            {}

            boost::system_time now(boost::get_system_time());
            if (giveup_at <= now)
            {
              break;
            }
          }
        }
        else
        {
          yae_ilog("skipping EPG update for channels %i.* (%s)",
                   major,
                   frequency.c_str());
        }

        TunerStatus tuner_status;
        session_ptr->get_tuner_status(tuner_status);
        yae::update_tuner_cache(cache, tuner_status, ctx);

        int64_t now = yae::TTime::now().get(1);
        cache["timestamp"] = (Json::Value::Int64)now;
        dvr_.update_tuner_cache(device_name, tuner_cache);

        yae::save_epg(dvr_, frequency, ctx);
      }
      else if (update_mgt)
      {
        yae_wlog("failed to start EPG update for channels %i.* (%s)",
                 major,
                 frequency.c_str());
        continue;
      }
    }
  }


  //----------------------------------------------------------------
  // DVR::update_epg
  //
  void
  DVR::update_epg()
  {
    yae::shared_ptr<UpdateProgramGuide, yae::Worker::Task> task;
    task.reset(new UpdateProgramGuide(*this));
    worker_.add(task);
  }

  //----------------------------------------------------------------
  // StorageCleanup
  //
  struct StorageCleanup : yae::Worker::Task
  {
    StorageCleanup(DVR & dvr):
      dvr_(dvr)
    {}

    // virtual:
    void execute(const yae::Worker & worker)
    {
      std::set<std::string> enabled_tuners;
      if (!dvr_.discover_enabled_tuners(enabled_tuners))
      {
        // there are no enabled tuners, it's not up to us to clean up:
        return;
      }

      static const uint64_t min_free_bytes = 9000000000ull;
      yae_dlog("storage cleanup to free up %" PRIu64 " bytes", min_free_bytes);
      yae::make_room_for(dvr_.basedir_.string(), min_free_bytes);
    }

    DVR & dvr_;
  };

  //----------------------------------------------------------------
  // DVR::cleanup_storage
  //
  void
  DVR::cleanup_storage()
  {
    yae::shared_ptr<StorageCleanup, yae::Worker::Task> task;
    task.reset(new StorageCleanup(*this));
    worker_.add(task);
  }

  //----------------------------------------------------------------
  // DVR::cleanup_logs
  //
  void
  DVR::cleanup_logs()
  {
    yae::TLog & logger = yae::logger();

    std::string ts =
      unix_epoch_time_to_localtime_str(TTime::now().get(1), "", "-", "");

    fs::path log_path =
      yaetv_ / yae::strfmt("yaetv-%s.log", ts.c_str());

    logger.assign(std::string("yaetv"), new LogToFile(log_path.string()));

    cleanup_yaetv_logs(yaetv_.string());
  }

  //----------------------------------------------------------------
  // DVR::capture_stream
  //
  DVR::TStreamPtr
  DVR::capture_stream(const HDHomeRun::TSessionPtr & session_ptr,
                      const std::string & frequency,
                      const TTime & duration)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    TStreamPtr stream_ptr = stream_[frequency].lock();

    if (stream_ptr && stream_ptr->session_)
    {
      YAE_ASSERT(stream_ptr->session_ != session_ptr);

      // do not interfere with an existing session:
      return DVR::TStreamPtr();
    }

    stream_ptr.reset(new Stream(*this, session_ptr, frequency));
    Stream & stream = *stream_ptr;
    HDHomeRun::Session & session = *stream.session_;
    session.extend(TTime::now() + duration);

    TWorkerPtr & worker_ptr = stream_worker_[frequency];
    if (!worker_ptr)
    {
      worker_ptr.reset(new yae::Worker());
    }

    stream.open(stream_ptr, worker_ptr);

    return stream_ptr;
  }

  //----------------------------------------------------------------
  // DVR::capture_stream
  //
  DVR::TStreamPtr
  DVR::capture_stream(const std::string & frequency,
                      const TTime & duration)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    TStreamPtr stream_ptr = stream_[frequency].lock();

    if (!stream_ptr || !stream_ptr->session_)
    {
      // start a new session:
      std::set<std::string> enabled_tuners;
      if (!discover_enabled_tuners(enabled_tuners))
      {
        // there are no enabled tuners, nothing to do here:
        return TStreamPtr();
      }

      uint32_t freq_hz = boost::lexical_cast<uint32_t>(frequency);
      HDHomeRun::TSessionPtr session_ptr =
        hdhr_.open_session(enabled_tuners, freq_hz);
      if (!session_ptr)
      {
        // no tuners available:
        return DVR::TStreamPtr();
      }

      stream_ptr.reset(new Stream(*this, session_ptr, frequency));

      // keep track of streams, but don't extend their lifetime:
      stream_[frequency] = stream_ptr;
    }

    Stream & stream = *stream_ptr;
    HDHomeRun::Session & session = *stream.session_;

    if (session.exclusive())
    {
      // session sharing is not allowed:
      return DVR::TStreamPtr();
    }

    session.extend(TTime::now() + duration);

    if (!stream.is_open())
    {
      TWorkerPtr & worker_ptr = stream_worker_[frequency];
      if (!worker_ptr)
      {
        worker_ptr.reset(new yae::Worker());
      }

      stream.open(stream_ptr, worker_ptr);
    }

    return stream_ptr;
  }

  //----------------------------------------------------------------
  // DVR::get_existing_stream
  //
  DVR::TStreamPtr
  DVR::get_existing_stream(const std::string & frequency)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    TStreamPtr stream_ptr = stream_[frequency].lock();
    return stream_ptr;
  }

  //----------------------------------------------------------------
  // DVR::no_signal
  //
  void
  DVR::no_signal(const std::string & frequency)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    packet_handler_.erase(frequency);
    stream_worker_.erase(frequency);
    stream_.erase(frequency);
  }

  //----------------------------------------------------------------
  // DVR::get
  //
  void
  DVR::get(std::map<std::string, DVR::TPacketHandlerPtr> & ph) const
  {
    YAE_BENCHMARK(probe, "DVR::get packet handlers");
    boost::unique_lock<boost::mutex> lock(mutex_);
    ph = packet_handler_;
  }

  //----------------------------------------------------------------
  // DVR::get
  //
  void
  DVR::get(std::map<std::string, Wishlist::Item> & wishlist) const
  {
    YAE_BENCHMARK(probe, "DVR::get wishlist");
    wishlist_.get(wishlist);
  }

  //----------------------------------------------------------------
  // DVR::wishlist_remove
  //
  bool
  DVR::wishlist_remove(const std::string & wi_key)
  {
    // update the wishlist:
    if (!wishlist_.remove(wi_key))
    {
      return false;
    }

    save_wishlist();
    return true;
  }

  //----------------------------------------------------------------
  // DVR::wishlist_update
  //
  void
  DVR::wishlist_update(const std::string & wi_key,
                       const Wishlist::Item & new_item)
  {
    // update the wishlist:
    wishlist_.update(wi_key, new_item);
    save_wishlist();
  }

  //----------------------------------------------------------------
  // DVR::get_epg
  //
  void
  DVR::get_epg(yae::mpeg_ts::EPG & epg, const std::string & lang) const
  {
    YAE_BENCHMARK(probe, "DVR::get_epg");

    std::map<std::string, TPacketHandlerPtr> packet_handlers;
    get(packet_handlers);

    for (std::map<std::string, TPacketHandlerPtr>::const_iterator
           i = packet_handlers.begin(); i != packet_handlers.end(); ++i)
    {
      const TPacketHandlerPtr & packet_handler_ptr = i->second;
      if (packet_handler_ptr)
      {
        const PacketHandler & packet_handler = *packet_handler_ptr;
        packet_handler.ctx_.get_epg_now(epg, lang);
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::save
  //
  void
  DVR::save_epg(const std::string & frequency,
                const yae::mpeg_ts::Context & ctx) const
  {
    YAE_BENCHMARK(probe, "DVR::save_epg");

    Json::Value json;
    json["timestamp"] = Json::Int64(yae::TTime::now().get(1));
    ctx.save(json[frequency]);

    boost::unique_lock<boost::mutex> lock(mutex_);
    std::string epg_path =
      (basedir_ / ".yaetv" / ("epg-" + frequency + ".json")).string();
    YAE_ASSERT(yae::atomic_save(epg_path, json));
  }

  //----------------------------------------------------------------
  // DVR::save_epg
  //
  void
  DVR::save_epg() const
  {
    YAE_BENCHMARK(probe, "DVR::get_epg (all)");

    std::map<std::string, TPacketHandlerPtr> packet_handlers;
    get(packet_handlers);

    for (std::map<std::string, TPacketHandlerPtr>::const_iterator
           i = packet_handlers.begin(); i != packet_handlers.end(); ++i)
    {
      const std::string & frequency = i->first;
      const PacketHandler & packet_handler = *(i->second.get());
      save_epg(frequency, packet_handler.ctx_);
    }

    save_frequencies();
  }

  //----------------------------------------------------------------
  // DVR::save_frequencies
  //
  void
  DVR::save_frequencies() const
  {
    YAE_BENCHMARK(probe, "DVR::save_frequencies");

    std::map<std::string, yae::TChannels> frequencies;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      for (std::map<std::string, TPacketHandlerPtr>::const_iterator
             i = packet_handler_.begin(); i != packet_handler_.end(); ++i)
      {
        const std::string & frequency = i->first;
        yae::TChannels & channels = frequencies[frequency];
        DVR::get_channels(frequency, channels);
      }
    }

    Json::Value json;
    yae::save(json, frequencies);

    boost::unique_lock<boost::mutex> lock(mutex_);
    std::string freq_path = (yaetv_ / "frequencies.json").string();
    yae::TOpenFile freq_file;
    if (freq_file.open(freq_path, "wb"))
    {
      freq_file.save(json);
    }
  }

  //----------------------------------------------------------------
  // LogElapsedTime
  //
  struct LogElapsedTime
  {
    boost::chrono::steady_clock::time_point t0_;
    std::string where_;
    std::string what_;
    int priority_;

    LogElapsedTime(const std::string & where,
                   const std::string & what,
                   int priority = yae::TLog::kInfo):
      t0_(boost::chrono::steady_clock::now()),
      where_(where),
      what_(what),
      priority_(priority)
    {}

    ~LogElapsedTime()
    {
      boost::chrono::steady_clock::time_point
        t1 = boost::chrono::steady_clock::now();

      uint64 dt_usec =
        boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
        count();

      yae::log(priority_,
               where_.c_str(),
               "elapsed time: %" PRIu64 ".%06" PRIu64 "s, %s",
               dt_usec / 1000000,
               dt_usec % 1000000,
               what_.c_str());
    }

  protected:
    // intentionally disabled:
    LogElapsedTime(const LogElapsedTime &);
    LogElapsedTime & operator = (const LogElapsedTime &);
  };

//----------------------------------------------------------------
// YAE_LOG_ELAPSED_TIME
//
#define YAE_LOG_ELAPSED_TIME(varname, what) \
  yae::LogElapsedTime varname(__FILE__ ":" YAE_STR(__LINE__), \
                              what, yae::TLog::kInfo)

  //----------------------------------------------------------------
  // DVR::load_epg
  //
  void
  DVR::load_epg()
  {
    YAE_BENCHMARK(probe, "DVR::load_epg");

    std::map<std::string, std::string> epg_by_freq;
    {
      std::string epg_dir = (basedir_ / ".yaetv").string();
      CollectFiles collect_files(epg_by_freq, epg_by_freq_rx);
      for_each_file_at(epg_dir, collect_files);
    }

    for (std::map<std::string, std::string>::const_iterator
           i = epg_by_freq.begin(); i != epg_by_freq.end(); ++i)
    {
      if (!service_loop_worker_)
      {
        break;
      }

      const std::string & name = i->first;
      const std::string & path = i->second;
      std::string frequency = name.substr(4, name.size() - 9);
      YAE_LOG_ELAPSED_TIME(probe, path);

      Json::Value epg;
      if (yae::TOpenFile(path, "rb").load(epg))
      {
        TPacketHandlerPtr packet_handler_ptr;
        {
          boost::unique_lock<boost::mutex> lock(mutex_);
          packet_handler_ptr = packet_handler_[frequency];
          if (!packet_handler_ptr)
          {
            packet_handler_ptr.reset(new PacketHandler(*this, frequency));
            packet_handler_[frequency] = packet_handler_ptr;
          }
        }

        yae_ilog("loading EPG for % 9s Hz", frequency.c_str());
        PacketHandler & packet_handler = *packet_handler_ptr;
        try
        {
          packet_handler.ctx_.load(epg[frequency]);
        }
        catch (const std::exception & e)
        {
          yae_elog("DVR::load_epg %s, exception: %s", path.c_str(), e.what());
        }
        catch (...)
        {
          yae_elog("DVR::load_epg %s, unexpected exception", path.c_str());
        }
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::Blocklist
  //
  DVR::Blocklist::Blocklist(int64_t lastmod):
    lastmod_(lastmod)
  {}

  //----------------------------------------------------------------
  // DVR::Blocklist::clear
  //
  void
  DVR::Blocklist::clear()
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::clear");
    boost::unique_lock<boost::mutex> lock(mutex_);

    channels_.clear();
    lastmod_ = std::numeric_limits<int64_t>::min();
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::toggle
  //
  void
  DVR::Blocklist::toggle(uint32_t ch_num)
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::toggle");
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::set<uint32_t>::iterator found = channels_.find(ch_num);
    if (found == channels_.end())
    {
      channels_.insert(ch_num);
    }
    else
    {
      channels_.erase(found);
    }
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::get
  //
  void
  DVR::Blocklist::get(std::set<uint32_t> & channels) const
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::get");
    boost::unique_lock<boost::mutex> lock(mutex_);
    channels = channels_;
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::swap_to_latest
  //
  bool
  DVR::Blocklist::swap_to_latest(DVR::Blocklist & blocklist)
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::swap_to_latest");
    boost::unique_lock<boost::mutex> lock_this(mutex_);
    boost::unique_lock<boost::mutex> lock_that(blocklist.mutex_);

    if (lastmod_ >= blocklist.lastmod_)
    {
      return false;
    }

    std::swap(lastmod_, blocklist.lastmod_);
    channels_.swap(blocklist.channels_);
    return true;
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::save
  //
  void
  DVR::Blocklist::save(Json::Value & json) const
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::save");
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::list<std::string> blocklist;
    for (std::set<uint32_t>::const_iterator
           i = channels_.begin(); i != channels_.end(); ++i)
    {
      const uint32_t ch_num = *i;
      uint16_t major = yae::mpeg_ts::channel_major(ch_num);
      uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
      std::string ch_str = strfmt("%i.%i", int(major), int(minor));
      blocklist.push_back(ch_str);
    }

    yae::save(json, blocklist);
  }

  //----------------------------------------------------------------
  // DVR::Blocklist::load
  //
  void
  DVR::Blocklist::load(const Json::Value & json)
  {
    YAE_BENCHMARK(probe, "DVR::Blocklist::load");
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::list<std::string> blocklist;
    yae::load(json, blocklist);

    channels_.clear();
    for (std::list<std::string>::const_iterator
           i = blocklist.begin(); i != blocklist.end(); ++i)
    {
      const std::string & ch_str = *i;
      uint32_t ch_num = parse_channel_str(ch_str);
      channels_.insert(ch_num);
    }
  }

  //----------------------------------------------------------------
  // DVR::toggle_blocklist
  //
  void
  DVR::toggle_blocklist(uint32_t ch_num)
  {
    // toggle the blocklist item:
    blocklist_.toggle(ch_num);
  }

  //----------------------------------------------------------------
  // DVR::save_blocklist
  //
  void
  DVR::save_blocklist() const
  {
    YAE_BENCHMARK(probe, "DVR::save_blocklist");

    Json::Value json;
    blocklist_.save(json);

    std::string path = (basedir_ / ".yaetv" / "blocklist.json").string();
    YAE_ASSERT(yae::atomic_save(path, json));

    // and another one, for local backup:
    path = (yaetv_ / "blocklist.json").string();
    YAE_ASSERT(yae::atomic_save(path, json));
  }

  //----------------------------------------------------------------
  // DVR::load_blocklist
  //
  bool
  DVR::load_blocklist()
  {
    YAE_BENCHMARK(probe, "DVR::load_blocklist");

    try
    {
      std::string path = (basedir_ / ".yaetv" / "blocklist.json").string();
      Json::Value json;

      if (!yae::attempt_load(path, json))
      {
        yae_ilog("failed to load blocklist %s", path.c_str());

        // try the local backup:
        path = (yaetv_ / "blocklist.json").string();

        if (!yae::attempt_load(path, json))
        {
          yae_ilog("failed to load blocklist %s", path.c_str());

          // load the old blocklist:
          path = (yaetv_ / "blacklist.json").string();

          if (!yae::attempt_load(path, json))
          {
            yae_ilog("failed to load blocklist %s", path.c_str());
            return false;
          }
        }
      }

      int64_t lastmod = yae::stat_lastmod(path.c_str());
      DVR::Blocklist blocklist(lastmod);
      blocklist.load(json);

      if (blocklist_.swap_to_latest(blocklist))
      {
        struct tm tm;
        unix_epoch_time_to_localtime(lastmod, tm);

        std::string lastmod_txt = to_yyyymmdd_hhmmss(tm);
        yae_ilog("loading blocklist %s, lastmod %s",
                 path.c_str(),
                 lastmod_txt.c_str());
        return true;
      }
    }
    catch (const std::exception & e)
    {
      yae_elog("DVR::load_blocklist exception: %s", e.what());
    }
    catch (...)
    {
      yae_elog("DVR::load_blocklist unexpected exception");
    }

    return false;
  }

  //----------------------------------------------------------------
  // DVR::save_wishlist
  //
  void
  DVR::save_wishlist() const
  {
    YAE_BENCHMARK(probe, "DVR::save_wishlist");

    Json::Value json;
    wishlist_.save(json);

    std::string path = (basedir_ / ".yaetv" / "wishlist.json").string();
    YAE_ASSERT(yae::atomic_save(path, json));

    // and another one, for local backup:
    path = (yaetv_ / "wishlist.json").string();
    YAE_ASSERT(yae::atomic_save(path, json));
  }

  //----------------------------------------------------------------
  // DVR::load_wishlist
  //
  bool
  DVR::load_wishlist()
  {
    YAE_BENCHMARK(probe, "DVR::load_wishlist");

    try
    {
      std::string path = (basedir_ / ".yaetv" / "wishlist.json").string();
      Json::Value json;

      if (!yae::attempt_load(path, json))
      {
        yae_ilog("failed to load wishlist %s", path.c_str());

        // load the local backup:
        path = (yaetv_ / "wishlist.json").string();

        if (!yae::attempt_load(path, json))
        {
          yae_ilog("failed to load wishlist %s", path.c_str());
          return false;
        }
      }

      int64_t lastmod = yae::stat_lastmod(path.c_str());
      Wishlist wishlist(lastmod);
      wishlist.load(json);

      if (wishlist_.swap_to_latest(wishlist))
      {
        struct tm tm;
        unix_epoch_time_to_localtime(lastmod, tm);

        std::string lastmod_txt = to_yyyymmdd_hhmmss(tm);
        yae_ilog("loading wishlist %s, lastmod %s",
                 path.c_str(),
                 lastmod_txt.c_str());
        return true;
      }
    }
    catch (const std::exception & e)
    {
      yae_elog("DVR::load_wishlist exception: %s", e.what());
    }
    catch (...)
    {
      yae_elog("DVR::load_wishlist unexpected exception");
    }

    return false;
  }

  //----------------------------------------------------------------
  // DVR::save_schedule
  //
  void
  DVR::save_schedule() const
  {
    YAE_BENCHMARK(probe, "DVR::save_schedule");

    Json::Value json;
    schedule_.save(json);

    boost::unique_lock<boost::mutex> lock(mutex_);
    {
      std::string path = (yaetv_ / "schedule.json").string();
      YAE_ASSERT(yae::atomic_save(path, json));
    }
  }

  //----------------------------------------------------------------
  // DVR::load_schedule
  //
  void
  DVR::load_schedule()
  {
    YAE_BENCHMARK(probe, "DVR::load_schedule");

    Json::Value json;
    std::string path = (yaetv_ / "schedule.json").string();
    if (!yae::TOpenFile(path, "rb").load(json))
    {
      return;
    }

    boost::unique_lock<boost::mutex> lock(mutex_);
    schedule_.load(json);
  }

  //----------------------------------------------------------------
  // wishlist_item_filename
  //
  static std::string
  wishlist_item_filename(const yae::mpeg_ts::EPG::Channel & channel,
                         const yae::mpeg_ts::EPG::Program & program)
  {
    std::string ts = to_yyyymmdd_hhmm(program.tm_, "", "-", "");
    std::string name = strfmt("rec-%02i.%02i-%s.json",
                              channel.major_,
                              channel.minor_,
                              ts.c_str());
    return name;
  }

  //----------------------------------------------------------------
  // DVR::schedule_recording
  //
  void
  DVR::schedule_recording(const yae::mpeg_ts::EPG::Channel & channel,
                          const yae::mpeg_ts::EPG::Program & program)
  {
    Wishlist::Item rec;
    rec.channel_ = std::pair<uint16_t, uint16_t>(channel.major_,
                                                 channel.minor_);
    rec.title_ = program.title_;
    rec.date_ = program.tm_;

    TTime t0(program.tm_.tm_sec + 60 *
             (program.tm_.tm_min + 60 *
              program.tm_.tm_hour), 1);
    TTime t1 = t0 + TTime(program.duration_, 1);
    rec.when_ = Timespan(t0, t1);

    yae_ilog("schedule recording: %02i.%02i %02i:%02i %s",
             channel.major_,
             channel.minor_,
             program.tm_.tm_hour,
             program.tm_.tm_min,
             program.title_.c_str());

    Json::Value json;
    yae::save(json, rec);

    // avoid race condition with Schedule::update:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);

      std::string name = wishlist_item_filename(channel, program);
      std::string path = (basedir_ / ".yaetv" / name).string();

      // if cancelled, then un-cancel:
      remove_utf8((path + ".cancelled").c_str());

      // write it out:
      if (!yae::atomic_save(path, json))
      {
        yae_elog("write failed: %s", path.c_str());
        return;
      }
    }

    set_next_schedule_refresh(TTime::now());

    if (service_loop_worker_)
    {
      // wake up the worker:
      service_loop_worker_->interrupt();
    }
  }

  //----------------------------------------------------------------
  // DVR::cancel_recording
  //
  void
  DVR::cancel_recording(const yae::mpeg_ts::EPG::Channel & channel,
                        const yae::mpeg_ts::EPG::Program & program)
  {
    yae_ilog("cancel recording: %02i.%02i %02i:%02i %s",
             channel.major_,
             channel.minor_,
             program.tm_.tm_hour,
             program.tm_.tm_min,
             program.title_.c_str());

    // avoid race condition with Schedule::update:
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::string name = wishlist_item_filename(channel, program);
    std::string path = (basedir_ / ".yaetv" / name).string();

    // clean up any prior placeholder:
    remove_utf8((path + ".cancelled").c_str());

    int err = rename_utf8(path.c_str(), (path + ".cancelled").c_str());
    YAE_ASSERT(!err);

    uint32_t ch_num = yae::mpeg_ts::channel_number(channel.major_,
                                                   channel.minor_);
    schedule_.remove(ch_num, program.gps_time_);
  }

  //----------------------------------------------------------------
  // DVR::explicitly_scheduled
  //
  yae::shared_ptr<Wishlist::Item>
  DVR::explicitly_scheduled(const yae::mpeg_ts::EPG::Channel & channel,
                            const yae::mpeg_ts::EPG::Program & program) const
  {
    YAE_BENCHMARK(probe, "DVR::explicitly_scheduled");

    // avoid race condition with Schedule::update:
    boost::unique_lock<boost::mutex> lock(mutex_);

    Json::Value json;
    yae::shared_ptr<Wishlist::Item> item_ptr;

    std::string name = wishlist_item_filename(channel, program);
    std::string path = (basedir_ / ".yaetv" / name).string();

    yae::TOpenFile file;
    if (file.open(path, "rb") && file.load(json))
    {
      Wishlist::Item item;
      yae::load(json, item);

      if (item.matches(channel, program))
      {
        item_ptr.reset(new Wishlist::Item(item));
      }
    }

    return item_ptr;
  }

  //----------------------------------------------------------------
  // remove_recording
  //
  static uint64_t
  remove_recording(const std::string & mpg)
  {
    yae_ilog("removing %s", mpg.c_str());

    uint64_t size_mpg = yae::stat_filesize(mpg.c_str());
    if (fs::exists(mpg) && !yae::remove_utf8(mpg))
    {
      yae_wlog("failed to remove %s", mpg.c_str());
      return 0;
    }

    std::string dat = mpg.substr(0, mpg.size() - 4) + ".dat";
    if (fs::exists(dat) && !yae::remove_utf8(dat))
    {
      yae_wlog("failed to remove %s", dat.c_str());
    }

    std::string seen = mpg.substr(0, mpg.size() - 4) + ".seen";
    if (fs::exists(seen) && !yae::remove_utf8(seen))
    {
      yae_wlog("failed to remove %s", seen.c_str());
    }

    std::string json = mpg.substr(0, mpg.size() - 4) + ".json";
    if (fs::exists(json) && !yae::remove_utf8(json))
    {
      yae_wlog("failed to remove %s", json.c_str());
    }

    return size_mpg;
  }

  //----------------------------------------------------------------
  // load_recording
  //
  static TRecPtr
  load_recording(const std::string & mpg)
  {
    try
    {
      std::string path = mpg.substr(0, mpg.size() - 4) + ".json";
      Json::Value json;
      yae::TOpenFile(path, "rb").load(json);
      TRecPtr rec_ptr(new Recording::Rec());
      yae::load(json, *rec_ptr);
      return rec_ptr;
    }
    catch (...)
    {}

    return TRecPtr();
  }

  //----------------------------------------------------------------
  // DVR::toggle_recording
  //
  void
  DVR::toggle_recording(uint32_t ch_num, uint32_t gps_time)
  {
    yae::shared_ptr<Wishlist::Item> explicitly_scheduled;
    yae::mpeg_ts::EPG::Channel channel;

    // avoid race condition with EPG updates:
    {
      boost::unique_lock<boost::mutex> lock(epg_mutex_);
      channel = yae::get(epg_.channels_, ch_num);
    }

    const yae::mpeg_ts::EPG::Program * program = channel.find(gps_time);
    if (program)
    {
      explicitly_scheduled = DVR::explicitly_scheduled(channel, *program);
    }
    else
    {
      yae_elog("toggle recording: not found in EPG");
    }

    if (explicitly_scheduled)
    {
      cancel_recording(channel, *program);
      return;
    }

    if (schedule_.toggle(ch_num, gps_time))
    {
      return;
    }

    if (program)
    {
      schedule_recording(channel, *program);
      return;
    }
  }

  //----------------------------------------------------------------
  // DVR::delete_recording
  //
  void
  DVR::delete_recording(const Recording::Rec & rec)
  {
    // shortcut:
    uint32_t ch_num = rec.ch_num();
    uint32_t gps_t0 = rec.gps_t0_;

    // cancel recording, if recording:
    {
      TRecordingPtr rec_ptr = schedule_.get(ch_num, gps_t0);
      if (rec_ptr && !rec_ptr->is_cancelled())
      {
        toggle_recording(ch_num, gps_t0);
      }
    }

    std::string filepath = rec.get_filepath(basedir_);
    yae_ilog("deleting recording %s", filepath.c_str());
    remove_recording(filepath);
  }

  //----------------------------------------------------------------
  // remove_excess_recordings
  //
  void
  remove_excess_recordings(const fs::path & basedir,
                           const Recording::Rec & rec)
  {
    YAE_BENCHMARK(probe, "remove_excess_recordings");

    if (!rec.max_recordings_)
    {
      // unlimited:
      return;
    }

    std::map<std::string, std::string> recordings;
    {
      std::string title_path = rec.get_title_path(basedir).string();
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(title_path, collect_recordings);
    }

    std::size_t num_recordings = 0;
    std::list<std::pair<std::string, TRecPtr> > recs;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & mpg = i->second;
      TRecPtr rec_ptr = load_recording(mpg);

      if (rec.utc_t0_ == rec_ptr->utc_t0_)
      {
        // ignore the current/in-progress recording:
        continue;
      }

      recs.push_back(std::make_pair(mpg, rec_ptr));
      num_recordings++;
    }

    std::size_t removed_recordings = 0;
    for (std::list<std::pair<std::string, TRecPtr> >::const_iterator
           i = recs.begin(); i != recs.end(); ++i)
    {
      if (num_recordings - removed_recordings <= rec.max_recordings_)
      {
        break;
      }

      const std::string & mpg = i->first;
      const TRecPtr & rec_ptr = i->second;
      if (rec.utc_t0_ <= rec_ptr->utc_t0_)
      {
        continue;
      }

      if (remove_recording(mpg))
      {
        removed_recordings++;
      }
    }
  }

  //----------------------------------------------------------------
  // make_room_for
  //
  bool
  make_room_for(const fs::path & basedir,
                const Recording::Rec & rec,
                uint64_t num_sec)
  {
    YAE_BENCHMARK(probe, "make_room_for rec");

    // remove any existing old recordings beyond max recordings limit:
    yae::remove_excess_recordings(basedir, rec);

    uint64_t title_bytes = ((120 + num_sec) * 20000000) >> 3;
    if (yae::make_room_for(basedir.string(), title_bytes))
    {
      return true;
    }

    yae_elog("failed to free up disk space (%" PRIu64 " MiB) for %i.%i %s",
             title_bytes >> 20,
             rec.channel_major_,
             rec.channel_minor_,
             rec.full_title_.c_str());
    return false;
  }

  //----------------------------------------------------------------
  // make_room_for
  //
  bool
  make_room_for(const std::string & path, uint64_t required_bytes)
  {
    YAE_BENCHMARK(probe, "make_room_for");

    uint64_t filesystem_bytes = 0;
    uint64_t filesystem_bytes_free = 0;
    uint64_t available_bytes = 0;

    if (!yae::stat_diskspace(path.c_str(),
                             filesystem_bytes,
                             filesystem_bytes_free,
                             available_bytes))
    {
      yae_elog("failed to query available disk space for %s", path.c_str());
      return false;
    }

    yae_ilog("checking storage for %s: "
             "%" PRIu64 " GB total, "
             "%" PRIu64 " GB free, "
             "%" PRIu64 " GB available",
             path.c_str(),
             filesystem_bytes / 1000000000,
             filesystem_bytes_free / 1000000000,
             available_bytes / 1000000000);

    if (required_bytes < available_bytes)
    {
      return true;
    }

    std::map<std::string, std::string> recordings;
    {
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(path, collect_recordings);
    }

    std::size_t removed_bytes = 0;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      if (required_bytes < available_bytes + removed_bytes)
      {
        break;
      }

      const std::string & mpg = i->second;
      removed_bytes += remove_recording(mpg);
    }

    if (required_bytes < available_bytes + removed_bytes)
    {
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DVR::make_room_for
  //
  bool
  DVR::make_room_for(const Recording::Rec & rec, uint64_t num_sec)
  {
    cleanup_yaetv_dir();
    return yae::make_room_for(basedir_, rec, num_sec);
  }

  //----------------------------------------------------------------
  // DVR::already_recorded
  //
  TRecPtr
  DVR::already_recorded(const yae::mpeg_ts::EPG::Channel & channel,
                        const yae::mpeg_ts::EPG::Program & program) const
  {
    YAE_BENCHMARK(probe, "DVR::already_recorded");

    if (program.description_.empty())
    {
      // can't check for duplicates without a description:
      return TRecPtr();
    }

    Recording::Rec rec(channel, program);
    std::map<std::string, std::string> recordings;
    {
      std::string title_path = rec.get_title_path(basedir_).string();
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(title_path, collect_recordings);
    }

    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & mpg = i->second;
      TRecPtr rec_ptr = load_recording(mpg);
      if (!rec_ptr)
      {
        continue;
      }

      const Recording::Rec & recorded = *rec_ptr;
      if (recorded.cancelled_ ||
          rec.utc_t0_ <= recorded.utc_t0_ ||
          rec.description_ != recorded.description_)
      {
        continue;
      }

      // check that the existing recording is approximately complete:
      std::string json_path = mpg.substr(0, mpg.size() - 4) + ".json";
      int64_t utc_t0 = yae::stat_lastmod(json_path.c_str());
      int64_t utc_t1 = yae::stat_lastmod(mpg.c_str());
      int64_t recorded_duration = utc_t1 - utc_t0;
      if (program.duration_ <= recorded_duration)
      {
        return rec_ptr;
      }
      else
      {
        yae_ilog("found an incomplete recording: %s, %s",
                 json_path.c_str(),
                 program.description_.c_str());
      }
    }

    return TRecPtr();
  }

  //----------------------------------------------------------------
  // get_playlist
  //
  static std::string
  get_playlist(const Recording::Rec & rec)
  {
    std::string playlist = yae::strfmt("%02i.%02i %s",
                                       rec.channel_major_,
                                       rec.channel_minor_,
                                       rec.get_short_title().c_str());
    return playlist;
  }

  //----------------------------------------------------------------
  // DVR::get_existing_recordings
  //
  void
  DVR::get_existing_recordings(FoundRecordings & found) const
  {
    YAE_BENCHMARK(probe, "DVR::get_existing_recordings");

    std::map<std::string, std::string> recordings;
    {
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(basedir_.string(), collect_recordings);
    }

    TRecs rec_by_fn;
    std::map<std::string, TRecs> rec_by_pl;
    std::map<uint32_t, TRecsByTime> rec_by_ch;

    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & filename = i->first;
      const std::string & filepath = i->second;

      TRecPtr rec_ptr = load_recording(filepath);
      if (!rec_ptr)
      {
        continue;
      }

      const Recording::Rec & recorded = *rec_ptr;
      std::string playlist = yae::get_playlist(recorded);

      rec_by_fn[filename] = rec_ptr;
      rec_by_pl[playlist][filename] = rec_ptr;

      uint32_t ch_num = yae::mpeg_ts::channel_number(recorded.channel_major_,
                                                     recorded.channel_minor_);
      rec_by_ch[ch_num][recorded.gps_t0_] = rec_ptr;
    }

    found.by_filename_.swap(rec_by_fn);
    found.by_playlist_.swap(rec_by_pl);
    found.by_channel_.swap(rec_by_ch);
  }

  //----------------------------------------------------------------
  // DVR::is_ready_to_play
  //
  yae::shared_ptr<DVR::Playback>
  DVR::is_ready_to_play(const Recording::Rec & rec) const
  {
    YAE_BENCHMARK(probe, "DVR::is_ready_to_play");

    yae::shared_ptr<Playback> result;
    fs::path title_path = rec.get_title_path(basedir_);
    std::string mpg_path = rec.get_title_filepath(title_path, ".mpg");

    if (fs::exists(mpg_path))
    {
      std::string playlist = yae::get_playlist(rec);
      std::string basepath = mpg_path.substr(0, mpg_path.size() - 4);
      std::string filename = mpg_path.substr(title_path.string().size() + 1);
      result.reset(new Playback(playlist, filename, basepath));
    }

    return result;
  }

  //----------------------------------------------------------------
  // DVR::watch_live
  //
  void
  DVR::watch_live(uint32_t ch_num)
  {
    uint32_t prev_ch = schedule_.enable_live(ch_num);
    if (prev_ch)
    {
      std::map<uint32_t, std::string> frequencies;
      DVR::get_channels(frequencies);
      std::string frequency = yae::at(frequencies, prev_ch);

      boost::unique_lock<boost::mutex> lock(mutex_);
      TStreamPtr stream = stream_[frequency].lock();
      if (stream)
      {
        stream->packet_handler_->refresh_cached_recordings();
      }
    }

    TTime lastmod;
    yae::mpeg_ts::EPG epg;
    if (get_cached_epg(lastmod, epg))
    {
      DVR::evaluate(epg);
    }
  }

  //----------------------------------------------------------------
  // DVR::close_live
  //
  void
  DVR::close_live()
  {
    uint32_t live_ch = schedule_.disable_live();
    if (live_ch)
    {
      std::map<uint32_t, std::string> frequencies;
      DVR::get_channels(frequencies);
      std::string frequency = yae::get(frequencies, live_ch);

      boost::unique_lock<boost::mutex> lock(mutex_);
      TStreamPtr stream = stream_[frequency].lock();
      if (stream)
      {
        stream->packet_handler_->refresh_cached_recordings();
      }
    }

    TTime lastmod;
    yae::mpeg_ts::EPG epg;
    if (get_cached_epg(lastmod, epg))
    {
      DVR::evaluate(epg);
    }
  }

  //----------------------------------------------------------------
  // DVR::evaluate
  //
  void
  DVR::evaluate(const yae::mpeg_ts::EPG & epg)
  {
    uint32_t margin_sec = margin_.get(1);
    schedule_.update(*this, epg);

    std::string writer_uuid = get_writer_uuid();
    if (!writer_uuid.empty())
    {
      return;
    }

    std::map<uint32_t, std::string> frequencies;
    DVR::get_channels(frequencies);
    if (frequencies.empty())
    {
      return;
    }

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;

      uint32_t gps_time = TTime::gps_now().get(1);

      std::set<TRecordingPtr> recs;
      schedule_.get(recs, ch_num, gps_time, margin_sec);
      if (recs.empty())
      {
#if 0
        yae_ilog("nothing scheduled for %i.%i",
                 yae::mpeg_ts::channel_major(ch_num),
                 yae::mpeg_ts::channel_minor(ch_num));
#endif
        continue;
      }

      for (std::set<TRecordingPtr>::const_iterator
             j = recs.begin(); j != recs.end(); ++j)
      {
        Recording & recording = *(*j);
        bool is_recording = recording.is_recording();

        TRecPtr rec_ptr = recording.get_rec();
        const Recording::Rec & rec = *rec_ptr;
        uint64_t num_sec = rec.gps_t1_ + margin_sec - gps_time;

        if (!is_recording)
        {
          std::string title_path = rec.get_title_path(basedir_).string();

          if (!make_room_for(rec, num_sec))
          {
            continue;
          }

          if (!yae::mkdir_p(title_path))
          {
            yae_elog("failed to mkdir %s", title_path.c_str());
            continue;
          }
        }

        std::map<uint32_t, std::string>::const_iterator
          found = frequencies.find(ch_num);
        if (found == frequencies.end())
        {
          yae_elog("can't find frequency for channel %i.%i, can't record %s",
                   int(rec.channel_major_),
                   int(rec.channel_minor_),
                   rec.get_basename().c_str());
          continue;
        }

        std::string frequency = found->second;
        TStreamPtr stream = capture_stream(frequency, TTime(num_sec, 1));

        // find the current EPG channel program that corresponds
        // to the midpoint of the scheduled recording:
        const yae::mpeg_ts::EPG::Program * program =
          channel.find(rec.gps_midpoint());

        // update the recording info on disk if it doesn't match
        if (stream && stream->packet_handler_)
        {
          TRecPtr saved_rec_ptr(new Recording::Rec(rec));
          Recording::Rec & saved_rec = *saved_rec_ptr;
          saved_rec.load(basedir_);

          const std::string & device_info =
            stream->packet_handler_->ctx_.log_prefix_;

          if ((program &&
               program->description_.empty() == false &&
               program->description_ != saved_rec.description_) ||
              device_info != saved_rec.device_info_)
          {
            saved_rec.device_info_ = device_info;

            if (program)
            {
              saved_rec.update(channel,
                               *program,
                               rec.made_by_,
                               rec.max_recordings_);
            }

            saved_rec.save(basedir_);
            recording.set_rec(saved_rec_ptr);
          }
        }

        if (!stream)
        {
          yae_elog("failed to start a stream: %s",
                   rec.get_basename().c_str());
        }
        else if (!is_recording)
        {
          yae_ilog("%sstarting stream: %s",
                   stream->packet_handler_->ctx_.log_prefix_.c_str(),
                   rec.get_basename().c_str());

          stream->packet_handler_->refresh_cached_recordings();

          // FIXME: there is probably a better place for this:
          save_schedule();
        }
        else
        {
          yae_ilog("%salready recording: %s",
                   stream->packet_handler_->ctx_.log_prefix_.c_str(),
                   rec.get_basename().c_str());
        }

        recording.set_stream(stream);
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::get_cached_epg
  //
  bool
  DVR::get_cached_epg(TTime & lastmod, yae::mpeg_ts::EPG & epg) const
  {
    YAE_BENCHMARK(probe, "DVR::get_cached_epg");

    const bool is_current_writer = this->is_local_uuid_writer_uuid();

    boost::unique_lock<boost::mutex> lock(epg_mutex_);
    YAE_BENCHMARK(probe1, "DVR::get_cached_epg after mutex");

    if (lastmod.invalid() || lastmod < epg_lastmod_)
    {
      lastmod = epg_lastmod_;
      epg.channels_.clear();

      for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
             i = epg_.channels_.begin(); i != epg_.channels_.end(); ++i)
      {
        const uint32_t ch_num = i->first;
        const yae::mpeg_ts::EPG::Channel & channel = i->second;

        if (is_current_writer &&
            !channel_frequency_lut_.empty() &&
            !yae::has(channel_frequency_lut_, ch_num))
        {
          // don't list channels that disappeared:
          continue;
        }

        epg.channels_[ch_num] = channel;
      }

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DVR::get_tuner_cache
  //
  void
  DVR::get_tuner_cache(const std::string & device_name,
                       Json::Value & tuner_cache) const
  {
    YAE_BENCHMARK(probe, "DVR::get_tuner_cache");

    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    tuner_cache = tuner_cache_.get(device_name, Json::Value());
  }

  //----------------------------------------------------------------
  // DVR::update_tuner_cache
  //
  void
  DVR::update_tuner_cache(const std::string & device_name,
                          const Json::Value & tuner_cache)
  {
    YAE_BENCHMARK(probe, "DVR::update_tuner_cache");

    Json::Value cache;
    {
      boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
      tuner_cache_[device_name] = tuner_cache;
      update_channel_frequency_luts();
      cache = tuner_cache_;
    }

    // save tuner_cache to disk:
    std::string path = (yaetv_ / "tuners.json").string();
    yae::TOpenFile(path, "wb").save(cache);
  }

  //----------------------------------------------------------------
  // DVR::update_channel_frequency_luts
  //
  void
  DVR::update_channel_frequency_luts()
  {
    YAE_BENCHMARK(probe, "DVR::update_channel_frequency_luts");

    frequency_channel_lut_.clear();

    std::set<std::string> signal_present;
    std::set<std::string> signal_absent;

    std::set<std::string> channels_present;
    std::set<std::string> channels_absent;

    const Json::Value & const_tuner_cache = tuner_cache_;
    for (Json::Value::const_iterator i = const_tuner_cache.begin();
         i != tuner_cache_.end(); ++i)
    {
      std::string device_name = i.key().asString();
      const Json::Value & tuner_cache = *i;
      const Json::Value & frequencies = tuner_cache["frequencies"];

      for (Json::Value::const_iterator j = frequencies.begin();
           j != frequencies.end(); ++j)
      {
        std::string frequency = j.key().asString();
        const Json::Value & cache = *j;
        const Json::Value & programs = cache["programs"];
        const Json::Value & status = cache["status"];

        bool no_signal_present =
          !status.get("signal_present", false).asBool();

        uint32_t signal_strength =
          status.get("signal_strength", 100).asUInt();

        uint32_t signal_to_noise_quality =
          status.get("signal_to_noise_quality", 0).asUInt();

        // uint32_t symbol_error_quality =
        //   status.get("symbol_error_quality", 100).asUInt();

        bool poor_signal_present =
          (signal_strength < 70 &&
           signal_to_noise_quality < 70);

        if (no_signal_present ||
            poor_signal_present ||
            programs.empty())
        {
          signal_absent.insert(frequency);
          continue;
        }
        else
        {
          signal_present.insert(frequency);
        }

        TChannels & channels = frequency_channel_lut_[frequency];

        for (Json::Value::const_iterator k = programs.begin();
             k != programs.end(); ++k)
        {
          const Json::Value & program = *k;
          uint16_t major = uint16_t(program["major"].asUInt());
          uint16_t minor = uint16_t(program["minor"].asUInt());
          std::string name = program["name"].asString();
          channels[major][minor] = name;

          uint32_t ch_num = yae::mpeg_ts::channel_number(major, minor);
          std::string & ch_freq = channel_frequency_lut_[ch_num];
          if (ch_freq.empty())
          {
            ch_freq = frequency;
          }
          else if (ch_freq != frequency)
          {
            // Hmm, channel is available on multiple frequencies,
            // so choose the one that has the strongest signal:
            Json::Value other = frequencies.get(ch_freq, Json::Value());

            bool other_signal_present = other.
              get("status", Json::Value()).
              get("signal_present", false).
              asBool();

            uint32_t other_signal_to_noise_quality = other.
              get("status", Json::Value()).
              get("signal_to_noise_quality", 0).
              asUInt();

            yae_ilog("%i.%i is available on multiple frequencies: "
                     "%s snq %u, %s snq %u",
                     major, minor,
                     ch_freq.c_str(), other_signal_to_noise_quality,
                     frequency.c_str(), signal_to_noise_quality);

            if (other_signal_present &&
                signal_to_noise_quality <= other_signal_to_noise_quality)
            {
              // use the other frequency:
              yae_ilog("will use % 9s Hz for %i.%i",
                       ch_freq.c_str(), major, minor);
            }
            else
            {
              yae_ilog("will use % 9s Hz for %i.%i",
                       frequency.c_str(), major, minor);
              ch_freq = frequency;
            }
          }
        }

        if (channels.empty())
        {
          channels_absent.insert(frequency);
        }
        else
        {
          channels_present.insert(frequency);
        }
      }
    }

    for (std::set<std::string>::const_iterator
           i = channels_absent.begin(); i != channels_absent.end(); ++i)
    {
      const std::string & frequency = *i;
      if (yae::has(channels_present, frequency))
      {
        continue;
      }

      yae_ilog("no channels found at % 9s Hz", frequency.c_str());
      frequency_channel_lut_.erase(frequency);
    }

    for (std::set<std::string>::const_iterator
           i = signal_absent.begin(); i != signal_absent.end(); ++i)
    {
      const std::string & frequency = *i;
      if (yae::has(signal_present, frequency))
      {
        continue;
      }

      yae_ilog("no signal present at % 9s Hz", frequency.c_str());
      frequency_channel_lut_.erase(frequency);
    }

    YAE_ASSERT(frequency_channel_lut_.empty() ==
               channel_frequency_lut_.empty());
  }

  //----------------------------------------------------------------
  // DVR::save_heartbeat
  //
  void
  DVR::save_heartbeat()
  {
    YAE_BENCHMARK(probe, "DVR::save_heartbeat");

    std::string heartbeat_path =
      (basedir_ / ".yaetv" / ("heartbeat-" + local_uuid_ + ".json")).string();

    Json::Value json;
    json["uuid"] = local_uuid_;
    json["host"] = boost::asio::ip::host_name();
    json["has_tuners"] = check_local_recording_allowed();
    json["heartbeat"] = (Json::Value::Int64)(TTime::now().get(1));

    YAE_ASSERT(yae::atomic_save(heartbeat_path, json));
  }

  //----------------------------------------------------------------
  // DVR::check_local_recording_allowed
  //
  bool
  DVR::check_local_recording_allowed() const
  {
    YAE_BENCHMARK(probe, "DVR::check_local_recording_allowed");

    // check whether recording has been explicitly disabled:
    bool recording_allowed = true;
    {
      boost::unique_lock<boost::mutex> lock(preferences_mutex_);
      if (!preferences_.get("allow_recording", true).asBool())
      {
        recording_allowed = false;
      }
    }

    // check whether there are any other DVR instances writing
    // to the same storage:
    std::map<std::string, std::string> dvr_instances;
    {
      std::string heartbeat_dir = (basedir_ / ".yaetv").string();
      CollectFiles collect_files(dvr_instances, heartbeat_rx);
      for_each_file_at(heartbeat_dir, collect_files);
    }

    static const int64_t one_day = 24 * 60 * 60;
    const int64_t now = yae::TTime::now().get(1);

    for (std::map<std::string, std::string>::const_iterator
           i = dvr_instances.begin(); i != dvr_instances.end(); ++i)
    {
      const std::string & name = i->first;
      const std::string & path = i->second;

      Json::Value dvr;
      if (!yae::attempt_load(path, dvr))
      {
        YAE_ASSERT(false);

        // give up:
        continue;
      }

      std::string dvr_uuid = dvr.get("uuid", std::string()).asString();
      if (dvr_uuid == local_uuid_)
      {
        // skip self:
        continue;
      }

      std::string dvr_host = dvr.get("host", std::string()).asString();
      int64_t heartbeat = dvr.get("heartbeat", 0).asInt64();

      int64_t dt_sec = now - heartbeat;
      if (dt_sec > 60)
      {
        // slow heartbeat ... possibly dead:
        yae_dlog("possibly dead DVR instance: "
                 "uuid: %s, "
                 "host: %s, "
                 "time since last heartbeat (%s): %" PRIi64 "",
                 dvr_uuid.c_str(),
                 dvr_host.c_str(),
                 yae::unix_epoch_time_to_localtime_str(heartbeat).c_str(),
                 dt_sec);

        if (dt_sec >= 24 * 60 * 60)
        {
          // dead DVR instance, last heartbeat was more than a day ago:
          yae_ilog("removing dead DVR info: %s", path.c_str());
          yae::remove_utf8(path);
        }

        boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
        if (dvr_uuid == writer_uuid_)
        {
          writer_uuid_.clear();
        }

        continue;
      }

      bool has_tuners = dvr.get("has_tuners", false).asBool();
      if (!has_tuners)
      {
        boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
        if (dvr_uuid == writer_uuid_)
        {
          yae_ilog("un-selecting writer DVR instance %s (%s) "
                   "since it no longer has any tuners",
                   dvr_uuid.c_str(),
                   dvr_host.c_str());
          writer_uuid_.clear();
        }

        // ignore non-recording DVR instances:
        continue;
      }

      boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
      if (writer_uuid_.empty())
      {
        yae_ilog("selecting writer DVR instance %s (%s)",
                 dvr_uuid.c_str(),
                 dvr_host.c_str());
        writer_uuid_ = dvr_uuid;
      }

      // another writer exists ... there had better be just one:
      YAE_ASSERT(writer_uuid_ == dvr_uuid);
      return false;
    }

    // no writer DVR instances detected:
    {
      boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
      if (!writer_uuid_.empty())
      {
        yae_ilog("un-selecting writer DVR instance %s, "
                 "it appears to have vanished",
                 writer_uuid_.c_str());
        writer_uuid_.clear();
      }
    }

    // check if we have any enabled tuners:
    if (recording_allowed)
    {
      std::set<std::string> enabled_tuners;
      if (this->discover_enabled_tuners(enabled_tuners))
      {
        return true;
      }
    }

    // no enabled tuners discovered:
    boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
    if (writer_uuid_.empty())
    {
      yae_dlog("no writer DVR instance detected");
    }

    return false;
  }

  //----------------------------------------------------------------
  // DVR::get_channel_luts
  //
  void
  DVR::get_channel_luts(std::map<uint32_t, std::string> & chan_freq,
                        std::map<std::string, TChannels> & freq_chan) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channel_luts");
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    chan_freq = channel_frequency_lut_;
    freq_chan = frequency_channel_lut_;
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  void
  DVR::get_channels(std::map<uint32_t, std::string> & chan_freq) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channels chan_freq");
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    chan_freq = channel_frequency_lut_;
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  void
  DVR::get_channels(std::map<std::string, TChannels> & channels) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channels freq_chan");
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    channels = frequency_channel_lut_;
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  bool
  DVR::get_channels(const std::string & freq, TChannels & channels) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channels @freq");
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    std::map<std::string, TChannels>::const_iterator found =
      frequency_channel_lut_.find(freq);
    if (found == frequency_channel_lut_.end())
    {
      return false;
    }

    channels = found->second;
    return true;
  }

  //----------------------------------------------------------------
  // DVR::get_channels_str
  //
  std::string
  DVR::get_channels_str(const std::string & frequency) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channels_str @freq");
    std::ostringstream oss;
    const char * sep = "";

    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    std::map<std::string, TChannels>::const_iterator found =
      frequency_channel_lut_.find(frequency);
    if (found != frequency_channel_lut_.end())
    {
      const TChannels & channels = found->second;
      for (TChannels::const_iterator
             i = channels.begin(); i != channels.end(); ++i)
      {
        uint16_t major = i->first;
        const TChannelNames & names = i->second;
        for (TChannelNames::const_iterator
               j = names.begin(); j != names.end(); ++j)
        {
          uint16_t minor = j->first;
          const std::string & name = j->second;

          oss << sep << major << "." << minor << " " << name;
          sep = ", ";
        }
      }
    }

    return oss.str();
  }

  //----------------------------------------------------------------
  // DVR::get_channel_name
  //
  bool
  DVR::get_channel_name(uint16_t major,
                        uint16_t minor,
                        std::string & name) const
  {
    YAE_BENCHMARK(probe, "DVR::get_channels_name major.minor");
    uint32_t ch_num = yae::mpeg_ts::channel_number(major, minor);

    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    std::string frequency = yae::get(channel_frequency_lut_, ch_num);
    if (frequency.empty())
    {
      return false;
    }

    std::map<std::string, TChannels>::const_iterator found =
      frequency_channel_lut_.find(frequency);
    if (found == frequency_channel_lut_.end())
    {
      return false;
    }

    const TChannels & channels = found->second;
    TChannels::const_iterator found_major = channels.find(major);
    if (found_major == channels.end())
    {
      return false;
    }

    const TChannelNames & ch_names = found_major->second;
    TChannelNames::const_iterator found_minor = ch_names.find(minor);
    if (found_minor == ch_names.end())
    {
      return false;
    }

    name = found_minor->second;
    return true;
  }

  //----------------------------------------------------------------
  // DVR::maybe_skip
  //
  bool
  DVR::maybe_skip(const std::string & frequency) const
  {
    std::map<std::string, Json::Value> tuners;
    {
      boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
      for (Json::Value::const_iterator
             i = tuner_cache_.begin(); i != tuner_cache_.end(); ++i)
      {
        const std::string & name = i.key().asString();
        const Json::Value & cache = *i;
        Json::Value & tuner = tuners[name];
        tuner = cache.
          get("frequencies", Json::Value()).
          get(frequency, Json::Value());
      }
    }

    std::set<uint32_t> blocked_channels;
    blocklist_.get(blocked_channels);

    for (std::map<std::string, Json::Value>::const_iterator
           i = tuners.begin(); i != tuners.end(); ++i)
    {
      const Json::Value & tuner = i->second;
      const Json::Value & programs = tuner.get("programs", Json::Value());
      if (programs.empty())
      {
        return false;
      }

      // check if any of the channels on this frequency are unblocked:
      for (Json::Value::const_iterator
             i = programs.begin(), end = programs.end(); i != end; ++i)
      {
        const Json::Value & program = *i;
        uint16_t major = uint16_t(program["major"].asUInt());
        uint16_t minor = uint16_t(program["minor"].asUInt());
        uint32_t ch_num = yae::mpeg_ts::channel_number(major, minor);
        if (!yae::has(blocked_channels, ch_num))
        {
          return false;
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // DVR::has_preferences
  //
  bool
  DVR::has_preferences() const
  {
    YAE_BENCHMARK(probe, "DVR::has_preferences");
    boost::unique_lock<boost::mutex> lock(preferences_mutex_);
    if (preferences_.empty())
    {
      return false;
    }

    return fs::is_directory(basedir_.string());
  }

  //----------------------------------------------------------------
  // DVR::get_preferences
  //
  void
  DVR::get_preferences(Json::Value & preferences) const
  {
    YAE_BENCHMARK(probe, "DVR::get_preferences");
    boost::unique_lock<boost::mutex> lock(preferences_mutex_);
    preferences = preferences_;
  }

  //----------------------------------------------------------------
  // DVR::set_preferences
  //
  void
  DVR::set_preferences(const Json::Value & preferences)
  {
    YAE_BENCHMARK(probe, "DVR::set_preferences");
    std::string basedir =
      preferences.get("basedir", basedir_.string()).asString();

    // may need to restart the DVR if settings have changed:
    {
      boost::unique_lock<boost::mutex> lock(preferences_mutex_);
      if (preferences_ == preferences)
      {
        return;
      }

      preferences_ = preferences;

      std::string path = (yaetv_ / "settings.json").string();
      YAE_ASSERT(yae::TOpenFile(path, "wb").save(preferences));
    }

    restart(basedir);
  }

  //----------------------------------------------------------------
  // DVR::discover_enabled_tuners
  //
  bool
  DVR::discover_enabled_tuners(std::set<std::string> & tuner_names) const
  {
    YAE_BENCHMARK(probe, "DVR::discover_enabled_tuners");
    Json::Value tuners;
    {
      boost::unique_lock<boost::mutex> lock(preferences_mutex_);

      if (!preferences_.get("allow_recording", true).asBool())
      {
        return false;
      }

      tuners = preferences_.get("tuners", Json::Value(Json::objectValue));
    }

    std::string writer_uuid = get_writer_uuid();
    if (!writer_uuid.empty())
    {
      // another DVR instance is writing to the same storage:
      return false;
    }

    std::list<TunerDevicePtr> devices;
    hdhr_.discover_devices(devices);

    for (std::list<TunerDevicePtr>::const_iterator
           i = devices.begin(); i != devices.end(); ++i)
    {
      const TunerDevice & device = *(*(i));
      for (int j = 0, num_tuners = device.num_tuners(); j < num_tuners; j++)
      {
        std::string tuner_name = device.tuner_name(j);
        bool enabled = tuners.get(tuner_name, false).asBool();
        if (enabled)
        {
          tuner_names.insert(tuner_name);
        }
      }
    }

    return !tuner_names.empty();
  }

  //----------------------------------------------------------------
  // DVR::is_local_uuid_writer_uuid
  //
  bool
  DVR::is_local_uuid_writer_uuid() const
  {
    boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
    return local_uuid_ == writer_uuid_;
  }

  //----------------------------------------------------------------
  // DVR::get_writer_uuid
  //
  std::string
  DVR::get_writer_uuid() const
  {
    boost::unique_lock<boost::mutex> lock(writer_uuid_mutex_);
    return writer_uuid_;
  }

  //----------------------------------------------------------------
  // DVR::get_channelmap
  //
  std::string
  DVR::get_channelmap() const
  {
    boost::unique_lock<boost::mutex> lock(preferences_mutex_);
    std::string channelmap =
      preferences_.get("channelmap", "us-bcast").asString();
    return channelmap;
  }

  //----------------------------------------------------------------
  // cleanup_yaetv_logs
  //
  void
  cleanup_yaetv_logs(const std::string & yaetv_dir)
  {
    std::map<std::string, std::string> logs;
    {
      CollectFiles collect_files(logs, yaetv_log_rx);
      for_each_file_at(yaetv_dir, collect_files);
    }

    // remove all logs except 8 most recent:
    std::map<std::string, std::string>::reverse_iterator it = logs.rbegin();
    for (int i = 0; i < 8 && it != logs.rend(); i++, ++it) {}

    for (; it != logs.rend(); ++it)
    {
      const std::string & path = it->second;
      if (!yae::remove_utf8(path))
      {
        yae_wlog("failed to remove %s", path.c_str());
      }
      else
      {
        yae_ilog("removed old log: %s", path.c_str());
      }
    }
  }

}
