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


// File         : the_log.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Mar 23 11:04:53 MDT 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : A text log object -- behaves almost like a std::ostream.

// local includes:
#include "the_log.hxx"


//----------------------------------------------------------------
// the_log_t::the_log_t
// 
the_log_t::the_log_t():
  mutex_(NULL)
{
  mutex_ = the_mutex_interface_t::create();
}

//----------------------------------------------------------------'
// the_log_t::~the_log_t
// 
the_log_t::~the_log_t()
{
  delete mutex_;
}

//----------------------------------------------------------------
// the_log_t::log_no_lock
// 
void
the_log_t::log_no_lock(std::ostream & (*f)(std::ostream &))
{
  f(line_);
}

//----------------------------------------------------------------
// the_log_t::operator
// 
the_log_t &
the_log_t::operator << (std::ostream & (*f)(std::ostream &))
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  log_no_lock(f);
  return *this;
}

//----------------------------------------------------------------
// the_log_t::precision
// 
int
the_log_t::precision()
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  int p = line_.precision();
  return p;
}

//----------------------------------------------------------------
// the_log_t::precision
// 
int
the_log_t::precision(int n)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  int p = line_.precision(n);
  return p;
}

//----------------------------------------------------------------
// the_log_t::flags
// 
std::ios::fmtflags
the_log_t::flags() const
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  std::ios::fmtflags f = line_.flags();
  return f;
}

//----------------------------------------------------------------
// the_log_t::flags
// 
std::ios::fmtflags
the_log_t::flags(std::ios::fmtflags fmt)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  std::ios::fmtflags f = line_.flags(fmt);
  return f;
}

//----------------------------------------------------------------
// the_log_t::setf
// 
void
the_log_t::setf(std::ios::fmtflags fmt)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  line_.setf(fmt);
}

//----------------------------------------------------------------
// the_log_t::setf
// 
void
the_log_t::setf(std::ios::fmtflags fmt, std::ios::fmtflags msk)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  line_.setf(fmt, msk);
}

//----------------------------------------------------------------
// the_log_t::unsetf
// 
void
the_log_t::unsetf(std::ios::fmtflags fmt)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  line_.unsetf(fmt);
}

//----------------------------------------------------------------
// the_log_t::copyfmt
// 
void
the_log_t::copyfmt(std::ostream & ostm)
{
  the_lock_t<the_mutex_interface_t> lock(mutex_);
  line_.copyfmt(ostm);
}


//----------------------------------------------------------------
// null_log
// 
the_null_log_t *
null_log()
{
  static the_null_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_null_log_t;
  }
  
  return log;
}


//----------------------------------------------------------------
// cerr_log
// 
the_stream_log_t *
cerr_log()
{
  static the_stream_log_t * log = NULL;
  if (log == NULL)
  {
    log = new the_stream_log_t(std::cerr);
  }
  
  return log;
}
