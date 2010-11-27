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


//----------------------------------------------------------------
// the_walltime_t
// 
struct the_walltime_t
{
  the_walltime_t();
  
  void mark();
  
  the_walltime_t & operator -= (const the_walltime_t & ref);
  the_walltime_t & operator += (const the_walltime_t & ref);
  
  unsigned int sec_;
  unsigned int usec_;
};


#endif // THE_WALLTIME_HXX_
