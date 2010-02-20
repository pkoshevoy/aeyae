// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_thread.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 17:00:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt thread wrapper class.

// local include:
#include "Qt3/the_qt_thread.hxx"
#include "Qt3/the_qt_thread_storage.hxx"
#include "Qt3/the_qt_mutex.hxx"
#include "thread/the_mutex_interface.hxx"
#include "thread/the_transaction.hxx"
#include "thread/the_thread_pool.hxx"

// system includes:
#include <iostream>

// namespace access:
using std::cout;
using std::cerr;
using std::endl;

//----------------------------------------------------------------
// DEBUG_THREAD
// 
// #define DEBUG_THREAD


//----------------------------------------------------------------
// THREAD_STORAGE
// 
static the_qt_thread_storage_t THREAD_STORAGE;

//----------------------------------------------------------------
// the_qt_thread_t::the_qt_thread_t
// 
the_qt_thread_t::the_qt_thread_t():
  QThread(),
  the_thread_interface_t(the_qt_mutex_t::create())
{
  if (THREAD_STORAGE.localData() == NULL)
  {
    THREAD_STORAGE.setLocalData(new the_thread_observer_t(*this));
  }
}

//----------------------------------------------------------------
// the_qt_thread_t::~the_qt_thread_t
// 
the_qt_thread_t::~the_qt_thread_t()
{}

//----------------------------------------------------------------
// the_qt_thread_t::delete_this
// 
void
the_qt_thread_t::delete_this()
{
  delete this;
}

//----------------------------------------------------------------
// ImageProcessingThread::thread_storage
// 
the_thread_storage_t &
the_qt_thread_t::thread_storage()
{
  return THREAD_STORAGE;
}

//----------------------------------------------------------------
// the_qt_thread_t::start
// 
void
the_qt_thread_t::start()
{
  the_lock_t<the_mutex_interface_t> locker(mutex_);
#ifdef DEBUG_THREAD
  cerr << "start of thread " << this << " requested" << endl;
#endif
  
  if (QThread::running())
  {
    if (!stopped_)
    {
      // already running:
#ifdef DEBUG_THREAD
      cerr << "thread " << this << " is already running" << endl;
#endif
      return;
    }
    else
    {
      // wait for the shutdown to succeed, then restart the thread:
#ifdef DEBUG_THREAD
      cerr << "waiting for thread " << this << " to shut down" << endl;
#endif
      wait();
    }
  }
  
#ifdef DEBUG_THREAD
  cerr << "starting thread " << this << endl;
#endif
  
  // clear the termination flag:
  stopped_ = false;
  QThread::start();
}

//----------------------------------------------------------------
// the_qt_thread_t::wait
// 
void
the_qt_thread_t::wait()
{
  QThread::wait();
}

//----------------------------------------------------------------
// the_qt_thread_t::take_a_nap
// 
void
the_qt_thread_t::take_a_nap(const unsigned long & microseconds)
{
  QThread::usleep(microseconds);
}

//----------------------------------------------------------------
// the_qt_thread_t::terminators
// 
the_terminators_t &
the_qt_thread_t::terminators()
{
  return terminators_;
}

//----------------------------------------------------------------
// the_qt_thread_t::run
// 
void
the_qt_thread_t::run()
{
  // setup the thread storage:
  {
    the_lock_t<the_mutex_interface_t> locker(mutex_);
    THREAD_STORAGE.setLocalData(new the_thread_observer_t(*this));
  }
  
  // process the transactions:
  work();
  
  // clean up the thread storage:
  THREAD_STORAGE.setLocalData(NULL);
}
