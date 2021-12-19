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

// aeyae:
#include "yae/utils/yae_benchmark.h"

// yaeui:
#include "yaeTimelineModel.h"


namespace yae
{

  //----------------------------------------------------------------
  // kSeparatorForFrameNumber
  //
  static const char * kSeparatorForFrameNumber = ":";

  //----------------------------------------------------------------
  // kSeparatorForCentiSeconds
  //
  static const char * kSeparatorForCentiSeconds = ".";

  //----------------------------------------------------------------
  // kClockTemplate
  //
  static QString kClockTemplate = QString::fromUtf8("00:00:00:00");

  //----------------------------------------------------------------
  // kUnknownDuration
  //
  static QString kUnknownDuration = QString::fromUtf8("  :  :  :  ");

  //----------------------------------------------------------------
  // getTimeStamp
  //
  static QString
  makeTimeStamp(double seconds,
                double frameRate,
                const char * frameNumSep,
                bool trimLeadingZeros)
  {
    if (seconds == std::numeric_limits<double>::max())
    {
      return kUnknownDuration;
    }

    // round to nearest frame:
    double fpsWhole = ceil(frameRate);
    seconds = (seconds * fpsWhole + 0.5) / fpsWhole;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double frame = remainder * fpsWhole;
    uint64 frameNo = int(frame);

    int sec = int(seconds);
    int min = sec / 60;
    int hour = min / 60;

    sec %= 60;
    min %= 60;

#if 0
    yae_debug << "frame: " << frameNo
              << "\tseconds: " << seconds
              << ", remainder: " << remainder
              << ", fps " << frameRate
              << ", fps (whole): " << fpsWhole
              << ", " << frameRate * remainder
              << ", " << frame;
#endif

    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << hour << ':'
       << std::setw(2) << std::setfill('0') << min << ':'
       << std::setw(2) << std::setfill('0') << sec << frameNumSep
       << std::setw(2) << std::setfill('0') << frameNo;

    std::string str(os.str().c_str());
    const char * text = str.c_str();

    if (trimLeadingZeros)
    {
      // crop the leading zeros up to s:ff
      const char * tend = text + 7;

      while (text < tend)
      {
        if (*text != '0' && *text != ':')
        {
          break;
        }

        ++text;
      }
    }

    return QString::fromUtf8(text);
  }

