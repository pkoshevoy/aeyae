// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 14:16:37 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#endif

// aeyae:
#include "yae_synchronous.h"

// namespace access:
using boost::posix_time::to_simple_string;

//----------------------------------------------------------------
// YAE_DEBUG_SHARED_CLOCK
//
#define YAE_DEBUG_SHARED_CLOCK 0


namespace yae
{

  //----------------------------------------------------------------
  // TimeSegment
  //
  struct TimeSegment
  {
    TimeSegment():
      realtime_(false),
      waitForMe_(boost::system_time()),
      delayInSeconds_(0.0),
      stopped_(false),
      observer_(NULL)
    {}

    // this indicates whether the clock is in the real-time mode,
    // requires monotonically increasing current time
    // and disallows rolling back time:
    bool realtime_;

    // this keeps track of "when" the time segment was specified:
    boost::system_time origin_;

    // current time:
    TTime t0_;

    // this keeps track of "when" someone announced they will be late:
    boost::system_time waitForMe_;

    // how long to wait:
    double delayInSeconds_;

    // this indicates whether the clock is stopped while waiting for someone:
    bool stopped_;

    // shared clock observer interface, may be NULL:
    IClockObserver * observer_;

    // avoid concurrent access from multiple threads:
    mutable boost::mutex mutex_;
  };

  //----------------------------------------------------------------
  // TTimeSegmentPtr
  //
  typedef yae::shared_ptr<TimeSegment> TTimeSegmentPtr;


  //----------------------------------------------------------------
  // SharedClock::Private
  //
  struct SharedClock::Private
  {
    Private(const SharedClock & clock):
      clock_(clock),
      shared_(new TimeSegment()),
      copied_(false)
    {
      waitingFor_ = boost::get_system_time();
    }

    //! Check whether a given clock and this clock
    //! refer to the same time segment:
    bool sharesCurrentTimeWith(const SharedClock & c) const;

    //! NOTE: setMasterClock will fail if the given clock
    //! and this clock do not refer to the same current time.
    //!
    //! Specify which is the master reference clock:
    bool setMasterClock(const SharedClock & master);

    //! NOTE: setting current time is permitted
    //! only when this clock is the master clock:
    bool allowsSettingTime() const;

    // realtime requires monotonically increasing
    // current time and disallows rolling back time:
    bool setRealtime(bool realtime);

    // check whether the master clock has been updated recently:
    bool isMasterClockBehindRealtime() const;

    //! reset to initial state, do not notify the observer:
    bool resetCurrentTime();

    //! set current time (only if this is the master clock):
    bool setCurrentTime(const TTime & t0,
                        double latencyInSeconds = 0.0,
                        bool notifyObserver = true);

    //! retrieve the reference time interval and time since last clock update;
    //! returns false when clock is not set or is stopped while the clock
    //! owner is waiting for someone to catch up;
    //! returns true when clock is running:
    bool getCurrentTime(TTime & t0,
                        double & elapsedTime,
                        bool & withinRealtimeTolerance) const;

    //! announce that you are late so others would stop and wait for you:
    void waitForMe(double waitInSeconds = 1.0);
    void waitForOthers();

    //! the reader may call this after seeking
    //! to terminate waitForOthers early, to avoid
    //! stuttering playback when seeking backwards:
    void cancelWaitForOthers();

    void setObserver(IClockObserver * observer);

    //! notify the observer (if it exists) that there will be no
    //! further updates to the current time on this clock,
    //! most likely because playback has reached the end:
    bool noteTheClockHasStopped();

    const SharedClock & clock_;
    TTimeSegmentPtr shared_;
    boost::system_time waitingFor_;
    bool copied_;
  };


  //----------------------------------------------------------------
  // SharedClock::SharedClock
  //
  SharedClock::SharedClock()
  {
    private_ = new SharedClock::Private(*this);
  }

  //----------------------------------------------------------------
  // SharedClock::~SharedClock
  //
  SharedClock::~SharedClock()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // SharedClock::SharedClock
  //
  SharedClock::SharedClock(const SharedClock & c)
  {
    private_ = new SharedClock::Private(*this);
    private_->shared_ = c.private_->shared_;
    private_->copied_ = true;
    private_->waitingFor_ = boost::get_system_time();
  }

  //----------------------------------------------------------------
  // SharedClock::operator =
  //
  SharedClock &
  SharedClock::operator = (const SharedClock & c)
  {
    if (this != &c)
    {
      private_->shared_ = c.private_->shared_;
      private_->copied_ = true;
      private_->waitingFor_ = boost::get_system_time();
    }

    return *this;
  }

