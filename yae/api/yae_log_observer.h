// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Apr 25 12:45:40 PM MDT 2026
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LOG_OBSERVER_H_
#define YAE_LOG_OBSERVER_H_

// aeyae:
#include "yae_log.h"
#include "yae_message_carrier_interface.h"

// standard:
#include <list>
#include <string>


namespace yae
{

  //----------------------------------------------------------------
  // LogObserver
  //
  struct LogObserver : yae::IMessageCarrier
  {
    struct Message
    {
      Message(int priority = -1,
              const char * source = "",
              const char * message = ""):
        priority_(priority),
        source_(source),
        message_(message)
      {}

      int priority_;
      std::string source_;
      std::string message_;
    };

    // LogObserver doesn't own the log_, it just adds to it:
    std::list<Message> & log_;
    int priority_;

    LogObserver(std::list<Message> & log, int priority = yae::TLog::kError):
      log_(log),
      priority_(priority)
    {}

    // yae::IPlugin inerface:
    void destroy()
    { delete this; }

    LogObserver * clone() const
    { return new LogObserver(log_, priority_); }

    const char * name() const
    { return "LogObserver"; }

    const char * guid() const
    { return "acb048ae-500d-428a-ae56-654719d79180"; }

    yae::ISettingGroup * settings()
    { return NULL; }

    // yae::IMessageCarrier interface:
    int priorityThreshold() const
    { return priority_; }

    void setPriorityThreshold(int priority)
    { priority_ = (yae::TLog::TPriority)priority; }

    void deliver(int priority,
                 const char * source,
                 const char * message)
    { log_.push_back(Message(priority, source, message)); }
  };

}


#endif // YAE_LOG_OBSERVER_H_
