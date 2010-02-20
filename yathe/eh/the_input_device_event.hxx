// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_input_device_event.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : mouse, keyboard and tablet event classes.

#ifndef THE_INPUT_DEVICE_EVENT_HXX_
#define THE_INPUT_DEVICE_EVENT_HXX_

// system includes:
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <assert.h>

// local includes:
#include "math/v3x1p3x1.hxx"
#include "io/io_base.hxx"

// forward declarations:
class the_view_t;


//----------------------------------------------------------------
// the_input_device_event_t
// 
// the base class for mouse/keybd/etc... events:
// 
class the_input_device_event_t : public io_base_t
{
public:
  the_input_device_event_t(the_view_t * widget = NULL):
    widget_(widget)
  {}
  
  // destination widget accessor:
  inline the_view_t * widget() const
  { return widget_; }
  
  the_view_t * widget_;
  
protected:
  void save(std::ostream & so,
	    const char * magic) const;
  
  bool load(std::istream & si,
	    const std::string & loaded_magic,
	    const char * magic);
};


//----------------------------------------------------------------
// the_mouse_event_t
// 
// Mouse event container:
class the_mouse_event_t : public the_input_device_event_t
{
public:
  the_mouse_event_t(the_view_t * widget = NULL,
		    unsigned int btns = 0,
		    unsigned int tran = 0,
		    unsigned int mods = 0,
		    bool double_click = false,
		    bool moving = false,
		    const p2x1_t & scs_pt = p2x1_t(0, 0));
  
  // virtual: file I/O API:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);
  
  inline unsigned int btns() const { return btns_; }
  inline unsigned int tran() const { return tran_; }
  inline unsigned int mods() const { return mods_; }
  
  inline const p2x1_t & scs_pt() const
  { return scs_pt_; }
  
  // this must be called in order to let the mouse button class
  // know how it can detect the down and up key transitions:
  static void setup_transition_detectors(unsigned int tran_down,
					 unsigned int tran_up);
  
  // transition ids:
  static unsigned int tran_down_;
  static unsigned int tran_up_;
  
  unsigned int btns_; // ids of the mouse buttons ORed together.
  unsigned int tran_; // type of transition -- up, down, etc...
  unsigned int mods_; // active modifier keys (shift, alt, ctrl).
  
  bool double_click_;
  bool moving_;
  
  // pointer location expressed in the screen coordinate system:
  p2x1_t scs_pt_;
};


//----------------------------------------------------------------
// the_wheel_event_t
// 
// Wheel event container:
class the_wheel_event_t : public the_input_device_event_t
{
public:
  the_wheel_event_t(the_view_t * widget = NULL,
		    unsigned int btns = 0,
		    unsigned int tran = 0,
		    unsigned int mods = 0,
		    const p2x1_t & scs_pt = p2x1_t(0, 0),
		    double degrees_rotated = 0,
		    bool vertical = true);
  
  // virtual: file I/O API:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);
  
  // accessors:
  inline unsigned int btns() const { return btns_; }
  inline unsigned int tran() const { return tran_; }
  inline unsigned int mods() const { return mods_; }
  
  inline const p2x1_t & scs_pt() const
  { return scs_pt_; }
  
  inline double degrees_rotated() const
  { return degrees_rotated_; }
  
  inline bool horizontal() const
  { return !vertical_; }
  
  inline bool vertical() const
  { return vertical_; }
  
  unsigned int btns_; // ids of the mouse buttons ORed together.
  unsigned int tran_; // type of transition (up or down).
  unsigned int mods_; // active modifier keys (shift, alt, ctrl).
  
  // pointer location expressed in the screen coordinate system:
  p2x1_t scs_pt_;
  
  // the distance that the wheel is rotated, in steps:
  double degrees_rotated_;
  
  // wheel orientation:
  bool vertical_;
};


//----------------------------------------------------------------
// the_keybd_event_t
// 
// Keyboard event container:
class the_keybd_event_t : public the_input_device_event_t
{
public:
  the_keybd_event_t(the_view_t * widget = NULL,
		    unsigned int key = 0,
		    unsigned int tran = 0,
		    unsigned int mods = 0,
		    bool autorepeat = false);
  
  // virtual: file I/O API:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);
  
  inline unsigned int key()  const { return key_; }
  inline unsigned int tran() const { return tran_; }
  inline unsigned int mods() const { return mods_; }
  
  inline bool autorepeat() const { return autorepeat_; }
  
  // this must be called in order to let the keyboard key class
  // know how it can detect the down and up key transitions:
  static void setup_transition_detectors(int tran_down,
					 int tran_up);
  
  // transition ids:
  static unsigned int tran_down_;
  static unsigned int tran_up_;
  
  unsigned int key_;  // id of the keyboard key.
  unsigned int tran_; // type of transition (up/dn).
  unsigned int mods_; // active modifier keys.
  bool autorepeat_;
};

extern ostream &
operator << (ostream & stream, const the_keybd_event_t & ke);


//----------------------------------------------------------------
// the_tablet_tool_t
// 
typedef enum {
  THE_TABLET_UNKNOWN_E,
  THE_TABLET_PEN_E,
  THE_TABLET_ERASER_E,
  THE_TABLET_CURSOR_E
} the_tablet_tool_t;

//----------------------------------------------------------------
// the_wacom_event_t
// 
// Tablet event container:
class the_wacom_event_t : public the_input_device_event_t
{
public:
  the_wacom_event_t(the_view_t * widget = NULL,
		    the_tablet_tool_t tool = THE_TABLET_PEN_E,
		    unsigned long int tool_id = 0,
		    const p2x1_t & scs_pt = p2x1_t(0, 0),
		    const p2x1_t & tilt = p2x1_t(0, 0),
		    float pressure = 0,
		    float tangential_pressure = 0,
		    float rotation = 0,
		    float z_position = 0);
  
  // virtual: file I/O API:
  void save(std::ostream & so) const;
  bool load(std::istream & si, const std::string & magic);
  
  // the type of tool that generated the event:
  inline the_tablet_tool_t tool() const
  { return tool_; }
  
  // unique id of the tablet tool that generated the event:
  inline const unsigned long int & tool_id() const
  { return tool_id_; }
  
  // tool location:
  inline const p2x1_t & scs_pt() const
  { return scs_pt_; }
  
  // pen/eraser tilt coordinates (u, v):
  // u: [-1.0 (left), 1.0 (right)]
  // v: [-1.0 (up),   1.0 (down) ]
  inline const p2x1_t & tilt() const
  { return tilt_; }
  
  // pen/eraser pressure [0.0 (no pressure), 1.0 (max pressure)]:
  inline float pressure() const
  { return pressure_; }
  
  inline float tangential_pressure() const
  { return tangential_pressure_; }
  
  // cursor orientation on the tablet (not yet supported by Qt):
  inline float rotation() const
  { return rotation_; }
  
  // For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;
  
  // tablet tool type:
  the_tablet_tool_t tool_;
  
  // tablet tool ID (unique):
  unsigned long int tool_id_;
  
  // tool location expressed in the screen coordinate system:
  p2x1_t scs_pt_;
  
  // pen/erase tilt vector:
  p2x1_t tilt_;
  
  // pen/eraser pressure:
  float pressure_;
  float tangential_pressure_;
  
  // cursor orientation:
  float rotation_;
  
  // Z-azis position of the device:
  float z_position_;
};

extern ostream &
operator << (ostream & stream, const the_wacom_event_t & we);


#endif // THE_INPUT_DEVICE_EVENT_HXX_
