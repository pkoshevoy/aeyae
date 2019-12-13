// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Dec  1 12:38:37 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <iomanip>
#include <iostream>
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
  // Wishlist::Item::matches
  //
  bool
  Wishlist::Item::matches(const yae::mpeg_ts::EPG::Channel & channel,
                          const yae::mpeg_ts::EPG::Program & program) const
  {
    if (ch_num_)
    {
      uint32_t require_ch_num = *ch_num_;
      uint32_t ch_num = yae::mpeg_ts::channel_number(channel.major_,
                                                     channel.minor_);
      if (ch_num != *ch_num_)
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
      TTime t1 = t0 + TTime(program.duration_, 1);
      if (!timespan.contains(Timespan(t0, t1)))
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

      if (boost::regex_match(program.title_, *rx_title_))
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

      if (boost::regex_match(program.description_, *rx_description_))
      {
        ok = true;
      }
    }

    return ok || (ch_num_ && (date_ || when_));
  }

  //----------------------------------------------------------------
  // Wishlist::matches
  //
  bool
  Wishlist::matches(const yae::mpeg_ts::EPG::Channel & channel,
                    const yae::mpeg_ts::EPG::Program & program) const
  {
    for (std::list<Item>::const_iterator
           i = items_.begin(); i != items_.end(); ++i)
    {
      const Item & item = *i;
      if (item.matches(channel, program))
      {
        return true;
      }
    }

    return false;
  }


  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording():
    gps_start_(0),
    cancelled_(false)
  {}


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
        if (!wishlist.matches(channel, program))
        {
          continue;
        }

        uint32_t gps_end = program.gps_time_ + program.duration_;
        if (gps_end <= channel.gps_time_)
        {
          // it's in the past:
          continue;
        }

        boost::unique_lock<boost::mutex> lock(mutex_);
        TRecordingPtr & rec_ptr = recordings_[ch_num][gps_end];
        if (rec_ptr)
        {
          continue;
        }

        rec_ptr.reset(new Recording());
        Recording & rec = *rec_ptr;
        rec.gps_start_ = program.gps_time_;

        std::ostringstream oss;
        oss << to_yyyymmdd_hhmmss(program.tm_, "", "-", "")
            << " "
            << std::setfill('0') << std::setw(2) << channel.major_
            << "."
            << std::setfill('0') << std::setw(2) << channel.minor_
            << " "
            << program.title_
            << ".ts";
        rec.filename_ = oss.str().c_str();
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
        const uint32_t gps_end = j->first;
        if (gps_end < channel.gps_time_)
        {
          // it's in the past:
          continue;
        }

        const TRecordingPtr & rec_ptr = j->second;
        updated_schedule[ch_num][gps_end] = rec_ptr;
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

    // find the earliest recording with end time greater or equal to gps_time:
    TScheduledRecordings::const_iterator
      found = schedule.lower_bound(gps_time);
    if (found == schedule.end())
    {
      // nothing scheduled at or after given GPS time:
      return TRecordingPtr();
    }

    const Recording & rec = *(found->second);
    if (gps_time < rec.gps_start_)
    {
      // scheduled recording is later than given GPS time:
      return TRecordingPtr();
    }

    return found->second;
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

    // FIXME: check that VCT is complete:
    if (bucket.guide_.empty())
    {
      return;
    }

    // consume the backlog:
    handle_backlog(bucket, gps_time);
  }

  //----------------------------------------------------------------
  // DVR::PacketHandler::handle_backlog
  //
  void
  DVR::PacketHandler::handle_backlog(const yae::mpeg_ts::Bucket & bucket,
                                     uint32_t gps_time)
  {
    yae::mpeg_ts::IPacketHandler::Packet pkt;
    while (packets_.pop(pkt))
    {
      const yae::IBuffer & data = *(pkt.data_);

      std::map<uint16_t, uint32_t>::const_iterator found =
        bucket.pid_to_ch_num_.find(pkt.pid_);

      // FIXME: check whether current pstream should be stord to disk
      // using the appropriate filename for the recorded EPG Program

      if (found == bucket.pid_to_ch_num_.end())
      {
        for (std::map<uint32_t, yae::TOpenFilePtr>::iterator
               i = channels_.begin(); i != channels_.end(); ++i)
        {
          yae::TOpenFile & file = *(i->second);
          YAE_ASSERT(file.write(data.get(), data.size()));
        }
      }
      else
      {
        const uint32_t ch_num = found->second;
        yae::TOpenFilePtr & file_ptr = channels_[ch_num];
        if (!file_ptr)
        {
          uint16_t major = yae::mpeg_ts::channel_major(ch_num);
          uint16_t minor = yae::mpeg_ts::channel_minor(ch_num);

          const yae::mpeg_ts::ChannelGuide & chan =
            yae::at(bucket.guide_, ch_num);

          std::string fn = yae::strfmt("%02u.%02u-%s.ts",
                                       major,
                                       minor,
                                       chan.name_.c_str());
          std::string filepath = (dvr_.basedir_ / fn).string();
          file_ptr.reset(new TOpenFile(filepath, "wb"));
        }

        yae::TOpenFile & file = *file_ptr;
        YAE_ASSERT(file.write(data.get(), data.size()));
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
    // boost::unique_lock<boost::mutex> lock(dvr_.mutex_);
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
  DVR::DVR():
    yaepg_(yae::get_user_folder_path(".yaepg")),
    basedir_(yae::get_temp_dir_utf8())
  {
    YAE_ASSERT(yae::mkdir_p(yaepg_.string()));

    std::string freq_path = (yaepg_ / "frequencies.json").string();
    Json::Value json;
    yae::TOpenFile(freq_path, "rb").load(json);

    std::list<std::string> frequencies;
    yae::load(json, frequencies);

    for (std::list<std::string>::const_iterator
           i = frequencies.begin(); i != frequencies.end(); ++i)
    {
      const std::string & frequency = *i;
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
    UpdateProgramGuide(DVR & dvr);

    // virtual:
    void execute(const yae::Worker & worker);

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

      DVR::TStreamPtr stream_ptr = dvr_.capture_stream(frequency, sample_dur);
      if (!stream_ptr)
      {
        // no tuners available:
        continue;
      }

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

        if (stream.epg_ready_.timed_wait(lock, giveup_at))
        {
          break;
        }

        boost::system_time now(boost::get_system_time());
        if (giveup_at <= now)
        {
          break;
        }
      }

      const DVR::PacketHandler & packet_handler = *stream.packet_handler_;
      const yae::mpeg_ts::Context & ctx = packet_handler.ctx_;
      ctx.dump();
      dvr_.save_epg(frequency, ctx);
      dvr_.save_frequencies();
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

    std::list<std::string> frequencies;
    for (std::map<std::string, TPacketHandlerPtr>::const_iterator
           i = packet_handlers.begin(); i != packet_handlers.end(); ++i)
    {
      const std::string & frequency = i->first;
      frequencies.push_back(frequency);

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
    std::list<std::string> frequencies;
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      for (std::map<std::string, TPacketHandlerPtr>::const_iterator
             i = packet_handler_.begin(); i != packet_handler_.end(); ++i)
      {
        frequencies.push_back(i->first);
      }
    }

    Json::Value json;
    yae::save(json, frequencies);

    std::string freq_path = (yaepg_ / "frequencies.json").string();
    yae::TOpenFile freq_file;
    if (freq_file.open(freq_path, "wb"))
    {
      freq_file.save(json);
    }
  }

}
