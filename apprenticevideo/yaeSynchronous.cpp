// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 14:16:37 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include <yaeSynchronous.h>

// boost includes:
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>


namespace yae
{

  //----------------------------------------------------------------
  // TimeSegment
  // 
  struct TimeSegment
  {
    TimeSegment() {}
    
    // this keeps track of "when" the time segment was specified:
    boost::system_time origin_;
    
    TTime t0_;
    TTime dt_;
    
    mutable boost::mutex mutex_;
  };

  //----------------------------------------------------------------
  // TTimeSegmentPtr
  // 
  typedef boost::shared_ptr<TimeSegment> TTimeSegmentPtr;

  //----------------------------------------------------------------
  // SharedClock::TPrivate
  // 
  class SharedClock::TPrivate
  {
  public:
    
    TPrivate():
      shared_(new TimeSegment()),
      copied_(false)
    {}
    
    TPrivate(const TPrivate & p):
      shared_(p.shared_),
      copied_(true)
    {}

    TPrivate & operator = (const TPrivate & p)
    {
      if (this != &p)
      {
        shared_ = p.shared_;
        copied_ = true;
      }

      return *this;
    }
    
    TTimeSegmentPtr shared_;
    bool copied_;
  };


  //----------------------------------------------------------------
  // SharedClock::SharedClock
  // 
  SharedClock::SharedClock():
    private_(new TPrivate())
  {}

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
  SharedClock::SharedClock(const SharedClock & c):
    private_(new TPrivate(*c.private_))
  {}
  
  //----------------------------------------------------------------
  // SharedClock::operator =
  // 
  SharedClock &
  SharedClock::operator = (const SharedClock & c)
  {
    if (private_ != c.private_)
    {
      delete private_;
      private_ = new TPrivate(*c.private_);
    }

    return *this;
  }
  
  //----------------------------------------------------------------
  // SharedClock::sharesCurrentTimeWith
  // 
  bool
  SharedClock::sharesCurrentTimeWith(const SharedClock & c) const
  {
    return private_->shared_ == c.private_->shared_;
  }
  
  //----------------------------------------------------------------
  // SharedClock::setMasterClock
  // 
  bool
  SharedClock::setMasterClock(const SharedClock & master)
  {
    if (sharesCurrentTimeWith(master))
    {
      private_->copied_ = (this != &master);
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
    return !private_->copied_;
  }
  
  //----------------------------------------------------------------
  // SharedClock::setCurrentTime
  // 
  bool
  SharedClock::setCurrentTime(const TTime & t0,
                              const TTime & dt,
                              double latency)
  {
    if (!private_->copied_)
    {
      boost::system_time now(boost::get_system_time());
      now += boost::posix_time::microseconds(long(latency * 1e+6));
      
      TimeSegment & timeSegment = *(private_->shared_);
      boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
      
      timeSegment.origin_ = now;
      timeSegment.t0_ = t0;
      timeSegment.dt_ = dt;
      
      return true;
    }

    return false;
  }
  
  //----------------------------------------------------------------
  // SharedClock::getCurrentTime
  // 
  double
  SharedClock::getCurrentTime(TTime & t0, TTime & dt) const
  {
    const TimeSegment & timeSegment = *(private_->shared_);
    boost::lock_guard<boost::mutex> lock(timeSegment.mutex_);
    
    t0 = timeSegment.t0_;
    dt = timeSegment.dt_;
    double msecSegmentDuration = double(dt.time_) / double(dt.base_) * 1e+3;
    
    boost::system_time now(boost::get_system_time());
    boost::posix_time::time_duration delta = now - timeSegment.origin_;
    double msecCurrentPosition = double(delta.total_milliseconds());
    
    double relativePosition =
      dt.time_ ? (msecCurrentPosition / msecSegmentDuration) : 0.0;
    
    return relativePosition;
  }
  

  //----------------------------------------------------------------
  // ISynchronous::~ISynchronous
  // 
  ISynchronous::~ISynchronous()
  {}

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
