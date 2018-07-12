// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 19:50:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_LOG_H_
#define YAE_LOG_H_

// aeyae:
#include "../api/yae_api.h"
#include "../api/yae_message_carrier_interface.h"

// standard C++ library:
#include <map>
#include <string>

// boost library:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif


namespace yae
{

  //----------------------------------------------------------------
  // TLog
  //
  // A thread-safe message logger supporting message delivery
  // via multiple carriers.
  //
  struct YAE_API TLog
  {
    TLog(const std::string & carrierId = std::string(),
         IMessageCarrier * carrier = NULL)
    {
      assign(carrierId, carrier);
    }

    ~TLog()
    {
      clear();
    }

#if __cplusplus < 201103L
  private:
    TLog(const TLog &);
    TLog & operator = (const TLog &);
  public:
#else
    TLog(TLog &&) = delete;
    TLog(const TLog &) = delete;
    TLog & operator = (TLog &&) = delete;
    TLog & operator = (const TLog &) = delete;
#endif

    // dispose of all carriers associated with this log instance:
    inline void clear()
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

    // add or update the carrier associated with a given carrierId:
    inline void assign(const std::string & carrierId,
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

    // dispose of a carrier associated with a given carrierId:
    inline void remove(const std::string & carrierId)
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

    //! broadcast a given message to every carrier
    //! registered with this log instance:
    inline void deliver(IMessageCarrier::TPriority messagePriority,
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
    }

    inline void error(const char * source, const char * message)
    { deliver(IMessageCarrier::kError, source, message); }

    inline void warn(const char * source, const char * message)
    { deliver(IMessageCarrier::kWarning, source, message); }

    inline void info(const char * source, const char * message)
    { deliver(IMessageCarrier::kInfo, source, message); }

    inline void debug(const char * source, const char * message)
    { deliver(IMessageCarrier::kDebug, source, message); }

    //----------------------------------------------------------------
    // Scribe
    //
    struct Scribe
    {
      struct Private
      {
        Private(TLog & logger,
                IMessageCarrier::TPriority priority,
                const char * source):
          logger_(logger),
          priority_(priority),
          source_(source)
        {}

        ~Private()
        {
          logger_.deliver(priority_, source_, oss_.str().c_str());
        }

        TLog & logger_;
        IMessageCarrier::TPriority priority_;
        const char * source_;
        mutable std::ostringstream oss_;
      };

      Scribe(TLog & logger,
             IMessageCarrier::TPriority priority,
             const char * source):
        private_(new Private(logger, priority, source))
      {}

      template <typename TData>
      Scribe
      operator << (const TData & data) const
      {
        private_->oss_ << data;
        return Scribe(*this);
      };

    protected:
      yae::shared_ptr<Private> private_;
    };

    inline Scribe error(const char * source)
    { return Scribe(*this, IMessageCarrier::kError, source); }

    inline Scribe warn(const char * source)
    { return Scribe(*this, IMessageCarrier::kWarning, source); }

    inline Scribe info(const char * source)
    { return Scribe(*this, IMessageCarrier::kInfo, source); }

    inline Scribe debug(const char * source)
    { return Scribe(*this, IMessageCarrier::kDebug, source); }

  protected:
    mutable boost::mutex mutex_;
    std::map<std::string, IMessageCarrier *> carriers_;
  };

  //----------------------------------------------------------------
  // logger
  //
  YAE_API TLog & logger();

}

//----------------------------------------------------------------
// yae_elog
//
#define yae_elog logger().error(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_wlog
//
#define yae_wlog logger().warn(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_log
//
#define yae_log logger().info(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_dlog
//
#define yae_dlog logger().debug(__FILE__ ":" YAE_STR(__LINE__))


#endif // YAE_LOG_H_
