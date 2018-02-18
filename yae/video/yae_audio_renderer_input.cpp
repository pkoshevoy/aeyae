// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  5 22:01:08 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include "yae_audio_renderer_input.h"


//----------------------------------------------------------------
// YAE_DEBUG_AUDIO_RENDERER
//
#define YAE_DEBUG_AUDIO_RENDERER 0


namespace yae
{

  //----------------------------------------------------------------
  // AudioRendererInput::AudioRendererInput
  //
  AudioRendererInput::AudioRendererInput(SharedClock & sharedClock):
    reader_(NULL),
    sampleSize_(0),
    audioFrameOffset_(0),
    outputLatency_(0.0),
    clock_(sharedClock),
    pause_(true)
  {}

  //----------------------------------------------------------------
  // AudioRendererInput::open
  //
  bool
  AudioRendererInput::open(IReader * reader)
  {
    stop();

    pause_ = true;
    reader_ = reader;

    // avoid stale leftovers:
    audioFrame_ = TAudioFramePtr();
    audioFrameOffset_ = 0;
    sampleSize_ = 0;

    if (!reader_)
    {
      return false;
    }

    std::size_t selTrack = reader_->getSelectedAudioTrackIndex();
    std::size_t numTracks = reader_->getNumberOfAudioTracks();
    if (selTrack >= numTracks)
    {
      return false;
    }

    AudioTraits atts;
    if (!reader_->getAudioTraitsOverride(atts))
    {
      return false;
    }

    sampleSize_ = getBitsPerSample(atts.sampleFormat_) / 8;
    terminator_.stopWaiting(false);
    return true;
  }

  //----------------------------------------------------------------
  // AudioRendererInput::stop
  //
  void
  AudioRendererInput::stop()
  {
    pause_ = false;
    terminator_.stopWaiting(true);
  }

  //----------------------------------------------------------------
  // AudioRendererInput::close
  //
  void
  AudioRendererInput::close()
  {
    open(NULL);
  }

  //----------------------------------------------------------------
  // AudioRendererInput::pause
  //
  void
  AudioRendererInput::pause(bool paused)
  {
    pause_ = paused;
  }

  //----------------------------------------------------------------
  // AudioRendererInput::maybeReadOneFrame
  //
  void
  AudioRendererInput::maybeReadOneFrame(IReader * reader,
                                        TTime & framePosition)
  {
    while (!audioFrame_)
    {
      YAE_ASSERT(!audioFrameOffset_);

      // fetch the next audio frame from the reader:
      if (!reader->readAudio(audioFrame_, &terminator_))
      {
        if (clock_.allowsSettingTime())
        {
          clock_.noteTheClockHasStopped();
        }

        break;
      }

      if (resetTimeCountersIndicated(audioFrame_.get()))
      {
#if YAE_DEBUG_AUDIO_RENDERER
        std::cerr
          << "\nRESET AUDIO TIME COUNTERS\n"
          << std::endl;
#endif
        clock_.resetCurrentTime();
        audioFrame_.reset();
        continue;
      }
    }

    if (audioFrame_)
    {
      framePosition = audioFrame_->time_ +
        TTime(std::size_t(double(audioFrameOffset_) *
                          audioFrame_->tempo_ +
                          0.5),
              audioFrame_->traits_.sampleRate_);
    }
    else
    {
      framePosition = TTime();
    }
  }

