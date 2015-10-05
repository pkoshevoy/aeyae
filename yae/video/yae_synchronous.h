// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb 12 13:59:04 MST 2011
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SYNCHRONOUS_H_
#define YAE_SYNCHRONOUS_H_

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#endif

// aeyae:
#include "yae_video.h"


namespace yae
{
  struct YAE_API SharedClock;

  //----------------------------------------------------------------
  // IClockObserver
  //
  //! Use the observer interface to receive notifications when
  //! setCurrentTime is called on the master clock
  //!
  //! NOTE: this is an notification interface, therefore the implementation
  //! must be thread-safe, asynchronous, non-blocking...
  struct YAE_API IClockObserver
  {
    virtual ~IClockObserver() {}
    virtual void noteCurrentTimeChanged(const SharedClock & c,
                                        const TTime & t0) = 0;
    virtual void noteTheClockHasStopped(const SharedClock & c) = 0;
  };

  //----------------------------------------------------------------
  // TimeSegment
  //
  struct YAE_API TimeSegment
  {
    TimeSegment();

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
  typedef boost::shared_ptr<TimeSegment> TTimeSegmentPtr;

  //----------------------------------------------------------------
  // SharedClock
  //
  //! implicitly shared thread-safe time piece,
  //! useful for various synchronization tasks
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

  private:
    TTimeSegmentPtr shared_;
    boost::system_time waitingFor_;
    bool copied_;
  };

  //----------------------------------------------------------------
  // ISynchronous
  //
  struct YAE_API ISynchronous
  {
    virtual ~ISynchronous() {}

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
