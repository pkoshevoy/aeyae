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

// yae includes:
#include <yaeTimelineControls.h>


namespace yae
{

  //----------------------------------------------------------------
  // kClockTemplate
  //
  static QString kClockTemplate = QString::fromUtf8("00:00:00:00");

  //----------------------------------------------------------------
  // getTimeStamp
  //
  static QString
  getTimeStamp(double seconds, double frameRate)
  {
    int sec = int(seconds);
    int min = sec / 60;
    int hour = min / 60;

    sec %= 60;
    min %= 60;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double fpsWhole = ceil(frameRate);
    uint64 frameNo = int(remainder * fpsWhole);

#if 0
    std::cerr << "frame: " << frameNo
              << ", remained: " << remainder
              << ", fps: " << fpsWhole
              << ", " << frameRate * remainder
              << std::endl;
#endif

    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << hour << ':'
       << std::setw(2) << std::setfill('0') << min << ':'
       << std::setw(2) << std::setfill('0') << sec << ':'
       << std::setw(2) << std::setfill('0') << frameNo;

    // crop the leading zeros up to s:ff
    std::string str(os.str().c_str());
    const char * text = str.c_str();
    const char * tend = text + 7;

    while (text < tend)
    {
      if (*text != '0' && *text != ':')
      {
        break;
      }

      ++text;
    }

    return QString::fromUtf8(text);
  }

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
    ignoreClockStopped_(false),
    activeMarker_(NULL),
    reader_(NULL),
    unknownDuration_(false),
    timelineStart_(0.0),
    timelineDuration_(0.0),
    timelinePosition_(0.0),
    frameRate_(23.976),
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
    markerTimeIn_.position_ = timelineStart_;
    markerTimeOut_.position_ = timelineStart_ + timelineDuration_;
    markerPlayhead_.position_ = timelineStart_;

    // current state of playback controls:
    currentState_ = TimelineControls::kIdle;

    // this is to avoid too much repainting:
    repaintTimer_.setSingleShot(true);
    repaintTimer_.setInterval(100);

    bool ok = connect(&repaintTimer_, SIGNAL(timeout()),
                      this, SLOT(repaintTimerExpired()));
    YAE_ASSERT(ok);
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
    clockFont.setBold(true);
    clockFont.setPixelSize(11);
    clockFont.setStyle(QFont::StyleNormal);
#if QT_VERSION < 0x040700
    clockFont.setStyleHint(QFont::Courier);
#else
    clockFont.setStyleHint(QFont::Monospace);
#endif
    clockFont.setStyleStrategy(QFont::PreferDefault);
    clockFont.setWeight(QFont::Normal);

    QFontMetrics fm(clockFont);
    QSize bbox = fm.size(Qt::TextSingleLine, kClockTemplate);
    bbox += QSize(4, 8);

    bool ok = true;

    if (auxPlayhead_)
    {
      auxPlayhead_->setFont(clockFont);
      auxPlayhead_->setMinimumSize(bbox);
      auxPlayhead_->setMaximumSize(bbox);
      auxPlayhead_->setEnabled(timelineDuration_);

      ok = connect(auxPlayhead_, SIGNAL(returnPressed()),
                   this, SLOT(seekToAuxPlayhead()));
      YAE_ASSERT(ok);
    }

    if (auxDuration_)
    {
      auxDuration_->setFont(clockFont);
      auxDuration_->setMinimumSize(bbox);
      auxDuration_->setMaximumSize(bbox);
      auxDuration_->setEnabled(timelineDuration_);
    }

