// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_walltime.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 30 17:20:05 MST 2008
// Copyright    : Pavel Koshevoy (C) 2008
// License      : MIT
// Description  : Helper class for keeping track of time.

// system includes:
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include <stddef.h>

// local includes:
#include "the_walltime.hxx"


//----------------------------------------------------------------
// the_walltime_t
// 
the_walltime_t::the_walltime_t()
{
  mark();
}

//----------------------------------------------------------------
// the_walltime_t::mark
// 
void
the_walltime_t::mark()
{
#if defined(_WIN32)
  
  uint64_t msec = GetTickCount64();
  sec_ = msec / 1000;
  usec_ = (msec % 1000) * 1000;
  
#else
  
  struct timeval tv;
  gettimeofday(&tv, NULL);
  sec_ = tv.tv_sec + tv.tv_usec / 1000000;
  usec_ = tv.tv_usec % 1000000;
  
#endif
}

//----------------------------------------------------------------
// the_walltime_t::operator -=
// 
the_walltime_t &
the_walltime_t::operator -= (const the_walltime_t & ref)
{
  if (ref.usec_ > usec_)
  {
    usec_ = (usec_ + 1000000) - ref.usec_;
    sec_ -= (ref.sec_ + 1);
  }
  else
  {
    usec_ -= ref.usec_;
    sec_ -= ref.sec_;
  }
  
  return *this;
}

//----------------------------------------------------------------
// the_walltime_t::operator +=
// 
the_walltime_t &
the_walltime_t::operator += (const the_walltime_t & ref)
{
  usec_ += ref.usec_;
  sec_ += ref.sec_ + usec_ / 1000000;
  usec_ %= 1000000;
  
  return *this;
}
