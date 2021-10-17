// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <map>
#include <set>

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

// yae includes:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/utils/yae_benchmark.h"

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // TimePos::TimePos
  //
  TimePos::TimePos(double sec):
    sec_(sec)
  {}

  //----------------------------------------------------------------
  // TimePos::to_str
  //
  std::string
  TimePos::to_str() const
  {
    return TTime(sec_).to_hhmmss_ms();
  }

  //----------------------------------------------------------------
  // TimePos::lt
  //
  bool
  TimePos::lt(const TFrameBase & f, double dur) const
  {
    double t = f.time_.sec() + dur;
    return (sec_ < t);
  }

  //----------------------------------------------------------------
  // TimePos::gt
  //
  bool
  TimePos::gt(const TFrameBase & f, double dur) const
  {
    double t = f.time_.sec() + dur;
    return (t < sec_);
  }

  //----------------------------------------------------------------
  // TimePos::to_str
  //
  std::string
  TimePos::to_str(const TFrameBase & f, double dur) const
  {
    return (f.time_ + dur).to_hhmmss_ms();
  }

  //----------------------------------------------------------------
  // TimePos::seek
  //
  int
  TimePos::seek(AVFormatContext * context, const AVStream * stream) const
  {
    int64_t ts = int64_t(sec_ * double(AV_TIME_BASE));

    if (stream)
    {
      AVRational tb;
      tb.num = 1;
      tb.den = AV_TIME_BASE;

      ts = av_rescale_q(ts, tb, stream->time_base);
    }

    int streamIndex = stream ? stream->index : -1;
    int seekFlags = 0;
    int err = avformat_seek_file(context,
                                 streamIndex,
                                 kMinInt64,
                                 ts,
                                 ts, // kMaxInt64,
                                 seekFlags);

    if (err < 0)
    {
      if (!ts)
      {
        // must be trying to rewind a stream of undefined duration:
        seekFlags |= AVSEEK_FLAG_BYTE;
      }

      err = avformat_seek_file(context,
                               streamIndex,
                               kMinInt64,
                               ts,
                               ts, // kMaxInt64,
                               seekFlags | AVSEEK_FLAG_ANY);
    }

    return err;
  }


  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt(const AVPacket * pkt):
    packet_(av_packet_alloc()),
    pbuffer_(NULL),
    demuxer_(NULL),
    program_(0)
  {
    if (pkt)
    {
      av_packet_ref(packet_, pkt);
    }
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 0, 0)
    else
    {
      av_init_packet(packet_);
    }
