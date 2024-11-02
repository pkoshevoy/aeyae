// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 22 17:27:40 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_RECORDING_H_
#define YAE_RECORDING_H_

// standard:
#include <string>

// boost:
#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/video/yae_istream.h"
#include "yae/video/yae_mpeg_ts.h"

// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // Recording
  //
  struct YAE_API Recording
  {
    typedef boost::shared_lock<boost::shared_mutex> TReadLock;
    typedef boost::unique_lock<boost::shared_mutex> TWriteLock;

    //----------------------------------------------------------------
    // MadeBy
    //
    enum MadeBy
    {
      kUnspecified = 0,
      kWishlistItem = 1,
      kExplicitlyScheduled = 2,
      kLiveChannel = 3
    };

    //----------------------------------------------------------------
    // Rec
    //
    struct YAE_API Rec
    {
      Rec();
      Rec(const Rec & rec);
      Rec(const yae::mpeg_ts::EPG::Channel & channel,
          const yae::mpeg_ts::EPG::Program & program,
          Recording::MadeBy rec_cause = Recording::kUnspecified,
          uint16_t max_recordings = 0);

      void update(const yae::mpeg_ts::EPG::Channel & channel,
                  const yae::mpeg_ts::EPG::Program & program,
                  Recording::MadeBy rec_cause = Recording::kUnspecified,
                  uint16_t max_recordings = 0);

      Rec & operator = (const Rec & r);

      bool operator == (const Rec & r) const;

      inline bool operator != (const Rec & r) const
      { return !(this->operator == (r)); }

      inline uint32_t ch_num() const
      { return yae::mpeg_ts::channel_number(channel_major_, channel_minor_); }

      inline uint32_t gps_t0() const
      { return gps_t0_; }

      inline uint32_t gps_t1() const
      { return gps_t1_; }

      inline uint32_t gps_midpoint() const
      { return gps_t0_ + this->get_duration() / 2; }

      inline uint32_t get_duration() const
      { return gps_t1_ - gps_t0_; }

      inline bool is_recordable() const
      { return TTime::gps_now().get(1) < gps_t1_; }

      // truncate (season finale), (season premiere), (series premiere), etc...
      std::string get_short_title() const;

      fs::path get_title_path(const fs::path & basedir) const;

      // for backwards compatibility with old recordings
      // that were recorded at the full_title_ path:
      fs::path get_title_path(const fs::path & basedir,
                              const std::string & title) const;

      inline std::string get_basename() const
      { return this->get_basename(this->get_short_title()); }

      std::string get_basename(const std::string & title) const;

      std::string get_basepath(const fs::path & basedir) const;
      std::string get_filename(const fs::path & basedir,
                               const char * ext) const;

      std::string get_filepath(const fs::path & basedir,
                               const char * suffix = ".mpg") const;

      std::string get_title_filepath(const fs::path & title_path,
                                     const char * suffix) const;

      std::string get_title_filepath(const fs::path & title_path,
                                     const std::string & title,
                                     const char * suffix) const;

      // helpers for interop with code that expect EPG::Channel, etc...
      yae::mpeg_ts::EPG::Channel to_epg_channel() const;
      yae::mpeg_ts::EPG::Program to_epg_program() const;

      // for updating recording.json during the recording:
      bool save(const fs::path & basedir) const;
      bool load(const fs::path & basedir);

      Recording::MadeBy made_by_;
      bool cancelled_;
      uint64_t utc_t0_;
      uint32_t gps_t0_;
      uint32_t gps_t1_;
      uint16_t channel_major_;
      uint16_t channel_minor_;
      std::string channel_name_;
      std::string full_title_;
      std::string rating_;
      std::string description_;
      std::string device_info_;
      uint16_t max_recordings_;
    };

    Recording();
    Recording(const yae::shared_ptr<Recording::Rec> & rec);
    Recording(const yae::mpeg_ts::EPG::Channel & channel,
              const yae::mpeg_ts::EPG::Program & program,
              Recording::MadeBy rec_cause = Recording::kUnspecified,
              uint16_t max_recordings = 0);
    ~Recording();

    // NOTE: this will close any open .mpg .dat files
    // if the recording has been cancelled
    void set_rec(const yae::shared_ptr<Recording::Rec> & rec);

    inline yae::shared_ptr<Recording::Rec> get_rec() const
    {
      TReadLock lock(mutex_);
      return rec_;
    }

    inline uint32_t ch_num() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->ch_num();
    }

    inline uint32_t gps_t0() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->gps_t0_;
    }

    inline uint32_t gps_t1() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->gps_t1_;
    }

    inline uint32_t is_cancelled() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->cancelled_;
    }

    inline uint32_t made_by_wishlist() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->made_by_ == Recording::kWishlistItem;
    }

    inline uint16_t max_recordings() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->max_recordings_;
    }

    inline std::string get_short_title() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->get_short_title();
    }

    inline fs::path get_title_path(const fs::path & basedir) const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->get_title_path(basedir);
    }

    inline std::string get_basename() const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->get_basename();
    }

    inline std::string get_filepath(const fs::path & basedir,
                                    const char * suffix = ".mpg") const
    {
      yae::shared_ptr<Recording::Rec> rec = get_rec();
      return rec->get_filepath(basedir, suffix);
    }

    //----------------------------------------------------------------
    // Writer
    //
    struct YAE_API Writer
    {
      Writer();

      void write(const yae::Data & data);

      enum { kTimebase = 1000 };

      yae::TOpenFile mpg_; // transport stream (188 byte packets)
      yae::TOpenFile dat_; // time:filesize 8 byte pairs
      uint64_t dat_time_;
      uint64_t mpg_size_;

    private:
      // intentionally disabled:
      Writer(const Writer &);
      Writer & operator = (const Writer &);
    };

    bool has_writer() const;

  protected:
    // NOTE: this will return NULL writer if the recording is cancelled:
    yae::shared_ptr<Writer> get_writer(const fs::path & basedir);

  public:
    void write(const fs::path & basedir, const yae::Data & data);

    bool is_recording() const;
    void set_stream(const yae::shared_ptr<IStream> & s);

  private:
    // intentionally disabled:
    Recording(const Recording &);
    Recording & operator = (const Recording &);

  protected:
    // avoid data races:
    mutable boost::shared_mutex mutex_;

    // recording attributes:
    yae::shared_ptr<Recording::Rec> rec_;

    // keep-alive the stream as long as the Recording exists:
    yae::shared_ptr<IStream> stream_;

    // create writer on-demand, destroy when cancelled:
    yae::shared_ptr<Writer> writer_;
  };

  YAE_API void save(Json::Value & json, const Recording::Rec & rec);
  YAE_API void load(const Json::Value & json, Recording::Rec & rec);

  YAE_API void save(Json::Value & json, const Recording & recording);
  YAE_API void load(const Json::Value & json, Recording & recording);

  //----------------------------------------------------------------
  // TRecordingPtr
  //
  typedef yae::shared_ptr<Recording> TRecordingPtr;

  //----------------------------------------------------------------
  // maybe_yaetv_recording
  //
  YAE_API TRecordingPtr
  maybe_yaetv_recording(const std::string & resource_path_utf8);

}


#endif // YAE_RECORDING_H_
