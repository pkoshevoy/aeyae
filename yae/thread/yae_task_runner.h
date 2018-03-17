// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jan  6 21:56:03 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TASK_RUNNER_H_
#define YAE_TASK_RUNNER_H_

// boost libraries:
#ifndef Q_MOC_RUN
#include <boost/chrono/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif


namespace yae
{
  //----------------------------------------------------------------
  // TaskRunner
  //
  struct YAE_API TaskRunner
  {
    //----------------------------------------------------------------
    // TimePoint
    //
    typedef boost::chrono::steady_clock::time_point TimePoint;

    //----------------------------------------------------------------
    // Task
    //
    struct YAE_API Task
    {
      Task(const TimePoint & start = boost::chrono::steady_clock::now()):
        start_(start)
      {}

      virtual ~Task() {}
      virtual void run() = 0;

      TimePoint start_;
    };

    //----------------------------------------------------------------
    // TaskPtr
    //
    typedef boost::shared_ptr<Task> TaskPtr;

    //----------------------------------------------------------------
    // Status
    //
    struct Status
    {
      Status():
        done_(true)
      {}

      bool setDone(bool done)
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        bool was_done = done_;
        done_ = done;
        return was_done;
      }

    private:
      mutable boost::mutex mutex_;
      bool done_;
    };

    //----------------------------------------------------------------
    // Todo
    //
    struct Todo : public Task
    {
      Todo(Status & status):
        status_(status)
      {}

      Status & status_;
    };

    static TaskRunner & singleton();

    TaskRunner();
    ~TaskRunner();

    void add(const TimePoint & t, const TaskPtr & task);

  private:
    // intentionally disabled:
    TaskRunner(const TaskRunner &);
    TaskRunner & operator = (const TaskRunner &);

    struct TPrivate;
    TPrivate * private_;
  };


  //----------------------------------------------------------------
  // AsyncTaskQueue
  //
  struct AsyncTaskQueue
  {
    struct Task
    {
      virtual ~Task() {}
      virtual void run() = 0;
    };

    typedef boost::weak_ptr<Task> TaskPtr;

    AsyncTaskQueue();
    ~AsyncTaskQueue();

    void add(const TaskPtr & task);

  protected:
    // intentionally disabled:
    AsyncTaskQueue(const AsyncTaskQueue &);
    AsyncTaskQueue & operator = (const AsyncTaskQueue &);

    struct Private;
    Private * private_;
  };
}


#endif // YAE_TASK_RUNNER_H_
