// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeTexturedRect.h"


namespace yae
{

  //----------------------------------------------------------------
  // TexturedRect::TexturedRect
  //
  TexturedRect::TexturedRect(const char * id, const Texture & texture):
    Item(id),
    texture_(texture)
  {}

  //----------------------------------------------------------------
  // TexturedRect::paintContent
  //
  void
  TexturedRect::paintContent() const
  {
    double u1 = 0.0;
    double v1 = 0.0;
    if (!texture_.bind(u1, v1))
    {
      return;
    }

    BBox bbox;
    this->get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = x0 + bbox.w_;
    double y1 = y0 + bbox.h_;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glTexCoord2d(0.0, 0.0));
      YAE_OGL_11(glVertex2d(x0, y0));

      YAE_OGL_11(glTexCoord2d(0.0, v1));
      YAE_OGL_11(glVertex2d(x0, y1));

      YAE_OGL_11(glTexCoord2d(u1, 0.0));
      YAE_OGL_11(glVertex2d(x1, y0));

      YAE_OGL_11(glTexCoord2d(u1, v1));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());

    texture_.unbind();
  }
}
