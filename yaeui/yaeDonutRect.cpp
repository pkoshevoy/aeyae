// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jun 29 21:16:03 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeDonutRect.h"


namespace yae
{
#if 0
  static const unsigned char kCheckerBoardBitmap[] = {
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,

    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,

    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,
    0xff, 0x00, 0xff, 0x00,

    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff,
    0x00, 0xff, 0x00, 0xff
  };
#endif

  //----------------------------------------------------------------
  // DonutRect::DonutRect
  //
  DonutRect::DonutRect(const char * id):
    Item(id),
    xHole_(0.5, 0.0),
    yHole_(0.5, 0.0),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5)))
  {}

  //----------------------------------------------------------------
  // DonutRect::uncache
  //
  void
  DonutRect::uncache()
  {
    // xHole_.uncache();
    // yHole_.uncache();
    opacity_.uncache();
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // DonutRect::paint
  //
  void
  DonutRect::paintContent() const
  {
    BBox bbox;
    Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double x1 = bbox.w_ * xHole_.origin_ + x0;
    double x2 = bbox.w_ * xHole_.length_ + x1;
    double x3 = bbox.w_ + x0;

    double y0 = bbox.y_;
    double y1 = bbox.h_ * yHole_.origin_ + y0;
    double y2 = bbox.h_ * yHole_.length_ + y1;
    double y3 = bbox.h_ + y0;

    double opacity = opacity_.get();
    const Color & color = color_.get();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));
#if 0
    YAE_OGL_11(glRasterPos2d(x0, y0));
    YAE_OGL_11(glPolygonStipple(kCheckerBoardBitmap));
    YAE_OGL_11(glEnable(GL_POLYGON_STIPPLE));
#endif
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x1, y1));
      YAE_OGL_11(glVertex2d(x0, y3));
      YAE_OGL_11(glVertex2d(x1, y2));
      YAE_OGL_11(glVertex2d(x3, y3));
      YAE_OGL_11(glVertex2d(x2, y2));
      YAE_OGL_11(glVertex2d(x3, y0));
      YAE_OGL_11(glVertex2d(x2, y1));
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());
#if 0
    YAE_OGL_11(glDisable(GL_POLYGON_STIPPLE));
#endif
  }

  //----------------------------------------------------------------
  // DonutRect::visible
  //
  bool
  DonutRect::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // DonutRect::get
  //
  void
  DonutRect::get(Property property, double & value) const
  {
    if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else if (property == kPropertyDonutHoleLeft)
    {
      const Segment & x = this->xExtent();
      value = x.origin_ + x.length_ * xHole_.origin_;
    }
    else if (property == kPropertyDonutHoleRight)
    {
      const Segment & x = this->xExtent();
      value = x.origin_ + x.length_ * (xHole_.origin_ + xHole_.length_);
    }
    else if (property == kPropertyDonutHoleTop)
    {
      const Segment & y = this->yExtent();
      value = y.origin_ + y.length_ * yHole_.origin_;
    }
    else if (property == kPropertyDonutHoleBottom)
    {
      const Segment & y = this->yExtent();
      value = y.origin_ + y.length_ * (yHole_.origin_ + yHole_.length_);
    }
    else if (property == kPropertyDonutHoleWidth)
    {
      const Segment & x = this->xExtent();
      value = x.length_ * xHole_.length_;
    }
    else if (property == kPropertyDonutHoleHeight)
    {
      const Segment & y = this->yExtent();
      value = y.length_ * yHole_.length_;
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // DonutRect::get
  //
  void
  DonutRect::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = color_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

}
