// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : DebugMutex.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A chatty mutex class, may be useful when debugging.

#ifndef DEBUG_MUTEX_H_
#define DEBUG_MUTEX_H_

// Qt includes:
#include <QMutex>


//----------------------------------------------------------------
// DebugMutex
// 
class DebugMutex : public QMutex
{
public:
  inline void lock()
  {
    std::cerr << "mutex " << this << " -> lock" << std::endl;
    QMutex::lock();
  }
  
  bool tryLock()
  {
    bool ok = QMutex::tryLock();
    std::cerr << "mutex " << this << " -> tryLock: " << ok << std::endl;
    return ok;
  }
  
  void unlock()
  {
    std::cerr << "mutex " << this << " -> unlock" << std::endl;
    QMutex::unlock();
  }
};


#endif // DEBUG_MUTEX_H_
