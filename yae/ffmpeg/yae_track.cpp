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

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt()
  {
    memset(this, 0, sizeof(AVPacket));
    av_init_packet(this);
  }

  //----------------------------------------------------------------
  // AvPkt::AvPkt
  //
  AvPkt::AvPkt(const AvPkt & pkt)
  {
    memset(this, 0, sizeof(AVPacket));
    av_packet_ref(this, &pkt);
  }

  //----------------------------------------------------------------
  // AvPkt::~AvPkt
  //
  AvPkt::~AvPkt()
  {
    av_packet_unref(this);
  }

  //----------------------------------------------------------------
  // AvPkt::operator =
  //
  AvPkt &
  AvPkt::operator = (const AvPkt & pkt)
  {
    av_packet_unref(this);
    av_packet_ref(this, &pkt);
    return *this;
  }


  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm()
  {
    memset(this, 0, sizeof(AVFrame));
    av_frame_unref(this);
  }

  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm(const AvFrm & frame)
  {
    memset(this, 0, sizeof(AVFrame));
    av_frame_ref(this, &frame);
  }

  //----------------------------------------------------------------
  // AvFrm::~AvFrm
  //
  AvFrm::~AvFrm()
  {
    av_frame_unref(this);
  }

  //----------------------------------------------------------------
  // AvFrm::operator
  //
  AvFrm &
  AvFrm::operator = (const AvFrm & frame)
  {
    av_frame_unref(this);
    av_frame_ref(this, &frame);
    return *this;
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
  // is_framerate_valid
  //
  static inline bool
  is_framerate_valid(const AVRational & r)
  {
    return r.num > 0 && r.den > 0;
  }

  //----------------------------------------------------------------
  // is_frame_duration_plausible
  //
  static bool
  is_frame_duration_plausible(const TTime & dt, const AVRational & frame_rate)
  {
    int64 d = dt.getTime(frame_rate.num) / frame_rate.den;
    int64 err = d - frame_rate.den;

    if (frame_rate.den < std::abs(d))
    {
      // error is more than the expected frame duration:
      return false;
    }

    return true;
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
    bool ok = (nextPTS.time_ != AV_NOPTS_VALUE &&
               nextPTS.base_ != AV_NOPTS_VALUE &&
               nextPTS.base_ != 0 &&
               (!hasPrevPTS ||
                (prevPTS.base_ == nextPTS.base_ ?
                 prevPTS.time_ < nextPTS.time_ :
                 prevPTS.toSeconds() < nextPTS.toSeconds())));
#if 0
    if (ok && debugMessage)
    {
      std::cerr << "PTS OK: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << ", " << debugMessage << std::endl;
    }
#else
    (void)debugMessage;
#endif

    if (!ok || !stream)
    {
      return ok;
    }

    // sanity check -- verify the timestamp difference
    // is close to frame duration:
    if (hasPrevPTS)
    {
      bool has_avg_frame_rate = is_framerate_valid(stream->avg_frame_rate);
      bool has_r_frame_rate = is_framerate_valid(stream->r_frame_rate);

      if (has_avg_frame_rate || has_r_frame_rate)
      {
        TTime dt = nextPTS - prevPTS;

        if (!((has_avg_frame_rate &&
               is_frame_duration_plausible(dt, stream->avg_frame_rate)) ||
              (has_r_frame_rate &&
               is_frame_duration_plausible(dt, stream->r_frame_rate))))
        {
          // error is more than the expected and estimated frame duration:
#ifndef NDEBUG
          std::cerr
            << "\nNOTE: detected large error in frame duration, dt: "
            << dt.toSeconds() << " sec";

          if (has_avg_frame_rate)
          {
            std::cerr
              << ", expected (avg): "
              << TTime(stream->avg_frame_rate.den,
                       stream->avg_frame_rate.num).toSeconds()
              << " sec";
          }

          if (has_r_frame_rate)
          {
            std::cerr
              << ", expected (r): "
              << TTime(stream->r_frame_rate.den,
                       stream->r_frame_rate.num).toSeconds()
              << " sec";
          }

          if (debugMessage)
          {
            std::cerr << ", " << debugMessage;
          }

          std::cerr << std::endl;
#endif
          return false;
        }
      }
    }

    // another possible sanity check -- verify that timestamp is
    // within [start, end] range, although that might be too strict
    // because duration is often estimated from bitrate
    // and is therefore inaccurate

    return true;
  }



  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(AVFormatContext * context, AVStream * stream):
    thread_(this),
    context_(context),
    stream_(stream),
    preferSoftwareDecoder_(false),
    switchDecoderToRecommended_(false),
    sent_(0),
    received_(0),
    errors_(0),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
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
  Track::Track(Track & track):
    thread_(this),
    context_(NULL),
    stream_(NULL),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackEnabled_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    std::swap(context_, track.context_);
    std::swap(stream_, track.stream_);
    std::swap(codecContext_, track.codecContext_);
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
  // TDecoderMap
  //
  typedef std::map<AVCodecID, std::set<const AVCodec *> > TDecoderMap;

  //----------------------------------------------------------------
  // TDecoders
  //
  struct TDecoders : public TDecoderMap
  {

    //----------------------------------------------------------------
    // TDecoders
    //
    TDecoders()
    {
      for (const AVCodec * c = av_codec_next(NULL); c; c = av_codec_next(c))
      {
        if (av_codec_is_decoder(c))
        {
          TDecoderMap::operator[](c->id).insert(c);
        }
      }
    }

    //----------------------------------------------------------------
    // find
    //
    // FIXME: perhaps should also include a list of acceptable
    // output pixel formats (or sample formats)
    //
    void
    find(const AVCodecParameters & params,
         std::list<AvCodecContextPtr> & decoders,
         bool preferSoftwareDecoder) const
    {
      decoders.clear();

      TDecoderMap::const_iterator found = TDecoderMap::find(params.codec_id);
      if (found == TDecoderMap::end())
      {
        return;
      }

      std::list<AvCodecContextPtr> hardware;
      std::list<AvCodecContextPtr> software;
      std::list<AvCodecContextPtr> experimental;

      typedef std::set<const AVCodec *> TCodecs;
      const TCodecs & codecs = found->second;
      for (TCodecs::const_iterator i = codecs.begin(); i != codecs.end(); ++i)
      {
        const AVCodec * c = *i;
        if (c->capabilities & AV_CODEC_CAP_EXPERIMENTAL)
        {
          AvCodecContextPtr ctx = tryToOpen(c, &params);
          if (ctx)
          {
            experimental.push_back(ctx);
          }
        }
        else if (al::ends_with(c->name, "_vda"))
        {
          if (params.format != AV_PIX_FMT_YUV420P &&
              params.codec_id != AV_CODEC_ID_MJPEG)
          {
            // 4:2:0 is the only one that works with h264_cuvud and mpeg2_cuvid
            // however, 4:2:2 and 4:4:4 work with mjpeg_cuvid
            continue;
          }

          // verify that the GPU can handle this stream:
          AvCodecContextPtr ctx = tryToOpen(c, &params);
          if (ctx)
          {
            hardware.push_front(ctx);
          }
        }
        else if (al::ends_with(c->name, "_qsv") ||
                 al::ends_with(c->name, "_vda"))
        {
          // verify that the GPU can handle this stream:
          AvCodecContextPtr ctx = tryToOpen(c, &params);
          if (ctx)
          {
            hardware.push_back(ctx);
          }
        }
        else
        {
          AvCodecContextPtr ctx = tryToOpen(c, &params);
          if (ctx)
          {
            software.push_back(ctx);
          }
        }
      }

      if (preferSoftwareDecoder)
      {
        decoders.splice(decoders.end(), software);
        decoders.splice(decoders.end(), hardware);
      }
      else
      {
        decoders.splice(decoders.end(), hardware);
        decoders.splice(decoders.end(), software);
      }

      decoders.splice(decoders.end(), experimental);
    }
  };

  //----------------------------------------------------------------
  // get_decoders
  //
  static const TDecoders & get_decoders()
  {
    static const TDecoders decoders;
    return decoders;
  }

  //----------------------------------------------------------------
  // find_best_decoder_for
  //
  AvCodecContextPtr
  find_best_decoder_for(const AVCodecParameters & params,
                        std::list<AvCodecContextPtr> & untried,
                        bool preferSoftwareDecoder)
  {
    const TDecoders & decoders = get_decoders();

    if (untried.empty())
    {
      decoders.find(params, untried, preferSoftwareDecoder);
    }

    AvCodecContextPtr ctx = untried.front();
    untried.pop_front();

#ifndef NDEBUG
    if (ctx)
    {
      std::cerr << "\n\nUSING DECODER: " << ctx->codec->name << "\n\n"
                << std::endl;
    }
#endif

    return ctx;
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

    const AVCodecParameters & codecParams = *(stream_->codecpar);

    codecContext_ = find_best_decoder_for(codecParams,
                                          candidates_,
                                          preferSoftwareDecoder_);
    if (!codecContext_ && stream_->codecpar->codec_id != AV_CODEC_ID_TEXT)
    {
      // unsupported codec:
      return NULL;
    }

    sent_ = 0;
    received_ = 0;
    errors_ = 0;
    return codecContext_.get();
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    if (stream_ && codecContext_)
    {
      codecContext_.reset();
    }
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

    if (stream_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // return track duration:
      start.base_ = stream_->time_base.den;
      start.time_ =
        stream_->start_time != int64_t(AV_NOPTS_VALUE) ?
        stream_->time_base.num * stream_->start_time : 0;

      duration.time_ = stream_->time_base.num * stream_->duration;
      duration.base_ = stream_->time_base.den;
      return true;
    }

    if (!context_)
    {
      YAE_ASSERT(false);
      return false;
    }

    if (context_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      start.base_ = AV_TIME_BASE;
      start.time_ =
        context_->start_time != int64_t(AV_NOPTS_VALUE) ?
        context_->start_time : 0;

      duration.time_ = context_->duration;
      duration.base_ = AV_TIME_BASE;
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

    const AVCodecParameters & codecParams = *(stream_->codecpar);
    if (context_->nb_streams == 1 && codecParams.bit_rate)
    {
      double t =
        double(fileBits / codecParams.bit_rate) +
        double(fileBits % codecParams.bit_rate) /
        double(codecParams.bit_rate);

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

#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(54, 3, 0)
  //----------------------------------------------------------------
  // av_frame_get_best_effort_timestamp
  //
  inline int64_t
  av_frame_get_best_effort_timestamp(const AVFrame * frame)
  {
    return frame->pkt_pts;
  }
#endif

  //----------------------------------------------------------------
  // Track::decoderPull
  //
  int
  Track::decoderPull(AVCodecContext * ctx)
  {
    int err = 0;
    while (true)
    {
      AvFrm decodedFrame;
      err = avcodec_receive_frame(ctx, &decodedFrame);
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
      decodedFrame.pts = av_frame_get_best_effort_timestamp(&decodedFrame);
      handle(decodedFrame);
    }

    return err;
  }

  //----------------------------------------------------------------
  // Track::decode
  //
  int
  Track::decode(AVCodecContext * ctx, const AvPkt & pkt)
  {
    int errSend = AVERROR(EAGAIN);
    int errRecv = AVERROR(EAGAIN);

    while (errSend == AVERROR(EAGAIN))
    {
      boost::this_thread::interruption_point();

      errSend = avcodec_send_packet(ctx, &pkt);
      if (errSend < 0 && errSend != AVERROR(EAGAIN) && errSend != AVERROR_EOF)
      {
#ifndef NDEBUG
        dump_averror(std::cerr, errSend);
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
          dump_averror(std::cerr, errRecv);
        }
#endif
        break;
      }
    }

    return errRecv;
  }

  //----------------------------------------------------------------
  // Track::switchDecoder
  //
  bool
  Track::switchDecoder()
  {
    // while (!candidates_.empty())
    {
      codecContext_.reset();
      AVCodecContext * ctx = open();

      int err = AVERROR(EAGAIN);
      for (std::list<TPacketPtr>::const_iterator
             i = packets_.begin(), end = packets_.end(); i != end; ++i)
      {
        const AvPkt & pkt = *(*i);
        int err = decode(ctx, pkt);
        if (err < 0 && err != AVERROR(EAGAIN))
        {
          break;
        }
      }

      if (err == 0)
      {
        return true;
      }

      if (err == AVERROR(EAGAIN))
      {
        return false;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Track::tryToSwitchDecoder
  //
  void
  Track::tryToSwitchDecoder(const std::string & name)
  {
    const TDecoders & decoders = get_decoders();
    const AVCodecParameters & params = *(stream_->codecpar);

    std::list<AvCodecContextPtr> candidates;
    decoders.find(params, candidates, preferSoftwareDecoder_);

    std::list<AvCodecContextPtr> a;
    std::list<AvCodecContextPtr> b;

    for (std::list<AvCodecContextPtr>::const_iterator i = candidates.begin();
         i != candidates.end(); ++i)
    {
      const AvCodecContextPtr & c = *i;
      const AVCodecContext * ctx = c.get();

      if (name == ctx->codec->name)
      {
        a.push_back(c);
      }
      else
      {
        b.push_back(c);
      }
    }

    a.splice(a.end(), b);

    recommended_ = a;
    switchDecoderToRecommended_ = true;
  }

  //----------------------------------------------------------------
  // VideoTrack::threadLoop
  //
  void
  Track::threadLoop()
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

        if (!packetPtr)
        {
          if (codecContext_)
          {
            // flush out buffered frames with an empty packet:
            AVCodecContext * ctx = codecContext_.get();
            AvPkt pkt;
            decode(ctx, pkt);
          }
        }
        else
        {
          const AvPkt & pkt = *packetPtr;

          if (switchDecoderToRecommended_ && pkt.flags & AV_PKT_FLAG_KEY)
          {
            candidates_.clear();
            candidates_.splice(candidates_.end(), recommended_);
            switchDecoderToRecommended_ = false;

            // flush any buffered frames:
            if (codecContext_)
            {
              AvPkt flushPkt;
              decode(codecContext_.get(), flushPkt);
            }

            // close the codec:
            codecContext_.reset();
          }

          AVCodecContext * ctx = open();
          if (!ctx)
          {
            // codec is not supported
            break;
          }

          if (sent_ > 60)
          {
            packets_.pop_front();
          }
          packets_.push_back(packetPtr);

          int receivedPrior = received_;
          int err = decode(ctx, pkt);

          if (received_ > receivedPrior)
          {
            packets_.clear();
            sent_ = 0;
            errors_ = 0;
          }
          else if (err < 0 && err != AVERROR(EAGAIN) &&
                   (!received_ || errors_ >= 6))
          {
            switchDecoder();
          }
        }
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
    thread_.stop();
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
}
