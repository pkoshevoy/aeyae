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

// portaudio includes:
#include <portaudio.h>

// yae includes:
#include <yaeAudioRendererPortaudio.h>

//----------------------------------------------------------------
// YAE_DEBUG_AUDIO_RENDERER
//
#define YAE_DEBUG_AUDIO_RENDERER 0


namespace yae
{
  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate
  //
  class AudioRendererPortaudio::TPrivate
  {
  public:
    TPrivate(SharedClock & sharedClock);
    ~TPrivate();

    unsigned int countAvailableDevices() const;
    unsigned int getDefaultDeviceIndex() const;

    bool getDeviceName(unsigned int deviceIndex,
                       std::string & deviceName) const;

    unsigned int getDeviceIndex(const std::string & audioDevice) const;

    void match(unsigned int deviceIndex,
               const AudioTraits & source,
               AudioTraits & output) const;

    bool open(unsigned int deviceIndex, IReader * reader);
    void stop();
    void close();

    void pause(bool pause);

    void maybeReadOneFrame(IReader * reader, TTime & framePosition);
    void skipToTime(const TTime & t, IReader * reader);
    void skipForward(const TTime & dt, IReader * reader);

  private:
    static bool openStream(PaDeviceIndex outputDevice,
                           const AudioTraits & atts,
                           PaStream ** outputStream,
                           PaStreamParameters * outputParams,
                           PaStreamCallback streamCallback,
                           void * userData);

    // portaudio stream callback:
    static int callback(const void * input,
                        void * output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo * timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void * userData);

    // a helper used by the portaudio stream callback:
    int serviceTheCallback(const void * input,
                           void * output,
                           unsigned long frameCount,
                           const PaStreamCallbackTimeInfo * timeInfo,
                           PaStreamCallbackFlags statusFlags);

    // status code returned by Pa_Initialize:
    PaError initErr_;

    // a mapping from output device enumeration to portaudio device index:
    std::vector<PaDeviceIndex> outputDevices_;
    unsigned int defaultDevice_;

    // audio source:
    IReader * reader_;

    // output stream and its configuration:
    PaStream * output_;
    PaStreamParameters outputParams_;
    unsigned int sampleSize_;

    // this is a tool for aborting a request for an audio frame
    // from the decoded frames queue, used to avoid a deadlock:
    QueueWaitMgr terminator_;

    // current audio frame:
    TAudioFramePtr audioFrame_;

    // number of samples already consumed from the current audio frame:
    std::size_t audioFrameOffset_;

    // maintain (or synchronize to) this clock:
    SharedClock & clock_;

