// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 11 15:54:48 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUTO_CROP_H_
#define YAE_AUTO_CROP_H_

// aeyae:
#include "yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // TCropFrame
  //
  struct YAE_API TCropFrame
  {
    TCropFrame()
    {
      clear();
    }

    inline void clear()
    {
      x_ = 0;
      y_ = 0;
      w_ = 0;
      h_ = 0;
    }

    inline bool isEmpty() const
    {
      return !w_ || !h_;
    }

    inline double dar() const
    {
      return h_ ? double(w_) / double(h_) : 0.0;
    }

    int x_;
    int y_;
    int w_;
    int h_;
  };

  //----------------------------------------------------------------
  // TAutoCropCallback
  //
  // NOTE: The callback may return a non-NULL video frame to be fed
  // to the auto-crop detector.  This helps when video playback is
  // paused and frames are not delivered via setFrame as ususal.
  //
  typedef TVideoFramePtr(*TAutoCropCallback)(void * /* callback context */,
                                             const TCropFrame & /* results */,
                                             bool /* detection finished */);


  //----------------------------------------------------------------
  // TAutoCropDetect
  //
  struct YAE_API TAutoCropDetect
  {
    TAutoCropDetect();
    ~TAutoCropDetect();

    void reset(void * callbackContext, TAutoCropCallback callback);
    void setFrame(const TVideoFramePtr & frame);
    void threadLoop();
    void stop();

  private:
    struct TPrivate;
    TPrivate * private_;
  };

}


#endif // YAE_AUTO_CROP_H_
