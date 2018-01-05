// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost library:
#include <boost/algorithm/string.hpp>

// yae includes:
#include "yae/ffmpeg/yae_closed_captions.h"
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_pixel_format_ffmpeg.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_pixel_format_traits.h"

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
  VideoTrack::VideoTrack(Track & track):
    Track(track),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    deinterlace_(false),
    frameQueue_(kQueueSizeSmall),
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
  }

  //----------------------------------------------------------------
  // VideoTrack::initTraits
  //
  bool
  VideoTrack::initTraits()
  {
    if (!getTraits(override_))
    {
      return false;
    }

    native_ = override_;
    output_ = override_;

    // do not override width/height/sar unintentionally:
    override_.visibleWidth_ = 0;
    override_.visibleHeight_ = 0;
    override_.pixelAspectRatio_ = 0.0;
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::open
  //
  AVCodecContext *
  VideoTrack::open()
  {
    if (codecContext_)
    {
      return codecContext_.get();
    }

    AVCodecContext * ctx = Track::open();
    if (ctx)
    {
      framesDecoded_ = 0;
      framesProduced_ = 0;
      skipLoopFilter(skipLoopFilter_);
      skipNonReferenceFrames(skipNonReferenceFrames_);
    }

    return ctx;
  }

  //----------------------------------------------------------------
  // VideoTrack::skipLoopFilter
  //
  void
  VideoTrack::skipLoopFilter(bool skip)
  {
    skipLoopFilter_ = skip;

    if (codecContext_)
    {
      if (skipLoopFilter_)
      {
        codecContext_->skip_loop_filter = AVDISCARD_ALL;
        codecContext_->flags2 |= AV_CODEC_FLAG2_FAST;
      }
      else
      {
        codecContext_->skip_loop_filter = AVDISCARD_DEFAULT;
        codecContext_->flags2 &= ~(AV_CODEC_FLAG2_FAST);
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

    if (codecContext_)
    {
      if (skipNonReferenceFrames_)
      {
        codecContext_->skip_frame = AVDISCARD_NONREF;
      }
      else
      {
        codecContext_->skip_frame = AVDISCARD_DEFAULT;
      }
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::refreshTraits
  //
  void
  VideoTrack::refreshTraits()
  {
    // shortcut to native frame format traits:
    getTraits(native_);

    // frame size may have changed, so update output traits accordingly:
    output_ = override_;

    int transposeAngle =
      (override_.cameraRotation_ - native_.cameraRotation_) % 180;

    if (override_.visibleWidth_ ||
        override_.visibleHeight_ ||
        transposeAngle != 0)
    {
      // NOTE: the override provides a scale-to-fit frame envelope,
      // not the actual frame size:

      const double envelope_par =
        (override_.pixelAspectRatio_ ?
         override_.pixelAspectRatio_ :
         native_.pixelAspectRatio_);

      const double envelope_dar =
        envelope_par *
        (transposeAngle ?
         (double(override_.visibleHeight_) /
          double(override_.visibleWidth_)) :
         (double(override_.visibleWidth_) /
          double(override_.visibleHeight_)));

      const double native_dar =
        native_.pixelAspectRatio_ *
        (double(native_.visibleWidth_) /
         double(native_.visibleHeight_));

      double dar_scale = native_dar / envelope_par;

      if (native_dar < envelope_dar)
      {
        if (transposeAngle)
        {
          output_.visibleWidth_ = override_.visibleWidth_ * dar_scale + 0.5;
          output_.visibleHeight_ = override_.visibleWidth_;
        }
        else
        {
          output_.visibleWidth_ = override_.visibleHeight_ * dar_scale + 0.5;
          output_.visibleHeight_ = override_.visibleHeight_;
        }

        output_.offsetLeft_ = 0;
        output_.offsetTop_ = 0;
        output_.encodedWidth_ = output_.visibleWidth_;
        output_.encodedHeight_ = output_.visibleHeight_;
      }
      else
      {
        if (transposeAngle)
        {
          output_.visibleWidth_ = override_.visibleHeight_;
          output_.visibleHeight_ = override_.visibleHeight_ / dar_scale + 0.5;
        }
        else
        {
          output_.visibleWidth_ = override_.visibleWidth_;
          output_.visibleHeight_ = override_.visibleWidth_ / dar_scale + 0.5;
        }

        output_.offsetLeft_ = 0;
        output_.offsetTop_ = 0;
        output_.encodedWidth_ = output_.visibleWidth_;
        output_.encodedHeight_ = output_.visibleHeight_;
      }
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

    if (override_.pixelAspectRatio_ > 0.0 &&
        override_.pixelAspectRatio_ != native_.pixelAspectRatio_)
    {
      output_.visibleWidth_ = int(native_.visibleWidth_ *
                                  native_.pixelAspectRatio_ /
                                  override_.pixelAspectRatio_ + 0.5);
    }

    if (override_.pixelAspectRatio_)
    {
      output_.pixelAspectRatio_ = override_.pixelAspectRatio_;
    }
    else
    {
      output_.pixelAspectRatio_ = native_.pixelAspectRatio_;
    }

    if (output_.pixelFormat_ == kPixelFormatY400A &&
        native_.pixelFormat_ != kPixelFormatY400A)
    {
      // sws_getContext doesn't support Y400A, so drop the alpha channel:
      output_.pixelFormat_ = kPixelFormatGRAY8;
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::reconfigure
  //
  bool
  VideoTrack::reconfigure()
  {
    refreshTraits();

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(output_.pixelFormat_);

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
    std::cerr << "\n\t\t\t\tVIDEO TRACK DECODER STARTUP" << std::endl;
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
    std::cerr << "\n\t\t\t\tVIDEO TRACK DECODER SHUTDOWN" << std::endl;
#endif

    filterGraph_.reset();
    hasPrevPTS_ = false;
    frameQueue_.close();
    packetQueue_.close();
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
      double s0 = sf.time_.toSeconds();
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
  // VideoTrack::handle
  //
  void
  VideoTrack::handle(const AvFrm & decodedFrame)
  {
    try
    {
      AvFrm decodedFrameCopy(decodedFrame);
      AVFrame & decoded = decodedFrameCopy.get();
      framesDecoded_++;

#ifndef NDEBUG
      {
        boost::chrono::steady_clock::time_point
          t1 = boost::chrono::steady_clock::now();

        uint64 dt =
          boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
          count();

        double fps = double(framesDecoded_) / (1e-6 * double(dt));

        std::cerr
          << codecContext_->codec->name
          << ", frames decoded: " << framesDecoded_
          << ", elapsed time: " << dt << " usec, decoder fps: " << fps
          << std::endl;
      }
#endif

      enum AVPixelFormat ffmpegPixelFormat =
        yae_to_ffmpeg(output_.pixelFormat_);

      // configure the filter chain:
      std::ostringstream filters;

      bool outputNeedsScale =
        (override_.visibleWidth_ || override_.visibleHeight_ ||
         (override_.pixelAspectRatio_ > 0.0 &&
          override_.pixelAspectRatio_ != native_.pixelAspectRatio_)) &&
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
        (output_.cameraRotation_ - native_.cameraRotation_) % 180;

      bool flipAngle =
        transposeAngle ? 0 :
        (output_.cameraRotation_ - native_.cameraRotation_) % 360;

      bool toggleUpsideDown =
        (native_.isUpsideDown_ != output_.isUpsideDown_);

      if (toggleUpsideDown || flipAngle)
      {
        if (toggleUpsideDown && flipAngle)
        {
          // cancel-out two vertical flips:
          add_to(filters, "hflip");
        }
        else if (flipAngle)
        {
          add_to(filters, "hflip, vflip");
        }
        else
        {
          add_to(filters, "vflip");
        }
      }

      if (outputNeedsScale)
      {
        add_to(filters)
          << "scale=w=" << output_.visibleWidth_
          << ":h=" << output_.visibleHeight_;
      }

      if (override_.pixelAspectRatio_)
      {
        add_to(filters) << "setsar=sar=" << output_.pixelAspectRatio_;
      }

      if (transposeAngle)
      {
        add_to(filters) << ((transposeAngle < 0) ?
                            "transpose=dir=clock" :
                            "transpose=dir=cclock");
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
      bool frameTraitsChanged = false;
      if (!filterGraph_.setup(decoded.width,
                              decoded.height,
                              frameRate_,
                              stream_->time_base,
                              decoded.sample_aspect_ratio,
                              (AVPixelFormat)decoded.format,
                              ffmpegPixelFormat,
                              filterChain.c_str(),
                              &frameTraitsChanged))
      {
        YAE_ASSERT(false);
        return;
      }

      if (frameTraitsChanged && !reconfigure())
      {
        YAE_ASSERT(false);
        return;
      }

      if (decoded.pts == AV_NOPTS_VALUE)
      {
        decoded.pts = decoded.best_effort_timestamp;
      }

      if (decoded.pts == AV_NOPTS_VALUE)
      {
        decoded.pts = decoded.pkt_pts;
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
        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, t, stream_, "video t += 1/fps");
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
          double ta = prevPTS_.toSeconds();
          double tb = t.toSeconds();
          // std::cerr << "video pts: " << tb << std::endl;
          double dt = tb - ta;
          double fd = 1.0 / native_.frameRate_;
          // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
          if (dt > 3.01 * fd)
          {
            std::cerr
              << "\nNOTE: detected large PTS jump: " << std::endl
              << "frame\t:" << framesDecoded_ - 2 << " - " << ta << std::endl
              << "frame\t:" << framesDecoded_ - 1 << " - " << tb << std::endl
              << "difference " << dt << " seconds, equivalent to "
              << dt / fd << " frames" << std::endl
              << std::endl;
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

      if (!filterGraph_.push(&decoded))
      {
        YAE_ASSERT(false);
        return;
      }

      while (true)
      {
        AVRational filterGraphOutputTimeBase;
        AvFrm frm;
        AVFrame & output = frm.get();

        if (!filterGraph_.pull(&output, filterGraphOutputTimeBase))
        {
          break;
        }

        TVideoFramePtr vfPtr(new TVideoFrame());
        TVideoFrame & vf = *vfPtr;

        vf.time_.base_ = filterGraphOutputTimeBase.den;
        vf.time_.time_ = filterGraphOutputTimeBase.num * output.pts;

        // make sure the frame is in the in/out interval:
        if (playbackEnabled_)
        {
          double t = vf.time_.toSeconds();
          double dt = 1.0 / double(output_.frameRate_);
          if (t > timeOut_ || (t + dt) < timeIn_)
          {
            if (t > timeOut_)
            {
              discarded_++;
            }

#if 0
            std::cerr << "discarding video frame: " << t
                      << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                      << std::endl;
#endif
            return;
          }

          discarded_ = 0;
        }

        YAE_ASSERT(output_.initAbcToRgbMatrix_);
        vf.traits_ = output_;

        if (output.linesize[0] < 0)
        {
          // upside-down frame, actually flip it around (unlike vflip):
          const pixelFormat::Traits * ptts =
            pixelFormat::getTraits(output_.pixelFormat_);

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
          double v0 = vf.time_.toSeconds();
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

#ifndef NDEBUG
      {
        boost::chrono::steady_clock::time_point
          t1 = boost::chrono::steady_clock::now();

        uint64 dt =
          boost::chrono::duration_cast<boost::chrono::microseconds>(t1 - t0_).
          count();

        framesProduced_++;
        double fps = double(framesProduced_) / (1e-6 * double(dt));

        std::cerr
          << Track::id_
          << ", frames produced: " << framesProduced_
          << ", elapsed time: " << dt << " usec, fps: " << fps
          << std::endl;
      }
#endif

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        {
          std::string ts = to_hhmmss_usec(vfPtr);
          std::cerr << "push video frame: " << ts << std::endl;
        }
#endif

        // put the output frame into frame queue:
        if (!frameQueue_.push(vfPtr, &terminator_))
        {
          return;
        }

        // std::cerr << "V: " << vf.time_.toSeconds() << std::endl;
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
    std::cerr << "\n\t\t\t\tVIDEO TRACK THREAD STOP" << std::endl;
#endif

    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraits
  //
  bool
  VideoTrack::getTraits(VideoTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    const AVCodecParameters & codecParams = *(stream_->codecpar);
    AVPixelFormat pixelFormat = (AVPixelFormat)(codecParams.format);

    //! pixel format:
    t.pixelFormat_ = ffmpeg_to_yae(pixelFormat);

    //! for the color conversion coefficients:
    t.colorSpace_ = to_yae_color_space(codecParams.color_space);
    t.colorRange_ = to_yae_color_range(codecParams.color_range);
    t.initAbcToRgbMatrix_ = &init_abc_to_rgb_matrix;

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
          t.frameRate_ = toScalar<double, const char *>(frameRateTag->value);
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
            toScalar<int64_t, const char *>(totalFramesTag->value);

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
      YAE_ASSERT(false);
    }

    //! encoded frame size (including any padding):
    t.encodedWidth_ = codecParams.width;
    t.encodedHeight_ = codecParams.height;

    //! top/left corner offset to the visible portion of the encoded frame:
    t.offsetTop_ = 0;
    t.offsetLeft_ = 0;

    //! dimensions of the visible portion of the encoded frame:
    t.visibleWidth_ = codecParams.width;
    t.visibleHeight_ = codecParams.height;

    //! pixel aspect ration, used to calculate visible frame dimensions:
    t.pixelAspectRatio_ = 1.0;

    if (codecParams.sample_aspect_ratio.num &&
        codecParams.sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(codecParams.sample_aspect_ratio.num) /
                             double(codecParams.sample_aspect_ratio.den));
    }

    if (stream_->sample_aspect_ratio.num &&
        stream_->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(stream_->sample_aspect_ratio.num) /
                             double(stream_->sample_aspect_ratio.den));
    }

    //! a flag indicating whether video is upside-down:
    t.isUpsideDown_ = false;

    //! check for rotation:
    {
      AVDictionaryEntry * rotate =
        av_dict_get(stream_->metadata, "rotate", NULL, 0);

      if (rotate)
      {
        t.cameraRotation_ = toScalar<int>(rotate->value);
      }
      else
      {
        t.cameraRotation_ = 0;
      }
    }

    return
      t.frameRate_ > 0.0 &&
      t.encodedWidth_ > 0 &&
      t.encodedHeight_ > 0 &&
      t.pixelFormat_ != kInvalidPixelFormat;
  }

  //----------------------------------------------------------------
  // VideoTrack::setTraitsOverride
  //
  bool
  VideoTrack::setTraitsOverride(const VideoTraits & traits, bool deint)
  {
    bool sameTraits = compare<VideoTraits>(override_, traits) == 0;
    if (sameTraits && deinterlace_ == deint)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(sameTraits || !alreadyDecoding);

    if (alreadyDecoding && !sameTraits)
    {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      std::cerr << "\n\t\t\t\tSET TRAITS OVERRIDE" << std::endl;
#endif

      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = traits;
    deinterlace_ = deint;

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
    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || resetTimeCountersIndicated(frame.get()))
      {
        break;
      }

      // discard outlier frames:
      double t = frame->time_.toSeconds();
      double dt = 1.0 / frame->traits_.frameRate_;

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      static TTime prevTime(0, 1000);

      std::string in = TTime(timeIn_).to_hhmmss_usec(":");
      std::cerr << "\n\t\t\t\t\tTIME IN:          " << in << std::endl;

      std::string ts = to_hhmmss_usec(frame);
      std::cerr << "\t\t\t\t\tPOP video frame:  " << ts << std::endl;

      std::string t0 = prevTime.to_hhmmss_usec(":");
      std::cerr << "\t\t\t\t\tPREV video frame: " << t0 << std::endl;
#endif

      if ((!playbackEnabled_ || t < timeOut_) && (t + dt) > timeIn_)
      {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::cerr << "\t\t\t\t\tNEXT video frame: " << ts << std::endl;
        prevTime = frame->time_;
#endif
        break;
      }
    }

    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::setPlaybackInterval
  //
  void
  VideoTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      std::string in = TTime(timeIn).to_hhmmss_usec(":");
      std::cerr
        << "SET VIDEO TRACK TIME IN: " << in
        << std::endl;
#endif

    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackEnabled_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // VideoTrack::resetTimeCounters
  //
  int
  VideoTrack::resetTimeCounters(double seekTime, bool dropPendingFrames)
  {
    packetQueue_.clear();

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
    std::cerr
      << "\n\tVIDEO TRACK reset time counter, start new sequence\n"
      << std::endl;
#endif

    // drop filtergraph contents:
    filterGraph_.reset();

    // force the closed captions decoder to be re-created on demand:
    cc_.reset();

    // push a special frame into frame queue to resetTimeCounters
    // down the line (the renderer):
    startNewSequence(frameQueue_, dropPendingFrames);

    int err = 0;
    if (stream_ && codecContext_)
    {
      const AVCodec * codec = codecContext_->codec;
      AVCodecContext * ctx = codecContext_.get();

      avcodec_flush_buffers(ctx);
#if 1
      avcodec_close(ctx);
      avcodec_parameters_to_context(ctx, stream_->codecpar);
      err = avcodec_open2(ctx, codec, NULL);
      YAE_ASSERT(err >= 0);
#endif
    }

    setPlaybackInterval(seekTime, timeOut_, playbackEnabled_);
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
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
  bool
  VideoTrack::setDeinterlacing(bool deint)
  {
    candidates_.clear();
    return setTraitsOverride(override_, deint);
  }

  //----------------------------------------------------------------
  // VideoTrack::enableClosedCaptions
  //
  void
  VideoTrack::enableClosedCaptions(unsigned int cc)
  {
    AvCodecContextPtr keepAlive(codecContext_);
    AVCodecContext * ctx = keepAlive.get();

    candidates_.clear();
    preferSoftwareDecoder_ = cc > 0;

    if (ctx && cc && !cc_.enabled())
    {
      // switch to a decoder that produces side-data:
      std::size_t n = strlen(ctx->codec->name);
      std::string name;

      if (al::ends_with(ctx->codec->name, "_qsv"))
      {
        name = std::string(ctx->codec->name, ctx->codec->name + (n - 4));
      }
      else if (al::ends_with(ctx->codec->name, "_cuvid"))
      {
        name = std::string(ctx->codec->name, ctx->codec->name + (n - 6));
      }

      if (name == "mpeg2")
      {
        name = "mpeg2video";
      }

      if (name.size())
      {
        tryToSwitchDecoder(name);
      }
    }

    cc_.enableClosedCaptions(cc);
  }
}
