// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Feb 20 22:30:53 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// local:
#include "yaeArrowItem.h"
#include "yaeBBox.h"


namespace yae
{

  //----------------------------------------------------------------
  // paint_arrow_left
  //
  static void
  paint_arrow_left(const BBox & bbox,
                   double weight,
                   double opacity,
                   const Color & color)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;
    double x = x0 + bbox.w_ * 0.5;
    double y = y0 + bbox.h_ * 0.5;
    double a = weight * M_SQRT1_2;
    double w = weight * 0.5;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));

    {
      yaegl::BeginEnd mode(GL_TRIANGLE_FAN);

      YAE_OGL_11(glVertex2d(x0, y));
      YAE_OGL_11(glVertex2d(x, y1));
      YAE_OGL_11(glVertex2d(x + a, y1 - a));
      YAE_OGL_11(glVertex2d(x0 + 2.0 * a + w, y + w));
      YAE_OGL_11(glVertex2d(x1, y + w));
      YAE_OGL_11(glVertex2d(x1, y - w));
      YAE_OGL_11(glVertex2d(x0 + 2.0 * a + w, y - w));
      YAE_OGL_11(glVertex2d(x + a, y0 + a));
      YAE_OGL_11(glVertex2d(x, y0));
    }
  }

  //----------------------------------------------------------------
  // paint_arrow_right
  //
  static void
  paint_arrow_right(const BBox & bbox,
                    double weight,
                    double opacity,
                    const Color & color)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;
    double x = x0 + bbox.w_ * 0.5;
    double y = y0 + bbox.h_ * 0.5;
    double a = weight * M_SQRT1_2;
    double w = weight * 0.5;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));

    {
      yaegl::BeginEnd mode(GL_TRIANGLE_FAN);

      YAE_OGL_11(glVertex2d(x1, y));
      YAE_OGL_11(glVertex2d(x, y0));
      YAE_OGL_11(glVertex2d(x - a, y0 + a));
      YAE_OGL_11(glVertex2d(x1 - 2.0 * a - w, y - w));
      YAE_OGL_11(glVertex2d(x0, y - w));
      YAE_OGL_11(glVertex2d(x0, y + w));
      YAE_OGL_11(glVertex2d(x1 - 2.0 * a - w, y + w));
      YAE_OGL_11(glVertex2d(x - a, y1 - a));
      YAE_OGL_11(glVertex2d(x, y1));
    }
  }

  //----------------------------------------------------------------
  // paint_arrow_up
  //
  static void
  paint_arrow_up(const BBox & bbox,
                 double weight,
                 double opacity,
                 const Color & color)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;
    double x = x0 + bbox.w_ * 0.5;
    double y = y0 + bbox.h_ * 0.5;
    double a = weight * M_SQRT1_2;
    double w = weight * 0.5;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));

    {
      yaegl::BeginEnd mode(GL_TRIANGLE_FAN);

      YAE_OGL_11(glVertex2d(x, y0));
      YAE_OGL_11(glVertex2d(x0, y));
      YAE_OGL_11(glVertex2d(x0 + a, y + a));
      YAE_OGL_11(glVertex2d(x - w, 2.0 * a + w));
      YAE_OGL_11(glVertex2d(x - w, y1));
      YAE_OGL_11(glVertex2d(x + w, y1));
      YAE_OGL_11(glVertex2d(x + w, 2.0 * a + w));
      YAE_OGL_11(glVertex2d(x1 - a, y + a));
      YAE_OGL_11(glVertex2d(x1, y));
    }
  }

  //----------------------------------------------------------------
  // paint_arrow_down
  //
  static void
  paint_arrow_down(const BBox & bbox,
                   double weight,
                   double opacity,
                   const Color & color)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;
    double x = x0 + bbox.w_ * 0.5;
    double y = y0 + bbox.h_ * 0.5;
    double a = weight * M_SQRT1_2;
    double w = weight * 0.5;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          Color::transform(color.a(), opacity)));

    {
      yaegl::BeginEnd mode(GL_TRIANGLE_FAN);

      YAE_OGL_11(glVertex2d(x, y1));
      YAE_OGL_11(glVertex2d(x1, y));
      YAE_OGL_11(glVertex2d(x1 - a, y - a));
      YAE_OGL_11(glVertex2d(x + w, y1 - 2.0 * a - w));
      YAE_OGL_11(glVertex2d(x + w, y0));
      YAE_OGL_11(glVertex2d(x - w, y0));
      YAE_OGL_11(glVertex2d(x - w, y1 - 2.0 * a - w));
      YAE_OGL_11(glVertex2d(x0 + a, y - a));
      YAE_OGL_11(glVertex2d(x0, y));
    }
  }

  //----------------------------------------------------------------
  // ArrowItem::ArrowItem
  //
  ArrowItem::ArrowItem(const char * id, ArrowItem::Direction direction):
    Item(id),
    weight_(ItemRef::constant(2.0)),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0xffffff, 0.5)))
  {
    paint_arrow_ = (direction == ArrowItem::kUp ? &paint_arrow_up :
                    direction == ArrowItem::kDown ? &paint_arrow_down :
                    direction == ArrowItem::kRight ? &paint_arrow_right :
                    &paint_arrow_left);
  }

  //----------------------------------------------------------------
  // ArrowItem::uncache
  //
  void
  ArrowItem::uncache()
  {
    weight_.uncache();
    opacity_.uncache();
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // ArrowItem::paintContent
  //
  void
  ArrowItem::paintContent() const
  {
    BBox bbox;
    Item::get(kPropertyBBox, bbox);

    double weight = weight_.get();
    double opacity = opacity_.get();
    const Color & color = color_.get();

    paint_arrow_(bbox, weight, opacity, color);
  }

  //----------------------------------------------------------------
  // ArrowItem::visible
  //
  bool
  ArrowItem::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // ArrowItem::get
  //
  void
  ArrowItem::get(Property property, double & value) const
  {
    if (property == kPropertyWeight)
    {
      value = weight_.get();
    }
    else if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // ArrowItem::get
  //
  void
  ArrowItem::get(Property property, Color & value) const
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
