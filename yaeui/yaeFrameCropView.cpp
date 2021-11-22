// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jun 27 20:23:31 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local includes:
#include "yaeFrameCropView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // FrameCropView::FrameCropView
  //
  FrameCropView::FrameCropView():
    ItemView("frameCrop"),
    mainView_(NULL)
  {}

  //----------------------------------------------------------------
  // FrameCropView::init
  //
  void
  FrameCropView::init(ItemView * mainView)
  {
    mainView_ = mainView;
    Item & root = *root_;
    root.children_.clear();

    frameCropItem_.reset(new FrameCropItem("FrameCropItem", *this));
    FrameCropItem & item = root.add<FrameCropItem>(frameCropItem_);
    item.anchors_.fill(root);
  }

  //----------------------------------------------------------------
  // FrameCropView::style
  //
  ItemViewStyle *
  FrameCropView::style() const
  {
    return mainView_ ? mainView_->style() : NULL;
  }

  //----------------------------------------------------------------
  // FrameCropView::resizeTo
  //
  bool
  FrameCropView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }

    if (mainView_)
    {
      ItemViewStyle * style = mainView_->style();
      requestUncache(style);
    }

    return true;
  }

  //----------------------------------------------------------------
  // FrameCropView::processMouseTracking
  //
  bool
  FrameCropView::processMouseTracking(const TVec2D & mousePt)
  {
    if (this->isEnabled() && frameCropItem_)
    {
      return frameCropItem_->processMouseTracking(mousePt);
    }

    return false;
  }

  //----------------------------------------------------------------
  // FrameCropView::processKeyEvent
  //
  bool
  FrameCropView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    if (this->isEnabled() && frameCropItem_)
    {
      return frameCropItem_->processKeyEvent(canvas, e);
    }

    return ItemView::processKeyEvent(canvas, e);
  }

}
