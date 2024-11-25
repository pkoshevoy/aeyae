// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jan  2 18:37:06 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUDIO_RENDERER_H_
#define YAE_AUDIO_RENDERER_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_audio_renderer_input.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"

// standard:
#include <string>


namespace yae
{

  //----------------------------------------------------------------
  // IAudioRenderer
  //
  struct YAE_API IAudioRenderer : public ISynchronous
  {
  protected:
    IAudioRenderer();
    virtual ~IAudioRenderer();

  public:

    //! The de/structor is intentionally hidden, use destroy() method instead.
    //! This is necessary in order to avoid conflicting memory manager
    //! problems that arise on windows when various libs are linked to
    //! different versions of runtime library.  Each library uses its own
    //! memory manager, so allocating in one library call and deallocating
    //! in another library will not work.  This can be avoided by hiding
    //! the standard constructor/destructor and providing an explicit
    //! interface for de/allocating an object instance, thus ensuring that
    //! the same memory manager will perform de/allocation.
    virtual void destroy() = 0;

    //! return a human readable name for this renderer (preferably unique):
    virtual const char * getName() const = 0;

    //! get default output device audio traits matched to source audio traits,
    //! however output device audio traits may not be exactly the same
    //! as the source traits due to hardware constraints:
    virtual void match(const AudioTraits & source,
                       AudioTraits & output) const = 0;

    //! initialize a given audio rendering device:
    virtual bool open(IReader * reader) = 0;

    //! terminate audio rendering:
    virtual void stop() = 0;

    //! terminate audio rendering, discard current reader:
    virtual void close() = 0;

    //! the initial state after open(...) must be paused;
    //! use this to resume or pause the rendering thread loop;
    virtual void pause(bool paused = true) = 0;

    //! shortcut:
    inline void resume()
    { pause(false); }

    //! this is used for single-frame stepping while playback is paused:
    virtual void skipToTime(const TTime & t, IReader * reader) = 0;
    virtual void skipForward(const TTime & dt, IReader * reader) = 0;

    //! accessors:
    virtual const AudioRendererInput & input() const = 0;
  };

  //----------------------------------------------------------------
  // TAudioRendererPtr
  //
  typedef yae::shared_ptr<IAudioRenderer, ISynchronous, yae::call_destroy>
  TAudioRendererPtr;

}


#endif // YAE_AUDIO_RENDERER_H_
