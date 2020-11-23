// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun May  1 13:23:52 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// std includes:
#include <iostream>
#include <sstream>

// Qt includes:
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFontMetrics>
#include <QTime>

// local:
#include "yaeTimelineControls.h"


namespace yae
{

  //----------------------------------------------------------------
  // Marker::Marker
  //
  Marker::Marker():
    position_(0.0),
    positionAnchor_(0.0)
  {
    hotspot_[0] = 0;
    hotspot_[1] = 0;
  }

  //----------------------------------------------------------------
  // Marker::overlaps
  //
  bool
  Marker::overlaps(const QPoint & coords,

                   // these parameters are used to derive current
                   // marker position:
                   const int & xOrigin,
                   const int & yOrigin,
                   const int & unitLength) const
  {
    QRect bbox = image_.rect();

    int x0 = coords.x() - (xOrigin +
                           int(0.5 + unitLength * position_) -
                           hotspot_[0]);

    int y0 = coords.y() - (yOrigin - hotspot_[1]);

    // the mouse coordinates are tested with [-3, 3] margin
    // in order to make it easier to click on small handles:
    for (int i = -3; i <= 3; i++)
    {
      for (int j = -3; j <= 3; j++)
      {
        int x = x0 + i;
        int y = y0 + j;

        if (!bbox.contains(x, y))
        {
          continue;
        }

        QRgb rgba = image_.pixel(x, y);
        int alpha = qAlpha(rgba);

        if (alpha > 0)
        {
          return true;
        }
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Marker::setAnchor
  //
  void
  Marker::setAnchor()
  {
    positionAnchor_ = position_;
  }


  //----------------------------------------------------------------
  // TimelineControls::TimelineControls
  //
  TimelineControls::TimelineControls(QWidget * parent, Qt::WindowFlags f):
    QWidget(parent, f),
    activeMarker_(NULL),
    auxPlayhead_(NULL),
    auxDuration_(NULL),
    auxFocusWidget_(NULL),
    repaintTimer_(this)
  {
    padding_ = 8;
    lineWidth_ = 3;

    setMinimumHeight(padding_ * 2 + lineWidth_);
    setMinimumWidth(padding_ * 2 + 64);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(0, 0, 0));
    pal.setColor(QPalette::Text, QColor(200, 200, 200));
    setPalette(pal);

    // load graphics for direct manipulation handles:
    markerTimeIn_.image_ = QImage(":/images/timeIn.png");
    markerTimeOut_.image_ = QImage(":/images/timeOut.png");
    markerPlayhead_.image_ = QImage(":/images/playHead.png");

    // setup hotspots:
    markerTimeIn_.hotspot_[0] = markerTimeIn_.image_.width() - 1;
    markerTimeIn_.hotspot_[1] = 12;

    markerTimeOut_.hotspot_[0] = 0;
    markerTimeOut_.hotspot_[1] = 12;

    markerPlayhead_.hotspot_[0] = markerPlayhead_.image_.width() / 2;
    markerPlayhead_.hotspot_[1] = 8;

    // setup marker positions:
    markerTimeIn_.position_ = model_.timelineStart();
    markerTimeOut_.position_ = model_.timelineEnd();
    markerPlayhead_.position_ = model_.timelineStart();

    // current state of playback controls:
    currentState_ = TimelineControls::kIdle;

    // this is to avoid too much repainting:
    repaintTimer_.setSingleShot(true);
    repaintTimer_.setInterval(100);

    bool ok = connect(&repaintTimer_, SIGNAL(timeout()),
                      this, SLOT(repaintTimerExpired()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(modelChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(markerTimeInChanged()),
                 this, SLOT(modelTimeInChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(markerTimeOutChanged()),
                 this, SLOT(modelTimeOutChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(auxPlayheadChanged()),
                 this, SLOT(modelPlayheadChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(auxDurationChanged()),
                 this, SLOT(modelDurationChanged()));
    YAE_ASSERT(ok);

    model_.trimLeadingZeros(true);
  }

  //----------------------------------------------------------------
  // TimelineControls::~TimelineControls
  //
  TimelineControls::~TimelineControls()
  {}

  //----------------------------------------------------------------
  // TimelineControls::setAuxWidgets
  //
  void
  TimelineControls::setAuxWidgets(QLineEdit * playhead,
                                  QLineEdit * duration,
                                  QWidget * focusWidget)
  {
    auxFocusWidget_ = focusWidget;

    if (auxPlayhead_)
    {
      auxPlayhead_->disconnect();
    }

    if (auxDuration_)
    {
      auxDuration_->disconnect();
    }

    auxPlayhead_ = playhead;
    auxDuration_ = duration;

    QFont clockFont = font();

#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    clockFont.setHintingPreference(QFont::PreferFullHinting);
#endif
    clockFont.setFamily("Andale Mono, "
                        "Menlo, "
                        "Monaco, "
                        "Droid Sans Mono, "
                        "DejaVu Sans Mono, "
                        "Bitstream Vera Sans Mono, "
                        "Consolas, "
                        "Lucida Sans Typewriter, "
                        "Lucida Console, "
                        "Courier New");
    // clockFont.setBold(true);
    clockFont.setPixelSize(11);
    clockFont.setStyleHint(QFont::Monospace);
    clockFont.setFixedPitch(true);
    clockFont.setStyleStrategy((QFont::StyleStrategy)
                               (QFont::PreferOutline |
                                // QFont::PreferAntialias |
                                QFont::OpenGLCompatible));
    // clockFont.setWeight(QFont::Normal);

    QFontMetrics fm(clockFont);
    QSize bbox = fm.size(Qt::TextSingleLine, model_.clockTemplate());
    bbox += QSize(4, 8);

    bool ok = true;

    if (auxPlayhead_)
    {
      auxPlayhead_->setFont(clockFont);
      auxPlayhead_->setMinimumSize(bbox);
      auxPlayhead_->setMaximumSize(bbox);
      auxPlayhead_->setEnabled(model_.timelineDuration() > 0.0);

      ok = connect(auxPlayhead_, SIGNAL(returnPressed()),
                   this, SLOT(seekToAuxPlayhead()));
      YAE_ASSERT(ok);
    }

    if (auxDuration_)
    {
      auxDuration_->setFont(clockFont);
      auxDuration_->setMinimumSize(bbox);
      auxDuration_->setMaximumSize(bbox);
      auxDuration_->setEnabled(model_.timelineDuration() > 0.0);
    }

    modelChanged();
    modelPlayheadChanged();
    modelDurationChanged();
  }

  //----------------------------------------------------------------
  // TimelineControls::seekToAuxPlayhead
  //
  void
  TimelineControls::seekToAuxPlayhead()
  {
    QString hhmmssff = auxPlayhead_->text();
    model_.seekTo(hhmmssff);

    if (auxFocusWidget_)
    {
      auxFocusWidget_->setFocus();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::requestRepaint
  //
  void
  TimelineControls::requestRepaint()
  {
    if (isHidden())
    {
      return;
    }

    if (!repaintTimer_.isActive())
    {
      repaintTimer_.start();
      repaintTimerStartTime_.start();
    }
    else if (repaintTimerStartTime_.elapsed() > 500)
    {
      // this shouldn't happen, but for some reason it does on the Mac,
      // sometimes the timer remains active but the timeout signal is
      // never delivered; this is a workaround:
#ifndef NDEBUG
      yae_debug << "REPAINT TIMEOUT WAS LATE";
#endif
      repaintTimer_.stop();

      // updateAuxPlayhead(timelinePosition_);
      repaint();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::repaintTimerExpired
  //
  void
  TimelineControls::repaintTimerExpired()
  {
    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::modelChanged
  //
  void
  TimelineControls::modelChanged()
  {
    if (auxPlayhead_)
    {
      auxPlayhead_->setEnabled(model_.hasTimelineDuration());
    }

    if (auxDuration_)
    {
      auxDuration_->setEnabled(model_.hasTimelineDuration());
    }

    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::modelTimeInChanged
  //
  void
  TimelineControls::modelTimeInChanged()
  {
    markerTimeIn_.position_ = model_.markerTimeIn();
  }

  //----------------------------------------------------------------
  // TimelineControls::modelTimeOutChanged
  //
  void
  TimelineControls::modelTimeOutChanged()
  {
    markerTimeOut_.position_ = model_.markerTimeOut();
  }

  //----------------------------------------------------------------
  // TimelineControls::modelPlayheadChanged
  //
  void
  TimelineControls::modelPlayheadChanged()
  {
    // yae_debug << "playhead: " << model_.auxPlayhead().toUtf8().constData();

    markerPlayhead_.position_ = model_.markerPlayhead();

    if (auxPlayhead_)
    {
      if (!auxPlayhead_->isEnabled())
      {
        auxPlayhead_->setText(model_.clockTemplate());
      }
      else if (!auxPlayhead_->hasFocus())
      {
        auxPlayhead_->setText(model_.auxPlayhead());
      }
    }

    requestRepaint();
  }

  //----------------------------------------------------------------
  // TimelineControls::modelDurationChanged
  //
  void
  TimelineControls::modelDurationChanged()
  {
    // yae_debug << "duration: " << model_.auxDuration().toUtf8().constData();

    if (auxDuration_)
    {
      if (!auxDuration_->isEnabled())
      {
        auxDuration_->setText(model_.clockTemplate());
      }
      else if (model_.unknownDuration())
      {
        auxDuration_->setText(tr("N/A"));
      }
      else
      {
        auxDuration_->setText(model_.auxDuration());
      }
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::paintEvent
  //
  void
  TimelineControls::paintEvent(QPaintEvent * e)
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);

    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);

    if (!model_.hasTimelineDuration())
    {
      p.setBrush(QColor(0x40, 0x40, 0x40));
      p.drawRect(xOrigin,
                 yOriginPlayhead,
                 unitLength,
                 lineWidth_);
      return;
    }

    int inExt = int(0.5 + double(unitLength) * markerTimeIn_.position_);
    int outExt = int(0.5 + double(unitLength) * markerTimeOut_.position_);
    int playExt = int(0.5 + double(unitLength) * markerPlayhead_.position_);

    p.setBrush(QColor(0x80, 0x80, 0x80));
    p.drawRect(xOrigin,
               yOriginPlayhead,
               inExt,
               lineWidth_);

    p.setBrush(QColor(0x40, 0x80, 0xff));
    p.drawRect(xOrigin + inExt,
               yOriginPlayhead,
               outExt - inExt,
               lineWidth_);

    p.setBrush(QColor(0x80, 0x80, 0x80));
    p.drawRect(xOrigin + outExt,
               yOriginPlayhead,
               unitLength - outExt,
               lineWidth_);

    p.drawImage(xOrigin + inExt - markerTimeIn_.hotspot_[0],
                yOriginInOut - markerTimeIn_.hotspot_[1],
                markerTimeIn_.image_);

    p.drawImage(xOrigin + outExt - markerTimeOut_.hotspot_[0],
                yOriginInOut - markerTimeOut_.hotspot_[1],
                markerTimeOut_.image_);

    p.drawImage(xOrigin + playExt - markerPlayhead_.hotspot_[0],
                yOriginPlayhead - markerPlayhead_.hotspot_[1],
                markerPlayhead_.image_);
  }

  //----------------------------------------------------------------
  // TimelineControls::mousePressEvent
  //
  void
  TimelineControls::mousePressEvent(QMouseEvent * e)
  {
    if (!model_.hasTimelineDuration())
    {
      return;
    }

    QPoint pt = e->pos();

    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);

    dragStart_ = pt;
    markerTimeIn_.setAnchor();
    markerTimeOut_.setAnchor();
    markerPlayhead_.setAnchor();

    if (markerPlayhead_.overlaps(pt, xOrigin, yOriginPlayhead, unitLength))
    {
      // std::cout << "PLAYHEAD" << std::endl;
      currentState_ = kDraggingPlayheadMarker;
      activeMarker_ = &markerPlayhead_;
      model_.emitUserIsSeeking(true);
    }
    else if (markerTimeOut_.overlaps(pt, xOrigin, yOriginInOut, unitLength))
    {
      // std::cout << "OUT POINT" << std::endl;
      currentState_ = kDraggingTimeOutMarker;
      activeMarker_ = &markerTimeOut_;
    }
    else if (markerTimeIn_.overlaps(pt, xOrigin, yOriginInOut, unitLength))
    {
      // std::cout << "IN POINT" << std::endl;
      currentState_ = kDraggingTimeInMarker;
      activeMarker_ = &markerTimeIn_;
    }
    else
    {
      currentState_ = kIdle;
      activeMarker_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::mouseReleaseEvent
  //
  void
  TimelineControls::mouseReleaseEvent(QMouseEvent * e)
  {
    if (currentState_ == kDraggingPlayheadMarker)
    {
      model_.emitUserIsSeeking(false);
    }

    currentState_ = kIdle;
    activeMarker_ = NULL;
  }

  //----------------------------------------------------------------
  // TimelineControls::mouseMoveEvent
  //
  void
  TimelineControls::mouseMoveEvent(QMouseEvent * e)
  {
    if (!model_.hasTimelineDuration())
    {
      return;
    }

    QPoint pt = e->pos();

    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);

    if (currentState_ == kIdle || !activeMarker_)
    {
      double t = double(pt.x() - xOrigin) / double(unitLength);
      t = std::max(0.0, std::min(1.0, t));

      QString mousePosition = model_.getTimeStampAt(t);
      setToolTip(mousePosition);
      return;
    }

    int dx = pt.x() - dragStart_.x();

    double t =
      activeMarker_->positionAnchor_ +
      double(dx) / double(unitLength);

    t = std::max(0.0, std::min(1.0, t));

    activeMarker_->position_ = t;

    if (currentState_ == kDraggingTimeInMarker)
    {
      model_.setMarkerTimeIn(t);
    }

    if (currentState_ == kDraggingTimeOutMarker)
    {
      model_.setMarkerTimeOut(t);
    }

    if (currentState_ == kDraggingPlayheadMarker)
    {
      model_.setMarkerPlayhead(t);
    }

    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::mouseDoubleClickEvent
  //
  void
  TimelineControls::mouseDoubleClickEvent(QMouseEvent * e)
  {
    int xOrigin = 0;
    int yOriginInOut = 0;
    int yOriginPlayhead = 0;
    int unitLength = 0;
    getMarkerCSys(xOrigin, yOriginInOut, yOriginPlayhead, unitLength);

    QPoint pt = e->pos();
    double t = double(pt.x() - xOrigin) / double(unitLength);
    t = std::max(0.0, std::min(1.0, t));
    double seconds = model_.getTimeAsSecondsAt(t);
    model_.seekTo(seconds);
  }

  //----------------------------------------------------------------
  // TimelineControls::keyPressEvent
  //
  void
  TimelineControls::keyPressEvent(QKeyEvent * e)
  {
    int key = e->key();

    if (activeMarker_ &&
        currentState_ != kIdle &&
        key == Qt::Key_Escape)
    {
      activeMarker_->position_ = activeMarker_->positionAnchor_;
      currentState_ = kIdle;
      activeMarker_ = NULL;

      update();
    }
    else
    {
      e->ignore();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::getMarkerCSys
  //
  void
  TimelineControls::getMarkerCSys(int & xOrigin,
                                  int & yOriginInOut,
                                  int & yOriginPlayhead,
                                  int & unitLength) const
  {
    xOrigin = padding_;
    int mh = minimumHeight();
    yOriginInOut = (height() - mh) / 2 + (mh - padding_);
    yOriginPlayhead = yOriginInOut - lineWidth_;
    unitLength = width() - padding_ * 2;
  }

}
