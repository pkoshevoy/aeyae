// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_transaction.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Feb 16 09:52:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A thread transaction class.

// system includes:
#include <stddef.h>
#include <assert.h>

// local includes:
#include "thread/the_transaction.hxx"
#include "thread/the_thread_interface.hxx"
#include "thread/the_mutex_interface.hxx"


//----------------------------------------------------------------
// the_transaction_t::the_transaction_t
// 
the_transaction_t::the_transaction_t():
  mutex_(the_mutex_interface_t::create()),
  request_(NOTHING_E),
  state_(PENDING_E),
  notify_cb_(NULL),
  notify_cb_data_(NULL),
  status_cb_(NULL),
  status_cb_data_(NULL)
{}

//----------------------------------------------------------------
// the_transaction_t::~the_transaction_t
// 
the_transaction_t::~the_transaction_t()
{
  if (mutex_ != NULL)
  {
    mutex_->delete_this();
    mutex_ = NULL;
  }
}

//----------------------------------------------------------------
// the_transaction_t::notify
// 
void
the_transaction_t::notify(the_transaction_handler_t * handler,
			  state_t s,
			  const char * message)
{
  set_state(s);
  blab(handler, message);
  
  if (notify_cb_ == NULL)
  {
    handler->handle(this, s);
  }
  else
  {
    notify_cb_(notify_cb_data_, this, s);
  }
}

//----------------------------------------------------------------
// the_transaction_t::blab
// 
void
the_transaction_t::blab(the_transaction_handler_t * handler,
			const char * message)
{
  if (message == NULL) return;
  
  if (status_cb_ == NULL)
  {
    handler->blab(message);
  }
  else
  {
    status_cb_(status_cb_data_, this, message);
  }
}

//----------------------------------------------------------------
// the_transaction_t::callback_request
// 
bool
the_transaction_t::callback_request()
{
  if (status_cb_ == NULL)
  {
    return false;
  }
  
  // change the request state:
  {
    the_lock_t<the_mutex_interface_t> locker(mutex_);
    request_ = WAITING_E;
  }
  
  // execute the status callback:
  status_cb_(status_cb_data_, this, NULL);
  
  while (true)
  {
    sleep_msec(100);
    
    // check the request state:
    the_lock_t<the_mutex_interface_t> locker(mutex_);
    if (request_ == NOTHING_E)
    {
      break;
    }
  }
  
  return true;
}

//----------------------------------------------------------------
// the_transaction_t::callback
// 
void
the_transaction_t::callback()
{
  // remove the callback request:
  the_lock_t<the_mutex_interface_t> locker(mutex_);
  request_ = NOTHING_E;
}

//----------------------------------------------------------------
// operator <<
// 
std::ostream &
operator << (std::ostream & so, const the_transaction_t::state_t & state)
{
  switch (state)
  {
    case the_transaction_t::PENDING_E:
      so << "pending";
      return so;
      
    case the_transaction_t::SKIPPED_E:
      so << "skipped";
      return so;
      
    case the_transaction_t::STARTED_E:
      so << "started";
      return so;
      
    case the_transaction_t::ABORTED_E:
      so << "aborted";
      return so;
      
    case the_transaction_t::DONE_E:
      so << "done";
      return so;
      
    default:
      so << int(state);
      assert(0);
      return so;
  }
}
