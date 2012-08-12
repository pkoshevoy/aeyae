// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 11 15:54:48 MDT 2012
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_AUTO_CROP_H_
#define YAE_AUTO_CROP_H_

// yae includes:
#include <yaeAPI.h>
#include <yaePixelFormatTraits.h>
#include <yaeThreading.h>


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
  typedef void(*TAutoCropCallback)(void *, const TCropFrame &);


  //----------------------------------------------------------------
  // TAutoCropDetect
  //
  struct YAE_API TAutoCropDetect
  {
    TAutoCropDetect();

    void reset(void * callbackContext, TAutoCropCallback callback);
    void setFrame(const TVideoFramePtr & frame);
    void threadLoop();
    void stop();

    mutable boost::mutex mutex_;
    mutable boost::condition_variable cond_;
    void * callbackContext_;
    TAutoCropCallback callback_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    bool done_;
  };

}


#endif // YAE_AUTO_CROP_H_
