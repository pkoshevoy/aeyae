// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/utils/yae_benchmark.h"

// standard:
#include <map>
#include <set>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

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
  // TimePos::get
  //
  double
  TimePos::get() const
  {
    return sec_;
  }

  //----------------------------------------------------------------
  // TimePos::set
  //
  void
  TimePos::set(double pos)
  {
    sec_ = pos;
  }

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
  // TimePos::seek
  //
  int
  TimePos::seek(AVFormatContext * context, const AVStream * stream) const
  {
    int64_t ts = int64_t(sec_ * double(AV_TIME_BASE));
    bool seekToStart = (ts <= context->start_time);
    int streamIndex = stream ? stream->index : -1;

    if (stream)
    {
      // ts expressed in stream timebase:
      ts =
        seekToStart ? stream->start_time :
        int64_t((stream->time_base.num * sec_) /
                (stream->time_base.den));
    }
    else if (seekToStart)
    {
      streamIndex = av_find_default_stream_index(context);
      if (streamIndex >= 0 && streamIndex < context->nb_streams)
      {
        // ts expressed in stream timebase:
        stream = context->streams[streamIndex];
        ts = stream->start_time;
      }
      else
      {
        // ts expressed in AV_TIME_BASE timebase:
        ts = context->start_time;
        streamIndex = -1;
      }
    }

    int seekFlags = 0;
    int err = avformat_seek_file(context,
                                 streamIndex,
                                 kMinInt64,
                                 ts,
                                 ts, // kMaxInt64,
                                 seekFlags);

    if (err < 0)
    {
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
  // PacketPos::PacketPos
  //
  PacketPos::PacketPos(int64_t packet_pos, uint64_t packet_size):
    pos_(packet_pos, packet_size)
  {}

  //----------------------------------------------------------------
  // PacketPos::get
  //
  double
  PacketPos::get() const
  {
    return pos_.sec();
  }

  //----------------------------------------------------------------
  // PacketPos::set
  //
  void
  PacketPos::set(double sec)
  {
    pos_.time_ = int64_t(pos_.base_ * sec);
  }

  //----------------------------------------------------------------
  // PacketPos::to_str
  //
  std::string
  PacketPos::to_str() const
  {
    return pos_.sec_msec();
  }

  //----------------------------------------------------------------
  // PacketPos::lt
  //
  bool
  PacketPos::lt(const TFrameBase & f, double dur) const
  {
    return (pos_ < f.pos_);
  }

  //----------------------------------------------------------------
  // PacketPos::gt
  //
  bool
  PacketPos::gt(const TFrameBase & f, double dur) const
  {
    return (f.pos_ < pos_);
  }

  //----------------------------------------------------------------
  // PacketPos::seek
  //
  int
  PacketPos::seek(AVFormatContext * context, const AVStream * stream) const
  {
    (void)stream;

    int seekFlags = (AVSEEK_FLAG_BYTE | AVSEEK_FLAG_ANY);
    int err = avformat_seek_file(context,
                                 -1,
                                 pos_.time_ - pos_.base_,
                                 pos_.time_,
                                 pos_.time_ + pos_.base_,
                                 seekFlags);
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
    if (!ctx)
    {
      return;
    }

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
  Track::Track(AVFormatContext * context, AVStream * stream, bool hwdec):
    packetRateEstimator_(1536),
    thread_(this),
    hwdec_(hwdec),
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
    packet_pos_(4), // fifo capacity
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
    hwdec_(false),
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
    packet_pos_(4), // fifo capacity
    discarded_(0)
  {
    std::swap(hwdec_, track->hwdec_);
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
  // maybe_set_avopt
  //
  // avoid setting unsupported options:
  //
  bool
  maybe_set_avopt(AVDictionary *& opts,
                  AVCodecContext * ctx,
                  const char * name,
                  const char * value)
  {
    if (!(name && *name && value && *value))
    {
      return false;
    }

    // avoid setting unsupported options:
    int search_flags = 0;
    void * target_obj = NULL;
    const AVOption * o = av_opt_find2(ctx->priv_data,
                                      name,
                                      NULL,
                                      0,
                                      search_flags,
                                      &target_obj);
    if (o && target_obj)
    {
      av_dict_set(&opts, name, value, 0);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // Track::open
  //
  AvCodecContextPtr
  Track::open()
  {
    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;

    if (ctx_ptr)
    {
      return ctx_ptr;
    }

    if (!stream_)
    {
      return ctx_ptr;
    }

    // keep-alive:
    AvCodecParameters params(stream_->codecpar);
    const AVCodecParameters & codecpar = params.get();
    const AVCodec * codec = avcodec_find_decoder(codecpar.codec_id);
    return maybe_open(codec, codecpar, NULL);
  }

  //----------------------------------------------------------------
  // Track::maybe_open
  //
  AvCodecContextPtr
  Track::maybe_open(const AVCodec * codec,
                    const AVCodecParameters & params,
                    AVDictionary * opts)
  {
    if (!codec && stream_->codecpar->codec_id != AV_CODEC_ID_TEXT)
    {
      // unsupported codec:
      return AvCodecContextPtr();
    }

    int err = 0;
    yae::AvBufferRef hw_device_ctx;

    int hw_config_index = 0;
    while (hwdec_ && params.width > 640 && params.height > 360)
    {
      const AVCodecHWConfig * hw =
        avcodec_get_hw_config(codec, hw_config_index);

      hw_config_index++;

      if (!hw)
      {
        break;
      }

      if (hw->device_type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX &&
          codec->id == AV_CODEC_ID_H264)
      {
        // vt has problems decoding and seeking 20220407-up30635-capture.ts
        continue;
      }

      if ((AV_CODEC_HW_CONFIG_METHOD_AD_HOC & hw->methods) ==
          (AV_CODEC_HW_CONFIG_METHOD_AD_HOC))
      {
        // methods requiring this sort of configuration are deprecated
        // and others should be used instead:
        continue;
      }

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

    AvCodecContextPtr ctx_ptr(avcodec_alloc_context3(codec));
    AVCodecContext * ctx = ctx_ptr.get();
    avcodec_parameters_to_context(ctx, &params);
    ctx->opaque = this;

    const enum AVPixelFormat * codec_pix_fmts = NULL;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(61, 13, 100)
    codec_pix_fmts = ctx->codec->pix_fmts;
#else
    avcodec_get_supported_config(ctx,
                                 NULL, // use ctx->codec
                                 AV_CODEC_CONFIG_PIX_FORMAT,
                                 0, // flags
                                 (const void **)&codec_pix_fmts,
                                 NULL);
#endif

    if (codec_pix_fmts && !yae::has<AVPixelFormat>(codec_pix_fmts,
                                                   ctx->pix_fmt,
                                                   AV_PIX_FMT_NONE))
    {
      AVPixelFormat found =
        yae::find_nearest_pix_fmt(ctx->pix_fmt, codec_pix_fmts);

      if (found == AV_PIX_FMT_NONE)
      {
        yae_elog("%s doesn't support pixel format %s",
                 codec->name,
                 av_get_pix_fmt_name(ctx->pix_fmt));
        found = codec_pix_fmts[0];
      }

      if (found != AV_PIX_FMT_NONE)
      {
        yae_ilog("adjusting %s pixel format from %s to %s",
                 codec->name,
                 av_get_pix_fmt_name(ctx->pix_fmt),
                 av_get_pix_fmt_name(found));
        ctx->pix_fmt = found;
      }
    }

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
#ifndef __APPLE_
      ctx->hw_device_ctx ?
      std::min(16, nthreads) :
#endif
      std::max(1, nthreads);

    av_dict_set_int(&opts, "threads", nthreads, 0);

    ctx->thread_count = nthreads;
    // ctx->thread_type = FF_THREAD_SLICE;
    // ctx->thread_type = FF_THREAD_FRAME;

    err = avcodec_open2(ctx, codec, &opts);
    if (err < 0)
    {
      yae_elog("avcodec_open2 failed: %s", yae::av_errstr(err).c_str());
      return AvCodecContextPtr();
    }

    std::swap(hw_device_ctx_, hw_device_ctx);
    codecContext_ = ctx_ptr;
    sent_ = 0;
    received_ = 0;
    errors_ = 0;
    return ctx_ptr;
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    hw_frames_ctx_.reset();
    hw_device_ctx_.reset();
    codecContext_.reset();
  }

  //----------------------------------------------------------------
  // Track::maybe_reopen
  //
  void
  Track::maybe_reopen(bool hwdec)
  {
    if (hwdec_ == hwdec)
    {
      return;
    }

    hwdec_ = hwdec;
    close();
    open();
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
    this->packetQueueOpen();
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
    if (!(codecpar_next_ && codecpar_next_->same_codec(stream_->codecpar)))
    {
      codecpar_next_.reset(new yae::AvCodecParameters(stream_->codecpar));
    }

    if (packetPtr)
    {
      packetPtr->codecpar_ = codecpar_next_;
      packetPtr->timebase_ = stream_->time_base;

      const AvPkt & pkt = *packetPtr;
      const AVPacket & packet = pkt.get();
      this->update_packet_queue_size(packet);
    }

    return this->packet_queue_push(packetPtr, waitMgr);
  }

  //----------------------------------------------------------------
  // Track::update_packet_queue_size
  //
  void
  Track::update_packet_queue_size(const AVPacket & packet)
  {
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

    // must not set packet queue size to 0
    // or we'll hoard an unlimited number of decoded frames
    // in memory when playback is paused:
    {
      // avoid log span due to flip-flopping queue size:
      static const uint64_t padding = 10;

      uint64_t new_queue_size = std::max<uint64_t>(24, rate + 0.5);
      uint64_t max_queue_size = packetQueue_.getMaxSize();
      uint64_t cur_queue_size = packetQueue_.getSize();

      const uint64_t diff =
        (new_queue_size < max_queue_size) ?
        max_queue_size - new_queue_size :
        new_queue_size - max_queue_size;

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
                 new_queue_size,
                 cur_queue_size,
                 percent_padding);
      }
    }
  }

  //----------------------------------------------------------------
  // Track::packet_queue_push
  //
  bool
  Track::packet_queue_push(const TPacketPtr & packetPtr,
                           QueueWaitMgr * waitMgr)
  {
    return packetQueue_.push(packetPtr, waitMgr);
  }

  //----------------------------------------------------------------
  // Track::packet_queue_pop
  //
  bool
  Track::packet_queue_pop(TPacketPtr & packetPtr,
                          QueueWaitMgr * waitMgr)
  {
    return packetQueue_.pop(packetPtr, waitMgr);
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
        if (packet.pos != -1)
        {
          packet_pos_.push(packet.pos);
        }

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
    TPacketPtr prev = prev_packet_;
    prev_packet_ = packetPtr;

    if (!packetPtr)
    {
      this->flush();
      return;
    }

    // check for timeline anomalies:
    bool timeline_anomaly = false;
    if (prev)
    {
      const AVPacket & a = prev->get();
      const AVPacket & b = packetPtr->get();

      const AVRational & a_tb = prev->timebase_;
      const AVRational & b_tb = packetPtr->timebase_;

      if (a.dts != AV_NOPTS_VALUE &&
          b.dts != AV_NOPTS_VALUE)
      {
        static const Rational msec(1, 1000);
        int64_t a_dts = av_rescale_q(a.dts, a_tb, b_tb);
        int64_t b_dts = b.dts;
        int64_t dt_msec = av_rescale_q(b.dts - a_dts, b_tb, msec);
        timeline_anomaly = (dt_msec < 0 || dt_msec > 5000);
      }
    }

    // handle codec changes:
    bool codec_changed =
      (codecpar_curr_ && !codecpar_curr_->same_codec(*(packetPtr->codecpar_)));
    if (codec_changed)
    {
      this->flush();
      codecContext_.reset();
    }

    // save for future reference:
    codecpar_curr_ = packetPtr->codecpar_;

    AvCodecContextPtr ctx = this->open();
    if (!ctx)
    {
      // codec is not supported
      return;
    }

    if (codec_changed || timeline_anomaly)
    {
      bool dropPendingFrames = false;
      this->resetTimeCounters(TSeekPosPtr(), dropPendingFrames);
    }

    const AvPkt & pkt = *packetPtr;
    decode(ctx.get(), pkt);
  }

  //----------------------------------------------------------------
  // Track::flush
  //
  void
  Track::flush()
  {
    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;
    AVCodecContext * ctx = ctx_ptr.get();

    if (ctx)
    {
      // flush out buffered frames with an empty packet:
      this->decode(ctx, AvPkt());
    }

    packet_pos_.clear();
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
        if (!this->packet_queue_pop(packetPtr, &terminator_))
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
    this->packetQueueClose();
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
