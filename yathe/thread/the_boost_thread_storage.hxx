// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_boost_thread_storage.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu Oct 23 21:37:43 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : A Qt wrapper for Boost thread specific storage.

#ifndef THE_BOOST_THREAD_STORAGE_HXX_
#define THE_BOOST_THREAD_STORAGE_HXX_

// local includes:
#include "thread/the_thread_storage.hxx"

// Boost includes:
#include <boost/thread/tss.hpp>


//----------------------------------------------------------------
// the_boost_thread_storage_t
//
class the_boost_thread_storage_t :
  public boost::thread_specific_ptr<the_thread_observer_t>,
  public the_thread_storage_t
{
public:
  // virtual: check whether the thread storage has been initialized:
  bool is_ready() const
  {
    return (boost::thread_specific_ptr<the_thread_observer_t>::get() != NULL);
  }

  // virtual: check whether the thread has been stopped:
  bool thread_stopped() const
  {
    return
      boost::thread_specific_ptr<the_thread_observer_t>::get()->
      thread_.stopped();
  }

  // virtual: terminator access:
  the_terminators_t & terminators()
  {
    return
      boost::thread_specific_ptr<the_thread_observer_t>::get()->
      thread_.terminators();
  }

  // virtual:
  unsigned int thread_id() const
  {
    return
      boost::thread_specific_ptr<the_thread_observer_t>::get()->
      thread_.id();
  }
};


#endif // THE_BOOST_THREAD_STORAGE_HXX_
