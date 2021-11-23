// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Mar  9 19:22:53 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeAspectRatioView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatioView::AspectRatioView
  //
  AspectRatioView::AspectRatioView(const char * name):
    ItemView(name),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // AspectRatioView::init
  //
  void
  AspectRatioView::init(ItemViewStyle * new_style,
                        const AspectRatio * options,
                        std::size_t num_options)
  {
    style_ = new_style;
    const ItemViewStyle & style = *style_;

    Item & root = *root_;
    root.children_.clear();

    aspectRatioItem_.reset(new AspectRatioItem("AspectRatioItem",
                                               *this,
                                               options,
                                               num_options));
    AspectRatioItem & aspectRatioItem =
      root.add<AspectRatioItem>(aspectRatioItem_);
    aspectRatioItem.anchors_.fill(root);
  }

  //----------------------------------------------------------------
  // AspectRatioView::processKeyEvent
  //
  bool
  AspectRatioView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    if (this->isEnabled() && aspectRatioItem_)
    {
      return aspectRatioItem_->processKeyEvent(canvas, e);
    }

    return ItemView::processKeyEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // AspectRatioView::setEnabled
  //
  void
  AspectRatioView::setEnabled(bool enable)
  {
    if (!style_ || !aspectRatioItem_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);

    aspectRatioItem_->setVisible(enable);
  }

}
