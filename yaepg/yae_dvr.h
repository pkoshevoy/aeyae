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

    const Item *
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

    fs::path get_title_path(const fs::path & basedir) const;
    std::string get_basename() const;
    yae::TOpenFilePtr open_file(const fs::path & basedir);

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

    yae::TOpenFilePtr file_;
    yae::shared_ptr<IStream> stream_;
  };

  void save(Json::Value & json, const Recording & rec);
  void load(const Json::Value & json, Recording & rec);


  //----------------------------------------------------------------
  // TRecordingPtr
  //
  typedef yae::shared_ptr<Recording> TRecordingPtr;

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
    void update(const yae::mpeg_ts::EPG & epg,
                const Wishlist & wishlist);

    // find a scheduled recording corresponding to the
    // given channel number and gps time:
    TRecordingPtr get(uint32_t ch_num, uint32_t gps_time) const;

    // with margins, it's possible to have more than 1 recording active
    // when one recoding is near the end and another is near beginning:
    void get(std::set<TRecordingPtr> & recordings,
             uint32_t ch_num,
             uint32_t gps_time,
             uint32_t margin_sec) const;

    void save(Json::Value & json) const;
    void load(const Json::Value & json);

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
      // yae::TOpenFilePtr file_;
      yae::mpeg_ts::Context ctx_;
      yae::RingBuffer ring_buffer_;
      yae::TTime start_;

      // buffer packets until we have enough info (EPG)
      // to enable us to handle them properly:
      yae::fifo<Packet> packets_;
      std::map<uint32_t, yae::TOpenFilePtr> channels_;
    };

    //----------------------------------------------------------------
    // TPacketHandlerPtr
    //
    typedef yae::shared_ptr<PacketHandler, yae::mpeg_ts::IPacketHandler>
    TPacketHandlerPtr;


    //----------------------------------------------------------------
    // TWorkerPtr
    //
    typedef yae::shared_ptr<yae::Worker> TWorkerPtr;

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
      DVR::TWorkerPtr worker_;
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


    DVR(const std::string & basedir = std::string());
    ~DVR();

    // TStreamPtr get_stream(const std::string & frequency);
    void shutdown();
    void scan_channels();
    void update_epg(bool slow = false);

    TStreamPtr capture_stream(const std::string & frequency,
                              const TTime & duration);

    TWorkerPtr get_stream_worker(const std::string & frequency);

    void get_epg(yae::mpeg_ts::EPG & epg,
                 const std::string & lang = std::string("eng")) const;

    void save_epg(const std::string & frequency,
                  const yae::mpeg_ts::Context & ctx) const;

    void save_epg() const;
    void save_frequencies() const;

    void save_wishlist() const;
    bool load_wishlist();

    void save_schedule() const;

    void remove_excess_recordings(const Recording & rec);
    bool make_room_for(const Recording & rec, uint64_t num_sec);

    void evaluate(const yae::mpeg_ts::EPG & epg);

    // protect against concurrent access:
    mutable boost::mutex mutex_;

    yae::HDHomeRun hdhr_;
    yae::Worker worker_;
    fs::path yaepg_;
    fs::path basedir_;

    // keep track of existing streams, but don't extend their lifetime:
    std::map<std::string, TWorkerPtr> stream_worker_;
    std::map<std::string, yae::weak_ptr<Stream, IStream> > stream_;
    std::map<std::string, TPacketHandlerPtr> packet_handler_;

    // recordings wishlist, schedule, etc:
    Schedule schedule_;
    Wishlist wishlist_;

    yae::TTime channel_scan_period_;
    yae::TTime epg_refresh_period_;
    yae::TTime margin_;
  };

}


#endif // YAE_DVR_H_
