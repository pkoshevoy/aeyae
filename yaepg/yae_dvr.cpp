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

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// yae:
#include "yae/api/yae_log.h"

// epg:
#include "yae_dvr.h"


namespace yae
{

  //----------------------------------------------------------------
  // Wishlist::Wishlist
  //
  Wishlist::Wishlist():
    lastmod_(std::numeric_limits<int64_t>::min())
  {}

  //----------------------------------------------------------------
  // Wishlist::Item::matches
  //
  bool
  Wishlist::Item::matches(const yae::mpeg_ts::EPG::Channel & channel,
                          const yae::mpeg_ts::EPG::Program & program) const
  {
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
      YAE_EXPECT(tm.tm_gmtoff == program.tm_.tm_gmtoff);

      if (tm.tm_year != program.tm_.tm_year ||
          tm.tm_mon  != program.tm_.tm_mon  ||
          tm.tm_mday != program.tm_.tm_mday)
      {
        return false;
      }
    }

    if (when_)
    {
      const Timespan & timespan = *when_;
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

    if (weekday_mask_)
    {
      uint8_t weekday_mask = *weekday_mask_;
      uint8_t program_wday = (1 << program.tm_.tm_wday);
      if ((weekday_mask & program_wday) != program_wday)
      {
        return false;
      }
    }

    bool ok = false;
    if (!title_.empty())
    {
      if (!rx_title_)
      {
        rx_title_.reset(boost::regex(title_, boost::regex::icase));
      }

      if (program.title_ == title_ ||
          boost::regex_match(program.title_, *rx_title_))
      {
        ok = true;
      }
    }

    if (!description_.empty())
    {
      if (!rx_description_)
      {
        rx_description_.reset(boost::regex(description_, boost::regex::icase));
      }

      if (program.description_ == description_ ||
          boost::regex_match(program.description_, *rx_description_))
      {
        ok = true;
      }
    }

    return ok || (channel_ && (date_ || weekday_mask_ || when_));
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

    if (when_)
    {
      Json::Value & when = json["when"];
      when["t0"] = when_->t0_.to_hhmmss();
      when["t1"] = when_->t1_.to_hhmmss();
    }

    if (weekday_mask_)
    {
      static const char * alphabet[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
      };

      const uint16_t weekday_mask = *weekday_mask_;
      const char * separator = "";
      std::ostringstream oss;
      for (int i = 0; i < 7; i++)
      {
        int j = (i + 1) % 7;
        uint16_t required = (1 << j);
        if ((weekday_mask & required) == required)
        {
          oss << separator << alphabet[j];
          separator = " ";
        }
      }

      json["weekdays"] = oss.str();
    }

    yae::save(json, "date", date_);
    yae::save(json, "title", title_);
    yae::save(json, "description", description_);
    yae::save(json, "max_recordings", max_recordings_);
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

      std::vector<std::string> tokens;
      YAE_ASSERT(yae::split(tokens, ".", major_minor.c_str()) == 2);

      channel.first = boost::lexical_cast<uint16_t>(tokens[0]);
      channel.second = boost::lexical_cast<uint16_t>(tokens[1]);
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
      when_.reset(ts);
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

      weekday_mask_.reset(weekday_mask);
    }

