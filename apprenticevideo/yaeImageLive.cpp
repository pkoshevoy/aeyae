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
  // paintImage
  //
  static void
  paintImage(CanvasRenderer * renderer,
             double opacity,
             double x,
             double y,
             double w_max,
             double h_max)
  {
    double croppedWidth = 0.0;
    double croppedHeight = 0.0;
    int cameraRotation = 0;
    renderer->imageWidthHeightRotated(croppedWidth,
                                      croppedHeight,
                                      cameraRotation);
    if (!croppedWidth || !croppedHeight)
    {
      return;
    }

    double w = w_max;
    double h = h_max;
    double car = w_max / h_max;
    double dar = croppedWidth / croppedHeight;

    if (dar < car)
    {
      w = h_max * dar;
      x += 0.5 * (w_max - w);
    }
    else
    {
      h = w_max / dar;
      y += 0.5 * (h_max - h);
    }

    TGLSaveMatrixState pushViewMatrix(GL_MODELVIEW);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(x, y, 0.0));
    YAE_OGL_11(glScaled(w / croppedWidth, h / croppedHeight, 1.0));

    if (cameraRotation && cameraRotation % 90 == 0)
    {
      YAE_OGL_11(glTranslated(0.5 * croppedWidth, 0.5 * croppedHeight, 0));
      YAE_OGL_11(glRotated(double(cameraRotation), 0, 0, 1));

      if (cameraRotation % 180 != 0)
      {
        YAE_OGL_11(glTranslated(-0.5 * croppedHeight,
                                -0.5 * croppedWidth, 0));
      }
      else
      {
        YAE_OGL_11(glTranslated(-0.5 * croppedWidth,
                                -0.5 * croppedHeight, 0));
      }
    }

    renderer->draw(opacity);
    yae_assert_gl_no_error();
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
    paintImage(renderer, opacity, x, y, w_max, h_max);

    CanvasRenderer * overlay = canvas_->overlayRenderer();
    if (overlay && overlay->pixelTraits() && canvas_->overlayHasContent())
    {
      paintImage(overlay, opacity, x, y, w_max, h_max);
    }
  }

}
