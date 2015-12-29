// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost library:
#include <boost/chrono.hpp>

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
    TPrivate(const Canvas::ILayer & canvasLayer, Item & scrollbar):
      canvasLayer_(canvasLayer),
      scrollbar_(scrollbar),
      startPos_(0.0),
      nsamples_(0),
      v0_(0.0)
    {}

    void addSample(double dt, double y)
    {
      int i = nsamples_ % TPrivate::kMaxSamples;
      samples_[i].x() = dt;
      samples_[i].y() = y;
      nsamples_++;
    }

    double estimateVelocity(int i0, int i1) const
    {
      if (nsamples_ < 2 || i0 == i1)
      {
        return 0.0;
      }

      const TVec2D & a = samples_[i0];
      const TVec2D & b = samples_[i1];

      double t0 = a.x();
      double t1 = b.x();
      double dt = t1 - t0;
      if (dt <= 0.0)
      {
        YAE_ASSERT(false);
        return 0.0;
      }

      double y0 = a.y();
      double y1 = b.y();
      double dy = y1 - y0;

      double v = dy / dt;
      return v;
    }

    double estimateVelocity(int nsamples) const
    {
      int n = std::min<int>(nsamples, nsamples_);
      int i0 = (nsamples_ - n) % TPrivate::kMaxSamples;
      int i1 = (nsamples_ - 1) % TPrivate::kMaxSamples;
      return estimateVelocity(i0, i1);
    }

    const Canvas::ILayer & canvasLayer_;
    Item & scrollbar_;
    double startPos_;

    // flicking animation parameters:
    QTimer timer_;
    boost::chrono::steady_clock::time_point tStart_;

    // for each sample point: x = t - tStart, y = dragEnd(t)
    enum { kMaxSamples = 5 };
    TVec2D samples_[kMaxSamples];
    std::size_t nsamples_;

    // velocity estimate based on available samples:
    double v0_;
  };

  //----------------------------------------------------------------
  // FlickableArea::FlickableArea
  //
  FlickableArea::FlickableArea(const char * id,
                               const Canvas::ILayer & canvasLayer,
                               Item & scrollbar):
    InputArea(id),
    p_(new TPrivate(canvasLayer, scrollbar))
  {
    bool ok = connect(&p_->timer_, SIGNAL(timeout()),
                      this, SLOT(onTimeout()));
    YAE_ASSERT(ok);
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
    p_->timer_.stop();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;
    double y = scrollview.position_ * yRange + sh * degrees / 360.0;
    double s = std::min<double>(1.0, std::max<double>(0.0, y / yRange));
    scrollview.position_ = s;

    p_->scrollbar_.uncache();
    p_->canvasLayer_.delegate()->requestRepaint();

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onPress
  //
  bool
  FlickableArea::onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
  {
    p_->timer_.stop();

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
    p_->timer_.stop();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;
    double dy = (rootCSysDragEnd.y() - rootCSysDragStart.y());
    double ds = dy / yRange;

    double s = std::min<double>(1.0, std::max<double>(0.0, p_->startPos_ + ds));
    scrollview.position_ = s;

    p_->scrollbar_.uncache();
    p_->canvasLayer_.delegate()->requestRepaint();

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
    double dy = (rootCSysDragEnd.y() - rootCSysDragStart.y());

    double secondsElapsed = boost::chrono::duration<double>
      (boost::chrono::steady_clock::now() - p_->tStart_).count();
    p_->addSample(secondsElapsed, rootCSysDragEnd.y());

    double vi = p_->estimateVelocity(3);
    p_->v0_ = dy / secondsElapsed;

    double k = vi / p_->v0_;

#if 0
    std::cerr << "FIXME: v0: " << p_->v0_ << ", k: " << k << std::endl;
#endif

    if (k > 0.1)
    {
      p_->timer_.start(16);
      onTimeout();
    }

    p_->nsamples_ = 0;
    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onTimeout
  //
  void
  FlickableArea::onTimeout()
  {
    if (!p_->timer_.isActive())
    {
      // do not interfere with velocity estimation:
      return;
    }

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;

    boost::chrono::steady_clock::time_point tNow =
      boost::chrono::steady_clock::now();

    double secondsElapsed = boost::chrono::duration<double>
      (tNow - p_->tStart_).count();

    double v0 = p_->v0_;
    double dy = v0 * secondsElapsed;
    double ds = dy / yRange;
    double s =
      std::min<double>(1.0, std::max<double>(0.0, scrollview.position_ + ds));

    scrollview.position_ = s;
    p_->tStart_ = tNow;
    {
      TMakeCurrentContext currentContext(*(p_->canvasLayer_.context()));
      p_->scrollbar_.uncache();
    }

    p_->canvasLayer_.delegate()->requestRepaint();

    if (s == 0.0 || s == 1.0 || v0 == 0.0)
    {
      // stop the animation:
      p_->timer_.stop();
      return;
    }

    // deceleration (friction) coefficient:
    const double k = sh * 0.1;

    double v1 = v0 * (1.0 - k * secondsElapsed / fabs(v0));
    if (v0 * v1 < 0.0)
    {
      // bounce detected:
      p_->timer_.stop();
      return;
    }

#if 0
    std::cerr
      << "FIXME: v0: " << v0
      << ", v1: " << v1
      << ", dv: " << v1 - v0 << std::endl;
#endif

    YAE_ASSERT(fabs(v1) < fabs(v0));
    p_->v0_ = v1;
  }

}
