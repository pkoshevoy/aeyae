// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeBBox.h"
#include "yaeRectangle.h"


namespace yae
{
  //----------------------------------------------------------------
  // Rectangle::Rectangle
  //
  Rectangle::Rectangle(const char * id):
    Item(id),
    border_(ItemRef::constant(0.0)),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25)))
  {}

  //----------------------------------------------------------------
  // paintRect
  //
  static void
  paintRect(const BBox & bbox,
            double border,
            double opacity,
            const Color & color,
            const Color & colorBorder)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x0, y1));
      YAE_OGL_11(glVertex2d(x1, y0));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());

    if (border > 0.0)
    {
      YAE_OGL_11(glColor4ub(colorBorder.r(),
                            colorBorder.g(),
                            colorBorder.b(),
                            Color::transform(colorBorder.a(), opacity)));
      YAE_OGL_11(glLineWidth(border));
      YAE_OGL_11(glBegin(GL_LINE_LOOP));
      {
        YAE_OGL_11(glVertex2d(x0, y0));
        YAE_OGL_11(glVertex2d(x0, y1));
        YAE_OGL_11(glVertex2d(x1, y1));
        YAE_OGL_11(glVertex2d(x1, y0));
      }
      YAE_OGL_11(glEnd());
    }
  }

  //----------------------------------------------------------------
  // Rectangle::uncache
  //
  void
  Rectangle::uncache()
  {
    border_.uncache();
    opacity_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Rectangle::paint
  //
  void
  Rectangle::paintContent() const
  {
    BBox bbox;
    Item::get(kPropertyBBox, bbox);

    double border = border_.get();
    double opacity = opacity_.get();
    const Color & color = color_.get();
    const Color & colorBorder = colorBorder_.get();

    paintRect(bbox,
              border,
              opacity,
              color,
              colorBorder);
  }

  //----------------------------------------------------------------
  // Rectangle::visible
  //
  bool
  Rectangle::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // Rectangle::get
  //
  void
  Rectangle::get(Property property, double & value) const
  {
    if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else if (property == kPropertyBorderWidth)
    {
      value = border_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // Rectangle::get
  //
  void
  Rectangle::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else if (property == kPropertyColorBorder)
    {
      value = colorBorder_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

}
