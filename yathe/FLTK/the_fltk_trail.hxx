// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_fltk_trail.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : non-functioning event trail recoring/playback mechanism
//                implemented as a compatibility layer for code that
//                requires it.

#ifndef THE_FLTK_TRAIL_HXX_
#define THE_FLTK_TRAIL_HXX_

// local includes:
#include "ui/the_trail.hxx"


//----------------------------------------------------------------
// the_fltk_trail_t
//
class the_fltk_trail_t : public the_trail_t
{
public:
  the_fltk_trail_t(int & argc, char ** argv, bool record_by_default = false);
};


#endif // THE_FLTK_TRAIL_HXX_
