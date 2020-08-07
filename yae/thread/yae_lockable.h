// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Aug  7 10:15:30 MDT 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LOCKABLE_H_
#define YAE_LOCKABLE_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_time.h"


namespace yae
{

  //----------------------------------------------------------------
  // ILockable
  //
  struct ILockable
  {
    virtual ~ILockable() {}
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool try_lock() = 0;
    virtual bool timed_lock(const TTime & when_to_give_up) = 0;
  };


  //----------------------------------------------------------------
  // Mutex
  //
  struct Mutex : ILockable
  {
    Mutex();
    virtual ~Mutex();

    virtual void lock();
    virtual void unlock();
    virtual bool try_lock();
    virtual bool timed_lock(const TTime & when_to_give_up);

  private:
    Mutex(const Mutex &);
    Mutex & operator = (const Mutex &);

    struct Private;
    Private * private_;
  };


  //----------------------------------------------------------------
  // IWaitable
  //
  struct IWaitable
  {
    virtual ~IWaitable() {}

    virtual void wait(ILockable & lockable) = 0;
    virtual bool timed_wait(ILockable & lockable,
                            const TTime & when_to_give_up) = 0;
    virtual void notify_one() = 0;
    virtual void notify_all() = 0;
  };


  //----------------------------------------------------------------
  // ConditionVariable
  //
  struct ConditionVariable : IWaitable
  {
    ConditionVariable();
    ~ConditionVariable();

    void wait(ILockable & lockable);
    bool timed_wait(ILockable & lockable, const TTime & when_to_give_up);
    void notify_one();
    void notify_all();

  private:
    ConditionVariable(const ConditionVariable &);
    ConditionVariable & operator = (const ConditionVariable &);

    struct Private;
    Private * private_;
 };


  //----------------------------------------------------------------
  // Lock
  //
  template <typename TLockable>
  struct Lock : ILockable
  {
    Lock(TLockable & lockable, bool lock_now = true):
      lockable_(lockable),
      locked_(false)
    {
      if (lock_now)
      {
        lock();
      }
    }

    ~Lock()
    {
      unlock();
    }

    void lock()
    {
      if (!locked_)
      {
        lockable_.lock();
        locked_ = true;
      }
    }

    void unlock()
    {
      if (locked_)
      {
        lockable_.unlock();
        locked_ = false;
      }
    }

    bool try_lock()
    {
      if (!locked_ && lockable_.try_lock())
      {
        locked_ = true;
      }

      return locked_;
    }

    bool timed_lock(const TTime & when_to_give_up)
    {
      if (!locked_ && lockable_.timed_lock(when_to_give_up))
      {
        locked_ = true;
      }

      return locked_;
    }

  private:
    Lock(const Lock &);
    Lock & operator = (const Lock &);

    TLockable & lockable_;
    bool locked_;
  };


  //----------------------------------------------------------------
  // Waiter
  //
  template <typename TWaitable>
  struct Waiter
  {
    Waiter():
      waitable_(NULL),
      wait_(true)
    {}

    inline void waiting_for(TWaitable * waitable)
    {
      Lock<Mutex> lock(mutex_);
      waitable_ = waitable;
    }

    inline void stop_waiting(bool stop = true)
    {
      Lock<Mutex> lock(mutex_);
      wait_ = !stop;

      if (waitable_)
      {
        waitable_->notify_all();
      }
    }

    inline bool keep_waiting() const
    {
      Lock<Mutex> lock(mutex_);
      return wait_;
    }

  protected:
    mutable Mutex mutex_;
    TWaitable * waitable_;
    bool wait_;
  };


  //----------------------------------------------------------------
  // WaitTerminator
  //
  template <typename TWaitable>
  struct WaitTerminator
  {
    typedef Waiter<TWaitable> TWaiter;

    WaitTerminator(TWaiter * waiter, TWaitable * waitable):
      waiter_(waiter)
    {
      if (waiter_)
      {
        waiter_->waiting_for(waitable);
      }
    }

    ~WaitTerminator()
    {
      if (waiter_)
      {
        waiter_->waiting_for(NULL);
      }
    }

    inline bool keep_waiting() const
    {
      return waiter_ ? waiter_->keep_waiting() : true;
    }

  private:
    TWaiter * waiter_;
  };

}


#endif // YAE_LOCKABLE_H_
