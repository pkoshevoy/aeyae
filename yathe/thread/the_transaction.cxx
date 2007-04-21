// File         : the_transaction.cxx
// Author       : Paul A. Koshevoy
// Created      : Fri Feb 16 09:52:00 MST 2007
// Copyright    : (C) 2007
// License      : 
// Description  : 

// system includes:
#include <stddef.h>
#include <assert.h>

// local includes:
#include "thread/the_transaction.hxx"
#include "thread/the_thread_interface.hxx"


//----------------------------------------------------------------
// the_transaction_t::the_transaction_t
// 
the_transaction_t::the_transaction_t():
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
{}

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
