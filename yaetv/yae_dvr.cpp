// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Dec  1 12:38:37 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/random/mersenne_twister.hpp>
#include <boost/thread.hpp>
#endif

#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif

// aeyae:
#include "yae/api/yae_log.h"

// yaetv:
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
  Wishlist::Wishlist():
    lastmod_(std::numeric_limits<int64_t>::min())
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
  // Wishlist::Item::to_txt
  //
  std::string
  Wishlist::Item::to_txt() const
  {
    const char * sep = "";
    std::ostringstream oss;

    if (when_)
    {
      const Timespan & when = *when_;
      oss << sep << when.t0_.to_hhmm() << " - " << when.t1_.to_hhmm();
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

    if (date_)
    {
      const struct tm & tm = *date_;
      int64_t ts = yae::localtime_to_unix_epoch_time(tm);
      oss << sep << yae::unix_epoch_time_to_localdate(ts);
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

    if (when_)
    {
      const Timespan & when = *when_;
      oss << sep << when.t0_.to_hhmm() << " - " << when.t1_.to_hhmm();
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
          oss << s << kWeekdays[i];
          s = " ";
        }

        wday <<= 1;
      }

      sep = ", ";
    }

    if (date_)
    {
      const struct tm & tm = *date_;
      int64_t ts = yae::localtime_to_unix_epoch_time(tm);
      oss << sep << yae::unix_epoch_time_to_localdate(ts);
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
    if (match_title)
    {
      if (!rx_title_)
      {
        rx_title_.reset(boost::regex(title_, boost::regex::icase));
      }

      if (!(program.title_ == title_ ||
            boost::regex_match(program.title_, *rx_title_)))
      {
        return false;
      }
    }

    bool match_description = !description_.empty();
    if (match_description)
    {
      if (!rx_description_)
      {
        rx_description_.reset(boost::regex(description_, boost::regex::icase));
      }

      if (!(program.description_ == description_ ||
            boost::regex_match(program.description_, *rx_description_)))
      {
        return false;
      }
    }

    if (channel_ && (date_ ||
                     weekday_mask ||
                     match_timespan ||
                     match_title ||
                     match_description))
    {
      return true;
    }

    return false;
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
    yae::load(json, "disabled", disabled_);
  }

  //----------------------------------------------------------------
  // Wishlist::get
  //
  void
  Wishlist::get(std::map<std::string, Item> & wishlist) const
  {
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
    for (std::list<Item>::const_iterator
           i = items_.begin(); i != items_.end(); ++i)
    {
      const Item & item = *i;
      if (item.matches(channel, program))
      {
        return yae::shared_ptr<Item>(new Item(item));
      }
    }

    return yae::shared_ptr<Item>();
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
  // save
  //
  void
  save(Json::Value & json, const Wishlist & wishlist)
  {
    save(json["items"], wishlist.items_);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, Wishlist & wishlist)
  {
    load(json["items"], wishlist.items_);
  }


  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording():
    made_by_(Recording::kUnspecified),
    cancelled_(false),
    utc_t0_(0),
    gps_t0_(0),
    gps_t1_(0),
    channel_major_(0),
    channel_minor_(0),
    max_recordings_(0)
  {}

  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording(const yae::mpeg_ts::EPG::Channel & channel,
                       const yae::mpeg_ts::EPG::Program & program):
    made_by_(Recording::kUnspecified),
    cancelled_(false),
    utc_t0_(localtime_to_unix_epoch_time(program.tm_)),
    gps_t0_(program.gps_time_),
    gps_t1_(program.gps_time_ + program.duration_),
    channel_major_(channel.major_),
    channel_minor_(channel.minor_),
    channel_name_(channel.name_),
    title_(program.title_),
    rating_(program.rating_),
    description_(program.description_),
    max_recordings_(0),
    dat_time_(0),
    mpg_size_(0)
  {}

  //----------------------------------------------------------------
  // Recording::~Recording
  //
  Recording::~Recording()
  {
    if (mpg_ && mpg_->is_open())
    {
      std::string fn = get_basename();
      yae_ilog("stopped recording: %s", fn.c_str());
    }
  }

  //----------------------------------------------------------------
  // Recording::get_title_path
  //
  fs::path
  Recording::get_title_path(const fs::path & basedir) const
  {
    // title path:
    std::string channel;
    {
      std::ostringstream oss;
      oss << std::setfill('0') << std::setw(2) << channel_major_
          << "."
          << std::setfill('0') << std::setw(2) << channel_minor_;
      channel = oss.str().c_str();
    }

    std::string safe_title = sanitize_filename_utf8(title_);
    fs::path title_path = basedir / channel / safe_title;
    return title_path;
  }

  //----------------------------------------------------------------
  // Recording::get_basename
  //
  std::string
  Recording::get_basename() const
  {
    std::string safe_title = sanitize_filename_utf8(title_);

    struct tm tm;
    unix_epoch_time_to_localtime(utc_t0_, tm);
    std::string datetime_txt = to_yyyymmdd_hhmm(tm, "", "-", "");

    std::ostringstream oss;
    oss << datetime_txt
        << " "
        << std::setfill('0') << std::setw(2) << channel_major_
        << "."
        << std::setfill('0') << std::setw(2) << channel_minor_
        << " "
        << safe_title;

    std::string basename = oss.str().c_str();
    return basename;
  }

  //----------------------------------------------------------------
  // Recording::get_filepath
  //
  std::string
  Recording::get_filepath(const fs::path & basedir, const char * ext) const
  {
    fs::path title_path = get_title_path(basedir);
    std::string basename = get_basename();
    std::string basepath = (title_path / basename).string();
    std::string filepath = basepath + ext;
    return filepath;
  }

  //----------------------------------------------------------------
  // Recording::open_mpg
  //
  yae::TOpenFilePtr
  Recording::open_mpg(const fs::path & basedir)
  {
    if (mpg_ && mpg_->is_open())
    {
      return mpg_;
    }

    mpg_.reset();

    uint32_t num_sec = gps_t1_ - gps_t0_;
    yae::make_room_for(basedir, *this, num_sec);

    fs::path title_path = get_title_path(basedir);
    std::string title_path_str = title_path.string();
    if (!yae::mkdir_p(title_path_str))
    {
      yae_elog("mkdir_p failed for: %s", title_path_str.c_str());
      return TOpenFilePtr();
    }

    std::string basename = get_basename();
    std::string basepath = (title_path / basename).string();
    std::string filepath = basepath + ".mpg";
    mpg_.reset(new yae::TOpenFile(filepath, "ab"));
    bool ok = mpg_->is_open();

    yae_ilog("writing to: %s, %s", filepath.c_str(), ok ? "ok" : "failed");
    if (!ok)
    {
      mpg_.reset();
      yae_elog("fopen failed for: %s", filepath.c_str());
    }
    else
    {
      dat_time_ = 0;
      mpg_size_ = yae::stat_filesize((basepath + ".mpg").c_str());

      uint64_t misalignment = mpg_size_ % 188;
      if (misalignment)
      {
        // realign to TS packet boundary:
        std::size_t padding = 188 - misalignment;
        std::vector<uint8_t> zeros(padding);
        mpg_->write(zeros);
        mpg_size_ += padding;
      }

      Json::Value json;
      yae::save(json, *this);
      std::string filepath = basepath + ".json";
      if (!yae::TOpenFile(filepath, "wb").save(json))
      {
        yae_wlog("fopen failed for: %s", filepath.c_str());
      }
    }

    return mpg_;
  }

  //----------------------------------------------------------------
  // Recording::open_dat
  //
  yae::TOpenFilePtr
  Recording::open_dat(const fs::path & basedir)
  {
    if (dat_ && dat_->is_open())
    {
      return dat_;
    }

    dat_.reset();

    std::string filepath = get_filepath(basedir, ".dat");
    dat_.reset(new yae::TOpenFile(filepath, "ab"));

    if (!dat_->is_open())
    {
      yae_wlog("fopen failed for: %s", filepath.c_str());
      dat_.reset();
    }

    return dat_;
  }

  //----------------------------------------------------------------
  // Recording::write
  //
  void
  Recording::write(const fs::path & basedir, const yae::IBuffer & data)
  {
    open_mpg(basedir);
    if (!mpg_)
    {
      return;
    }

    open_dat(basedir);
    write_dat();

    YAE_EXPECT(mpg_->write(data.get(), data.size()));
    mpg_size_ += data.size();
  }

  //----------------------------------------------------------------
  // Recording::write_dat
  //
  void
  Recording::write_dat()
  {
    if (!dat_)
    {
      return;
    }

    int64_t time_now = yae::TTime::now().get(1);
    int64_t elapsed_sec = time_now - dat_time_;

    if (elapsed_sec > 0)
    {
      dat_time_ = time_now;
      yae::Data payload(16);
      yae::Bitstream bs(payload);
      bs.write_bits(64, dat_time_);
      bs.write_bits(64, mpg_size_);
      YAE_ASSERT(dat_->write(payload.get(), payload.size()));
      dat_->flush();
    }
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const Recording & rec)
  {
    save(json["cancelled"], rec.cancelled_);
    save(json["utc_t0"], rec.utc_t0_);
    save(json["gps_t0"], rec.gps_t0_);
    save(json["gps_t1"], rec.gps_t1_);
    save(json["channel_major"], rec.channel_major_);
    save(json["channel_minor"], rec.channel_minor_);
    save(json["channel_name"], rec.channel_name_);
    save(json["title"], rec.title_);
    save(json["rating"], rec.rating_);
    save(json["description"], rec.description_);

    if (rec.max_recordings_)
    {
      save(json["max_recordings"], rec.max_recordings_);
    }

#ifndef NDEBUG
    std::ostringstream oss;
    oss << unix_epoch_time_to_localtime_str(rec.utc_t0_) << " "
        << rec.channel_major_ << "."
        << rec.channel_minor_ << " "
        << rec.title_ << " (now "
        << unix_epoch_time_to_localtime_str(TTime::now().get(1)) << ")";
    save(json["_debug"], oss.str());
#endif
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, Recording & rec)
  {
    load(json["cancelled"], rec.cancelled_);
    load(json["utc_t0"], rec.utc_t0_);
    load(json["gps_t0"], rec.gps_t0_);
    load(json["gps_t1"], rec.gps_t1_);
    load(json["channel_major"], rec.channel_major_);
    load(json["channel_minor"], rec.channel_minor_);
    load(json["channel_name"], rec.channel_name_);
    load(json["title"], rec.title_);
    load(json["rating"], rec.rating_);
    load(json["description"], rec.description_);

    if (json.isMember("max_recordings"))
    {
      load(json["max_recordings"], rec.max_recordings_);
    }
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

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
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

        if (want->skip_duplicates())
        {
          TRecordingPtr rec_ptr = dvr.already_recorded(channel, program);
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
        TRecordingPtr & rec_ptr = recordings_[ch_num][program.gps_time_];

        if (!rec_ptr)
        {
          rec_ptr.reset(new Recording());
        }

        Recording & rec = *rec_ptr;
        rec.made_by_ = rec_cause;
        rec.utc_t0_ = localtime_to_unix_epoch_time(program.tm_);
        rec.gps_t0_ = program.gps_time_;
        rec.gps_t1_ = gps_t1;
        rec.channel_major_ = channel.major_;
        rec.channel_minor_ = channel.minor_;
        rec.channel_name_ = channel.name_;
        rec.title_ = program.title_;
        rec.rating_ = program.rating_;
        rec.description_ = program.description_;
        rec.max_recordings_ = want->max_recordings();
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
        const TRecordingPtr & rec_ptr = j->second;
        const Recording & rec = *rec_ptr;

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

        updated_schedule[ch_num][gps_t0] = rec_ptr;
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
    boost::unique_lock<boost::mutex> lock(mutex_);
    recordings = recordings_;
  }

  //----------------------------------------------------------------
  // next
  //
  TRecordingPtr
  next(const TRecordingPtr & after_this,
       const std::map<uint32_t, TScheduledRecordings> & recordings,
       uint32_t ch_num,
       uint32_t gps_time)
  {
    std::map<uint32_t, TScheduledRecordings>::const_iterator
      ch_found = recordings.find(ch_num);

    if (ch_found == recordings.end())
    {
      // nothing scheduled for this channel:
      return TRecordingPtr();
    }

    // recordings are indexed by GPS start time:
    const TScheduledRecordings & schedule = ch_found->second;
    if (schedule.empty())
    {
      // nothing scheduled for this channel:
      return TRecordingPtr();
    }

    uint32_t schedule_t0 = schedule.begin()->first;
    uint32_t schedule_t1 = schedule.rbegin()->second->gps_t1_;
    if (gps_time < schedule_t0 || schedule_t1 <= gps_time)
    {
      // nothing scheduled at given time:
      return TRecordingPtr();
    }

    // find the earliest recording with start time greater than gps_time:
    uint32_t rec_gps_t0 = 0;
    TRecordingPtr rec_ptr;

    TScheduledRecordings::const_iterator it = schedule.upper_bound(gps_time);
    if (it == schedule.end())
    {
      TScheduledRecordings::const_reverse_iterator rit = schedule.rbegin();
      it = yae::next(rit).base();
      YAE_ASSERT(rit->second == it->second);
    }
    else if (it != schedule.begin())
    {
      --it;
    }

    while (it != schedule.end())
    {
      rec_gps_t0 = it->first;
      rec_ptr = it->second;

      if (!after_this || after_this->gps_t0_ < rec_ptr->gps_t0_)
      {
        break;
      }

      rec_ptr.reset();
      ++it;
    }

    if (!rec_ptr)
    {
      return TRecordingPtr();
    }

    const Recording & rec = *rec_ptr;
    if (!after_this && (gps_time < rec_gps_t0 || rec.gps_t1_ <= gps_time))
    {
      return after_this;
    }

    return rec_ptr;
  }

  //----------------------------------------------------------------
  // find
  //
  TRecordingPtr
  find(const std::map<uint32_t, TScheduledRecordings> & recordings,
       uint32_t ch_num,
       uint32_t gps_time)
  {
    return yae::next(TRecordingPtr(), recordings, ch_num, gps_time);
  }

  //----------------------------------------------------------------
  // Schedule::get
  //
  TRecordingPtr
  Schedule::get(uint32_t ch_num, uint32_t gps_time) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return yae::find(recordings_, ch_num, gps_time);
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

    if (leading && leading->cancelled_)
    {
      leading.reset();
    }

    if (trailing && trailing->cancelled_)
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

    Recording & rec = *(found_rec->second);
    rec.cancelled_ = !(rec.cancelled_);

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

    found_rec->second->cancelled_ = true;
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
        if (rec_ptr && rec_ptr->made_by_ == Recording::kWishlistItem)
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

    yae::RingBuffer & ring_buffer = packet_handler_.ring_buffer_;
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
            yae_wlog("%sTSPacket too short (%i bytes), %s ...",
                     ctx.log_prefix_.c_str(),
                     bytes_consumed,
                     yae::to_hex(pkt_data->get(), 32, 4).c_str());
            continue;
          }

          ctx.push(pkt);

          yae::mpeg_ts::IPacketHandler::Packet packet(pkt.pid_, pkt_data);
          ctx.handle(packet, packet_handler_);
        }
        catch (const std::exception & e)
        {
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("%sfailed to parse %s: %s",
                   ctx.log_prefix_.c_str(),
                   data_hex.c_str(),
                   e.what());
        }
        catch (...)
        {
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("%sfailed to parse %s: unexpected exception",
                   ctx.log_prefix_.c_str(),
                   data_hex.c_str());
        }
      }
    }
  }


  //----------------------------------------------------------------
  // DVR::PacketHandler::PacketHandler
  //
  DVR::PacketHandler::PacketHandler(DVR & dvr):
    dvr_(dvr),
    ring_buffer_(188 * 262144),
    packets_(400000), // 75.2MB
    recordings_update_gps_time_(0)
  {
    worker_.set_queue_size_limit(1);
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::~PacketHandler
  //
  DVR::PacketHandler::~PacketHandler()
  {
    ring_buffer_.close();
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
    packets_.push(pkt);

    if (bucket.guide_.empty() ||
        (!packets_.full() && !bucket.vct_table_set_.is_complete()))
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
        const yae::IBuffer & data)
  {
    for (std::set<TRecordingPtr>::const_iterator
           i = recs.begin(); i != recs.end(); ++i)
    {
      Recording & rec = *(*i);
      rec.write(dvr.basedir_, data);
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
      yae::Timesheet::Probe probe(ctx_.timesheet_,
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
#if 0
      double ring_buffer_occupancy = ring_buffer_.occupancy();
      yae_ilog("%sring buffer occupancy: %f",
               ctx_.log_prefix_.c_str(),
               ring_buffer_occupancy);
#endif
    }

    yae::Timesheet::Probe probe(ctx_.timesheet_,
                                "PacketHandler::handle_backlog",
                                "write");


    yae::mpeg_ts::IPacketHandler::Packet pkt;
    while (packets_.pop(pkt))
    {
      const yae::IBuffer & data = *(pkt.data_);

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

      YAE_ASSERT(session_ptr);
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
      packet_handler_.reset(new PacketHandler(dvr_));
      dvr_.packet_handler_[frequency] = packet_handler_;
    }

    PacketHandler & packet_handler = *packet_handler_;
    packet_handler.packets_.clear();
    packet_handler.ring_buffer_.open(188 * 262144);

    if (session_)
    {
      std::ostringstream oss;
      std::string tuner_name = session_->tuner_name();
      oss << tuner_name << " " << frequency << "Hz";

      uint16_t channel_major = dvr_.get_channel_major(frequency_);
      if (channel_major)
      {
        oss << ", channel major " << channel_major;
      }

      oss << ", ";

      packet_handler.ctx_.log_prefix_ = oss.str().c_str();
      yae_ilog("stream start: %s", packet_handler.ctx_.log_prefix_.c_str());
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

    yae_ilog("stream stop: %s", packet_handler.ctx_.log_prefix_.c_str());
    packet_handler.ring_buffer_.close();

    // it's as ready as it's going to be:
    epg_ready_.notify_all();

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
    if (!packet_handler.ring_buffer_.is_open())
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
    yae::RingBuffer & ring_buffer = packet_handler.ring_buffer_;
    yae::mpeg_ts::Context & ctx = packet_handler.ctx_;

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

    // check if Channel Guide extends to 9 hours from now
    {
      static const TTime nine_hours(9 * 60 * 60, 1);
      int64_t t = (TTime::now() + nine_hours).get(1);
      if (ctx.channel_guide_overlaps(t))
      {
        epg_ready_.notify_all();
      }
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

    yae::mpeg_ts::EPG epg;
    dvr_.get_epg(epg);
    dvr_.cache_epg(epg);
    dvr_.update_epg();

    // pull EPG, evaluate wishlist, start captures, etc...
    while (!keep_going_.stop_)
    {
      now = TTime::now().rebased(1);

      if (dvr_.load_wishlist())
      {
        dvr_.save_schedule();
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
        bool blacklist_changed = dvr_.load_blacklist();

        if (blacklist_changed || dvr_.next_epg_refresh() <= now)
        {
          dvr_.set_next_epg_refresh(now + dvr_.schedule_refresh_period_ * 2.0);
          dvr_.update_epg();
        }

        if (dvr_.next_channel_scan() <= now)
        {
          dvr_.set_next_channel_scan(now + dvr_.channel_scan_period_);
          dvr_.scan_channels();
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
    yaetv_(yaetv_dir),
    basedir_(basedir.empty() ? yae::get_temp_dir_utf8() : basedir),
    channel_scan_period_(24 * 60 * 60, 1),
    epg_refresh_period_(30 * 60, 1),
    schedule_refresh_period_(30, 1),
    margin_(60, 1)
  {
    YAE_THROW_IF(!yae::mkdir_p(yaetv_.string()));
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

    // load the blacklist:
    load_blacklist();

    // load the wishlist:
    load_wishlist();

    // load the schedule:
    {
      std::string path = (yaetv_ / "schedule.json").string();
      Json::Value json;
      yae::TOpenFile(path, "rb").load(json);
      schedule_.load(json);
    }

    yae_ilog("DVR start, recordings storage: %s", basedir.c_str());
    basedir_ = basedir.empty() ? yae::get_temp_dir_utf8() : basedir;

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
      for_each_file_at(yaetv_.string(), collect_files);
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
    for (std::map<std::string, yae::TChannels>::const_iterator
           i = frequencies.begin(); i != frequencies.end(); ++i)
    {
      const std::string & frequency = i->first;
      const yae::TChannels & channels = i->second;

      if (channels.empty())
      {
        continue;
      }

      std::string epg_path =
        (yaetv_ / ("epg-" + frequency + ".json")).string();

      TPacketHandlerPtr & packet_handler_ptr = packet_handler_[frequency];
      packet_handler_ptr.reset(new PacketHandler(*this));

      Json::Value epg;
      if (yae::TOpenFile(epg_path, "rb").load(epg))
      {
        PacketHandler & packet_handler = *packet_handler_ptr;
        packet_handler.ctx_.load(epg[frequency]);
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::shutdown
  //
  void
  DVR::shutdown()
  {
    yae_ilog("DVR shutdown");
    service_loop_worker_.reset();

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
        ph.ring_buffer_.close();
        ph.worker_.stop();
        ph.worker_.wait_until_finished();

        // save timesheet to disk:
        yae::mpeg_ts::Context & ctx = ph.ctx_;
        std::string timesheet = ctx.timesheet_.to_str();
        std::string fn = strfmt("timesheet-dvr.%s.log",
                                ctx.log_prefix_.c_str());
        fn = sanitize_filename_utf8(fn);
        fn = (fs::path(yae::get_temp_dir_utf8()) / fn).string();
        yae::TOpenFile(fn, "ab").write(timesheet);
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

    ctx.dump();

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
      handler_ptr.reset(new DVR::PacketHandler(dvr_));
    }

    const DVR::PacketHandler & packet_handler = *handler_ptr;
    const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
#if 0
    const yae::mpeg_ts::Bucket & bucket = ctx.get_current_bucket();
    TTime elapsed_time_since_mgt = bucket.elapsed_time_since_mgt();
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
      yae_ilog("%s channel scan failed to tune to %u",
               session_ptr->device_name().c_str(),
               tuner_channel.frequency_);
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
        if (stream.epg_ready_.timed_wait(lock, giveup_at))
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

      // FIXME: pkoshevoy: check whether device is enabled in settings:
      const TunerDevicePtr & device_ptr = *i;
      const TunerDevice & device = *device_ptr;

      Json::Value tuner_cache;
      dvr_.get_tuner_cache(device.name(), tuner_cache);
      bool rescan = false;

      int64_t timestamp = tuner_cache.get("timestamp", 0).asInt64();
      int64_t now = yae::TTime::now().get(1);
      int64_t elapsed = now - timestamp;
      int64_t threshold = 10 * 24 * 60 * 60;

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
              dvr_.no_signal(frequency);
            }
            continue;
          }

          DVR::TPacketHandlerPtr & handler_ptr =
            dvr_.packet_handler_[frequency];
          YAE_ASSERT(handler_ptr);

          const DVR::PacketHandler & packet_handler = *handler_ptr;
          const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
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
    UpdateProgramGuide(DVR & dvr);

    // virtual:
    void execute(const yae::Worker & worker);

    // helper:
    DVR & dvr_;
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

    static const TTime sample_dur(30, 1);
    std::map<uint32_t, std::string> channels;
    dvr_.get_channels(channels);

    std::list<std::string> frequencies;
    std::map<std::string, uint16_t> channel_major;

#if 1
    for (std::map<uint32_t, std::string>::const_iterator
           i = channels.begin(); i != channels.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      uint16_t major = yae::mpeg_ts::channel_major(ch_num);

      DVR::Blacklist blacklist;
      dvr_.get(blacklist);

      if (has(blacklist.channels_, ch_num))
      {
        uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
        yae_ilog("skipping EPG update for blacklisted channel %i.%i",
                 int(major),
                 int(minor));
        continue;
      }

      const std::string & frequency = i->second;
      if (frequencies.empty() || frequencies.back() != frequency)
      {
        frequencies.push_back(frequency);
      }

      channel_major[frequency] = major;
    }
#else
    {
      std::string frequency = "503000000"; // 14.*
      frequencies.push_back(frequency);
      channel_major[frequency] = 14;
    }
#endif

    for (std::list<std::string>::const_iterator
           i = frequencies.begin(); i != frequencies.end(); ++i)
    {
      if (yae::Worker::Task::cancelled_)
      {
        return;
      }

      // shortuct:
      const std::string & frequency = *i;
      uint16_t major = yae::at(channel_major, frequency);

      DVR::TPacketHandlerPtr & handler_ptr = dvr_.packet_handler_[frequency];
      if (!handler_ptr)
      {
        handler_ptr.reset(new DVR::PacketHandler(dvr_));
      }

      const DVR::PacketHandler & packet_handler = *handler_ptr;
      const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
      const yae::mpeg_ts::Bucket & bucket = ctx.get_current_bucket();

      TTime elapsed_time_since_mgt = bucket.elapsed_time_since_mgt();
      YAE_ASSERT(elapsed_time_since_mgt.time_ >= 0);

      if (elapsed_time_since_mgt < dvr_.epg_refresh_period_)
      {
        yae_ilog("skipping EPG update for channels %i.* (%s)",
                 major,
                 frequency.c_str());
        // yae::save_epg(dvr_, frequency, ctx);
      }
      else
      {
        DVR::TStreamPtr stream_ptr =
          dvr_.capture_stream(frequency, sample_dur);
        if (stream_ptr)
        {
          HDHomeRun::TSessionPtr session_ptr = stream_ptr->session_;
          std::string device_name = session_ptr->device_name();
          Json::Value tuner_cache;
          dvr_.get_tuner_cache(device_name, tuner_cache);
          Json::Value & cache = tuner_cache["frequencies"][frequency];

          // wait until EPG is ready:
          DVR::Stream & stream = *stream_ptr;
          boost::system_time giveup_at(boost::get_system_time());
          giveup_at += boost::posix_time::seconds(sample_dur.get(1));

          yae_ilog("%sstarted EPG update for channels %i.* (%s)",
                   ctx.log_prefix_.c_str(),
                   major,
                   frequency.c_str());

          while (true)
          {
            boost::unique_lock<boost::mutex> lock(mutex_);
            if (yae::Worker::Task::cancelled_)
            {
              return;
            }

            try
            {
              if (stream.epg_ready_.timed_wait(lock, giveup_at))
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

          TunerStatus tuner_status;
          session_ptr->get_tuner_status(tuner_status);
          yae::update_tuner_cache(cache, tuner_status, ctx);

          int64_t now = yae::TTime::now().get(1);
          cache["timestamp"] = (Json::Value::Int64)now;
          dvr_.update_tuner_cache(device_name, tuner_cache);

          yae::save_epg(dvr_, frequency, ctx);
        }
        else
        {
          yae_wlog("failed to start EPG update for channels %i.* (%s)",
                   major,
                   frequency.c_str());
          continue;
        }
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
    boost::unique_lock<boost::mutex> lock(mutex_);
    ph = packet_handler_;
  }

  //----------------------------------------------------------------
  // DVR::get
  //
  void
  DVR::get(Blacklist & blacklist) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    blacklist = blacklist_;
  }

  //----------------------------------------------------------------
  // DVR::get
  //
  void
  DVR::get(std::map<std::string, Wishlist::Item> & wishlist) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    wishlist_.get(wishlist);
  }

  //----------------------------------------------------------------
  // DVR::wishlist_remove
  //
  bool
  DVR::wishlist_remove(const std::string & wi_key)
  {
    // update the wishlist:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      if (!wishlist_.remove(wi_key))
      {
        return false;
      }
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
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      wishlist_.update(wi_key, new_item);
    }

    save_wishlist();
  }

  //----------------------------------------------------------------
  // DVR::get_epg
  //
  void
  DVR::get_epg(yae::mpeg_ts::EPG & epg, const std::string & lang) const
  {
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
    Json::Value json;
    json["timestamp"] = Json::Int64(yae::TTime::now().get(1));
    ctx.save(json[frequency]);

    boost::unique_lock<boost::mutex> lock(mutex_);
    std::string epg_path = (yaetv_ / ("epg-" + frequency + ".json")).string();
    yae::TOpenFile epg_file;
    if (epg_file.open(epg_path, "wb"))
    {
      epg_file.save(json);
    }
  }

  //----------------------------------------------------------------
  // DVR::save_epg
  //
  void
  DVR::save_epg() const
  {
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
  // DVR::Blacklist::Blacklist
  //
  DVR::Blacklist::Blacklist():
    lastmod_(std::numeric_limits<int64_t>::min())
  {}

  //----------------------------------------------------------------
  // DVR::Blacklist::toggle
  //
  void
  DVR::Blacklist::toggle(uint32_t ch_num)
  {
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
  // DVR::toggle_blacklist
  //
  void
  DVR::toggle_blacklist(uint32_t ch_num)
  {
    // toggle the blacklist item:
    boost::unique_lock<boost::mutex> lock(mutex_);
    blacklist_.toggle(ch_num);
  }

  //----------------------------------------------------------------
  // DVR::save_blacklist
  //
  void
  DVR::save_blacklist() const
  {
    std::list<std::string> blacklist;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);

      for (std::set<uint32_t>::const_iterator i = blacklist_.channels_.begin();
           i != blacklist_.channels_.end(); ++i)
      {
        const uint32_t ch_num = *i;
        uint16_t major = yae::mpeg_ts::channel_major(ch_num);
        uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
        std::string ch_str = strfmt("%i.%i", int(major), int(minor));
        blacklist.push_back(ch_str);
      }
    }

    Json::Value json;
    yae::save(json, blacklist);

    std::string path = (yaetv_ / "blacklist.json").string();
    yae::TOpenFile(path, "wb").save(json);
  }

  //----------------------------------------------------------------
  // DVR::load_blacklist
  //
  bool
  DVR::load_blacklist()
  {
    try
    {
      std::string path = (yaetv_ / "blacklist.json").string();
      int64_t lastmod = yae::stat_lastmod(path.c_str());
      if (blacklist_.lastmod_ < lastmod)
      {
        struct tm tm;
        unix_epoch_time_to_localtime(lastmod, tm);
        std::string lastmod_txt = to_yyyymmdd_hhmmss(tm);
        yae_ilog("loading blacklist %s, lastmod %s",
                 path.c_str(),
                 lastmod_txt.c_str());

        Json::Value json;
        yae::TOpenFile(path, "rb").load(json);
        std::list<std::string> blacklist;
        yae::load(json, blacklist);

        boost::unique_lock<boost::mutex> lock(mutex_);
        blacklist_.channels_.clear();
        for (std::list<std::string>::const_iterator
               i = blacklist.begin(); i != blacklist.end(); ++i)
        {
          const std::string & ch_str = *i;
          uint32_t ch_num = parse_channel_str(ch_str);
          blacklist_.channels_.insert(ch_num);
        }

        blacklist_.lastmod_ = lastmod;
        return true;
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // DVR::save_wishlist
  //
  void
  DVR::save_wishlist() const
  {
    Json::Value json;
    yae::save(json, wishlist_);

    boost::unique_lock<boost::mutex> lock(mutex_);
    std::string path = (yaetv_ / "wishlist.json").string();
    yae::TOpenFile file;
    if (!(file.open(path, "wb") && file.save(json)))
    {
      yae_elog("write failed: %s", path.c_str());
    }
  }

  //----------------------------------------------------------------
  // DVR::load_wishlist
  //
  bool
  DVR::load_wishlist()
  {
    try
    {
      std::string path = (yaetv_ / "wishlist.json").string();
      int64_t lastmod = yae::stat_lastmod(path.c_str());
      if (wishlist_.lastmod_ < lastmod)
      {
        struct tm tm;
        unix_epoch_time_to_localtime(lastmod, tm);
        std::string lastmod_txt = to_yyyymmdd_hhmmss(tm);
        yae_ilog("loading wishlist %s, lastmod %s",
                 path.c_str(),
                 lastmod_txt.c_str());

        boost::unique_lock<boost::mutex> lock(mutex_);
        Json::Value json;
        if (yae::TOpenFile(path, "rb").load(json))
        {
          Wishlist wishlist;
          yae::load(json, wishlist);
          wishlist_.items_.swap(wishlist.items_);
          wishlist_.lastmod_ = lastmod;
          return true;
        }
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
    Json::Value json;
    schedule_.save(json);

    boost::unique_lock<boost::mutex> lock(mutex_);
    {
      std::string path = (yaetv_ / "schedule.json").string();
      yae::TOpenFile file;
      if (!(file.open(path, "wb") && file.save(json)))
      {
        yae_elog("write failed: %s", path.c_str());
      }
    }

    // and another one, for post-mortem debugging:
    {
      int64_t t = yae::TTime::now().get(1);
      t -= t % 1800; // round-down to half-hour:

      struct tm tm;
      yae::unix_epoch_time_to_localtime(t, tm);
      std::string fn = strfmt("schedule-%02i%02i.json", tm.tm_hour, tm.tm_min);
      std::string path = (yaetv_ / fn).string();
      yae::TOpenFile file;
      if (!(file.open(path, "wb") && file.save(json)))
      {
        yae_elog("write failed: %s", path.c_str());
      }
    }
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
      std::string path = (yaetv_ / name).string();

      // if cancelled, then un-cancel:
      remove_utf8((path + ".cancelled").c_str());

      // write it out, then close the file:
      {
        yae::TOpenFile file;
        if (!(file.open(path, "wb") && file.save(json)))
        {
          yae_elog("write failed: %s", path.c_str());
          return;
        }
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
    std::string path = (yaetv_ / name).string();

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
    // avoid race condition with Schedule::update:
    boost::unique_lock<boost::mutex> lock(mutex_);

    Json::Value json;
    yae::shared_ptr<Wishlist::Item> item_ptr;

    std::string name = wishlist_item_filename(channel, program);
    std::string path = (yaetv_ / name).string();

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
  static TRecordingPtr
  load_recording(const std::string & mpg)
  {
    try
    {
      std::string path = mpg.substr(0, mpg.size() - 4) + ".json";
      Json::Value json;
      yae::TOpenFile(path, "rb").load(json);
      TRecordingPtr rec_ptr(new Recording());
      yae::load(json, *rec_ptr);
      return rec_ptr;
    }
    catch (...)
    {}

    return TRecordingPtr();
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
      boost::unique_lock<boost::mutex> lock(mutex_);
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
  DVR::delete_recording(const Recording & rec)
  {
    // shortcut:
    uint32_t ch_num = rec.ch_num();

    // cancel recording, if recording:
    {
      TRecordingPtr rec_ptr = schedule_.get(rec.ch_num(), rec.gps_t0_);
      if (rec_ptr && !rec_ptr->cancelled_)
      {
        toggle_recording(ch_num, rec.gps_t0_);
      }
    }

    yae_ilog("deleting recording %s", rec.get_filepath(basedir_).c_str());
    remove_recording(rec.get_filepath(basedir_));
  }

  //----------------------------------------------------------------
  // remove_excess_recordings
  //
  void
  remove_excess_recordings(const fs::path & basedir,
                           const Recording & rec)
  {
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
    std::list<std::pair<std::string, TRecordingPtr> > recs;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & mpg = i->second;
      TRecordingPtr rec_ptr = load_recording(mpg);

      if (rec.utc_t0_ == rec_ptr->utc_t0_)
      {
        // ignore the current/in-progress recording:
        continue;
      }

      recs.push_back(std::make_pair(mpg, rec_ptr));
      num_recordings++;
    }

    std::size_t removed_recordings = 0;
    for (std::list<std::pair<std::string, TRecordingPtr> >::const_iterator
           i = recs.begin(); i != recs.end(); ++i)
    {
      if (num_recordings - removed_recordings <= rec.max_recordings_)
      {
        break;
      }

      const std::string & mpg = i->first;
      const TRecordingPtr & rec_ptr = i->second;
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
                const Recording & rec,
                uint64_t num_sec)
  {
    // remove any existing old recordings beyond max recordings limit:
    yae::remove_excess_recordings(basedir, rec);

    // must accommodate max 19.39Mbps ATSC transport stream:
    std::string title_path = rec.get_title_path(basedir).string();
    uint64_t title_bytes = (num_sec * 20000000) >> 3;

    uint64_t filesystem_bytes = 0;
    uint64_t filesystem_bytes_free = 0;
    uint64_t available_bytes = 0;

    if (!yae::stat_diskspace(basedir.string().c_str(),
                             filesystem_bytes,
                             filesystem_bytes_free,
                             available_bytes))
    {
      yae_elog("failed to query available disk space for %i.%i %s",
               rec.channel_major_,
               rec.channel_minor_,
               title_path.c_str());
      return false;
    }

    if (title_bytes < available_bytes)
    {
      return true;
    }

    std::map<std::string, std::string> recordings;
    {
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(basedir.string(), collect_recordings);
    }

    std::size_t removed_bytes = 0;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      if (title_bytes < available_bytes + removed_bytes)
      {
        break;
      }

      const std::string & mpg = i->second;
      removed_bytes += remove_recording(mpg);
    }

    if (title_bytes < available_bytes + removed_bytes)
    {
      return true;
    }

    yae_elog("failed to free up disk space (%" PRIu64 " MiB) for %i.%i %s",
             title_bytes >> 20,
             rec.channel_major_,
             rec.channel_minor_,
             rec.title_.c_str());
    return false;
  }

  //----------------------------------------------------------------
  // DVR::make_room_for
  //
  bool
  DVR::make_room_for(const Recording & rec, uint64_t num_sec)
  {
    cleanup_yaetv_dir();

    return yae::make_room_for(basedir_, rec, num_sec);
  }

  //----------------------------------------------------------------
  // DVR::already_recorded
  //
  TRecordingPtr
  DVR::already_recorded(const yae::mpeg_ts::EPG::Channel & channel,
                        const yae::mpeg_ts::EPG::Program & program) const
  {
    if (program.description_.empty())
    {
      // can't check for duplicates without a description:
      return TRecordingPtr();
    }

    Recording rec(channel, program);
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
      TRecordingPtr rec_ptr = load_recording(mpg);
      if (!rec_ptr)
      {
        continue;
      }

      const Recording & recorded = *rec_ptr;
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
                 recorded.get_basename().c_str(),
                 program.description_.c_str());
      }
    }

    return TRecordingPtr();
  }

  //----------------------------------------------------------------
  // DVR::get_recordings
  //
  void
  DVR::get_recordings(TRecordings & by_filename,
                      std::map<std::string, TRecordings> & by_playlist,
                      std::map<uint32_t, TScheduledRecordings> & by_chan) const
  {
    std::map<std::string, std::string> recordings;
    {
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(basedir_.string(), collect_recordings);
    }

    TRecordings rec_by_fn;
    std::map<std::string, TRecordings> rec_by_pl;
    std::map<uint32_t, TScheduledRecordings> rec_by_channel;

    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      const std::string & filename = i->first;
      const std::string & filepath = i->second;

      TRecordingPtr rec_ptr = yae::get(by_filename, filename, TRecordingPtr());
      if (!rec_ptr)
      {
        rec_ptr = load_recording(filepath);
      }

      if (!rec_ptr)
      {
        continue;
      }

      const Recording & recorded = *rec_ptr;
      std::string playlist = yae::strfmt("%02i.%02i %s",
                                         recorded.channel_major_,
                                         recorded.channel_minor_,
                                         recorded.title_.c_str());

      rec_by_fn[filename] = rec_ptr;
      rec_by_pl[playlist][filename] = rec_ptr;

      uint32_t ch_num = yae::mpeg_ts::channel_number(recorded.channel_major_,
                                                     recorded.channel_minor_);
      rec_by_channel[ch_num][recorded.gps_t0_] = rec_ptr;
    }

    by_filename.swap(rec_by_fn);
    by_playlist.swap(rec_by_pl);
    by_chan.swap(rec_by_channel);
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
      std::string frequency = yae::at(frequencies, live_ch);

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
        Recording & rec = *(*j);
        bool is_recording = rec.stream_ && rec.stream_->is_open();
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

        rec.stream_ = stream;
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::get_cached_epg
  //
  bool
  DVR::get_cached_epg(TTime & lastmod, yae::mpeg_ts::EPG & epg) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (lastmod.invalid() || lastmod < epg_lastmod_)
    {
      lastmod = epg_lastmod_;
      epg = epg_;
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
          channel_frequency_lut_[ch_num] = frequency;
        }

        if (channels.empty())
        {
          frequency_channel_lut_.erase(frequency);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  void
  DVR::get_channels(std::map<uint32_t, std::string> & chan_freq) const
  {
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    chan_freq = channel_frequency_lut_;
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  void
  DVR::get_channels(std::map<std::string, TChannels> & channels) const
  {
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    channels = frequency_channel_lut_;
  }

  //----------------------------------------------------------------
  // DVR::get_channels
  //
  bool
  DVR::get_channels(const std::string & freq, TChannels & channels) const
  {
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
  // DVR::get_channel_major
  //
  uint16_t
  DVR::get_channel_major(const std::string & frequency) const
  {
    boost::unique_lock<boost::mutex> lock(tuner_cache_mutex_);
    std::map<std::string, TChannels>::const_iterator found =
      frequency_channel_lut_.find(frequency);
    if (found == frequency_channel_lut_.end())
    {
      return 0;
    }

    const TChannels & channels = found->second;
    if (channels.empty())
    {
      return 0;
    }

    YAE_ASSERT(channels.size() == 1);
    uint16_t major = channels.begin()->first;
    return major;
  }

  //----------------------------------------------------------------
  // DVR::get_channel_name
  //
  bool
  DVR::get_channel_name(uint16_t major,
                        uint16_t minor,
                        std::string & name) const
  {
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
  // DVR::has_preferences
  //
  bool
  DVR::has_preferences() const
  {
    boost::unique_lock<boost::mutex> lock(preferences_mutex_);
    return !preferences_.empty();
  }

  //----------------------------------------------------------------
  // DVR::get_preferences
  //
  void
  DVR::get_preferences(Json::Value & preferences) const
  {
    boost::unique_lock<boost::mutex> lock(preferences_mutex_);
    preferences = preferences_;
  }

  //----------------------------------------------------------------
  // DVR::set_preferences
  //
  void
  DVR::set_preferences(const Json::Value & preferences)
  {
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
  DVR::discover_enabled_tuners(std::set<std::string> & tuner_names)
  {
    Json::Value tuners;
    {
      boost::unique_lock<boost::mutex> lock(preferences_mutex_);
      tuners = preferences_.get("tuners", Json::Value(Json::objectValue));
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
        bool enabled = tuners.get(tuner_name, true).asBool();
        if (enabled)
        {
          tuner_names.insert(tuner_name);
        }
      }
    }

    return !tuner_names.empty();
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

    // remove all logs except 3 most recent:
    std::map<std::string, std::string>::reverse_iterator it = logs.rbegin();
    for (int i = 0; i < 3 && it != logs.rend(); i++, ++it) {}

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
