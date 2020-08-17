// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <algorithm>

// local interfaces:
#include "yaeBBox.h"
#include "yaeCanvasRenderer.h"
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // Gradient::Gradient
  //
  Gradient::Gradient(const char * id):
    Item(id),
    opacity_(ItemRef::constant(1.0)),
    orientation_(Gradient::kVertical)
  {}

  //----------------------------------------------------------------
  // Gradient::uncache
  //
  void
  Gradient::uncache()
  {
    opacity_.uncache();
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Gradient::paintContent
  //
  void
  Gradient::paintContent() const
  {
    const TGradientPtr & gradientPtr = color_.get();
    if (!gradientPtr)
    {
      YAE_ASSERT(false);
      return;
    }

    const TGradient & gradient = *gradientPtr;
    if (gradient.size() < 2)
    {
      YAE_ASSERT(false);
      return;
    }

    BBox bbox;
    this->Item::get(kPropertyBBox, bbox);

    TVec2D xvec(bbox.w_, 0.0);
    TVec2D yvec(0.0, bbox.h_);

    TVec2D o(bbox.x_, bbox.y_);
    TVec2D u = (orientation_ == Gradient::kHorizontal) ? xvec : yvec;
    TVec2D v = (orientation_ == Gradient::kHorizontal) ? yvec : xvec;

    TGradient::const_iterator i = gradient.begin();
    double t0 = i->first;
    const Color * c0 = &(i->second);
    const double opacity = opacity_.get();
    unsigned char a0 = Color::transform(c0->a(), opacity);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    for (++i; i != gradient.end(); ++i)
    {
      double t1 = i->first;
      const Color * c1 = &(i->second);
      unsigned char a1 = Color::transform(c1->a(), opacity);

      TVec2D p0 = (o + u * t0);
      TVec2D p1 = (o + u * t1);

      YAE_OGL_11(glColor4ub(c0->r(), c0->g(), c0->b(), a0));
      YAE_OGL_11(glVertex2dv(p0.coord_));
      YAE_OGL_11(glVertex2dv((p0 + v).coord_));

      YAE_OGL_11(glColor4ub(c1->r(), c1->g(), c1->b(), a1));
      YAE_OGL_11(glVertex2dv(p1.coord_));
      YAE_OGL_11(glVertex2dv((p1 + v).coord_));

      std::swap(t0, t1);
      std::swap(c0, c1);
      std::swap(a0, a1);
    }
    YAE_OGL_11(glEnd());
  }

  //----------------------------------------------------------------
  // Gradient::visible
  //
  bool
  Gradient::visible() const
  {
    return Item::visible() && opacity_.get() > 0.0;
  }

  //----------------------------------------------------------------
  // Gradient::get
  //
  void
  Gradient::get(Property property, double & value) const
  {
    if (property == kPropertyOpacity)
    {
      value = opacity_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }
}
