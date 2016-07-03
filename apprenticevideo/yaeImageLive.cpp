// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan  1 17:16:07 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeCanvas.h"
#include "yaeImageLive.h"


namespace yae
{

  //----------------------------------------------------------------
  // ImageLive::ImageLive
  //
  ImageLive::ImageLive(const char * id):
    Item(id),
    canvas_(NULL),
    opacity_(ItemRef::constant(1.0))
  {}

  //----------------------------------------------------------------
  // ImageLive::uncache
  //
  void
  ImageLive::uncache()
  {
    opacity_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // ImageLive::paint
  //
  bool
  ImageLive::paint(const Segment & xregion,
                   const Segment & yregion,
                   Canvas * canvas) const
  {
    canvas_ = canvas;
    return Item::paint(xregion, yregion, canvas);
  }

  //----------------------------------------------------------------
  // ImageLive::paintContent
  //
  void
  ImageLive::paintContent() const
  {
    if (!canvas_)
    {
      return;
    }

    double x = this->left();
    double y = this->top();
    double w_max = this->width();
    double h_max = this->height();
    double opacity = opacity_.get();

    CanvasRenderer * renderer = canvas_->canvasRenderer();
    renderer->paintImage(x, y, w_max, h_max, opacity);

    CanvasRenderer * overlay = canvas_->overlayRenderer();
    if (overlay && overlay->pixelTraits() && canvas_->overlayHasContent())
    {
      overlay->paintImage(x, y, w_max, h_max, opacity);
    }
  }

}
