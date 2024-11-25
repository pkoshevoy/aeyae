// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jul  8 13:10:51 MDT 2017
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIOUNIT_RENDERER_H_
#define YAE_AUDIOUNIT_RENDERER_H_


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#ifdef __APPLE__

  //----------------------------------------------------------------
  // *TAuCtxPull
  //
  typedef bool(*TAuCtxPull)(void *, // owner
                            void *, // data
                            unsigned long, // samples to read
                            int, // channel count
                            bool); // planar

  //----------------------------------------------------------------
  // *TAuCtxStop
  //
  typedef void(*TAuCtxStop)(void *); // owner

  //----------------------------------------------------------------
  // yae_au_ctx_create
  //
  void *
  yae_au_ctx_create(void * owner,
                    TAuCtxPull pull,
                    TAuCtxStop stop);

  //----------------------------------------------------------------
  // yae_au_ctx_destroy
  //
  void
  yae_au_ctx_destroy(void ** context);

  //----------------------------------------------------------------
  // yae_au_ctx_stop
  //
  void
  yae_au_ctx_stop(void * context);

  //----------------------------------------------------------------
  // yae_au_ctx_open_stream
  //
  bool
  yae_au_ctx_open_stream(void * context,
                         int sample_rate,
                         int num_channels);
#endif // __APPLE__
#ifdef __cplusplus
}
#endif // __cplusplus


#ifdef __cplusplus
// aeyae:
#include "yae/video/yae_audio_renderer.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// standard:
#include <string>


namespace yae
{
  //----------------------------------------------------------------
  // AudioRenderer
  //
  struct YAEUI_API AudioUnitRenderer : public IAudioRenderer
  {
  private:
    //! intentionally disabled:
    AudioUnitRenderer(const AudioUnitRenderer &);
    AudioUnitRenderer & operator = (const AudioUnitRenderer &);

    //! private implementation details:
    class TPrivate;
    TPrivate * private_;

  protected:
    AudioUnitRenderer();
    ~AudioUnitRenderer();

  public:
    static AudioUnitRenderer * create();
    virtual void destroy();

    //! return a human readable name for this renderer (preferably unique):
    virtual const char * getName() const;

    //! get output device audio traits matched to source audio traits,
    //! however output device audio traits may not be exactly the same
    //! as the source traits due to hardware constraints:
    virtual void match(const AudioTraits & source,
                       AudioTraits & output) const;

    //! begin rendering audio frames from a given reader:
    virtual bool open(IReader * reader);

    //! terminate audio rendering:
    virtual void stop();

    //! terminate audio rendering, discard current reader:
    virtual void close();

    //! the initial state after open(...) must be paused;
    //! use this to resume or pause the rendering thread loop;
    virtual void pause(bool paused);

    //! this is used for single-frame stepping while playback is paused:
    virtual void skipToTime(const TTime & t, IReader * reader);
    virtual void skipForward(const TTime & dt, IReader * reader);

    //! accessors:
    virtual const AudioRendererInput & input() const;
  };
}

#endif // __cplusplus
#endif // YAE_AUDIOUNIT_RENDERER_H_
