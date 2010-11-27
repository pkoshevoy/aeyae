// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_walltime.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 30 17:20:05 MST 2008
// Copyright    : Pavel Koshevoy (C) 2008
// License      : MIT
// Description  : Helper class for keeping track of time.

#ifndef THE_WALLTIME_HXX_
#define THE_WALLTIME_HXX_

// system includes:
#ifndef _WIN32
#  include <inttypes.h>
#else
#  ifndef uint64_t
typedef unsigned __int64 uint64_t;
#  endif
#  ifndef int64_t
typedef __int64 int64_t;
#  endif
#endif


//----------------------------------------------------------------
// the_walltime_t
// 
struct the_walltime_t
{
  the_walltime_t();
  
  void mark();
  
  the_walltime_t & operator -= (const the_walltime_t & ref);
  the_walltime_t & operator += (const the_walltime_t & ref);
  
  // return a difference in seconds between this and a given wall time:
  inline double operator - (const the_walltime_t & t) const
  {
    double sec = double(sec_) - double(t.sec_);
    double usec = double(usec_) - double(t.usec_);
    return sec + usec * 1e-6;
  }
  
  // return this wall time in seconds:
  inline double seconds() const
  { return double(sec_) + double(usec_) * 1e-6; }
  
  int64_t sec_;
  unsigned int usec_;
};


#endif // THE_WALLTIME_HXX_
