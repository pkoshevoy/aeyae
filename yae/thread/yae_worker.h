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
      virtual ~Task() {}
      virtual void execute(const Worker & worker) = 0;
    };

    Worker(unsigned int offset = 0, unsigned int stride = 1);
    ~Worker();

    void start();
    void stop();

    void set_queue_size_limit(std::size_t n);
    void add(const yae::shared_ptr<Task> & task);
    void wait_until_finished();

    bool is_busy() const;

    inline bool is_idle() const
    { return !is_busy(); }

    const unsigned int offset_;
    const unsigned int stride_;

    void thread_loop();

  private:
    // intentionally disabled:
    Worker(const Worker &);
    Worker & operator = (const Worker &);

  protected:
    mutable boost::mutex mutex_;
    mutable boost::condition_variable signal_;
    std::size_t limit_;
    std::size_t count_;
    std::list<yae::shared_ptr<Task> > todo_;
    Thread<Worker> thread_;
    bool stop_;
  };

}


#endif // YAE_WORKER_H_
