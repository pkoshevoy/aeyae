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

namespace yae
{

 //----------------------------------------------------------------
  // TimeSegment::TimeSegment
  //
  TimeSegment::TimeSegment():
    waitForMe_(boost::get_system_time()),
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
  // SharedClock::setCurrentTime
  //
  bool
  SharedClock::setCurrentTime(const TTime & t0, double latency)
  {
    TTimeSegmentPtr keepAlive(shared_);

    if (!copied_)
    {
      boost::system_time now(boost::get_system_time());
      now += boost::posix_time::microseconds(long(latency * 1e+6));

      TimeSegment & timeSegment = *keepAlive;
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);

      timeSegment.origin_ = now;
      timeSegment.t0_ = t0;

      if (timeSegment.observer_)
      {
        timeSegment.observer_->noteCurrentTimeChanged(t0);
      }

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // SharedClock::getCurrentTime
  //
  bool
  SharedClock::getCurrentTime(TTime & t0, double & elapsedTime) const
  {
    TTimeSegmentPtr keepAlive(shared_);
    const TimeSegment & timeSegment = *keepAlive;
    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);

    t0 = timeSegment.t0_;

    if (timeSegment.stopped_ || timeSegment.origin_.is_not_a_date_time())
    {
      elapsedTime = 0.0;
      return false;
    }

    boost::system_time now(boost::get_system_time());
    boost::posix_time::time_duration delta = now - timeSegment.origin_;

    elapsedTime = double(delta.total_milliseconds()) * 1e-3;

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
    boost::posix_time::time_duration delta = now - timeSegment.waitForMe_;
    double timeSinceLastDelay = double(delta.total_milliseconds()) / 1000.0;

    if (timeSinceLastDelay > timeSegment.delayInSeconds_)
    {
      timeSegment.waitForMe_ = now;
      timeSegment.delayInSeconds_ = delayInSeconds;
      waitingFor_ = timeSegment.waitForMe_;

#if 1
      std::cerr << "waitFor: " << to_simple_string(timeSegment.waitForMe_)
                << ", " << delayInSeconds
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
      timeSegment_(timeSegment),
      stopped_(false)
    {
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
    }

  private:
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

    boost::system_time waitFor;
    double delayInSeconds = 0.0;
    {
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      waitFor = timeSegment.waitForMe_;
      delayInSeconds = timeSegment.delayInSeconds_;
    }

    if (waitingFor_ < waitFor)
    {
      waitingFor_ = waitFor;

      std::cerr << "waiting: " << to_simple_string(waitFor) << std::endl;

      TStopTime stopTime(timeSegment, allowsSettingTime());
      boost::this_thread::sleep(boost::posix_time::milliseconds
                                (long(0.5 + delayInSeconds * 1000.0)));
    }
#if 0
    else
    {
      std::cerr << "waitFor: " << to_simple_string(waitingFor_)
                << std::endl
                << "waiting: " << to_simple_string(waitFor)
                << std::endl;
    }
#endif
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
        timeSegment.observer_->noteTheClockHasStopped();
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
