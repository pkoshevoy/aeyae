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
  struct IMessageCarrier;

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
    //----------------------------------------------------------------
    // Message
    //
    struct YAE_API Message
    {
      Message(int priority = 0,
              const char * source = "",
              const char * text = "");

      bool operator == (const Message & msg) const;

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
  // to_str
  //
  YAE_API const char *
  to_str(TLog::TPriority p);

  //----------------------------------------------------------------
  // logger
  //
  YAE_API TLog & logger();

  //----------------------------------------------------------------
  // log
  //
  YAE_API void
  log(int priority, const char * source, const char * format, ...);

}


//----------------------------------------------------------------
// yae_error
//
#define yae_error yae::logger().error(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_warn
//
#define yae_warn yae::logger().warn(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_info
//
#define yae_info yae::logger().info(__FILE__ ":" YAE_STR(__LINE__))

//----------------------------------------------------------------
// yae_debug
//
#define yae_debug yae::logger().debug(__FILE__ ":" YAE_STR(__LINE__))


//----------------------------------------------------------------
// yae_elog
//
#define yae_elog(format, ...)                                           \
  yae::log(yae::TLog::kError,                                           \
           __FILE__ ":" YAE_STR(__LINE__),                              \
           (format),                                                    \
           ##__VA_ARGS__ )

//----------------------------------------------------------------
// yae_wlog
//
#define yae_wlog(format, ...)                                           \
  yae::log(yae::TLog::kWarning,                                         \
           __FILE__ ":" YAE_STR(__LINE__),                              \
           (format),                                                    \
           ##__VA_ARGS__ )

//----------------------------------------------------------------
// yae_ilog
//
#define yae_ilog(format, ...)                                           \
  yae::log(yae::TLog::kInfo,                                            \
           __FILE__ ":" YAE_STR(__LINE__),                              \
           (format),                                                    \
           ##__VA_ARGS__ )

//----------------------------------------------------------------
// yae_dlog
//
#define yae_dlog(format, ...)                                           \
  yae::log(yae::TLog::kDebug,                                           \
           __FILE__ ":" YAE_STR(__LINE__),                              \
           (format),                                                    \
           ##__VA_ARGS__ )


#endif // YAE_LOG_H_
