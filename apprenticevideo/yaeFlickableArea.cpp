// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost library:
#ifndef Q_MOC_RUN
#include <boost/chrono.hpp>
#endif

// Qt library:
#include <QApplication>
#include <QTimer>

// local interfaces:
#include "yaeCanvas.h"
#include "yaeFlickableArea.h"
#include "yaeScrollview.h"


namespace yae
{

  //----------------------------------------------------------------
  // FlickableArea::TPrivate
  //
  struct FlickableArea::TPrivate
  {
    //----------------------------------------------------------------
    // Animator
    //
    struct Animator : public ItemView::IAnimator
    {
      Animator(FlickableArea & flickable):
        flickable_(flickable)
      {}

      // virtual:
      void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
      {
        flickable_.animate();
      }

      FlickableArea & flickable_;
    };

    TPrivate(ItemView & itemView,
             FlickableArea & flickable,
             Item * vscrollbar,
             Item * hscrollbar):
      itemView_(itemView),
      flickable_(flickable),
      vscrollbar_(vscrollbar),
      hscrollbar_(hscrollbar),
      estimating_(false),
      nsamples_(0),
      v0_(0.0)
    {
      animator_.reset(new Animator(flickable));
    }

    void addSample(double t, const TVec2D & pos)
    {
      int i = nsamples_ % TPrivate::kMaxSamples;
      t_[i] = t;
      pos_[i] = pos;
      nsamples_++;
    }

    double estimateVelocity(int i0, int i1) const
    {
      if (nsamples_ < 2 || i0 == i1)
      {
        return 0.0;
      }

      const double & t0 = t_[i0];
      const double & t1 = t_[i1];

      double dt = t1 - t0;
      if (dt <= 0.0)
      {
        YAE_ASSERT(false);
        return 0.0;
      }

      const TVec2D & a = pos_[i0];
      const TVec2D & b = pos_[i1];
      TVec2D ab = b - a;
      double norm_ab = ab.norm();

      double v = norm_ab / dt;
      return v;
    }

    double estimateVelocity(int nsamples) const
    {
      int n = std::min<int>(nsamples, nsamples_);
      int i0 = (nsamples_ - n) % TPrivate::kMaxSamples;
      int i1 = (nsamples_ - 1) % TPrivate::kMaxSamples;
      return estimateVelocity(i0, i1);
    }

    void requestAnimate()
    {
      itemView_.addAnimator(animator_);
    }

    void dontAnimate()
    {
      itemView_.delAnimator(animator_);
      estimating_ = true;
    }

    void uncacheScrollbars()
    {
      if (vscrollbar_)
      {
        vscrollbar_->uncache();
      }

      if (hscrollbar_)
      {
        hscrollbar_->uncache();
      }
    }

    ItemView & itemView_;
    FlickableArea & flickable_;
    Item * vscrollbar_;
    Item * hscrollbar_;
    TVec2D startPos_;
    bool estimating_;

    // flicking animation parameters:
    ItemView::TAnimatorPtr animator_;
    boost::chrono::steady_clock::time_point tStart_;

    // for each sample point: x = t - tStart, y = dragEnd(t)
    enum { kMaxSamples = 5 };
    double t_[kMaxSamples];
    TVec2D pos_[kMaxSamples];
    std::size_t nsamples_;

    // velocity estimate based on available samples:
    TVec2D v0_;
  };

  //----------------------------------------------------------------
  // FlickableArea::FlickableArea
  //
  FlickableArea::FlickableArea(const char * id,
                               ItemView & itemView,
                               Item & vscrollbar):
    InputArea(id),
    p_(NULL)
  {
    p_ = new TPrivate(itemView, *this, &vscrollbar, NULL);
  }

  //----------------------------------------------------------------
  // FlickableArea::FlickableArea
  //
  FlickableArea::FlickableArea(const char * id,
                               ItemView & itemView,
                               Item * vscrollbar,
                               Item * hscrollbar):
    InputArea(id),
    p_(NULL)
  {
    p_ = new TPrivate(itemView, *this, vscrollbar, hscrollbar);
  }

