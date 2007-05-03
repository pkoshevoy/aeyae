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


// File         : the_qt_mutex.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 16:12:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt mutex wrapper class.

// local includes:
#include "Qt/the_qt_mutex.hxx"

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
