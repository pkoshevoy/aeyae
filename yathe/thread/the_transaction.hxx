// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

/*
Copyright 2004-2007 University of Utah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


// File         : the_transaction.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Feb 16 09:52:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A thread transaction class.

#ifndef THE_TRANSACTION_HXX_
#define THE_TRANSACTION_HXX_

// system includes:
#include <iostream>
#include <string>

// the includes:
#include <utils/the_text.hxx>
#include <utils/the_log.hxx>

// forward declarations:
class the_thread_interface_t;
class the_transaction_handler_t;


//----------------------------------------------------------------
// the_transaction_t
// 
// NOTE: the transaction will not take ownership of the mutex.
// 
class the_transaction_t
{
public:
  the_transaction_t();
  virtual ~the_transaction_t();
  
  // execute the transaction:
  virtual void execute(the_thread_interface_t * thread) = 0;
  
  // transaction execution state:
  typedef enum {
    PENDING_E,
    SKIPPED_E,
    STARTED_E,
    ABORTED_E,
    DONE_E
  } state_t;
  
  inline state_t state() const
  { return state_; }
  
  inline void set_state(const state_t & s)
  { state_ = s; }
  
  inline bool done() const
  { return state_ == DONE_E; }
  
  //----------------------------------------------------------------
  // notify_cb_t
  // 
  typedef void(*notify_cb_t)(void *, the_transaction_t *, state_t s);
  
  inline notify_cb_t notify_cb() const
  { return notify_cb_; }
  
  inline void set_notify_cb(notify_cb_t cb, void * cb_data)
  {
    notify_cb_ = cb;
    notify_cb_data_ = cb_data;
  }
  
  //----------------------------------------------------------------
  // status_cb_t
  // 
  typedef void(*status_cb_t)(void *, the_transaction_t *, const char *);
  
  inline status_cb_t status_cb() const
  { return status_cb_; }
  
  inline void set_status_cb(status_cb_t cb, void * cb_data)
  {
    status_cb_ = cb;
    status_cb_data_ = cb_data;
  }
  
  // notify the transaction about a change in it's state:
  virtual void notify(the_transaction_handler_t * handler,
		      state_t s,
		      const char * message = NULL);
  
  // helper:
  virtual void blab(the_transaction_handler_t * handler,
		    const char * message);
  
  // FIXME: this is a relic:
  inline static const the_text_t tr(const char * text)
  { return the_text_t(text); }
  
  inline static const the_text_t & tr(const the_text_t & text)
  { return text; }
  
protected:
  // current state of the transaction:
  state_t state_;
  
public:
  // the callbacks:
  notify_cb_t notify_cb_;
  void * notify_cb_data_;
  
  status_cb_t status_cb_;
  void * status_cb_data_;
};

//----------------------------------------------------------------
// operator << 
// 
extern std::ostream &
operator << (std::ostream & so, const the_transaction_t::state_t & state);


//----------------------------------------------------------------
// the_transaction_handler_t
// 
class the_transaction_handler_t
{
public:
  virtual ~the_transaction_handler_t() {}
  
  virtual void handle(the_transaction_t * transaction,
		      the_transaction_t::state_t s) = 0;
  virtual void blab(const char * message) const = 0;
};


//----------------------------------------------------------------
// the_transaction_log_t
// 
class the_transaction_log_t : public the_log_t
{
public:
  the_transaction_log_t(the_transaction_handler_t * handler):
    handler_(handler)
  {}
  
  // virtual:
  the_log_t & operator << (std::ostream & (*f)(std::ostream &))
  {
    the_log_t::operator << (f);
    std::string text(the_log_t::line_.str());
    the_log_t::line_.str("");
    handler_->blab(text.c_str());
    return *this;
  }
  
  template <typename data_t>
  the_log_t & operator << (const data_t & data)
  { return the_log_t::operator << (data); }
  
private:
  the_transaction_handler_t * handler_;
};


#endif // THE_TRANSACTION_HXX_
