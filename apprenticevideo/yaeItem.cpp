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
#include "yaeInputArea.h"
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
    void evaluate(Segment & r) const
    {
      r.length_ = item_.calcContentWidth();
      r.origin_ =
        item_.anchors_.left_.isValid() ? item_.left() :
        item_.anchors_.right_.isValid() ? item_.right() - r.length_ :
        item_.anchors_.hcenter_.isValid() ? item_.hcenter() - r.length_ * 0.5 :
        std::numeric_limits<double>::max();

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->xExtent();
        r.expand(footprint);
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
    void evaluate(Segment & r) const
    {
      r.length_ = item_.calcContentHeight();
      r.origin_ =
        item_.anchors_.top_.isValid() ? item_.top() :
        item_.anchors_.bottom_.isValid() ? item_.bottom() - r.length_ :
        item_.anchors_.vcenter_.isValid() ? item_.vcenter() - r.length_ * 0.5 :
        std::numeric_limits<double>::max();

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->yExtent();
        r.expand(footprint);
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
        YAE_ASSERT(item.anchors_.vcenter_.isValid() ||
                   !item.children_.empty());
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
  // Transition::Polyline::Polyline
  //
  Transition::Polyline::Polyline(double duration_sec,
                                 double v0,
                                 double v1,
                                 unsigned int n):
    duration_ns_(duration_sec * 1e+9)
  {
    pt_[0.0] = v0;
    pt_[1.0] = v1;
    tween_smooth(n);
  }

  //----------------------------------------------------------------
  // Transition::Polyline::tween_smooth
  //
  Transition::Polyline &
  Transition::Polyline::tween_smooth(unsigned int n)
  {
    if (n < 1 || pt_.size() != 2)
    {
      YAE_ASSERT(n < 1);
      return *this;
    }

    const Point & p0 = *(pt_.begin());
    double t0 = p0.first;
    double v0 = p0.second;

    const Point & p1 = *(pt_.rbegin());
    double t1 = p1.first;
    double v1 = p1.second;

    double dv = (v1 - v0);
    if (dv == 0.0)
    {
      // no need to blend between identical values, skip it
      return *this;
    }

    double dt = t1 - t0;
    for (unsigned int i = 0; i < n; i++)
    {
      double s = double(i + 1) / double(n + 1);
      double c = cos(M_PI * (1.0 + s));
      double v = v0 + dv * 0.5 * (1.0 + c);
      pt_[t0 + dt * s] = v;
    }

    return *this;
  }

  //----------------------------------------------------------------
  // Transition::evaluate
  //
  double
  Transition::Polyline::evaluate(double t) const
  {
    YAE_ASSERT(duration_ns_ && !pt_.empty());

    // get iterator to the first element that is greater than given key (t);
    std::map<double, double>::const_iterator i = pt_.upper_bound(t);

    if (i == pt_.begin())
    {
      // clamp to start:
      return i->second;
    }

    if (i == pt_.end())
    {
      // clamp to end:
      return pt_.rbegin()->second;
    }

    // interpolate:
    double t1 = i->first;
    double v1 = i->second;
    --i;
    double t0 = i->first;
    double v0 = i->second;

    double dt = (t1 - t0);
    double s = (t - t0) / dt;
    double v = s * v1 + (1.0 - s) * v0;

    return v;
  }

  //----------------------------------------------------------------
  // Transition::Transition
  //
  Transition::Transition(double duration,
                         double v0,
                         double v1,
                         unsigned int n_spinup,
                         unsigned int n_steady,
                         unsigned int n_spindown)
  {
    init(Polyline(duration, v0, v1, n_spinup),
         Polyline(duration, v1, v1, n_steady),
         Polyline(duration, v1, v0, n_spindown));
  }

  //----------------------------------------------------------------
  // Transition::Transition
  //
  Transition::Transition(const Polyline & spinup,
                         const Polyline & steady,
                         const Polyline & spindown)
  {
    init(spinup, steady, spindown);
  }

  //----------------------------------------------------------------
  // Transition::init
  //
  void
  Transition::init(const Polyline & spinup,
                   const Polyline & steady,
                   const Polyline & spindown)
  {
    spinup_ = spinup;
    steady_ = steady;
    spindown_ = spindown;
    duration_ns_ = 0.0;

    if (spinup.duration_ns_ > 0.0)
    {
      segment_[duration_ns_] = &spinup_;
      duration_ns_ += spinup.duration_ns_;
    }

    if (steady.duration_ns_ > 0.0)
    {
      segment_[duration_ns_] = &steady_;
      duration_ns_ += steady.duration_ns_;
    }

    if (spindown.duration_ns_ > 0.0)
    {
      segment_[duration_ns_] = &spindown_;
      duration_ns_ += spindown.duration_ns_;
    }
  }

  //----------------------------------------------------------------
  // Transition::is_done
  //
  bool
  Transition::is_done() const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    if (now < t0_)
    {
      return false;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();
    bool done = (duration_ns_ < t);
    return done;
  }

  //----------------------------------------------------------------
  // Transition::is_steady
  //
  bool
  Transition::is_steady() const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    if (now < t0_)
    {
      return false;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();
    if (t < spinup_.duration_ns_)
    {
      return false;
    }

    t -= spinup_.duration_ns_;
    bool steady = (t < steady_.duration_ns_);
    return steady;
  }

  //----------------------------------------------------------------
  // Transition::in_spinup
  //
  bool
  Transition::in_spinup() const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    if (now < t0_)
    {
      return false;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();
    bool spinup = (t < spinup_.duration_ns_);
    return spinup;
  }

  //----------------------------------------------------------------
  // Transition::in_spindown
  //
  bool
  Transition::in_spindown() const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    if (now < t0_)
    {
      return false;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();
    if (t < spinup_.duration_ns_)
    {
      return false;
    }

    t -= spinup_.duration_ns_;
    bool spindown = (steady_.duration_ns_ <= t);
    return spindown;
  }

  //----------------------------------------------------------------
  // Transition::in_progress
  //
  bool
  Transition::in_progress() const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    if (now < t0_)
    {
      return false;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();
    bool done = (duration_ns_ < t);
    return !done;
  }

  //----------------------------------------------------------------
  // Transition::get_state
  //
  Transition::State
  Transition::get_state(const TimePoint & now,
                        const Polyline *& seg,
                        double & seg_pos) const
  {
    if (now < t0_)
    {
      seg = segment_.begin()->second;
      seg_pos = 0.0;
      return Transition::kPending;
    }

    boost::chrono::nanoseconds ns = now - t0_;
    boost::uint64_t t = ns.count();

    boost::uint64_t t1 = spinup_.duration_ns_;
    if (t < t1)
    {
      seg = &spinup_;
      seg_pos = double(t) / double(spinup_.duration_ns_);
      return Transition::kSpinup;
    }

    boost::uint64_t t0 = t1;
    t1 += steady_.duration_ns_;
    if (t < t1)
    {
      seg = &steady_;
      seg_pos = double(t - t0) / double(steady_.duration_ns_);
      return Transition::kSteady;
    }

    t0 = t1;
    t1 += spindown_.duration_ns_;
    if (t < t1)
    {
      seg = &spindown_;
      seg_pos = double(t - t0) / double(spindown_.duration_ns_);
      return Transition::kSpindown;
    }

    seg = segment_.rbegin()->second;
    seg_pos = 1.0;
    return Transition::kDone;
  }

  //----------------------------------------------------------------
  // Transition::start
  //
  void
  Transition::start()
  {
    TimePoint now = boost::chrono::steady_clock::now();
    const Polyline * seg = NULL;
    double seg_pos = 0.0;
    Transition::State current_state = get_state(now, seg, seg_pos);

    if (current_state == Transition::kSteady)
    {
      t0_ = now - boost::chrono::nanoseconds(spinup_.duration_ns_);
    }
    else if (current_state == Transition::kSpindown)
    {
      boost::uint64_t skip_spinup_ns =
        boost::uint64_t(double(spinup_.duration_ns_) * (1.0 - seg_pos));
      t0_ = now - boost::chrono::nanoseconds(skip_spinup_ns);
    }
    else if (current_state != Transition::kSpinup)
    {
      t0_ = now;
    }
  }

  //----------------------------------------------------------------
  // Transition::start_from_steady
  //
  void
  Transition::start_from_steady()
  {
    TimePoint now = boost::chrono::steady_clock::now();
    t0_ = now - boost::chrono::nanoseconds(spinup_.duration_ns_);
  }

  //----------------------------------------------------------------
  // Transition::evaluate
  //
  void
  Transition::evaluate(double & result) const
  {
    TimePoint now = boost::chrono::steady_clock::now();
    const Polyline * seg = NULL;
    double seg_pos = 0.0;

    get_state(now, seg, seg_pos);
    YAE_ASSERT(seg);

    if (seg)
    {
      result = seg->evaluate(seg_pos);
    }
  }

  //----------------------------------------------------------------
  // Transition::get_periodic_value
  //
  double
  Transition::get_periodic_value(double period_scale,
                                 boost::uint64_t offset_ns) const
  {
    TimePoint now = boost::chrono::steady_clock::now();

    boost::uint64_t period_ns = (boost::uint64_t)
      // (std::max(1.0, double(duration_ns_ * period_scale)));
      duration_ns_;

    boost::uint64_t elapsed_ns = boost::chrono::nanoseconds(now - t0_).count();
    elapsed_ns = (boost::uint64_t)(double(elapsed_ns) / period_scale);
    elapsed_ns = (elapsed_ns + offset_ns) % period_ns;

    TimePoint t = t0_ + boost::chrono::nanoseconds(elapsed_ns);

    const Polyline * seg = NULL;
    double seg_pos = 0.0;
    get_state(t, seg, seg_pos);
    YAE_ASSERT(seg);

    double v = seg ? seg->evaluate(seg_pos) : 0.0;
    return v;
  }


  //----------------------------------------------------------------
  // Margins::Refs::Refs
  //
  Margins::Refs::Refs()
  {
    set(ItemRef::constant(0));
  }

  //----------------------------------------------------------------
  // Margins::Refs::set
  //
  void
  Margins::Refs::set(const ItemRef & ref)
  {
    left_ = ref;
    right_ = ref;
    top_ = ref;
    bottom_ = ref;
  }

  //----------------------------------------------------------------
  // Margins::Refs::uncache
  //
  void
  Margins::Refs::uncache()
  {
    left_.uncache();
    right_.uncache();
    top_.uncache();
    bottom_.uncache();
  }

  //----------------------------------------------------------------
  // Margins::get_refs
  //
  Margins::Refs &
  Margins::get_refs() const
  {
    if (!refs_)
    {
      refs_.reset(new Refs());
    }

    return *refs_;
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
  // InputHandler::InputHandler
  //
  InputHandler::InputHandler(InputArea * inputArea, const TVec2D & csysOrigin):
    csysOrigin_(csysOrigin)
  {
    if (inputArea)
    {
      ItemPtr itemPtr = inputArea->self_.lock();
      InputAreaPtr inputAreaPtr = itemPtr.cast<InputArea>();

      if (!inputAreaPtr)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("failed to acquire input area pointer");
      }

      // store a weak pointer:
      input_ = inputAreaPtr;
    }
  }

  //----------------------------------------------------------------
  // InputHandler::inputArea
  //
  InputArea *
  InputHandler::inputArea() const
  {
    return input_.lock().get();
  }


  //----------------------------------------------------------------
  // VisibleItem::VisibleItem
  //
  VisibleItem::VisibleItem(Item * item, const TVec2D & csysOrigin):
    csysOrigin_(csysOrigin)
  {
    if (item)
    {
      yae::shared_ptr<Item> item_ptr = item->self_.lock();

      if (!item_ptr)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("failed to acquire item pointer");
      }

      item_ = item_ptr;
    }
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

    uncacheSelf();
  }

  //----------------------------------------------------------------
  // Item::uncacheSelf
  //
  void
  Item::uncacheSelf()
  {
    anchors_.uncache();
    margins_.uncache();
    width_.uncache();
    height_.uncache();
    xContent_.uncache();
    yContent_.uncache();
    xExtent_.uncache();
    yExtent_.uncache();
    visible_.uncache();

    notifyObservers(kOnUncache);
  }

  //----------------------------------------------------------------
  // Item::uncacheSelfAndChildren
  //
  void
  Item::uncacheSelfAndChildren()
  {
    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->uncacheSelfAndChildren();
    }

    uncacheSelf();
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
    else if (property == kPropertyAnchorLeft)
    {
      value = anchors_.left_.get();
    }
    else if (property == kPropertyAnchorRight)
    {
      value = anchors_.right_.get();
    }
    else if (property == kPropertyAnchorTop)
    {
      value = anchors_.top_.get();
    }
    else if (property == kPropertyAnchorBottom)
    {
      value = anchors_.bottom_.get();
    }
    else if (property == kPropertyAnchorHCenter)
    {
      value = anchors_.hcenter_.get();
    }
    else if (property == kPropertyAnchorVCenter)
    {
      value = anchors_.vcenter_.get();
    }
    else if (property == kPropertyMarginLeft)
    {
      value = margins_.left();
    }
    else if (property == kPropertyMarginRight)
    {
      value = margins_.right();
    }
    else if (property == kPropertyMarginTop)
    {
      value = margins_.top();
    }
    else if (property == kPropertyMarginBottom)
    {
      value = margins_.bottom();
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
      return;
    }

    YAE_ASSERT(false);
    throw std::runtime_error("unsupported item property of type <bool>");
    value = false;
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
  // Item::get
  //
  void
  Item::get(Property property, TVar & value) const
  {
    YAE_ASSERT(false);
    throw std::runtime_error("unsupported item property of type <TVar>");
    value = TVar();
  }

  //----------------------------------------------------------------
  // Item::xContentDisableCaching
  //
  void
  Item::xContentDisableCaching()
  {
    xContent_.disableCaching();
  }

  //----------------------------------------------------------------
  // Item::yContentDisableCaching
  //
  void
  Item::yContentDisableCaching()
  {
    yContent_.disableCaching();
  }

  //----------------------------------------------------------------
  // Item::xExtentDisableCaching
  //
  void
  Item::xExtentDisableCaching()
  {
    xExtent_.disableCaching();
  }

  //----------------------------------------------------------------
  // Item::yExtentDisableCaching
  //
  void
  Item::yExtentDisableCaching()
  {
    yExtent_.disableCaching();
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
      l += margins_.left();
      r -= margins_.right();

      double w = r - l;
      width_.cache(w);
      return w;
    }

    // width is based on horizontal footprint of item content:
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
        YAE_ASSERT(anchors_.hcenter_.isValid() || !children_.empty());
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
      t += margins_.top();
      b -= margins_.bottom();

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
      l += margins_.left();
      return l;
    }

    if (anchors_.right_.isValid())
    {
      double w = width();
      double r = anchors_.right_.get();
      double l = (r - margins_.right()) - w;
      return l;
    }

    if (anchors_.hcenter_.isValid())
    {
      double w = width();
      double c = anchors_.hcenter_.get();
      double l = c - 0.5 * w;
      return l;
    }

    const Segment & xContent = this->xContent();
    if (!xContent.isEmpty())
    {
      double l = xContent.origin_;
      l += margins_.left();
      return l;
    }

    return margins_.left();
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
      r -= margins_.right();
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
      t += margins_.top();
      return t;
    }

    if (anchors_.bottom_.isValid())
    {
      double h = height();
      double b = anchors_.bottom_.get();
      double t = (b - margins_.bottom()) - h;
      return t;
    }

    if (anchors_.vcenter_.isValid())
    {
      double h = height();
      double c = anchors_.vcenter_.get();
      double t = c - 0.5 * h;
      return t;
    }

    const Segment & yContent = this->yContent();
    if (!yContent.isEmpty())
    {
      double t = yContent.origin_;
      t += margins_.top();
      return t;
    }

    return margins_.top();
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
      b -= margins_.bottom();
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
      double ml = margins_.left();
      double mr = margins_.right();
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
      double mt = margins_.top();
      double mb = margins_.bottom();
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
    YAE_ASSERT(false);
    throw std::runtime_error(oss.str().c_str());
    return *this;
  }

  //----------------------------------------------------------------
  // Item::delAttr
  //
  bool
  Item::delAttr(const char * key)
  {
    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const Item & child = *(i->get());
      if (child.id_ == key)
      {
        YAE_ASSERT(!child.visible());
        children_.erase(i);
        return true;
      }
    }

    return false;
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
  // Item::getVisibleItems
  //
  void
  Item::getVisibleItems(// coordinate system origin of
                        // the item, expressed in the
                        // coordinate system of the root item:
                        const TVec2D & itemCSysOrigin,

                        // point expressed in the coord.sys. of the item,
                        // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                        const TVec2D & itemCSysPoint,

                        // pass back visible items overlapping above point,
                        // along with its coord. system origin expressed
                        // in the coordinate system of the root item:
                        std::list<VisibleItem> & visibleItems)
  {
    if (!(visible() && overlaps(itemCSysPoint)))
    {
      return;
    }

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getVisibleItems(itemCSysOrigin, itemCSysPoint, visibleItems);
    }

    visibleItems.push_back(VisibleItem(this, itemCSysOrigin));
  }

  //----------------------------------------------------------------
  // Item::onFocus
  //
  void
  Item::onFocus()
  {
    notifyObservers(kOnFocus);
  }

  //----------------------------------------------------------------
  // Item::onFocusOut
  //
  void
  Item::onFocusOut()
  {
    notifyObservers(kOnFocusOut);
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
  // Item::visibleInRegion
  //
  bool
  Item::visibleInRegion(const Segment & xregion,
                        const Segment & yregion) const
  {
    if (!Item::visible())
    {
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yregion.disjoint(yfootprint))
    {
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xregion.disjoint(xfootprint))
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Item::paintChildren
  //
  void
  Item::paintChildren(const Segment & xregion,
                      const Segment & yregion,
                      Canvas * canvas) const
  {
    // only paint childen that are visible in the painted region, in Z order:
    std::map<double, std::list<ItemPtr> > order;
    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      if (child->visibleInRegion(xregion, yregion))
      {
        double z = child->attr<double>("z-order", 0.0);
        order[z].push_back(child);
      }
      else
      {
        child->unpaint();
      }
    }

    // NOTE: reverse order doesn't reverse z-order,
    // it only affects painting of items at the same z-order level:
    bool paint_in_reverse = this->attr<bool>("paint-in-reverse-order", false);

    for (std::map<double, std::list<ItemPtr> >::iterator
           i = order.begin(); i != order.end(); ++i)
    {
      std::list<ItemPtr> & children = i->second;
      if (paint_in_reverse)
      {
        while (!children.empty())
        {
          const ItemPtr & child = children.back();
          child->paint(xregion, yregion, canvas);
          children.pop_back();
        }
      }
      else
      {
        while (!children.empty())
        {
          const ItemPtr & child = children.front();
          child->paint(xregion, yregion, canvas);
          children.pop_front();
        }
      }
    }
  }

  //----------------------------------------------------------------
  // Item::paint
  //
  bool
  Item::paint(const Segment & xregion,
              const Segment & yregion,
              Canvas * canvas) const
  {
    if (!visibleInRegion(xregion, yregion))
    {
      unpaint();
      return false;
    }

    this->paintContent();
    painted_ = true;

    paintChildren(xregion, yregion, canvas);

    notifyObservers(kOnPaint);
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

    notifyObservers(kOnUnpaint);
  }

  //----------------------------------------------------------------
  // Item::notifyObservers
  //
  void
  Item::notifyObservers(Item::Event e) const
  {
    if (!eo_)
    {
      return;
    }

    TEventObservers::const_iterator found = eo_->find(e);
    if (found == eo_->end())
    {
      return;
    }

    const std::set<TObserverPtr> & observers = found->second;
    for (std::set<TObserverPtr>::const_iterator i = observers.begin();
         i != observers.end(); ++i)
    {
      TObserverPtr observer = *i;
      observer->observe(*this, e);
    }
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Item::dump
  //
  void
  Item::dump(std::ostream & os, const std::string & indent) const
  {
    os << indent
       << "id: " << id_
       << std::endl;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->dump(os, indent + "  ");
    }
  }
#endif


  //----------------------------------------------------------------
  // TransitionItem::TransitionItem
  //
  TransitionItem::TransitionItem(const char * id,
                                 const Transition::Polyline & spinup,
                                 const Transition::Polyline & steady,
                                 const Transition::Polyline & spindown):
    Item(id),
    expression_(Item::addExpr(new Transition(spinup, steady, spindown))),
    transition_(*(dynamic_cast<Transition *>
                  (const_cast<IProperties<double> *>(expression_.ref()))))
  {
    override_ = expression_;
  }

  //----------------------------------------------------------------
  // TransitionItem::pause
  //
  void
  TransitionItem::pause(const ItemRef & v)
  {
    override_ = v;
  }

  //----------------------------------------------------------------
  // TransitionItem::is_paused
  //
  bool
  TransitionItem::is_paused() const
  {
    bool paused = (override_.ref() != expression_.ref());
    return paused;
  }

  //----------------------------------------------------------------
  // TransitionItem::start
  //
  void
  TransitionItem::start()
  {
    bool paused = is_paused();
    if (paused)
    {
      if (override_.get() == transition_.get_steady_value())
      {
        // skip the intro:
        transition_.start_from_steady();
      }
      else
      {
        // start from the beginning:
        transition_.start();
      }

      override_ = expression_;
    }
    else
    {
      transition_.start();
    }
  }

  //----------------------------------------------------------------
  // TransitionItem::get
  //
  void
  TransitionItem::get(Property property, double & value) const
  {
    if (property == kPropertyTransition)
    {
      value = override_.get();
    }
    else
    {
      Item::get(property, value);
    }
  }

  //----------------------------------------------------------------
  // TransitionItem::uncache
  //
  void
  TransitionItem::uncache()
  {
    override_.uncache();
    expression_.uncache();
    Item::uncache();
  }

}
