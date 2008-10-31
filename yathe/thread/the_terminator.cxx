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


// File         : the_terminator.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 18:08:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a thread terminator convenience class


// local includes:
#include "thread/the_terminator.hxx"
#include "utils/the_exception.hxx"

// system includes:
#include <exception>
#include <sstream>
#include <iostream>
#include <assert.h>

// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// DEBUG_TERMINATORS
// 
// #define DEBUG_TERMINATORS


//----------------------------------------------------------------
// the_terminator_t::the_terminator_t
// 
the_terminator_t::the_terminator_t(const char * id):
  id_(id),
  termination_requested_(should_terminate_immediately())
{
  terminate_on_request();
  add(this);
}

//----------------------------------------------------------------
// the_terminator_t::
// 
the_terminator_t::~the_terminator_t()
{
  del(this);
}

//----------------------------------------------------------------
// the_terminator_t::terminate
// 
void
the_terminator_t::terminate()
{
#ifdef DEBUG_TERMINATORS
  cerr << "TERMINATE: " << id_ << ", this: " << this << endl;
#endif // DEBUG_TERMINATORS
  termination_requested_ = true;
}

//----------------------------------------------------------------
// the_terminator_t::throw_exception
// 
void
the_terminator_t::throw_exception() const
{
  // abort, termination requested:
  std::ostringstream os;
  os << id_ << " interrupted" << endl;
  
  the_exception_t e(os.str().c_str());
  throw e;
}

//----------------------------------------------------------------
// the_terminator_t::verify_termination
// 
bool
the_terminator_t::verify_termination()
{
  return the_thread_storage().terminators().verify_termination();
}

//----------------------------------------------------------------
// the_terminator_t::should_terminate_immediately
// 
bool
the_terminator_t::should_terminate_immediately()
{
  return the_thread_storage().thread_stopped();
}

//----------------------------------------------------------------
// the_terminator_t::add
// 
void
the_terminator_t::add(the_terminator_t * terminator)
{
  the_thread_storage().terminators().add(terminator);
}

//----------------------------------------------------------------
// the_terminator_t::del
// 
void
the_terminator_t::del(the_terminator_t * terminator)
{
  the_thread_storage().terminators().del(terminator);
}


//----------------------------------------------------------------
// the_terminators_t::~the_terminators_t
// 
the_terminators_t::~the_terminators_t()
{
  for (std::list<the_terminator_t *>::iterator i = terminators_.begin();
       i != terminators_.end(); ++i)
  {
    the_terminator_t * t = *i;
    delete t;
  }
    
  terminators_.clear();
}

//----------------------------------------------------------------
// the_terminators_t::terminate
// 
void
the_terminators_t::terminate()
{
  the_lock_t<the_terminators_t> lock(*this);
  
#ifdef DEBUG_TERMINATORS
  cerr << endl << &terminators_ << ": terminate_all -- begin" << endl;
#endif // DEBUG_TERMINATORS
  
  for (std::list<the_terminator_t *>::iterator i = terminators_.begin();
       i != terminators_.end(); ++i)
  {
    the_terminator_t * t = *i;
    t->terminate();
  }
  
#ifdef DEBUG_TERMINATORS
  cerr << &terminators_ << ": terminate_all -- end" << endl;
#endif // DEBUG_TERMINATORS
}

//----------------------------------------------------------------
// the_terminators_t::verify_termination
// 
bool
the_terminators_t::verify_termination()
{
  the_lock_t<the_terminators_t> lock(*this);
  
#ifdef DEBUG_TERMINATORS
  for (std::list<the_terminator_t *>::iterator iter = terminators_.begin();
       iter != terminators_.end(); ++iter)
  {
    the_terminator_t * t = *iter;
    cerr << "ERROR: remaining terminators: " << t->id() << endl;
  }
#endif
  
  return terminators_.empty();
}

//----------------------------------------------------------------
// the_terminators_t::add
// 
void
the_terminators_t::add(the_terminator_t * terminator)
{
  the_lock_t<the_terminators_t> lock(*this);
  terminators_.push_front(terminator);
  
#ifdef DEBUG_TERMINATORS
  cerr << &terminators_ << ": appended " << terminator->id()
       << " terminator, addr " << terminator << endl;
#endif // DEBUG_TERMINATORS
}

//----------------------------------------------------------------
// the_terminators_t::del
// 
void
the_terminators_t::del(the_terminator_t * terminator)
{
  the_lock_t<the_terminators_t> lock(*this);
  
  std::list<the_terminator_t *>::iterator iter =
    std::find(terminators_.begin(), terminators_.end(), terminator);
  
  if (iter == terminators_.end())
  {
    cerr << "ERROR: no such terminator: " << terminator->id() << endl;
    assert(0);
    return;
  }
  
  terminators_.erase(iter);
  
#ifdef DEBUG_TERMINATORS
  cerr << &terminators_ << ": removed " << terminator->id()
       << " terminator, addr " << terminator << endl;
#endif // DEBUG_TERMINATORS
}
