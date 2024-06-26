// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jul  2 11:46:54 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeDashedRect.h"


namespace yae
{
  //----------------------------------------------------------------
  // DashedRect::DashedRect
  //
  DashedRect::DashedRect(const char * id):
    Item(id),
    border_(ItemRef::constant(2.0)),
    opacity_(ItemRef::constant(1.0)),
    fg_(ColorRef::constant(Color(0xffffff, 0.78))),
    bg_(ColorRef::constant(Color(0x000000, 0.22)))
  {}

  //----------------------------------------------------------------
  // DashedRect::uncache
  //
  void
  DashedRect::uncache()
  {
    border_.uncache();
    opacity_.uncache();
    fg_.uncache();
    bg_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // drawLineRect
  //
  static void
  drawLineRect(double x0,
               double y0,
               double x1,
               double y1,
               const Color & color,
               double opacity,
               GLushort pattern,
               GLubyte zoom)
  {
    YAE_OGL_11_HERE();
    YAE_OGL_11(glLineStipple(zoom, pattern));
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));

    {
      yaegl::BeginEnd mode(GL_LINE_LOOP);

      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x0, y1));
      YAE_OGL_11(glVertex2d(x1, y1));
      YAE_OGL_11(glVertex2d(x1, y0));
    }
  }

  //----------------------------------------------------------------
  // DashedRect::paint
  //
  void
  DashedRect::paintContent() const
  {
    BBox bbox;
    Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double x1 = bbox.w_ + x0;

    double y0 = bbox.y_;
    double y1 = bbox.h_ + y0;

    double border = border_.get();
    double opacity = opacity_.get();
    const Color & fg = fg_.get();
    const Color & bg = bg_.get();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glLineWidth(border));

    TGLSaveState pushAttr(GL_ENABLE_BIT);
    YAE_OGL_11(glEnable(GL_LINE_STIPPLE));
    YAE_OGL_11(glDisable(GL_LINE_SMOOTH));

    drawLineRect(x0, y0, x1, y1, fg, opacity, 0xff00, 2);
    drawLineRect(x0, y0, x1, y1, bg, opacity, 0x00ff, 2);
  }

  //----------------------------------------------------------------
  // DashedRect::visible
  //
  bool
  DashedRect::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // DashedRect::get
  //
  void
  DashedRect::get(Property property, double & value) const
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
  // DashedRect::get
  //
  void
  DashedRect::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = fg_.get();
    }
    else if (property == kPropertyColorBg)
    {
      value = bg_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

}
