// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_mutex.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt mutex wrapper class.

// local includes:
#include "Qt3/the_qt_mutex.hxx"

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
// the_qt_mutex_t::the_qt_mutex_t
//
the_qt_mutex_t::the_qt_mutex_t():
  QMutex(),
  the_mutex_interface_t()
{}

//----------------------------------------------------------------
// the_qt_mutex_t::~the_qt_mutex_t
//
the_qt_mutex_t::~the_qt_mutex_t()
{}

//----------------------------------------------------------------
// the_qt_mutex_t::delete_this
//
void
the_qt_mutex_t::delete_this()
{
  delete this;
}

//----------------------------------------------------------------
// the_qt_mutex_t::create
//
the_mutex_interface_t *
the_qt_mutex_t::create()
{
  return new the_qt_mutex_t();
}

//----------------------------------------------------------------
// the_qt_mutex_t::lock
//
void
the_qt_mutex_t::lock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\tlock" << endl;
#endif

  QMutex::lock();
}

//----------------------------------------------------------------
// the_qt_mutex_t::unlock
//
void
the_qt_mutex_t::unlock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\tunlock" << endl;
#endif

  QMutex::unlock();
}

//----------------------------------------------------------------
// the_qt_mutex_t::try_lock
//
bool
the_qt_mutex_t::try_lock()
{
#ifdef DEBUG_MUTEX
  cerr << this << "\ttry_lock" << endl;
#endif

  return QMutex::tryLock();
}
