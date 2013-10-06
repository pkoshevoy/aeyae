// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr 30 21:24:13 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_CONTROLS_H_
#define YAE_TIMELINE_CONTROLS_H_

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QWidget>
#include <QUrl>
#include <QImage>
#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QTime>

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
  class TimelineControls : public QWidget,
                           public IClockObserver
  {
    Q_OBJECT;

  public:
    TimelineControls(QWidget * parent = NULL, Qt::WindowFlags f = 0);
    ~TimelineControls();

    // optional widgets used to display (or edit) playhead
    // position and total duration:
    void setAuxWidgets(QLineEdit * playhead,
                       QLineEdit * duration,

                       // after user enters new playhead position
                       // focus will be given to this widget:
                       QWidget * focusWidget);

    // NOTE: this instance of TimelineControls will register itself
    // as an observer of the given shared clock; it will unregister
    // itself as the observer of previous shared clock.
    void observe(const SharedClock & sharedClock);

    void resetFor(IReader * reader);
    void adjustTo(IReader * reader);

    // accessors:
    double timelineStart() const;
    double timelineDuration() const;
    double timeIn() const;
    double timeOut() const;

    // helper:
    double currentTime() const;

    // virtual: thread safe, asynchronous, non-blocking:
    void noteCurrentTimeChanged(const TTime & currentTime);
    void noteTheClockHasStopped();

    // helper used to block the "clock has stopped" events
    // to avoid aborting playback prematurely:
    void ignoreClockStoppedEvent(bool ignore);

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
    void userIsSeeking(bool seeking);
    void clockStopped();

  public slots:
    void setInPoint();
    void setOutPoint();
    void seekFromCurrentTime(double offsetSeconds);
    void seekTo(double absoluteSeconds);
    void seekTo(const QString & HhMmSsFf);
    void seekToAuxPlayhead();
    void requestRepaint();

  protected slots:
    void repaintTimerExpired();

  protected:
    // virtual:
    bool event(QEvent * e);
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

    void updateAuxPlayhead(double position);
    void updateAuxDuration(double duration);

    //----------------------------------------------------------------
    // ClockStoppedEvent
    //
    struct ClockStoppedEvent : public QEvent
    {
      ClockStoppedEvent(): QEvent(QEvent::User) {}
    };

    //----------------------------------------------------------------
    // TimelineEvent
    //
    struct TimelineEvent : public QEvent
    {
      //----------------------------------------------------------------
      // TPayload
      //
      struct TPayload
      {
        TPayload(): dismissed_(true) {}

        bool set(const TTime & currentTime)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          bool postThePayload = dismissed_;
          currentTime_ = currentTime;
          dismissed_ = false;
          return postThePayload;
        }

        void get(TTime & currentTime)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          currentTime = currentTime_;
          dismissed_ = true;
        }

      private:
        mutable boost::mutex mutex_;
        TTime currentTime_;
        bool dismissed_;
      };

      TimelineEvent(TPayload & payload):
        QEvent(QEvent::User),
        payload_(payload)
      {}

      TPayload & payload_;
    };

    // event payload used for asynchronous timeline updates:
    TimelineEvent::TPayload payload_;

    // direct manipulation handles representing in/out time points
    // and current playback position marker (playhead):
    Marker markerTimeIn_;
    Marker markerTimeOut_;
    Marker markerPlayhead_;
    Marker * activeMarker_;
    QPoint dragStart_;

    // current state of playback controls:
    TState currentState_;

    // horizontal and vertical padding around the timeline:
    int padding_;

    // timeline line width in pixels:
    int lineWidth_;

    // a clock used to synchronize playback renderers,
    // used for playhead position:
    SharedClock sharedClock_;
    bool ignoreClockStopped_;

    // current reader:
    IReader * reader_;

    // a flag indicating whether source duration is known or not:
    bool unknownDuration_;

    // playback doesn't necessarily start at zero seconds:
    double timelineStart_;

    // playback duration in seconds:
    double timelineDuration_;

    // playback position delivered via most recent timeline event:
    double timelinePosition_;

    // these are used to format the timecode text fields:
    double frameRate_;
    const char * frameNumberSeparator_;

    // text widgets for displaying (or editing) playhead position
    // and displaying total duration:
    QLineEdit * auxPlayhead_;
    QLineEdit * auxDuration_;

    // after user enters new playhead position (via auxPlayead_)
    // focus will be given to this widget:
    QWidget * auxFocusWidget_;

    // repaint buffering:
    QTimer repaintTimer_;
    QTime repaintTimerStartTime_;
  };
}


#endif // YAE_TIMELINE_CONTROLS_H_