    yae::load(json, "date", date_);
    yae::load(json, "title", title_);
    yae::load(json, "description", description_);
    yae::load(json, "max_recordings", max_recordings_);
  }

  //----------------------------------------------------------------
  // Wishlist::matches
  //
  const Wishlist::Item *
  Wishlist::matches(const yae::mpeg_ts::EPG::Channel & channel,
                    const yae::mpeg_ts::EPG::Program & program) const
  {
    for (std::list<Item>::const_iterator
           i = items_.begin(); i != items_.end(); ++i)
    {
      const Item & item = *i;
      if (item.matches(channel, program))
      {
        return &item;
      }
    }

    return NULL;
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
    cancelled_(false),
    utc_t0_(0),
    gps_t0_(0),
    gps_t1_(0),
    channel_major_(0),
    channel_minor_(0),
    max_recordings_(0)
  {}

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

    struct tm tm = { 0 };
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
  // Recording::open_file
  //
  yae::TOpenFilePtr
  Recording::open_file(const fs::path & basedir)
  {
    if (file_ && file_->is_open())
    {
      return file_;
    }

    file_.reset();

    fs::path title_path = get_title_path(basedir);
    std::string title_path_str = title_path.string();
    if (!yae::mkdir_p(title_path_str))
    {
      yae_elog("mkdir_p failed for: %s", title_path_str.c_str());
      return TOpenFilePtr();
    }

    std::string basename = get_basename();
    std::string basepath = (title_path / basename).string();
    std::string filepath = basepath + ".ts";
    file_.reset(new yae::TOpenFile(filepath, "ab"));
    bool ok = file_->is_open();

    yae_ilog("writing to: %s, %s", filepath.c_str(), ok ? "ok" : "failed");
    if (!ok)
    {
      file_.reset();
      yae_elog("fopen failed for: %s", filepath.c_str());
    }
    else
    {
      Json::Value json;
      yae::save(json, *this);
      std::string filepath = basepath + ".json";
      if (!yae::TOpenFile(filepath, "wb").save(json))
      {
        yae_wlog("fopen failed for: %s", filepath.c_str());
      }
    }

    return file_;
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
  // Schedule::update
  //
  void
  Schedule::update(const yae::mpeg_ts::EPG & epg,
                   const Wishlist & wishlist)
  {
    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;

      for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
             j = channel.programs_.begin(); j != channel.programs_.end(); ++j)
      {
        const yae::mpeg_ts::EPG::Program & program = *j;

        uint32_t gps_t1 = program.gps_time_ + program.duration_;
        if (gps_t1 <= channel.gps_time())
        {
          // it's in the past:
          continue;
        }

        const Wishlist::Item * want = wishlist.matches(channel, program);
        if (!want)
        {
          continue;
        }

        boost::unique_lock<boost::mutex> lock(mutex_);
        TRecordingPtr & rec_ptr = recordings_[ch_num][program.gps_time_];
        if (!rec_ptr)
        {
          rec_ptr.reset(new Recording());
        }

        Recording & rec = *rec_ptr;
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
    std::map<uint32_t, TScheduledRecordings> updated_schedule;

    boost::unique_lock<boost::mutex> lock(mutex_);
    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = recordings_.begin(); i != recordings_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
        ch_found = epg.channels_.find(ch_num);
      if (ch_found == epg.channels_.end())
      {
        continue;
      }

      const yae::mpeg_ts::EPG::Channel & channel = ch_found->second;
      const TScheduledRecordings & schedule = i->second;
      for (TScheduledRecordings::const_iterator
             j = schedule.begin(); j != schedule.end(); ++j)
      {
        const uint32_t gps_t0 = j->first;
        const TRecordingPtr & rec_ptr = j->second;
        const Recording & rec = *rec_ptr;

        if (rec.gps_t1_ < channel.gps_time())
        {
          // it's in the past:
          continue;
        }

        updated_schedule[ch_num][gps_t0] = rec_ptr;
      }
    }

    recordings_.swap(updated_schedule);
  }

  //----------------------------------------------------------------
  // Schedule::get
  //
  TRecordingPtr
  Schedule::get(uint32_t ch_num, uint32_t gps_time) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    std::map<uint32_t, TScheduledRecordings>::const_iterator
      ch_found = recordings_.find(ch_num);

    if (ch_found == recordings_.end())
    {
      // nothing scheduled for this channel:
      return TRecordingPtr();
    }

    // recordings are indexed by GPS end time:
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
      TScheduledRecordings::const_reverse_iterator it = schedule.rbegin();
      rec_gps_t0 = it->first;
      rec_ptr = it->second;
    }
    else if (it != schedule.begin())
    {
      --it;
      rec_gps_t0 = it->first;
      rec_ptr = it->second;
    }

    if (!rec_ptr)
    {
      return TRecordingPtr();
    }

    const Recording & rec = *rec_ptr;
    if (gps_time < rec_gps_t0 || rec.gps_t1_ <= gps_time)
    {
      return TRecordingPtr();
    }

    return rec_ptr;
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
  // save
  //
  void
  Schedule::save(Json::Value & json) const
  {
    yae::save(json["recordings"], recordings_);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  Schedule::load(const Json::Value & json)
  {
    yae::load(json["recordings"], recordings_);
  }


  //----------------------------------------------------------------
  // ParseStream
  //
  struct ParseStream : yae::Worker::Task
  {
    ParseStream(DVR::PacketHandler & packet_handler,
                const std::string & tuner_name,
                const std::string & frequency,
                std::size_t size);

    // virtual:
    void execute(const yae::Worker & worker);

    DVR::PacketHandler & packet_handler_;
    std::string tuner_name_;
    std::string frequency_;
    std::size_t size_;
  };

  //----------------------------------------------------------------
  // ParseStream::ParseStream
  //
  ParseStream::ParseStream(DVR::PacketHandler & packet_handler,
                           const std::string & tuner_name,
                           const std::string & frequency,
                           std::size_t size):
    packet_handler_(packet_handler),
    tuner_name_(tuner_name),
    frequency_(frequency),
    size_(size)
  {}

  //----------------------------------------------------------------
  // ParseStream::execute
  //
  void
  ParseStream::execute(const yae::Worker & worker)
  {
    (void)worker;

    yae::RingBuffer & ring_buffer = packet_handler_.ring_buffer_;
    // TOpenFile & file = *(packet_handler_.file_);
    yae::mpeg_ts::Context & ctx = packet_handler_.ctx_;

    yae::TTime start = TTime::now();
    std::size_t done = 0;
    while (true)
    {
      std::size_t todo = std::min<std::size_t>(188 * 7, size_ - done);
      if (todo < 188)
      {
        YAE_EXPECT(!todo);
        break;
      }

      yae::Data data(todo);
      std::size_t size = ring_buffer.pull(data.get(), data.size());

      if (!size)
      {
        if (!ring_buffer.is_open())
        {
          break;
        }

        continue;
      }

      done += size;
      data.truncate(size);
      // file.write(data.get(), size);

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
            yae_wlog("TSPacket too short (%i bytes), %s ...",
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

          yae_wlog("failed to parse %s, tuner %s, %sHz: %s",
                   data_hex.c_str(),
                   tuner_name_.c_str(),
                   frequency_.c_str(),
                   e.what());
        }
        catch (...)
        {
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("failed to parse %s..., tuner %s, %sHz %s: "
                   "unexpected exception",
                   data_hex.c_str(),
                   tuner_name_.c_str(),
                   frequency_.c_str());
        }
      }
    }
  }


  //----------------------------------------------------------------
  // DVR::PacketHandler::PacketHandler
  //
  DVR::PacketHandler::PacketHandler(DVR & dvr):
    dvr_(dvr),
    ring_buffer_(188 * 4096),
    packets_(400000) // 75.2MB
  {}

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
      yae::TOpenFilePtr file = rec.open_file(dvr.basedir_);
      if (file)
      {
        YAE_ASSERT(file->write(data.get(), data.size()));
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::handle_backlog
  //
  void
  DVR::PacketHandler::handle_backlog(const yae::mpeg_ts::Bucket & bucket,
                                     uint32_t gps_time)
  {
    uint32_t margin = dvr_.margin_.get(1);
    std::map<uint32_t, std::set<TRecordingPtr> > recordings;

    for (std::map<uint32_t, yae::mpeg_ts::ChannelGuide>::const_iterator
           i = bucket.guide_.begin(); i != bucket.guide_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      std::set<TRecordingPtr> recs;
      dvr_.schedule_.get(recs, ch_num, gps_time, margin);
      if (!recs.empty())
      {
        recordings[ch_num].swap(recs);
      }
    }

    yae::mpeg_ts::IPacketHandler::Packet pkt;
    while (packets_.pop(pkt))
    {
      const yae::IBuffer & data = *(pkt.data_);

      std::map<uint16_t, uint32_t>::const_iterator found =
        bucket.pid_to_ch_num_.find(pkt.pid_);

      if (found == bucket.pid_to_ch_num_.end())
      {
        for (std::map<uint32_t, std::set<TRecordingPtr> >::const_iterator
               it = recordings.begin(); it != recordings.end(); ++it)
        {
          const std::set<TRecordingPtr> & recs = it->second;
          write(dvr_, recs, data);
        }
      }
      else
      {
        const uint32_t ch_num = found->second;
        std::map<uint32_t, std::set<TRecordingPtr> >::const_iterator
          it = recordings.find(ch_num);
        if (it != recordings.end())
        {
          const std::set<TRecordingPtr> & recs = it->second;
          write(dvr_, recs, data);
        }
      }
    }
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
    packet_handler.ring_buffer_.open(188 * 4096);
  }

  //----------------------------------------------------------------
  // DVR::Stream::~Stream
  //
  DVR::Stream::~Stream()
  {}

  //----------------------------------------------------------------
  // DVR::Stream::open
  //
  void
  DVR::Stream::open(const DVR::TStreamPtr & stream_ptr)
  {
    yae::shared_ptr<CaptureStream, yae::Worker::Task> task;
    task.reset(new CaptureStream(stream_ptr));

    worker_ = dvr_.get_stream_worker(frequency_);
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
    packet_handler.ring_buffer_.close();

    // it's as ready as it's going to be:
    epg_ready_.notify_all();
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

#if 0
    std::string data_hex =
      yae::to_hex(data, std::min<std::size_t>(size, 32), 4);

    yae_dlog("%s %sHz: %5i %s...",
             tuner_name.c_str(),
             frequency.c_str(),
             int(size),
             data_hex.c_str());
#endif

    PacketHandler & packet_handler = *packet_handler_;
    yae::RingBuffer & ring_buffer = packet_handler.ring_buffer_;
    yae::mpeg_ts::Context & ctx = packet_handler.ctx_;

    // check if Channel Guide extends to 9 hours from now
    {
      static const TTime nine_hours(9 * 60 * 60, 1);
      int64_t t = (TTime::now() + nine_hours).get(1);
      if (ctx.channel_guide_overlaps(t))
      {
        epg_ready_.notify_all();
      }
    }

    yae::shared_ptr<ParseStream, yae::Worker::Task> task;
    task.reset(new ParseStream(packet_handler,
                               session_->tuner_name(),
                               frequency_,
                               size));
    packet_handler.worker_.add(task);

    if (ring_buffer.push(data, size) != size)
    {
      return false;
    }

    return true;
  }


  //----------------------------------------------------------------
  // DVR::DVR
  //
  DVR::DVR(const std::string & basedir):
    yaepg_(yae::get_user_folder_path(".yaepg")),
    basedir_(basedir.empty() ? yae::get_temp_dir_utf8() : basedir),
    channel_scan_period_(24 * 60 * 60, 1),
    epg_refresh_period_(30 * 60, 1),
    margin_(60, 1)
  {
    YAE_ASSERT(yae::mkdir_p(yaepg_.string()));

    // load the frequencies:
    std::map<std::string, yae::mpeg_ts::TChannels> frequencies;
    try
    {
      std::string freq_path = (yaepg_ / "frequencies.json").string();
      Json::Value json;
      yae::TOpenFile(freq_path, "rb").load(json);
      yae::load(json, frequencies);
    }
    catch (...)
    {}

    scan_channels();

    if (frequencies.empty())
    {
      worker_.wait_until_finished();

      std::map<uint32_t, std::string> channels;
      hdhr_.get_channels(channels);

      for (std::map<uint32_t, std::string>::const_iterator
             i = channels.begin(); i != channels.end(); ++i)
      {
        const uint32_t ch_num = i->first;
        const std::string & frequency = i->second;
        uint16_t major = yae::mpeg_ts::channel_major(ch_num);
        uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);
        frequencies[frequency][major][minor] = std::string();
      }
    }

    // load the EPG:
    for (std::map<std::string, yae::mpeg_ts::TChannels>::const_iterator
           i = frequencies.begin(); i != frequencies.end(); ++i)
    {
      const std::string & frequency = i->first;
      std::string epg_path =
        (yaepg_ / ("epg-" + frequency + ".json")).string();

      Json::Value epg;
      if (yae::TOpenFile(epg_path, "rb").load(epg))
      {
        TPacketHandlerPtr & packet_handler_ptr = packet_handler_[frequency];
        packet_handler_ptr.reset(new PacketHandler(*this));

        PacketHandler & packet_handler = *packet_handler_ptr;
        packet_handler.ctx_.load(epg[frequency]);
      }
    }

    // load the wishlist:
    load_wishlist();

    // load the schedule:
    {
      std::string path = (yaepg_ / "schedule.json").string();
      Json::Value json;
      yae::TOpenFile(path, "rb").load(json);
      schedule_.load(json);
    }
  }

  //----------------------------------------------------------------
  // DVR::~DVR
  //
  DVR::~DVR()
  {
    shutdown();
  }

  //----------------------------------------------------------------
  // DVR::shutdown
  //
  void
  DVR::shutdown()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    worker_.stop();

    for (std::map<std::string, yae::weak_ptr<Stream, IStream> >::const_iterator
           i = stream_.begin(); i != stream_.end(); ++i)
    {
      TStreamPtr stream_ptr = i->second.lock();
      if (stream_ptr)
      {
        Stream & stream = *stream_ptr;
        stream.close();
      }
    }

    for (std::map<std::string, TWorkerPtr>::const_iterator
           i = stream_worker_.begin(); i != stream_worker_.end(); ++i)
    {
      TWorkerPtr worker_ptr = i->second;
      if (worker_ptr)
      {
        worker_ptr->stop();
        worker_ptr->wait_until_finished();
      }
    }
  }


  //----------------------------------------------------------------
  // ScanChannels
  //
  struct ScanChannels : yae::Worker::Task
  {
    ScanChannels(DVR & dvr);

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
  // ScanChannels::execute
  //
  void
  ScanChannels::execute(const yae::Worker & worker)
  {
    (void)worker;

    HDHomeRun::TSessionPtr session_ptr = dvr_.hdhr_.open_session();
    if (!session_ptr)
    {
      return;
    }

    dvr_.hdhr_.scan_channels(session_ptr, keep_going_);
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
    UpdateProgramGuide(DVR & dvr, bool slow);

    // virtual:
    void execute(const yae::Worker & worker);

    DVR & dvr_;
    bool slow_;
  };

  //----------------------------------------------------------------
  // UpdateProgramGuide::UpdateProgramGuide
  //
  UpdateProgramGuide::UpdateProgramGuide(DVR & dvr, bool slow):
    dvr_(dvr),
    slow_(slow)
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
    dvr_.hdhr_.get_channels(channels);

    std::list<std::string> frequencies;
    for (std::map<uint32_t, std::string>::const_iterator
           i = channels.begin(); i != channels.end(); ++i)
    {
      const std::string & frequency = i->second;
      if (frequencies.empty() || frequencies.back() != frequency)
      {
        frequencies.push_back(frequency);
      }
    }

    for (std::list<std::string>::const_iterator
           i = frequencies.begin(); i != frequencies.end(); ++i)
    {
      // shortuct:
      const std::string & frequency = *i;

      DVR::TPacketHandlerPtr pkt_handler_ptr = dvr_.packet_handler_[frequency];
      if (!pkt_handler_ptr)
      {
        continue;
      }

      const DVR::PacketHandler & packet_handler = *pkt_handler_ptr;
      const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
      const yae::mpeg_ts::Bucket & bucket = ctx.get_current_bucket();

      if (dvr_.epg_refresh_period_ <= bucket.elapsed_time_since_mgt())
      {
        DVR::TStreamPtr stream_ptr =
          dvr_.capture_stream(frequency, sample_dur);
        if (stream_ptr)
        {
          // wait until EPG is ready:
          DVR::Stream & stream = *stream_ptr;
          boost::system_time giveup_at(boost::get_system_time());
          giveup_at += boost::posix_time::seconds(sample_dur.get(1));

          boost::unique_lock<boost::mutex> lock(mutex_);
          while (true)
          {
            if (yae::Worker::Task::cancelled_)
            {
              return;
            }

            if (slow_)
            {
              boost::this_thread::sleep_for(boost::chrono::seconds(1));
            }
            else if (stream.epg_ready_.timed_wait(lock, giveup_at))
            {
              break;
            }

            boost::system_time now(boost::get_system_time());
            if (giveup_at <= now)
            {
              break;
            }
          }
        }
        else
        {
          yae_wlog("failed to start EPG update for %s", frequency.c_str());
        }
      }
      else
      {
        yae_dlog("skipping EPG update for %s", frequency.c_str());
      }

      // ctx.dump();
      dvr_.save_epg(frequency, ctx);
      dvr_.save_frequencies();

#if 0 // ndef NDEBUG
      {
        TTime now = TTime::now();
        std::string ts =
          unix_epoch_time_to_localtime_str(now.get(1), "", "-", "");

        yae::mpeg_ts::EPG epg;
        ctx.get_epg(epg);

        for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
               i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
        {
          const yae::mpeg_ts::EPG::Channel & channel = i->second;
          std::string fn = strfmt("epg-%02i.%02i-%s.json",
                                  channel.major_,
                                  channel.minor_,
                                  ts.c_str());

          Json::Value json;
          yae::mpeg_ts::save(json, channel);
          yae::TOpenFile((dvr_.yaepg_ / fn).string(), "wb").save(json);
        }
      }
#endif
    }
  }


  //----------------------------------------------------------------
  // DVR::update_epg
  //
  void
  DVR::update_epg(bool slow)
  {
    yae::shared_ptr<UpdateProgramGuide, yae::Worker::Task> task;
    task.reset(new UpdateProgramGuide(*this, slow));
    worker_.add(task);
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

    if (!stream_ptr)
    {
      // start a new session:
      HDHomeRun::TSessionPtr session_ptr = hdhr_.open_session();
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
    session.extend(TTime::now() + duration);

    if (!stream.is_open())
    {
      stream.open(stream_ptr);
    }

    return stream_ptr;
  }

  //----------------------------------------------------------------
  // DVR::get_stream_worker
  //
  DVR::TWorkerPtr
  DVR::get_stream_worker(const std::string & frequency)
  {
    TWorkerPtr & worker_ptr = stream_worker_[frequency];
    if (!worker_ptr)
    {
      worker_ptr.reset(new yae::Worker());
    }
    return worker_ptr;
  }

  //----------------------------------------------------------------
  // get_packet_handlers
  //
  static void
  get_packet_handlers(const DVR & dvr,
                      std::map<std::string, DVR::TPacketHandlerPtr> & ph)
  {
    boost::unique_lock<boost::mutex> lock(dvr.mutex_);
    ph = dvr.packet_handler_;
  }

  //----------------------------------------------------------------
  // DVR::get_epg
  //
  void
  DVR::get_epg(yae::mpeg_ts::EPG & epg, const std::string & lang) const
  {
    std::map<std::string, TPacketHandlerPtr> packet_handlers;
    get_packet_handlers(*this, packet_handlers);

    for (std::map<std::string, TPacketHandlerPtr>::const_iterator
           i = packet_handlers.begin(); i != packet_handlers.end(); ++i)
    {
      const PacketHandler & packet_handler = *(i->second.get());
      packet_handler.ctx_.get_epg(epg, lang);
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
    std::string epg_path = (yaepg_ / ("epg-" + frequency + ".json")).string();
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
    get_packet_handlers(*this, packet_handlers);

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
    std::map<std::string, yae::mpeg_ts::TChannels> frequencies;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      for (std::map<std::string, TPacketHandlerPtr>::const_iterator
             i = packet_handler_.begin(); i != packet_handler_.end(); ++i)
      {
        const std::string & frequency = i->first;
        const PacketHandler & packet_handler = *(i->second);
        const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;

        yae::mpeg_ts::TChannels & channels = frequencies[frequency];
        ctx.get_channels(channels);
      }
    }

    Json::Value json;
    yae::save(json, frequencies);

    boost::unique_lock<boost::mutex> lock(mutex_);
    std::string freq_path = (yaepg_ / "frequencies.json").string();
    yae::TOpenFile freq_file;
    if (freq_file.open(freq_path, "wb"))
    {
      freq_file.save(json);
    }
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
    std::string path = (yaepg_ / "wishlist.json").string();
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
      std::string path = (yaepg_ / "wishlist.json").string();
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
    std::string path = (yaepg_ / "schedule.json").string();
    yae::TOpenFile file;
    if (!(file.open(path, "wb") && file.save(json)))
    {
      yae_elog("write failed: %s", path.c_str());
    }
  }

  //----------------------------------------------------------------
  // CollectRecordings
  //
  struct CollectRecordings
  {
    CollectRecordings(std::map<std::string, std::string> & dst):
      pattern_("^.+\\.ts$", boost::regex::icase),
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
  // remove_recording
  //
  static uint64_t
  remove_recording(const std::string & ts)
  {
    yae_ilog("removing %s", ts.c_str());

    uint64_t size_ts = yae::stat_filesize(ts.c_str());
    if (!yae::remove_utf8(ts))
    {
      yae_wlog("failed to remove %s", ts.c_str());
      return 0;
    }

    std::string json = ts.substr(0, ts.size() - 3) + ".json";
    if (!yae::remove_utf8(json))
    {
      yae_wlog("failed to remove %s", json.c_str());
    }

    return size_ts;
  }

  //----------------------------------------------------------------
  // DVR::remove_excess_recordings
  //
  void
  DVR::remove_excess_recordings(const Recording & rec)
  {
    if (!rec.max_recordings_)
    {
      // unlimited:
      return;
    }

    std::map<std::string, std::string> recordings;
    {
      std::string title_path = rec.get_title_path(basedir_).string();
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(title_path, collect_recordings);
    }

    std::size_t removed_recordings = 0;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      if (recordings.size() - removed_recordings <= rec.max_recordings_)
      {
        break;
      }

      const std::string & ts = i->second;
      if (remove_recording(ts))
      {
        removed_recordings++;
      }
    }
  }

  //----------------------------------------------------------------
  // DVR::make_room_for
  //
  bool
  DVR::make_room_for(const Recording & rec, uint64_t num_sec)
  {
    // first, remove any existing old recordings beyond max recordings limit:
    remove_excess_recordings(rec);

    // must accommodate max 19.39Mbps ATSC transport stream:
    std::string title_path = rec.get_title_path(basedir_).string();
    uint64_t title_bytes = (num_sec * 20000000) >> 3;

    uint64_t filesystem_bytes = 0;
    uint64_t filesystem_bytes_free = 0;
    uint64_t available_bytes = 0;

    if (!yae::stat_diskspace(title_path.c_str(),
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
      std::string title_path = rec.get_title_path(basedir_).string();
      CollectRecordings collect_recordings(recordings);
      for_each_file_at(title_path, collect_recordings);
    }

    std::size_t removed_bytes = 0;
    for (std::map<std::string, std::string>::iterator
           i = recordings.begin(); i != recordings.end(); ++i)
    {
      if (title_bytes < available_bytes + removed_bytes)
      {
        break;
      }

      const std::string & ts = i->second;
      removed_bytes += remove_recording(ts);
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
  // DVR::evaluate
  //
  void
  DVR::evaluate(const yae::mpeg_ts::EPG & epg)
  {
    uint32_t margin_sec = margin_.get(1);
    schedule_.update(epg, wishlist_);

    std::map<uint32_t, std::string> frequencies;
    hdhr_.get_channels(frequencies);

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;
      uint32_t gps_time = channel.gps_time();

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
          if (!make_room_for(rec, num_sec))
          {
            continue;
          }
        }

        std::string frequency = yae::at(frequencies, ch_num);
        TStreamPtr stream = capture_stream(frequency, TTime(num_sec, 1));
        if (!is_recording)
        {
          yae_ilog("starting stream: %s", rec.get_basename().c_str());

          // FIXME: there is probably a better place for this:
          save_schedule();
        }
        else
        {
          yae_ilog("already recording: %s", rec.get_basename().c_str());
        }

        rec.stream_ = stream;
      }
    }
  }

}
