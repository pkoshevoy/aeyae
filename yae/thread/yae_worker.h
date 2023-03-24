// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Nov 24 14:30:02 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_WORKER_H_
#define YAE_WORKER_H_

// standard:
#include <list>
#include <vector>

// boost libraries:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/thread/yae_threading.h"


namespace yae
{

  //----------------------------------------------------------------
  // Worker
  //
  struct YAE_API Worker
  {
    //----------------------------------------------------------------
    // Task
    //
    struct YAE_API Task
    {
      Task();
      virtual ~Task();

      virtual void execute(const Worker & worker) = 0;
      virtual void cancel();
      virtual bool cancelled() const;

    protected:
      mutable boost::condition_variable signal_;
      mutable boost::mutex mutex_;
      bool cancelled_;
    };

    //----------------------------------------------------------------
    // TTaskPtr
    //
    typedef yae::shared_ptr<Task> TTaskPtr;


    //----------------------------------------------------------------
    // TaskQueue
    //
    struct YAE_API TaskQueue
    {
      TaskQueue(std::size_t limit = 0):
        limit_(limit),
        todo_(0),
        busy_(0)
      {}

    protected:
      friend struct yae::Worker;

      mutable boost::mutex mutex_;
      boost::condition_variable signal_;
      std::list<TTaskPtr> fifo_;

      // 0 == unlimited:
      std::size_t limit_;

      // NOTE: the number of waiting and executin tasks
      // must be less than or equal to the above limit:
      std::size_t todo_;
      std::size_t busy_;
    };

    //----------------------------------------------------------------
    // TTaskQueuePtr
    //
    typedef yae::shared_ptr<TaskQueue> TTaskQueuePtr;


    //----------------------------------------------------------------
    // Worker
    //
    Worker(unsigned int offset = 0,
           unsigned int stride = 1,
           const std::string & name = std::string(),
           const TTaskQueuePtr & task_queue = TTaskQueuePtr(),
           bool start_now = true);
    ~Worker();

    void start();
    void stop();
    bool stop_requested() const;

    inline const std::string & name() const
    { return name_; }

    inline TTaskQueuePtr get_queue() const
    { return tasks_; }

    // replace current task queue with a given task queue (if different),
    // return previous task queue:
    TTaskQueuePtr set_queue(const TTaskQueuePtr & task_queue);

    void set_queue_size_limit(std::size_t n);
    std::size_t get_queue_size_limit() const;

    bool add(const yae::shared_ptr<Task> & task);
    void wait_until_finished();

    bool is_busy() const;

    inline bool is_idle() const
    { return !is_busy(); }

    const unsigned int offset_;
    const unsigned int stride_;

    void thread_loop();

    void interrupt();

  private:
    // intentionally disabled:
    Worker(const Worker &);
    Worker & operator = (const Worker &);

  protected:
    std::string name_;
    Thread<Worker> thread_;
    TTaskQueuePtr tasks_;
    TTaskPtr busy_;
    bool stop_;
  };

  //----------------------------------------------------------------
  // TWorkerPtr
  //
  typedef yae::shared_ptr<yae::Worker> TWorkerPtr;

}


#endif // YAE_WORKER_H_
