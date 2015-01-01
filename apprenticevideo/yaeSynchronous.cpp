// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 14:16:37 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// yae includes:
#include <yaeSynchronous.h>

// boost includes:
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

// namespace access:
using boost::posix_time::to_simple_string;

//----------------------------------------------------------------
// YAE_DEBUG_SHARED_CLOCK
//
#define YAE_DEBUG_SHARED_CLOCK 0


namespace yae
{

  //----------------------------------------------------------------
  // TimeSegment::TimeSegment
  //
  TimeSegment::TimeSegment():
    realtime_(false),
    waitForMe_(boost::system_time()),
    delayInSeconds_(0.0),
    stopped_(false),
    observer_(NULL)
  {}

  //----------------------------------------------------------------
  // SharedClock::SharedClock
  //
  SharedClock::SharedClock():
    shared_(new TimeSegment()),
    copied_(false)
  {
    waitingFor_ = boost::get_system_time();
  }

  //----------------------------------------------------------------
  // SharedClock::~SharedClock
  //
  SharedClock::~SharedClock()
  {}

  //----------------------------------------------------------------
  // SharedClock::SharedClock
  //
  SharedClock::SharedClock(const SharedClock & c):
    shared_(c.shared_),
    copied_(true)
  {
    waitingFor_ = boost::get_system_time();
  }

  //----------------------------------------------------------------
  // SharedClock::operator =
  //
  SharedClock &
  SharedClock::operator = (const SharedClock & c)
  {
    if (this != &c)
    {
      shared_ = c.shared_;
      copied_ = true;
      waitingFor_ = boost::get_system_time();
    }

    return *this;
  }

  //----------------------------------------------------------------
  // SharedClock::sharesCurrentTimeWith
  //
  bool
  SharedClock::sharesCurrentTimeWith(const SharedClock & c) const
  {
    return shared_ == c.shared_;
  }

  //----------------------------------------------------------------
  // SharedClock::setMasterClock
  //
  bool
  SharedClock::setMasterClock(const SharedClock & master)
  {
    if (sharesCurrentTimeWith(master))
    {
      copied_ = (this != &master);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::allowsSettingTime
  //
  bool
  SharedClock::allowsSettingTime() const
  {
    return !copied_;
  }

  //----------------------------------------------------------------
  // SharedClock::setRealtime
  //
  bool
  SharedClock::setRealtime(bool enabled)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    if (!copied_)
    {
#if YAE_DEBUG_SHARED_CLOCK
      std::cerr
        << "\nNOTE: master clock realtime enabled: " << enabled
        << std::endl
        << std::endl;
#endif
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      timeSegment.realtime_ = enabled;
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
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;

    if (!copied_)
    {
#if YAE_DEBUG_SHARED_CLOCK
      std::cerr
        << "\nNOTE: master clock reset\n"
        << std::endl;
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
  // SharedClock::setCurrentTime
  //
  bool
  SharedClock::setCurrentTime(const TTime & t,
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
        double dt = (ct - t).toSeconds();
        if (hasTime && dt > 0.5)
        {
#if YAE_DEBUG_SHARED_CLOCK
          std::cerr
            << "\nNOTE: badly muxed file?\n"
            << "rollback denied: " << t.to_hhmmss_usec(":") << std::endl
            << "current time at: " << ct.to_hhmmss_usec(":") << std::endl
            << "dt: " << dt << std::endl
            << std::endl;
#endif
          return false;
        }
      }

      timeSegment.origin_ = now;
      timeSegment.t0_ = t;

      if (timeSegment.observer_ && notifyObserver)
      {
        timeSegment.observer_->noteCurrentTimeChanged(*this, t);
      }

      return true;
    }
    else if (notifyObserver &&
             timeSegment.observer_ &&
             !masterClockIsAccurate)
    {
      timeSegment.observer_->noteCurrentTimeChanged(*this, t);
    }

    return false;
  }

  //----------------------------------------------------------------
  // kRealtimeTolerance
  //
  static const double kRealtimeTolerance = 2.0;

  //----------------------------------------------------------------
  // SharedClock::getCurrentTime
  //
  bool
  SharedClock::getCurrentTime(TTime & t0,
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
  // SharedClock::waitForMe
  //
  void
  SharedClock::waitForMe(double delayInSeconds)
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
      std::cerr
        << "waitFor: " << to_simple_string(timeSegment.waitForMe_)
        << ", " << TTime(delayInSeconds).to_hhmmss_usec(":")
        << std::endl;
#endif
    }
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
      std::cerr << "STOP TIME, begin" << std::endl;
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

      std::cerr
        << "STOP TIME, end: " << TTime(elapsedTime).to_hhmmss_usec(":")
        << std::endl;
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
  // SharedClock::waitForOthers
  //
  void
  SharedClock::waitForOthers()
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
    std::cerr
      << "\nWAITING FOR: " << to_simple_string(waitingFor_)
      << ", " << TTime(1e-3 * msecToWait).to_hhmmss_usec(":")
      << std::endl
      << std::endl;
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
        std::cerr
          << "\nWAIT FINISHED: " << to_simple_string(waitingFor_)
          << ", " << TTime(1e-3 * msecToWait).to_hhmmss_usec(":")
          << std::endl
          << std::endl;
#endif
        return;
      }

      long msecSleep = std::min<long>(msecToWait - msecElapsed, 20);
      boost::this_thread::sleep(boost::posix_time::milliseconds(msecSleep));

      // check whether the request has changed or was cancelled:
      {
        boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
        if (timeSegment.waitForMe_ != waitingFor_)
        {
          // nothing to do:
#if YAE_DEBUG_SHARED_CLOCK
          std::cerr
            << "\nWAIT CANCELLED: " << to_simple_string(waitingFor_)
            << ", " << TTime(1e-3 * msecToWait).to_hhmmss_usec(":")
            << std::endl
            << std::endl;
#endif
          return;
        }
      }
    }
  }

  //----------------------------------------------------------------
  // SharedClock::cancelWaitForOthers
  //
  void
  SharedClock::cancelWaitForOthers()
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;
    {
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      if (!timeSegment.waitForMe_.is_not_a_date_time())
      {
#if YAE_DEBUG_SHARED_CLOCK
        std::cerr
          << "\nCANCEL WAIT REQUEST: "
          << to_simple_string(timeSegment.waitForMe_)
          << ", " << TTime(timeSegment.delayInSeconds_).to_hhmmss_usec(":")
          << std::endl
          << std::endl;
#endif
        timeSegment.waitForMe_ = boost::system_time();
        timeSegment.delayInSeconds_ = 0.0;
      }
    }
  }

  //----------------------------------------------------------------
  // SharedClock::setObserver
  //
  void
  SharedClock::setObserver(IClockObserver * observer)
  {
    TTimeSegmentPtr keepAlive(shared_);
    TimeSegment & timeSegment = *keepAlive;
    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
    timeSegment.observer_ = observer;
  }

  //----------------------------------------------------------------
  // SharedClock::noteTheClockHasStopped
  //
  bool
  SharedClock::noteTheClockHasStopped()
  {
    if (!copied_)
    {
      TTimeSegmentPtr keepAlive(shared_);
      TimeSegment & timeSegment = *keepAlive;
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);

      if (timeSegment.observer_)
      {
        timeSegment.observer_->noteTheClockHasStopped(*this);
      }

      return true;
    }

    return false;
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
