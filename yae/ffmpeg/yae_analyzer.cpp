// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat May 31 04:47:09 PM MDT 2025
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_analyzer.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/utils/yae_utils.h"

// standard:
#include <iterator>


namespace yae
{

  //----------------------------------------------------------------
  // Analyzer::PacketInfo::PacketInfo
  //
  Analyzer::PacketInfo::PacketInfo(const AVStream * stream,
                                   const AVPacket * packet):
    byte_pos_(-1),
    codec_id_(AV_CODEC_ID_NONE),
    dts_(0, 0)
  {
    this->init(stream, packet);
  }

  //----------------------------------------------------------------
  // Analyzer::PacketInfo::init
  //
  void
  Analyzer::PacketInfo::init(const AVStream * stream,
                             const AVPacket * packet)
  {
    byte_pos_ = packet->pos;

    if (stream->codecpar)
    {
      codec_id_ = stream->codecpar->codec_id;
    }

    static const Rational msec(1, 1000);
    if (packet->dts != AV_NOPTS_VALUE)
    {
      dts_.base_ = 1000;
      dts_.time_ = av_rescale_q(packet->dts, stream->time_base, msec);
    }
    else if (packet->pts != AV_NOPTS_VALUE)
    {
      dts_.base_ = 1000;
      dts_.time_ = av_rescale_q(packet->pts, stream->time_base, msec);
    }
  }


  //----------------------------------------------------------------
  // Analyzer::StreamInfo::StreamInfo
  //
  Analyzer::StreamInfo::StreamInfo():
    num_packets_(0),
    dts_anomaly_(false)
  {}

  //----------------------------------------------------------------
  // Analyzer::StreamInfo::add
  //
  void
  Analyzer::StreamInfo::add(const AVStream * stream,
                            const AVPacket * packet)
  {
    Analyzer::PacketInfo pkt(stream, packet);
    if (pkt.byte_pos_ != -1 && pkt.dts_.valid())
    {
      if (!pkt_.empty())
      {
        const PacketInfo & prev = pkt_.back();

        // check for non-monotonically increasing time stamps:
        if (pkt.dts_ <= prev.dts_)
        {
          dts_anomaly_ = true;
        }

        // check for forward jumps in time:
        double dt_sec = (pkt.dts_ - prev.dts_).sec();
        if (dt_sec > 60.0)
        {
          dts_anomaly_ = true;
        }
      }

      pkt_.push_back(pkt);
    }

    num_packets_ += 1;
  }

  //----------------------------------------------------------------
  // Analyzer::StreamInfo::duration
  //
  double
  Analyzer::StreamInfo::duration_sec() const
  {
    if (pkt_.empty())
    {
      return 0.0;
    }

    const PacketInfo & a = pkt_.front();
    const PacketInfo & b = pkt_.back();
    double sec = (b.dts_ - a.dts_).sec();
    return sec;
  }

  //----------------------------------------------------------------
  // Analyzer::StreamInfo::bytes_per_sec
  //
  double
  Analyzer::StreamInfo::bytes_per_sec() const
  {
    if (!yae::is_size_two_or_more(pkt_))
    {
      return 0.0;
    }

    const PacketInfo & a = pkt_.front();
    const PacketInfo & b = pkt_.back();
    double sec = (b.dts_ - a.dts_).sec();
    if (sec <= 0.0)
    {
      return 0.0;
    }

    double bytes = double(b.byte_pos_ - a.byte_pos_);
    double r = bytes / sec;
    return r;
  }


  //----------------------------------------------------------------
  // Analyzer::SourceInfo::SourceInfo
  //
  Analyzer::SourceInfo::SourceInfo()
  {
    this->clear();
  }

  //----------------------------------------------------------------
  // Analyzer::SourceInfo::clear
  //
  void
  Analyzer::SourceInfo::clear()
  {
    byte_pos_ = std::numeric_limits<int64_t>::max();
    stream_.clear();
    num_packets_ = 0;
  }

