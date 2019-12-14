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

      yae::optional<uint32_t> ch_num_;
      yae::optional<struct tm> date_;
      yae::optional<Timespan> when_;
      std::string title_;
      std::string description_;

    protected:
      mutable yae::optional<boost::regex> rx_title_;
      mutable yae::optional<boost::regex> rx_description_;
    };

    bool matches(const yae::mpeg_ts::EPG::Channel & channel,
                 const yae::mpeg_ts::EPG::Program & program) const;

    std::list<Item> items_;
  };


  //----------------------------------------------------------------
  // Recording
  //
  struct Recording
  {
    Recording();

    yae::TOpenFilePtr open_file(const fs::path & basedir);

    yae::shared_ptr<IStream> stream_;
    uint32_t gps_t1_;
    std::string filename_;
    yae::TOpenFilePtr file_;
    bool cancelled_;
  };

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
  };

}


#endif // YAE_DVR_H_
