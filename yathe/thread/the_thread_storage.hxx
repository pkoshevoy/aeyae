// File         : the_thread_storage.hxx
// Author       : Paul A. Koshevoy
// Created      : Fri Jan 2 09:30:00 MDT 2007
// Copyright    : (C) 2007
// License      : GPL.
// Description  : a terminator thread storage interface class

#ifndef THE_THREAD_STORAGE_HXX_
#define THE_THREAD_STORAGE_HXX_

// forward declarations:
class the_thread_interface_t;
class the_terminators_t;

//----------------------------------------------------------------
// the_thread_observer_t
// 
class the_thread_observer_t
{
public:
  the_thread_observer_t(the_thread_interface_t & thread):
    thread_(thread)
  {}
  
  the_thread_interface_t & thread_;
};

//----------------------------------------------------------------
// the_thread_storage_t
// 
class the_thread_storage_t
{
public:
  virtual ~the_thread_storage_t() {}
  
  // check whether the thread storage has been initialized:
  virtual bool is_ready() const = 0;
  
  // check whether the thread has been stopped:
  virtual bool thread_stopped() const = 0;
  
  // terminator access:
  virtual the_terminators_t & terminators() = 0;
  
  // thread id:
  virtual unsigned int thread_id() const = 0;
};

//----------------------------------------------------------------
// the_thread_storage_provider_t
// 
typedef the_thread_storage_t&(*the_thread_storage_provider_t)();

//----------------------------------------------------------------
// set_the_thread_storage_provider
// 
// Set the new thread storage provider, return the old provider.
// 
extern the_thread_storage_provider_t
set_the_thread_storage_provider(the_thread_storage_provider_t p);

//----------------------------------------------------------------
// the_thread_storage
// 
// Thread storage accessors.
// 
extern the_thread_storage_t & the_thread_storage();


#endif // THE_THREAD_STORAGE_HXX_