#endif
  }

  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt(const AvPkt & pkt):
    packet_(av_packet_alloc()),
    pbuffer_(pkt.pbuffer_),
    demuxer_(pkt.demuxer_),
    program_(pkt.program_),
    trackId_(pkt.trackId_)
  {
    av_packet_ref(packet_, pkt.packet_);
  }

  //----------------------------------------------------------------
  // AvPkt::~AvPkt
  //
  AvPkt::~AvPkt()
  {
    av_packet_free(&packet_);
  }

  //----------------------------------------------------------------
  // AvPkt::operator =
  //
  AvPkt &
  AvPkt::operator = (const AvPkt & pkt)
  {
    if (this != &pkt)
    {
      av_packet_unref(packet_);
      av_packet_ref(packet_, pkt.packet_);

      demuxer_ = pkt.demuxer_;
      trackId_ = pkt.trackId_;
      program_ = pkt.program_;
      pbuffer_ = pkt.pbuffer_;
    }

    return *this;
  }

  //----------------------------------------------------------------
  // clone
  //
  TPacketPtr
  clone(const TPacketPtr & packet_ptr)
  {
    return packet_ptr ? TPacketPtr(new AvPkt(*packet_ptr)) : TPacketPtr();
  }


  //----------------------------------------------------------------
  // AvCodecContextPtr::destroy
  //
  void
  AvCodecContextPtr::destroy(AVCodecContext * ctx)
  {
    avcodec_close(ctx);
    avcodec_free_context(&ctx);
  }


  //----------------------------------------------------------------
  // tryToOpen
  //
  AvCodecContextPtr
  tryToOpen(const AVCodec * c,
            const AVCodecParameters * params,
            AVDictionary * opts)
  {
    unsigned int nthreads = boost::thread::hardware_concurrency();
    nthreads = std::min<unsigned int>(16, nthreads);

    AvCodecContextPtr ctx(avcodec_alloc_context3(c));
    if (params)
    {
      avcodec_parameters_to_context(ctx.get(), params);
    }

    av_dict_set_int(&opts, "threads", nthreads, 0);

    int err = avcodec_open2(ctx.get(), c, &opts);
    if (err < 0)
    {
      return AvCodecContextPtr();
    }

    return ctx;
  }


  //----------------------------------------------------------------
  // verify_pts
  //
  // verify that presentation timestamps are monotonically increasing
  //
  bool
  verify_pts(bool hasPrevPTS,
             const TTime & prevPTS,
             const TTime & nextPTS,
             const AVStream * stream,
             const char * debugMessage)
  {
    bool ok = (int64_t(nextPTS.time_) != AV_NOPTS_VALUE &&
               int64_t(nextPTS.base_) != AV_NOPTS_VALUE &&
               nextPTS.base_ != 0 &&
               (!hasPrevPTS ||
                (prevPTS.base_ == nextPTS.base_ ?
                 prevPTS.time_ < nextPTS.time_ :
                 prevPTS.sec() < nextPTS.sec())));
#if 0
    if (ok && debugMessage)
    {
      yae_debug << "PTS OK: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << " = " << nextPTS.to_hhmmss_us()
                << ", " << debugMessage << "\n";
    }
    else if (debugMessage && hasPrevPTS)
    {
      yae_debug << "prev PTS: "
                << prevPTS.time_ << "/" << prevPTS.base_
                << " = " << prevPTS.to_hhmmss_us()
                << ", next PTS: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << " = " << nextPTS.to_hhmmss_us()
                << ", " << debugMessage << "\n";
    }
#else
    (void)debugMessage;
#endif

    if (!ok || !stream)
    {
      return ok;
    }

    return true;
  }



  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(AVFormatContext * context, AVStream * stream):
    packetRateEstimator_(1536),
    thread_(this),
    context_(context),
    stream_(stream),
    sent_(0),
    received_(0),
    errors_(0),
    posIn_(new TimePos(TTime::min_flicks_as_sec())),
    posOut_(new TimePos(TTime::max_flicks_as_sec())),
    playbackEnabled_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    if (context_ && stream_)
    {
      YAE_ASSERT(context_->streams[stream_->index] == stream_);
    }
  }

  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(Track * track):
    packetRateEstimator_(1536),
    packetQueue_(track->packetQueue_.getMaxSize()),
    thread_(this),
    context_(NULL),
    stream_(NULL),
    sent_(0),
    received_(0),
    errors_(0),
    posIn_(new TimePos(TTime::min_flicks_as_sec())),
    posOut_(new TimePos(TTime::max_flicks_as_sec())),
    playbackEnabled_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    std::swap(hw_device_ctx_, track->hw_device_ctx_);
    std::swap(hw_frames_ctx_, track->hw_frames_ctx_);
    std::swap(context_, track->context_);
    std::swap(stream_, track->stream_);
    std::swap(codecContext_, track->codecContext_);
  }

  //----------------------------------------------------------------
  // Track::~Track
  //
  Track::~Track()
  {
    threadStop();
    close();
  }

  //----------------------------------------------------------------
  // Track::initTraits
  //
  bool
  Track::initTraits()
  {
    YAE_ASSERT(false);
    return true;
  }

  //----------------------------------------------------------------
  // Track::open
  //
  AVCodecContext *
  Track::open()
  {
    if (codecContext_)
    {
      return codecContext_.get();
    }

    if (!stream_)
    {
      return NULL;
    }

    const AVCodecParameters & params = *(stream_->codecpar);
    const AVCodec * codec = avcodec_find_decoder(params.codec_id);
    if (!codec && stream_->codecpar->codec_id != AV_CODEC_ID_TEXT)
    {
      // unsupported codec:
      return NULL;
    }

    int err = 0;
    yae::AvBufferRef hw_device_ctx;

#if 1 // ndef __APPLE__
    int hw_config_index = 0;
    while (params.width > 640 && params.height > 360)
    {
      const AVCodecHWConfig * hw =
        avcodec_get_hw_config(codec, hw_config_index);

      if (!hw)
      {
        break;
      }

      hw_config_index++;
      int hw_device_frames = (AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX |
                              AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX);
      if ((hw->methods & hw_device_frames) == hw_device_frames)
      {
        err = av_hwdevice_ctx_create(&hw_device_ctx.ref_,
                                     hw->device_type,
                                     NULL, // const char *, device to open
                                     NULL, // AVDictionary *, device options
                                     0); // flags
        if (err >= 0)
        {
          yae_ilog("av_hwdevice_ctx_create succeeded for %s",
                   av_hwdevice_get_type_name(hw->device_type));
          break;
        }

        yae_wlog("av_hwdevice_ctx_create failed for %s: %s",
                 av_hwdevice_get_type_name(hw->device_type),
                 yae::av_errstr(err).c_str());
        YAE_ASSERT(!hw_device_ctx.ref_);
      }
    }
#endif

    AvCodecContextPtr ctx_ptr(avcodec_alloc_context3(codec));
    AVCodecContext * ctx = ctx_ptr.get();
    avcodec_parameters_to_context(ctx, &params);
    ctx->opaque = this;

    if (hw_device_ctx.ref_)
    {
      ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx.ref_);
    }

