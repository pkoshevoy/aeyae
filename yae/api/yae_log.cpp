// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 19:50:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>

// boost library:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif

// aeyae:
#include "../api/yae_log.h"
#include "../api/yae_message_carrier_interface.h"
#include "../utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // TLog::Private
  //
  struct TLog::Private
  {
    Private():
      message_repeated_(0)
    {}

    ~Private()
    {
      clear();
    }

    void clear();
    void assign(const std::string & carrierId,
                IMessageCarrier * carrier);

    // dispose of a carrier associated with a given carrierId:
    void remove(const std::string & carrierId);

    //! broadcast a given message to every carrier
    //! registered with this log instance:
    void deliver(int messagePriority,
                 const char * source,
                 const char * message);

    //----------------------------------------------------------------
    // Message
    //
    struct Message
    {
      Message(int priority = 0,
              const char * source = "",
              const char * text = ""):
        priority_(priority),
        source_(source),
        message_(text)
      {}

      inline bool operator == (const Message & msg) const
      {
        return (msg.priority_ == priority_ &&
                msg.source_ == source_ &&
                msg.message_ == message_);
      }

      int priority_;
      std::string source_;
      std::string message_;
    };

    mutable boost::mutex mutex_;
    std::map<std::string, IMessageCarrier *> carriers_;
    Message last_message_;
    uint64_t message_repeated_;
  };


  //----------------------------------------------------------------
  // TLog::TLog
  //
  TLog::TLog(const std::string & carrierId,
             IMessageCarrier * carrier):
    private_(new TLog::Private())
  {
    assign(carrierId, carrier);
  }

  //----------------------------------------------------------------
  // TLog::~TLog
  //
  TLog::~TLog()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // TLog::Private::clear
  //
  void
  TLog::Private::clear()
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
    last_message_ = Message();
    message_repeated_ = 0;
  }

  //----------------------------------------------------------------
  // TLog::clear
  //
  void
  TLog::clear()
  {
    private_->clear();
  }

  //----------------------------------------------------------------
  // TLog::Private::assign
  //
  // add or update the carrier associated with a given carrierId:
  //
  void
  TLog::Private::assign(const std::string & carrierId,
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
  // TLog::assign
  //
  void
  TLog::assign(const std::string & carrierId,
               IMessageCarrier * carrier)
  {
    private_->assign(carrierId, carrier);
  }

  //----------------------------------------------------------------
  // TLog::Private::remove
  //
  // dispose of a carrier associated with a given carrierId:
  //
  void
  TLog::Private::remove(const std::string & carrierId)
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
  // TLog::remove
  //
  void
  TLog::remove(const std::string & carrierId)
  {
    private_->remove(carrierId);
  }

  //----------------------------------------------------------------
  // TLog::Private::deliver
  //
  //! broadcast a given message to every carrier
  //! registered with this log instance:
  //
  void
  TLog::Private::deliver(int messagePriority,
                         const char * source,
                         const char * message)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);

    Message new_message(messagePriority, source, message);
    if (last_message_ == new_message)
    {
      message_repeated_++;
      return;
    }

    if (message_repeated_)
    {
      std::string msg = strfmt("%s (message repeated %" PRIu64 " times)",
                               last_message_.message_.c_str(),
                               message_repeated_);
      message_repeated_ = 0;

      for (std::map<std::string, IMessageCarrier *>::iterator
           i = carriers_.begin(); i != carriers_.end(); ++i)
      {
        IMessageCarrier * carrier = i->second;

        if (carrier && messagePriority >= carrier->priorityThreshold())
        {
          carrier->deliver(last_message_.priority_,
                           last_message_.source_.c_str(),
                           msg.c_str());
        }
      }
    }

    last_message_ = new_message;

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
  // TLog::deliver
  //
  void
  TLog::deliver(int messagePriority,
                const char * source,
                const char * message)
  {
    private_->deliver(messagePriority, source, message);
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
  // to_str
  //
  const char *
  to_str(TLog::TPriority p)
  {
    switch (p)
    {
      case TLog::kDebug:
        return "DEBUG";

      case TLog::kInfo:
        return "INFO";

      case TLog::kWarning:
        return "WARNING";

      case TLog::kError:
        return "ERROR";

      default:
        break;
    }

    YAE_ASSERT(false);
    return "FIXME";
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
