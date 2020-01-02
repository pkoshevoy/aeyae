// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_log.h"

// local:
#include "yae_signal_handler.h"


namespace yae
{

  //----------------------------------------------------------------
  // signal_handler_cb
  //
  static void
  signal_handler_cb(int sig)
  {
    yae_wlog("caught signal: %i", sig);
    SignalHandler & sh = signal_handler();
    sh.handle(sig);
  }


  //----------------------------------------------------------------
  // SignalHandler::SignalHandler
  //
  SignalHandler::SignalHandler()
  {
#if defined(SIGINFO)
    signal(SIGINFO, &signal_handler_cb);
#endif

#if defined(SIGPIPE)
    signal(SIGPIPE, &signal_handler_cb);
#endif

#if defined(SIGINT)
    signal(SIGINT, &signal_handler_cb);
#endif
  }

  //----------------------------------------------------------------
  // SignalHandler::handle
  //
  void
  SignalHandler::handle(int sig)
  {
    // hold the mutex to update the received signals set, and to notify:
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      received_.insert(sig);
      signal_.notify_all();
    }

    call_callbacks(sig);
  }

  //----------------------------------------------------------------
  // SignalHandler::acknowledge
  //
  bool
  SignalHandler::acknowledge(int sig)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    std::set<int>::iterator found = received_.find(sig);
    if (found == received_.end())
    {
      return false;
    }

    received_.erase(found);
    return true;
  }

  //----------------------------------------------------------------
  // SignalHandler::received_siginfo
  //
  bool
  SignalHandler::received_siginfo()
  {
#if defined(SIGINFO)
    return acknowledge(SIGINFO);
#else
    return false;
#endif
  }

  //----------------------------------------------------------------
  // SignalHandler::received_sigpipe
  //
  bool
  SignalHandler::received_sigpipe()
  {
#if defined(SIGPIPE)
    return acknowledge(SIGPIPE);
#else
    return false;
#endif
  }

  //----------------------------------------------------------------
  // SignalHandler::received_sigint
  //
  bool
  SignalHandler::received_sigint()
  {
#if defined(SIGINT)
    return acknowledge(SIGINT);
#else
    return false;
#endif
  }

  //----------------------------------------------------------------
  // SignalHandler::add
  //
  void
  SignalHandler::add(SignalHandler::TFuncPtr callback, void * context)
  {
    callback_.insert(std::make_pair(callback, context));

    std::set<int> received;
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      received.swap(received_);
    }

    for (std::set<int>::const_iterator
           i = received.begin(); i != received.end(); ++i)
    {
      int sig = *i;
      call_callbacks(sig);
    }
  }

  //----------------------------------------------------------------
  // SignalHandler::call_callbacks
  //
  void
  SignalHandler::call_callbacks(int sig)
  {
    for (std::set<std::pair<TFuncPtr, void *> >::const_iterator
           i = callback_.begin(); i != callback_.end(); ++i)
    {
      const TFuncPtr & callback = (*i).first;
      void * context = (*i).second;
      callback(context, sig);
    }
  }

  //----------------------------------------------------------------
  // signal_handler
  //
  SignalHandler &
  signal_handler()
  {
    static SignalHandler signal_handler_;
    return signal_handler_;
  }

  //----------------------------------------------------------------
  // signal_handler_received_siginfo
  //
  bool
  signal_handler_received_siginfo()
  {
    return signal_handler().received_siginfo();
  }

  //----------------------------------------------------------------
  // signal_handler_received_sigpipe
  //
  bool
  signal_handler_received_sigpipe()
  {
    return signal_handler().received_sigpipe();
  }

  //----------------------------------------------------------------
  // signal_handler_received_sigint
  //
  bool
  signal_handler_received_sigint()
  {
    return signal_handler().received_sigint();
  }

}