  //----------------------------------------------------------------
  // Analyzer::SourceInfo::get_info_at_byte_pos
  //
  bool
  Analyzer::SourceInfo::get_info_at_byte_pos(AVFormatContext * ctx,
                                             int64_t byte_pos,
                                             int64_t pos_tolerance)
  {
    this->clear();

    int seek_flags = (AVSEEK_FLAG_BYTE | AVSEEK_FLAG_ANY);
    int err = avformat_seek_file(ctx,
                                 -1, // stream index
                                 byte_pos - pos_tolerance,
                                 byte_pos,
                                 byte_pos + pos_tolerance,
                                 seek_flags);
    if (err < 0)
    {
      return false;
    }

    TPacketPtr packetPtr(new AvPkt());
    AVPacket & packet = packetPtr->get();
    for (int i = 0; i < 3000; i++)
    {
      err = av_read_frame(ctx, &packet);
      if (err < 0)
      {
        break;
      }

      if (packet.stream_index < ctx->nb_streams)
      {
        const AVStream * s = ctx->streams[packet.stream_index];
        YAE_ASSERT(s->index == packet.stream_index);

        AVMediaType media_type =
          s->codecpar ?
          s->codecpar->codec_type :
          AVMEDIA_TYPE_UNKNOWN;

        if (media_type == AVMEDIA_TYPE_VIDEO ||
            media_type == AVMEDIA_TYPE_AUDIO)
        {
          StreamInfo & info = stream_[s->index];
          info.add(s, &packet);
          num_packets_ += 1;

          if (packet.pos != -1 &&
              packet.pos < byte_pos_)
          {
            byte_pos_ = packet.pos;
          }

          double info_seconds = info.duration_sec();
          if (info_seconds > 1.0)
          {
            // that's probably enough:
            break;
          }
        }
      }

      av_packet_unref(&packet);
    }

    if (byte_pos_ == std::numeric_limits<int64_t>::max())
    {
      byte_pos_ = byte_pos;
    }

    return num_packets_ > 0;
  }

  //----------------------------------------------------------------
  // Analyzer::SourceInfo::init
  //
  bool
  Analyzer::SourceInfo::init(AVFormatContext * ctx,
                             int64_t start_pos,
                             int64_t end_pos,
                             int64_t packet_size,
                             int64_t packet_step)
  {
    int64_t pos_step = packet_size * packet_step;
    int64_t pos_tolerance = (pos_step < 0) ? -pos_step : pos_step;
    int64_t num_steps = (end_pos - start_pos) / pos_step;
    for (int64_t i = 0; i <= num_steps; i += 1)
    {
      int64_t pos = start_pos + i * pos_step;
      if (this->get_info_at_byte_pos(ctx, pos, pos_tolerance))
      {
        return true;
      }
    }

    return false;
  }


  //----------------------------------------------------------------
  // verify_streams_and_dts
  //
  bool
  Analyzer::verify_streams_and_dts(const Analyzer::SourceInfo & src_a,
                                   const Analyzer::SourceInfo & src_b) const
  {
    YAE_ASSERT(src_a.byte_pos_ <= src_b.byte_pos_);

    std::map<int, StreamInfo>::const_iterator ia = src_a.stream_.begin();
    std::map<int, StreamInfo>::const_iterator ib = src_b.stream_.begin();

    std::map<int, StreamInfo>::const_iterator ia_end = src_a.stream_.end();
    std::map<int, StreamInfo>::const_iterator ib_end = src_b.stream_.end();

    for (; ia != ia_end && ib != ib_end; ++ia, ++ib)
    {
      int xa = ia->first;
      int xb = ib->first;
      if (xa != xb)
      {
        // inconsistent stream index bethween this and src_b:
        return false;
      }

      const StreamInfo & sa = ia->second;
      const StreamInfo & sb = ib->second;

      if (sa.dts_anomaly_ || sb.dts_anomaly_)
      {
        return false;
      }

      // verify monotonically increasing DTS:
      if (sa.pkt_.empty() || sb.pkt_.empty())
      {
        continue;
      }

      const PacketInfo & pa = sa.pkt_.front();
      const PacketInfo & pb = sb.pkt_.front();
      YAE_ASSERT(pa.byte_pos_ <= pb.byte_pos_);
#if 0
      if (pa.codec_id_ != pb.codec_id_)
      {
        return false;
      }
#endif
      if (pb.dts_ < pa.dts_)
      {
        return false;
      }

      // check for gaps by estimating the expected DTS:
      double ta = pa.dts_.sec();
      double bytes_per_sec = sa.bytes_per_sec();
      if (bytes_per_sec <= 0.0)
      {
        continue;
      }

      int64_t ab = pb.byte_pos_ - pa.byte_pos_;
      double tb = pb.dts_.sec();
      double dt = tb - ta;

      double dt_expect = ab / bytes_per_sec;
      double tb_expect = ta + dt_expect;
      double err_sec = tb - tb_expect;
      if (err_sec < -60.0 || err_sec > 60.0)
      {
        return false;
      }
    }

    return (ia == ia_end && ib == ib_end);
  }

