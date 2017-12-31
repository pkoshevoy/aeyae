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
  // Task
  //
  struct YAE_API Task
  {
    //----------------------------------------------------------------
    // TimePoint
    //
    typedef boost::chrono::steady_clock::time_point TimePoint;

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
  // Todo
  //
  struct Todo : public Task
  {
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

    Todo(Status & status):
      status_(status)
    {}

    Status & status_;
  };

  //----------------------------------------------------------------
  // TaskRunner
  //
  struct YAE_API TaskRunner
  {
    static TaskRunner & singleton();

    TaskRunner();
    ~TaskRunner();

    void add(const Task::TimePoint & t, const TaskPtr & task);

  private:
    // intentionally disabled:
    TaskRunner(const TaskRunner &);
    TaskRunner & operator = (const TaskRunner &);

    struct TPrivate;
    TPrivate * private_;
  };
}


#endif // YAE_TASK_RUNNER_H_
