// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 19:50:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <stdarg.h>

// aeyae:
#include "../api/yae_log.h"
#include "../api/yae_message_carrier_interface.h"
#include "../utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // TLog::TLog
  //
  TLog::TLog(const std::string & carrierId,
             IMessageCarrier * carrier)
  {
    assign(carrierId, carrier);
  }

  //----------------------------------------------------------------
  // TLog::~TLog
  //
  TLog::~TLog()
  {
    clear();
  }

  //----------------------------------------------------------------
  // TLog::clear
  //
  void
  TLog::clear()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    for (std::map<std::string, IMessageCarrier *>::iterator
           i = carriers_.begin(); i != carriers_.end(); ++i)
    {
      IMessageCarrier *& carrier = i->second;
      if (carrier)
      {
        carrier->destroy();
        carrier = NULL;
      }
    }

    carriers_.clear();
  }

  //----------------------------------------------------------------
  // TLog::assign
  //
  // add or update the carrier associated with a given carrierId:
  //
  void
  TLog::assign(const std::string & carrierId,
               IMessageCarrier * carrier)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    IMessageCarrier * prevCarrier = carriers_[carrierId];

    if (carrier != prevCarrier)
    {
      if (prevCarrier)
      {
        prevCarrier->destroy();
      }

      carriers_[carrierId] = carrier;
    }
  }

  //----------------------------------------------------------------
  // TLog::remove
  //
  // dispose of a carrier associated with a given carrierId:
  //
  void
  TLog::remove(const std::string & carrierId)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    std::map<std::string, IMessageCarrier *>::iterator
      found = carriers_.find(carrierId);
    if (found == carriers_.end())
    {
      return;
    }

    IMessageCarrier * carrier = found->second;
    if (carrier)
    {
      carrier->destroy();
    }

    carriers_.erase(found);
  }

  //----------------------------------------------------------------
  // TLog::deliver
  //
  //! broadcast a given message to every carrier
  //! registered with this log instance:
  //
  void
  TLog::deliver(int messagePriority,
                const char * source,
                const char * message)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    for (std::map<std::string, IMessageCarrier *>::iterator
           i = carriers_.begin(); i != carriers_.end(); ++i)
    {
      IMessageCarrier * carrier = i->second;

      if (carrier && messagePriority >= carrier->priorityThreshold())
      {
        carrier->deliver(messagePriority, source, message);
      }
    }

#if 0 // ndef NDEBUG
    YAE_BREAKPOINT_IF(messagePriority == kError);
#endif
  }

  //----------------------------------------------------------------
  // TLog::Scribe::Private::Private
  //
  TLog::Scribe::Private::Private(TLog & logger,
                                 int priority,
                                 const char * source):
    logger_(logger),
    priority_(priority),
    source_(source)
  {}

  //----------------------------------------------------------------
  // TLog::Scribe::Private::~Private
  //
  TLog::Scribe::Private::~Private()
  {
    logger_.deliver(priority_, source_, oss_.str().c_str());
  }

  //----------------------------------------------------------------
  // log
  //
  void
  log(int priority, const char * source, const char * format, ...)
  {
    va_list args;
    va_start(args, format);
    std::string message = vstrfmt(format, args);
    va_end(args);

    yae::logger().deliver(priority, source, message.c_str());
  }
}
