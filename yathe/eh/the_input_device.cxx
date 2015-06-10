// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_input_device.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : mouse, keyboard and tablet abstraction classes.

// local includes:
#include "eh/the_input_device.hxx"


//----------------------------------------------------------------
// the_input_device_t::latest_time_stamp_
//
unsigned int
the_input_device_t::latest_time_stamp_ = 0;
