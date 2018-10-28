// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Oct  7 13:52:18 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system:
#include <math.h>

// local:
#include "yae_plot_item.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlotItem::Private
  //
  struct PlotItem::Private
  {
    Private(PlotItem & item, const TDataSourcePtr & data):
      item_(item),
      data_(data)
    {}

    void paint();

    PlotItem & item_;
    Segment xregion_;
    Segment yregion_;
    TDataSourcePtr data_;
  };

  //----------------------------------------------------------------
  // PlotItem::Private::paint
  //
  void
  PlotItem::Private::paint()
  {
    if (!data_)
    {
      return;
    }

    const Color & color = item_.color_.get();
    const TDataSource & data = *data_;
    std::size_t sz = data.size();

    if (!sz)
    {
      return;
    }

    double v_min = std::numeric_limits<double>::max();
    double v_max = std::numeric_limits<double>::min();

    if (!data.get_range(v_min, v_max))
    {
      return;
    }

    BBox bbox;
    item_.Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    ScaleLinear sx(0, sz, x0, x1);
    ScaleLinear sy(v_min, v_max, y0, y1);

    double r0 = xregion_.to_wcs(0.0);
    double r1 = xregion_.to_wcs(1.0);

    std::size_t i0 = std::max<double>(0.0, sx.invert(r0));
    std::size_t i1 = std::min<double>(sz, ceil(sx.invert(r1)));

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_LINE_STRIP));
    for (std::size_t i = i0; i < i1; i++)
    {
      double v = 0.0;
      data.get(i, v);

      double x = sx(i);
      double y = sy(v);
      YAE_OGL_11(glVertex2d(x, y));
    }
    YAE_OGL_11(glEnd());
  }


  //----------------------------------------------------------------
  // PlotItem::PlotItem
  //
  PlotItem::PlotItem(const char * name, const TDataSourcePtr & data):
    Item(name),
    private_(new PlotItem::Private(*this, data))
  {}

  //----------------------------------------------------------------
  // PlotItem::~PlotItem
  //
  PlotItem::~PlotItem()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // PlotItem::setData
  //
  void
  PlotItem::setData(const TDataSourcePtr & data)
  {
    private_->data_ = data;
  }

  //----------------------------------------------------------------
  // PlotItem::uncache
  //
  void
  PlotItem::uncache()
  {
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // PlotItem::paintContent
  //
  void
  PlotItem::paintContent() const
  {
    private_->paint();
  }

  //----------------------------------------------------------------
  // PlotItem::paint
  //
  bool
  PlotItem::paint(const Segment & xregion,
                  const Segment & yregion,
                  Canvas * canvas) const
  {
    private_->xregion_ = xregion;
    private_->yregion_ = yregion;
    return Item::paint(xregion, yregion, canvas);
  }

  //----------------------------------------------------------------
  // PlotItem::get
  //
  void
  PlotItem::get(Property property, Color & value) const
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
