// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 12:41:40 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_RENDERER_H_
#define YAE_VIDEO_RENDERER_H_

// system includes:
#include <string>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeSynchronous.h>
#include <yaeVideoCanvas.h>


namespace yae
{

  //----------------------------------------------------------------
  // VideoRenderer
  //
  struct YAE_API VideoRenderer : public ISynchronous
  {
  private:
    //! intentionally disabled:
    VideoRenderer(const VideoRenderer &);
    VideoRenderer & operator = (const VideoRenderer &);

    //! private implementation details:
    class TPrivate;
    TPrivate * private_;

  protected:
    VideoRenderer();
    ~VideoRenderer();

  public:
    static VideoRenderer * create();
    void destroy();

    //! begin rendering video frames onto a given canvas:
    bool open(IVideoCanvas * canvas,
              IReader * reader,
              bool forOneFrameOnly);

    //! terminate video rendering:
    void close();

    const TTime & skipToNextFrame();

    //! the initial state after open(...) must be paused;
    //! use this to resume or pause the rendering thread loop;
    void pause(bool paused = true);

    //! shortcut:
    inline void resume()
    { pause(false); }
  };
}


#endif // YAE_VIDEO_RENDERER_H_