  //----------------------------------------------------------------
  // FlickableArea::~FlickableArea
  //
  FlickableArea::~FlickableArea()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // FlickableArea::onScroll
  //
  bool
  FlickableArea::onScroll(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint,
                          double degrees)
  {
    p_->dontAnimate();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_->height();
    double yRange = sh - ch;
    double y = scrollview.position_.y() * yRange + sh * degrees / 360.0;
    double s = std::min<double>(1.0, std::max<double>(0.0, y / yRange));
    scrollview.position_.set_y(s);

    p_->uncacheScrollbars();
    p_->itemView_.delegate()->requestRepaint();

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onPress
  //
  bool
  FlickableArea::onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
  {
    p_->dontAnimate();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    p_->startPos_ = scrollview.position_;
    p_->tStart_ = boost::chrono::steady_clock::now();
    p_->nsamples_ = 0;
    p_->addSample(0.0, rootCSysPoint.y());

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onDrag
  //
  bool
  FlickableArea::onDrag(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd)
  {
    p_->dontAnimate();

    Scrollview & scrollview = Item::ancestor<Scrollview>();

    double sw = scrollview.width();
    double cw = scrollview.content_->width();
    double xRange = sw - cw;

    double sh = scrollview.height();
    double ch = scrollview.content_->height();
    double yRange = sh - ch;

    TVec2D drag = rootCSysDragEnd - rootCSysDragStart;
    double tx = drag.x() / xRange;
    double ty = drag.y() / yRange;

    TVec2D pos = p_->startPos_ + TVec2D(tx, ty);
    pos.clamp(0.0, 1.0);

    scrollview.position_ = pos;

    p_->uncacheScrollbars();
    p_->itemView_.delegate()->requestRepaint();

    double secondsElapsed = boost::chrono::duration<double>
      (boost::chrono::steady_clock::now() - p_->tStart_).count();
    p_->addSample(secondsElapsed, rootCSysDragEnd.y());

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onDragEnd
  //
  bool
  FlickableArea::onDragEnd(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd)
  {
    TVec2D v = rootCSysDragEnd - rootCSysDragStart;

    double secondsElapsed = boost::chrono::duration<double>
      (boost::chrono::steady_clock::now() - p_->tStart_).count();
    p_->addSample(secondsElapsed, rootCSysDragEnd);

    double vi = p_->estimateVelocity(3);
    p_->v0_ = v * (1.0 / secondsElapsed);

    double k = vi / p_->v0_.norm();

#if 0
    std::cerr << "FIXME: v0: " << p_->v0_ << ", k: " << k << std::endl;
#endif

    if (k > 0.1)
    {
      p_->estimating_ = false;
      p_->requestAnimate();
      animate();
    }

    p_->nsamples_ = 0;
    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::isAnimating
  //
  bool
  FlickableArea::isAnimating() const
  {
    return p_->estimating_ == false;
  }

  //----------------------------------------------------------------
  // FlickableArea::stopAnimating
  //
  void
  FlickableArea::stopAnimating()
  {
    p_->dontAnimate();
  }

  //----------------------------------------------------------------
  // FlickableArea::onTimeout
  //
  void
  FlickableArea::animate()
  {
    if (p_->estimating_)
    {
      // do not interfere with velocity estimation:
      return;
    }

    Scrollview & scrollview = Item::ancestor<Scrollview>();

    double sw = scrollview.width();
    double cw = scrollview.content_->width();
    double xRange = sw - cw;

    double sh = scrollview.height();
    double ch = scrollview.content_->height();
    double yRange = sh - ch;

    boost::chrono::steady_clock::time_point tNow =
      boost::chrono::steady_clock::now();

    double secondsElapsed = boost::chrono::duration<double>
      (tNow - p_->tStart_).count();

    TVec2D v = p_->v0_ * secondsElapsed;
    v.set_x(v.x() / xRange);
    v.set_y(v.y() / yRange);

    TVec2D pos = scrollview.position_ + v;
    pos.clamp(0.0, 1.0);

    TVec2D diff = pos - scrollview.position_;
    scrollview.position_ = pos;
#if 0
    std::cerr
      << "FIXME: pos: " << pos
      << ", xrange: " << xRange
      << ", yrange: " << yRange
      << std::endl;
#endif

    p_->tStart_ = tNow;
    {
      TMakeCurrentContext currentContext(*(p_->itemView_.context()));
      p_->uncacheScrollbars();
    }

    if (v.normSqrd() == 0.0 || diff.normSqrd() == 0.0)
    {
      // motion stopped, stop the animation:
      p_->dontAnimate();
      return;
    }

    // deceleration (friction) coefficient:
    const double k = sh * 0.1;

    double t = 1.0 - k * secondsElapsed / p_->v0_.norm();
    if (t < 0.0)
    {
      // bounce detected:
      p_->dontAnimate();
      return;
    }

    TVec2D v1 = p_->v0_ * t;

#if 0
    std::cerr
      << "FIXME: v0: " << p_->v0_
      << ", v1: " << v1
      << ", dv: " << v1 - p_->v0_ << std::endl;
#endif

    YAE_ASSERT(v1.normSqrd() < v0.normSqrd());
    p_->v0_ = v1;
  }

}
