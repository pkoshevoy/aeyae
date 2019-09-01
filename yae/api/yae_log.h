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

// standard C++ library:
#include <map>
#include <string>

// boost library:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#endif


namespace yae
{
  // forward declarations:
  struct YAE_API IMessageCarrier;

  //----------------------------------------------------------------
  // TLog
  //
  // A thread-safe message logger supporting message delivery
  // via multiple carriers.
  //
  struct YAE_API TLog
  {
    enum TPriority
    {
      kDebug   = 0,
      kInfo    = 1,
      kWarning = 2,
      kError   = 3
    };


    TLog(const std::string & carrierId = std::string(),
         IMessageCarrier * carrier = NULL);

    ~TLog();

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
    void clear();

    // add or update the carrier associated with a given carrierId:
    void assign(const std::string & carrierId,
                IMessageCarrier * carrier);

    // dispose of a carrier associated with a given carrierId:
    void remove(const std::string & carrierId);

    //! broadcast a given message to every carrier
    //! registered with this log instance:
    void deliver(int messagePriority,
                 const char * source,
                 const char * message);

    inline void error(const char * source, const char * message)
    { deliver(kError, source, message); }

    inline void warn(const char * source, const char * message)
    { deliver(kWarning, source, message); }

    inline void info(const char * source, const char * message)
    { deliver(kInfo, source, message); }

    inline void debug(const char * source, const char * message)
    { deliver(kDebug, source, message); }

    //----------------------------------------------------------------
    // Scribe
    //
    struct YAE_API Scribe
    {
      //----------------------------------------------------------------
      // Private
      //
      struct YAE_API Private
      {
        Private(TLog & logger, int priority, const char * source);
        ~Private();

        TLog & logger_;
        int priority_;
        const char * source_;
        mutable std::ostringstream oss_;
      };

      Scribe(TLog & logger, int priority, const char * source):
        private_(new Private(logger, priority, source))
      {}

      template <typename TData>
      Scribe
      operator << (const TData & data) const
      {
        private_->oss_ << data;
        return Scribe(*this);
      }

    protected:
      boost::shared_ptr<Private> private_;
    };

    inline Scribe error(const char * source)
    { return Scribe(*this, kError, source); }

    inline Scribe warn(const char * source)
    { return Scribe(*this, kWarning, source); }

    inline Scribe info(const char * source)
    { return Scribe(*this, kInfo, source); }

    inline Scribe debug(const char * source)
    { return Scribe(*this, kDebug, source); }

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
#define yae_elog yae::logger().error(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_wlog
//
#define yae_wlog yae::logger().warn(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_log
//
#define yae_log yae::logger().info(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_dlog
//
#define yae_dlog yae::logger().debug(__FILE__ ":" YAE_STR(__LINE__))


#endif // YAE_LOG_H_
