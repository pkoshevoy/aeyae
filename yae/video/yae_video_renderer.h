// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 12:41:40 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_RENDERER_H_
#define YAE_VIDEO_RENDERER_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"
#include "yae/video/yae_video_canvas.h"

// standard:
#include <string>


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
    bool open(IVideoCanvas * canvas, IReader * reader);

    //! stop the renderer thread but keep current canvas and reader:
    void stop();

    //! stop the renderer thread and discard current canvas and reader:
    void close();

    //! return the time of currently rendered frame, if any:
    TTime getCurrentTime() const;

    //! try to skip to next frame
    //! if successful pass-back the frame position and return true
    //! otherwise return false.
    //!
    //! NOTE: this function will not block waiting for the next frame,
    //! instead it will return false.
    bool skipToNextFrame(TTime & framePosition);

    //! the initial state after open(...) must be paused;
    //! use this to resume or pause the rendering thread loop;
    void pause(bool paused = true);

    //! check whether the renderer is paused:
    bool isPaused() const;

    //! shortcut:
    inline void resume()
    { pause(false); }
  };

  //----------------------------------------------------------------
  // TVideoRendererPtr
  //
  typedef yae::shared_ptr<VideoRenderer, ISynchronous, yae::call_destroy>
  TVideoRendererPtr;
}


#endif // YAE_VIDEO_RENDERER_H_
