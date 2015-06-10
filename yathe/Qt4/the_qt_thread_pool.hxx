// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_thread_pool.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Feb 21 11:27:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt based thread pool class.

#ifndef THE_QT_THREAD_POOL_HXX_
#define THE_QT_THREAD_POOL_HXX_

// Qt includes:
#include <QObject>

// system includes:
#include <list>
#include <assert.h>

// local includes:
#include "thread/the_thread_pool.hxx"


//----------------------------------------------------------------
// the_qt_thread_pool_t
//
class the_qt_thread_pool_t : public QObject,
			     public the_thread_pool_t
{
  Q_OBJECT

public:
  the_qt_thread_pool_t(unsigned int num_threads);

  // virtual:
  void handle(the_transaction_t * transaction, the_transaction_t::state_t s);
  void blab(const char * message) const;

signals:
  void status_update(const QString & message) const;
  void transaction_started(the_transaction_t * transaction);
  void transaction_finished(the_transaction_t * transaction);
  void thread_stopped(the_thread_interface_t * thread,
		      bool all_transactions_completed);
};


#endif // THE_QT_THREAD_POOL_HXX_
