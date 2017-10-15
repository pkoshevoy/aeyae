// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/ffmpeg/yae_audio_track.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // AudioTrack::AudioTrack
  //
  AudioTrack::AudioTrack(Track & track):
    Track(track),
    frameQueue_(kQueueSizeMedium),
    nativeChannels_(0),
    outputChannels_(0),
    nativeBytesPerSample_(0),
    outputBytesPerSample_(0),
    hasPrevPTS_(false),
    prevNumSamples_(0),
    samplesDecoded_(0),
    tempoFilter_(NULL)
  {
    YAE_ASSERT(stream_->codecpar->codec_type == AVMEDIA_TYPE_AUDIO);

    // match output queue size to input queue size:
    frameQueue_.setMaxSize(packetQueue_.getMaxSize());
  }

  //----------------------------------------------------------------
  // AudioTrack::~AudioTrack
  //
  AudioTrack::~AudioTrack()
  {
    delete tempoFilter_;
    tempoFilter_ = NULL;
  }

  //----------------------------------------------------------------
  // AudioTrack::initTraits
  //
  bool
  AudioTrack::initTraits()
  {
    return getTraits(override_);
  }

  //----------------------------------------------------------------
  // AudioTrack::open
  //
  AVCodecContext *
  AudioTrack::open()
  {
    if (codecContext_)
    {
      return codecContext_.get();
    }

    AVCodecContext * ctx = Track::open();
    if (ctx)
    {
      samplesDecoded_ = 0;

      if (!ctx->channel_layout)
      {
        ctx->channel_layout = av_get_default_channel_layout(ctx->channels);
      }
    }

    return ctx;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderStartup
  //
  bool
  AudioTrack::decoderStartup()
  {
    output_ = override_;

    outputChannels_ = getNumberOfChannels(output_.channelLayout_);

    outputBytesPerSample_ =
      outputChannels_ * getBitsPerSample(output_.sampleFormat_) / 8;

    if (!getTraits(native_))
    {
      return false;
    }

    noteNativeTraitsChanged();

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    samplesDecoded_ = 0;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderShutdown
  //
  bool
  AudioTrack::decoderShutdown()
  {
    frameQueue_.close();
    packetQueue_.close();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decode
  //
  void
  AudioTrack::handle(const AvFrm & decodedFrame)
  {
    try
    {
      boost::this_thread::interruption_point();

      // assemble audio frame, piecewise:
      std::list<std::vector<unsigned char> > chunks;
      std::size_t outputBytes = 0;

      // shortcuts:
      enum AVSampleFormat outputFormat =
        yae_to_ffmpeg(output_.sampleFormat_, output_.channelFormat_);

      int64 outputChannelLayout =
        av_get_default_channel_layout(outputChannels_);

      // assemble output audio frame
      if (decodedFrame.nb_samples)
      {
        AvFrm decoded(decodedFrame);

        if (hasPrevPTS_ && decoded.pts != AV_NOPTS_VALUE)
        {
          // check for broken non-monotonically increasing timestamps:
          TTime nextPTS(stream_->time_base.num * decoded.pts,
                        stream_->time_base.den);

          if (nextPTS < prevPTS_)
          {
#ifndef NDEBUG
            std::cerr
              << "\nNOTE: non-monotonically increasing "
              << "audio timestamps detected:" << std::endl
              << "  prev = " << prevPTS_.to_hhmmss_usec(":") << std::endl
              << "  next = " << nextPTS.to_hhmmss_usec(":") << std::endl
              << std::endl;
#endif
            hasPrevPTS_ = false;
          }
        }

        if (!decoded.channel_layout)
        {
          decoded.channel_layout =
            av_get_default_channel_layout(decoded.channels);
        }

        const char * filterChain = NULL;
        bool frameTraitsChanged = false;
        if (!filterGraph_.setup(// input format:
                                stream_->time_base,
                                (enum AVSampleFormat)decoded.format,
                                decoded.sample_rate,
                                decoded.channel_layout,

                                // output format:
                                outputFormat,
                                output_.sampleRate_,
                                outputChannelLayout,

                                filterChain,
                                &frameTraitsChanged))
        {
          YAE_ASSERT(false);
          return;
        }

        if (frameTraitsChanged)
        {
          // detected a change in the number of audio channels,
          // or detected a change in audio sample rate,
          // prepare to remix or resample accordingly:
          if (!getTraits(native_))
          {
            return;
          }

          noteNativeTraitsChanged();
        }

        if (!filterGraph_.push(&decoded))
        {
          YAE_ASSERT(false);
          return;
        }

        while (true)
        {
          AvFrm output;
          if (!filterGraph_.pull(&output))
          {
            break;
          }

          const int bufferSize = output.nb_samples * outputBytesPerSample_;
          chunks.push_back(std::vector<unsigned char>
                           (output.data[0],
                            output.data[0] + bufferSize));
          outputBytes += bufferSize;
        }
      }

      if (!outputBytes)
      {
        return;
      }

      std::size_t numOutputSamples = outputBytes / outputBytesPerSample_;
      samplesDecoded_ += numOutputSamples;

      TAudioFramePtr afPtr(new TAudioFrame());
      TAudioFrame & af = *afPtr;

      af.traits_ = output_;
      af.time_.base_ = stream_->time_base.den;

      bool gotPTS = false;

      if (!gotPTS && decodedFrame.pts != AV_NOPTS_VALUE)
      {
        af.time_.time_ = stream_->time_base.num * decodedFrame.pts;
        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, af.time_, stream_,
                            "audio decodedFrame.pts");
      }

      if (!gotPTS)
      {
        af.time_.base_ = output_.sampleRate_;
        af.time_.time_ = samplesDecoded_ - numOutputSamples;
        af.time_ += TTime(startTime_, stream_->time_base.den);

        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, af.time_, stream_,
                            "audio t + t_start");
      }

      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_ += TTime(prevNumSamples_, output_.sampleRate_);

        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, af.time_, stream_,
                            "audio t_prev + n_prev");
      }

      YAE_ASSERT(gotPTS);
      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_.time_++;

        gotPTS = verify_pts(hasPrevPTS_, prevPTS_, af.time_, stream_,
                            "audio t++");
      }

      YAE_ASSERT(gotPTS);
      if (gotPTS)
      {
#ifndef NDEBUG
        if (hasPrevPTS_)
        {
          double ta = prevPTS_.toSeconds();
          double tb = af.time_.toSeconds();
          // std::cerr << "audio pts: " << tb << std::endl;
          double dt = tb - ta;
          // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
          if (dt > 0.67)
          {
            std::cerr
              << "\nNOTE: detected large audio PTS jump -- " << std::endl
              << dt << " seconds" << std::endl
              << std::endl;
          }
        }
#endif

        hasPrevPTS_ = true;
        prevPTS_ = af.time_;
        prevNumSamples_ = numOutputSamples;
      }

      // make sure the frame is in the in/out interval:
      if (playbackEnabled_)
      {
        double t = af.time_.toSeconds();
        double dt = double(numOutputSamples) / double(output_.sampleRate_);
        if (t > timeOut_ || (t + dt) < timeIn_)
        {
          if (t > timeOut_)
          {
            discarded_++;
          }

#if 0
          std::cerr << "discarding audio frame: " << t
                    << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                    << std::endl;
#endif
          return;
        }

        discarded_ = 0;
      }

      TPlanarBufferPtr sampleBuffer(new TPlanarBuffer(1),
                                    &IPlanarBuffer::deallocator);
      af.data_ = sampleBuffer;

      bool shouldAdjustTempo = true;
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);
        shouldAdjustTempo = tempoFilter_ && tempo_ != 1.0;
        af.tempo_ = shouldAdjustTempo ? tempo_ : 1.0;
      }

      if (!shouldAdjustTempo)
      {
        // concatenate chunks into a contiguous frame buffer:
        sampleBuffer->resize(0, outputBytes, 1, sizeof(double));
        unsigned char * afSampleBuffer = sampleBuffer->data(0);

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();
          memcpy(afSampleBuffer, chunk, chunkSize);

          afSampleBuffer += chunkSize;
          chunks.pop_front();
        }
      }
      else
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);

        // pass source samples through the tempo filter:
        std::size_t frameSize = 0;

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();

          const unsigned char * srcStart = chunk;
          const unsigned char * srcEnd = srcStart + chunkSize;
          const unsigned char * src = srcStart;

          while (src < srcEnd)
          {
            unsigned char * dstStart = &tempoBuffer_[0];
            unsigned char * dstEnd = dstStart + tempoBuffer_.size();
            unsigned char * dst = dstStart;
            tempoFilter_->apply(&src, srcEnd, &dst, dstEnd);

            std::size_t tmpSize = dst - dstStart;
            if (tmpSize)
            {
              sampleBuffer->resize(frameSize + tmpSize, sizeof(double));

              unsigned char * afSampleBuffer = sampleBuffer->data(0);
              memcpy(afSampleBuffer + frameSize, dstStart, tmpSize);
              frameSize += tmpSize;
            }
          }

          YAE_ASSERT(src == srcEnd);
          chunks.pop_front();
        }
      }

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      {
        std::string ts = to_hhmmss_usec(afPtr);
        std::cerr << "push audio frame: " << ts << std::endl;
      }
