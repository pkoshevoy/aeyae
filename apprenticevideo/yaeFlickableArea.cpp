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
             SliderDrag * vslider,
             SliderDrag * hslider):
      itemView_(itemView),
      flickable_(flickable),
      vslider_(vslider),
      hslider_(hslider),
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

      if (n > 1)
      {
        int prev = (nsamples_ - 2) % TPrivate::kMaxSamples;
        int curr = (nsamples_ - 1) % TPrivate::kMaxSamples;

        const double & t0 = t_[prev];
        const double & t1 = t_[curr];
        double dt = t1 - t0;

        if (dt > 1e-1)
        {
          // significant pause between final 2 samples,
          // probably not a flick but a simple drag-and-release:
          return 0.0;
        }
#if 0
        yae_debug << "FIXME: estimateVelocity, dt: " << dt;
#endif
      }

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
      if (vslider_)
      {
        vslider_->scrollbar_.uncache();
      }

      if (hslider_)
      {
        hslider_->scrollbar_.uncache();
      }
    }

    ItemView & itemView_;
    FlickableArea & flickable_;
    SliderDrag * vslider_;
    SliderDrag * hslider_;
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
                               SliderDrag & vslider):
    InputArea(id),
    p_(NULL)
  {
    p_ = new TPrivate(itemView, *this, &vslider, NULL);
  }

  //----------------------------------------------------------------
  // FlickableArea::FlickableArea
  //
  FlickableArea::FlickableArea(const char * id,
                               ItemView & itemView,
                               SliderDrag * vslider,
                               SliderDrag * hslider):
    InputArea(id),
    p_(NULL)
  {
    p_ = new TPrivate(itemView, *this, vslider, hslider);
  }

  //----------------------------------------------------------------
  // FlickableArea::~FlickableArea
  //
  FlickableArea::~FlickableArea()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // FlickableArea::setHorSlider
  //
  void
  FlickableArea::setHorSlider(SliderDrag * hslider)
  {
    p_->hslider_ = hslider;
  }

  //----------------------------------------------------------------
  // FlickableArea::setVerSlider
  //
  void
  FlickableArea::setVerSlider(SliderDrag * vslider)
  {
    p_->vslider_ = vslider;
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

    Scrollview & vsv = find_vscrollview();
    double sh = vsv.height();
    double ch = vsv.content_->height();
    double yRange = sh - ch;
    double y = vsv.position_y() * yRange + sh * degrees / 360.0;
    double s = std::min<double>(1.0, std::max<double>(0.0, y / yRange));
    vsv.set_position_y(s);

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

    Scrollview & hsv = find_hscrollview();
    p_->startPos_.set_x(hsv.position_x());

    Scrollview & vsv = find_vscrollview();
    p_->startPos_.set_y(vsv.position_y());

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

    Scrollview & hsv = find_hscrollview();
    Scrollview & vsv = find_vscrollview();

    double sw = hsv.width();
    double cw = hsv.content_->width();
    double xRange = sw - cw;

    double sh = vsv.height();
    double ch = vsv.content_->height();
    double yRange = sh - ch;

    TVec2D drag = rootCSysDragEnd - rootCSysDragStart;
    double tx = drag.x() / xRange;
    double ty = drag.y() / yRange;

    TVec2D pos = p_->startPos_ + TVec2D(tx, ty);
    pos.clamp(0.0, 1.0);

    hsv.set_position_x(pos.x());
    vsv.set_position_y(pos.y());

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
    yae_debug << "FIXME: v0: " << p_->v0_ << ", k: " << k;
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

    Scrollview & hsv = find_hscrollview();
    Scrollview & vsv = find_vscrollview();

    double sw = hsv.width();
    double cw = hsv.content_->width();
    double xRange = sw - cw;

    double sh = vsv.height();
    double ch = vsv.content_->height();
    double yRange = sh - ch;

    boost::chrono::steady_clock::time_point tNow =
      boost::chrono::steady_clock::now();

    double secondsElapsed = boost::chrono::duration<double>
      (tNow - p_->tStart_).count();

    TVec2D v = p_->v0_ * secondsElapsed;
    v.set_x(v.x() / xRange);
    v.set_y(v.y() / yRange);

    TVec2D p0(hsv.position_x(), vsv.position_y());
    TVec2D p1 = p0 + v;
    p1.clamp(0.0, 1.0);

    TVec2D diff = p1 - p0;
    hsv.set_position_x(p1.x());
    vsv.set_position_y(p1.y());
#if 0
    yae_debug
      << "FIXME: pos: " << p1
      << ", xrange: " << xRange
      << ", yrange: " << yRange;
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
    yae_debug
      << "FIXME: v0: " << p_->v0_
      << ", v1: " << v1
      << ", dv: " << v1 - p_->v0_;
#endif

    YAE_ASSERT(v1.normSqrd() < p_->v0_.normSqrd());
    p_->v0_ = v1;
  }

  //----------------------------------------------------------------
  // FlickableArea::find_hscrollview
  //
  Scrollview &
  FlickableArea::find_hscrollview() const
  {
    Scrollview & hsv =
      p_->hslider_ ? p_->hslider_->scrollview_ :
      p_->vslider_ ? p_->vslider_->scrollview_ :
      Item::ancestor<Scrollview>();
    return hsv;
  }

  //----------------------------------------------------------------
  // FlickableArea::find_vscrollview
  //
  Scrollview &
  FlickableArea::find_vscrollview() const
  {
    Scrollview & vsv =
      p_->vslider_ ? p_->vslider_->scrollview_ :
      p_->hslider_ ? p_->hslider_->scrollview_ :
      Item::ancestor<Scrollview>();
    return vsv;
  }

}
