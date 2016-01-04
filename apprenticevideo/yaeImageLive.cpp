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
    canvas_(NULL)
  {}

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

    CanvasRenderer * renderer = canvas_->canvasRenderer();
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

    double x = this->left();
    double y = this->top();
    double w_max = this->width();
    double h_max = this->height();
    double w = w_max;
    double h = h_max;
    double car = w / h;
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

    renderer->draw();
    yae_assert_gl_no_error();
  }

}
