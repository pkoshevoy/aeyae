// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jul  8 13:10:51 MDT 2017
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

// Apple imports:
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreServices/CoreServices.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

// yae includes:
#include "yaeAudioRendererInput.h"
#include "yaeAudioUnitRenderer.h"


namespace yae
{
  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate
  //
  class AudioUnitRenderer::TPrivate
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
                           double & outputLatency,
                           AudioUnit & au,
                           AudioStreamBasicDescription & sd,
                           AURenderCallback cb,
                           void * userData);

    // AudioUnit stream callback:
    static OSStatus callback(// The client data that is provided either
                             // with the AURenderCallbackStruct
                             // or as specified with the Add API call
                             void * inRefCon,

                             // Flags used to describe more about the
                             // context of this call (pre or post in the
                             // notify case for instance)
                             AudioUnitRenderActionFlags * ioActionFlags,

                             // The times stamp associated with
                             // this call of audio unit render
                             const AudioTimeStamp * inTimeStamp,

                             // The bus numbeer associated with this call
                             // of audio unit render
                             UInt32 inBusNumber,

                             // The number of sample frames that will be
                             // represented in the audio data in the
                             // provided ioData parameter
                             UInt32 inNumberFrames,

                             // The AudioBufferList that will be used to
                             // contain the rendered or provided audio data
                             AudioBufferList * ioData);

    void au_render_cb(// Flags used to describe more about the
                      // context of this call (pre or post in the
                      // notify case for instance)
                      AudioUnitRenderActionFlags * ioActionFlags,

                      // The times stamp associated with
                      // this call of audio unit render
                      const AudioTimeStamp * inTimeStamp,

                      // The bus numbeer associated with this call
                      // of audio unit render
                      UInt32 inBusNumber,

                      // The number of sample frames that will be
                      // represented in the audio data in the
                      // provided ioData parameter
                      UInt32 inNumberFrames,

                      // The AudioBufferList that will be used to
                      // contain the rendered or provided audio data
                      AudioBufferList * ioData);

  public:
    // audio source:
    AudioRendererInput input_;

  private:
    AudioUnit au_;
    AudioStreamBasicDescription sd_;
  };

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::TPrivate
  //
  AudioUnitRenderer::TPrivate::TPrivate(SharedClock & sharedClock):
    input_(sharedClock)
  {
    memset(&au_, 0, sizeof(au_));
    memset(&sd_, 0, sizeof(sd_));
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::~TPrivate
  //
  AudioUnitRenderer::TPrivate::~TPrivate()
  {
    close();
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::match
  //
  void
  AudioUnitRenderer::TPrivate::match(const AudioTraits & srcAtts,
                                     AudioTraits & outAtts) const
  {
    if (&outAtts != &srcAtts)
    {
      outAtts = srcAtts;
    }

    outAtts.sampleFormat_ = kAudio32BitFloat;
    outAtts.channelFormat_ = kAudioChannelsPacked;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::open
  //
  bool
  AudioUnitRenderer::TPrivate::open(IReader * reader)
  {
    stop();

    AudioTraits atts;
    if (input_.open(reader) &&
        input_.reader_->getAudioTraitsOverride(atts))
    {
      return openStream(atts,
                        input_.outputLatency_,
                        au_,
                        sd_,
                        &callback,
                        this);
    }

    return false;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::stop
  //
  void
  AudioUnitRenderer::TPrivate::stop()
  {
    input_.stop();

    if (au_)
    {
      OSStatus err = noErr;

      err = AudioOutputUnitStop(au_);
      YAE_ASSERT(!err);

      err = AudioUnitReset(au_, kAudioUnitScope_Global, 0);
      YAE_ASSERT(!err);

      CloseComponent(au_);
      memset(&au_, 0, sizeof(au_));
      memset(&sd_, 0, sizeof(sd_));
    }
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::close
  //
  void
  AudioUnitRenderer::TPrivate::close()
  {
    open(NULL);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::openStream
  //
  bool
  AudioUnitRenderer::TPrivate::
  openStream(const AudioTraits & atts,
             double & outputLatency,
             AudioUnit & au,
             AudioStreamBasicDescription & sd,
             AURenderCallback cb,
             void * userData)
  {
    if (atts.sampleFormat_ == kAudioInvalidFormat)
    {
      return false;
    }

    OSStatus err = noErr;
    ComponentDescription desc;
    memset(&desc, 0, sizeof(desc));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    Component comp = FindNextComponent(NULL, &desc);

    memset(&au, 0, sizeof(au));

    err = OpenAComponent(comp, &au);
    if (err)
    {
      YAE_ASSERT(false);
      return false;
    }

    UInt32 value = kAudioConverterQuality_Max;
    err = AudioUnitSetProperty(au,
                               kAudioUnitProperty_RenderQuality,
                               kAudioUnitScope_Global,
                               0, // 0 == output
                               &value,
                               sizeof(value));
    YAE_ASSERT(!err);

    memset(&sd, 0, sizeof(sd));
    sd.mFramesPerPacket  = 1;
    sd.mBitsPerChannel   = sizeof(float) * 8;
    sd.mFormatFlags      = kAudioFormatFlagsNativeFloatPacked;
    sd.mFormatID         = kAudioFormatLinearPCM;
    sd.mSampleRate       = int(atts.sampleRate_);
    sd.mChannelsPerFrame = int(getNumberOfChannels(atts.channelLayout_));
    sd.mBytesPerPacket   = sizeof(float) * sd.mChannelsPerFrame;
    sd.mBytesPerFrame    = sizeof(float) * sd.mChannelsPerFrame;

    err = AudioUnitSetProperty(au,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0, // 0 == output
                               &sd,
                               sizeof(sd));
    if (err)
    {
      YAE_ASSERT(false);
      AudioUnitReset(au, kAudioUnitScope_Global, 0);
      CloseComponent(au);
      memset(&au, 0, sizeof(au));
      return false;
    }

    AURenderCallbackStruct rcb;
    memset(&rcb, 0, sizeof(rcb));
    rcb.inputProc = cb;
    rcb.inputProcRefCon = userData;

    err = AudioUnitSetProperty(au,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Output,
                               0, // 0 == output
                               &rcb,
                               sizeof(rcb));
    YAE_ASSERT(!err);

    err = AudioUnitInitialize(au);
    if (err)
    {
      YAE_ASSERT(false);
      AudioUnitReset(au, kAudioUnitScope_Global, 0);
      CloseComponent(au);
      memset(&au, 0, sizeof(au));
      return false;
    }

    // FIXME: is this a safe assumption?
    outputLatency = 16e-3;

    err = AudioOutputUnitStart(au);
    if (err)
    {
      YAE_ASSERT(false);
      AudioUnitReset(au, kAudioUnitScope_Global, 0);
      CloseComponent(au);
      memset(&au, 0, sizeof(au));
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::callback
  //
  OSStatus
  AudioUnitRenderer::TPrivate::
  callback(// The client data that is provided either
           // with the AURenderCallbackStruct
           // or as specified with the Add API call
           void * userData,

           // Flags used to describe more about the
           // context of this call (pre or post in the
           // notify case for instance)
           AudioUnitRenderActionFlags * ioActionFlags,

           // The times stamp associated with
           // this call of audio unit render
           const AudioTimeStamp * inTimeStamp,

           // The bus numbeer associated with this call
           // of audio unit render
           UInt32 inBusNumber,

           // The number of sample frames that will be
           // represented in the audio data in the
           // provided ioData parameter
           UInt32 inNumberFrames,

           // The AudioBufferList that will be used to
           // contain the rendered or provided audio data
           AudioBufferList * ioData)
  {
    TPrivate * context = (TPrivate *)userData;

    try
    {
      context->au_render_cb(ioActionFlags,
                            inTimeStamp,
                            inBusNumber,
                            inNumberFrames,
                            ioData);
      return noErr;
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      std::cerr
        << "AudioUnitRenderer::TPrivate::callback: "
        << "abort due to exception: " << e.what()
        << std::endl;
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      std::cerr
        << "AudioUnitRenderer::TPrivate::callback: "
        << "abort due to unexpected exception"
        << std::endl;
#endif
    }

    context->stop();
    return noErr;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::serviceTheCallback
  //
  void
  AudioUnitRenderer::TPrivate::
  au_render_cb(// Flags used to describe more about the
               // context of this call (pre or post in the
               // notify case for instance)
               AudioUnitRenderActionFlags * ioActionFlags,

               // The times stamp associated with
               // this call of audio unit render
               const AudioTimeStamp * inTimeStamp,

               // The bus numbeer associated with this call
               // of audio unit render
               UInt32 inBusNumber,

               // The number of sample frames that will be
               // represented in the audio data in the
               // provided ioData parameter
               UInt32 inNumberFrames,

               // The AudioBufferList that will be used to
               // contain the rendered or provided audio data
               AudioBufferList * ioData)
  {
    YAE_ASSERT(ioData->mNumberBuffers == 1);
    YAE_ASSERT(ioData->mBuffers[0].mNumberChannels == sd_.mChannelsPerFrame);

    bool dstPlanar = !(sd_.mFormatFlags & kAudioFormatFlagsNativeFloatPacked);
    input_.getData(ioData->mBuffers[0].mData,
                   inNumberFrames,
                   sd_.mChannelsPerFrame,
                   dstPlanar);
  }


  //----------------------------------------------------------------
  // AudioUnitRenderer::AudioUnitRenderer
  //
  AudioUnitRenderer::AudioUnitRenderer():
    private_(new TPrivate(ISynchronous::clock_))
  {}

  //----------------------------------------------------------------
  // AudioUnitRenderer::~AudioUnitRenderer
  //
  AudioUnitRenderer::~AudioUnitRenderer()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::create
  //
  AudioUnitRenderer *
  AudioUnitRenderer::create()
  {
    return new AudioUnitRenderer();
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::destroy
  //
  void
  AudioUnitRenderer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::getName
  //
  const char *
  AudioUnitRenderer::getName() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::match
  //
  void
  AudioUnitRenderer::match(const AudioTraits & source,
                                AudioTraits & output) const
  {
    return private_->match(source, output);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::open
  //
  bool
  AudioUnitRenderer::open(IReader * reader)
  {
    return private_->open(reader);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::stop
  //
  void
  AudioUnitRenderer::stop()
  {
    private_->stop();
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::close
  //
  void
  AudioUnitRenderer::close()
  {
    private_->close();
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::pause
  //
  void
  AudioUnitRenderer::pause(bool paused)
  {
    private_->input_.pause(paused);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::skipToTime
  //
  void
  AudioUnitRenderer::skipToTime(const TTime & t, IReader * reader)
  {
    private_->input_.skipToTime(t, reader);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::skipForward
  //
  void
  AudioUnitRenderer::skipForward(const TTime & dt, IReader * reader)
  {
    private_->input_.skipForward(dt, reader);
  }
}
