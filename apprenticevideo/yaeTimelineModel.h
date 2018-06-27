// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Apr 30 21:24:13 MDT 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_MODEL_H_
#define YAE_TIMELINE_MODEL_H_

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// Qt includes:
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
#include <QQuickItem>
#else
#include <QObject>
#endif
#include <QEvent>
#include <QTimer>
#include <QTime>

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"


namespace yae
{

  //----------------------------------------------------------------
  // TimelineModel
  //
  class TimelineModel :
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    public QQuickItem,
#else
    public QObject,
#endif
    public IClockObserver
  {
    //----------------------------------------------------------------
    // TQtBase
    //
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    typedef QQuickItem TQtBase;
#else
    typedef QObject TQtBase;
#endif

    Q_OBJECT;

    Q_PROPERTY(QString auxPlayhead
               READ auxPlayhead
               WRITE setAuxPlayhead
               NOTIFY auxPlayheadChanged);

    Q_PROPERTY(QString auxDuration
               READ auxDuration
               NOTIFY auxDurationChanged);

    Q_PROPERTY(double markerTimeIn
               READ markerTimeIn
               WRITE setMarkerTimeIn
               NOTIFY markerTimeInChanged);

    Q_PROPERTY(double markerTimeOut
               READ markerTimeOut
               WRITE setMarkerTimeOut
               NOTIFY markerTimeOutChanged);

    Q_PROPERTY(double markerPlayhead
               READ markerPlayhead
               WRITE setMarkerPlayhead
               NOTIFY markerPlayheadChanged);

  public:
    TimelineModel();
    ~TimelineModel();

    // NOTE: this instance of TimelineModel will register itself
    // as an observer of the given shared clock; it will unregister
    // itself as the observer of previous shared clock.
    void observe(const SharedClock & sharedClock);

    // accessor to the current shared clock:
    inline const SharedClock & sharedClock() const
    { return sharedClock_; }

  protected:
    void reset(IReader * reader);

  public:
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
    void noteCurrentTimeChanged(const SharedClock & c,
                                const TTime & currentTime);
    void noteTheClockHasStopped(const SharedClock & c);

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

    inline const QString & auxPlayhead() const
    { return auxPlayhead_; }

    inline const QString & auxDuration() const
    { return auxDuration_; }

    inline double markerTimeIn() const
    { return markerTimeIn_; }

    inline double markerTimeOut() const
    { return markerTimeOut_; }

    inline double markerPlayhead() const
    { return markerPlayhead_; }

    void setAuxPlayhead(const QString & playhead);
    void setMarkerTimeIn(double marker);
    void setMarkerTimeOut(double marker);
    void setMarkerPlayhead(double marker);

  signals:
    void auxPlayheadChanged();
    void auxDurationChanged();
    void markerTimeInChanged();
    void markerTimeOutChanged();
    void markerPlayheadChanged();

    void moveTimeIn(double t);
    void moveTimeOut(double t);
    void movePlayHead(double t);
    void userIsSeeking(bool seeking);
    void clockStopped(const SharedClock & c);

  public slots:
    void setInPoint();
    void setOutPoint();
    void seekFromCurrentTime(double offsetSeconds);
    void seekTo(double absoluteSeconds);
    bool seekTo(const QString & HhMmSsFf);

  protected slots:
    void slideshowTimerExpired();

  protected:
    // virtual:
    bool event(QEvent * e);

    bool updateAuxPlayhead(const QString & position);
    bool updateAuxDuration(const QString & duration);
    bool updateMarkerTimeIn(double marker);
    bool updateMarkerTimeOut(double marker);
    bool updateMarkerPlayhead(double marker);

    //----------------------------------------------------------------
    // ClockStoppedEvent
    //
    struct ClockStoppedEvent : public QEvent
    {
      ClockStoppedEvent(const SharedClock & c):
        QEvent(QEvent::User),
        clock_(c)
      {}

      SharedClock clock_;
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
    double markerTimeIn_;
    double markerTimeOut_;
    double markerPlayhead_;

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

    // textual representation of playhead position and total duration:
    QString auxPlayhead_;
    QString auxDuration_;

    // delay signaling stopped-clock when source duration is unknown
    // or has just one frame (a single picture):
    std::list<SharedClock> stoppedClock_;
    QTimer slideshowTimer_;
    QTime slideshowTimerStart_;
  };

  //----------------------------------------------------------------
  // TIgnoreClockStop
  //
  struct YAE_API TIgnoreClockStop
  {
    TIgnoreClockStop(TimelineModel & timeline);
    ~TIgnoreClockStop();

  private:
    TIgnoreClockStop(const TIgnoreClockStop &);
    TIgnoreClockStop & operator = (const TIgnoreClockStop &);

    static int count_;
    TimelineModel & timeline_;
  };

}


#endif // YAE_TIMELINE_MODEL_H_
