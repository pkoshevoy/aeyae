// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jul  8 13:10:51 MDT 2017
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <stdexcept>

// aeyae:
#include "yae/video/yae_audio_renderer_input.h"

// yaeui:
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
    static bool pull_cb(void * context, // this
                        void * data,
                        unsigned long samples_to_read,
                        int channel_count,
                        bool planar);

    static void stop_cb(void * context); // this

    // opaque:
    void * yae_au_ctx_;

  public:
    // protect against concurrent access:
    mutable boost::mutex mutex_;

    // audio source:
    AudioRendererInput input_;
  };

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::TPrivate
  //
  AudioUnitRenderer::TPrivate::TPrivate(SharedClock & sharedClock):
    input_(sharedClock)
  {
    yae_au_ctx_ = yae_au_ctx_create(this, &pull_cb, &stop_cb);
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::~TPrivate
  //
  AudioUnitRenderer::TPrivate::~TPrivate()
  {
    close();
    yae_au_ctx_destroy(&yae_au_ctx_);
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

    boost::unique_lock<boost::mutex> lock(mutex_);
#ifndef NDEBUG
    yae_debug << "AudioUnitRenderer::TPrivate::open " << reader;
#endif

    AudioTraits atts;
    if (input_.open(reader) &&
        input_.reader_->getAudioTraitsOverride(atts))
    {
      if (atts.sampleFormat_ == kAudioInvalidFormat)
      {
        return false;
      }

      int sample_rate = int(atts.sampleRate_);
      int num_channels = int(getNumberOfChannels(atts.channelLayout_));
      if (yae_au_ctx_open_stream(yae_au_ctx_, sample_rate, num_channels))
      {
        // FIXME: is this a safe assumption?
        input_.outputLatency_ = 16e-3;
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::stop
  //
  void
  AudioUnitRenderer::TPrivate::stop()
  {
#ifndef NDEBUG
    yae_debug << "AudioUnitRenderer::TPrivate::stop";
#endif
    input_.stop();

    boost::unique_lock<boost::mutex> lock(mutex_);
    yae_au_ctx_stop(yae_au_ctx_);
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
  // AudioUnitRenderer::TPrivate::pull_cb
  //
  bool
  AudioUnitRenderer::TPrivate::pull_cb(void * context,
                                       void * data,
                                       unsigned long samples_to_read,
                                       int channel_count,
                                       bool planar)
  {
    TPrivate * renderer = (TPrivate *)context;
    try
    {
      boost::unique_lock<boost::mutex> lock(renderer->mutex_);
      renderer->input_.getData(data,
                               samples_to_read,
                               channel_count,
                               planar);
      return true;
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      yae_error
        << "AudioUnitRenderer::TPrivate::callback: "
        << "abort due to exception: " << e.what();
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      yae_error
        << "AudioUnitRenderer::TPrivate::callback: "
        << "abort due to unexpected exception";
#endif
    }

    return false;
  }

  //----------------------------------------------------------------
  // AudioUnitRenderer::TPrivate::stop_cb
  //
  void
  AudioUnitRenderer::TPrivate::stop_cb(void * context)
  {
#ifndef NDEBUG
    yae_debug << "AudioUnitRenderer::TPrivate::stop_cb";
#endif
    TPrivate * renderer = (TPrivate *)context;
    renderer->stop();
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
#ifndef NDEBUG
    yae_debug << "AudioUnitRenderer::pause " << (paused ? "true" : "false");
#endif
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
