// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Jan  6 21:56:03 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <list>
#include <map>

// boost libraries:
#include <boost/thread.hpp>

// aeyae:
#include "yae_threading.h"
#include "yae_task_runner.h"


namespace yae
{

  //----------------------------------------------------------------
  // TTaskRunnerTasks
  //
  typedef std::list<TaskRunner::TaskPtr> TTaskRunnerTasks;

  //----------------------------------------------------------------
  // TTimePoint
  //
  typedef TaskRunner::TimePoint TTimePoint;

  //----------------------------------------------------------------
  // TaskRunner::TPrivate
  //
  struct TaskRunner::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void add(const TTimePoint & t, const TaskRunner::TaskPtr & task);
    void threadLoop();

    Thread<TaskRunner::TPrivate> thread_;
    boost::mutex mutex_;
    boost::condition_variable cond_;

    std::map<TTimePoint, TTaskRunnerTasks> tasks_;
  };

  //----------------------------------------------------------------
  // TaskRunner::TPrivate::TPrivate
  //
  TaskRunner::TPrivate::TPrivate()
  {
    thread_.setContext(this);
    thread_.run();
  }

  //----------------------------------------------------------------
  // TaskRunner::TPrivate::~TPrivate
  //
  TaskRunner::TPrivate::~TPrivate()
  {
    thread_.stop();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // TaskRunner::TPrivate::add
  //
  void
  TaskRunner::TPrivate::add(const TTimePoint & t,
                            const TaskRunner::TaskPtr & task)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    tasks_[t].push_back(task);
    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // ready
  //
  static bool
  ready(std::map<TTimePoint, TTaskRunnerTasks> & tasks,
        std::list<TaskRunner::TaskPtr> & next,
        TTimePoint & next_time_point)
  {
    std::map<TTimePoint, TTaskRunnerTasks>::iterator i = tasks.begin();
    if (i == tasks.end())
    {
      return false;
    }

    TTimePoint now = (boost::chrono::steady_clock::now() +
                      boost::chrono::milliseconds(1));

    if (now < i->first)
    {
      next_time_point = i->first;
      return false;
    }

    next.swap(i->second);
    tasks.erase(i);
    return true;
  }

  //----------------------------------------------------------------
  // TaskRunner::TPrivate::threadLoop
  //
  void
  TaskRunner::TPrivate::threadLoop()
  {
    TTaskRunnerTasks next;
    TTimePoint next_time_point;

    while (true)
    {
      // grab the next set of tasks:
      {
        boost::unique_lock<boost::mutex> lock(mutex_);
        while (tasks_.empty())
        {
          // sleep until there is at least one task in the queue:
          cond_.wait(lock);
          boost::this_thread::interruption_point();
        }

        ready(tasks_, next, next_time_point);
      }

      while (!next.empty())
      {
        boost::this_thread::interruption_point();
        TaskRunner::TaskPtr task = next.front();
        next.pop_front();
        task->run();
      }

      TTimePoint now = boost::chrono::steady_clock::now();
      if (now < next_time_point)
      {
        boost::chrono::nanoseconds ns = next_time_point - now;
        boost::this_thread::sleep_for(ns);
      }
    }
  }

  //----------------------------------------------------------------
  // TaskRunner::singleton
  //
  TaskRunner &
  TaskRunner::singleton()
  {
    static TaskRunner taskRunner;
    return taskRunner;
  }

  //----------------------------------------------------------------
  // TaskRunner::TaskRunner
  //
  TaskRunner::TaskRunner():
    private_(new TPrivate())
  {}

  //----------------------------------------------------------------
  // TaskRunner::~TaskRunner
  //
  TaskRunner::~TaskRunner()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TaskRunner::add
  //
  void
  TaskRunner::add(const TTimePoint & t, const TaskPtr & task)
  {
    private_->add(t, task);
  }


  //----------------------------------------------------------------
  // AsyncTaskQueue::Private
  //
  struct AsyncTaskQueue::Private
  {
    typedef AsyncTaskQueue::Task Task;
    typedef AsyncTaskQueue::TaskPtr TaskPtr;

    Private()
    {
      thread_.setContext(this);
      thread_.run();
    }

    ~Private()
    {
      thread_.stop();
      thread_.wait();
    }

    void add(const TaskPtr & task)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      tasks_.push_front(task);
      signal_.notify_all();
    }

    void threadLoop()
    {
      while (true)
      {
        // wait for work:
        boost::unique_lock<boost::mutex> lock(mutex_);
        while (tasks_.empty())
        {
          // sleep until there is at least one task in the queue:
          signal_.wait(lock);
          boost::this_thread::interruption_point();
        }

        while (!tasks_.empty())
        {
          boost::shared_ptr<Task> task = tasks_.front().lock();
          tasks_.pop_front();

          if (task)
          {
            lock.unlock();
            task->run();
            lock.lock();
          }

          boost::this_thread::interruption_point();
        }
      }
    }

    boost::mutex mutex_;
    std::list<TaskPtr> tasks_;
    boost::condition_variable signal_;
    Thread<AsyncTaskQueue::Private> thread_;
  };


  //----------------------------------------------------------------
  // AsyncTaskQueue::AsyncTaskQueue
  //
  AsyncTaskQueue::AsyncTaskQueue():
    private_(new AsyncTaskQueue::Private())
  {}

  //----------------------------------------------------------------
  // AsyncTaskQueue::~AsyncTaskQueue
  //
  AsyncTaskQueue::~AsyncTaskQueue()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // AsyncTaskQueue::add
  //
  void
  AsyncTaskQueue::add(const AsyncTaskQueue::TaskPtr & task)
  {
    private_->add(task);
  }

}
