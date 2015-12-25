// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

// boost library:
#include <boost/chrono.hpp>

// Qt library:
#include <QApplication>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QUrl>
#include <QWheelEvent>

// local interfaces:
#include "yaeItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // CalcXContent
  //
  struct CalcXContent : public TSegmentExpr
  {
    CalcXContent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = item_.calcContentWidth();
      result.origin_ =
        item_.anchors_.left_.isValid() ?
        item_.left() :

        item_.anchors_.right_.isValid() ?
        item_.right() - result.length_ :

        item_.hcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->xExtent();
        result.expand(footprint);
      }
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcYContent
  //
  struct CalcYContent : public TSegmentExpr
  {
    CalcYContent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = item_.calcContentHeight();
      result.origin_ =
        item_.anchors_.top_.isValid() ?
        item_.top() :

        item_.anchors_.bottom_.isValid() ?
        item_.bottom() - result.length_ :

        item_.vcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->yExtent();
        result.expand(footprint);
      }
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcXExtent
  //
  struct CalcXExtent : public TSegmentExpr
  {
    CalcXExtent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = item_.left();
      result.length_ = item_.width();
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcYExtent
  //
  struct CalcYExtent : public TSegmentExpr
  {
    CalcYExtent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = item_.top();
      result.length_ = item_.height();
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // itemHeightDueToItemContent
  //
  double
  itemHeightDueToItemContent(const Item & item)
  {
    double h = 0.0;

    const Segment & yContent = item.yContent();
    if (!yContent.isEmpty())
    {
      if (item.anchors_.top_.isValid())
      {
        double t = item.top();
        double b = yContent.end();
        h = b - t;
      }
      else if (item.anchors_.bottom_.isValid())
      {
        double t = yContent.start();
        double b = item.bottom();
        h = b - t;
      }
      else
      {
        YAE_ASSERT(item.anchors_.vcenter_.isValid());
        h = yContent.length_;
      }
    }

    return h;
  }


  //----------------------------------------------------------------
  // InvisibleItemZeroHeight::InvisibleItemZeroHeight
  //
  InvisibleItemZeroHeight::InvisibleItemZeroHeight(const Item & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // InvisibleItemZeroHeight::evaluate
  //
  void
  InvisibleItemZeroHeight::evaluate(double & result) const
  {
    if (item_.visible())
    {
      result = itemHeightDueToItemContent(item_);
      return;
    }

    result = 0.0;
  }


  //----------------------------------------------------------------
  // InscribedCircleDiameterFor::InscribedCircleDiameterFor
  //
  InscribedCircleDiameterFor::InscribedCircleDiameterFor(const Item & item):
    item_(item)
  {}

  //----------------------------------------------------------------
  // InscribedCircleDiameterFor::evaluate
  //
  void
  InscribedCircleDiameterFor::evaluate(double & result) const
  {
    double w = 0.0;
    double h = 0.0;
    item_.get(kPropertyWidth, w);
    item_.get(kPropertyHeight, h);
    result = std::min<double>(w, h);
  }


  //----------------------------------------------------------------
  // Margins::Margins
  //
  Margins::Margins()
  {
    set(ItemRef::constant(0));
  }

  //----------------------------------------------------------------
  // Margins::uncache
  //
  void
  Margins::uncache()
  {
    left_.uncache();
    right_.uncache();
    top_.uncache();
    bottom_.uncache();
  }

  //----------------------------------------------------------------
  // Margins::set
  //
  void
  Margins::set(const ItemRef & ref)
  {
    left_ = ref;
    right_ = ref;
    top_ = ref;
    bottom_ = ref;
  }


  //----------------------------------------------------------------
  // Anchors::uncache
  //
  void
  Anchors::uncache()
  {
    left_.uncache();
    right_.uncache();
    top_.uncache();
    bottom_.uncache();
    hcenter_.uncache();
    vcenter_.uncache();
  }

  //----------------------------------------------------------------
  // Anchors::offset
  //
  void
  Anchors::offset(const TDoubleProp & ref,
                  double ox0, double ox1,
                  double oy0, double oy1)
  {
    left_ = ItemRef::offset(ref, kPropertyLeft, ox0);
    right_ = ItemRef::offset(ref, kPropertyRight, ox1);
    top_ = ItemRef::offset(ref, kPropertyTop, oy0);
    bottom_ = ItemRef::offset(ref, kPropertyBottom, oy1);
  }

  //----------------------------------------------------------------
  // Anchors::center
  //
  void
  Anchors::center(const TDoubleProp & ref)
  {
    hcenter_ = ItemRef::offset(ref, kPropertyHCenter);
    vcenter_ = ItemRef::offset(ref, kPropertyVCenter);
  }

  //----------------------------------------------------------------
  // Anchors::topLeft
  //
  void
  Anchors::topLeft(const TDoubleProp & ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::topRight
  //
  void
  Anchors::topRight(const TDoubleProp & ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomLeft
  //
  void
  Anchors::bottomLeft(const TDoubleProp & ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomRight
  //
  void
  Anchors::bottomRight(const TDoubleProp & ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }


  //----------------------------------------------------------------
  // Item::Item
  //
  Item::Item(const char * id):
    parent_(NULL),
    visible_(BoolRef::constant(true)),
    xContent_(addExpr(new CalcXContent(*this))),
    yContent_(addExpr(new CalcYContent(*this))),
    xExtent_(addExpr(new CalcXExtent(*this))),
    yExtent_(addExpr(new CalcYExtent(*this))),
    painted_(false)
  {
    if (id)
    {
      id_.assign(id);
    }
  }

  //----------------------------------------------------------------
  // Item::calcContentWidth
  //
  double
  Item::calcContentWidth() const
  {
    return 0.0;
  }

  //----------------------------------------------------------------
  // Item::calcContentHeight
  //
  double
  Item::calcContentHeight() const
  {
    return 0.0;
  }

  //----------------------------------------------------------------
  // Item::uncache
  //
  void
  Item::uncache()
  {
    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->uncache();
    }

    anchors_.uncache();
    margins_.uncache();
    width_.uncache();
    height_.uncache();
    xContent_.uncache();
    yContent_.uncache();
    xExtent_.uncache();
    yExtent_.uncache();
    visible_.uncache();
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, double & value) const
  {
    if (property == kPropertyWidth)
    {
      value = this->width();
    }
    else if (property == kPropertyHeight)
    {
      value = this->height();
    }
    else if (property == kPropertyLeft)
    {
      value = this->left();
    }
    else if (property == kPropertyRight)
    {
      value = this->right();
    }
    else if (property == kPropertyTop)
    {
      value = this->top();
    }
    else if (property == kPropertyBottom)
    {
      value = this->bottom();
    }
    else if (property == kPropertyHCenter)
    {
      value = this->hcenter();
    }
    else if (property == kPropertyVCenter)
    {
      value = this->vcenter();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <double>");
      value = std::numeric_limits<double>::max();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, Segment & value) const
  {
    if (property == kPropertyXContent)
    {
      value = this->xContent();
    }
    else if (property == kPropertyYContent)
    {
      value = this->yContent();
    }
    else if (property == kPropertyXExtent)
    {
      value = this->xExtent();
    }
    else if (property == kPropertyYExtent)
    {
      value = this->yExtent();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <Segment>");
      value = Segment();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, BBox & bbox) const
  {
    if (property == kPropertyBBoxContent)
    {
      const Segment & x = this->xContent();
      const Segment & y = this->yContent();

      bbox.x_ = x.origin_;
      bbox.w_ = x.length_;

      bbox.y_ = y.origin_;
      bbox.h_ = y.length_;
    }
    else if (property == kPropertyBBox)
    {
      const Segment & xExtent = this->xExtent();
      const Segment & yExtent = this->yExtent();

      bbox.x_ = xExtent.origin_;
      bbox.w_ = xExtent.length_;

      bbox.y_ = yExtent.origin_;
      bbox.h_ = yExtent.length_;
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <BBox>");
      bbox = BBox();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, bool & value) const
  {
    if (property == kPropertyVisible)
    {
      value = this->visible();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <bool>");
      value = false;
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, Color & value) const
  {
    YAE_ASSERT(false);
    throw std::runtime_error("unsupported item property of type <Color>");
    value = Color();
  }

  //----------------------------------------------------------------
  // Item::xContent
  //
  const Segment &
  Item::xContent() const
  {
    return xContent_.get();
  }

  //----------------------------------------------------------------
  // Item::yContent
  //
  const Segment &
  Item::yContent() const
  {
    return yContent_.get();
  }

  //----------------------------------------------------------------
  // Item::xExtent
  //
  const Segment &
  Item::xExtent() const
  {
    return xExtent_.get();
  }

  //----------------------------------------------------------------
  // Item::yExtent
  //
  const Segment &
  Item::yExtent() const
  {
    return yExtent_.get();
  }

  //----------------------------------------------------------------
  // Item::width
  //
  double
  Item::width() const
  {
    if (width_.isValid() || width_.isCached())
    {
      return width_.get();
    }

    if (anchors_.left_.isValid() && anchors_.right_.isValid())
    {
      double l = anchors_.left_.get();
      double r = anchors_.right_.get();
      l += margins_.left_.get();
      r -= margins_.right_.get();

      double w = r - l;
      width_.cache(w);
      return w;
    }

    // height is based on horizontal footprint of item content:
    const Segment & xContent = this->xContent();
    double w = 0.0;

    if (!xContent.isEmpty())
    {
      if (anchors_.left_.isValid())
      {
        double l = left();
        double r = xContent.end();
        w = r - l;
      }
      else if (anchors_.right_.isValid())
      {
        double l = xContent.start();
        double r = right();
        w = r - l;
      }
      else
      {
        YAE_ASSERT(anchors_.hcenter_.isValid());
        w = xContent.length_;
      }
    }

    width_.cache(w);
    return w;
  }

  //----------------------------------------------------------------
  // Item::height
  //
  double
  Item::height() const
  {
    if (height_.isValid() || height_.isCached())
    {
      return height_.get();
    }

    if (anchors_.top_.isValid() && anchors_.bottom_.isValid())
    {
      double t = anchors_.top_.get();
      double b = anchors_.bottom_.get();
      t += margins_.top_.get();
      b -= margins_.bottom_.get();

      double h = b - t;
      height_.cache(h);
      return h;
    }

    // height is based on vertical footprint of item content:
    double h = itemHeightDueToItemContent(*this);
    height_.cache(h);
    return h;
  }

  //----------------------------------------------------------------
  // Item::left
  //
  double
  Item::left() const
  {
    if (anchors_.left_.isValid())
    {
      double l = anchors_.left_.get();
      l += margins_.left_.get();
      return l;
    }

    if (anchors_.right_.isValid())
    {
      double w = width();
      double r = anchors_.right_.get();
      double l = (r - margins_.right_.get()) - w;
      return l;
    }

    if (anchors_.hcenter_.isValid())
    {
      double w = width();
      double c = anchors_.hcenter_.get();
      double l = c - 0.5 * w;
      return l;
    }

    return margins_.left_.get();
  }

  //----------------------------------------------------------------
  // Item::right
  //
  double
  Item::right() const
  {
    if (anchors_.right_.isValid())
    {
      double r = anchors_.right_.get();
      r -= margins_.right_.get();
      return r;
    }

    double l = left();
    double w = width();
    double r = l + w;
    return r;
  }

  //----------------------------------------------------------------
  // Item::top
  //
  double
  Item::top() const
  {
    if (anchors_.top_.isValid())
    {
      double t = anchors_.top_.get();
      t += margins_.top_.get();
      return t;
    }

    if (anchors_.bottom_.isValid())
    {
      double h = height();
      double b = anchors_.bottom_.get();
      double t = (b - margins_.bottom_.get()) - h;
      return t;
    }

    if (anchors_.vcenter_.isValid())
    {
      double h = height();
      double c = anchors_.vcenter_.get();
      double t = c - 0.5 * h;
      return t;
    }

    return margins_.top_.get();
  }

  //----------------------------------------------------------------
  // Item::bottom
  //
  double
  Item::bottom() const
  {
    if (anchors_.bottom_.isValid())
    {
      double b = anchors_.bottom_.get();
      b += margins_.bottom_.get();
      return b;
    }

    double t = top();
    double h = height();
    double b = t + h;
    return b;
  }

  //----------------------------------------------------------------
  // Item::hcenter
  //
  double
  Item::hcenter() const
  {
    if (anchors_.hcenter_.isValid())
    {
      double hc = anchors_.hcenter_.get();
      double ml = margins_.left_.get();
      double mr = margins_.right_.get();
      double c = hc + ml - mr;
      return c;
    }

    double l = left();
    double w = width();
    double c = l + 0.5 * w;
    return c;
  }

  //----------------------------------------------------------------
  // Item::vcenter
  //
  double
  Item::vcenter() const
  {
    if (anchors_.vcenter_.isValid())
    {
      double vc = anchors_.vcenter_.get();
      double mt = margins_.top_.get();
      double mb = margins_.bottom_.get();
      double c = vc + mt - mb;
      return c;
    }

    double t = top();
    double h = height();
    double c = t + 0.5 * h;
    return c;
  }

  //----------------------------------------------------------------
  // Item::visible
  //
  bool
  Item::visible() const
  {
    return visible_.get();
  }

  //----------------------------------------------------------------
  // Item::operator[]
  //
  const Item &
  Item::operator[](const char * id) const
  {
    Item & item = const_cast<Item &>(*this);
    Item & found = item.operator[](id);
    return found;
  }

  //----------------------------------------------------------------
  // Item::operator[]
  //
  Item &
  Item::operator[](const char * id)
  {
    if (strcmp(id, "/") == 0)
    {
      Item * p = this;
      while (p->parent_)
      {
        p = p->parent_;
      }

      return *p;
    }

    if (strcmp(id, ".") == 0)
    {
      return *this;
    }

    if (strcmp(id, "..") == 0)
    {
      if (parent_)
      {
        return *parent_;
      }

      std::ostringstream oss;
      oss << id_ << ": has not parent";
      throw std::runtime_error(oss.str().c_str());
      return *this;
    }

    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      Item & child = *(*i);
      if (child.id_ == id)
      {
        return child;
      }
    }

    std::ostringstream oss;
    oss << id_ << ": item not found: " << id;
    throw std::runtime_error(oss.str().c_str());
    return *this;
  }

  //----------------------------------------------------------------
  // Item::overlaps
  //
  bool
  Item::overlaps(const TVec2D & pt) const
  {
    if (!Item::visible())
    {
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yfootprint.disjoint(pt.y()))
    {
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xfootprint.disjoint(pt.x()))
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Item::getInputHandlers
  //
  void
  Item::getInputHandlers(// coordinate system origin of
                         // the input area, expressed in the
                         // coordinate system of the root item:
                         const TVec2D & itemCSysOrigin,

                         // point expressed in the coord. system of the item,
                         // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                         const TVec2D & itemCSysPoint,

                         // pass back input areas overlapping above point,
                         // along with its coord. system origin expressed
                         // in the coordinate system of the root item:
                         std::list<InputHandler> & inputHandlers)
  {
    if (!overlaps(itemCSysPoint))
    {
      return;
    }

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);
    }
  }

  //----------------------------------------------------------------
  // Item::processEvent
  //
  bool
  Item::processEvent(Canvas::ILayer & canvasLayer,
                     Canvas * canvas,
                     QEvent * event)
  {
    (void) canvasLayer;
    (void) canvas;
    (void) event;
    return false;
  }

  //----------------------------------------------------------------
  // Item::paint
  //
  bool
  Item::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::visible())
    {
      unpaint();
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yregion.disjoint(yfootprint))
    {
      unpaint();
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xregion.disjoint(xfootprint))
    {
      unpaint();
      return false;
    }

    this->paintContent();
    painted_ = true;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint(xregion, yregion);
    }

    return true;
  }

  //----------------------------------------------------------------
  // Item::unpaint
  //
  void
  Item::unpaint() const
  {
    if (!painted_)
    {
      return;
    }

    this->unpaintContent();
    painted_ = false;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->unpaint();
    }
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Item::dump
  //
  void
  Item::dump(std::ostream & os, const std::string & indent) const
  {
    BBox bbox;
    this->get(kPropertyBBox, bbox);

    os << indent
       << "x: " << bbox.x_
       << ", y: " << bbox.y_
       << ", w: " << bbox.w_
       << ", h: " << bbox.h_
       << ", id: " << id_
       << std::endl;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->dump(os, indent + "  ");
    }
  }
#endif

}
