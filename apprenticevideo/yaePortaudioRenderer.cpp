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
#include "yaeAudioRendererInput.h"
#include "yaePortaudioRenderer.h"


namespace yae
{
  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate
  //
  class PortaudioRenderer::TPrivate
  {
  public:
    TPrivate(SharedClock & sharedClock);
    ~TPrivate();

    void match(const AudioTraits & source,
               AudioTraits & output) const;

    bool open(IReader * reader);
    void stop();
    void close();

  private:
    static bool openStream(const AudioTraits & atts,
                           double * outputLatency,
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
    void serviceTheCallback(void * output, unsigned long frameCount);

  public:
    // audio source:
    AudioRendererInput input_;

  private:
    // status code returned by Pa_Initialize:
    PaError initErr_;

    // output stream and its configuration:
    PaStream * output_;
    PaStreamParameters outputParams_;
  };

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::TPrivate
  //
  PortaudioRenderer::TPrivate::TPrivate(SharedClock & sharedClock):
    input_(sharedClock),
    initErr_(Pa_Initialize()),
    output_(NULL)
  {
    memset(&outputParams_, 0, sizeof(outputParams_));

    if (initErr_ != paNoError)
    {
      std::string err("Pa_Initialize failed: ");
      err += Pa_GetErrorText(initErr_);
      throw std::runtime_error(err);
    }
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::~TPrivate
  //
  PortaudioRenderer::TPrivate::~TPrivate()
  {
    if (initErr_ == paNoError)
    {
      Pa_Terminate();
    }
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::match
  //
  void
  PortaudioRenderer::TPrivate::match(const AudioTraits & srcAtts,
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

    if (output_)
    {
      YAE_ASSERT(!output_);
      return;
    }

    if (srcAtts.channelFormat_ == kAudioChannelsPlanar)
    {
      // packed sample format is preferred:
      outAtts.channelFormat_ = kAudioChannelsPacked;
    }

    const PaHostApiInfo * host = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(host->defaultOutputDevice);

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
      double outputLatency = 0.0;
      if (openStream(outAtts,
                     &outputLatency,
                     &testStream,
                     &testStreamParams,
                     NULL, // no callback, blocking mode
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
  // PortaudioRenderer::TPrivate::open
  //
  bool
  PortaudioRenderer::TPrivate::open(IReader * reader)
  {
    stop();

    AudioTraits atts;
    if (input_.open(reader) &&
        input_.reader_->getAudioTraitsOverride(atts))
    {
      return openStream(atts,
                        &input_.outputLatency_,
                        &output_,
                        &outputParams_,
                        &callback,
                        this);
    }

    return false;
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::stop
  //
  void
  PortaudioRenderer::TPrivate::stop()
  {
    input_.stop();

    if (output_)
    {
      Pa_StopStream(output_);
      Pa_CloseStream(output_);
      output_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::close
  //
  void
  PortaudioRenderer::TPrivate::close()
  {
    open(NULL);
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::openStream
  //
  bool
  PortaudioRenderer::TPrivate::
  openStream(const AudioTraits & atts,
             double * outputLatency,
             PaStream ** outputStream,
             PaStreamParameters * outputParams,
             PaStreamCallback streamCallback,
             void * userData)
  {
    if (atts.sampleFormat_ == kAudioInvalidFormat)
    {
      return false;
    }

    const PaHostApiInfo * host = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
    const PaDeviceInfo * devInfo = Pa_GetDeviceInfo(host->defaultOutputDevice);

    outputParams->suggestedLatency = devInfo->defaultHighOutputLatency;
    if (outputLatency)
    {
      *outputLatency = outputParams->suggestedLatency;
    }

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

    PaError errCode = Pa_OpenDefaultStream(outputStream,
                                           0,
                                           outputParams->channelCount,
                                           outputParams->sampleFormat,
                                           double(atts.sampleRate_),
                                           paFramesPerBufferUnspecified,
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
  // PortaudioRenderer::TPrivate::callback
  //
  int
  PortaudioRenderer::TPrivate::
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
      context->serviceTheCallback(output, samplesToRead);
      return paContinue;
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      std::cerr
        << "PortaudioRenderer::TPrivate::callback: "
        << "abort due to exception: " << e.what()
        << std::endl;
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      std::cerr
        << "PortaudioRenderer::TPrivate::callback: "
        << "abort due to unexpected exception"
        << std::endl;
#endif
    }

    return paContinue;
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::TPrivate::serviceTheCallback
  //
  void
  PortaudioRenderer::TPrivate::
  serviceTheCallback(void * output,
                     unsigned long samplesToRead /* per channel */)
  {
    bool dstPlanar = (outputParams_.sampleFormat & paNonInterleaved) != 0;
    input_.getData(output,
                   samplesToRead,
                   outputParams_.channelCount,
                   dstPlanar);
  }


  //----------------------------------------------------------------
  // PortaudioRenderer::PortaudioRenderer
  //
  PortaudioRenderer::PortaudioRenderer():
    private_(new TPrivate(ISynchronous::clock_))
  {}

  //----------------------------------------------------------------
  // PortaudioRenderer::~PortaudioRenderer
  //
  PortaudioRenderer::~PortaudioRenderer()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::create
  //
  PortaudioRenderer *
  PortaudioRenderer::create()
  {
    return new PortaudioRenderer();
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::destroy
  //
  void
  PortaudioRenderer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::getName
  //
  const char *
  PortaudioRenderer::getName() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::match
  //
  void
  PortaudioRenderer::match(const AudioTraits & source,
                                AudioTraits & output) const
  {
    return private_->match(source, output);
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::open
  //
  bool
  PortaudioRenderer::open(IReader * reader)
  {
    return private_->open(reader);
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::stop
  //
  void
  PortaudioRenderer::stop()
  {
    private_->stop();
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::close
  //
  void
  PortaudioRenderer::close()
  {
    private_->close();
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::pause
  //
  void
  PortaudioRenderer::pause(bool paused)
  {
    private_->input_.pause(paused);
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::skipToTime
  //
  void
  PortaudioRenderer::skipToTime(const TTime & t, IReader * reader)
  {
    private_->input_.skipToTime(t, reader);
  }

  //----------------------------------------------------------------
  // PortaudioRenderer::skipForward
  //
  void
  PortaudioRenderer::skipForward(const TTime & dt, IReader * reader)
  {
    private_->input_.skipForward(dt, reader);
  }
}
