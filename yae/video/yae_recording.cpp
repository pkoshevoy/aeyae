// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 22 17:34:01 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// aeyae:
#include "yae/video/yae_recording.h"


namespace yae
{
  //----------------------------------------------------------------
  // Recording::Rec::Rec
  //
  Recording::Rec::Rec():
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
  // Recording::Rec::Rec
  //
  Recording::Rec::Rec(const Recording::Rec & rec)
  {
    *this = rec;
  }

  //----------------------------------------------------------------
  // Recording::Rec::Rec
  //
  Recording::Rec::Rec(const yae::mpeg_ts::EPG::Channel & channel,
                      const yae::mpeg_ts::EPG::Program & program,
                      Recording::MadeBy rec_cause,
                      uint16_t max_recordings):
    cancelled_(false)
  {
    update(channel, program, rec_cause, max_recordings);
  }

  //----------------------------------------------------------------
  // Recording::Rec::update
  //
  void
  Recording::Rec::update(const yae::mpeg_ts::EPG::Channel & channel,
                         const yae::mpeg_ts::EPG::Program & program,
                         Recording::MadeBy rec_cause,
                         uint16_t max_recordings)
  {
    made_by_ = rec_cause;
    utc_t0_ = localtime_to_unix_epoch_time(program.tm_);
    gps_t0_ = program.gps_time_;
    gps_t1_ = program.gps_time_ + program.duration_;
    channel_major_ = channel.major_;
    channel_minor_ = channel.minor_;
    channel_name_ = channel.name_;
    full_title_ = program.title_;
    rating_ = program.rating_;
    description_ = program.description_;
    max_recordings_ = max_recordings;
  }

  //----------------------------------------------------------------
  // Recording::Rec::operator =
  //
  Recording::Rec &
  Recording::Rec::operator = (const Recording::Rec & r)
  {
    made_by_ = r.made_by_;
    cancelled_ = r.cancelled_;
    utc_t0_ = r.utc_t0_;
    gps_t0_ = r.gps_t0_;
    gps_t1_ = r.gps_t1_;
    channel_major_ = r.channel_major_;
    channel_minor_ = r.channel_minor_;
    channel_name_ = r.channel_name_;
    full_title_ = r.full_title_;
    rating_ = r.rating_;
    description_ = r.description_;
    device_info_ = r.device_info_;
    max_recordings_ = r.max_recordings_;
    title_path_ = r.title_path_;
    title_filepath_no_ext_ = r.title_filepath_no_ext_;
    return *this;
  }

