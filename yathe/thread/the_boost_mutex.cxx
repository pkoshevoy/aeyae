// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_boost_mutex.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Thu Oct 23 21:52:20 MDT 2008
// Copyright    : (C) 2008
// License      : MIT
// Description  : A Boost mutex wrapper class.

// local includes:
#include "thread/the_boost_mutex.hxx"

// system includes:
#include <iostream>

// namespace access:
using std::cerr;
using std::endl;

//----------------------------------------------------------------
// define
//
// #define DEBUG_MUTEX


//----------------------------------------------------------------
// the_boost_mutex_t::the_boost_mutex_t
//
the_boost_mutex_t::the_boost_mutex_t():
  the_mutex_interface_t()
{}

//----------------------------------------------------------------
// the_boost_mutex_t::~the_boost_mutex_t
//
the_boost_mutex_t::~the_boost_mutex_t()
{}

//----------------------------------------------------------------
// the_boost_mutex_t::delete_this
//
void
the_boost_mutex_t::delete_this()
{
  delete this;
}

//----------------------------------------------------------------
// the_boost_mutex_t::create
//
the_mutex_interface_t *
the_boost_mutex_t::create()
{
  return new the_boost_mutex_t();
}

//----------------------------------------------------------------
// the_boost_mutex_t::lock
//
void
the_boost_mutex_t::lock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\tlock" << endl;
#endif

  mutex_.lock();
}

//----------------------------------------------------------------
// the_boost_mutex_t::unlock
//
void
the_boost_mutex_t::unlock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\tunlock" << endl;
#endif

  mutex_.unlock();
}

//----------------------------------------------------------------
// the_boost_mutex_t::try_lock
//
bool
the_boost_mutex_t::try_lock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\ttry_lock" << endl;
#endif

  return mutex_.try_lock();
}