    // a flag indicating whether the renderer should be paused:
    bool pause_;
  };

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::TPrivate
  //
  AudioRendererPortaudio::TPrivate::TPrivate(SharedClock & sharedClock):
    initErr_(Pa_Initialize()),
    defaultDevice_(0),
    reader_(NULL),
    output_(NULL),
    sampleSize_(0),
    audioFrameOffset_(0),
    clock_(sharedClock),
    pause_(true)
  {
    memset(&outputParams_, 0, sizeof(outputParams_));

    if (initErr_ != paNoError)
    {
      std::string err("Pa_Initialize did not succeed: ");
      err += Pa_GetErrorText(initErr_);
      throw std::runtime_error(err);
    }

    // enumerate available output devices:
    const PaDeviceIndex defaultDev = Pa_GetDefaultOutputDevice();
    const PaDeviceIndex nDevsTotal = Pa_GetDeviceCount();

    for (PaDeviceIndex i = 0; i < nDevsTotal; i++)
    {
      const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(i);
      if (devInfo->maxOutputChannels < 1)
      {
        continue;
      }

      if (defaultDev == i)
      {
        defaultDevice_ = (unsigned int)outputDevices_.size();
      }

      outputDevices_.push_back(i);
    }
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::~TPrivate
  //
  AudioRendererPortaudio::TPrivate::~TPrivate()
  {
    if (initErr_ == paNoError)
    {
      Pa_Terminate();
    }
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::countAvailableDevices
  //
  unsigned int
  AudioRendererPortaudio::TPrivate::countAvailableDevices() const
  {
    return (unsigned int)outputDevices_.size();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::getDefaultDeviceIndex
  //
  unsigned int
  AudioRendererPortaudio::TPrivate::getDefaultDeviceIndex() const
  {
    return defaultDevice_;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::getDeviceName
  //
  bool
  AudioRendererPortaudio::TPrivate::
  getDeviceName(unsigned int deviceIndex,
                std::string & deviceName) const
  {
    if (deviceIndex >= countAvailableDevices())
    {
      return false;
    }

    const PaDeviceIndex i = outputDevices_[deviceIndex];
    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(i);
    if (!devInfo)
    {
      return false;
    }

    const PaHostApiInfo * apiInfo = Pa_GetHostApiInfo(devInfo->hostApi);
    if (!apiInfo)
    {
      return false;
    }

    std::ostringstream oss;
    oss << apiInfo->name << ": " << devInfo->name;
    deviceName.assign(oss.str().c_str());

    return true;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::getDeviceIndex
  //
  unsigned int
  AudioRendererPortaudio::TPrivate::
  getDeviceIndex(const std::string & audioDevice) const
  {
    unsigned int numDevs = (unsigned int)outputDevices_.size();
    for (unsigned int i = 0; i < numDevs; i++)
    {
      std::string devName;
      if (getDeviceName(i, devName))
      {
        if (devName == audioDevice)
        {
          return i;
        }
      }
    }

    return numDevs;
  }

  //----------------------------------------------------------------
  // testCallback
  //
  static int testCallback(const void * input,
                          void * output,
                          unsigned long samplesToRead, // per channel
                          const PaStreamCallbackTimeInfo * timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void * userData)
  {
    return paAbort;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::match
  //
  void
  AudioRendererPortaudio::TPrivate::match(unsigned int deviceIndex,
                                          const AudioTraits & srcAtts,
                                          AudioTraits & outAtts) const
  {
    if (&outAtts != &srcAtts)
    {
      outAtts = srcAtts;
    }

    if (outAtts.sampleFormat_ == kAudioInvalidFormat)
    {
      return;
    }

    if (srcAtts.channelFormat_ == kAudioChannelsPlanar)
    {
      // packed sample format is preferred:
      outAtts.channelFormat_ = kAudioChannelsPacked;
    }

    PaDeviceIndex outputDevice = outputDevices_[deviceIndex];
    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(outputDevice);

    int sourceChannels = getNumberOfChannels(srcAtts.channelLayout_);
    if (devInfo->maxOutputChannels < sourceChannels)
    {
      outAtts.channelLayout_ = TAudioChannelLayout(devInfo->maxOutputChannels);
    }

    // test the configuration:
    PaStream * testStream = NULL;
    PaStreamParameters testStreamParams;

    while (true)
    {
      if (openStream(outputDevice,
                     outAtts,
                     &testStream,
                     &testStreamParams,
                     &testCallback,
                     NULL))
      {
        break;
      }

      if (outAtts.sampleRate_ != devInfo->defaultSampleRate)
      {
        outAtts.sampleRate_ = (unsigned int)(devInfo->defaultSampleRate);
      }
      else if (outAtts.channelLayout_ > kAudioStereo)
      {
        outAtts.channelLayout_ = kAudioStereo;
      }
      else if (outAtts.channelLayout_ > kAudioMono)
      {
        outAtts.channelLayout_ = kAudioMono;
      }
      else
      {
        outAtts.channelLayout_ = kAudioChannelLayoutInvalid;
        break;
      }
    }

    if (testStream)
    {
      Pa_StopStream(testStream);
      Pa_CloseStream(testStream);
      testStream = NULL;
    }
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::open
  //
  bool
  AudioRendererPortaudio::TPrivate::open(unsigned int deviceIndex,
                                         IReader * reader)
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
      return true;
    }

    std::size_t selTrack = reader_->getSelectedAudioTrackIndex();
    std::size_t numTracks = reader_->getNumberOfAudioTracks();
    if (selTrack >= numTracks)
    {
      return true;
    }

    AudioTraits atts;
    if (!reader_->getAudioTraitsOverride(atts))
    {
      return false;
    }

    sampleSize_ = getBitsPerSample(atts.sampleFormat_) / 8;

    PaDeviceIndex outputDevice = outputDevices_[deviceIndex];
    terminator_.stopWaiting(false);
    return openStream(outputDevice,
                      atts,
                      &output_,
                      &outputParams_,
                      &callback,
                      this);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::stop
  //
  void
  AudioRendererPortaudio::TPrivate::stop()
  {
    pause_ = false;
    terminator_.stopWaiting(true);

    if (output_)
    {
      Pa_StopStream(output_);
      Pa_CloseStream(output_);
      output_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::close
  //
  void
  AudioRendererPortaudio::TPrivate::close()
  {
    open(0, NULL);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::pause
  //
  void
  AudioRendererPortaudio::TPrivate::pause(bool paused)
  {
    pause_ = paused;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::maybeReadOneFrame
  //
  void
  AudioRendererPortaudio::TPrivate::maybeReadOneFrame(IReader * reader,
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
  // AudioRendererPortaudio::TPrivate::skipToTime
  //
  void
  AudioRendererPortaudio::TPrivate::skipToTime(const TTime & t,
                                               IReader * reader)
  {
    terminator_.stopWaiting(true);

#if YAE_DEBUG_AUDIO_RENDERER
    std::cerr
      << "TRY TO SKIP AUDIO TO @ " << t.to_hhmmss_usec(":")
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

        std::size_t srcFrameSize = audioFrame_->data_->rowBytes(0);
        std::size_t numSamples = srcFrameSize / (srcSampleSize * srcChannels);
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
            double((t - audioFrame_->time_).getTime(sampleRate)) /
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
        << "SKIP AUDIO TO @ " << framePosition.to_hhmmss_usec(":")
        << std::endl;
#endif

      if (clock_.allowsSettingTime())
      {
#if YAE_DEBUG_AUDIO_RENDERER
        std::cerr
          << "AUDIO (s) SET CLOCK: " << framePosition.to_hhmmss_usec(":")
          << std::endl;
#endif
        clock_.setCurrentTime(framePosition,
                              outputParams_.suggestedLatency);
      }
    }
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::skipForward
  //
  void
  AudioRendererPortaudio::TPrivate::skipForward(const TTime & dt,
                                                IReader * reader)
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
  // AudioRendererPortaudio::TPrivate::openStream
  //
  bool
  AudioRendererPortaudio::TPrivate::
  openStream(PaDeviceIndex outputDevice,
             const AudioTraits & atts,
             PaStream ** outputStream,
             PaStreamParameters * outputParams,
             PaStreamCallback streamCallback,
             void * userData)
  {
    if (atts.sampleFormat_ == kAudioInvalidFormat)
    {
      return false;
    }

    outputParams->device = outputDevice;

    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(outputParams->device);
    outputParams->suggestedLatency = devInfo->defaultHighOutputLatency;

    outputParams->hostApiSpecificStreamInfo = NULL;
    outputParams->channelCount = getNumberOfChannels(atts.channelLayout_);

    switch (atts.sampleFormat_)
    {
      case kAudio8BitOffsetBinary:
        outputParams->sampleFormat = paUInt8;
        break;

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        outputParams->sampleFormat = paInt16;
        break;

      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        outputParams->sampleFormat = paInt32;
        break;

      case kAudio24BitLittleEndian:
        outputParams->sampleFormat = paInt24;
        break;

      case kAudio32BitFloat:
        outputParams->sampleFormat = paFloat32;
        break;

      default:
        return false;
    }

    if (atts.channelFormat_ == kAudioChannelsPlanar)
    {
      outputParams->sampleFormat |= paNonInterleaved;
    }

    PaError errCode = Pa_OpenStream(outputStream,
                                    NULL,
                                    outputParams,
                                    double(atts.sampleRate_),
                                    paFramesPerBufferUnspecified,
                                    paNoFlag, // paClipOff,
                                    streamCallback,
                                    userData);
    if (errCode != paNoError)
    {
      *outputStream = NULL;
      return false;
    }

    errCode = Pa_StartStream(*outputStream);
    if (errCode != paNoError)
    {
      Pa_CloseStream(*outputStream);
      *outputStream = NULL;
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::TPrivate::callback
  //
  int
  AudioRendererPortaudio::TPrivate::
  callback(const void * input,
           void * output,
           unsigned long samplesToRead, // per channel
           const PaStreamCallbackTimeInfo * timeInfo,
           PaStreamCallbackFlags statusFlags,
           void * userData)
  {
    try
    {
      TPrivate * context = (TPrivate *)userData;
      return context->serviceTheCallback(input,
                                         output,
                                         samplesToRead,
                                         timeInfo,
                                         statusFlags);
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      std::cerr
        << "AudioRendererPortaudio::TPrivate::callback: "
        << "abort due to exception: " << e.what()
        << std::endl;
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      std::cerr
        << "AudioRendererPortaudio::TPrivate::callback: "
        << "abort due to unexpected exception"
        << std::endl;
#endif
    }

    return paContinue;
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
  // AudioRendererPortaudio::TPrivate::serviceTheCallback
  //
  int
  AudioRendererPortaudio::TPrivate::
  serviceTheCallback(const void * /* input */,
                     void * output,
                     unsigned long samplesToRead, // per channel
                     const PaStreamCallbackTimeInfo * timeInfo,
                     PaStreamCallbackFlags /* statusFlags */)
  {
    static const double secondsToPause = 0.1;
    while (pause_ && !boost::this_thread::interruption_requested())
    {
      boost::this_thread::disable_interruption here;
      boost::this_thread::sleep(boost::posix_time::milliseconds
                                (long(secondsToPause * 1000.0)));
    }

    boost::this_thread::interruption_point();

    bool dstPlanar = (outputParams_.sampleFormat & paNonInterleaved) != 0;
    unsigned char * dstBuf = (unsigned char *)output;
    unsigned char ** dst = dstPlanar ? (unsigned char **)output : &dstBuf;

    std::size_t dstStride =
      dstPlanar ? sampleSize_ : sampleSize_ * outputParams_.channelCount;

    std::size_t dstChunkSize = samplesToRead * dstStride;

    TTime framePosition;
    maybeReadOneFrame(reader_, framePosition);
    if (!audioFrame_)
    {
      fillWithSilence(dst,
                      dstPlanar,
                      dstChunkSize,
                      outputParams_.channelCount);
      return paContinue;
    }

    unsigned int srcSampleSize =
      getBitsPerSample(audioFrame_->traits_.sampleFormat_) / 8;
    YAE_ASSERT(srcSampleSize > 0);

    int srcChannels =
      getNumberOfChannels(audioFrame_->traits_.channelLayout_);
    YAE_ASSERT(srcChannels > 0);

    bool srcPlanar =
      audioFrame_->traits_.channelFormat_ == kAudioChannelsPlanar;

    unsigned int sampleRate = audioFrame_->traits_.sampleRate_;
    TTime frameDuration(samplesToRead, sampleRate);

    bool detectedStaleFrame =
      (outputParams_.channelCount != srcChannels ||
       sampleSize_ != srcSampleSize ||
       dstPlanar != srcPlanar);

    std::size_t srcStride =
      srcPlanar ? srcSampleSize : srcSampleSize * srcChannels;

    while (dstChunkSize)
    {
      maybeReadOneFrame(reader_, framePosition);
      if (!audioFrame_)
      {
        fillWithSilence(dst,
                        dstPlanar,
                        dstChunkSize,
                        outputParams_.channelCount);
        return paContinue;
      }

      const AudioTraits & t = audioFrame_->traits_;

      srcSampleSize = getBitsPerSample(t.sampleFormat_) / 8;
      YAE_ASSERT(srcSampleSize > 0);

      srcChannels = getNumberOfChannels(t.channelLayout_);
      YAE_ASSERT(srcChannels > 0);

      srcPlanar = t.channelFormat_ == kAudioChannelsPlanar;

      detectedStaleFrame =
        (outputParams_.channelCount != srcChannels ||
         sampleSize_ != srcSampleSize ||
         dstPlanar != srcPlanar);

      srcStride = srcPlanar ? srcSampleSize : srcSampleSize * srcChannels;

      clock_.waitForOthers();

      if (detectedStaleFrame)
      {
#ifndef NDEBUG
        std::cerr
          << "expected " << outputParams_.channelCount
          << " channels, received " << srcChannels
          << std::endl;
#endif

        audioFrame_ = TAudioFramePtr();
        audioFrameOffset_ = 0;

        std::size_t channelSize = samplesToRead * sampleSize_;
        if (dstPlanar)
        {
          for (int i = 0; i < outputParams_.channelCount; i++)
          {
            memset(dst[i], 0, channelSize);
          }
        }
        else
        {
          memset(dst[0], 0, channelSize * outputParams_.channelCount);
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
          std::size_t samplesConsumed = chunkSize / srcStride;
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
        << "AUDIO (c) SET CLOCK: " << framePosition.to_hhmmss_usec(":")
        << std::endl;
#endif
      clock_.setCurrentTime(framePosition,
                            outputParams_.suggestedLatency);
    }

    return paContinue;
  }


  //----------------------------------------------------------------
  // AudioRendererPortaudio::AudioRendererPortaudio
  //
  AudioRendererPortaudio::AudioRendererPortaudio():
    private_(new TPrivate(ISynchronous::clock_))
  {}

  //----------------------------------------------------------------
  // AudioRendererPortaudio::~AudioRendererPortaudio
  //
  AudioRendererPortaudio::~AudioRendererPortaudio()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::create
  //
  AudioRendererPortaudio *
  AudioRendererPortaudio::create()
  {
    return new AudioRendererPortaudio();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::destroy
  //
  void
  AudioRendererPortaudio::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::getName
  //
  const char *
  AudioRendererPortaudio::getName() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::countAvailableDevices
  //
  unsigned int
  AudioRendererPortaudio::countAvailableDevices() const
  {
    return private_->countAvailableDevices();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::getDefaultDeviceIndex
  //
  unsigned int
  AudioRendererPortaudio::getDefaultDeviceIndex() const
  {
    return private_->getDefaultDeviceIndex();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::getDeviceName
  //
  bool
  AudioRendererPortaudio::getDeviceName(unsigned int deviceIndex,
                                        std::string & deviceName) const
  {
    return private_->getDeviceName(deviceIndex, deviceName);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::getDeviceIndex
  //
  unsigned int
  AudioRendererPortaudio::getDeviceIndex(const std::string & devName) const
  {
    return private_->getDeviceIndex(devName);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::match
  //
  void
  AudioRendererPortaudio::match(unsigned int deviceIndex,
                                const AudioTraits & source,
                                AudioTraits & output) const
  {
    return private_->match(deviceIndex, source, output);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::open
  //
  bool
  AudioRendererPortaudio::open(unsigned int deviceIndex, IReader * reader)
  {
    return private_->open(deviceIndex, reader);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::stop
  //
  void
  AudioRendererPortaudio::stop()
  {
    private_->stop();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::close
  //
  void
  AudioRendererPortaudio::close()
  {
    private_->close();
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::pause
  //
  void
  AudioRendererPortaudio::pause(bool paused)
  {
    private_->pause(paused);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::skipToTime
  //
  void
  AudioRendererPortaudio::skipToTime(const TTime & t, IReader * reader)
  {
    private_->skipToTime(t, reader);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::skipForward
  //
  void
  AudioRendererPortaudio::skipForward(const TTime & dt, IReader * reader)
  {
    private_->skipForward(dt, reader);
  }
}
