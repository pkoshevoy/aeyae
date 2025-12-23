// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_closed_captions.h"
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_pixel_format_ffmpeg.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_linear_algebra.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_texture_generator.h"

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// ffmpeg:
extern "C"
{
#include <libavutil/display.h>
#include <libavutil/mastering_display_metadata.h>
}

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // TAVFrameBuffer::TAVFrameBuffer
  //
  TAVFrameBuffer::TAVFrameBuffer(AVFrame * src)
  {
    // this is a shallow reference counted copy:
    frame_ = AvFrm(src);
  }

  //----------------------------------------------------------------
  // TAVFrameBuffer::destroy
  //
  void
  TAVFrameBuffer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // TAVFrameBuffer::planes
  //
  std::size_t
  TAVFrameBuffer::planes() const
  {
    const AVFrame & frame = frame_.get();
    enum AVPixelFormat pix_fmt = (enum AVPixelFormat)frame.format;
    int n = av_pix_fmt_count_planes(pix_fmt);
    YAE_ASSERT(n >= 0);
    return (std::size_t)n;
  }

  //----------------------------------------------------------------
  // TAVFrameBuffer::data
  //
  unsigned char *
  TAVFrameBuffer::data(std::size_t plane) const
  {
    const AVFrame & frame = frame_.get();
    return frame.data[plane];
  }

  //----------------------------------------------------------------
  // TAVFrameBuffer::rowBytes
  //
  std::size_t
  TAVFrameBuffer::rowBytes(std::size_t plane) const
  {
    const AVFrame & frame = frame_.get();
    return frame.linesize[plane];
  }


  //----------------------------------------------------------------
  // aFollowsB
  //
  static bool
  aFollowsB(const TVideoFramePtr & a,
            const TVideoFramePtr & b)
  {
    TTime framePosition;
    if (a->time_.base_ == b->time_.base_)
    {
      return a->time_.time_ > b->time_.time_;
    }

    double ta = double(a->time_.time_) / double(a->time_.base_);
    double tb = double(b->time_.time_) / double(b->time_.base_);
    return ta > tb;
  }

  //----------------------------------------------------------------
  // VideoTrack::VideoTrack
  //
  VideoTrack::VideoTrack(Track * track):
    Track(track),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    deinterlace_(false),
    hasPrevPTS_(false),
    framesDecoded_(0),
    framesProduced_(0),
    subs_(NULL)
  {
    YAE_ASSERT(stream_->codecpar->codec_type == AVMEDIA_TYPE_VIDEO);

    // make sure the frames are sorted from oldest to newest:
    frameQueue_.setSortFunc(&aFollowsB);

    frameRate_.num = 1;
    frameRate_.den = AV_TIME_BASE;

    // if audio packets are late ... demuxer can become stuck
    // trying to add a video packet to an already full queue,
    // and never adding any packets to the audio packet queue
    // and therefore not advancing the audio playhead position
    // thus creating a deadlock...
    //
    // add some extra padding to the queue size to help avoid this
    //
    packetQueue_.setPadding(78);
  }

  //----------------------------------------------------------------
  // VideoTrack::~VideoTrack
  //
  VideoTrack::~VideoTrack()
  {
    frameQueue_.close();
  }

  //----------------------------------------------------------------
  // VideoTrack::initTraits
  //
  bool
  VideoTrack::initTraits()
  {
    if (!getTraits(native_))
    {
      return false;
    }

    output_ = native_;
    override_ = VideoTraits();

    // do not unintentionally override width, height, sar, etc:
    override_.dynamic_range_.set_zero();
    override_.visibleWidth_ = 0;
    override_.visibleHeight_ = 0;
    override_.pixelAspectRatio_ = 0.0;
    overrideSourcePAR_ = 0.0;

    return true;
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
    TDecoders()
    {
      void * opaque = NULL;
      for (const AVCodec * c = av_codec_iterate(&opaque); c;
           c = av_codec_iterate(&opaque))
      {
        if (av_codec_is_decoder(c))
        {
          TDecoderMap::operator[](c->id).insert(c);
        }
      }
    }

    bool find(std::list<const AVCodec *> & hardware,
              std::list<const AVCodec *> & software,
              std::list<const AVCodec *> & experimental,
              const AVCodecParameters & params,
              bool allow_hwdec) const
    {
      TDecoderMap::const_iterator found = TDecoderMap::find(params.codec_id);
      if (found == TDecoderMap::end())
      {
        return false;
      }

      std::list<const AVCodec *> no_hwconfig;
      typedef std::set<const AVCodec *> TCodecs;
      const TCodecs & codecs = found->second;

      for (TCodecs::const_iterator i = codecs.begin(); i != codecs.end(); ++i)
      {
        const AVCodec * c = *i;

        if (al::ends_with(c->name, "_v4l2m2m"))
        {
          // ignore it, it always fails anyway:
          continue;
        }

        if (al::ends_with(c->name, "_cuvid"))
        {
          // ignore it, use nvdec instead:
          continue;
        }

        if ((c->capabilities & AV_CODEC_CAP_EXPERIMENTAL) ==
            AV_CODEC_CAP_EXPERIMENTAL)
        {
          experimental.push_back(c);
          continue;
        }

        const AVCodecHWConfig * hw =
          allow_hwdec ? avcodec_get_hw_config(c, 0) : NULL;

        if (hw)
        {
          hardware.push_back(c);
        }
        else if ((c->capabilities & AV_CODEC_CAP_HARDWARE) ==
                 AV_CODEC_CAP_HARDWARE &&
                 allow_hwdec)
        {
          no_hwconfig.push_back(c);
        }
        else
        {
          software.push_back(c);
        }
      }

      experimental.splice(experimental.end(), no_hwconfig);
      return !(hardware.empty() && software.empty() && experimental.empty());
    }
  };

  //----------------------------------------------------------------
  // find_decoders_for
  //
  bool
  find_decoders_for(std::list<const AVCodec *> & candidates,
                    const AVCodecParameters & params,
                    bool allow_hwdec)
  {
    static const TDecoders decoders;

    std::list<const AVCodec *> hardware;
    std::list<const AVCodec *> software;
    std::list<const AVCodec *> experimental;
    if (!decoders.find(hardware, software, experimental, params, allow_hwdec))
    {
      return false;
    }

    candidates.clear();
    candidates.splice(candidates.end(), hardware);
    candidates.splice(candidates.end(), software);
    candidates.splice(candidates.end(), experimental);
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::open
  //
  AvCodecContextPtr
  VideoTrack::open()
  {
    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;

    if (ctx_ptr)
    {
      return ctx_ptr;
    }

    std::list<const AVCodec *> candidates;
    const AVCodecParameters & params = *(stream_->codecpar);
    if (!yae::find_decoders_for(candidates, params, true))
    {
      return ctx_ptr;
    }

    for (std::list<const AVCodec *>::const_iterator i = candidates.begin();
         i != candidates.end(); ++i)
    {
      const AVCodec * codec = *i;
      ctx_ptr = this->maybe_open(codec, params, NULL);
      if (!ctx_ptr)
      {
        continue;
      }

      framesDecoded_ = 0;
      framesProduced_ = 0;
      skipLoopFilter(skipLoopFilter_);
      skipNonReferenceFrames(skipNonReferenceFrames_);
      return ctx_ptr;
    }

    return ctx_ptr;
  }

  //----------------------------------------------------------------
  // VideoTrack::skipLoopFilter
  //
  void
  VideoTrack::skipLoopFilter(bool skip)
  {
    skipLoopFilter_ = skip;

    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;
    AVCodecContext * ctx = ctx_ptr.get();

    if (ctx)
    {
      if (skipLoopFilter_)
      {
        ctx->skip_loop_filter = AVDISCARD_ALL;
        ctx->flags2 |= AV_CODEC_FLAG2_FAST;
      }
      else
      {
        ctx->skip_loop_filter = AVDISCARD_DEFAULT;
        ctx->flags2 &= ~(AV_CODEC_FLAG2_FAST);
      }
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::skipNonReferenceFrames
  //
  void
  VideoTrack::skipNonReferenceFrames(bool skip)
  {
    skipNonReferenceFrames_ = skip;

    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;
    AVCodecContext * ctx = ctx_ptr.get();

    if (ctx)
    {
      if (skipNonReferenceFrames_)
      {
        ctx->skip_frame = AVDISCARD_NONREF;
      }
      else
      {
        ctx->skip_frame = AVDISCARD_DEFAULT;
      }
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::refreshTraits
  //
  void
  VideoTrack::refreshTraits(const AVFrame * decoded)
  {
    // shortcut to native frame format traits:
    getTraits(native_, decoded);

    // frame size may have changed, so update output traits accordingly:
    output_.set_overrides(override_);

    if (native_.pixelFormat_ != kInvalidPixelFormat &&
        override_.pixelFormat_ == kInvalidPixelFormat)
    {
      // Sony_Whale_Tracks.ts doesn't probe the pixel format during probing:
      output_.pixelFormat_ = native_.pixelFormat_;
      output_.av_fmt_ = native_.av_fmt_;
      output_.av_rng_ = native_.av_rng_;
      output_.av_pri_ = native_.av_pri_;
      output_.av_trc_ = native_.av_trc_;
      output_.av_csp_ = native_.av_csp_;
      output_.colorspace_ = native_.colorspace_;
      output_.dynamic_range_ = native_.dynamic_range_;
    }

    double sourcePixelAspectRatio =
      overrideSourcePAR_ ? overrideSourcePAR_ :  native_.pixelAspectRatio_;

    double overridePixelAspectRatio =
      override_.pixelAspectRatio_; // overrideSourcePAR_ ? 1.0 : 0.0;

    output_.pixelAspectRatio_ = sourcePixelAspectRatio;

    int transposeAngle =
      ((native_.cameraRotation_ - override_.cameraRotation_) - 180) % 180;

    if (override_.visibleWidth_ ||
        override_.visibleHeight_ ||
        transposeAngle != 0)
    {
      // NOTE: the override provides a scale-to-fit frame envelope,
      // not the actual frame size:

      int src_w = native_.visibleWidth_ * sourcePixelAspectRatio + 0.5;
      int src_h = native_.visibleHeight_;

      int envelope_w =
        override_.visibleWidth_ ?
        override_.visibleWidth_ :
        transposeAngle ? src_h : src_w;

      int envelope_h =
        override_.visibleHeight_ ?
        override_.visibleHeight_ :
        transposeAngle ? src_w : src_h;

      const double envelope_dar =
        double(envelope_w) / double(envelope_h);

      const double native_dar =
        transposeAngle ?
        double(src_h) / double(src_w) :
        double(src_w) / double(src_h);

      if (native_dar <= envelope_dar)
      {
        output_.visibleWidth_ = envelope_h * native_dar;
        output_.visibleHeight_ = envelope_h;
      }
      else
      {
        output_.visibleWidth_ = envelope_w;
        output_.visibleHeight_ = envelope_w / native_dar;
      }

      output_.offsetLeft_ = 0;
      output_.offsetTop_ = 0;
      output_.encodedWidth_ = output_.visibleWidth_;
      output_.encodedHeight_ = output_.visibleHeight_;
      output_.pixelAspectRatio_ = 1.0;
    }
    else
    {
      output_.encodedWidth_ = native_.encodedWidth_;
      output_.encodedHeight_ = native_.encodedHeight_;
      output_.offsetLeft_ = native_.offsetLeft_;
      output_.offsetTop_ = native_.offsetTop_;
      output_.visibleWidth_ = native_.visibleWidth_;
      output_.visibleHeight_ = native_.visibleHeight_;
    }

    if (overridePixelAspectRatio > 0.0 &&
        overridePixelAspectRatio != output_.pixelAspectRatio_)
    {
      if (overridePixelAspectRatio > 1.0)
      {
        output_.visibleWidth_ = int(output_.visibleWidth_ *
                                    output_.pixelAspectRatio_ /
                                    overridePixelAspectRatio + 0.5);
        output_.encodedWidth_ = output_.visibleWidth_;
      }
      else
      {
        output_.visibleHeight_ = int(output_.visibleHeight_ *
                                     overridePixelAspectRatio + 0.5);
        output_.encodedHeight_ = output_.visibleHeight_;

      }
      output_.pixelAspectRatio_ = overridePixelAspectRatio;
    }

    // scale filter will fail if you ask it to rescale
    // yuv420 422x240 into rgb24 256x145 because output
    // dimensions must be divisible by the
    // input yuv420 chroma sub-sampling factors
    int subsample_hor_log2 = 0;
    int subsample_ver_log2 = 0;
    YAE_ASSERT(av_pix_fmt_get_chroma_sub_sample(native_.av_fmt_,
                                                &subsample_hor_log2,
                                                &subsample_ver_log2) == 0);
    int subsample_hor = 1 << subsample_hor_log2;
    int subsample_ver = 1 << subsample_ver_log2;

    output_.visibleWidth_ -= (output_.visibleWidth_ % subsample_hor);
    output_.visibleHeight_ -= (output_.visibleHeight_ % subsample_ver);

    if (output_.pixelFormat_ == kPixelFormatY400A &&
        native_.pixelFormat_ != kPixelFormatY400A)
    {
      // sws_getContext doesn't support Y400A, so drop the alpha channel:
      output_.setPixelFormat(kPixelFormatGRAY8);
    }
  }

  //----------------------------------------------------------------
  // confirm_supported_output_format
  //
  static bool
  confirm_supported_output_format(const AvFrmSpecs & specs)
  {
    // pixel format shortcut:
    TPixelFormatId out_fmt = ffmpeg_to_yae(yae::pix_fmt(specs));
    const pixelFormat::Traits * ptts = pixelFormat::getTraits(out_fmt);

    if (!ptts)
    {
      YAE_ASSERT(false);
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderStartup
  //
  bool
  VideoTrack::decoderStartup()
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    yae_debug << "\n\t\t\t\tVIDEO TRACK DECODER STARTUP\n";
#endif

    refreshTraits();

    framesDecoded_ = 0;
    framesProduced_ = 0;
#ifndef NDEBUG
    this->t0_ = boost::chrono::steady_clock::now();
#endif

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    // shortcut to the frame rate:
    frameRate_ =
      (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den) ?
      stream_->avg_frame_rate : stream_->r_frame_rate;

    hasPrevPTS_ = false;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderShutdown
  //
  bool
  VideoTrack::decoderShutdown()
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    yae_debug << "\n\t\t\t\tVIDEO TRACK DECODER SHUTDOWN\n";
#endif

    filterGraph_.reset();
    hasPrevPTS_ = false;
    frameQueue_.close();
    this->packetQueueClose();
    return true;
  }

  //----------------------------------------------------------------
  // TSubsPredicate
  //
  struct TSubsPredicate
  {
    TSubsPredicate(double now):
      now_(now)
    {}

    bool operator() (const TSubsFrame & sf) const
    {
      double s0 = sf.time_.sec();
      return s0 <= now_;
    }

    double now_;
  };


  //----------------------------------------------------------------
  // gatherApplicableSubtitles
  //
  static void
  gatherApplicableSubtitles(std::list<TSubsFrame> & subs,
                            double v0, // frame start in seconds
                            double v1, // frame end in seconds
                            SubtitlesTrack & subTrack,
                            QueueWaitMgr & terminator)
  {
    TSubsPredicate subSelector(v1);
    subTrack.queue_.get(subSelector, subTrack.active_, &terminator);

    TSubsFrame next;
    subTrack.queue_.peek(next, &terminator);
    subTrack.fixupEndTimes(v1, next);
    subTrack.expungeOldSubs(v0);

    subTrack.get(v0, v1, subs);
  }

  //----------------------------------------------------------------
  // add_to
  //
  static std::ostringstream &
  add_to(std::ostringstream & filters, const char * filter = NULL)
  {
    if (!filters.str().empty())
    {
      filters << ',';
    }

    if (filter)
    {
      filters << filter;
    }

    return filters;
  }

  //----------------------------------------------------------------
  // FrameGen
  //
  struct FrameGen
  {
    AvFrm get(const AvFrm & src) const
    {
      const AVFrame & frame = src.get();
      const AVPixelFormat pix_fmt = src.get_pix_fmt();
      const char * pix_fmt_name = av_get_pix_fmt_name(pix_fmt);
      const Colorspace * csp = Colorspace::get(frame.colorspace,
                                               frame.color_primaries,
                                               frame.color_trc);
      std::string key = yae::strfmt("%04ix%04i, %s, %s",
                                    frame.width,
                                    frame.height,
                                    pix_fmt_name,
                                    csp->name_.c_str());
      AvFrm & frm = cache_[key];
      if (!frm.has_data())
      {
        ColorbarsGenerator tex_gen(frame.width, frame.height, csp);
        frm = make_textured_frame(tex_gen,
                                  pix_fmt,
                                  frame.width,
                                  frame.height,
                                  frame.color_range);
        save_as_png(frm, std::string("/tmp/tex_gen_"), TTime(1, 60));
      }

      av_frame_copy_props(&frm.get(), &src.get());
      return frm;
    }

    mutable std::map<std::string, AvFrm> cache_;
  };

  //----------------------------------------------------------------
  // VideoTrack::handle
  //
  void
  VideoTrack::handle(const AvFrm & decodedFrame)
  {
    // YAE_BENCHMARK(benchmark, "VideoTrack::handle");

    // keep alive:
    Track::TInfoPtr track_info_ptr = this->get_info();
    YAE_RETURN_IF(!track_info_ptr);

    const Track::Info & track_info = *track_info_ptr;

    try
    {
      AvFrm decodedFrameCopy(decodedFrame);
      decodedFrameCopy.hwdownload();

      AVFrame & decoded = decodedFrameCopy.get();
      framesDecoded_++;

      // update native traits first:
      VideoTraits native_traits;
      this->getTraits(native_traits, &decoded);
      if (native_ != native_traits)
      {
        native_ = native_traits;

        // keep-alive:
        TEventObserverPtr eo = eo_;
        if (eo)
        {
          std::string track_id = this->get_track_id();
          Json::Value event;
          event["event_type"] = "traits_changed";
          event["track_id"] = track_id;
          eo->note(event);
        }
      }

      // fill in any missing specs:
      add_missing_specs(decodedFrameCopy.get(), AvFrmSpecs(native_));

#if 0
      // for debugging Colorspace and frame utils:
      static const FrameGen frameGen;
      decodedFrameCopy = frameGen.get(decodedFrameCopy);
#endif

#if 0 // ndef NDEBUG
      {
        boost::chrono::steady_clock::time_point
          t1 = boost::chrono::steady_clock::now();

        uint64 dt =
          boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
          count();

        double fps = double(framesDecoded_) / (1e-6 * double(dt));

        yae_debug
          << id_
          << ", frames decoded: " << framesDecoded_
          << ", elapsed time: " << dt << " usec, decoder fps: " << fps
          << "\n";
      }
#endif

      double sourcePixelAspectRatio =
        overrideSourcePAR_ ? overrideSourcePAR_ :  native_.pixelAspectRatio_;

      double overridePixelAspectRatio =
        override_.pixelAspectRatio_; // overrideSourcePAR_ ? 1.0 : 0.0;

      const AvFrmSpecs src_specs(decoded);
      if (!yae::same_specs(src_specs, filterGraph_.src_specs()))
      {
        refreshTraits(&decoded);
      }

      AvFrmSpecs outSpecs = src_specs;

      // apply overrides:
      if (override_.pixelFormat_ != kInvalidPixelFormat)
      {
        outSpecs.format = override_.av_fmt_;
      }

      if (override_.av_rng_ != AVCOL_RANGE_UNSPECIFIED)
      {
        outSpecs.color_range = override_.av_rng_;
      }

      if (override_.colorspace_)
      {
        outSpecs.colorspace = override_.av_csp_;
        outSpecs.color_primaries = override_.av_pri_;
        outSpecs.color_trc = override_.av_trc_;
      }

      // configure the filter chain:
      std::ostringstream filters;

      bool outputNeedsScale =
        (override_.visibleWidth_ || override_.visibleHeight_ ||
         (overridePixelAspectRatio > 0.0 &&
          overridePixelAspectRatio != sourcePixelAspectRatio)) &&
        (native_.visibleWidth_ != output_.visibleWidth_ ||
         native_.visibleHeight_ != output_.visibleHeight_);

      bool nativeNeedsCrop =
        (native_.offsetTop_ != 0 || native_.offsetLeft_ != 0);

      bool outputNeedsCrop =
        (output_.offsetTop_ != 0 || output_.offsetLeft_ != 0);

      YAE_ASSERT(!(outputNeedsCrop && outputNeedsScale));

      bool shouldCrop = nativeNeedsCrop && outputNeedsScale;

      if (shouldCrop)
      {
        filters
          << "crop=x=" << native_.offsetLeft_
          << ":y=" << native_.offsetTop_
          << ":out_w=" << native_.visibleWidth_
          << ":out_h=" << native_.visibleHeight_;
      }

      if (deinterlace_)
      {
        if (skipNonReferenceFrames_)
        {
          // when non-reference frames are discarded deinterlacing filter
          // loses ability to detect interlaced frames, therefore
          // it is better to simply drop a field:
          add_to(filters)
            << "yadif=mode=send_frame_nospatial:parity=tff:deint=all";
        }
        else
        {
          add_to(filters)
            << "yadif=mode=send_frame:parity=auto:deint=all";
        }
      }
#if 0
      else // inverse telecine
      {
        add_to(filters)
          << "fieldmatch=order=tff:combmatch=full, "
          << "yadif=deint=interlaced, "
          << "decimate";
      }
#endif

      int transposeAngle =
        ((native_.cameraRotation_ - output_.cameraRotation_) - 180) % 180;

      bool flipAngle =
        transposeAngle ? 0 :
        (output_.cameraRotation_ - native_.cameraRotation_) % 360;

      bool vflip =
        (native_.vflip_ != output_.vflip_);

      bool hflip =
        (native_.hflip_ != output_.hflip_);

      if (vflip || hflip || flipAngle)
      {
        if (vflip && flipAngle)
        {
          // cancel-out two vertical flips:
          add_to(filters, "hflip");
        }
        else if (flipAngle)
        {
          add_to(filters, "hflip, vflip");
        }
        else if (vflip)
        {
          add_to(filters, "vflip");
        }
        else if (hflip)
        {
          add_to(filters, "hflip");
        }
      }

      if (outputNeedsScale || transposeAngle)
      {
        if (transposeAngle)
        {
          add_to(filters)
            << "scale=w=" << output_.visibleHeight_
            << ":h=" << output_.visibleWidth_;
        }
        else
        {
          add_to(filters)
            << "scale=w=" << output_.visibleWidth_
            << ":h=" << output_.visibleHeight_;
        }
      }

      if (transposeAngle)
      {
        add_to(filters) << ((transposeAngle < 0) ?
                            "transpose=dir=clock" :
                            "transpose=dir=cclock");

        outSpecs.width = output_.visibleHeight_;
        outSpecs.height = output_.visibleWidth_;
      }
      else
      {
        outSpecs.width = output_.visibleWidth_;
        outSpecs.height = output_.visibleHeight_;
      }

      if (overridePixelAspectRatio)
      {
        add_to(filters) << "setsar=sar=" << output_.pixelAspectRatio_;
        outSpecs.sample_aspect_ratio.num = 216000 * output_.pixelAspectRatio_;
        outSpecs.sample_aspect_ratio.den = 216000;
      }

#if 0
      add_to(filters) <<
        "boxblur="
        "luma_radius=min(h\\,w)/8:"
        "luma_power=1:"
        "chroma_radius=min(cw\\,ch)/8:"
        "chroma_power=1,"

        "drawtext="
        "fontfile=/usr/share/fonts/truetype/DroidSansMono.ttf:"
        "fontcolor=white:"
        "box=1:"
        "boxcolor=black:"
        "boxborderw=5:"
        "x=(w-text_w)-5:"
        "y=h-(max_glyph_h+10)*3:"
        "text=%{n} %{pict_type} %{pts\\\\:hms}";
#endif

      std::string filterChain(filters.str().c_str());
      yae::sanitize_color_specs(outSpecs);

      if (filterGraph_.setup(decoded,
                             frameRate_,
                             stream_->time_base,
                             outSpecs,
                             filterChain.c_str()))
      {
        yae_ilog("VideoTrack filters: %s", filterGraph_.get_filters().c_str());
        if (!confirm_supported_output_format(outSpecs))
        {
          return;
        }
      }

      if (decoded.pts == AV_NOPTS_VALUE)
      {
        decoded.pts = decoded.best_effort_timestamp;
      }

      if (decoded.pts == AV_NOPTS_VALUE)
      {
        decoded.pts = decoded.pkt_dts;
      }

      TTime t0(stream_->time_base.num * decoded.pts,
               stream_->time_base.den);

      TTime t(t0);

      bool gotPTS = verify_pts(hasPrevPTS_, prevPTS_, t, stream_, "video t");

      if (!gotPTS && !hasPrevPTS_)
      {
        t.time_ = stream_->time_base.num * startTime_;
        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, t, stream_, "video t0");
      }

      if (!gotPTS && hasPrevPTS_ && frameRate_.num && frameRate_.den)
      {
        // increment by average frame duration:
        t = prevPTS_;
        t += TTime(frameRate_.den, frameRate_.num);
        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, t, stream_,
                            "video t += 1/fps");
      }

      YAE_ASSERT(gotPTS);
      if (!gotPTS && hasPrevPTS_)
      {
        t = prevPTS_;
        t.time_++;
        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, t, stream_, "video t++");
      }

      YAE_ASSERT(gotPTS);
      if (gotPTS)
      {
#ifndef NDEBUG
        if (hasPrevPTS_)
        {
          double ta = prevPTS_.sec();
          double tb = t.sec();
          // yae_debug << "video pts: " << tb << "\n";
          double dt = tb - ta;
          double fd = 1.0 / native_.frameRate_;
          // yae_debug << ta << " ... " << tb << ", dt: " << dt << "\n";
          if (dt > 3.01 * fd)
          {
            yae_debug
              << "\nNOTE: detected large PTS jump: "
              << "\nframe\t:" << framesDecoded_ - 2 << " - " << ta
              << "\nframe\t:" << framesDecoded_ - 1 << " - " << tb
              << "\ndifference " << dt << " seconds, equivalent to "
              << dt / fd << " frames" << "\n\n";
          }
        }
#endif

        hasPrevPTS_ = true;
        prevPTS_ = t;

        if (t != t0)
        {
          // update AVFrame.pts to match:
          AVRational timeBase;
          timeBase.num = 1;
          timeBase.den = t.base_;
          decoded.pts = av_rescale_q(t.time_, timeBase, stream_->time_base);
        }
      }

      // decode CEA-608 packets, if there are any:
      cc_.decode(stream_->time_base, decoded, &terminator_);
#if 0
      std::string fn_prefix = "/tmp/video-filter-push-";
      YAE_ASSERT(save_as_png(decodedFrameCopy, fn_prefix, TTime(1, 30)));
#endif
      filterGraph_.push(&decoded);

      while (true)
      {
        AVRational filterGraphOutputTimeBase;
        AvFrm frm;
        AVFrame & output = frm.get();

        if (!filterGraph_.pull(&output, filterGraphOutputTimeBase))
        {
          break;
        }
#if 0
        std::string fn_prefix = "/tmp/video-filter-pull-";
        YAE_ASSERT(save_as_png(frm, fn_prefix, TTime(1, 30)));
#endif
        TVideoFramePtr vfPtr(new TVideoFrame());
        TVideoFrame & vf = *vfPtr;

        if (!packet_pos_.empty())
        {
          vf.pos_.base_ = 188;
          vf.pos_.time_ =
            (packet_pos_.size() == 1) ?
            packet_pos_.front() :
            packet_pos_.pop();
        }

        vf.time_.base_ = filterGraphOutputTimeBase.den;
        vf.time_.time_ = filterGraphOutputTimeBase.num * output.pts;
        vf.trackId_ = track_info.track_id_;

        // make sure the frame is in the in/out interval:
        if (playbackEnabled_)
        {
          double dt = 1.0 / double(output_.frameRate_);
          bool after_out_point = posOut_->lt(vf);
          bool before_in_point = posIn_->gt(vf, dt);
          if (after_out_point || before_in_point)
          {
            if (after_out_point)
            {
              discarded_++;
            }
#if 0
            yae_debug << "discarding video frame: " << posIn_->to_str(vf)
                      << ", expecting [" << posIn_->to_str()
                      << ", " << posOut_->to_str() << ")\n";
#endif
            return;
          }

          discarded_ = 0;
        }

        vf.traits_ = output_;
        vf.traits_.set_pixel_format(yae::pix_fmt(output));

        // preserve output color specs:
        vf.traits_.av_rng_ = outSpecs.color_range;
        vf.traits_.av_pri_ = outSpecs.color_primaries;
        vf.traits_.av_trc_ = outSpecs.color_trc;
        vf.traits_.av_csp_ = outSpecs.colorspace;

        if (output.linesize[0] < 0)
        {
          // upside-down frame, actually flip it around (unlike vflip):
          const pixelFormat::Traits * ptts =
            pixelFormat::getTraits(vf.traits_.pixelFormat_);

          unsigned char stride[4] = { 0 };
          std::size_t numSamplePlanes = ptts->getPlanes(stride);

          std::size_t lumaPlane =
            (ptts->flags_ & pixelFormat::kPlanar) ? 0 : numSamplePlanes;

          std::size_t alphaPlane =
            ((ptts->flags_ & pixelFormat::kAlpha) &&
             (ptts->flags_ & pixelFormat::kPlanar)) ?
            numSamplePlanes - 1 : numSamplePlanes;

          for (unsigned char i = 0; i < numSamplePlanes; i++)
          {
            std::size_t rows = output.height;
            if (i != lumaPlane && i != alphaPlane)
            {
              rows /= ptts->chromaBoxH_;
            }

            int rowBytes = -output.linesize[i];
            if (rowBytes <= 0)
            {
              continue;
            }

            temp_.resize(rowBytes);
            unsigned char * temp = &temp_[0];
            unsigned char * tail = output.data[i];
            unsigned char * head = tail + output.linesize[i] * (rows - 1);

            output.data[i] = head;
            output.linesize[i] = rowBytes;

            while (head < tail)
            {
              memcpy(temp, head, rowBytes);
              memcpy(head, tail, rowBytes);
              memcpy(tail, temp, rowBytes);

              head += rowBytes;
              tail -= rowBytes;
            }
          }
        }

        // check for content light level:
        {
          const AVFrameSideData * side_data = NULL;
#if 0
          if ((side_data = av_frame_get_side_data
               (&output, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA)))
          {
            const AVMasteringDisplayMetadata * metadata =
              (const AVMasteringDisplayMetadata *)(side_data->data);

#ifndef NDEBUG
            if (metadata->has_primaries)
            {
              yae_dlog("primaries xyY: "
                       "W (%.4f, %.4f), "
                       "R (%.3f, %.3f), "
                       "G (%.3f, %.3f), "
                       "B (%.3f, %.3f)",
                       av_q2d(metadata->white_point[0]),
                       av_q2d(metadata->white_point[1]),
                       av_q2d(metadata->display_primaries[0][0]),
                       av_q2d(metadata->display_primaries[0][1]),
                       av_q2d(metadata->display_primaries[1][0]),
                       av_q2d(metadata->display_primaries[1][1]),
                       av_q2d(metadata->display_primaries[2][0]),
                       av_q2d(metadata->display_primaries[2][1]));
            }

            if (metadata->has_luminance)
            {
              yae_dlog("min luminance: %f cd/m2, max luminance %f cd/m2",
                       av_q2d(metadata->min_luminance),
                       av_q2d(metadata->max_luminance));
            }
#endif

            if (metadata->has_luminance)
            {
              double max_luminance = av_q2d(metadata->max_luminance);
              if (max_luminance > 100.0)
              {
                vf.traits_.dynamic_range_.max_cll_ = max_luminance;
              }
            }
          }
#endif
#if 1
          if ((side_data = av_frame_get_side_data
               (&output, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL)))
          {
            const AVContentLightMetadata * metadata =
              (const AVContentLightMetadata *)(side_data->data);

#if 0 // ndef NDEBUG
            yae_dlog("MaxFALL: %u cd/m2, MaxCLL: %u cd/m2",
                     metadata->MaxFALL,
                     metadata->MaxCLL);
#endif
            if (metadata->MaxCLL > 0)
            {
              vf.traits_.dynamic_range_.max_cll_ = metadata->MaxCLL;
            }

            if (metadata->MaxFALL > 0)
            {
              vf.traits_.dynamic_range_.max_fall_ = metadata->MaxFALL;
            }
          }
#endif
        }

        // use AVFrame directly:
        TIPlanarBufferPtr sampleBuffer(new TAVFrameBuffer(&output),
                                       &IPlanarBuffer::deallocator);
        vf.traits_.visibleWidth_ = output.width;
        vf.traits_.visibleHeight_ = output.height;
        vf.traits_.encodedWidth_ = vf.traits_.visibleWidth_;
        vf.traits_.encodedHeight_ = vf.traits_.visibleHeight_;
        vf.data_ = sampleBuffer;

        // don't forget about tempo scaling:
        {
          boost::lock_guard<boost::mutex> lock(tempoMutex_);
          vf.tempo_ = tempo_;
        }

        // check for applicable subtitles:
        {
          double v0 = vf.time_.sec();
          double v1 = v0 + (vf.traits_.frameRate_ ?
                            1.0 / vf.traits_.frameRate_ :
                            0.042);

          std::size_t nsubs = subs_ ? subs_->size() : 0;
          for (std::size_t i = 0; i < nsubs; i++)
          {
            SubtitlesTrack & subTrack = *((*subs_)[i]);
            gatherApplicableSubtitles(vf.subs_, v0, v1, subTrack, terminator_);
          }

          // and closed captions also:
          SubtitlesTrack * cc = cc_.captions();
          if (cc)
          {
            gatherApplicableSubtitles(vf.subs_, v0, v1, *cc, terminator_);
          }
        }

#if 0 // ndef NDEBUG
      {
        boost::chrono::steady_clock::time_point
          t1 = boost::chrono::steady_clock::now();

        uint64 dt =
          boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
          count();

        framesProduced_++;
        double fps = double(framesProduced_) / (1e-6 * double(dt));

        yae_debug
          << Track::id_
          << ", frames produced: " << framesProduced_
          << ", elapsed time: " << dt << " usec, fps: " << fps
          << "\n";
      }
#endif

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        {
          std::string ts = to_hhmmss_ms(vf);
          yae_debug << "push video frame: " << ts << "\n";
        }
#endif

        // match the output frame queue size to the input frame queue size,
        // plus 10 percent:
        frameQueue_.setMaxSize((packetQueue_.getMaxSize() * 11) / 10);

        // put the output frame into frame queue:
        if (!frameQueue_.push(vfPtr, &terminator_))
        {
          return;
        }

        // yae_debug << "V: " << vf.time_.sec() << "\n";
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // VideoTrack::threadStop
  //
  bool
  VideoTrack::threadStop()
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    yae_debug << "\n\t\t\t\tVIDEO TRACK THREAD STOP\n";
#endif

    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // fp16
  //
  static inline double fp16(int fp16_number)
  {
    static const double scale = double(1 << 16);
    return double(fp16_number) / scale;
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraits
  //
  bool
  VideoTrack::getTraits(VideoTraits & t, const AVFrame * decoded) const
  {
    if (!(stream_ && stream_->codecpar))
    {
      return false;
    }

    const AVCodecParameters & codecParams = *(stream_->codecpar);

    AvFrmSpecs specs;
    if (decoded)
    {
      specs = AvFrmSpecs(*decoded);
    }
    else if (native_.av_fmt_ != AV_PIX_FMT_NONE)
    {
      t = native_;
      return true;
    }
    else
    {
      specs.width = codecParams.width;
      specs.height = codecParams.height;
      specs.format = (AVPixelFormat)(codecParams.format);
      specs.colorspace = codecParams.color_space;
      specs.color_range = codecParams.color_range;
      specs.color_primaries = codecParams.color_primaries;
      specs.color_trc = codecParams.color_trc;
      specs.chroma_location = codecParams.chroma_location;
      specs.sample_aspect_ratio = codecParams.sample_aspect_ratio;

      // YAE_ASSERT(specs.format != AV_PIX_FMT_NONE);
      if (specs.format == AV_PIX_FMT_NONE)
      {
        // Sony_Whale_Tracks.ts?
        Track track(context_, stream_, hwdec_);
        VideoTrack video_track(&track);
        AvCodecContextPtr ctx_ptr = video_track.open();
        AVCodecContext * ctx = ctx_ptr.get();
        if (ctx->pix_fmt != AV_PIX_FMT_NONE)
        {
          specs.width = ctx->width;
          specs.height = ctx->height;
          specs.format = ctx->pix_fmt;
          specs.colorspace = ctx->colorspace;
          specs.color_range = ctx->color_range;
          specs.color_primaries = ctx->color_primaries;
          specs.color_trc = ctx->color_trc;
          specs.chroma_location = ctx->chroma_sample_location;
          specs.sample_aspect_ratio = ctx->sample_aspect_ratio;
        }
      }
    }

    specs.guess_missing_specs();

    t.av_fmt_ = specs.get_pix_fmt();
    t.av_rng_ = specs.color_range;
    t.av_pri_ = specs.color_primaries;
    t.av_trc_ = specs.color_trc;
    t.av_csp_ = specs.colorspace;

    t.colorspace_ = Colorspace::get(t.av_csp_, t.av_pri_, t.av_trc_);

    if (t.av_trc_ == AVCOL_TRC_SMPTE2084)
    {
      // HDR10, DolbyVision:
      t.dynamic_range_.Lw_ = 10000.0; // cd/m2
      t.dynamic_range_.max_cll_ = 4000.0; // cd/m2
      t.dynamic_range_.max_fall_ = 400.0; // cd/m2
    }
    else if (t.av_trc_ == AVCOL_TRC_ARIB_STD_B67)
    {
      // HLG:
      t.dynamic_range_.Lw_ = 1000.0; // cd/m2
      t.dynamic_range_.max_cll_ = 1000.0; // cd/m2
      t.dynamic_range_.max_fall_ = 400.0; // cd/m2
    }
    else
    {
      // SDR?
      t.dynamic_range_.Lw_ = 100.0; // cd/m2
      t.dynamic_range_.max_cll_ = 100.0; // cd/m2
      t.dynamic_range_.max_fall_ = 33.0; // cd/m2
    }

    //! pixel format:
    t.set_pixel_format(t.av_fmt_);

    //! frame rate:
    const AVRational & r_frame_rate = stream_->r_frame_rate;

    if (stream_->avg_frame_rate.num > 0 && stream_->avg_frame_rate.den > 0)
    {
      t.frameRate_ =
        double(stream_->avg_frame_rate.num) /
        double(stream_->avg_frame_rate.den);
    }
    else if (r_frame_rate.num > 0 && r_frame_rate.den > 0)
    {
      t.frameRate_ =
        double(r_frame_rate.num) /
        double(r_frame_rate.den);

      if (context_->metadata)
      {
        AVDictionaryEntry * frameRateTag =
          av_dict_get(context_->metadata, "framerate", NULL, 0);

        AVDictionaryEntry * totalFramesTag =
          av_dict_get(context_->metadata, "totalframes", NULL, 0);

        if (frameRateTag)
        {
          t.frameRate_ = to_scalar<double, const char *>(frameRateTag->value);
        }
        else if (totalFramesTag &&
                 context_->duration &&
                 context_->duration != int64_t(AV_NOPTS_VALUE))
        {
          // estimate frame rate based on duration
          // and metadata for total number of frames:
          double totalSeconds =
            double(context_->duration) / double(AV_TIME_BASE);

          int64_t totalFrames =
            to_scalar<int64_t, const char *>(totalFramesTag->value);

          if (totalFrames)
          {
            double r = double(totalFrames) / totalSeconds;
            t.frameRate_ = std::min<double>(t.frameRate_, r);
          }
        }
      }
    }
    else
    {
      t.frameRate_ = 0.0;
    }

    //! encoded frame size (including any padding):
    t.encodedWidth_ = specs.width;
    t.encodedHeight_ = specs.height;

    //! top/left corner offset to the visible portion of the encoded frame:
    t.offsetTop_ = 0;
    t.offsetLeft_ = 0;

    //! dimensions of the visible portion of the encoded frame:
    t.visibleWidth_ = specs.width;
    t.visibleHeight_ = specs.height;

    //! pixel aspect ration, used to calculate visible frame dimensions:
    t.pixelAspectRatio_ = 1.0;

    if (specs.sample_aspect_ratio.num &&
        specs.sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(specs.sample_aspect_ratio.num) /
                             double(specs.sample_aspect_ratio.den));
    }

    if (stream_->sample_aspect_ratio.num &&
        stream_->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(stream_->sample_aspect_ratio.num) /
                             double(stream_->sample_aspect_ratio.den));
    }

    //! check for rotation:
    {
#if 0
      AVDictionaryEntry * md = NULL;
      while (true)
      {
        md = av_dict_get(stream_->metadata, "", md, AV_DICT_IGNORE_SUFFIX);
        if (!md)
        {
          break;
        }

        yae_dlog("VideoTrack metadata: %s = %s", md->key, md->value);
      }
#endif

      // check for rotation, hflip, vflip:
      t.cameraRotation_ = 0;
      t.vflip_ = false;
      t.hflip_ = false;

      AVDictionaryEntry * rotate =
        av_dict_get(stream_->metadata, "rotate", NULL, 0);

      if (rotate)
      {
        t.cameraRotation_ = to_scalar<int>(rotate->value);
      }

      // check the side data:
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 15, 100)
      const int32_t * dm = (const int32_t *)
        av_stream_get_side_data(stream_, AV_PKT_DATA_DISPLAYMATRIX, NULL);
#else
      const AVPacketSideData * sd =
        av_packet_side_data_get(stream_->codecpar->coded_side_data,
                                stream_->codecpar->nb_coded_side_data,
                                AV_PKT_DATA_DISPLAYMATRIX);
      YAE_ASSERT(!sd || sd->size == 9 * sizeof(int32_t));
      const int32_t * dm = sd ? (const int32_t *)(sd->data) : NULL;
#endif
      if (dm)
      {
        m3x3_t M = yae::make_m3x3(fp16(dm[0]), fp16(dm[3]), 0.0,
                                  fp16(dm[1]), fp16(dm[4]), 0.0,
                                  0.0,         0.0,         1.0);
        v3x1_t p00 = yae::make_v3x1(0.0, 0.0, 1.0);
        v3x1_t p01 = yae::make_v3x1(0.0, 1.0, 1.0);
        v3x1_t p10 = yae::make_v3x1(1.0, 0.0, 1.0);

        v3x1_t q00 = M * p00;
        v3x1_t q01 = M * p01;
        v3x1_t q10 = M * p10;

        v3x1_t x = q10 - q00;
        v3x1_t y = q01 - q00;

        t.cameraRotation_ =
          (x[1] < -0.5 && y[0] > 0.5) ? 270 :
          (x[0] < -0.5 && y[1] < -0.5) ? 180 :
          (x[1] > 0.5 && y[0] < -0.5) ? 90 :
          // do not rotate:
          0;

        if (x[0] > 0.5 && y[1] < -0.5)
        {
          t.vflip_ = true;
        }
        else if (x[0] < -0.5 && y[1] > 0.5)
        {
          t.hflip_ = true;
        }
      }
    }

    return
      // t.pixelFormat_ != kInvalidPixelFormat &&
      t.frameRate_ > 0.0 &&
      t.encodedWidth_ > 0 &&
      t.encodedHeight_ > 0;
  }

  //----------------------------------------------------------------
  // VideoTrack::setTraitsOverride
  //
  bool
  VideoTrack::setTraitsOverride(const VideoTraits & traits)
  {
    bool sameTraits = compare<VideoTraits>(override_, traits) == 0;
    if (sameTraits)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(sameTraits || !alreadyDecoding);

    if (alreadyDecoding && !sameTraits)
    {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      yae_debug << "\n\t\t\t\tSET TRAITS OVERRIDE\n";
#endif

      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.interrupt();
      thread_.wait();
    }

    override_ = traits;

    if (alreadyDecoding && !sameTraits)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraitsOverride
  //
  bool
  VideoTrack::getTraitsOverride(VideoTraits & traits) const
  {
    traits = override_;
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getNextFrame
  //
  bool
  VideoTrack::getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    // YAE_BENCHMARK(benchmark, "VideoTrack::getNextFrame");

    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || resetTimeCountersIndicated(frame.get()))
      {
        break;
      }

      // discard outlier frames:
      const TVideoFrame & vf = *frame;
      double dt = vf.durationInSeconds();

      if ((!playbackEnabled_ || posOut_->gt(vf)) && posIn_->lt(vf, dt))
      {
        break;
      }
    }

    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::setPlaybackInterval
  //
  void
  VideoTrack::setPlaybackInterval(const TSeekPosPtr & posIn,
                                  const TSeekPosPtr & posOut,
                                  bool enabled)
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    yae_debug << "SET VIDEO TRACK TIME IN: " << posIn->to_str() << "\n";
#endif

    posIn_ = posIn;
    posOut_ = posOut;
    playbackEnabled_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // VideoTrack::resetTimeCounters
  //
  int
  VideoTrack::resetTimeCounters(const TSeekPosPtr & seekPos,
                                bool dropPendingFrames)
  {
    this->packetQueueClear();

    if (dropPendingFrames)
    {
      // NOTE: this drops any pending frames preventing their playback;
      // This is desirable when the user is seeking, but it prevents
      // proper in-out point playback because some frames will be dropped
      // when the video is rewound to the in-point:
      do { frameQueue_.clear(); }
      while (!packetQueue_.waitForConsumerToBlock(1e-2));
      frameQueue_.clear();
    }

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    yae_debug
      << "\n\tVIDEO TRACK reset time counter, start new sequence\n\n";
#endif

    // drop filtergraph contents:
    filterGraph_.reset();

    // force the closed captions decoder to be re-created on demand:
    cc_.reset();

    // push a special frame into frame queue to resetTimeCounters
    // down the line (the renderer):
    startNewSequence(frameQueue_, dropPendingFrames);

    // keep-alive:
    AvCodecContextPtr ctx_ptr = codecContext_;
    AVCodecContext * ctx = ctx_ptr.get();
    int err = 0;
    if (stream_ && ctx)
    {
      avcodec_flush_buffers(ctx);
#if 1
      Track::close();
      ctx_ptr = Track::open();
      ctx = ctx_ptr.get();
      YAE_ASSERT(ctx);
#endif
    }

    if (seekPos)
    {
      setPlaybackInterval(seekPos, posOut_, playbackEnabled_);
    }

    startTime_ = 0;
    hasPrevPTS_ = false;
    framesDecoded_ = 0;
    framesProduced_ = 0;
#ifndef NDEBUG
    this->t0_ = boost::chrono::steady_clock::now();
#endif

    return err;
  }

  //----------------------------------------------------------------
  // VideoTrack::setDeinterlacing
  //
  void
  VideoTrack::setDeinterlacing(bool deint)
  {
    deinterlace_ = deint;
  }

  //----------------------------------------------------------------
  // VideoTrack::overridePixelAspectRatio
  //
  void
  VideoTrack::overridePixelAspectRatio(double source_par)
  {
    overrideSourcePAR_ = source_par;
  }

  //----------------------------------------------------------------
  // VideoTrack::enableClosedCaptions
  //
  void
  VideoTrack::enableClosedCaptions(unsigned int cc)
  {
    cc_.enableClosedCaptions(cc);
  }

}
