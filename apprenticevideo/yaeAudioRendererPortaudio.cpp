// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  5 22:01:08 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
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
    
    bool open(unsigned int deviceIndex, IReader * reader);
    void close();
    
  private:
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
    
    // current audio frame:
    TAudioFramePtr audioFrame_;

    // number of samples already consumed from the current audio frame:
    std::size_t audioFrameOffset_;
    
    // maintain (os synchronize to) this clock:
    SharedClock & clock_;
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
    clock_(sharedClock)
  {
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
        defaultDevice_ = outputDevices_.size();
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
    return outputDevices_.size();
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
  // AudioRendererPortaudio::TPrivate::open
  // 
  bool
  AudioRendererPortaudio::TPrivate::open(unsigned int deviceIndex,
                                         IReader * reader)
  {
    if (output_)
    {
      Pa_StopStream(output_);
      Pa_CloseStream(output_);
      output_ = NULL;
    }
    
    // avoid leaving behind stale leftovers:
    audioFrame_ = TAudioFramePtr();
    audioFrameOffset_ = 0;
    
    reader_ = reader;
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
    
    outputParams_.device = outputDevices_[deviceIndex];
    
    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(outputParams_.device);
    outputParams_.suggestedLatency = devInfo->defaultHighOutputLatency;
    
    outputParams_.hostApiSpecificStreamInfo = NULL;
    outputParams_.channelCount = getNumberOfChannels(atts.channelLayout_);
    
    switch (atts.sampleFormat_)
    {
      case kAudio8BitOffsetBinary:
        outputParams_.sampleFormat = paUInt8;
        break;
        
      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        outputParams_.sampleFormat = paInt16;
        break;
        
      case kAudio24BitLittleEndian:
        outputParams_.sampleFormat = paInt24;
        break;
        
      case kAudio32BitFloat:
        outputParams_.sampleFormat = paFloat32;
        break;
        
      default:
        return false;
    }
    
    if (atts.channelFormat_ == kAudioChannelsPlanar)
    {
      outputParams_.sampleFormat |= paNonInterleaved;
    }
    
    PaError errCode = Pa_OpenStream(&output_,
                                    NULL,
                                    &outputParams_,
                                    double(atts.sampleRate_),
                                    paFramesPerBufferUnspecified,
                                    paNoFlag, // paClipOff,
                                    &callback,
                                    this);
    if (errCode != paNoError)
    {
      output_ = NULL;
      return false;
    }
    
    errCode = Pa_StartStream(output_);
    if (errCode != paNoError)
    {
      Pa_CloseStream(output_);
      output_ = NULL;
      return false;
    }
    
    return true;
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
    TPrivate * context = (TPrivate *)userData;
    return context->serviceTheCallback(input,
                                       output,
                                       samplesToRead,
                                       timeInfo,
                                       statusFlags);
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
    bool dstPlanar = (outputParams_.sampleFormat & paNonInterleaved) != 0;
    unsigned char * dstBuf = (unsigned char *)output;
    unsigned char ** dst = dstPlanar ? (unsigned char **)output : &dstBuf;
    
    if (!audioFrame_)
    {
      assert(!audioFrameOffset_);
      
      // fetch the next audio frame from the reader:
      if (!reader_->readAudio(audioFrame_))
      {
        return paComplete;
      }
    }
    
    unsigned int srcSampleSize =
      getBitsPerSample(audioFrame_->traits_.sampleFormat_) / 8;
    
    unsigned int srcChannels =
      getNumberOfChannels(audioFrame_->traits_.channelLayout_);
    
    bool srcPlanar =
      audioFrame_->traits_.channelFormat_ == kAudioChannelsPlanar;
    
    if (outputParams_.channelCount != srcChannels ||
        sampleSize_ != srcSampleSize ||
        dstPlanar != srcPlanar)
    {
      // detected stale leftovers:
      assert(false);
      audioFrame_ = TAudioFramePtr();
      audioFrameOffset_ = 0;
      return paContinue;
    }
    
    std::size_t srcStride =
      srcPlanar ? srcSampleSize : srcSampleSize * srcChannels;
    
    std::size_t dstChunkSize = samplesToRead * srcStride;
    
    unsigned int sampleRate = audioFrame_->traits_.sampleRate_;
    TTime frameDuration(samplesToRead, sampleRate);
    TTime framePosition(audioFrame_->time_);
    framePosition += TTime(audioFrameOffset_, sampleRate);
    
    while (dstChunkSize)
    {
      if (!audioFrame_)
      {
        assert(!audioFrameOffset_);
        
        // fetch the next audio frame from the reader:
        if (!reader_->readAudio(audioFrame_))
        {
          return paComplete;
        }
        
        const AudioTraits & t = audioFrame_->traits_;
        
        srcSampleSize = getBitsPerSample(t.sampleFormat_) / 8;
        srcChannels = getNumberOfChannels(t.channelLayout_);
        srcPlanar = t.channelFormat_ == kAudioChannelsPlanar;
        
        if (outputParams_.channelCount != srcChannels ||
            sampleSize_ != srcSampleSize ||
            dstPlanar != srcPlanar)
        {
          // detected stale leftovers:
          assert(false);
          audioFrame_ = TAudioFramePtr();
          audioFrameOffset_ = 0;
          return paContinue;
        }
        
        srcStride = srcPlanar ? srcSampleSize : srcSampleSize * srcChannels;
      }
      
      clock_.waitForOthers();
      
      const unsigned char * srcBuf = audioFrame_->sampleBuffer_->samples(0);
      std::size_t srcFrameSize = audioFrame_->sampleBuffer_->rowBytes(0);
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
    
    if (clock_.allowsSettingTime())
    {
      clock_.setCurrentTime(framePosition,
                            frameDuration,
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
  // AudioRendererPortaudio::open
  // 
  bool
  AudioRendererPortaudio::open(unsigned int deviceIndex,
                               IReader * reader)
  {
    return private_->open(deviceIndex, reader);
  }

  //----------------------------------------------------------------
  // AudioRendererPortaudio::close
  // 
  void
  AudioRendererPortaudio::close()
  {
    private_->close();
  }
}
