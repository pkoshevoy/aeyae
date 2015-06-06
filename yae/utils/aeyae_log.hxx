// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 19:50:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef AEYAE_LOG_HXX_
#define AEYAE_LOG_HXX_

// aeyae:
#include "../api/aeyae_api.hxx"
#include "../api/aeyae_message_carrier_interface.hxx"

// standard C++ library:
#include <map>
#include <mutex>
#include <string>


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

    TLog(TLog &&) = delete;
    TLog(const TLog &) = delete;

    TLog & operator = (TLog &&) = delete;
    TLog & operator = (const TLog &) = delete;

    // dispose of all carriers assiciated with this log instance:
    inline void clear()
    {
      std::lock_guard<std::mutex> lock(mutex_);

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
      std::lock_guard<std::mutex> lock(mutex_);

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
      std::lock_guard<std::mutex> lock(mutex_);

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
      std::lock_guard<std::mutex> lock(mutex_);

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

  protected:
    mutable std::mutex mutex_;
    std::map<std::string, IMessageCarrier *> carriers_;
  };

}


#endif // AEYAE_LOG_HXX_
