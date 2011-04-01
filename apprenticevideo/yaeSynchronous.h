// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 13:59:04 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SYNCHRONOUS_H_
#define YAE_SYNCHRONOUS_H_

// yae includes:
#include <yaeAPI.h>


namespace yae
{

  //----------------------------------------------------------------
  // SharedClock
  // 
  // implicitly shared thread-safe time piece,
  // useful for various synchronization tasks
  // 
  struct YAE_API SharedClock
  {
    SharedClock();
    ~SharedClock();

    //! NOTE: copies created using the copy constructor or
    //! the assignment operator are not "master", and
    //! will not allow setCurrentTime to succeed:
    SharedClock(const SharedClock & c);
    SharedClock & operator = (const SharedClock & c);
    
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
    
    //! set current time (only if this is the master clock):
    bool setCurrentTime(const TTime & t0, double latencyInSeconds = 0.0);
    
    //! retrieve the reference time interval and current playhead position;
    //! returns false when clock is not set or is stopped while the clock
    //! owner is waiting for someone to catch up;
    //! returns true when clock is running:
    bool getCurrentTime(TTime & t0, double & playheadPosition) const;
    
    //! annouce that you are late so others would stop and wait for you:
    void waitForMe(double waitInSeconds = 1.0);
    void waitForOthers();
    
  private:
    class TPrivate;
    TPrivate * private_;
  };
  
  //----------------------------------------------------------------
  // ISynchronous
  // 
  struct YAE_API ISynchronous
  {
    virtual ~ISynchronous();
    
    //! take responsibility for maintaining the shared reference clock:
    void takeThisClock(const SharedClock & yourNewClock);
    
    //! synchronize against a given clock (which may be maintained elsewhere):
    void obeyThisClock(const SharedClock & someRefClock);
    
    //! accessor to this objects shared clock:
    inline const SharedClock & clock() const
    { return clock_; }
    
  protected:
    mutable SharedClock clock_;
  };
}


#endif // YAE_SYNCHRONOUS_H_
