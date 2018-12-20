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
    Private(PlotItem & item):
      item_(item)
    {}

    void paint();

    PlotItem & item_;
    Segment xregion_;
    Segment yregion_;
  };

  //----------------------------------------------------------------
  // PlotItem::Private::paint
  //
  void
  PlotItem::Private::paint()
  {
    if (!(item_.data_x_ && item_.data_y_))
    {
      return;
    }

    const Color & color = item_.color_.get();
    const TDataSource & data_x = *item_.data_x_;
    const TDataSource & data_y = *item_.data_y_;

    std::size_t sz = std::min(data_x.size(), data_y.size());
    if (!sz)
    {
      return;
    }

    BBox bbox;
    item_.Item::get(kPropertyBBox, bbox);

    double x0 = bbox.x_;
    double y0 = bbox.y_;

    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    double t0 = data_x.get(0);
    double t1 = data_x.get(sz - 1);
    ScaleLinear si(t0, t1, 0, sz - 1);

    // find data origin within the domain:
    Segment range_x = data_x.range();
    Segment domain = (item_.domain_ ? *item_.domain_ : range_x);
    ScaleLinear sx(domain.to_wcs(0.0),
                   domain.to_wcs(1.0),
                   x0,
                   x1);

    Segment range_y = data_y.range();
    Segment range = (item_.range_ ? *item_.range_ : range_y).rounded();
    ScaleLinear sy(range.to_wcs(0.0),
                   range.to_wcs(1.0),
                   y1 - 1,
                   y0 + 1);

    double r0 = sx.invert(xregion_.to_wcs(0.0));
    double r1 = sx.invert(xregion_.to_wcs(1.0));

    std::size_t i0 = (std::size_t)
      (std::min<double>(sz, std::max(0.0, si(r0))));

    std::size_t i1 = (std::size_t)
      (std::min<double>(sz, std::max(0.0, ceil(si(r1)))));

    // this can be cached, and used as a VBO perhaps?
    std::vector<TVec2D> points(i1 - i0);
    for (std::size_t i = i0; i < i1; i++)
    {
      TVec2D & p = points[i - i0];

      double u = data_x.get(i);
      double v = data_y.get(i);
      p.set_x(sx(u));
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
  PlotItem::PlotItem(const char * name):
    Item(name),
    private_(new PlotItem::Private(*this)),
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
  // PlotItem::set_data
  //
  void
  PlotItem::set_data(const TDataSourcePtr & data_x,
                          const TDataSourcePtr & data_y)
  {
    YAE_ASSERT(!(data_x || data_y) || (data_y->size() <= data_x->size()));
    data_x_ = data_x;
    data_y_ = data_y;
  }

  //----------------------------------------------------------------
  // PlotItem::set_domain
  //
  void
  PlotItem::set_domain(const TSegmentPtr & domain)
  {
    domain_ = domain;
  }

  //----------------------------------------------------------------
  // PlotItem::set_range
  //
  void
  PlotItem::set_range(const TSegmentPtr & range)
  {
    range_ = range;
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
