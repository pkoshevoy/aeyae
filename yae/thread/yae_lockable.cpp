// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Aug  7 10:15:30 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/thread/yae_lockable.h"


namespace yae
{

  //----------------------------------------------------------------
  // Mutex::Private
  //
  struct Mutex::Private
  {
    inline void lock()
    { mutex_.lock(); }

    inline void unlock()
    { mutex_.unlock(); }

    inline bool try_lock()
    { mutex_.try_lock(); }

    inline bool timed_lock(const TTime & quitting_time)
    {
      if (quitting_time.invalid())
      {
        return false;
      }

      int64_t timeout_msec = (quitting_time - TTime::now()).get(1000000);
      boost::system_time when_to_give_up =
        boost::get_system_time() +
        boost::posix_time::microseconds(timeout_msec);

      return mutex_.timed_lock(when_to_give_up);
    }

    mutable boost::timed_mutex mutex_;
  };

  //----------------------------------------------------------------
  // Mutex::Mutex
  //
  Mutex::Mutex():
    private_(new Mutex::Private())
  {}

  //----------------------------------------------------------------
  // Mutex::~Mutex
  //
  Mutex::~Mutex()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // Mutex::lock
  //
  void
  Mutex::lock()
  {
    private_->lock();
  }

  //----------------------------------------------------------------
  // Mutex::unlock
  //
  void
  Mutex::unlock()
  {
    private_->unlock();
  }

  //----------------------------------------------------------------
  // Mutex::try_lock
  //
  bool
  Mutex::try_lock()
  {
    return private_->try_lock();
  }

  //----------------------------------------------------------------
  // Mutex::timed_lock
  //
  bool
  Mutex::timed_lock(const TTime & when_to_give_up)
  {
    return private_->timed_lock(when_to_give_up);
  }


  //----------------------------------------------------------------
  // ConditionVariable::Private
  //
  struct ConditionVariable::Private
  {
    bool wait(ILockable & lockable,
              const TTime & quitting_time = TTime(0, 0))
    {
      // add an observer:
      boost::shared_ptr<boost::timed_mutex> observer(new boost::timed_mutex());
      observer->lock();
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        observers_.push_back(observer);
      }

      bool ok = false;
      try
      {
        lockable.unlock();

        if (quitting_time.valid())
        {
          int64_t timeout_msec = (quitting_time - TTime::now()).get(1000000);
          boost::system_time when_to_give_up =
            boost::get_system_time() +
            boost::posix_time::microseconds(timeout_msec);
          ok = observer->timed_lock(when_to_give_up);
        }
        else
        {
          observer->lock();
          ok = true;
        }

        lockable.lock();
      }
      catch (...)
      {}

      if (!ok)
      {
        // remove the observer:
        boost::lock_guard<boost::mutex> lock(mutex_);
        observers_.remove(observer);
      }

      observer->unlock();
      return ok;
    }

    void notify_one()
    {
      boost::shared_ptr<boost::timed_mutex> observer;
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        if (observers_.empty())
        {
          return;
        }

        observer = observers_.front();
        observers_.pop_front();
      }

      observer->unlock();
    }

    void notify_all()
    {
      while (true)
      {
        boost::shared_ptr<boost::timed_mutex> observer;
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          if (observers_.empty())
          {
            return;
          }

          observer = observers_.front();
          observers_.pop_front();
        }

        observer->unlock();
      }
    }

  protected:
    boost::mutex mutex_;
    std::list<boost::shared_ptr<boost::timed_mutex> > observers_;
  };


  //----------------------------------------------------------------
  // ConditionVariable::ConditionVariable
  //
  ConditionVariable::ConditionVariable():
    private_(new ConditionVariable::Private())
  {}

  //----------------------------------------------------------------
  // ConditionVariable::~ConditionVariable
  //
  ConditionVariable::~ConditionVariable()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ConditionVariable::wait
  //
  void
  ConditionVariable::wait(ILockable & lockable)
  {
    private_->wait(lockable);
  }

  //----------------------------------------------------------------
  // ConditionVariable::timed_wait
  //
  bool
  ConditionVariable::timed_wait(ILockable & lockable,
                                const TTime & when_to_give_up)
  {
    return private_->wait(lockable, when_to_give_up);
  }

  //----------------------------------------------------------------
  // ConditionVariable::notify_one
  //
  void
  ConditionVariable::notify_one()
  {
    private_->notify_one();
  }

  //----------------------------------------------------------------
  // ConditionVariable::notify_all
  //
  void
  ConditionVariable::notify_all()
  {
    private_->notify_all();
  }

}
