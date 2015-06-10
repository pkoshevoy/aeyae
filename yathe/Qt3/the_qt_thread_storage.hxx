// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
#include <qthreadstorage.h>


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