  //----------------------------------------------------------------
  // TimelineModel::TimelineModel
  //
  TimelineModel::TimelineModel():
    ignoreClockStopped_(false),
    unknownDuration_(false),
    timelineStart_(0.0),
    timelineDuration_(0.0),
    timelinePosition_(0.0),
    trimLeadingZeros_(false),
    startFromZero_(true),
    localtimeOffset_(0),
    frameRate_(100.0),
    auxPlayhead_(kClockTemplate),
    auxDuration_(kUnknownDuration),
    slideshowTimer_(this)
  {
    frameNumberSeparator_ = kSeparatorForCentiSeconds;

    // setup marker positions:
    markerTimeIn_ = timelineStart_;
    markerTimeOut_ = timelineStart_ + timelineDuration_;
    markerPlayhead_ = timelineStart_;

    slideshowTimer_.setSingleShot(true);
    slideshowTimer_.setInterval(1000);

    bool ok = connect(&slideshowTimer_, SIGNAL(timeout()),
                      this, SLOT(slideshowTimerExpired()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TimelineModel::~TimelineModel
  //
  TimelineModel::~TimelineModel()
  {}

  //----------------------------------------------------------------
  // TimelineModel::timeIn
  //
  double
  TimelineModel::timeIn() const
  {
    double seconds = markerTimeIn_ * timelineDuration_ + timelineStart_;
    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineModel::timeOut
  //
  double
  TimelineModel::timeOut() const
  {
    double seconds = markerTimeOut_ * timelineDuration_ + timelineStart_;
    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineModel::currentTime
  //
  double
  TimelineModel::currentTime() const
  {
      double seconds = markerPlayhead_ * timelineDuration_ + timelineStart_;
      return seconds;
  }

  //----------------------------------------------------------------
  // TimelineModel::noteCurrentTimeChanged
  //
  void
  TimelineModel::noteCurrentTimeChanged(const SharedClock & c,
                                        const TTime & currentTime)
  {
    (void) c;

    bool postThePayload = payload_.set(currentTime);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new TimelineEvent(payload_));
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::noteTheClockHasStopped
  //
  void
  TimelineModel::noteTheClockHasStopped(const SharedClock & c)
  {
    if (!ignoreClockStopped_)
    {
      qApp->postEvent(this, new ClockStoppedEvent(c));
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::ignoreClockStoppedEvent
  //
  void
  TimelineModel::ignoreClockStoppedEvent(bool ignore)
  {
    ignoreClockStopped_ = ignore;
  }

  //----------------------------------------------------------------
  // TimelineModel::trimLeadingZeros
  //
  void
  TimelineModel::trimLeadingZeros(bool enable)
  {
    trimLeadingZeros_ = enable;
  }

  //----------------------------------------------------------------
  // TimelineModel::startFromZero
  //
  void
  TimelineModel::startFromZero(bool enable)
  {
    startFromZero_ = enable;
  }

  //----------------------------------------------------------------
  // TimelineModel::observe
  //
  void
  TimelineModel::observe(const SharedClock & sharedClock)
  {
    sharedClock_.setObserver(NULL);
    sharedClock_ = sharedClock;
    sharedClock_.setObserver(this);
  }

  //----------------------------------------------------------------
  // get_duration
  //
  bool
  get_duration(const IReader * reader, TTime & start, TTime & duration)
  {
    if (!reader)
    {
      start = TTime(0, 1);
      duration = TTime(0, 1);
      return true;
    }

    YAE_BENCHMARK(probe, "yae::get_duration");

    TTime audioStart;
    TTime audioDuration;
    bool audioOk = reader->getAudioDuration(audioStart, audioDuration);

    TTime videoStart;
    TTime videoDuration;
    bool videoOk = reader->getVideoDuration(videoStart, videoDuration);

    start = audioOk ? audioStart : videoStart;
    duration = audioOk ? audioDuration : videoDuration;

    if (audioOk && videoOk && videoStart < audioStart)
    {
      start = videoStart;
      duration = videoDuration;
    }

    return videoOk || audioOk;
  }

  //----------------------------------------------------------------
  // TimelineModel::reset
  //
  void
  TimelineModel::reset(IReader * reader)
  {
    TTime start;
    TTime duration;
    get_duration(reader, start, duration);

    frameRate_ = 100.0;
    frameNumberSeparator_ = kSeparatorForCentiSeconds;

    VideoTraits videoTraits;
    if (reader &&
        reader->getVideoTraits(videoTraits) &&
        videoTraits.frameRate_ < 100.0)
    {
      frameRate_ = videoTraits.frameRate_;
      frameNumberSeparator_ = kSeparatorForFrameNumber;
    }

    if (!startFromZero_)
    {
      // assume timestamps are actually unix time
      int64_t ts = start.get(1);
      struct tm tm;
      unix_epoch_time_to_localtime(ts, tm);
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      localtimeOffset_ = double(localtime_to_unix_epoch_time(tm));
    }

    timelineStart_ = start.sec();
    timelinePosition_ = timelineStart_;
    unknownDuration_ =
      duration.time_ == std::numeric_limits<int64>::max() ||
      !reader || !reader->isSeekable();

    timelineDuration_ = (unknownDuration_ ?
                         std::numeric_limits<double>::max() :
                         duration.sec());

    emit modelChanged();
  }

  //----------------------------------------------------------------
  // TimelineModel::resetFor
  //
  void
  TimelineModel::resetFor(IReader * reader)
  {
    reset(reader);

    double T1 = (unknownDuration_ ?
                 std::numeric_limits<double>::max() :
                 (startFromZero_ ? timelineDuration_ :
                  timelineStart_ + timelineDuration_ - localtimeOffset_));

    QString ts_p = getTimeStamp(startFromZero_ ? 0.0 :
                                timelineStart_ - localtimeOffset_);
    updateAuxPlayhead(ts_p);

    QString ts_d = getTimeStamp(T1);
    updateAuxDuration(ts_d);

    updateMarkerPlayhead(0.0);
    updateMarkerTimeIn(0.0);
    updateMarkerTimeOut(1.0);
  }

  //----------------------------------------------------------------
  // TimelineModel::adjustTo
  //
  void
  TimelineModel::adjustTo(IReader * reader)
  {
    // get current in/out/playhead positions in seconds:
    double t0 = timeIn();
    double t1 = timeOut();
    double t = currentTime();

    reset(reader);

    // shortcuts:
    double dT = timelineDuration_;
    double T0 = timelineStart_;
    double T1 = (unknownDuration_ ?
                 std::numeric_limits<double>::max() :
                 T0 + dT);

    t0 = std::max<double>(T0, std::min<double>(T1, t0));
    t1 = std::max<double>(T0, std::min<double>(T1, t1));
    t = std::max<double>(T0, std::min<double>(T1, t));

    QString ts_p = getTimeStamp(startFromZero_ ?
                                t - timelineStart_ :
                                t - localtimeOffset_);
    updateAuxPlayhead(ts_p);

    QString ts_d = getTimeStamp(startFromZero_ ?
                                timelineDuration_ :
                                T1 - localtimeOffset_);
    updateAuxDuration(ts_d);

    updateMarkerPlayhead(unknownDuration_ ? 0.0 : (t - T0) / dT);
    updateMarkerTimeIn(unknownDuration_ ? 0.0 : (t0 - T0) / dT);
    updateMarkerTimeOut(unknownDuration_ ? 1.0 : (t1 - T0) / dT);
  }

  //----------------------------------------------------------------
  // TimelineModel::updateDuration
  //
  void
  TimelineModel::updateDuration(IReader * reader)
  {
    YAE_BENCHMARK(probe, "TimelineModel::updateDuration");

    // get current in/out/playhead positions in seconds:
    double t0 = timeIn();
    double t1 = timeOut();
    double t = currentTime();

    TTime start;
    TTime duration;
    get_duration(reader, start, duration);

    if (!startFromZero_)
    {
      // assume timestamps are actually unix time
      int64_t ts = start.get(1);
      struct tm tm;
      unix_epoch_time_to_localtime(ts, tm);
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      localtimeOffset_ = double(localtime_to_unix_epoch_time(tm));
    }

    unknownDuration_ =
      duration.time_ == std::numeric_limits<int64>::max() ||
      !reader || !reader->isSeekable();

    timelineDuration_ = (unknownDuration_ ?
                         std::numeric_limits<double>::max() :
                         duration.sec());

    double dT = timelineDuration_;
    double T0 = timelineStart_;
    double T1 = (unknownDuration_ ?
                 std::numeric_limits<double>::max() :
                 T0 + dT);

    if (reader)
    {
      double tIn = TTime::min_flicks_as_sec();
      double tOut = TTime::max_flicks_as_sec();
      reader->getPlaybackInterval(tIn, tOut);

      if (tIn == TTime::min_flicks_as_sec())
      {
        t0 = T0;
      }

      if (tOut == TTime::max_flicks_as_sec())
      {
        t1 = T1;
      }
    }
    else
    {
      emit modelChanged();
    }

    t0 = std::max<double>(T0, std::min<double>(T1, t0));
    t1 = std::max<double>(T0, std::min<double>(T1, t1));
    t = std::max<double>(T0, std::min<double>(T1, t));

    QString ts_p = getTimeStamp(startFromZero_ ?
                                t - timelineStart_ :
                                t - localtimeOffset_);
    updateAuxPlayhead(ts_p);

    QString ts_d = getTimeStamp(startFromZero_ ?
                                timelineDuration_ :
                                T1 - localtimeOffset_);
    updateAuxDuration(ts_d);

    updateMarkerPlayhead(unknownDuration_ ? 0.0 : (t - T0) / dT);
    updateMarkerTimeIn(unknownDuration_ ? 0.0 : (t0 - T0) / dT);
    updateMarkerTimeOut(unknownDuration_ ? 1.0 : (t1 - T0) / dT);
  }

  //----------------------------------------------------------------
  // TimelineModel::clockTemplate
  //
  const QString &
  TimelineModel::clockTemplate()
  {
    return kClockTemplate;
  }

  //----------------------------------------------------------------
  // TimelineModel::getTimeAsSecondsAt
  //
  double
  TimelineModel::getTimeAsSecondsAt(double marker) const
  {
    double seconds = timelineStart_ + marker * timelineDuration_;
    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineModel::getTimeStamp
  //
  QString
  TimelineModel::getTimeStamp(double seconds) const
  {
    QString ts = makeTimeStamp(seconds,
                               frameRate_,
                               frameNumberSeparator_,
                               trimLeadingZeros_);
    return ts;
  }

  //----------------------------------------------------------------
  // TimelineModel::getTimeStampAt
  //
  QString
  TimelineModel::getTimeStampAt(double marker) const
  {
    double seconds = getTimeAsSecondsAt(marker);
    QString ts = getTimeStamp(startFromZero_ ?
                              seconds - timelineStart_ :
                              seconds - localtimeOffset_);
    return ts;
  }

  //----------------------------------------------------------------
  // TimelineModel::emitUserIsSeeking
  //
  void
  TimelineModel::emitUserIsSeeking(bool seeking)
  {
    emit userIsSeeking(seeking);
  }

  //----------------------------------------------------------------
  // TimelineModel::setAuxPlayhead
  //
  void
  TimelineModel::setAuxPlayhead(const QString & playhead)
  {
    seekTo(playhead);
  }

  //----------------------------------------------------------------
  // TimelineModel::setMarkerTimeIn
  //
  void
  TimelineModel::setMarkerTimeIn(double marker)
  {
    if (updateMarkerTimeIn(marker))
    {
      if (markerTimeOut_ < markerTimeIn_)
      {
        setMarkerTimeOut(markerTimeIn_);
      }

      double seconds = (timelineDuration_ * markerTimeIn_ + timelineStart_);
      emit moveTimeIn(seconds);
     }
  }

  //----------------------------------------------------------------
  // TimelineModel::setMarkerTimeOut
  //
  void
  TimelineModel::setMarkerTimeOut(double marker)
  {
    if (updateMarkerTimeOut(marker))
    {
      if (markerTimeIn_ > markerTimeOut_)
      {
        setMarkerTimeIn(markerTimeOut_);
      }

      double seconds = (timelineDuration_ * markerTimeOut_ + timelineStart_);
      emit moveTimeOut(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::setMarkerPlayhead
  //
  void
  TimelineModel::setMarkerPlayhead(double marker)
  {
    if (!timelineDuration_ || unknownDuration_)
    {
      return;
    }

    if (updateMarkerPlayhead(marker))
    {
      double seconds = markerPlayhead_ * timelineDuration_ + timelineStart_;
      emit movePlayHead(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::setInPoint
  //
  void
  TimelineModel::setInPoint()
  {
    if (!unknownDuration_)
    {
      setMarkerTimeIn(markerPlayhead_);
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::setOutPoint
  //
  void
  TimelineModel::setOutPoint()
  {
    if (!unknownDuration_)
    {
      setMarkerTimeOut(markerPlayhead_);
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::seekFromCurrentTime
  //
  void
  TimelineModel::seekFromCurrentTime(double secOffset)
  {
    double t0 = currentTime();
    if (t0 > 1e-1)
    {
      double seconds = std::max<double>(0.0, t0 + secOffset);
#if 0
      yae_debug << "seek from " << TTime(t0).to_hhmmss_ms()
                << " " << secOffset << " seconds to "
                << TTime(seconds).to_hhmmss_ms();
#endif
      seekTo(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::seekTo
  //
  void
  TimelineModel::seekTo(double seconds)
  {
    if (!timelineDuration_ || unknownDuration_)
    {
      return;
    }

    double marker = (seconds - timelineStart_) / timelineDuration_;
    setMarkerPlayhead(marker);
  }

  //----------------------------------------------------------------
  // parseTimeCode
  //
  // NOTE: returns the number of fields parsed
  //
  static unsigned int
  parseTimeCode(QString timecode,
                int & hh,
                int & mm,
                int & ss,
                int & ff)
  {
    // parse the timecode right to left 'ff' 1st, 'ss' 2nd, 'mm' 3rd, 'hh' 4th:
    unsigned int parsed = 0;

    // default separator:
    const char separator = ':';

    // convert white-space to default separator:
    timecode = timecode.simplified();
    timecode.replace(' ', separator);

    QStringList hh_mm_ss_ff = timecode.split(':', QString::SkipEmptyParts);
    int nc = hh_mm_ss_ff.size();

    if (!nc)
    {
      return 0;
    }

    // the right-most token is a frame number:
    QString last = hh_mm_ss_ff.back();
    hh_mm_ss_ff.pop_back();
    ff = last.toInt();
    parsed++;

    int hhmmss[3];
    hhmmss[0] = 0;
    hhmmss[1] = 0;
    hhmmss[2] = 0;

    for (int i = 0; i < 3 && !hh_mm_ss_ff.empty(); i++)
    {
      QString t = hh_mm_ss_ff.back();
      hh_mm_ss_ff.pop_back();

      hhmmss[2 - i] = t.toInt();
      parsed++;
    }

    hh = hhmmss[0];
    mm = hhmmss[1];
    ss = hhmmss[2];

    return parsed;
  }

  //----------------------------------------------------------------
  // TimelineModel::seekTo
  //
  bool
  TimelineModel::seekTo(const QString & hhmmssff)
  {
#if 0
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int ff = 0;

    unsigned int parsed = parseTimeCode(hhmmssff, hh, mm, ss, ff);
    if (!parsed)
    {
      return false;
    }

    double seconds = ss + 60.0 * (mm + 60.0 * hh);
    double msec = double(ff) / frameRate_;

    seekTo(seconds + msec);
#else
    TTime t;
    std::string tc = hhmmssff.toUtf8().constData();
    if (!parse_time(t, tc.c_str(), NULL, NULL, frameRate_))
    {
      return false;
    }

    double ts =
      startFromZero_ ?
      (timelineStart_ + t.sec()) :
      (localtimeOffset_ + t.sec());
    seekTo(ts);
#endif

    return true;
  }

  //----------------------------------------------------------------
  // TimelineModel::slideshowTimerExpired
  //
  void
  TimelineModel::slideshowTimerExpired()
  {
    while (!stoppedClock_.empty())
    {
      const SharedClock & c = stoppedClock_.front();
      if (sharedClock_.sharesCurrentTimeWith(c))
      {
#ifndef NDEBUG
        yae_debug << "NOTE: clock stopped";
#endif
        emit clockStopped(c);
      }
#ifndef NDEBUG
      else
      {
        yae_debug << "NOTE: ignoring stale delayed stopped clock";
      }
#endif

      stoppedClock_.pop_front();
    }
  }

  //----------------------------------------------------------------
  // TimelineModel::event
  //
  bool
  TimelineModel::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      TimelineEvent * timeChangedEvent = dynamic_cast<TimelineEvent *>(e);
      if (timeChangedEvent)
      {
        timeChangedEvent->accept();

        TTime currentTime;
        timeChangedEvent->payload_.get(currentTime);
        timelinePosition_ = currentTime.sec();

        double t = timelinePosition_ - timelineStart_;
        updateMarkerPlayhead(t / timelineDuration_);

        return true;
      }

      ClockStoppedEvent * clockStoppedEvent =
        dynamic_cast<ClockStoppedEvent *>(e);
      if (clockStoppedEvent)
      {
        const SharedClock & c = clockStoppedEvent->clock_;
        if (sharedClock_.sharesCurrentTimeWith(c))
        {
          stoppedClock_.push_back(c);

          if (unknownDuration_ || timelineDuration_ * frameRate_ < 2.0)
          {
            // this is probably a slideshow, delay it:
            if (!slideshowTimer_.isActive())
            {
              slideshowTimer_.start();
              slideshowTimerStart_.start();
            }
            else if (slideshowTimerStart_.elapsed() >
                     slideshowTimer_.interval() * 2)
            {
#ifndef NDEBUG
              yae_debug << "STOPPED CLOCK TIMEOUT WAS LATE";
#endif
              slideshowTimerExpired();
            }
          }
          else
          {
            slideshowTimerExpired();
          }

          return true;
        }

#ifndef NDEBUG
        yae_debug << "NOTE: ignoring stale ClockStoppedEvent";
#endif
        return true;
      }
    }

    return TQtBase::event(e);
  }

  //----------------------------------------------------------------
  // TimelineModel::updateAuxPlayhead
  //
  bool
  TimelineModel::updateAuxPlayhead(const QString & playhead)
  {
    if (playhead == auxPlayhead_)
    {
      return false;
    }

    auxPlayhead_ = playhead;
    emit auxPlayheadChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineModel::updateAuxDuration
  //
  bool
  TimelineModel::updateAuxDuration(const QString & duration)
  {
    if (duration == auxDuration_)
    {
      return false;
    }

    auxDuration_ = duration;
    emit auxDurationChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineModel::updateMarkerTimeIn
  //
  bool
  TimelineModel::updateMarkerTimeIn(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerTimeIn_)
    {
      return false;
    }

    markerTimeIn_ = t;
    emit markerTimeInChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineModel::updateMarkerTimeOut
  //
  bool
  TimelineModel::updateMarkerTimeOut(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerTimeOut_)
    {
      return false;
    }

    markerTimeOut_ = t;
    emit markerTimeOutChanged();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineModel::updateMarkerPlayhead
  //
  bool
  TimelineModel::updateMarkerPlayhead(double marker)
  {
    double t = std::min(1.0, std::max(0.0, marker));
    if (t == markerPlayhead_)
    {
      return false;
    }

    markerPlayhead_ = t;
    emit markerPlayheadChanged();

    double seconds =
      (markerPlayhead_ * timelineDuration_ +
       (startFromZero_ ? 0.0 : timelineStart_ - localtimeOffset_));

    QString ts = getTimeStamp(seconds);
    updateAuxPlayhead(ts);

    return true;
  }


  //----------------------------------------------------------------
  // TIgnoreClockStop::TIgnoreClockStop
  //
  TIgnoreClockStop::TIgnoreClockStop(TimelineModel & timeline):
    timeline_(timeline)
  {
    count_++;
    if (count_ < 2)
    {
      timeline_.ignoreClockStoppedEvent(true);
    }
  }

  //----------------------------------------------------------------
  // TIgnoreClockStop::~TIgnoreClockStop
  //
  TIgnoreClockStop::~TIgnoreClockStop()
  {
    count_--;
    if (!count_)
    {
      timeline_.ignoreClockStoppedEvent(false);
    }
  }

  //----------------------------------------------------------------
  // TIgnoreClockStop::count_
  //
  int
  TIgnoreClockStop::count_ = 0;

}
