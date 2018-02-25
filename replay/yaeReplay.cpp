// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 17 11:05:51 MDT 2016
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <string>

// boost:
#include <boost/filesystem/path.hpp>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"

// local:
#include "yaeReplay.h"

// namespace shortcuts:
namespace fs = boost::filesystem;


namespace yae
{
  //----------------------------------------------------------------
  // load
  //
  TDemuxerInterfacePtr
  load(DemuxerSummary & summary,
       const std::list<std::string> & sources,
       const std::map<std::string, ClipInfo> & clip,
       // these are expressed in seconds:
       const double buffer_duration,
       const double discont_tolerance)
  {
    boost::shared_ptr<SerialDemuxer> serial_demuxer(new SerialDemuxer());

    for (std::list<std::string>::const_iterator i = sources.begin();
         i != sources.end(); ++i)
    {
      const std::string & filePath = *i;

      std::list<TDemuxerPtr> demuxers;
      if (!open_primary_and_aux_demuxers(filePath, demuxers))
      {
        // failed to open the primary resource:
        av_log(NULL, AV_LOG_WARNING,
               "failed to open %s, skipping...",
               filePath.c_str());
        continue;
      }

      boost::shared_ptr<ParallelDemuxer>
        parallel_demuxer(new ParallelDemuxer());

      // wrap each demuxer in a DemuxerBuffer, build a summary:
      for (std::list<TDemuxerPtr>::const_iterator
             i = demuxers.begin(); i != demuxers.end(); ++i)
      {
        const TDemuxerPtr & demuxer = *i;

        TDemuxerInterfacePtr
          buffer(new DemuxerBuffer(demuxer, buffer_duration));

        DemuxerSummary summary;
        buffer->summarize(summary, discont_tolerance);

        std::cout
          << "\n" << demuxer->resourcePath() << ":\n"
          << summary << std::endl;

        parallel_demuxer->append(buffer, summary);
      }

      // summarize the demuxer:
      DemuxerSummary summary;
      parallel_demuxer->summarize(summary, discont_tolerance);

      // show the summary:
      std::cout << "\nparallel:\n" << summary << std::endl;

      if (yae::has(clip, filePath))
      {
        const ClipInfo & trim = yae::at(clip, filePath);
        const FramerateEstimator & fe = yae::at(summary.fps_, trim.track_);

        double fps = fe.best_guess();
        Timespan pts_span;

        if (!parse_time(pts_span.t0_, trim.t0_.c_str(), NULL, NULL, fps))
        {
          av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t0_.c_str());
          return TDemuxerInterfacePtr();
        }

        if (!parse_time(pts_span.t1_, trim.t1_.c_str(), NULL, NULL, fps))
        {
          av_log(NULL, AV_LOG_ERROR, "failed to parse %s", trim.t1_.c_str());
          return TDemuxerInterfacePtr();
        }

        boost::shared_ptr<TrimmedDemuxer> clip_demuxer(new TrimmedDemuxer());
        clip_demuxer->trim(parallel_demuxer, summary, trim.track_, pts_span);

        // summarize clip demuxer:
        DemuxerSummary clip_summary;
        clip_demuxer->summarize(clip_summary, discont_tolerance);

        // show clip summary:
        std::cout << "\ntrimmed:\n" << clip_summary << std::endl;

        serial_demuxer->append(clip_demuxer, clip_summary);
      }
      else
      {
        serial_demuxer->append(parallel_demuxer, summary);
      }
    }

    if (serial_demuxer->empty())
    {
      av_log(NULL, AV_LOG_ERROR, "failed to open any input files, gave up");
      return TDemuxerInterfacePtr();
    }

    // unwrap serial demuxer if there is just 1 source:
    if (serial_demuxer->num_sources() == 1)
    {
      summary = serial_demuxer->summaries().front();
      return serial_demuxer->sources().front();
    }

    serial_demuxer->summarize(summary, discont_tolerance);

    // show the summary:
    std::cout << "\nserial:\n" << summary << std::endl;

