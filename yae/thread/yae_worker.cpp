// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Nov 24 14:30:02 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <memory>

// aeyae:
#include "yae_worker.h"


namespace yae
{

  //----------------------------------------------------------------
  // Worker::Task::Task
  //
  Worker::Task::Task():
    cancelled_(false)
  {}

  //----------------------------------------------------------------
  // Worker::Task::~Task
  //
  Worker::Task::~Task()
  {}


  //----------------------------------------------------------------
  // Worker::Worker
  //
  Worker::Worker(unsigned int offset,
                 unsigned int stride,
                 const std::string & name,
                 const Worker::TTaskQueuePtr & task_queue,
                 bool start_now):
    offset_(offset),
    stride_(stride),
    name_(name),
    tasks_(task_queue),
    stop_(true)
  {
    if (name_.empty())
    {
      name_ = yae::strfmt("Worker %p", this);
    }

    if (!tasks_)
    {
      tasks_.reset(new TaskQueue());
    }

    if (start_now)
    {
      start();
    }
  }

  //----------------------------------------------------------------
  // Worker::~Worker
  //
  Worker::~Worker()
  {
    stop();
  }

  //----------------------------------------------------------------
  // Worker::start
  //
  void
  Worker::start()
  {
    // keep alive the task queue in case it is
    // replaced while we are accessing it here:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    if (!stop_)
    {
      return;
    }

    stop_ = false;
    thread_.set_context(this);
    thread_.run();
  }

  //----------------------------------------------------------------
  // Worker::stop
  //
  void
  Worker::stop()
  {
    // keep alive the task queue in case it is
    // replaced while we are accessing it here:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    // tell the thread to stop:
    {
      boost::unique_lock<boost::mutex> lock(tasks.mutex_);
      stop_ = true;

      for (std::list<yae::shared_ptr<Task> >::iterator
             i = tasks.fifo_.begin(); i != tasks.fifo_.end(); ++i)
      {
        TTaskPtr & task_ptr = *i;
        if (task_ptr)
        {
          task_ptr->cancel();
          task_ptr.reset();
        }
      }

      if (busy_)
      {
        busy_->cancel();
        busy_.reset();
      }

      tasks.signal_.notify_all();
    }

    thread_.interrupt();
    thread_.wait();
    thread_.set_context(NULL);
  }

  //----------------------------------------------------------------
  // Worker::stop_requested
  //
  bool
  Worker::stop_requested() const
  {
    // keep alive the task queue in case it is
    // replaced while we are accessing it here:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    return stop_;
  }

  //----------------------------------------------------------------
  // Worker::interrupt
  //
  void
  Worker::interrupt()
  {
    thread_.interrupt();
  }

  //----------------------------------------------------------------
  // Worker::thread_loop
  //
  void
  Worker::thread_loop()
  {
    while (true)
    {
      // keep alive the task queue in case it is
      // replaced while we are accessing it here:
      TTaskQueuePtr task_queue = tasks_;

      // shortcut:
      TaskQueue & tasks = *task_queue;

      boost::unique_lock<boost::mutex> lock(tasks.mutex_);
      while (tasks.fifo_.empty() && !stop_)
      {
        try
        {
          tasks.signal_.wait(lock);
        }
        catch (...)
        {}
      }

      if (stop_)
      {
        tasks.fifo_.clear();
        tasks.todo_ = 0;
        tasks.signal_.notify_all();
        break;
      }

      TTaskPtr task;
      while (!tasks.fifo_.empty())
      {
        YAE_ASSERT(tasks.todo_ > 0);
        task = tasks.fifo_.front();
        tasks.fifo_.pop_front();
        tasks.todo_--;

        if (task)
        {
          tasks.busy_++;
        }
        else
        {
          // wake up anyone that was blocked waiting to add another task:
          tasks.signal_.notify_all();
        }

        // break out regardless whether we got a task or not,
        // so that we can refresh the keep_alive copy
        // of the tasks_ queue, in case it was replaced:
        break;
      }

      if (task)
      {
        busy_ = task;
        lock.unlock();

        try
        {
          task->execute(*this);
        }
        catch (const std::exception & e)
        {
          yae_elog("%s: task failed, exception: %s", name_.c_str(), e.what());
        }
        catch (...)
        {
          yae_elog("%s: task failed, unexpected exception", name_.c_str());
        }

        lock.lock();
        busy_.reset();
        YAE_ASSERT(tasks.busy_ > 0);
        tasks.busy_--;
        tasks.signal_.notify_all();
      }
    }
  }

