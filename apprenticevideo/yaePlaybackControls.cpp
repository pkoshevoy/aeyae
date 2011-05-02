// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun May  1 13:23:52 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt includes:
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QPainter>
#include <QColor>
#include <QBrush>
#include <QPen>

// yae includes:
#include <yaePlaybackControls.h>


namespace yae
{
  
  //----------------------------------------------------------------
  // Marker::bbox
  // 
  QRect
  Marker::bbox() const
  {
    QRect bbox = pixmap_.rect();
    return bbox;
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
    reader_(NULL),
    timelineStart_(0.0),
    timelineDuration_(1.0),
    timerRefreshTimeline_(this)
  {
    pad_ = 8;
    yExt_ = 3;
    xPos_ = pad_;
    
    setFixedHeight(pad_ * 2 + yExt_);
    setMinimumWidth(300);
    
    // pixmaps used to draw in/out markers and the playhead
    markerTimeIn_.pixmap_ = QPixmap(":/images/timeIn.png");
    markerTimeOut_.pixmap_ = QPixmap(":/images/timeOut.png");
    markerPlayhead_.pixmap_ = QPixmap(":/images/playHead.png");
    
    // setup hotspots:
    markerTimeIn_.hotspot_[0] = markerTimeIn_.pixmap_.width() - 1;
    markerTimeIn_.hotspot_[1] = 12;
    
    markerTimeOut_.hotspot_[0] = 0;
    markerTimeOut_.hotspot_[1] = 12;
    
    markerPlayhead_.hotspot_[0] = markerPlayhead_.pixmap_.width() / 2;
    markerPlayhead_.hotspot_[1] = 8;
    
    // current state of playback controls:
    currentState_ = TimelineControls::kIdle;
    
    timerRefreshTimeline_.setSingleShot(false);
    timerRefreshTimeline_.setInterval(100);
    
    bool ok = true;
    ok = connect(&timerRefreshTimeline_, SIGNAL(timeout()),
                 this, SLOT(refreshTimeline()));
    YAE_ASSERT(ok);
  }
  
  //----------------------------------------------------------------
  // TimelineControls::~TimelineControls
  // 
  TimelineControls::~TimelineControls()
  {}

  //----------------------------------------------------------------
  // TimelineControls::reset
  // 
  void
  TimelineControls::reset(const SharedClock & sharedClock, IReader * reader)
  {
    sharedClock_ = sharedClock;
    
    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }
    
    timelineStart_ = start.toSeconds();
    timelineDuration_ = duration.toSeconds();
    
    markerTimeIn_.position_ = 0.1;
    markerTimeIn_.setAnchor();
    
    markerTimeOut_.position_ = 0.8;
    markerTimeOut_.setAnchor();
    
    timerRefreshTimeline_.start();
  }
  
  //----------------------------------------------------------------
  // TimelineControls::refreshTimeline
  // 
  void
  TimelineControls::refreshTimeline()
  {
    TTime lastUpdate; 	 
    double playheadPosition = 0.0; 	 
    if (sharedClock_.getCurrentTime(lastUpdate, playheadPosition)) 	 
    {
      double t = lastUpdate.toSeconds();
      t -= timelineStart_;
      markerPlayhead_.position_ = t / timelineDuration_;
      markerPlayhead_.setAnchor();
    }
    
    update();
  }
  
  //----------------------------------------------------------------
  // TimelineControls::paintEvent
  // 
  void
  TimelineControls::paintEvent(QPaintEvent * e)
  {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    xExt_ = width() - pad_ * 2;
    yPos_ = height() - yExt_ - pad_;
    
    int inExt = xExt_ * markerTimeIn_.position_;
    int outExt = xExt_ * markerTimeOut_.position_;
    int playExt = xExt_ * markerPlayhead_.position_;
    
    p.setPen(Qt::NoPen);
    
    p.setBrush(QBrush(QColor(0x80, 0x80, 0x80)));
    p.drawRect(QRect(xPos_, yPos_, inExt, yExt_));
    
    p.setBrush(QBrush(QColor(0x40, 0x80, 0xff)));
    p.drawRect(QRect(xPos_ + inExt, yPos_, outExt - inExt, yExt_));
    
    p.setBrush(QBrush(QColor(0x80, 0x80, 0x80)));
    p.drawRect(QRect(xPos_ + outExt, yPos_, xExt_ - outExt, yExt_));
    
    p.drawPixmap(xPos_ + inExt - markerTimeIn_.hotspot_[0],
                 yPos_ + yExt_ - markerTimeIn_.hotspot_[1],
                 markerTimeIn_.pixmap_);
    
    p.drawPixmap(xPos_ + outExt - markerTimeOut_.hotspot_[0],
                 yPos_ + yExt_ - markerTimeOut_.hotspot_[1],
                 markerTimeOut_.pixmap_);
    
    p.drawPixmap(xPos_ + playExt - markerPlayhead_.hotspot_[0],
                 yPos_ - markerPlayhead_.hotspot_[1],
                 markerPlayhead_.pixmap_);
  }

  //----------------------------------------------------------------
  // TimelineControls::wheelEvent
  // 
  void
  TimelineControls::wheelEvent(QWheelEvent * e)
  {}
  
  //----------------------------------------------------------------
  // TimelineControls::mousePressEvent
  // 
  void
  TimelineControls::mousePressEvent(QMouseEvent * e)
  {}
  
  //----------------------------------------------------------------
  // TimelineControls::mouseReleaseEvent
  // 
  void
  TimelineControls::mouseReleaseEvent(QMouseEvent * e)
  {}

  //----------------------------------------------------------------
  // TimelineControls::mouseMoveEvent
  // 
  void
  TimelineControls::mouseMoveEvent(QMouseEvent * e)
  {}

  //----------------------------------------------------------------
  // TimelineControls::keyPressEvent
  // 
  void
  TimelineControls::keyPressEvent(QKeyEvent * e)
  {}
  
  
  //----------------------------------------------------------------
  // PlaybackControls::PlaybackControls
  // 
  PlaybackControls::PlaybackControls(QWidget * parent, Qt::WindowFlags f):
    QWidget(parent, f),
    timeline_(new TimelineControls())
  {
    QVBoxLayout * layout = new QVBoxLayout(this);
    layout->addWidget(timeline_);
  }
  
  //----------------------------------------------------------------
  // PlaybackControls::~PlaybackControls
  // 
  PlaybackControls::~PlaybackControls()
  {}

  //----------------------------------------------------------------
  // PlaybackControls::initializeTimeline
  // 
  void
  PlaybackControls::reset(const SharedClock & sharedClock, IReader * reader)
  {
    timeline_->reset(sharedClock, reader);
  }
    
  //----------------------------------------------------------------
  // PlaybackControls::closeEvent
  // 
  void
  PlaybackControls::closeEvent(QCloseEvent * e)
  {}

  //----------------------------------------------------------------
  // PlaybackControls::keyPressEvent
  // 
  void
  PlaybackControls::keyPressEvent(QKeyEvent * e)
  {}
  
  //----------------------------------------------------------------
  // PlaybackControls::mouseDoubleClickEvent
  // 
  void
  PlaybackControls::mouseDoubleClickEvent(QMouseEvent * e)
  {}
  
}