    return serial_demuxer;
  }

  //----------------------------------------------------------------
  // demux
  //
  void
  demux(const TDemuxerInterfacePtr & demuxer,
        const DemuxerSummary & summary,
        const std::string & output_path,
        bool save_keyframes)
  {
    std::map<int, TTime> prog_dts;
    while (true)
    {
      AVStream * stream = NULL;
      TPacketPtr packet_ptr = demuxer->get(stream);
      if (!packet_ptr)
      {
        break;
      }

      // shortcuts:
      AvPkt & pkt = *packet_ptr;
      AVPacket & packet = pkt.get();

      std::cout
        << pkt.trackId_
        << ", demuxer: " << std::setw(2) << pkt.demuxer_->demuxer_index()
        << ", program: " << std::setw(3) << pkt.program_
        << ", pos: " << std::setw(12) << std::setfill(' ') << packet.pos
        << ", size: " << std::setw(6) << std::setfill(' ') << packet.size;

      TTime dts;
      if (get_dts(dts, stream, packet))
      {
        std::cout << ", dts: " << dts;

        TTime prev_dts =
          yae::get(prog_dts, pkt.program_,
                   TTime(std::numeric_limits<int64_t>::min(), dts.base_));

        // keep dts for reference:
        prog_dts[pkt.program_] = dts;

        if (dts < prev_dts)
        {
          av_log(NULL, AV_LOG_ERROR,
                 "non-monotonically increasing DTS detected, "
                 "program %03i, prev %s, curr %s\n",
                 pkt.program_,
                 prev_dts.to_hhmmss_frac(1000, ":", ".").c_str(),
                 dts.to_hhmmss_frac(1000, ":", ".").c_str());

          // the demuxer should always provide monotonically increasing DTS:
          YAE_ASSERT(false);
        }
      }
      else
      {
        // the demuxer should always provide a DTS:
        YAE_ASSERT(false);
      }

      if (packet.pts != AV_NOPTS_VALUE)
      {
        TTime pts(stream->time_base.num * packet.pts,
                  stream->time_base.den);

        std::cout << ", pts: " << pts;
      }

      if (packet.duration)
      {
        TTime dur(stream->time_base.num * packet.duration,
                  stream->time_base.den);

        std::cout << ", dur: " << dur;
      }

      const AVMediaType codecType = stream->codecpar->codec_type;

      int flags = packet.flags;
      if (codecType != AVMEDIA_TYPE_VIDEO)
      {
        flags &= ~(AV_PKT_FLAG_KEY);
      }

      bool is_keyframe = false;
      if (flags)
      {
        std::cout << ", flags:";

        if ((flags & AV_PKT_FLAG_KEY))
        {
          std::cout << " keyframe";
          is_keyframe = true;
        }

        if ((flags & AV_PKT_FLAG_CORRUPT))
        {
          std::cout << " corrupt";
        }

        if ((flags & AV_PKT_FLAG_DISCARD))
        {
          std::cout << " discard";
        }

        if ((flags & AV_PKT_FLAG_TRUSTED))
        {
          std::cout << " trusted";
        }

        if ((flags & AV_PKT_FLAG_DISPOSABLE))
        {
          std::cout << " disposable";
        }
      }

      for (int j = 0; j < packet.side_data_elems; j++)
      {
        std::cout
          << ", side_data[" << j << "] = { type: "
          << packet.side_data[j].type << ", size: "
          << packet.side_data[j].size << " }";
      }

      std::cout << std::endl;

      if (is_keyframe && save_keyframes)
      {
        fs::path folder = (fs::path(output_path) /
                           boost::replace_all_copy(pkt.trackId_, ":", "."));
        fs::create_directories(folder);

        std::string fn = (dts.to_hhmmss_frac(1000, "", ".") + ".png");
        std::string path((folder / fn).string());

        TrackPtr track_ptr = yae::get(summary.decoders_, pkt.trackId_);
        VideoTrackPtr decoder_ptr =
          boost::dynamic_pointer_cast<VideoTrack, Track>(track_ptr);

        if (!save_keyframe(path, decoder_ptr, packet_ptr, 0, 0, 0.0, 1.0))
        {
          break;
        }
      }
    }
  }
}