  //----------------------------------------------------------------
  // Recording::Rec::operator ==
  //
  bool
  Recording::Rec::operator == (const Recording::Rec & r) const
  {
    bool same = (made_by_ == r.made_by_ &&
                 cancelled_ == r.cancelled_ &&
                 utc_t0_ == r.utc_t0_ &&
                 gps_t0_ == r.gps_t0_ &&
                 gps_t1_ == r.gps_t1_ &&
                 channel_major_ == r.channel_major_ &&
                 channel_minor_ == r.channel_minor_ &&
                 channel_name_ == r.channel_name_ &&
                 full_title_ == r.full_title_ &&
                 rating_ == r.rating_ &&
                 description_ == r.description_ &&
                 max_recordings_ == r.max_recordings_);
    return same;
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_short_title
  //
  std::string
  Recording::Rec::get_short_title() const
  {
    static const char * tokens[] = {
      " (season premiere)",
      " (season finale)",
      " (series premiere)",
      " (series finale)",
      " (summer premiere)",
      " (time period ftc)",
      " (time period ltc)",
      " christmas special",
      " kickoff episode",
      " sneak peek",
    };

    std::string short_title = full_title_;
    std::string title_lower_case = yae::to_lower(full_title_);
    for (std::size_t i = 0, n = sizeof(tokens) / sizeof(tokens[0]); i < n; i++)
    {
      const char * token = tokens[i];
      std::string::size_type found = title_lower_case.find(token);
      if (found != std::string::npos)
      {
        short_title = full_title_.substr(0, found);
        break;
      }
    }

    short_title = yae::trim_ws(short_title);
    return short_title;
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_title_path
  //
  fs::path
  Recording::Rec::get_title_path(const fs::path & basedir) const
  {
    if (title_path_.empty())
    {
      title_path_ = this->find_title_path(basedir);
    }

    return title_path_;
  }

  //----------------------------------------------------------------
  // Recording::Rec::find_title_path
  //
  fs::path
  Recording::Rec::find_title_path(const fs::path & basedir) const
  {
    // check if there is a recording at the full_title_ path
    // for backwards compatibility with old recordings:
    fs::path title_path = this->get_title_path(basedir, full_title_);
    std::string fn_rec = this->find_title_filepath(title_path, ".json");
    if (fs::exists(fn_rec))
    {
      // for backwards compatibility with old recordings
      // that were recorded at the full_title_ path:
      return title_path;
    }

    std::string short_title = this->get_short_title();
    return this->get_title_path(basedir, short_title);
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_title_path
  //
  fs::path
  Recording::Rec::get_title_path(const fs::path & basedir,
                                 const std::string & title) const
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

    std::string safe_title = sanitize_filename_utf8(title);
    fs::path title_path = basedir / channel / safe_title;
    return title_path;
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_basename
  //
  std::string
  Recording::Rec::get_basename(const std::string & title) const
  {
    std::string safe_title = sanitize_filename_utf8(title);

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
  // Recording::Rec::get_basepath
  //
  std::string
  Recording::Rec::get_basepath(const fs::path & basedir) const
  {
    fs::path title_path = this->get_title_path(basedir);
    std::string json_path = this->get_title_filepath(title_path, ".json");
    std::string basepath = json_path.substr(0, json_path.size() - 5);
    return basepath;
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_filename
  //
  std::string
  Recording::Rec::get_filename(const fs::path & basedir,
                               const char * ext) const
  {
    fs::path title_path = this->get_title_path(basedir);
    std::string filepath = this->get_title_filepath(title_path, ext);
    std::string filename = filepath.substr(title_path.string().size() + 1);
    return filename;
  }

  //----------------------------------------------------------------
  // Recording::Rec::get_filepath
  //
  std::string
  Recording::Rec::get_filepath(const fs::path & basedir,
                               const char * ext) const
  {
    fs::path title_path = this->get_title_path(basedir);
    return this->get_title_filepath(title_path, ext);
  }

  //---------------------------------------------------------------
  // Recording::Rec::get_title_filepath
  //
  std::string
  Recording::Rec::get_title_filepath(const fs::path & title_path,
                                     const char * ext) const
  {
    if (title_filepath_no_ext_.empty())
    {
      std::string filepath = this->find_title_filepath(title_path, ".json");
      title_filepath_no_ext_ = filepath.substr(0, filepath.size() - 5);
    }

    return ((ext && *ext) ?
            (title_filepath_no_ext_ + ext) :
            title_filepath_no_ext_);
  }

  //---------------------------------------------------------------
  // Recording::Rec::get_title_filepath
  //
  std::string
  Recording::Rec::find_title_filepath(const fs::path & title_path,
                                      const char * ext) const
  {
    // for backwards compatibility with old recordings
    // which used short_title for the basename:
    std::string title = this->get_short_title();
    std::string filepath = this->get_title_filepath(title_path, title, ext);
    if (fs::exists(filepath))
    {
      return filepath;
    }

    return this->get_title_filepath(title_path, full_title_, ext);
  }

  //---------------------------------------------------------------
  // Recording::Rec::get_title_filepath
  //
  std::string
  Recording::Rec::get_title_filepath(const fs::path & title_path,
                                     const std::string & title,
                                     const char * ext) const
  {
    std::string basename = this->get_basename(title);
    std::string basepath = (title_path / basename).string();
    std::string filepath = basepath + ext;
    return filepath;
  }

  //----------------------------------------------------------------
  // Recording::Rec::to_epg_channel
  //
  yae::mpeg_ts::EPG::Channel
  Recording::Rec::to_epg_channel() const
  {
    yae::mpeg_ts::EPG::Channel channel;
    channel.major_ = channel_major_;
    channel.minor_ = channel_minor_;
    channel.name_ = channel_name_;
    return channel;
  }

  //----------------------------------------------------------------
  // Recording::Rec::to_epg_program
  //
  yae::mpeg_ts::EPG::Program
  Recording::Rec::to_epg_program() const
  {
    yae::mpeg_ts::EPG::Program program;
    program.title_ = full_title_;
    program.rating_ = rating_;
    program.description_ = description_;
    program.gps_time_ = gps_t0_;
    program.duration_ = gps_t1_ - gps_t0_;
    unix_epoch_time_to_localtime(utc_t0_, program.tm_);
    return program;
  }

  //----------------------------------------------------------------
  // Recording::Rec::save
  //
  bool
  Recording::Rec::save(const fs::path & basedir) const
  {
    fs::path title_path = this->get_title_path(basedir);
    std::string title_path_str = title_path.string();
    if (!yae::mkdir_p(title_path_str))
    {
      yae_elog("mkdir_p failed for: %s", title_path_str.c_str());
      return false;
    }

    Json::Value json;
    yae::save(json, *this);

    std::string path_json = this->get_title_filepath(title_path, ".json");
    if (!yae::TOpenFile(path_json, "wb").save(json))
    {
      yae_wlog("failed to save: %s", path_json.c_str());
      return false;
    }

    yae_dlog("saved: %s", path_json.c_str());
    return true;
  }

  //----------------------------------------------------------------
  // Recording::Rec::load
  //
  bool
  Recording::Rec::load(const fs::path & basedir)
  {
    fs::path title_path = this->get_title_path(basedir);
    std::string path_json = this->get_title_filepath(title_path, ".json");

    Json::Value json;
    if (!yae::TOpenFile(path_json, "rb").load(json))
    {
      yae_wlog("failed to save: %s", path_json.c_str());
      return false;
    }

    yae::load(json, *this);
    return true;
  }

  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording():
    rec_(new Recording::Rec())
  {}

  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording(const yae::shared_ptr<Recording::Rec> & rec):
    rec_(rec)
  {
    YAE_ASSERT(rec);
    YAE_THROW_IF(!rec);
  }

  //----------------------------------------------------------------
  // Recording::Recording
  //
  Recording::Recording(const yae::mpeg_ts::EPG::Channel & channel,
                       const yae::mpeg_ts::EPG::Program & program,
                       Recording::MadeBy rec_cause,
                       uint16_t max_recordings):
    rec_(new Recording::Rec(channel, program, rec_cause, max_recordings))
  {}

  //----------------------------------------------------------------
  // Recording::~Recording
  //
  Recording::~Recording()
  {
    if (writer_ && writer_->mpg_.is_open())
    {
      std::string fn = rec_->get_basename();
      yae_ilog("%p stopped recording: %s", this, fn.c_str());
    }
  }

  //----------------------------------------------------------------
  // Recording::set
  //
  void
  Recording::set_rec(const yae::shared_ptr<Recording::Rec> & rec_ptr)
  {
    // avoid data race:
    TWriteLock lock(mutex_);
    rec_ = rec_ptr;

    if (rec_->cancelled_)
    {
      stream_.reset();
      writer_.reset();
    }
  }

  //----------------------------------------------------------------
  // Recording::Writer::Writer
  //
  Recording::Writer::Writer():
    dat_time_(0),
    mpg_size_(0)
  {}

  //----------------------------------------------------------------
  // Recording::write
  //
  void
  Recording::Writer::write(const yae::Data & data)
  {
    YAE_EXPECT(mpg_.write(data.get(), data.size()));

    int64_t time_now = yae::TTime::now().get(Writer::kTimebase);
    int64_t elapsed_time = time_now - dat_time_;

    if (elapsed_time > Writer::kTimebase)
    {
      dat_time_ = time_now;

      if (dat_.is_open())
      {
        yae::Data payload(16);
        yae::Bitstream bs(payload);
        bs.write_bits(64, dat_time_);
        bs.write_bits(64, mpg_size_);

        YAE_ASSERT(dat_.write(payload.get(), payload.size()));
        dat_.flush();
      }

      mpg_.flush();
    }

    mpg_size_ += data.size();
  }

  //----------------------------------------------------------------
  // Recording::has_writer
  //
  bool
  Recording::has_writer() const
  {
    // avoid data race:
    TReadLock lock(mutex_);
    return (writer_ && writer_->mpg_.is_open());
  }

  //----------------------------------------------------------------
  // Recording::get_writer
  //
  yae::shared_ptr<Recording::Writer>
  Recording::get_writer(const fs::path & basedir)
  {
    // avoid data race:
    {
      TReadLock lock(mutex_);
      if (writer_ && writer_->mpg_.is_open())
      {
        return writer_;
      }

      writer_.reset();
    }

    yae::shared_ptr<Recording::Rec> rec_ptr = get_rec();
    const Recording::Rec & rec = *rec_ptr;

    yae::shared_ptr<Recording::Writer> writer_ptr;
    if (rec.cancelled_)
    {
      return writer_ptr;
    }

    fs::path title_path = rec.get_title_path(basedir);
    std::string title_path_str = title_path.string();
    if (!yae::mkdir_p(title_path_str))
    {
      yae_elog("%p mkdir_p failed for: %s", this, title_path_str.c_str());
      return writer_ptr;
    }

    std::string path_mpg = rec.get_title_filepath(title_path, ".mpg");
    std::string basepath = path_mpg.substr(0, path_mpg.size() - 4);

    writer_ptr.reset(new Recording::Writer());
    Recording::Writer & writer = *writer_ptr;

    writer.mpg_.open(path_mpg, "ab");
    bool ok = writer.mpg_.is_open();

    yae_ilog("%p writing to: %s, %s",
             this,
             path_mpg.c_str(),
             ok ? "ok" : "failed");
    if (!ok)
    {
      yae_elog("%p fopen failed for: %s", this, path_mpg.c_str());
      writer_ptr.reset();
      return writer_ptr;
    }

    writer.dat_time_ = 0;
    writer.mpg_size_ = yae::stat_filesize(path_mpg.c_str());

    uint64_t misalignment = writer.mpg_size_ % 188;
    if (misalignment)
    {
      // realign to TS packet boundary:
      std::size_t padding = 188 - misalignment;
      std::vector<uint8_t> zeros(padding);
      writer.mpg_.write(zeros);
      writer.mpg_size_ += padding;
    }

    std::string path_dat = basepath + ".dat";
    if (!writer.dat_.open(path_dat, "ab"))
    {
      yae_wlog("%p fopen failed for: %s", this, path_dat.c_str());
    }
    else
    {
      yae::Data payload(16);
      yae::Bitstream bs(payload);
      bs.write_bytes("timebase", 8);
      bs.write_bits(64, Writer::kTimebase);
      YAE_ASSERT(writer.dat_.write(payload.get(), payload.size()));
    }

    Json::Value json;
    yae::save(json, rec);

    std::string path_json = basepath + ".json";
    if (!yae::TOpenFile(path_json, "wb").save(json))
    {
      yae_wlog("%p fopen failed for: %s", this, path_json.c_str());
    }

    // avoid data race:
    TWriteLock lock(mutex_);
    writer_ = writer_ptr;
    return writer_ptr;
  }

  //----------------------------------------------------------------
  // Recording::write
  //
  void
  Recording::write(const fs::path & basedir, const yae::Data & data)
  {
    yae::shared_ptr<Writer> writer_ptr = get_writer(basedir);
    if (!writer_ptr)
    {
      return;
    }

    Writer & writer = *writer_ptr;
    writer.write(data);
  }

  //----------------------------------------------------------------
  // Recording::is_recording
  //
  bool
  Recording::is_recording() const
  {
    TReadLock lock(mutex_);
    return stream_ && stream_->is_open();
  }

  //----------------------------------------------------------------
  // Recording::set_stream
  //
  void
  Recording::set_stream(const yae::shared_ptr<IStream> & s)
  {
    TWriteLock lock(mutex_);
    stream_ = s;
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const Recording::Rec & rec)
  {
    save(json["cancelled"], rec.cancelled_);
    save(json["utc_t0"], rec.utc_t0_);
    save(json["gps_t0"], rec.gps_t0_);
    save(json["gps_t1"], rec.gps_t1_);
    save(json["channel_major"], rec.channel_major_);
    save(json["channel_minor"], rec.channel_minor_);
    save(json["channel_name"], rec.channel_name_);
    save(json["title"], rec.full_title_);
    save(json["rating"], rec.rating_);
    save(json["description"], rec.description_);

    if (!rec.device_info_.empty())
    {
      save(json["device_info"], rec.device_info_);
    }

    if (rec.max_recordings_)
    {
      save(json["max_recordings"], rec.max_recordings_);
    }

#ifndef NDEBUG
    std::ostringstream oss;
    oss << unix_epoch_time_to_localtime_str(rec.utc_t0_) << " "
        << rec.channel_major_ << "."
        << rec.channel_minor_ << " "
        << rec.full_title_ << " (now "
        << unix_epoch_time_to_localtime_str(TTime::now().get(1)) << ")";
    save(json["_debug"], oss.str());
#endif
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, Recording::Rec & rec)
  {
    load(json["cancelled"], rec.cancelled_);
    load(json["utc_t0"], rec.utc_t0_);
    load(json["gps_t0"], rec.gps_t0_);
    load(json["gps_t1"], rec.gps_t1_);
    load(json["channel_major"], rec.channel_major_);
    load(json["channel_minor"], rec.channel_minor_);
    load(json["channel_name"], rec.channel_name_);
    load(json["title"], rec.full_title_);
    load(json["rating"], rec.rating_);
    load(json["description"], rec.description_);

    if (json.isMember("device_info"))
    {
      load(json["device_info"], rec.device_info_);
    }

    if (json.isMember("max_recordings"))
    {
      load(json["max_recordings"], rec.max_recordings_);
    }
  }

  //----------------------------------------------------------------
  // save
  //
  void
  save(Json::Value & json, const Recording & recording)
  {
    yae::shared_ptr<Recording::Rec> rec = recording.get_rec();
    save(json, *rec);
  }

  //----------------------------------------------------------------
  // load
  //
  void
  load(const Json::Value & json, Recording & recording)
  {
    yae::shared_ptr<Recording::Rec> rec(new Recording::Rec);
    load(json, *rec);
    recording.set_rec(rec);
  }

  //----------------------------------------------------------------
  // maybe_yaetv_recording
  //
  TRecordingPtr
  maybe_yaetv_recording(const std::string & filepath)
  {
    TRecordingPtr rec_ptr;
    if (!al::ends_with(filepath, ".mpg"))
    {
      return rec_ptr;
    }

    std::string folder;
    std::string fn_ext;
    if (!parse_file_path(filepath, folder, fn_ext))
    {
      return rec_ptr;
    }

    std::string basename;
    std::string suffix;
    if (!parse_file_name(fn_ext, basename, suffix))
    {
      return rec_ptr;
    }

    fs::path path(folder);
    std::string fn_rec = (path / (basename + ".json")).string();
    if (!fs::exists(fn_rec))
    {
      return rec_ptr;
    }

    std::string fn_dat = (path / (basename + ".dat")).string();
    if (!fs::exists(fn_dat))
    {
      return rec_ptr;
    }

    try
    {
      Json::Value json;
      yae::TOpenFile(fn_rec, "rb").load(json);
      rec_ptr.reset(new Recording());
      yae::load(json, *rec_ptr);
    }
    catch (...)
    {
      rec_ptr.reset();
    }

    return rec_ptr;
  }

}
