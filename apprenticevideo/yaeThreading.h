// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 15 12:41:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_THREADING_H_
#define YAE_THREADING_H_

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
      context_->threadLoop();
    }
    
    TContext * context_;
  };
  
  //----------------------------------------------------------------
  // Thread
  // 
  template <typename TContext>
  struct Thread
  {
    Thread(TContext * context):
      context_(context)
    {}
    
    bool run()
    {
      if (!context_)
      {
        assert(false);
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
    
  protected:
    boost::thread thread_;
    TContext * context_;
  };
  
}


#endif // YAE_THREADING_H_