#if 1
    ctx->skip_frame = AVDISCARD_DEFAULT;
    ctx->error_concealment = 3;
    ctx->err_recognition = AV_EF_CAREFUL;
    ctx->skip_loop_filter = AVDISCARD_DEFAULT;
    ctx->workaround_bugs = 1;
#endif
    ctx->pkt_timebase = stream_->time_base;

    int nthreads = boost::thread::hardware_concurrency();
    nthreads =
      ctx->hw_device_ctx ?
      std::min(24, nthreads) :
      std::max(1, nthreads);

    AVDictionary * opts = NULL;
    av_dict_set_int(&opts, "threads", nthreads, 0);

    ctx->thread_count = nthreads;
    // ctx->thread_type = FF_THREAD_SLICE;
    // ctx->thread_type = FF_THREAD_FRAME;

    err = avcodec_open2(ctx, codec, &opts);
    if (err < 0)
    {
      yae_elog("avcodec_open2 failed: %s", yae::av_errstr(err).c_str());
      return NULL;
    }

    std::swap(hw_device_ctx_, hw_device_ctx);
    std::swap(codecContext_, ctx_ptr);
    sent_ = 0;
    received_ = 0;
    errors_ = 0;
    return ctx;
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    hw_frames_ctx_.reset();
    hw_device_ctx_.reset();

    if (stream_ && codecContext_)
    {
      codecContext_.reset();
    }
  }

  //----------------------------------------------------------------
  // Track::getCodecName
  //
  const char *
  Track::getCodecName() const
  {
    return stream_ ? avcodec_get_name(stream_->codecpar->codec_id) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getName
  //
  const char *
  Track::getName() const
  {
    return stream_ ? getTrackName(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getLang
  //
  const char *
  Track::getLang() const
  {
    return stream_ ? getTrackLang(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getDuration
  //
  bool
  Track::getDuration(TTime & start, TTime & duration) const
  {
    if (!stream_)
    {
      YAE_ASSERT(false);
      return false;
    }

    bool got_start = false;
    bool got_duration = false;

    if (stream_->start_time != int64_t(AV_NOPTS_VALUE))
    {
      // return track duration:
      start.base_ = stream_->time_base.den;
      start.time_ =
        stream_->start_time != int64_t(AV_NOPTS_VALUE) ?
        stream_->time_base.num * stream_->start_time : 0;
      got_start = true;
    }

    if (stream_->duration != int64_t(AV_NOPTS_VALUE))
    {
      duration.time_ = stream_->time_base.num * stream_->duration;
      duration.base_ = stream_->time_base.den;
      got_duration = true;
    }

    if (got_start && got_duration)
    {
      return true;
    }

    if (!context_)
    {
      YAE_ASSERT(false);
      return false;
    }

    if (!got_start && context_->start_time != int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      start.base_ = AV_TIME_BASE;
      start.time_ =
        context_->start_time != int64_t(AV_NOPTS_VALUE) ?
        context_->start_time : 0;
      got_start = true;
    }

    if (!got_duration && context_->duration != int64_t(AV_NOPTS_VALUE))
    {
      duration.time_ = context_->duration;
      duration.base_ = AV_TIME_BASE;
      got_duration = true;
    }

    if (got_start && got_duration)
    {
      return true;
    }

    int64 fileSize = avio_size(context_->pb);
    int64 fileBits = fileSize * 8;

    start.base_ = AV_TIME_BASE;
    start.time_ = 0;

    if (context_->bit_rate)
    {
      double t =
        double(fileBits / context_->bit_rate) +
        double(fileBits % context_->bit_rate) /
        double(context_->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return true;
    }

    const AVCodecParameters & params = *(stream_->codecpar);
    if (context_->nb_streams == 1 && params.bit_rate)
    {
      double t =
        double(fileBits / params.bit_rate) +
        double(fileBits % params.bit_rate) /
        double(params.bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return true;
    }

    // unknown duration:
    duration.time_ = std::numeric_limits<int64>::max();
    duration.base_ = AV_TIME_BASE;
    return false;
  }

  //----------------------------------------------------------------
  // Track::threadStart
  //
  bool
  Track::threadStart()
  {
    terminator_.stopWaiting(false);
    packetQueue_.open();
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Track::packetQueueOpen
  //
  void
  Track::packetQueueOpen()
  {
    packetQueue_.open();
  }

  //----------------------------------------------------------------
  // Track::packetQueueClose
  //
  void
  Track::packetQueueClose()
  {
    packetQueue_.close();
  }

  //----------------------------------------------------------------
  // Track::packetQueueClear
  //
  void
  Track::packetQueueClear()
  {
    // this can be called on the main thread, when seeking:
    {
      boost::lock_guard<boost::mutex> lock(packetRateMutex_);
      packetRateEstimator_.clear();
    }

    packetQueue_.clear();
  }

  //----------------------------------------------------------------
  // Track::packetQueuePush
  //
  bool
  Track::packetQueuePush(const TPacketPtr & packetPtr, QueueWaitMgr * waitMgr)
  {
    if (packetPtr)
    {
      const AvPkt & pkt = *packetPtr;
      const AVPacket & packet = pkt.get();

      double rate = 0.0;

      // this can be called on the main thread, when seeking:
      {
        boost::lock_guard<boost::mutex> lock(packetRateMutex_);

        if (packet.dts != AV_NOPTS_VALUE)
        {
          TTime dts(int64_t(stream_->time_base.num) * packet.dts,
                    uint64_t(stream_->time_base.den));

          if (!packetRateEstimator_.is_monotonically_increasing(dts))
          {
            packetRateEstimator_.clear();
          }

          packetRateEstimator_.push(dts);
        }
        else
        {
          packetRateEstimator_.push_same_as_last();
        }

        rate = packetRateEstimator_.window_avg();
      }

      if (rate > 0.0)
      {
        const uint64_t new_queue_size = std::max<uint64_t>(24, rate + 0.5);
        const uint64_t max_queue_size = packetQueue_.getMaxSize();

        const uint64_t diff =
          (new_queue_size < max_queue_size) ?
          max_queue_size - new_queue_size :
          new_queue_size - max_queue_size;

        // avoid log span due to flip-flopping queue size:
        static const uint64_t padding = 10;

        const uint64_t percent_padding =
          (new_queue_size < max_queue_size) ?
          (100 * diff) / max_queue_size :
          (100 * padding) / (new_queue_size + padding);

        if ((new_queue_size > max_queue_size ||
             (diff > padding && percent_padding > 15)) &&
            packetQueue_.setMaxSize(new_queue_size + padding))
        {
          yae_dlog("%s: estimated packet queue max size: %" PRIu64
                   ", current occupancy: %" PRIu64
                   ", percent padding: %" PRIu64,
                   id_.c_str(),
                   uint64_t(new_queue_size),
                   uint64_t(packetQueue_.getSize()),
                   percent_padding);
        }
      }
    }

    return packetQueue_.push(packetPtr, waitMgr);
  }

  //----------------------------------------------------------------
  // Track::decoderPull
  //
  int
  Track::decoderPull(AVCodecContext * ctx)
  {
    int err = 0;
    while (true)
    {
      AvFrm frm;
      AVFrame & decodedFrame = frm.get();
      {
        // YAE_BENCHMARK(benchmark, "avcodec_receive_frame");
        err = avcodec_receive_frame(ctx, &decodedFrame);
      }

      if (err < 0)
      {
        if (err != AVERROR(EAGAIN) && err != AVERROR_EOF)
        {
          errors_++;
        }

        break;
      }

      // FIXME: perhaps it may be useful to keep track of the number
      // of frames decoded successfully?

      received_++;
#if 0
      yae_wlog("%s decoded: %s",
               id_.c_str(),
               TTime(decodedFrame.pts *
                     stream_->time_base.num,
                     stream_->time_base.den).
               to_hhmmss_us().c_str());
#endif
      decodedFrame.pts = decodedFrame.best_effort_timestamp;

#if 0 // ndef NDEBUG
      for (int i = 0; i < decodedFrame.nb_side_data; i++)
      {
        const AVFrameSideData & sd = *(decodedFrame.side_data[i]);
        yae_dlog("%s frame side data %i: %s, size %i, %s",
                 id_.c_str(),
                 i,
                 av_frame_side_data_name(sd.type),
                 sd.size,
                 yae::to_hex(sd.data, std::min<std::size_t>(sd.size, 64)).
                 c_str());
      }
#endif

      handle(frm);
    }

    return err;
  }

  //----------------------------------------------------------------
  // Track::decode
  //
  int
  Track::decode(AVCodecContext * ctx, const AvPkt & pkt)
  {
    // YAE_BENCHMARK(benchmark, "Track::decode");

    int errSend = AVERROR(EAGAIN);
    int errRecv = AVERROR(EAGAIN);

    while (errSend == AVERROR(EAGAIN))
    {
      boost::this_thread::interruption_point();

      const AVPacket & packet = pkt.get();
      {
        // YAE_BENCHMARK(benchmark, "avcodec_send_packet");
        errSend = avcodec_send_packet(ctx, &packet);
      }

      if (errSend == AVERROR_EOF)
      {
        avcodec_flush_buffers(ctx);
        errSend = avcodec_send_packet(ctx, &packet);
      }

      if (errSend < 0 && errSend != AVERROR(EAGAIN) && errSend != AVERROR_EOF)
      {
#ifndef NDEBUG
        av_log(NULL, AV_LOG_WARNING,
               "[%s] Track::decode(%p), errSend: %i, %s\n",
               id_.c_str(),
               packet.data,
               errSend,
               av_errstr(errSend).c_str());
#endif
        errors_++;
        return errSend;
      }
      else if (errSend >= 0)
      {
        sent_++;
      }

      errRecv = decoderPull(ctx);
      if (errRecv < 0)
      {
#ifndef NDEBUG
        if (errRecv != AVERROR(EAGAIN) && errRecv != AVERROR_EOF)
        {
          av_log(NULL, AV_LOG_WARNING,
                 "[%s] Track::decode(%p), errRecv: %i, %s\n",
                 id_.c_str(),
                 packet.data,
                 errRecv,
                 av_errstr(errRecv).c_str());
        }
#endif
        break;
      }
    }

    return errRecv;
  }

  //----------------------------------------------------------------
  // Track::decode
  //
  void
  Track::decode(const TPacketPtr & packetPtr)
  {
    if (!packetPtr)
    {
      this->flush();
      return;
    }

    AVCodecContext * ctx = this->open();
    if (!ctx)
    {
      // codec is not supported
      return;
    }

    const AvPkt & pkt = *packetPtr;
    decode(ctx, pkt);
  }

  //----------------------------------------------------------------
  // Track::flush
  //
  void
  Track::flush()
  {
    AVCodecContext * ctx = codecContext_.get();
    if (ctx)
    {
      // flush out buffered frames with an empty packet:
      this->decode(ctx, AvPkt());
    }
  }

  //----------------------------------------------------------------
  // Track::thread_loop
  //
  void
  Track::thread_loop()
  {
    decoderStartup();

    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();

        TPacketPtr packetPtr;
        if (!packetQueue_.pop(packetPtr, &terminator_))
        {
          break;
        }

        decode(packetPtr);
      }
      catch (...)
      {
        break;
      }
    }

    decoderShutdown();
  }

  //----------------------------------------------------------------
  // Track::threadStop
  //
  bool
  Track::threadStop()
  {
    terminator_.stopWaiting(true);
    packetQueue_.close();
    thread_.interrupt();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Track::setTempo
  //
  bool
  Track::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);
    tempo_ = tempo;
    return true;
  }


  //----------------------------------------------------------------
  // same_codec
  //
  bool
  same_codec(const TrackPtr & a, const TrackPtr & b)
  {
    if (a == b)
    {
      return true;
    }

    if (!(a && b))
    {
      return false;
    }

    const AVStream & sa = a->stream();
    const AVStream & sb = b->stream();
    bool same = (sa.codecpar->codec_id == sb.codecpar->codec_id);

    return same;
  }
}