#endif

      // put the decoded frame into frame queue:
      if (!frameQueue_.push(afPtr, &terminator_))
      {
        return;
      }

      // std::cerr << "A: " << af.time_.toSeconds() << std::endl;
    }
    catch (...)
    {}

    return;
  }

  //----------------------------------------------------------------
  // AudioTrack::threadStop
  //
  bool
  AudioTrack::threadStop()
  {
    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // AudioTrack::noteNativeTraitsChanged
  //
  void
  AudioTrack::noteNativeTraitsChanged()
  {
    // reset the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      delete tempoFilter_;
      tempoFilter_ = NULL;
    }

    unsigned int bitsPerSample = getBitsPerSample(native_.sampleFormat_);
    nativeChannels_ = getNumberOfChannels(native_.channelLayout_);
    nativeBytesPerSample_ = (nativeChannels_ * bitsPerSample / 8);

    // initialize the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      YAE_ASSERT(!tempoFilter_);

      if ((output_.channelFormat_ != kAudioChannelsPlanar ||
           output_.channelLayout_ == kAudioMono))
      {
        if (output_.sampleFormat_ == kAudio8BitOffsetBinary)
        {
          tempoFilter_ = new TAudioTempoFilterU8();
        }
        else if (output_.sampleFormat_ == kAudio16BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI16();
        }
        else if (output_.sampleFormat_ == kAudio32BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI32();
        }
        else if (output_.sampleFormat_ == kAudio32BitFloat)
        {
          tempoFilter_ = new TAudioTempoFilterF32();
        }
        else if (output_.sampleFormat_ == kAudio64BitDouble)
        {
          tempoFilter_ = new TAudioTempoFilterF64();
        }

        if (tempoFilter_)
        {
          tempoFilter_->reset(output_.sampleRate_, outputChannels_);
          tempoFilter_->setTempo(tempo_);

          std::size_t fragmentSize = tempoFilter_->fragmentSize();
          tempoBuffer_.resize(fragmentSize * 3);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraits
  //
  bool
  AudioTrack::getTraits(AudioTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    const AVCodecParameters & codecParams = *(stream_->codecpar);
    AVSampleFormat sampleFormat = (AVSampleFormat)(codecParams.format);

    switch (sampleFormat)
    {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
        t.sampleFormat_ = kAudio8BitOffsetBinary;
        break;

      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio16BitBigEndian;
#else
        t.sampleFormat_ = kAudio16BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio32BitBigEndian;
#else
        t.sampleFormat_ = kAudio32BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        t.sampleFormat_ = kAudio32BitFloat;
        break;

      default:
        t.sampleFormat_ = kAudioInvalidFormat;
        break;
    }

    switch (codecParams.channels)
    {
      case 1:
        t.channelLayout_ = kAudioMono;
        break;

      case 2:
        t.channelLayout_ = kAudioStereo;
        break;

      case 3:
        t.channelLayout_ = kAudio2Pt1;
        break;

      case 4:
        t.channelLayout_ = kAudioQuad;
        break;

      case 5:
        t.channelLayout_ = kAudio4Pt1;
        break;

      case 6:
        t.channelLayout_ = kAudio5Pt1;
        break;

      case 7:
        t.channelLayout_ = kAudio6Pt1;
        break;

      case 8:
        t.channelLayout_ = kAudio7Pt1;
        break;

      default:
        t.channelLayout_ = kAudioChannelLayoutInvalid;
        break;
    }

    //! audio sample rate, Hz:
    t.sampleRate_ = codecParams.sample_rate;

    //! packed, planar:
    switch (sampleFormat)
    {
      case AV_SAMPLE_FMT_U8P:
      case AV_SAMPLE_FMT_S16P:
      case AV_SAMPLE_FMT_S32P:
      case AV_SAMPLE_FMT_FLTP:
        t.channelFormat_ = kAudioChannelsPlanar;
        break;

      default:
        t.channelFormat_ = kAudioChannelsPacked;
        break;
    }

    return
      t.sampleRate_ > 0 &&
      t.sampleFormat_ != kAudioInvalidFormat &&
      t.channelLayout_ != kAudioChannelLayoutInvalid;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTraitsOverride
  //
  bool
  AudioTrack::setTraitsOverride(const AudioTraits & override)
  {
    if (compare<AudioTraits>(override_, override) == 0)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(!alreadyDecoding);

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = override;

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraitsOverride
  //
  bool
  AudioTrack::getTraitsOverride(AudioTraits & override) const
  {
    override = override_;
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getNextFrame
  //
  bool
  AudioTrack::getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator)
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
      const AudioTraits & atraits = frame->traits_;

      unsigned int sampleSize = getBitsPerSample(atraits.sampleFormat_) / 8;
      YAE_ASSERT(sampleSize > 0);

      int channels = getNumberOfChannels(atraits.channelLayout_);
      YAE_ASSERT(channels > 0);

      std::size_t bytesPerSample = channels * sampleSize;
      std::size_t frameSize = frame->data_->rowBytes(0);
      std::size_t numSamples = bytesPerSample ? frameSize / bytesPerSample : 0;

      double t = frame->time_.toSeconds();
      double dt = double(numSamples) / double(atraits.sampleRate_);

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      static TTime prevTime(0, 1000);

      std::string in = TTime(timeIn_).to_hhmmss_usec(":");
      std::cerr << "\n\t\t\tAUDIO TIME IN:    " << in << std::endl;

      std::string ts = to_hhmmss_usec(frame);
      std::cerr << "\t\t\tPOP AUDIO frame:  " << ts << std::endl;

      std::string t0 = prevTime.to_hhmmss_usec(":");
      std::cerr << "\t\t\tPREV AUDIO frame: " << t0 << std::endl;
#endif

      if ((!playbackEnabled_ || t < timeOut_) && (t + dt) > timeIn_)
      {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::cerr << "\t\t\tNEXT AUDIO frame: " << ts << std::endl;
        prevTime = frame->time_;
#endif
        break;
      }
    }

    return ok;
  }

  //----------------------------------------------------------------
  // AudioTrack::setPlaybackInterval
  //
  void
  AudioTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
      std::string in = TTime(timeIn).to_hhmmss_usec(":");
      std::cerr
        << "SET AUDIO TRACK TIME IN: " << in
        << std::endl;
#endif

    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackEnabled_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // AudioTrack::resetTimeCounters
  //
  int
  AudioTrack::resetTimeCounters(double seekTime, bool dropPendingFrames)
  {
    packetQueue().clear();

    if (dropPendingFrames)
    {
      // NOTE: this drops any pending frames preventing their playback;
      // This is desirable when the user is seeking, but it prevents
      // proper in-out point playback because some frames will be dropped
      // when the video is rewound to the in-point:
      do { frameQueue_.clear(); }
      while (!packetQueue().waitForConsumerToBlock(1e-2));
      frameQueue_.clear();
    }

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    std::cerr
      << "\n\tAUDIO TRACK reset time counters, start new sequence\n"
      << std::endl;
#endif

    // drop filtergraph contents:
    filterGraph_.reset();

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
    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
    samplesDecoded_ = 0;

    return err;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTempo
  //
  bool
  AudioTrack::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);

    tempo_ = tempo;

    if (tempoFilter_ && !tempoFilter_->setTempo(tempo_))
    {
      return false;
    }

    if (tempo_ == 1.0 && tempoFilter_)
    {
      tempoFilter_->clear();
    }

    return true;
  }

}
