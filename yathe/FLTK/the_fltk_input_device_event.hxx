// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_fltk_input_device_event.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Aug 29 20:30:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : mouse, keyboard event wrapper classes.

#ifndef THE_FLTK_INPUT_DEVICE_EVENT_HXX_
#define THE_FLTK_INPUT_DEVICE_EVENT_HXX_

// local includes:
#include "math/v3x1p3x1.hxx"
#include "eh/the_input_device_event.hxx"

// FLTK includes:
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Enumerations.H>

// system includes:
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <assert.h>


//----------------------------------------------------------------
// the_mouse_event
//
extern the_mouse_event_t
the_mouse_event(Fl_Widget * widget, const int & event);

//----------------------------------------------------------------
// the_wheel_event
//
extern the_wheel_event_t
the_wheel_event(Fl_Widget * widget, const int & event);

//----------------------------------------------------------------
// the_keybd_event
//
extern the_keybd_event_t
the_keybd_event(Fl_Widget * widget, const int & event);


#endif // THE_FLTK_INPUT_DEVICE_EVENT_HXX_
