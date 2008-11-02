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
