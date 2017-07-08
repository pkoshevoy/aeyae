// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  5 21:57:57 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_PORTAUDIO_H_
#define YAE_AUDIO_RENDERER_PORTAUDIO_H_

// system includes:
#include <string>

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_audio_renderer.h"
#include "yae/video/yae_reader.h"


namespace yae
{
  //----------------------------------------------------------------
  // AudioRenderer
  //
  struct YAE_API AudioRendererPortaudio : public IAudioRenderer
  {
  private:
    //! intentionally disabled:
    AudioRendererPortaudio(const AudioRendererPortaudio &);
    AudioRendererPortaudio & operator = (const AudioRendererPortaudio &);

    //! private implementation details:
    class TPrivate;
    TPrivate * private_;

  protected:
    AudioRendererPortaudio();
    ~AudioRendererPortaudio();

  public:
    static AudioRendererPortaudio * create();
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
  };
}


#endif // YAE_AUDIO_RENDERER_PORTAUDIO_H_
