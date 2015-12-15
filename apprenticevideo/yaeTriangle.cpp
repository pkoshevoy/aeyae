// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QImage>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeTriangle.h"


namespace yae
{

  //----------------------------------------------------------------
  // Triangle::Triangle
  //
  Triangle::Triangle(const char * id):
    Item(id),
    collapsed_(BoolRef::constant(false)),
    border_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0xffffff, 1.0))),
    colorBorder_(ColorRef::constant(Color(0x7f7f7f, 0.25)))
  {}

  //----------------------------------------------------------------
  // Triangle::uncache
  //
  void
  Triangle::uncache()
  {
    collapsed_.uncache();
    border_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Triangle::paintContent
  //
  void
  Triangle::paintContent() const
  {
    static const double sin_30 = 0.5;
    static const double cos_30 = 0.866025403784439;

    bool collapsed = collapsed_.get();
    const Color & color = color_.get();
    const Segment & xseg = this->xExtent();
    const Segment & yseg = this->yExtent();

    double radius = 0.5 * (yseg.length_ < xseg.length_ ?
                           yseg.length_ :
                           xseg.length_);

    TVec2D center(xseg.center(), yseg.center());
    TVec2D p[3];

    if (collapsed)
    {
      p[0] = center + radius * TVec2D(1.0, 0.0);
      p[1] = center + radius * TVec2D(-sin_30, -cos_30);
      p[2] = center + radius * TVec2D(-sin_30, cos_30);
    }
    else
    {
      p[0] = center + radius * TVec2D(cos_30, -sin_30);
      p[1] = center + radius * TVec2D(-cos_30, -sin_30);
      p[2] = center + radius * TVec2D(0.0, 1.0);
    }

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_TRIANGLE_FAN));
    {
      YAE_OGL_11(glVertex2dv(center.coord_));
      YAE_OGL_11(glVertex2dv(p[0].coord_));
      YAE_OGL_11(glVertex2dv(p[1].coord_));
      YAE_OGL_11(glVertex2dv(p[2].coord_));
      YAE_OGL_11(glVertex2dv(p[0].coord_));
    }
    YAE_OGL_11(glEnd());

    double border = border_.get();
    if (border > 0.0)
    {
      const Color & colorBorder = colorBorder_.get();
      YAE_OGL_11(glColor4ub(colorBorder.r(),
                            colorBorder.g(),
                            colorBorder.b(),
                            colorBorder.a()));
      YAE_OGL_11(glLineWidth(border));
      YAE_OGL_11(glBegin(GL_LINE_LOOP));
      {
        YAE_OGL_11(glVertex2dv(p[0].coord_));
        YAE_OGL_11(glVertex2dv(p[1].coord_));
        YAE_OGL_11(glVertex2dv(p[2].coord_));
      }
      YAE_OGL_11(glEnd());
    }
  }

}
