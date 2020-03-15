// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 10 21:41:45 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system:
#include <limits>
#include <map>
#include <math.h>

// local:
#include "yaeAxisItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // AxisItem::Private
  //
  struct AxisItem::Private
  {
    Private(AxisItem & item):
      item_(item)
    {}

    void paint();
    void unpaint();

    AxisItem & item_;
    Segment xregion_;
    Segment yregion_;

    std::map<int, TTextPtr> labels_;
  };

  //----------------------------------------------------------------
  // AxisItem::Private::paint
  //
  void
  AxisItem::Private::paint()
  {
    const Color & color = item_.color_.get();

    BBox bbox;
    item_.Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    double t0 = item_.t0_.get();
    double t1 = item_.t1_.get();
    double dt = item_.tick_dt_.get();
    int mod_n = int(item_.mark_n_.get());

    double offset = fmod(t0, dt);

    ScaleLinear sx(t0, t1, x0, x1);
    double r0 = xregion_.to_wcs(0.0);
    double r1 = xregion_.to_wcs(1.0);

    int i0 = int((sx.invert(r0) - t0) / dt);
    int i1 = int((sx.invert(r1) - t0) / dt) + 1;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_LINES));

    YAE_OGL_11(glVertex2d(r0, y1 - 1));
    YAE_OGL_11(glVertex2d(r1, y1 - 1));

    for (std::size_t i = i0; i < i1; i++)
    {
      double t = t0 - offset + double(i) * dt;
      double x = round(sx.get(t));
      double z = (i % mod_n == 0) ? 7.0 : 2.0;
      YAE_OGL_11(glVertex2d(x, y1 - 1));
      YAE_OGL_11(glVertex2d(x, y1 + z));
    }

    YAE_OGL_11(glEnd());

    // draw tickmark labels:
    int i00 = i0 - (i0 % mod_n);
    int i11 = i1 - (i1 % mod_n);

    for (std::size_t i = i00; i <= i11; i += mod_n)
    {
      double t = t0 - offset + double(i) * dt;
      double x = round(sx.get(t));

      TTextPtr & p = labels_[i];
      if (!p)
      {
        p.reset(new Text(str("label_", i).c_str()));
        Text & text = *p;
        text.anchors_.left_ = ItemRef::constant(x + 3);
        text.anchors_.top_ = ItemRef::constant(y1);
        text.elide_ = Qt::ElideNone;
        text.fontSize_ = ItemRef::reference(item_.font_size_);
        text.font_ = item_.font_;

        std::string label =
          item_.formatter_ ?
          item_.formatter_->get(t) :
          yae::to_text(t);

        text.text_ = TVarRef::constant(TVar(QString::fromUtf8(label.c_str())));
      }

      p->paintContent();
    }

    // prune offscreen labels:
    if (!labels_.empty())
    {
      for (int i = labels_.begin()->first; i < i00; i += mod_n)
      {
        labels_.erase(i);
      }

      for (int i = i11 + mod_n, end = labels_.rbegin()->first;
           i <= end; i += mod_n)
      {
        labels_.erase(i);
      }
    }
  }

  //----------------------------------------------------------------
  // AxisItem::Private::unpaint
  //
  void
  AxisItem::Private::unpaint()
  {
    labels_.clear();
  }


  //----------------------------------------------------------------
  // AxisItem::AxisItem
  //
  AxisItem::AxisItem(const char * name):
    Item(name),
    private_(new AxisItem::Private(*this)),
    color_(ColorRef::constant(Color(0xffffff, 0.7))),
    t0_(ItemRef::constant(0)),
    t1_(ItemRef::constant(10000)),
    tick_dt_(ItemRef::constant(25)),
    mark_n_(ItemRef::constant(4))
  {
    font_size_ = ItemRef::constant(font_.pointSizeF());
  }

  //----------------------------------------------------------------
  // AxisItem::~AxisItem
  //
  AxisItem::~AxisItem()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // AxisItem::uncache
  //
  void
  AxisItem::uncache()
  {
    color_.uncache();
    t0_.uncache();
    t1_.uncache();
    tick_dt_.uncache();
    mark_n_.uncache();
    font_size_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // AxisItem::paintContent
  //
  void
  AxisItem::paintContent() const
  {
    private_->paint();
  }

  //----------------------------------------------------------------
  // AxisItem::unpaintContent
  //
  void
  AxisItem::unpaintContent() const
  {
    private_->unpaint();
  }

  //----------------------------------------------------------------
  // AxisItem::paint
  //
  bool
  AxisItem::paint(const Segment & xregion,
                  const Segment & yregion,
                  Canvas * canvas) const
  {
    private_->xregion_ = xregion;
    private_->yregion_ = yregion;
    return Item::paint(xregion, yregion, canvas);
  }

  //----------------------------------------------------------------
  // AxisItem::get
  //
  void
  AxisItem::get(Property property, Color & value) const
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