  //----------------------------------------------------------------
  // Worker::set_queue
  //
  Worker::TTaskQueuePtr
  Worker::set_queue(const Worker::TTaskQueuePtr & task_queue)
  {
    YAE_THROW_IF(!task_queue);

    TTaskQueuePtr prev_queue = tasks_;
    if (task_queue != prev_queue)
    {
      tasks_ = task_queue;

      // unblock thread_loop so it would stop waiting on the old task queue:
      TaskQueue & prev_tasks = *prev_queue;
      boost::unique_lock<boost::mutex> lock(prev_tasks.mutex_);
      prev_tasks.fifo_.push_front(TTaskPtr());
      prev_tasks.todo_++;
      prev_tasks.signal_.notify_all();
    }

    return prev_queue;
  }

  //----------------------------------------------------------------
  // Worker::set_queue_size_limit
  //
  void
  Worker::set_queue_size_limit(std::size_t n)
  {
    // keep alive the task queue in case it is
    // replaced while we are accessing it here:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    tasks.limit_ = n;
    tasks.signal_.notify_all();
  }

  //----------------------------------------------------------------
  // Worker::get_queue_size_limit
  //
  std::size_t
  Worker::get_queue_size_limit() const
  {
    // keep alive the task queue in case it is replaced at runtime:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    return tasks.limit_;
  }

  //----------------------------------------------------------------
  // Worker::add
  //
  bool
  Worker::add(const yae::shared_ptr<Task> & task)
  {
    // keep alive the task queue in case it is
    // replaced while we are accessing it here:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    while (!stop_ &&
           tasks.limit_ &&
           tasks.limit_ <= (tasks.todo_ + tasks.busy_))
    {
      try
      {
        tasks.signal_.wait(lock);
      }
      catch (...)
      {}
    }

    if (stop_)
    {
      return false;
    }

    tasks.fifo_.push_back(task);
    tasks.todo_++;
    tasks.signal_.notify_all();
    return true;
  }

  //----------------------------------------------------------------
  // Worker::wait_until_finished
  //
  void
  Worker::wait_until_finished()
  {
    // keep alive the task queue in case it is replaced at runtime:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    while (true)
    {
      if (tasks.fifo_.empty())
      {
        if (!tasks.busy_)
        {
          // done:
          break;
        }
      }
      else if (!tasks.fifo_.front())
      {
        // clean up cancelled tasks:
        YAE_ASSERT(tasks.todo_ > 0);
        tasks.fifo_.pop_front();
        tasks.todo_--;
        continue;
      }

      try
      {
        tasks.signal_.wait(lock);
      }
      catch (...)
      {}
    }

    YAE_ASSERT(!tasks.todo_ && !tasks.busy_);
  }

  //----------------------------------------------------------------
  // Worker::is_busy
  //
  bool
  Worker::is_busy() const
  {
    // keep alive the task queue in case it is replaced at runtime:
    TTaskQueuePtr task_queue = tasks_;

    // shortcut:
    TaskQueue & tasks = *task_queue;

    boost::unique_lock<boost::mutex> lock(tasks.mutex_);
    bool has_tasks = (tasks.todo_ + tasks.busy_) > 0;
    return has_tasks;
  }

}
