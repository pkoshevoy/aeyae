// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_thread_pool.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Feb 21 11:29:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A Qt based thread pool class.

// local includes:
#include "Qt4/the_qt_thread_pool.hxx"

//----------------------------------------------------------------
// the_qt_thread_pool_t::the_qt_thread_pool_t
//
the_qt_thread_pool_t::the_qt_thread_pool_t(unsigned int num_threads):
  QObject(),
  the_thread_pool_t(num_threads)
{}

//----------------------------------------------------------------
// the_qt_thread_pool_t::handle
//
void
the_qt_thread_pool_t::handle(the_transaction_t * transaction,
			     the_transaction_t::state_t s)
{
  switch (s)
  {
    case the_transaction_t::STARTED_E:
      emit transaction_started(transaction);
      break;

    case the_transaction_t::SKIPPED_E:
    case the_transaction_t::ABORTED_E:
    case the_transaction_t::DONE_E:
      emit transaction_finished(transaction);
      break;

    default:
      break;
  }
}

//----------------------------------------------------------------
// the_qt_thread_pool_t::blab
//
void
the_qt_thread_pool_t::blab(const char * message) const
{
  emit status_update(QString(message));
}
