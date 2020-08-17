// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Aug 13 22:30:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local:
#include "yaeBBox.h"
#include "yaeLShape.h"


namespace yae
{

  //----------------------------------------------------------------
  // LShape::LShape
  //
  LShape::LShape(const char * id):
    Item(id),
    weight_(ItemRef::constant(2.0)),
    opacity_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0xffffff, 0.5)))
  {}

  //----------------------------------------------------------------
  // LShape::uncache
  //
  void
  LShape::uncache()
  {
    weight_.uncache();
    opacity_.uncache();
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // paintLShape
  //
  static void
  paintLShape(const BBox & bbox,
              double weight,
              double opacity,
              const Color & color)
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

    YAE_OGL_11(glBegin(GL_TRIANGLE_FAN));
    {
      YAE_OGL_11(glVertex2d(x0 + weight, y1 - weight));
      YAE_OGL_11(glVertex2d(x0 + weight, y0));
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x0, y1));
      YAE_OGL_11(glVertex2d(x1, y1));
      YAE_OGL_11(glVertex2d(x1, y1 - weight));
    }
    YAE_OGL_11(glEnd());
  }

  //----------------------------------------------------------------
  // LShape::paintContent
  //
  void
  LShape::paintContent() const
  {
    BBox bbox;
    Item::get(kPropertyBBox, bbox);

    double weight = weight_.get();
    double opacity = opacity_.get();
    const Color & color = color_.get();

    paintLShape(bbox, weight, opacity, color);
  }

  //----------------------------------------------------------------
  // LShape::visible
  //
  bool
  LShape::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // LShape::get
  //
  void
  LShape::get(Property property, double & value) const
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
  // LShape::get
  //
  void
  LShape::get(Property property, Color & value) const
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