  //----------------------------------------------------------------
  // SharedClock::sharesCurrentTimeWith
  //
  bool
  SharedClock::Private::sharesCurrentTimeWith(const SharedClock & c) const
  {
    return shared_ == c.private_->shared_;
  }

  //----------------------------------------------------------------
  // SharedClock::sharesCurrentTimeWith
  //
  bool
  SharedClock::sharesCurrentTimeWith(const SharedClock & c) const
  {
    return private_->sharesCurrentTimeWith(c);
  }

  //----------------------------------------------------------------
  // SharedClock::Private::setMasterClock
  //
  bool
  SharedClock::Private::setMasterClock(const SharedClock & master)
  {
    if (sharesCurrentTimeWith(master))
    {
      copied_ = (&clock_ != &master);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::setMasterClock
  //
  bool
  SharedClock::setMasterClock(const SharedClock & master)
  {
    return private_->setMasterClock(master);
  }

  //----------------------------------------------------------------
  // SharedClock::Private::allowsSettingTime
  //
  bool
  SharedClock::Private::allowsSettingTime() const
  {
    return copied_;
  }

  //----------------------------------------------------------------
  // SharedClock::allowsSettingTime
  //
  bool
  SharedClock::allowsSettingTime() const
  {
    return private_->allowsSettingTime();
  }

  //----------------------------------------------------------------
  // SharedClock::Private::setRealtime
  //
  bool
  SharedClock::Private::setRealtime(bool enabled)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    if (!copied_)
    {
#if YAE_DEBUG_SHARED_CLOCK
      yae_debug
        << "\nNOTE: master clock realtime enabled: " << enabled
        << "\n\n";
#endif
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      timeSegment.realtime_ = enabled;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::setRealtime
  //
  bool
  SharedClock::setRealtime(bool enabled)
  {
    return private_->setRealtime(enabled);
  }

  //----------------------------------------------------------------
  // SharedClock::Private::resetCurrentTime
  //
  bool
  SharedClock::Private::resetCurrentTime()
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    if (!copied_)
    {
#if YAE_DEBUG_SHARED_CLOCK
      yae_debug << "\nNOTE: master clock reset\n\n\n";
#endif
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      timeSegment.origin_ = boost::get_system_time();
      timeSegment.t0_ = TTime();
      timeSegment.waitForMe_ = boost::system_time();
      timeSegment.delayInSeconds_ = 0.0;
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::resetCurrentTime
  //
  bool
  SharedClock::resetCurrentTime()
  {
    return private_->resetCurrentTime();
  }

  //----------------------------------------------------------------
  // SharedClock::Private::setCurrentTime
  //
  bool
  SharedClock::Private::setCurrentTime(const TTime & t,
                                       double latency,
                                       bool notifyObserver)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    TTime t0;
    double elapsedTime = 0.0;
    bool masterClockIsAccurate = false;
    bool hasTime = getCurrentTime(t0, elapsedTime, masterClockIsAccurate);

    if (!copied_)
    {
      boost::system_time now(boost::get_system_time());
      now += boost::posix_time::microseconds(long(latency * 1e+6));

      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (timeSegment.realtime_)
      {
        TTime ct = t0 + elapsedTime;
        double dt = (ct - t).sec();
        if (hasTime && dt > 0.5)
        {
#if YAE_DEBUG_SHARED_CLOCK
          yae_debug
            << "\nNOTE: badly muxed file?"
            << "\nrollback denied: " << t
            << "\ncurrent time at: " << ct
            << "\ndt: " << dt << "\n\n";
#endif
          return false;
        }
      }

      timeSegment.origin_ = now;
      timeSegment.t0_ = t;

      if (timeSegment.observer_ && notifyObserver)
      {
        timeSegment.observer_->noteCurrentTimeChanged(clock_, t);
      }

      return true;
    }

    if (notifyObserver && !masterClockIsAccurate)
    {
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (timeSegment.observer_)
      {
#ifdef YAE_DEBUG_SHARED_CLOCK
        yae_debug << "MASTER CLOCK IS NOT ACCURATE\n";
#endif
        timeSegment.observer_->noteCurrentTimeChanged(clock_, t);
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::setCurrentTime
  //
  bool
  SharedClock::setCurrentTime(const TTime & t,
                              double latency,
                              bool notifyObserver)
  {
    return private_->setCurrentTime(t, latency, notifyObserver);
  }

  //----------------------------------------------------------------
  // kRealtimeTolerance
  //
  static const double kRealtimeTolerance = 0.2;

  //----------------------------------------------------------------
  // SharedClock::Private::getCurrentTime
  //
  bool
  SharedClock::Private::getCurrentTime(TTime & t0,
                                       double & elapsedTime,
                                       bool & masterClockIsAccurate) const
  {
    TTimeSegmentPtr keepAlive(shared_);
    const TimeSegment & timeSegment = *keepAlive;
    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);

    t0 = timeSegment.t0_;

    if (timeSegment.stopped_ || timeSegment.origin_.is_not_a_date_time())
    {
      elapsedTime = 0.0;
      masterClockIsAccurate = true;
      return false;
    }

    boost::system_time now(boost::get_system_time());
    boost::posix_time::time_duration delta = now - timeSegment.origin_;

    elapsedTime = double(delta.total_milliseconds()) * 1e-3;
    masterClockIsAccurate =
      timeSegment.realtime_ &&
      elapsedTime <= kRealtimeTolerance;

    return true;
  }

  //----------------------------------------------------------------
  // SharedClock::getCurrentTime
  //
  bool
  SharedClock::getCurrentTime(TTime & t0,
                              double & elapsedTime,
                              bool & masterClockIsAccurate) const
  {
    return private_->getCurrentTime(t0, elapsedTime, masterClockIsAccurate);
  }

  //----------------------------------------------------------------
  // SharedClock::Private::waitForMe
  //
  void
  SharedClock::Private::waitForMe(double delayInSeconds)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;
    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);

    boost::system_time now(boost::get_system_time());
    double timeSinceLastDelay = std::numeric_limits<double>::max();

    if (!timeSegment.waitForMe_.is_not_a_date_time())
    {
      boost::posix_time::time_duration delta = now - timeSegment.waitForMe_;
      timeSinceLastDelay = 1e-3 * double(delta.total_milliseconds());
    }

    if (timeSinceLastDelay > timeSegment.delayInSeconds_)
    {
      timeSegment.waitForMe_ = now;
      timeSegment.delayInSeconds_ = delayInSeconds;
      waitingFor_ = timeSegment.waitForMe_;

#if YAE_DEBUG_SHARED_CLOCK
      yae_debug
        << "waitFor: " << to_simple_string(timeSegment.waitForMe_)
        << ", " << TTime(delayInSeconds)
        << "\n";
#endif
    }
  }

  //----------------------------------------------------------------
  // SharedClock::waitForMe
  //
  void
  SharedClock::waitForMe(double delayInSeconds)
  {
    private_->waitForMe(delayInSeconds);
  }

  //----------------------------------------------------------------
  // TStopTime
  //
  struct TStopTime
  {
    TStopTime(TimeSegment & timeSegment, bool stop):
#if YAE_DEBUG_SHARED_CLOCK
      startTime_(boost::get_system_time()),
#endif
      timeSegment_(timeSegment),
      stopped_(false)
    {
#if YAE_DEBUG_SHARED_CLOCK
      yae_debug << "STOP TIME, begin\n";
#endif

      if (stop)
      {
        boost::lock_guard<boost::mutex> lock(timeSegment_.mutex_);
        if (!timeSegment_.stopped_)
        {
          timeSegment_.stopped_ = true;
          timeSegment_.origin_ = boost::get_system_time();
          stopped_ = true;
        }
      }
    }

    ~TStopTime()
    {
      if (stopped_)
      {
        boost::lock_guard<boost::mutex> lock(timeSegment_.mutex_);
        timeSegment_.stopped_ = false;
        timeSegment_.origin_ = boost::get_system_time();
      }

#if YAE_DEBUG_SHARED_CLOCK
      boost::system_time now(boost::get_system_time());
      boost::posix_time::time_duration delta = now - startTime_;
      double elapsedTime = double(delta.total_milliseconds()) * 1e-3;

      yae_debug
        << "STOP TIME, end: " << TTime(elapsedTime)
        << "\n";
#endif
    }

  private:
#if YAE_DEBUG_SHARED_CLOCK
    boost::system_time startTime_;
#endif

    TimeSegment & timeSegment_;
    bool stopped_;
  };

  //----------------------------------------------------------------
  // SharedClock::Private::waitForOthers
  //
  void
  SharedClock::Private::waitForOthers()
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    boost::posix_time::ptime
      startTime(boost::posix_time::microsec_clock::universal_time());
    long msecToWait = 0;

    // check whether someone requested a slow-down:
    {
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (timeSegment.waitForMe_.is_not_a_date_time() ||
          timeSegment.waitForMe_ <= waitingFor_ ||
          timeSegment.delayInSeconds_ <= 0.0)
      {
        // nothing to do:
        return;
      }

      waitingFor_ = timeSegment.waitForMe_;
      msecToWait = long(0.5 + 1e+3 * timeSegment.delayInSeconds_);
    }

#if YAE_DEBUG_SHARED_CLOCK
    yae_debug
      << "\nWAITING FOR: " << to_simple_string(waitingFor_)
      << ", " << TTime(1e-3 * msecToWait)
      << "\n\n";
#endif

    TStopTime stopTime(timeSegment, allowsSettingTime());
    while (true)
    {
      boost::posix_time::ptime
        now(boost::posix_time::microsec_clock::universal_time());
      long msecElapsed = (now - startTime).total_milliseconds();

      if (msecElapsed >= msecToWait)
      {
#if YAE_DEBUG_SHARED_CLOCK
        yae_debug
          << "\nWAIT FINISHED: " << to_simple_string(waitingFor_)
          << ", " << TTime(1e-3 * msecToWait)
          << "\n\n";
#endif
        return;
      }

      long msecSleep = std::min<long>(msecToWait - msecElapsed, 20);
      boost::this_thread::sleep_for(boost::chrono::milliseconds(msecSleep));

      // check whether the request has changed or was cancelled:
      {
        boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
        if (timeSegment.waitForMe_ != waitingFor_)
        {
          // nothing to do:
#if YAE_DEBUG_SHARED_CLOCK
          yae_debug
            << "\nWAIT CANCELLED: " << to_simple_string(waitingFor_)
            << ", " << TTime(1e-3 * msecToWait)
            << "\n\n";
#endif
          return;
        }
      }
    }
  }

  //----------------------------------------------------------------
  // SharedClock::waitForOthers
  //
  void
  SharedClock::waitForOthers()
  {
    private_->waitForOthers();
  }

  //----------------------------------------------------------------
  // SharedClock::Private::cancelWaitForOthers
  //
  void
  SharedClock::Private::cancelWaitForOthers()
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;
    {
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (!timeSegment.waitForMe_.is_not_a_date_time())
      {
#if YAE_DEBUG_SHARED_CLOCK
        yae_debug
          << "\nCANCEL WAIT REQUEST: "
          << to_simple_string(timeSegment.waitForMe_)
          << ", " << TTime(timeSegment.delayInSeconds_)
          << "\n\n";
#endif
        timeSegment.waitForMe_ = boost::system_time();
        timeSegment.delayInSeconds_ = 0.0;
      }
    }
  }

  //----------------------------------------------------------------
  // SharedClock::cancelWaitForOthers
  //
  void
  SharedClock::cancelWaitForOthers()
  {
    private_->cancelWaitForOthers();
  }

  //----------------------------------------------------------------
  // SharedClock::Private::setObserver
  //
  void
  SharedClock::Private::setObserver(IClockObserver * observer)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
    timeSegment.observer_ = observer;
  }

