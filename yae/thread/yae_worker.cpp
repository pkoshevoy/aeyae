// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Nov 24 14:30:02 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


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
  // Worker::Task::cancel
  //
  void
  Worker::Task::cancel()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    cancelled_ = true;
  }

  //----------------------------------------------------------------
  // Worker::Task::cancelled
  //
  bool
  Worker::Task::cancelled() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return cancelled_;
  }


  //----------------------------------------------------------------
  // Worker::Worker
  //
  Worker::Worker(unsigned int offset, unsigned int stride):
    offset_(offset),
    stride_(stride),
    limit_(0),
    count_(0),
    stop_(false)
  {
    start();
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
    // tell the thread to stop:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      stop_ = true;

      for (std::list<yae::shared_ptr<Task> >::iterator
             i = todo_.begin(); i != todo_.end(); ++i)
      {
        yae::shared_ptr<Task> & task_ptr = *i;
        task_ptr->cancel();
        task_ptr.reset();
      }

      signal_.notify_all();
    }

    thread_.stop();
    thread_.wait();
  }

  //----------------------------------------------------------------
  // Worker::stop_requested
  //
  bool
  Worker::stop_requested() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return stop_;
  }

  //----------------------------------------------------------------
  // Worker::thread_loop
  //
  void
  Worker::thread_loop()
  {
    while (true)
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      while (todo_.empty() && !stop_)
      {
        signal_.wait(lock);
      }

      if (stop_)
      {
        todo_.clear();
        count_ = 0;
        signal_.notify_all();
        break;
      }

      yae::shared_ptr<Task> task;
      while (!todo_.empty())
      {
        task = todo_.front();
        if (task)
        {
          break;
        }

        // cleanup cancelled tasks:
        todo_.pop_front();
        count_--;
        signal_.notify_all();
      }

      if (task)
      {
        lock.unlock();

        task->execute(*this);

        lock.lock();
        todo_.pop_front();
        YAE_ASSERT(count_ > 0);
        count_--;
        signal_.notify_all();
      }
    }
  }

  //----------------------------------------------------------------
  // Worker::set_queue_size_limit
  //
  void
  Worker::set_queue_size_limit(std::size_t n)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    limit_ = n;
    signal_.notify_all();
  }

  //----------------------------------------------------------------
  // Worker::add
  //
  void
  Worker::add(const yae::shared_ptr<Task> & task)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (limit_ && limit_ <= count_)
    {
      signal_.wait(lock);
    }

    if (!stop_)
    {
      todo_.push_back(task);
      count_++;
      signal_.notify_all();
    }
  }

  //----------------------------------------------------------------
  // Worker::wait_until_finished
  //
  void
  Worker::wait_until_finished()
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (!todo_.empty())
    {
      signal_.wait(lock);
    }
  }

  //----------------------------------------------------------------
  // Worker::is_busy
  //
  bool
  Worker::is_busy() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    bool has_tasks = count_ > 0;
    return has_tasks;
  }

}