  //----------------------------------------------------------------
  // AudioRendererInput::skipToTime
  //
  void
  AudioRendererInput::skipToTime(const TTime & t, IReader * reader)
  {
    terminator_.stopWaiting(true);

#if YAE_DEBUG_AUDIO_RENDERER
    std::cerr
      << "TRY TO SKIP AUDIO TO @ " << t
      << std::endl;
#endif

    TTime framePosition;
    do
    {
      if (audioFrame_)
      {
        unsigned int srcSampleSize =
          getBitsPerSample(audioFrame_->traits_.sampleFormat_) / 8;
        YAE_ASSERT(srcSampleSize > 0);

        int srcChannels =
          getNumberOfChannels(audioFrame_->traits_.channelLayout_);
        YAE_ASSERT(srcChannels > 0);

        std::size_t bytesPerSample = srcSampleSize * srcChannels;
        std::size_t srcFrameSize = audioFrame_->data_->rowBytes(0);
        std::size_t numSamples = (bytesPerSample ?
                                  srcFrameSize / bytesPerSample :
                                  0);
        unsigned int sampleRate = audioFrame_->traits_.sampleRate_;

        TTime frameDuration(numSamples * audioFrame_->tempo_, sampleRate);
        TTime frameEnd = audioFrame_->time_ + frameDuration;

        // check whether the frame is too far in the past:
        bool frameTooOld = frameEnd <= t;

        // check whether the frame is too far in the future:
        bool frameTooNew = t < audioFrame_->time_;

        if (!frameTooOld && !frameTooNew)
        {
          // calculate offset:
          audioFrameOffset_ =
            double((t - audioFrame_->time_).get(sampleRate)) /
            audioFrame_->tempo_;
          break;
        }

        // skip the entire frame:
        audioFrame_ = TAudioFramePtr();
        audioFrameOffset_ = 0;

        if (frameTooNew)
        {
          // avoid skipping too far ahead:
          break;
        }
      }

      maybeReadOneFrame(reader, framePosition);

    } while (audioFrame_);

    if (audioFrame_)
    {
#if YAE_DEBUG_AUDIO_RENDERER
      std::cerr
        << "SKIP AUDIO TO @ " << framePosition
        << std::endl;
#endif

      if (clock_.allowsSettingTime())
      {
#if YAE_DEBUG_AUDIO_RENDERER
        std::cerr
          << "AUDIO (s) SET CLOCK: " << framePosition
          << std::endl;
#endif
        clock_.setCurrentTime(framePosition, -0.016);
      }
    }
  }

  //----------------------------------------------------------------
  // AudioRendererInput::skipForward
  //
  void
  AudioRendererInput::skipForward(const TTime & dt, IReader * reader)
  {
    TTime framePosition;
    maybeReadOneFrame(reader, framePosition);
    if (audioFrame_)
    {
      framePosition += dt;
      skipToTime(framePosition, reader);
    }
  }


  //----------------------------------------------------------------
  // fillWithSilence
  //
  static void
  fillWithSilence(unsigned char ** dst,
                  bool dstPlanar,
                  std::size_t chunkSize,
                  int channels)
  {
    if (dstPlanar)
    {
      for (int i = 0; i < channels; i++)
      {
        memset(dst[i], 0, chunkSize);
      }
    }
    else
    {
      memset(dst[0], 0, chunkSize);
    }
  }

