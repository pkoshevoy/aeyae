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
  // TaskRunner::TPrivate
  //
  struct TaskRunner::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void add(const Task::TimePoint & t, const TaskPtr & task);
    void threadLoop();

    Thread<TaskRunner::TPrivate> thread_;
    boost::mutex mutex_;
    boost::condition_variable cond_;
    std::map<Task::TimePoint, std::list<TaskPtr> > tasks_;
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
  TaskRunner::TPrivate::add(const Task::TimePoint & t, const TaskPtr & task)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    tasks_[t].push_back(task);
    cond_.notify_all();
  }

  //----------------------------------------------------------------
  // ready
  //
  static bool
  ready(std::map<Task::TimePoint, std::list<TaskPtr> > & tasks,
        std::list<TaskPtr> & next,
        Task::TimePoint & next_time_point)
  {
    std::map<Task::TimePoint, std::list<TaskPtr> >::iterator i = tasks.begin();
    if (i == tasks.end())
    {
      return false;
    }

    Task::TimePoint now = (boost::chrono::steady_clock::now() +
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
    std::list<TaskPtr> next;
    Task::TimePoint next_time_point;

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
        TaskPtr task = next.front();
        next.pop_front();
        task->run();
      }

      Task::TimePoint now = boost::chrono::steady_clock::now();
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
  TaskRunner::add(const Task::TimePoint & t, const TaskPtr & task)
  {
    private_->add(t, task);
  }

}
