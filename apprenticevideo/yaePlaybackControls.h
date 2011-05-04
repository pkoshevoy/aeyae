// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr 30 21:24:13 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYBACK_CONTROLS_H_
#define YAE_PLAYBACK_CONTROLS_H_

// Qt includes:
#include <QWidget>
#include <QUrl>
#include <QTimer>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeSynchronous.h>


namespace yae
{

  //----------------------------------------------------------------
  // Marker
  // 
  struct Marker
  {
    // calculate current bounding box of this marker:
    QRect bbox() const;
    
    // set anchor position to current marker position:
    void setAnchor();
    
    // pixmap of the marker:
    QPixmap pixmap_;
    
    // marker hot-spot position within the pixmap:
    int hotspot_[2];
    
    // current marker position on the timeline:
    double position_;
    
    // marker position at the start of a mouse drag that may have moved it:
    double positionAnchor_;
  };
  
  //----------------------------------------------------------------
  // TimelineControls
  // 
  class TimelineControls : public QWidget
  {
    Q_OBJECT;

  public:
    TimelineControls(QWidget * parent = NULL, Qt::WindowFlags f = 0);
    ~TimelineControls();
    
    void reset(const SharedClock & sharedClock, IReader * reader);
    
    enum TState
    {
      kIdle,
      kDraggingTimeInMarker,
      kDraggingTimeOutMarker,
      kDraggingPlayheadMarker
    };
    
  signals:
    void moveTimeIn(double t);
    void moveTimeOut(double t);
    void movePlayHead(double t);
    
  public slots:
    void refreshTimeline();
    
  protected:
    // virtual:
    void paintEvent(QPaintEvent * e);
    void wheelEvent(QWheelEvent * e);
    void mousePressEvent(QMouseEvent * e);
    void mouseReleaseEvent(QMouseEvent * e);
    void mouseMoveEvent(QMouseEvent * e);
    void keyPressEvent(QKeyEvent * e);
    
    // pixmaps used to draw in/out markers and the playhead
    Marker markerTimeIn_;
    Marker markerTimeOut_;
    Marker markerPlayhead_;
    
    // current state of playback controls:
    TState currentState_;

    // helpers:
    int pad_;
    int xExt_;
    int yExt_;
    int xPos_;
    int yPos_;
    
    // a clock used to synchronize playback renderers,
    // used for playhead position:
    SharedClock sharedClock_;
    
    // current reader:
    IReader * reader_;
    
    // playback doesn't necessarily start at zero seconds:
    double timelineStart_;
    
    // playback duration in seconds:
    double timelineDuration_;
    
    // a timer used for timeline marker position refresh:
    QTimer timerRefreshTimeline_;
  };

  //----------------------------------------------------------------
  // PlaybackControls
  // 
  class PlaybackControls : public QWidget
  {
    Q_OBJECT;
    
  public:
    PlaybackControls(QWidget * parent = NULL, Qt::WindowFlags f = 0);
    ~PlaybackControls();
    
  signals:
    void load(const QUrl & url);
    void togglePlayback();
    
  protected:
    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mouseDoubleClickEvent(QMouseEvent * e);
    
    // current playlist:
    QList<QUrl> playlist_;
  };
}


#endif // YAE_PLAYBACK_CONTROLS_H_
