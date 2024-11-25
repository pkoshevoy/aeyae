// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SIGNAL_HANDLER_H_
#define YAE_SIGNAL_HANDLER_H_

// aeyae:
#include "yae/api/yae_api.h"

// standard:
#include <set>
#include <utility>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS


namespace yae
{

  //----------------------------------------------------------------
  // SignalHandler
  //
  struct SignalHandler
  {
    SignalHandler();

    void handle(int sig);
    bool acknowledge(int sig);

    bool received_siginfo();
    bool received_sigpipe();
    bool received_sigint();

    typedef void(*TFuncPtr)(void *, int);
    void add(TFuncPtr cb, void * ctx);

  protected:
    void call_callbacks(int sig);

    boost::condition_variable signal_;
    mutable boost::mutex mutex_;
    std::set<int> received_;
    std::set<std::pair<TFuncPtr, void *> > callback_;
  };

  //----------------------------------------------------------------
  // signal_handler
  //
  SignalHandler & signal_handler();

  //----------------------------------------------------------------
  // signal_handler_received_siginfo
  //
  bool signal_handler_received_siginfo();

  //----------------------------------------------------------------
  // signal_handler_received_sigpipe
  //
  bool signal_handler_received_sigpipe();

  //----------------------------------------------------------------
  // signal_handler_received_sigint
  //
  bool signal_handler_received_sigint();

}


#endif // YAE_SIGNAL_HANDLER_H_
