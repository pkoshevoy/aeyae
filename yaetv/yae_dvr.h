// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Dec  1 12:33:37 MST 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DVR_H_
#define YAE_DVR_H_

// standard:
#include <list>
#include <map>
#include <string>

// boost:
#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/thread/yae_ring_buffer.h"
#include "yae/thread/yae_worker.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_fifo.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_mpeg_ts.h"

// epg:
#include "yae_hdhomerun.h"

// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{
  // forward declarations:
  struct DVR;

  //----------------------------------------------------------------
  // kWeekdays
  //
  extern YAE_API const char * kWeekdays[7];

  //----------------------------------------------------------------
  // kMonths
  //
  extern YAE_API const char * kMonths[12];


  //----------------------------------------------------------------
  // Wishlist
  //
  struct Wishlist
  {
    //----------------------------------------------------------------
    // Item
    //
    struct Item
    {
      bool matches(const yae::mpeg_ts::EPG::Channel & channel,
                   const yae::mpeg_ts::EPG::Program & program) const;

      void save(Json::Value & json) const;
      void load(const Json::Value & json);

      inline bool max_recordings() const
      { return max_recordings_ ? *max_recordings_ : 0; }

      inline bool skip_duplicates() const
      { return skip_duplicates_ ? *skip_duplicates_ : false; }

      enum Weekday
      {
        Sun = 1 << 0,
        Mon = 1 << 1,
        Tue = 1 << 2,
        Wed = 1 << 3,
        Thu = 1 << 4,
        Fri = 1 << 5,
        Sat = 1 << 6
      };

      yae::optional<bool> skip_duplicates_;
      yae::optional<uint16_t> max_recordings_;
      yae::optional<std::pair<uint16_t, uint16_t> > channel_;
      yae::optional<struct tm> date_;
      yae::optional<Timespan> when_;
      yae::optional<uint16_t> weekday_mask_;
      std::string title_;
      std::string description_;

    protected:
      mutable yae::optional<boost::regex> rx_title_;
      mutable yae::optional<boost::regex> rx_description_;
    };

    Wishlist();

    yae::shared_ptr<Item>
    matches(const yae::mpeg_ts::EPG::Channel & channel,
            const yae::mpeg_ts::EPG::Program & program) const;

    std::list<Item> items_;
    int64_t lastmod_;
  };

  void save(Json::Value & json, const Wishlist::Item & item);
  void load(const Json::Value & json, Wishlist::Item & item);

  void save(Json::Value & json, const Wishlist & wishlist);
  void load(const Json::Value & json, Wishlist & wishlist);


  //----------------------------------------------------------------
  // Recording
  //
  struct Recording
  {
    Recording();
    Recording(const yae::mpeg_ts::EPG::Channel & channel,
              const yae::mpeg_ts::EPG::Program & program);
    ~Recording();

    fs::path get_title_path(const fs::path & basedir) const;
    std::string get_basename() const;
    std::string get_filepath(const fs::path & basedir,
                             const char * suffix = ".mpg") const;

    yae::TOpenFilePtr open_mpg(const fs::path & basedir);
    yae::TOpenFilePtr open_dat(const fs::path & basedir);

    void write(const fs::path & basedir, const yae::IBuffer & data);
    void write_dat();

    inline uint32_t ch_num() const
    { return yae::mpeg_ts::channel_number(channel_major_, channel_minor_); }

    bool cancelled_;
    uint64_t utc_t0_;
    uint32_t gps_t0_;
    uint32_t gps_t1_;
    uint16_t channel_major_;
    uint16_t channel_minor_;
    std::string channel_name_;
    std::string title_;
    std::string rating_;
    std::string description_;
    uint16_t max_recordings_;

    yae::TOpenFilePtr mpg_; // transport stream (188 byte packets)
    yae::TOpenFilePtr dat_; // time:filesize 8 byte pairs
    yae::shared_ptr<IStream> stream_;
    uint64_t dat_time_;
    uint64_t mpg_size_;
  };

  void save(Json::Value & json, const Recording & rec);
  void load(const Json::Value & json, Recording & rec);


  //----------------------------------------------------------------
  // TRecordingPtr
  //
  typedef yae::shared_ptr<Recording> TRecordingPtr;

  //----------------------------------------------------------------
  // TRecordings
  //
  typedef std::map<std::string, TRecordingPtr> TRecordings;

  //----------------------------------------------------------------
  // TScheduledRecordings
  //
  // indexed by GPS start time of the recording:
  //
  typedef std::map<uint32_t, TRecordingPtr> TScheduledRecordings;

  //----------------------------------------------------------------
  // Schedule
  //
  struct Schedule
  {
    // given a wishlist and current epg
    // create program recording schedule
    void update(DVR & dvr, const yae::mpeg_ts::EPG & epg);

    void get(std::map<uint32_t, TScheduledRecordings> & recordings) const;

    // find a scheduled recording corresponding to the
    // given channel number and gps time:
    TRecordingPtr get(uint32_t ch_num, uint32_t gps_time) const;

    // with margins, it's possible to have more than 1 recording active
    // when one recoding is near the end and another is near beginning:
    void get(std::set<TRecordingPtr> & recordings,
             uint32_t ch_num,
             uint32_t gps_time,
             uint32_t margin_sec) const;

    bool toggle(uint32_t ch_num, uint32_t gps_time);
    void remove(uint32_t ch_num, uint32_t gps_time);

    void save(Json::Value & json) const;
    void load(const Json::Value & json);
    void clear();

  protected:
    // protect against concurrent access:
    mutable boost::mutex mutex_;

    // indexed by channel number:
    std::map<uint32_t, TScheduledRecordings> recordings_;
  };


  //----------------------------------------------------------------
  // DVR
  //
  struct DVR
  {
    //----------------------------------------------------------------
    // Blacklist
    //
    struct Blacklist
    {
      Blacklist();

      void toggle(uint32_t ch_num);

      std::set<uint32_t> channels_;
      int64_t lastmod_;
    };

    //----------------------------------------------------------------
    // PacketHandler
    //
    struct PacketHandler : yae::mpeg_ts::IPacketHandler
    {
      PacketHandler(DVR & dvr);
      virtual ~PacketHandler();

      virtual void handle(const yae::mpeg_ts::IPacketHandler::Packet & packet,
                          const yae::mpeg_ts::Bucket & bucket,
                          uint32_t gps_time);

      void handle_backlog(const yae::mpeg_ts::Bucket & bucket,
                          uint32_t gps_time);

      DVR & dvr_;
      yae::Worker worker_;
      yae::mpeg_ts::Context ctx_;
      yae::RingBuffer ring_buffer_;

      // buffer packets until we have enough info (EPG)
      // to enable us to handle them properly:
      yae::fifo<Packet> packets_;

      // cache scheduled recordings to avoid lock contention:
      std::map<uint32_t, std::set<TRecordingPtr> > recordings_;
      uint32_t recordings_update_gps_time_;
    };

    //----------------------------------------------------------------
    // TPacketHandlerPtr
    //
    typedef yae::shared_ptr<PacketHandler, yae::mpeg_ts::IPacketHandler>
    TPacketHandlerPtr;


    //----------------------------------------------------------------
    // Stream
    //
    struct Stream : IStream
    {
      Stream(DVR & dvr,
             const yae::HDHomeRun::TSessionPtr & session_ptr,
             const std::string & frequency);
      ~Stream();

      void open(const yae::shared_ptr<Stream, IStream> & self_ptr);

      virtual void close();
      virtual bool is_open() const;
      virtual bool push(const void * data, std::size_t size);

      DVR & dvr_;
      yae::TWorkerPtr worker_;
      yae::HDHomeRun::TSessionPtr session_;
      std::string frequency_;
      TPacketHandlerPtr packet_handler_;

      // this will signal when channel guide is ready for this frequency:
      boost::condition_variable epg_ready_;
    };

    //----------------------------------------------------------------
    // TStreamPtr
    //
    typedef yae::shared_ptr<Stream, IStream> TStreamPtr;


    //----------------------------------------------------------------
    // ServiceLoop
    //
    struct ServiceLoop : yae::Worker::Task
    {
      ServiceLoop(DVR & dvr);

      // virtual:
      void execute(const yae::Worker & worker);
      void cancel();

      DVR & dvr_;
      DontStop keep_going_;
    };


    DVR(const std::string & yaetv_dir,
        const std::string & recordings_dir);
    ~DVR();

    void init_packet_handlers();
    void shutdown();
    void scan_channels();
    void update_epg();

    TStreamPtr capture_stream(const std::string & frequency,
                              const TTime & duration);

    TWorkerPtr get_stream_worker(const std::string & frequency);

    void get(std::map<std::string, TPacketHandlerPtr> & packet_handlers) const;
    void get(Blacklist & blacklist) const;
    void get(Wishlist & wishlist) const;

    void get_epg(yae::mpeg_ts::EPG & epg,
                 const std::string & lang = std::string("eng")) const;

    void save_epg(const std::string & frequency,
                  const yae::mpeg_ts::Context & ctx) const;

    void save_epg() const;
    void save_frequencies() const;

    void toggle_blacklist(uint32_t ch_num);
    void save_blacklist() const;
    bool load_blacklist();

    void save_wishlist() const;
    bool load_wishlist();

    void save_schedule() const;

    // NOTE: this explicitly bypasses the regular Wishlist and generates
    // a separate item one-item wishlist just for the specified program:
    void schedule_recording(const yae::mpeg_ts::EPG::Channel & channel,
                            const yae::mpeg_ts::EPG::Program & program);

    void cancel_recording(const yae::mpeg_ts::EPG::Channel & channel,
                          const yae::mpeg_ts::EPG::Program & program);

    yae::shared_ptr<Wishlist::Item>
    explicitly_scheduled(const yae::mpeg_ts::EPG::Channel & channel,
                         const yae::mpeg_ts::EPG::Program & program) const;

    void toggle_recording(uint32_t ch_num, uint32_t gps_time);
    void delete_recording(const Recording & rec);

    void remove_excess_recordings(const Recording & rec);
    bool make_room_for(const Recording & rec, uint64_t num_sec);

    // find an earlier recording with the same program description:
    TRecordingPtr
    already_recorded(const yae::mpeg_ts::EPG::Channel & channel,
                     const yae::mpeg_ts::EPG::Program & program) const;

    void
    get_recordings(TRecordings & by_filename,
                   std::map<std::string, TRecordings> & by_playlist) const;

    void evaluate(const yae::mpeg_ts::EPG & epg);

    inline TTime next_channel_scan() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return TTime(next_channel_scan_);
    }

    inline void set_next_channel_scan(const TTime & t)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      next_channel_scan_ = t;
    }

    inline TTime next_epg_refresh() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return TTime(next_epg_refresh_);
    }

    inline void set_next_epg_refresh(const TTime & t)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      next_epg_refresh_ = t;
    }

    inline TTime next_schedule_refresh() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return TTime(next_schedule_refresh_);
    }

    inline void set_next_schedule_refresh(const TTime & t)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      next_schedule_refresh_ = t;
    }

    inline void cache_epg(const yae::mpeg_ts::EPG & epg)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      epg_ = epg;
      epg_lastmod_ = TTime::now();
    }

    bool get_cached_epg(TTime & lastmod, yae::mpeg_ts::EPG & epg) const;

    // protect against concurrent access:
    mutable boost::mutex mutex_;

    yae::HDHomeRun hdhr_;
    yae::Worker worker_;
    fs::path yaetv_;
    fs::path basedir_;

    // keep track of existing streams, but don't extend their lifetime:
    std::map<std::string, TWorkerPtr> stream_worker_;
    std::map<std::string, yae::weak_ptr<Stream, IStream> > stream_;
    std::map<std::string, TPacketHandlerPtr> packet_handler_;
    TWorkerPtr service_loop_worker_;

    // recordings wishlist, schedule, etc:
    Schedule schedule_;
    Wishlist wishlist_;

    // channels we don't want to waste time on:
    Blacklist blacklist_;

    TTime channel_scan_period_;
    TTime epg_refresh_period_;
    TTime schedule_refresh_period_;
    TTime margin_;

  protected:
    TTime next_channel_scan_;
    TTime next_epg_refresh_;
    TTime next_schedule_refresh_;

    yae::mpeg_ts::EPG epg_;
    TTime epg_lastmod_;
  };

}


#endif // YAE_DVR_H_
