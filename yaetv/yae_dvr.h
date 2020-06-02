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
#include <boost/random/mersenne_twister.hpp>
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
  // TChannelNames
  //
  // channel names indexed by channel_minor
  //
  typedef std::map<uint16_t, std::string> TChannelNames;

  //----------------------------------------------------------------
  // TChannels
  //
  // indexed by channel_major
  //
  typedef std::map<uint16_t, TChannelNames> TChannels;

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
      void set_title(const std::string & title_rx);
      void set_description(const std::string & desc_rx);

      // helpers:
      std::string ch_txt() const;
      std::string to_txt() const;
      std::string to_key() const;

      bool matches(const yae::mpeg_ts::EPG::Channel & channel,
                   const yae::mpeg_ts::EPG::Program & program) const;

      void save(Json::Value & json) const;
      void load(const Json::Value & json);

      inline uint16_t max_recordings() const
      { return max_recordings_ ? *max_recordings_ : 0; }

      inline bool skip_duplicates() const
      { return skip_duplicates_ && *skip_duplicates_; }

      inline bool is_disabled() const
      { return disabled_ && *disabled_; }

      inline bool operator == (const Wishlist::Item & other) const
      {
        return (skip_duplicates_ == other.skip_duplicates_ &&
                max_recordings_ == other.max_recordings_ &&
                channel_ == other.channel_ &&
                ((!date_ && !other.date_) ||
                 (date_ && other.date_ &&
                  same_localtime(*date_, *other.date_))) &&
                when_ == other.when_ &&
                weekday_mask_ == other.weekday_mask_ &&
                title_ == other.title_ &&
                description_ == other.description_);
      }

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

      yae::optional<bool> disabled_;
      yae::optional<bool> skip_duplicates_;
      yae::optional<uint16_t> max_recordings_;
      yae::optional<std::pair<uint16_t, uint16_t> > channel_;
      yae::optional<struct tm> date_;
      yae::optional<Timespan> when_;
      yae::optional<uint16_t> weekday_mask_;
      yae::optional<uint16_t> min_minutes_;
      yae::optional<uint16_t> max_minutes_;
      std::string title_;
      std::string description_;

    protected:
      mutable yae::optional<boost::regex> rx_title_;
      mutable yae::optional<boost::regex> rx_description_;
    };

    Wishlist();

    // store wishlist in a map, indexed by Item::to_str summary:
    void get(std::map<std::string, Item> & wishlist) const;

    bool remove(const std::string & wi_key);
    void update(const std::string & wi_key, const Wishlist::Item & new_item);

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
    struct Rec
    {
      Rec();
      Rec(const yae::mpeg_ts::EPG::Channel & channel,
          const yae::mpeg_ts::EPG::Program & program,
          Recording::MadeBy rec_cause = Recording::kUnspecified,
          uint16_t max_recordings = 0);

      void update(const yae::mpeg_ts::EPG::Channel & channel,
                  const yae::mpeg_ts::EPG::Program & program,
                  Recording::MadeBy rec_cause = Recording::kUnspecified,
                  uint16_t max_recordings = 0);

      inline uint32_t ch_num() const
      { return yae::mpeg_ts::channel_number(channel_major_, channel_minor_); }

      inline uint32_t gps_t0() const
      { return gps_t0_; }

      inline uint32_t gps_t1() const
      { return gps_t1_; }

      inline uint32_t get_duration() const
      { return gps_t1_ - gps_t0_; }

      inline bool is_recordable() const
      { return TTime::gps_now().get(1) < gps_t1_; }

      fs::path get_title_path(const fs::path & basedir) const;

      std::string get_basename() const;
      std::string get_filepath(const fs::path & basedir,
                               const char * suffix = ".mpg") const;

      Recording::MadeBy made_by_;
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
    struct Writer
    {
      Writer();

      void write(const yae::IBuffer & data);

      yae::TOpenFile mpg_; // transport stream (188 byte packets)
      yae::TOpenFile dat_; // time:filesize 8 byte pairs
      uint64_t dat_time_;
      uint64_t mpg_size_;

    private:
      // intentionally disabled:
      Writer(const Writer &);
      Writer & operator = (const Writer &);
    };

  protected:
    // NOTE: this will return NULL writer if the recording is cancelled:
    yae::shared_ptr<Writer> get_writer(const fs::path & basedir);

  public:
    void write(const fs::path & basedir, const yae::IBuffer & data);

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

  void save(Json::Value & json, const Recording::Rec & rec);
  void load(const Json::Value & json, Recording::Rec & rec);

  void save(Json::Value & json, const Recording & recording);
  void load(const Json::Value & json, Recording & recording);


  //----------------------------------------------------------------
  // TRecPtr
  //
  typedef yae::shared_ptr<Recording::Rec> TRecPtr;

  //----------------------------------------------------------------
  // TRecs
  //
  typedef std::map<std::string, TRecPtr> TRecs;

  //----------------------------------------------------------------
  // TRecsByTime
  //
  typedef std::map<uint32_t, TRecPtr> TRecsByTime;


  //----------------------------------------------------------------
  // FoundRecordings
  //
  struct FoundRecordings
  {
    // indexed by filename:
    TRecs by_filename_;

    // indexed by playlist:
    std::map<std::string, TRecs> by_playlist_;

    // indexed by channel number:
    std::map<uint32_t, TRecsByTime> by_channel_;
  };

  //----------------------------------------------------------------
  // TFoundRecordingsPtr
  //
  typedef yae::shared_ptr<FoundRecordings> TFoundRecordingsPtr;


  //----------------------------------------------------------------
  // TRecordingPtr
  //
  typedef yae::shared_ptr<Recording> TRecordingPtr;

  //----------------------------------------------------------------
  // TRecordings
  //
  // indexed by filename.mpg
  //
  typedef std::map<std::string, TRecordingPtr> TRecordings;

  //----------------------------------------------------------------
  // TScheduledRecordings
  //
  // indexed by GPS start time of the recording:
  //
  typedef std::map<uint32_t, TRecordingPtr> TScheduledRecordings;


  //----------------------------------------------------------------
  // next
  //
  TRecPtr
  next(const TRecPtr & after_this,
       const std::map<uint32_t, TRecsByTime> & recordings,
       uint32_t ch_num,
       uint32_t gpt_time);

  //----------------------------------------------------------------
  // next
  //
  template <typename TRec>
  yae::shared_ptr<TRec>
  next(const std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<TRec> > > &
       recordings,
       uint32_t ch_num,
       uint32_t gps_time,
       const yae::shared_ptr<TRec> & after_this = yae::shared_ptr<TRec>())
  {
    typedef yae::shared_ptr<TRec> TRecPtr;
    typedef std::map<uint32_t, TRecPtr> TRecsByTime;
    typedef std::map<uint32_t, TRecsByTime> TRecByChannel;

    typename TRecByChannel::const_iterator ch_found = recordings.find(ch_num);
    if (ch_found == recordings.end())
    {
      // nothing scheduled for this channel:
      return TRecPtr();
    }

    // recordings are indexed by GPS start time:
    const TRecsByTime & schedule = ch_found->second;
    if (schedule.empty())
    {
      // nothing scheduled for this channel:
      return TRecPtr();
    }

    uint32_t schedule_t0 = schedule.begin()->first;
    uint32_t schedule_t1 = schedule.rbegin()->second->gps_t1();
    if (gps_time < schedule_t0 || schedule_t1 <= gps_time)
    {
      // nothing scheduled at given time:
      return TRecPtr();
    }

    // find the earliest recording with start time greater than gps_time:
    uint32_t rec_gps_t0 = 0;
    TRecPtr rec_ptr;

    typename TRecsByTime::const_iterator it = schedule.upper_bound(gps_time);
    if (it == schedule.end())
    {
      typename TRecsByTime::const_reverse_iterator rit = schedule.rbegin();
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

      // sanity check:
      YAE_ASSERT(rec_gps_t0 == rec_ptr->gps_t0());

      if (!after_this || after_this->gps_t0() < rec_gps_t0)
      {
        break;
      }

      rec_ptr.reset();
      ++it;
    }

    if (!rec_ptr)
    {
      return TRecPtr();
    }

    const TRec & rec = *rec_ptr;
    if (!after_this && (gps_time < rec_gps_t0 || rec.gps_t1() <= gps_time))
    {
      return after_this;
    }

    return rec_ptr;
  }

  //----------------------------------------------------------------
  // Schedule
  //
  struct Schedule
  {
    // check whether a given recording is in progress:
    bool is_recording_now(const TRecPtr & rec_ptr) const;

    // return previous live channel number, set new live channel number:
    uint32_t enable_live(uint32_t ch_num);
    uint32_t disable_live();

    // returns 0 is live channel is disabled:
    uint32_t get_live_channel() const;

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

    // while watching a channel live -- treat everything
    // on the channel as if scheduled to record:
    yae::optional<uint32_t> live_ch_;
  };


  //----------------------------------------------------------------
  // remove_excess_recordings
  //
  void remove_excess_recordings(const fs::path & basedir,
                                const Recording & rec);

  //----------------------------------------------------------------
  // make_room_for
  //
  bool make_room_for(const fs::path & basedir,
                     const Recording::Rec & rec,
                     uint64_t num_sec);

  //----------------------------------------------------------------
  // make_room_for
  //
  bool make_room_for(const std::string & path,
                     uint64_t required_bytes);


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

      void clear();
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

      void refresh_cached_recordings();

      DVR & dvr_;
      yae::Worker worker_;
      yae::mpeg_ts::Context ctx_;
      yae::RingBuffer ring_buffer_;

      // buffer packets until we have enough info (EPG)
      // to enable us to handle them properly:
      yae::fifo<Packet> packets_;

      // cache scheduled recordings to avoid lock contention:
      mutable boost::mutex mutex_;
      boost::random::mt11213b prng_;
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

      void open(const yae::shared_ptr<Stream, IStream> & self_ptr,
                const yae::TWorkerPtr & worker_ptr);

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

    void restart(const std::string & basedir);

    void cleanup_yaetv_dir();
    void cleanup_explicitly_scheduled_items();

    void init_packet_handlers();
    void shutdown();
    void scan_channels();
    void update_epg();
    void cleanup_storage();

    TStreamPtr capture_stream(const HDHomeRun::TSessionPtr & session_ptr,
                              const std::string & frequency,
                              const TTime & duration);

    TStreamPtr capture_stream(const std::string & frequency,
                              const TTime & duration);

    TStreamPtr get_existing_stream(const std::string & frequency);

    void no_signal(const std::string & frequency);

    void get(std::map<std::string, TPacketHandlerPtr> & packet_handlers) const;
    void get(Blacklist & blacklist) const;
    void get(std::map<std::string, Wishlist::Item> & wishlist) const;

    bool wishlist_remove(const std::string & wi_key);
    void wishlist_update(const std::string & wi_key,
                         const Wishlist::Item & new_item);

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
    void delete_recording(const Recording::Rec & rec);

    bool make_room_for(const Recording::Rec & rec, uint64_t num_sec);

    // find an earlier recording with the same program description:
    TRecPtr
    already_recorded(const yae::mpeg_ts::EPG::Channel & channel,
                     const yae::mpeg_ts::EPG::Program & program) const;

    void get_existing_recordings(FoundRecordings & found) const;

    //----------------------------------------------------------------
    // ChanTime
    //
    struct ChanTime
    {
      ChanTime(uint32_t ch_num, uint32_t gps_time):
        ch_num_(ch_num),
        gps_time_(gps_time)
      {}

      uint32_t ch_num_;
      uint32_t gps_time_;
    };

    //----------------------------------------------------------------
    // Playback
    //
    struct Playback
    {
      Playback(const std::string & playlist = std::string(),
               const std::string & filename = std::string(),
               const std::string & basepath = std::string()):
        playlist_(playlist),
        filename_(filename),
        basepath_(basepath)
      {}

      std::string playlist_;
      std::string filename_;
      std::string basepath_;
    };

    yae::shared_ptr<Playback>
    is_ready_to_play(const Recording::Rec & rec) const;

    void watch_live(uint32_t ch_num);
    void close_live();

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

    inline TTime next_storage_cleanup() const
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      return TTime(next_storage_cleanup_);
    }

    inline void set_next_storage_cleanup(const TTime & t)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      next_storage_cleanup_ = t;
    }

    inline void cache_epg(const yae::mpeg_ts::EPG & epg)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      epg_ = epg;
      epg_lastmod_ = TTime::now();
    }

    bool get_cached_epg(TTime & lastmod, yae::mpeg_ts::EPG & epg) const;

    void get_tuner_cache(const std::string & device_name,
                         Json::Value & tuner_cache) const;

    void update_tuner_cache(const std::string & device_name,
                            const Json::Value & tuner_cache);

  protected:
    void update_channel_frequency_luts();

  public:
    // fill in the major.minor -> frequency lookup table:
    void get_channels(std::map<uint32_t, std::string> & chan_freq) const;
    void get_channels(std::map<std::string, TChannels> & channels) const;
    bool get_channels(const std::string & freq, TChannels & channels) const;

    // helper:
    uint16_t get_channel_major(const std::string & frequency) const;

    bool get_channel_name(uint16_t major,
                          uint16_t minor,
                          std::string & name) const;

    bool has_preferences() const;
    void get_preferences(Json::Value & preferences) const;
    void set_preferences(const Json::Value & preferences);

    // returns false if there are no enabled tuners:
    bool discover_enabled_tuners(std::set<std::string> & tuner_names);

    // default is us-bcast, can be configured in preferences:
    std::string get_channelmap() const;

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
    TTime storage_cleanup_period_;
    TTime margin_;

  protected:
    mutable boost::mutex tuner_cache_mutex_;
    Json::Value tuner_cache_;
    std::map<uint32_t, std::string> channel_frequency_lut_;
    std::map<std::string, TChannels> frequency_channel_lut_;

    TTime next_channel_scan_;
    TTime next_epg_refresh_;
    TTime next_schedule_refresh_;
    TTime next_storage_cleanup_;

    yae::mpeg_ts::EPG epg_;
    TTime epg_lastmod_;

    mutable boost::mutex preferences_mutex_;
    Json::Value preferences_;
  };


  //----------------------------------------------------------------
  // cleanup_yaetv_logs
  //
  void cleanup_yaetv_logs(const std::string & yaetv_dir);

}


#endif // YAE_DVR_H_
