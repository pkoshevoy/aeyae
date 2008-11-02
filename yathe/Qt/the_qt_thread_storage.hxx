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


// File         : the_qt_thread_storage.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Jan 2 09:40:00 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt thread storage wrapper class.

#ifndef THE_QT_THREAD_STORAGE_HXX_
#define THE_QT_THREAD_STORAGE_HXX_

// local includes:
#include "thread/the_thread_storage.hxx"

// Qt includes:
#include <QThreadStorage>


//----------------------------------------------------------------
// the_qt_thread_storage_t
//
class the_qt_thread_storage_t :
  public QThreadStorage<the_thread_observer_t *>,
  public the_thread_storage_t
{
public:
  // virtual: check whether the thread storage has been initialized:
  bool is_ready() const
  { return QThreadStorage<the_thread_observer_t *>::localData() != NULL; }
  
  // virtual: check whether the thread has been stopped:
  bool thread_stopped() const
  { return localData()->thread_.stopped(); }
  
  // virtual: terminator access:
  the_terminators_t & terminators()
  { return localData()->thread_.terminators(); }
  
  // virtual:
  unsigned int thread_id() const
  { return localData()->thread_.id(); }
};


#endif // THE_QT_THREAD_STORAGE_HXX_
