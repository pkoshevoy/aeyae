// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat May 31 04:47:09 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ANALYZER_H_
#define YAE_ANALYZER_H_

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/utils/yae_time.h"

// standard:
#include <list>
#include <map>
#include <string>

// ffmpeg:
extern "C"
{
#include <libavformat/avformat.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // Analyzer
  //
  struct YAE_API Analyzer
  {

    //----------------------------------------------------------------
    // PacketInfo
    //
    struct YAE_API PacketInfo
    {
      PacketInfo(const AVStream * stream,
                 const AVPacket * packet);

      void init(const AVStream * stream,
                const AVPacket * packet);

      AVCodecID codec_id_;
      int64_t byte_pos_;
      TTime dts_;
    };

    //----------------------------------------------------------------
    // StreamInfo
    //
    struct YAE_API StreamInfo
    {
      StreamInfo();

      void add(const AVStream * stream,
               const AVPacket * packet);

      double duration_sec() const;
      double bytes_per_sec() const;

      std::list<PacketInfo> pkt_;
      std::size_t num_packets_;
      bool dts_anomaly_;
    };

    //----------------------------------------------------------------
    // SourceInfo
    //
    struct YAE_API SourceInfo
    {
      SourceInfo();

      void clear();

      bool get_info_at_byte_pos(AVFormatContext * ctx,
                                int64_t byte_pos,
                                int64_t pos_tolerance);

      bool init(AVFormatContext * ctx,
                int64_t start_pos,
                int64_t end_pos,
                int64_t packet_size,
                int64_t packet_step);

      std::map<int, StreamInfo> stream_;
      std::size_t num_packets_;
      int64_t byte_pos_;
    };

    bool verify_streams_and_dts(const SourceInfo & src_a,
                                const SourceInfo & src_b) const;

    bool find_anomalies(AVFormatContext * ctx,
                        SourceInfo & src_a,
                        SourceInfo & src_b,
                        int64_t packet_size,
                        int64_t packet_step,
                        bool asap = false);

    bool find_anomalies(AVFormatContext * ctx,
                        int64_t packet_size,
                        bool asap = false);

    std::list<SourceInfo> samples_;
    std::list<SourceInfo> detected_;
  };

}


#endif // YAE_ANALYZER_H_
