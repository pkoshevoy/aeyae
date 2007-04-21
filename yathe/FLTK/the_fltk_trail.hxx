// File         : the_fltk_trail.hxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : GPL.
// Description  : event trail recoring/playback (regression testing).

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