  //----------------------------------------------------------------
  // AudioRendererInput::getData
  //
  void
  AudioRendererInput::getData(void * output,
                              unsigned long samplesToRead, // per channel
                              int dstChannelCount,
                              bool dstPlanar)
  {
    static const double secondsToPause = 0.1;
    while (pause_)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds
                                    (long(secondsToPause * 1000.0)));
    }

    boost::this_thread::interruption_point();

    unsigned char * dstBuf = (unsigned char *)output;
    unsigned char ** dst = dstPlanar ? (unsigned char **)output : &dstBuf;

    std::size_t dstStride =
      dstPlanar ? sampleSize_ : sampleSize_ * dstChannelCount;

    std::size_t dstChunkSize = samplesToRead * dstStride;

    TTime framePosition;
    maybeReadOneFrame(reader_, framePosition);
    if (!audioFrame_)
    {
      fillWithSilence(dst,
                      dstPlanar,
                      dstChunkSize,
                      dstChannelCount);
      return;
    }

    unsigned int srcSampleSize =
      getBitsPerSample(audioFrame_->traits_.sampleFormat_) / 8;
    YAE_ASSERT(srcSampleSize > 0);

    int srcChannels =
      getNumberOfChannels(audioFrame_->traits_.channelLayout_);
    YAE_ASSERT(srcChannels > 0);

    unsigned int sampleRate = audioFrame_->traits_.sampleRate_;
    TTime frameDuration(samplesToRead, sampleRate);

    while (dstChunkSize)
    {
      maybeReadOneFrame(reader_, framePosition);
      if (!audioFrame_)
      {
        fillWithSilence(dst,
                        dstPlanar,
                        dstChunkSize,
                        dstChannelCount);
        return;
      }

      const AudioTraits & t = audioFrame_->traits_;

      srcSampleSize = getBitsPerSample(t.sampleFormat_) / 8;
      YAE_ASSERT(srcSampleSize > 0);

      srcChannels = getNumberOfChannels(t.channelLayout_);
      YAE_ASSERT(srcChannels > 0);

      bool srcPlanar = t.channelFormat_ == kAudioChannelsPlanar;

      bool detectedStaleFrame =
        (dstChannelCount != srcChannels ||
         sampleSize_ != srcSampleSize ||
         dstPlanar != srcPlanar);

      std::size_t srcStride =
        srcPlanar ? srcSampleSize : srcSampleSize * srcChannels;

      clock_.waitForOthers();

      if (detectedStaleFrame)
      {
#ifndef NDEBUG
        std::cerr
          << "expected " << dstChannelCount
          << " channels, received " << srcChannels
          << std::endl;
#endif

        audioFrame_ = TAudioFramePtr();
        audioFrameOffset_ = 0;

        std::size_t channelSize = samplesToRead * sampleSize_;
        if (dstPlanar)
        {
          for (int i = 0; i < dstChannelCount; i++)
          {
            memset(dst[i], 0, channelSize);
          }
        }
        else
        {
          memset(dst[0], 0, channelSize * dstChannelCount);
        }

        audioFrame_ = TAudioFramePtr();
        audioFrameOffset_ = 0;
        break;
      }
      else
      {
        const unsigned char * srcBuf = audioFrame_->data_->data(0);
        std::size_t srcFrameSize = audioFrame_->data_->rowBytes(0);
        std::size_t srcChunkSize = 0;

        std::vector<const unsigned char *> chunks;
        if (!srcPlanar)
        {
          std::size_t bytesAlreadyConsumed = audioFrameOffset_ * srcStride;

          chunks.resize(1);
          chunks[0] = srcBuf + bytesAlreadyConsumed;
          srcChunkSize = srcFrameSize - bytesAlreadyConsumed;
        }
        else
        {
          std::size_t channelSize = srcFrameSize / srcChannels;
          std::size_t bytesAlreadyConsumed = audioFrameOffset_ * srcSampleSize;
          srcChunkSize = channelSize - bytesAlreadyConsumed;

          chunks.resize(srcChannels);
          for (int i = 0; i < srcChannels; i++)
          {
            chunks[i] = srcBuf + i * channelSize + bytesAlreadyConsumed;
          }
        }

        // avoid buffer overrun:
        std::size_t chunkSize = std::min(srcChunkSize, dstChunkSize);

        const std::size_t numChunks = chunks.size();
        for (std::size_t i = 0; i < numChunks; i++)
        {
          memcpy(dst[i], chunks[i], chunkSize);
          dst[i] += chunkSize;
        }

        // decrement the output buffer chunk size:
        dstChunkSize -= chunkSize;

        if (chunkSize < srcChunkSize)
        {
          std::size_t samplesConsumed = srcStride ? chunkSize / srcStride : 0;
          audioFrameOffset_ += samplesConsumed;
        }
        else
        {
          // the entire frame was consumed, release it:
          audioFrame_ = TAudioFramePtr();
          audioFrameOffset_ = 0;
        }
      }
    }

    if (clock_.allowsSettingTime())
    {
#if YAE_DEBUG_AUDIO_RENDERER
      std::cerr
        << "AUDIO (c) SET CLOCK: " << framePosition
        << std::endl;
#endif
      clock_.setCurrentTime(framePosition, -0.016);
    }

    return;
  }

}
