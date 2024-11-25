// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:41:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_THREADING_H_
#define YAE_THREADING_H_

// aeyae:
#include "yae/video/yae_video.h"

// standard:
#include <iostream>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS


namespace yae
{

  //----------------------------------------------------------------
  // Threadable
  //
  // Thread entry point:
  //
  template <typename TContext>
  struct Threadable
  {
    Threadable(TContext * context):
      context_(context)
    {}

    void operator()()
    {
      try
      {
        context_->thread_loop();
      }
      catch (const std::exception & e)
      {
        yae_debug << "Threadable::operator(): " << e.what();
      }
      catch (...)
      {
        yae_debug << "Threadable::operator(): unexpected exception";
      }
    }

    TContext * context_;
  };

  //----------------------------------------------------------------
  // Thread
  //
  template <typename TContext>
  struct Thread
  {
    Thread(TContext * context = NULL):
      thread_(NULL),
      context_(context)
    {}

    ~Thread()
    {
      delete thread_;
    }

    inline TContext * context() const
    {
      return context_;
    }

    void set_context(TContext * context)
    {
      YAE_ASSERT(!context || !context_);
      context_ = context;
    }

    bool run()
    {
      if (!context_)
      {
        YAE_ASSERT(false);
        return false;
      }

      if (thread_)
      {
        YAE_ASSERT(false);
        return false;
      }

      try
      {
        Threadable<TContext> threadable(context_);
        thread_ = new boost::thread(threadable);
        return true;
      }
      catch (const std::exception & e)
      {
        yae_debug << "Thread::run: " << e.what();
      }
      catch (...)
      {
        yae_debug << "Thread::run: unexpected exception";
      }

      delete thread_;
      thread_ = NULL;
      return false;
    }

    void interrupt()
    {
      try
      {
        if (thread_)
        {
          thread_->interrupt();
        }
      }
      catch (const std::exception & e)
      {
        yae_debug << "Thread::interrupt: " << e.what();
      }
      catch (...)
      {
        yae_debug << "Thread::interrupt: unexpected exception";
      }
    }

    bool wait()
    {
      try
      {
        if (thread_)
        {
          thread_->join();
          delete thread_;
          thread_ = NULL;
        }

        return true;
      }
      catch (const std::exception & e)
      {
        yae_debug << "Thread::wait: " << e.what();
      }
      catch (...)
      {
        yae_debug << "Thread::wait: unexpected exception";
      }

      return false;
    }

    bool isRunning() const
    {
      return thread_ && thread_->joinable();
    }

  protected:
    boost::thread * thread_;
    TContext * context_;
  };
}


#endif // YAE_THREADING_H_