  //----------------------------------------------------------------
  // Analyzer::find_anomalies
  //
  bool
  Analyzer::find_anomalies(AVFormatContext * ctx,
                           Analyzer::SourceInfo & src_a,
                           Analyzer::SourceInfo & src_b,
                           int64_t packet_size,
                           int64_t packet_step,
                           bool asap)
  {
    // check for stream anomalies:
    // - non-monotonically increasing DTS
    // - codec changes
    if (verify_streams_and_dts(src_a, src_b))
    {
      return false;
    }
    else if (asap)
    {
      detected_.push_back(src_b);
      return true;
    }

    int64_t num_packets = (src_b.byte_pos_ - src_a.byte_pos_) / packet_size;
    if (num_packets < packet_step)
    {
      detected_.push_back(src_b);
      return true;
    }

    SourceInfo src_ab;
    int64_t pos = src_a.byte_pos_ + packet_size * (num_packets / 2);
    YAE_ASSERT(src_ab.init(ctx, pos, pos, packet_size, packet_step));

    if (src_a.byte_pos_ == src_ab.byte_pos_ ||
        src_b.byte_pos_ == src_ab.byte_pos_)
    {
      detected_.push_back(src_b);
      return true;
    }

    bool a = find_anomalies(ctx, src_a, src_ab, packet_size, packet_step, asap);
    bool b = find_anomalies(ctx, src_ab, src_b, packet_size, packet_step, asap);
    return (a || b);
  }

  //----------------------------------------------------------------
  // find_anomalies
  //
  bool
  Analyzer::find_anomalies(AVFormatContext * ctx,
                           int64_t packet_size,
                           bool asap)
  {
    samples_.clear();
    detected_.clear();

    if (!(ctx && ctx->pb && ctx->pb->seekable))
    {
      return false;
    }

    // save current file position:
    int64_t original_pos = ctx->pb->pos;

    // sample the file uniformly:
    int64_t num_samples = 33;

    int64_t file_size = avio_size(ctx->pb);
    int64_t num_packets = (file_size / packet_size);
    int64_t packet_step = num_packets / 100;
    if (packet_step > 1000)
    {
      packet_step = 1000;
    }
    else
    {
      num_samples = 2;
    }

    for (int i = 0; i < num_samples; ++i)
    {
      samples_.push_back(SourceInfo());
      SourceInfo & src = samples_.back();

      int64_t pos = packet_size * ((num_packets * (i + 0)) / num_samples);
      int64_t end = packet_size * ((num_packets * (i + 1)) / num_samples);
      if (!src.init(ctx, pos, end, packet_size, packet_step))
      {
        samples_.pop_back();
        continue;
      }
    }

    if (yae::is_size_two_or_more(samples_))
    {
      std::list<SourceInfo>::iterator prev = samples_.begin();
      std::list<SourceInfo>::iterator curr = yae::next(prev);
      for (; curr != samples_.end(); prev = curr, ++curr)
      {
        SourceInfo & src_a = *prev;
        SourceInfo & src_b = *curr;
        this->find_anomalies(ctx, src_a, src_b, packet_size, packet_step, asap);
      }
    }

    // restore original file position:
    {
      int seek_flags = (AVSEEK_FLAG_BYTE | AVSEEK_FLAG_ANY);
      avformat_seek_file(ctx,
                         -1, // stream index
                         std::numeric_limits<int64_t>::min(),
                         original_pos,
                         std::numeric_limits<int64_t>::max(),
                         seek_flags);
    }

    return !detected_.empty();
  }


  //----------------------------------------------------------------
  // FileRegion::Track::Track
  //
  FileRegion::Track::Track():
    codec_id_(AV_CODEC_ID_NONE),
    num_packets_(0)
  {}


  //----------------------------------------------------------------
  // FileRegion::Program::empty
  //
  bool
  FileRegion::Program::empty() const
  {
    for (std::map<int, FileRegion::Track>::const_iterator
           i = tracks_.begin(); i != tracks_.end(); ++i)
    {
      const FileRegion::Track & track = i->second;
      if (!track.empty())
      {
        return false;
      }
    }

    return true;
  }


  //----------------------------------------------------------------
  // FileRegion::FileRegion
  //
  FileRegion::FileRegion(uint64_t p0, uint64_t p1):
    p0_(p0),
    p1_(p1)
  {}

  //----------------------------------------------------------------
  // FileRegion::empty
  //
  bool
  FileRegion::empty() const
  {
    for (std::map<int, FileRegion::Program>::const_iterator
           i = programs_.begin(); i != programs_.end(); ++i)
    {
      const FileRegion::Program & program = i->second;
      if (!program.empty())
      {
        return false;
      }
    }

    return true;
  }


