// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
