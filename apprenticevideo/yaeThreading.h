// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:41:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_THREADING_H_
#define YAE_THREADING_H_

// system includes:
#include <iostream>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>


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
        context_->threadLoop();
      }
      catch (std::exception & e)
      {
        std::cerr << "Threadable::operator(): " << e.what()
                  << std::endl;
      }
      catch (...)
      {
        std::cerr << "Threadable::operator(): unexpected exception"
                  << std::endl;
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
      context_(context)
    {}

    void setContext(TContext * context)
    {
      YAE_ASSERT(!context_);
      context_ = context;
    }

    bool run()
    {
      if (!context_)
      {
        YAE_ASSERT(false);
        return false;
      }

      try
      {
        Threadable<TContext> threadable(context_);
        thread_ = boost::thread(threadable);
        return true;
      }
      catch (std::exception & e)
      {
        std::cerr << "Thread::start: " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "Thread::start: unexpected exception" << std::endl;
      }

      return false;
    }

    void stop()
    {
      try
      {
        thread_.interrupt();
      }
      catch (std::exception & e)
      {
        std::cerr << "Thread::stop: " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "Thread::stop: unexpected exception" << std::endl;
      }
    }

    bool wait()
    {
      try
      {
        thread_.join();
        return true;
      }
      catch (std::exception & e)
      {
        std::cerr << "Thread::wait: " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "Thread::wait: unexpected exception" << std::endl;
      }

      return false;
    }

    bool isRunning() const
    {
      return thread_.joinable();
    }

  protected:
    boost::thread thread_;
    TContext * context_;
  };
}


#endif // YAE_THREADING_H_
