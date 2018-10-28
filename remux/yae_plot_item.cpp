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
  // PlotItem::paintContent
  //
  void
  PlotItem::paintContent() const
  {
    private_->paint();
  }

}