    updateAuxPlayhead(timelineStart_);
    updateAuxDuration(timelineStart_ + timelineDuration_);
  }

  //----------------------------------------------------------------
  // TimelineControls::timelineStart
  //
  double
  TimelineControls::timelineStart() const
  {
    return timelineStart_;
  }

  //----------------------------------------------------------------
  // TimelineControls::timelineDuration
  //
  double
  TimelineControls::timelineDuration() const
  {
    return timelineDuration_;
  }

  //----------------------------------------------------------------
  // TimelineControls::timeIn
  //
  double
  TimelineControls::timeIn() const
  {
    double seconds =
      markerTimeIn_.position_ * timelineDuration_ + timelineStart_;

    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::timeOut
  //
  double
  TimelineControls::timeOut() const
  {
    double seconds =
      markerTimeOut_.position_ * timelineDuration_ + timelineStart_;

    return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::currentTime
  //
  double
  TimelineControls::currentTime() const
  {
      double seconds =
        markerPlayhead_.position_ * timelineDuration_ + timelineStart_;

      return seconds;
  }

  //----------------------------------------------------------------
  // TimelineControls::noteCurrentTimeChanged
  //
  void
  TimelineControls::noteCurrentTimeChanged(const TTime & currentTime)
  {
    bool postThePayload = payload_.set(currentTime);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new TimelineEvent(payload_));
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::noteTheClockHasStopped
  //
  void
  TimelineControls::noteTheClockHasStopped()
  {
    if (!ignoreClockStopped_)
    {
      qApp->postEvent(this, new ClockStoppedEvent());
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::ignoreClockStoppedEvent
  //
  void
  TimelineControls::ignoreClockStoppedEvent(bool ignore)
  {
    ignoreClockStopped_ = ignore;
  }

  //----------------------------------------------------------------
  // TimelineControls::observe
  //
  void
  TimelineControls::observe(const SharedClock & sharedClock)
  {
    sharedClock_.setObserver(NULL);
    sharedClock_ = sharedClock;
    sharedClock_.setObserver(this);
  }

  //----------------------------------------------------------------
  // TimelineControls::resetFor
  //
  void
  TimelineControls::resetFor(IReader * reader)
  {
    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }

    VideoTraits videoTraits;
    if (reader->getVideoTraits(videoTraits))
    {
      frameRate_ =
        videoTraits.frameRate_ < 60.0 ? videoTraits.frameRate_ : 23.976;
    }

    timelineStart_ = start.toSeconds();
    timelinePosition_ = timelineStart_;

    unknownDuration_ = (duration.time_ == std::numeric_limits<int64>::max());
    timelineDuration_ = (unknownDuration_ ?
                         std::numeric_limits<double>::max() :
                         duration.toSeconds());

    if (auxPlayhead_)
    {
      auxPlayhead_->setEnabled(timelineDuration_);
    }

    if (auxDuration_)
    {
      auxDuration_->setEnabled(timelineDuration_);
    }

    updateAuxPlayhead(timelineStart_);
    updateAuxDuration(timelineStart_ + timelineDuration_);

    markerPlayhead_.position_ = 0.0;
    markerPlayhead_.setAnchor();

    markerTimeIn_.position_ = 0.0;
    markerTimeIn_.setAnchor();

    markerTimeOut_.position_ = 1.0;
    markerTimeOut_.setAnchor();

    setToolTip(QString());
    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::adjustTo
  //
  void
  TimelineControls::adjustTo(IReader * reader)
  {
    // get current in/out/playhead positions in seconds:
    double t0 = timeIn();
    double t1 = timeOut();
    double t = currentTime();

    TTime start;
    TTime duration;
    if (!reader->getAudioDuration(start, duration))
    {
      reader->getVideoDuration(start, duration);
    }

    VideoTraits videoTraits;
    if (reader->getVideoTraits(videoTraits))
    {
      frameRate_ =
        videoTraits.frameRate_ < 60.0 ? videoTraits.frameRate_ : 23.976;
    }

    timelineStart_ = start.toSeconds();
    timelineDuration_ = duration.toSeconds();
    timelinePosition_ = timelineStart_;

    // shortcuts:
    double dT = timelineDuration_;
    double T0 = timelineStart_;
    double T1 = T0 + dT;

    t0 = std::max<double>(T0, std::min<double>(T1, t0));
    t1 = std::max<double>(T0, std::min<double>(T1, t1));
    t = std::max<double>(T0, std::min<double>(T1, t));

    if (auxPlayhead_)
    {
      auxPlayhead_->setEnabled(timelineDuration_);
    }

    if (auxDuration_)
    {
      auxDuration_->setEnabled(timelineDuration_);
    }

    updateAuxPlayhead(t);
    updateAuxDuration(T1);

    markerPlayhead_.position_ = (t - T0) / dT;
    markerPlayhead_.setAnchor();

    markerTimeIn_.position_ = (t0 - T0) / dT;
    markerTimeIn_.setAnchor();

    markerTimeOut_.position_ = (t1 - T0) / dT;
    markerTimeOut_.setAnchor();

    setToolTip(QString());
    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::setInPoint
  //
  void
  TimelineControls::setInPoint()
  {
    if (currentState_ == kIdle)
    {
      markerTimeIn_.position_ = markerPlayhead_.position_;
      double seconds = (timelineDuration_ * markerTimeIn_.position_ +
                        timelineStart_);

      if (markerTimeOut_.position_ < markerTimeIn_.position_)
      {
        markerTimeOut_.position_ = markerTimeIn_.position_;
        emit moveTimeOut(seconds);
      }

      emit moveTimeIn(seconds);
      update();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::setOutPoint
  //
  void
  TimelineControls::setOutPoint()
  {
    if (currentState_ == kIdle)
    {
      markerTimeOut_.position_ = markerPlayhead_.position_;
      double seconds = (timelineDuration_ * markerTimeOut_.position_ +
                        timelineStart_);

      if (markerTimeIn_.position_ > markerTimeOut_.position_)
      {
        markerTimeIn_.position_ = markerTimeOut_.position_;
        emit moveTimeIn(seconds);
      }

      emit moveTimeOut(seconds);
      update();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::seekFromCurrentTime
  //
  void
  TimelineControls::seekFromCurrentTime(double secOffset)
  {
    double t0 = currentTime();
    if (t0 > 1e-1)
    {
      double seconds = std::max<double>(0.0, t0 + secOffset);
#if 0
      std::cerr << "seek from " << TTime(t0).to_hhmmss_usec(":")
                << " " << secOffset << " seconds to "
                << TTime(seconds).to_hhmmss_usec(":")
                << std::endl;
#endif
      seekTo(seconds);
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::seekTo
  //
  void
  TimelineControls::seekTo(double seconds)
  {
    if (!timelineDuration_ || unknownDuration_)
    {
      return;
    }

    double t = (seconds - timelineStart_) / timelineDuration_;
    t = std::min(1.0, std::max(0.0, t));

    markerPlayhead_.position_ = t;
    seconds = t * timelineDuration_ + timelineStart_;
    updateAuxPlayhead(seconds);

    emit movePlayHead(seconds);

    update();
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

    QStringList	hh_mm_ss_ff = timecode.split(':', QString::SkipEmptyParts);
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
  // TimelineControls::seekTo
  //
  void
  TimelineControls::seekTo(const QString & hhmmssff)
  {
    int hh = 0;
    int mm = 0;
    int ss = 0;
    int ff = 0;

    unsigned int parsed = parseTimeCode(hhmmssff, hh, mm, ss, ff);
    if (!parsed)
    {
      return;
    }

    double seconds = ss + 60.0 * (mm + 60.0 * hh);
    double msec = double(ff) / frameRate_;

    seekTo(seconds + msec);
  }

  //----------------------------------------------------------------
  // TimelineControls::seekToAuxPlayhead
  //
  void
  TimelineControls::seekToAuxPlayhead()
  {
    QString hhmmssff = auxPlayhead_->text();
    seekTo(hhmmssff);

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
#if 0
      std::cerr << "REPAINT TIMEOUT WAS LATE" << std::endl;
#endif
      repaintTimer_.stop();

      updateAuxPlayhead(timelinePosition_);
      repaint();
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::repaintTimerExpired
  //
  void
  TimelineControls::repaintTimerExpired()
  {
    updateAuxPlayhead(timelinePosition_);
    update();
  }

  //----------------------------------------------------------------
  // TimelineControls::event
  //
  bool
  TimelineControls::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      TimelineEvent * timeChangedEvent = dynamic_cast<TimelineEvent *>(e);
      if (timeChangedEvent)
      {
        timeChangedEvent->accept();

        TTime currentTime;
        timeChangedEvent->payload_.get(currentTime);
        timelinePosition_ = currentTime.toSeconds();

        double t = timelinePosition_ - timelineStart_;
        markerPlayhead_.position_ = t / timelineDuration_;

        requestRepaint();
        return true;
      }

      ClockStoppedEvent * clockStoppedEvent =
        dynamic_cast<ClockStoppedEvent *>(e);
      if (clockStoppedEvent)
      {
        emit clockStopped();
        return true;
      }
    }

    return QWidget::event(e);
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

    if (!timelineDuration_ || unknownDuration_)
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
    if (!timelineDuration_ || unknownDuration_)
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
      emit userIsSeeking(true);
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
      emit userIsSeeking(false);
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
    if (!timelineDuration_ || unknownDuration_)
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

      double seconds = t * timelineDuration_ + timelineStart_;
      QString mousePosition = getTimeStamp(seconds, frameRate_);
      setToolTip(mousePosition);
      return;
    }

    int dx = pt.x() - dragStart_.x();

    double t =
      activeMarker_->positionAnchor_ +
      double(dx) / double(unitLength);

    t = std::max(0.0, std::min(1.0, t));

    activeMarker_->position_ = t;
    double seconds = t * timelineDuration_ + timelineStart_;

    if (currentState_ == kDraggingTimeInMarker)
    {
      double t1 = std::max(activeMarker_->position_,
                           markerTimeOut_.positionAnchor_);

      if (t1 != markerTimeOut_.position_)
      {
        markerTimeOut_.position_ = t1;
        double seconds = t1 * timelineDuration_ + timelineStart_;
        emit moveTimeOut(seconds);
      }

      emit moveTimeIn(seconds);
    }

    if (currentState_ == kDraggingTimeOutMarker)
    {
      double t0 = std::min(activeMarker_->position_,
                           markerTimeIn_.positionAnchor_);

      if (t0 != markerTimeIn_.position_)
      {
        markerTimeIn_.position_ = t0;
        double seconds = t0 * timelineDuration_ + timelineStart_;
        emit moveTimeIn(seconds);
      }

      emit moveTimeOut(seconds);
    }

    if (currentState_ == kDraggingPlayheadMarker)
    {
      updateAuxPlayhead(seconds);
      emit movePlayHead(seconds);
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
    double seconds = t * timelineDuration_ + timelineStart_;
    seekTo(seconds);
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

  //----------------------------------------------------------------
  // TimelineControls::updateAuxPlayhead
  //
  void
  TimelineControls::updateAuxPlayhead(double position)
  {
    if (auxPlayhead_)
    {
      if (!auxPlayhead_->isEnabled())
      {
        auxPlayhead_->setText(kClockTemplate);
      }
      else if (!auxPlayhead_->hasFocus())
      {
        QString ts = getTimeStamp(position, frameRate_);
        auxPlayhead_->setText(ts);
      }
    }
  }

  //----------------------------------------------------------------
  // TimelineControls::updateAuxDuration
  //
  void
  TimelineControls::updateAuxDuration(double duration)
  {
    if (auxDuration_)
    {
      if (!auxDuration_->isEnabled())
      {
        auxDuration_->setText(kClockTemplate);
      }
      else if (unknownDuration_)
      {
        auxDuration_->setText(tr("N/A"));
      }
      else
      {
        QString ts = getTimeStamp(duration, frameRate_);
        auxDuration_->setText(ts);
      }
    }
  }

}
