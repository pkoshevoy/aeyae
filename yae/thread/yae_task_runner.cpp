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
#include "yae/api/yae_log.h"
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
    void thread_loop();

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
    thread_.set_context(this);
    thread_.run();
  }

  //----------------------------------------------------------------
  // TaskRunner::TPrivate::~TPrivate
  //
  TaskRunner::TPrivate::~TPrivate()
  {
    thread_.interrupt();
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
  // TaskRunner::TPrivate::thread_loop
  //
  void
  TaskRunner::TPrivate::thread_loop()
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
    typedef AsyncTaskQueue::TCallback TCallback;

    struct Todo
    {
      Todo(const TaskPtr & task = TaskPtr(),
           TCallback callback = NULL,
           void * context = NULL):
        task_(task),
        callback_(callback),
        context_(context)
      {}

      TaskPtr task_;
      TCallback callback_;
      void * context_;
    };

    Private():
      stop_(false),
      pause_(false),
      paused_(false)
    {
      thread_.set_context(this);
      thread_.run();
    }

    ~Private()
    {
      // attempt a graceful shutdown:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        stop_ = true;
        signal_.notify_all();
      }

      thread_.interrupt();
      thread_.wait();
    }

    void pause()
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      pause_ = true;
      signal_.notify_all();

      while (!paused_)
      {
        signal_.wait(lock);
      }
    }

    void resume()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      pause_ = false;
      signal_.notify_all();
    }

    void push_front(const TaskPtr & task, TCallback callback, void * context)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      todo_.push_front(Todo(task, callback, context));
      signal_.notify_all();
    }

    void push_back(const TaskPtr & task, TCallback callback, void * context)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      todo_.push_back(Todo(task, callback, context));
      signal_.notify_all();
    }

    void thread_loop()
    {
      while (!stop_)
      {
        // wait for work:
        boost::unique_lock<boost::mutex> lock(mutex_);
        while (!stop_ && todo_.empty())
        {
          // sleep until there is at least one task in the queue:
          paused_ = true;
          signal_.notify_all();
          signal_.wait(lock);
          boost::this_thread::interruption_point();
        }

        paused_ = false;
        signal_.notify_all();

        while (!stop_ && !todo_.empty())
        {
          while (!stop_ && pause_)
          {
            // sleep until processing is resumed:
            paused_ = true;
            signal_.notify_all();
            signal_.wait(lock);
            boost::this_thread::interruption_point();
          }

          paused_ = false;
          signal_.notify_all();

          Todo todo = todo_.front();
          todo_.pop_front();

          yae::shared_ptr<Task> task = todo.task_.lock();
          if (task)
          {
            lock.unlock();

            try
            {
              task->run();

              if (todo.callback_)
              {
                todo.callback_(task, todo.context_);
              }
            }
            catch (const std::exception & e)
            {
              yae_error << "AsyncTaskQueue task exception: " << e.what();
            }
            catch (...)
            {
              yae_error << "AsyncTaskQueue unexpected task exception";
            }

            lock.lock();
          }

          boost::this_thread::interruption_point();
        }
      }
    }

    boost::mutex mutex_;
    std::list<Todo> todo_;
    boost::condition_variable signal_;
    Thread<AsyncTaskQueue::Private> thread_;
    bool stop_;
    bool pause_;
    bool paused_;
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
  // AsyncTaskQueue::push_front
  //
  void
  AsyncTaskQueue::push_front(const AsyncTaskQueue::TaskPtr & task,
                             AsyncTaskQueue::TCallback callback,
                             void * context)
  {
    private_->push_front(task, callback, context);
  }

  //----------------------------------------------------------------
  // AsyncTaskQueue::push_back
  //
  void
  AsyncTaskQueue::push_back(const AsyncTaskQueue::TaskPtr & task,
                             AsyncTaskQueue::TCallback callback,
                             void * context)
  {
    private_->push_back(task, callback, context);
  }

  //----------------------------------------------------------------
  // AsyncTaskQueue::pause
  //
  void
  AsyncTaskQueue::pause()
  {
    private_->pause();
  }

  //----------------------------------------------------------------
  // AsyncTaskQueue::resume
  //
  void
  AsyncTaskQueue::resume()
  {
    private_->resume();
  }

}
