// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_thread.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Feb 18 17:00:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt thread wrapper class.

#ifndef THE_QT_THREAD_HXX_
#define THE_QT_THREAD_HXX_

// Qt includes:
#include <QThread>
#include <QObject>
#include <QMutex>

// system includes:
#include <list>

// local includes:
#include "thread/the_terminator.hxx"
#include "thread/the_thread_interface.hxx"

// forward declarations:
class the_mutex_interface_t;
class the_thread_storage_t;
class ImageProcessingThreadObserver;


//----------------------------------------------------------------
// the_qt_terminators_t
// 
class the_qt_terminators_t : public the_terminators_t
{
public:
  // virtual: concurrent access controls:
  void lock()	{ mutex_.lock(); }
  void unlock()	{ mutex_.unlock(); }
  
private:
  mutable QMutex mutex_;
};


//----------------------------------------------------------------
// the_qt_thread_t
// 
// 1. the thread will not take ownership of the transactions.
// 2. the thread will take ownership of the mutex.
// 
class the_qt_thread_t : public QThread,
			public the_thread_interface_t
{
  Q_OBJECT

public:
  the_qt_thread_t();
  
  // the destructor is protected on purpose,
  // see delete_this for details:
  virtual ~the_qt_thread_t();
  
  // In order to avoid memory management problems with shared libraries,
  // whoever provides this interface instance (via it's creator), has to
  // provide a way to delete the instance as well.  This will avoid
  // issues with multiple-instances of C runtime libraries being
  // used by the app and whatever libraries it links against that
  // either use or provide this interface:
  virtual void delete_this();
  
  // the creation method:
  static the_thread_interface_t * create()
  { return new the_qt_thread_t(); }
  
  // the thread storage accessor:
  static the_thread_storage_t & thread_storage();
  
  // virtual: start the thread:
  void start();
  
  // virtual:
  void wait();
  
  // virtual: put the thread to sleep:
  void take_a_nap(const unsigned long & microseconds);
  
  // virtual: accessor to the transaction terminators:
  the_terminators_t & terminators();
  
  // virtual:
  void handle(the_transaction_t * transaction, the_transaction_t::state_t s);
  void blab(const char * message) const;
  
signals:
  void status_update(const QString & message) const;
  void transaction_started(the_transaction_t * transaction);
  void transaction_finished(the_transaction_t * transaction);
  void thread_stopped(the_thread_interface_t * thread,
		      bool all_transactions_completed);
  
protected:
  // virtual:
  void run();
  
  // a list of active terminators for this thread:
  the_qt_terminators_t terminators_;
};


#endif // THE_QT_THREAD_HXX_
