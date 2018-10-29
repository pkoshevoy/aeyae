// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Oct  7 13:52:18 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system:
#include <limits>
#include <math.h>

// boost:
#include <boost/random/mersenne_twister.hpp>

// local:
#include "yae_plot_item.h"


namespace yae
{

  //----------------------------------------------------------------
  // MockDataSource::MockDataSource
  //
  MockDataSource::MockDataSource(std::size_t n, double v_min, double v_max):
    min_(std::numeric_limits<double>::max()),
    max_(-std::numeric_limits<double>::max())
  {
    static boost::random::mt11213b prng;

    static const double prng_max =
      std::numeric_limits<boost::random::mt11213b::result_type>::max();

    const double v_rng = v_max - v_min;
    const boost::random::mt11213b::result_type offset = prng();
    data_.resize(n);
    for (std::size_t i = 0; i < n; i++)
    {
      double v = v_min + v_rng * double((offset + i) % 640) / 640.0;
      min_ = std::min<double>(min_, v);
      max_ = std::max<double>(max_, v);
      data_[i] = v;
    }
  }


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

    Segment range = data.range();

    BBox bbox;
    item_.Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    ScaleLinear sx(0, sz, x0, x1);
    ScaleLinear sy(range.to_wcs(0.0),
                   range.to_wcs(1.0),
                   y1 - 1,
                   y0 + 1);

    double r0 = xregion_.to_wcs(0.0);
    double r1 = xregion_.to_wcs(1.0);

    // FIXME: should round the domain and range (at their magnitude)

    std::size_t i0 = (std::size_t)(std::max<double>(0.0, sx.invert(r0)));
    std::size_t i1 = (std::size_t)(std::min<double>(sz, ceil(sx.invert(r1))));

    // this can be cached, and used as a VBO perhaps?
    std::vector<TVec2D> points(i1 - i0);
    for (std::size_t i = i0; i < i1; i++)
    {
      TVec2D & p = points[i - i0];

      double v = data.get(i);
      p.set_x(sx(i));
      p.set_y(sy(v));
    }

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_LINE_STRIP));
    for (std::vector<TVec2D>::const_iterator
           i = points.begin(); i != points.end(); ++i)
    {
      const TVec2D & p = *i;
      YAE_OGL_11(glVertex2d(p.x(), p.y()));
    }
    YAE_OGL_11(glEnd());
  }


  //----------------------------------------------------------------
  // PlotItem::PlotItem
  //
  PlotItem::PlotItem(const char * name, const TDataSourcePtr & data):
    Item(name),
    private_(new PlotItem::Private(*this, data)),
    color_(ColorRef::constant(Color(0xff0000, 0.7)))
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
