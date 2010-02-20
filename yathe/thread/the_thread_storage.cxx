// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_thread_storage.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Jan 9 12:27:00 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A thread storage abstract interface class

// local includes:
#include <thread/the_thread_storage.hxx>
#include <thread/the_terminator.hxx>


//----------------------------------------------------------------
// the_dummy_terminators_t
// 
class the_dummy_terminators_t : public the_terminators_t
{
public:
  // virtual:
  void lock() {}
  void unlock() {}
};

//----------------------------------------------------------------
// the_dummy_thread_storage_t
// 
class the_dummy_thread_storage_t : public the_thread_storage_t
{
public:
  // virtual:
  bool is_ready() const
  { return true; }
  
  bool thread_stopped() const
  { return false; }
  
  the_terminators_t & terminators()
  { return terminators_; }
  
  unsigned int thread_id() const
  { return ~0u; }
  
private:
  the_dummy_terminators_t terminators_;
};

//----------------------------------------------------------------
// the_dummy_thread_storage
// 
static the_thread_storage_t &
the_dummy_thread_storage()
{
  static the_dummy_thread_storage_t thread_storage;
  return thread_storage;
}

//----------------------------------------------------------------
// thread_storage_provider_
// 
static the_thread_storage_provider_t
thread_storage_provider_ = the_dummy_thread_storage;

//----------------------------------------------------------------
// set_the_thread_storage_provider
// 
the_thread_storage_provider_t
set_the_thread_storage_provider(the_thread_storage_provider_t p)
{
  the_thread_storage_provider_t old = thread_storage_provider_;
  thread_storage_provider_ = p;
  return old;
}

//----------------------------------------------------------------
// the_thread_storage
// 
the_thread_storage_t &
the_thread_storage()
{
  return thread_storage_provider_();
}