  //----------------------------------------------------------------
  // analyze
  //
  bool
  analyze(AVFormatContext * ctx, std::list<FileRegion> & clips)
  {
    clips.clear();

    if (!(ctx && ctx->pb && ctx->pb->seekable))
    {
      return false;
    }

    // save current file position:
    int64_t restore_pos = ctx->pb->pos;
    int64_t file_size = avio_size(ctx->pb);
    int err = avformat_seek_file(ctx,
                                 -1, // stream index
                                 0, // min_ts
                                 0, // ts
                                 file_size, // max_ts
                                 AVSEEK_FLAG_BYTE);
    if (err < 0)
    {
      return false;
    }

    FileRegion clip(0, file_size);
    while (true)
    {
      TPacketPtr pkt_ptr(new AvPkt());
      AVPacket & pkt = pkt_ptr->get();

      int err = av_read_frame(ctx, &pkt);
      if (err == AVERROR_EOF)
      {
        break;
      }
      else if (err < 0)
      {
        continue;
      }

      if (pkt.pos < 0)
      {
        continue;
      }

      if (ctx->nb_streams <= pkt.stream_index)
      {
        continue;
      }

      const AVStream * s = ctx->streams[pkt.stream_index];
      if (!s || !s->codecpar)
      {
        continue;
      }

      if (s->codecpar->codec_id == AV_CODEC_ID_NONE)
      {
        continue;
      }

      if (pkt.dts == AV_NOPTS_VALUE)
      {
        continue;
      }

      TTime dts(s->time_base.num * pkt.dts,
                s->time_base.den);

      TTime dur(s->time_base.num * pkt.duration,
                s->time_base.den);

#if 1
      AVMediaType media_type =
        s->codecpar ?
        s->codecpar->codec_type :
        AVMEDIA_TYPE_UNKNOWN;

      // ignore potentially sparse media:
      if (media_type != AVMEDIA_TYPE_VIDEO &&
          media_type != AVMEDIA_TYPE_AUDIO)
      {
        continue;
      }
#endif

      int prog_id = -1;
      if (ctx->nb_programs > 0)
      {
        const AVProgram * found =
          av_find_program_from_stream(ctx, NULL, pkt.stream_index);
        if (!found)
        {
          continue;
        }

        prog_id = found->id;
      }

      // check for timeline anomalies:
      {
        FileRegion::Program & program = clip.programs_[prog_id];
        FileRegion::Track & track = program.tracks_[pkt.stream_index];

        if (!track.dts_span_.empty())
        {
          if (// check if DTS jumped back in time:
              dts < track.dts_span_.t1_ ||

              // check if DTS jumped forward in time:
              track.dts_span_.t1_ + TTime(10, 1) < dts)
          {
            // timeline anomaly:
            clip.p1_ = pkt.pos;
            clips.push_back(clip);
            clip = FileRegion(pkt.pos, file_size);
          }
        }
      }

      FileRegion::Program & program = clip.programs_[prog_id];
      FileRegion::Track & track = program.tracks_[pkt.stream_index];
      track.codec_id_ = s->codecpar->codec_id;
      track.dts_span_.add(dts);
      track.num_packets_ += 1;
    }

    if (!clip.empty())
    {
      clips.push_back(clip);
    }

    avformat_seek_file(ctx,
                       -1, // stream index
                       0, // min_ts
                       restore_pos, // ts
                       file_size, // max_ts
                       AVSEEK_FLAG_BYTE);

    return true;
  }

  //----------------------------------------------------------------
  // analyze
  //
  bool
  analyze(const std::string & resource_path,
          std::list<FileRegion> & clips)
  {
    AvInputContextPtr context(avformat_alloc_context());
    AVFormatContext * ctx = context.get();
    if (!ctx)
    {
      return false;
    }

    AVDictionary * options = NULL;

    // set probesize to 64 MiB:
    av_dict_set(&options, "probesize", "67108864", 0);

    // set analyze duration to 10 seconds:
    av_dict_set(&options, "analyzeduration", "10000000", 0);

    // set AVFMT_FLAG_GENPTS:
    av_dict_set(&options, "fflags", "+genpts", AV_DICT_APPEND);

    // set AVFMT_FLAG_DISCARD_CORRUPT:
    av_dict_set(&options, "fflags", "+discardcorrupt", AV_DICT_APPEND);

#ifdef AVFMT_FLAG_ALLOW_CODEC_CHANGES
    // Allow AVStream.codecpar codec_type and codec_id to change
    // when MPEG-TS PMT ES stream_type changes at runtime:
    // av_dict_set(&options, "allow_codec_changes", "1", 0);
    ctx->flags |= AVFMT_FLAG_ALLOW_CODEC_CHANGES;
#endif

    int err = avformat_open_input(&ctx,
                                  resource_path.c_str(),
                                  NULL, // AVInputFormat to force
                                  &options);
    av_dict_free(&options);

    if (err != 0)
    {
      return false;
    }

    err = avformat_find_stream_info(ctx, NULL);
    if (err < 0)
    {
      return false;
    }

    return yae::analyze(ctx, clips);
  }

}
