// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon May 25 18:19:22 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MESSAGE_CARRIER_INTERFACE_H_
#define YAE_MESSAGE_CARRIER_INTERFACE_H_

// aeyae:
#include "yae_api.h"
#include "yae_plugin_interface.h"


namespace yae
{

  struct YAE_API IMessageCarrier : public IPlugin
  {

    //----------------------------------------------------------------
    // TPriority
    //
    //! message priority levels, listed in descending priority order
    //
    enum TPriority
    {
      kDebug   = 0,
      kInfo    = 1,
      kWarning = 2,
      kError   = 3
    };

    //! accessor to current priority threshold for this message carrier:
    virtual TPriority priorityThreshold() const = 0;

    //! set the priority threshold -- messages with priority
    //! that is greater or equal to the threshold should be
    //! accepted for delivery, other messages may be ignored:
    virtual void setPriorityThreshold(TPriority priority) = 0;

    //! message delivery interface:
    virtual void deliver(TPriority messagePriority,
                         const char * source,
                         const char * message) = 0;
  };

}


#endif // YAE_MESSAGE_CARRIER_INTERFACE_H_
