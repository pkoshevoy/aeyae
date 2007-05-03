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
