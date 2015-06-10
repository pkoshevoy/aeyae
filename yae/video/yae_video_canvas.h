// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 17:50:11 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_CANVAS_H_
#define YAE_VIDEO_CANVAS_H_

// yae includes:
#include "yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // IVideoCanvas
  //
  struct YAE_API IVideoCanvas
  {
    virtual ~IVideoCanvas() {}

    virtual bool render(const TVideoFramePtr & frame) = 0;
  };
}


#endif // YAE_VIDEO_CANVAS_H_
