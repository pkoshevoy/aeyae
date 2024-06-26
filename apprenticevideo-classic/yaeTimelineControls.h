// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr 30 21:24:13 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_CONTROLS_H_
#define YAE_TIMELINE_CONTROLS_H_

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// Qt includes:
#include <QWidget>
#include <QUrl>
#include <QImage>
#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QTime>

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_time.h"
#include "yae/video/yae_reader_factory.h"
#include "yae/video/yae_synchronous.h"

// yaeui:
#include "yaeTimelineModel.h"


namespace yae
{

  //----------------------------------------------------------------
  // Marker
  //
  struct Marker
  {
    Marker();

    // check whether the marker image overlaps given coordinates
    // where marker image pixel alpha channel is greater than zero:
    bool overlaps(const QPoint & coords,

                  // these parameters are used to derive current
                  // marker position:
                  const int & xOrigin,
                  const int & yOrigin,
                  const int & unitLength) const;

    // set anchor position to current marker position:
    void setAnchor();

    // image of the marker:
    QImage image_;

    // marker hot-spot position within the image:
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
    TimelineControls(QWidget * parent = NULL,
                     Qt::WindowFlags f = Qt::WindowFlags(0));
    ~TimelineControls();

    TimelineModel model_;

    // optional widgets used to display (or edit) playhead
    // position and total duration:
    void setAuxWidgets(QLineEdit * playhead,
                       QLineEdit * duration,

                       // after user enters new playhead position
                       // focus will be given to this widget:
                       QWidget * focusWidget);

  public slots:
    void seekToAuxPlayhead();
    void requestRepaint();

  protected slots:
    void repaintTimerExpired();
    void modelChanged();
    void modelTimeInChanged();
    void modelTimeOutChanged();
    void modelPlayheadChanged();
    void modelDurationChanged();

  protected:
    // virtual:
    void paintEvent(QPaintEvent * e);
    void mousePressEvent(QMouseEvent * e);
    void mouseReleaseEvent(QMouseEvent * e);
    void mouseMoveEvent(QMouseEvent * e);
    void mouseDoubleClickEvent(QMouseEvent * e);
    void keyPressEvent(QKeyEvent * e);

    // accessors to coordinate system origin and x-axis unit length
    // on which the direct manipulation handles are drawn,
    // expressed in widget coordinate space:
    void getMarkerCSys(int & xOrigin,
                       int & yOriginInOut,
                       int & yOriginPlayhead,
                       int & unitLength) const;

    // direct manipulation handles representing in/out time points
    // and current playback position marker (playhead):
    Marker markerTimeIn_;
    Marker markerTimeOut_;
    Marker markerPlayhead_;
    Marker * activeMarker_;
    QPoint dragStart_;

    enum TState
    {
      kIdle,
      kDraggingTimeInMarker,
      kDraggingTimeOutMarker,
      kDraggingPlayheadMarker
    };

    // current state of playback controls:
    TState currentState_;

    // horizontal and vertical padding around the timeline:
    int padding_;

    // timeline line width in pixels:
    int lineWidth_;

    // text widgets for displaying (or editing) playhead position
    // and displaying total duration:
    QLineEdit * auxPlayhead_;
    QLineEdit * auxDuration_;

    // after user enters new playhead position (via auxPlayead_)
    // focus will be given to this widget:
    QWidget * auxFocusWidget_;

    // repaint buffering:
    QTimer repaintTimer_;
    yae::TTime repaintTimerStartTime_;
  };
}


#endif // YAE_TIMELINE_CONTROLS_H_
