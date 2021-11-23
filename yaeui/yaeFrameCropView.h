// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jun 27 20:11:55 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FRAME_CROP_VIEW_H_
#define YAE_FRAME_CROP_VIEW_H_

// local interfaces:
#include "yaeFrameCropItem.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // FrameCropView
  //
  class YAEUI_API FrameCropView : public ItemView
  {
  public:
    FrameCropView(const char * name);

    void init(ItemView * mainView);

    // virtual:
    ItemViewStyle * style() const;

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // accessor:
    inline FrameCropItem * item() const
    { return frameCropItem_.get(); }

  protected:
    ItemView * mainView_;
    yae::shared_ptr<FrameCropItem, Item> frameCropItem_;
  };

}


#endif // YAE_FRAME_CROP_VIEW_H_
