// File         : the_input_device.cxx
// Author       : Paul A. Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : GPL.
// Description  : mouse, keyboard and tablet abstraction classes.

// local includes:
#include "eh/the_input_device.hxx"


//----------------------------------------------------------------
// the_input_device_t::latest_time_stamp_
// 
unsigned int
the_input_device_t::latest_time_stamp_ = 0;
