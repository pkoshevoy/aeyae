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


// File         : the_qt_input_device_event.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2007/01/05 14:58:00
// Copyright    : (C) 2007
// License      : MIT
// Description  : Qt wrappers for input device events

#ifndef THE_QT_INPUT_DEVICE_EVENT_HXX_
#define THE_QT_INPUT_DEVICE_EVENT_HXX_

// local includes:
#include "eh/the_input_device_event.hxx"

// forward declarations:
class QWidget;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QTabletEvent;


//----------------------------------------------------------------
// the_mouse_event
// 
extern the_mouse_event_t
the_mouse_event(QWidget * widget, const QMouseEvent * e);

//----------------------------------------------------------------
// the_wheel_event
// 
extern the_wheel_event_t
the_wheel_event(QWidget * widget, const QWheelEvent * e);

//----------------------------------------------------------------
// the_keybd_event
// 
extern the_keybd_event_t
the_keybd_event(QWidget * widget, const QKeyEvent * e);

//----------------------------------------------------------------
// the_wacom_event
// 
extern the_wacom_event_t
the_wacom_event(QWidget * widget, const QTabletEvent * e);


#endif // THE_QT_INPUT_DEVICE_EVENT_HXX_