  //----------------------------------------------------------------
  // SharedClock::setObserver
  //
  void
  SharedClock::setObserver(IClockObserver * observer)
  {
    private_->setObserver(observer);
  }

  //----------------------------------------------------------------
  // SharedClock::Private::noteTheClockHasStopped
  //
  bool
  SharedClock::Private::noteTheClockHasStopped()
  {
    if (!copied_)
    {
      TTimeSegmentPtr keepAlive(shared_);
      TimeSegment & timeSegment = *keepAlive;

      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (timeSegment.observer_)
      {
        timeSegment.observer_->noteTheClockHasStopped(clock_);
      }

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::noteTheClockHasStopped
  //
  bool
  SharedClock::noteTheClockHasStopped()
  {
    return private_->noteTheClockHasStopped();
  }


  //----------------------------------------------------------------
  // ISynchronous::takeThisClock
  //
  void
  ISynchronous::takeThisClock(const SharedClock & yourNewClock)
  {
    clock_ = yourNewClock;
    clock_.setMasterClock(clock_);
  }

  //----------------------------------------------------------------
  // ISynchronous::obeyThisClock
  //
  void
  ISynchronous::obeyThisClock(const SharedClock & someRefClock)
  {
    clock_ = someRefClock;
    clock_.setMasterClock(someRefClock);
  }
}
