// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : the_input_device_event.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : mouse, keyboard and tablet event class implementation.

// local includes:
#include "eh/the_input_device_event.hxx"
#include "ui/the_trail.hxx"
#include "utils/the_indentation.hxx"
#include "utils/instance_method_call.hxx"
#include "io/io_base.hxx"
#include "io/the_file_io.hxx"


//----------------------------------------------------------------
// the_input_device_event_t::save_widget
// 
void
the_input_device_event_t::save(std::ostream & so, const char * magic) const
{
  if (!is_open(so)) return;
  
  instance_t instance(widget_);
  if (!instance.was_saved())
  {
    // save the instance:
    instance.save(so);
  }
  
  so << magic << ' ';
  save_address(so, instance.address());
  so << ' ';
}

//----------------------------------------------------------------
// the_input_device_event_t::load_widget
// 
bool
the_input_device_event_t::load(std::istream & si,
			       const std::string & loaded_magic,
			       const char * magic)
{
  if (loaded_magic != magic)
  {
    return false;
  }
  
  // load the instance pointer:
  uint64_t addr = 0;
  if (!load_address(si, addr))
  {
    return false;
  }
  
  // the loaded address should be known to us by now:
  instance_t instance;
  if (!instance.init(addr))
  {
    return false;
  }
  
  widget_ = (the_view_t *)(instance.address());
  return true;
}


//----------------------------------------------------------------
// the_mouse_btn_t::tran_down_
// 
unsigned int
the_mouse_event_t::tran_down_ = 0;

//----------------------------------------------------------------
// the_mouse_event_t::tran_up_
// 
unsigned int
the_mouse_event_t::tran_up_ = 0;

//----------------------------------------------------------------
// the_mouse_event_t::the_mouse_event_t
// 
the_mouse_event_t::the_mouse_event_t(the_view_t * widget,
				     unsigned int btns,
				     unsigned int tran,
				     unsigned int mods,
				     bool double_click,
				     bool moving,
				     const p2x1_t & scs_pt):
  the_input_device_event_t(widget),
  btns_(btns),
  tran_(tran),
  mods_(mods),
  double_click_(double_click),
  moving_(moving),
  scs_pt_(scs_pt)
{}

//----------------------------------------------------------------
// the_mouse_event_t::save
// 
void
the_mouse_event_t::save(std::ostream & so) const
{
  if (!is_open(so)) return;

  the_input_device_event_t::save(so, "the_mouse_event_t");
  ::save(so, btns_);
  ::save(so, tran_);
  ::save(so, mods_);
  ::save(so, double_click_);
  ::save(so, moving_);
  ::save(so, scs_pt_);
}

//----------------------------------------------------------------
// the_mouse_event_t::load
// 
bool
the_mouse_event_t::load(std::istream & si, const std::string & magic)
{
  if (!the_input_device_event_t::load(si, magic, "the_mouse_event_t"))
  {
    return false;
  }
  
  bool ok = (::load(si, btns_) &&
	     ::load(si, tran_) &&
	     ::load(si, mods_) &&
	     ::load(si, double_click_) &&
	     ::load(si, moving_) &&
	     ::load(si, scs_pt_));
  return ok;
}

//----------------------------------------------------------------
// the_mouse_event_t::setup_transition_detectors
// 
void
the_mouse_event_t::setup_transition_detectors(unsigned int tran_down,
					      unsigned int tran_up)
{
  tran_down_ = tran_down;
  tran_up_ = tran_up;
}


//----------------------------------------------------------------
// the_wheel_event_t::the_wheel_event_t
// 
the_wheel_event_t::the_wheel_event_t(the_view_t * widget,
				     unsigned int btns,
				     unsigned int tran,
				     unsigned int mods,
				     const p2x1_t & scs_pt,
				     double degrees_rotated,
				     bool vertical):
  the_input_device_event_t(widget),
  btns_(btns),
  tran_(tran),
  mods_(mods),
  scs_pt_(scs_pt),
  degrees_rotated_(degrees_rotated),
  vertical_(vertical)
{}

//----------------------------------------------------------------
// the_wheel_event_t::save
// 
void
the_wheel_event_t::save(std::ostream & so) const
{
  if (!is_open(so)) return;
  
  the_input_device_event_t::save(so, "the_wheel_event_t");
  ::save(so, btns_);
  ::save(so, tran_);
  ::save(so, mods_);
  ::save(so, scs_pt_);
  ::save(so, degrees_rotated_);
  ::save(so, vertical_);
}

//----------------------------------------------------------------
// the_wheel_event_t::load
// 
bool
the_wheel_event_t::load(std::istream & si, const std::string & magic)
{
  if (!the_input_device_event_t::load(si, magic, "the_wheel_event_t"))
  {
    return false;
  }
  
  bool ok = (::load(si, btns_) &&
	     ::load(si, tran_) &&
	     ::load(si, mods_) &&
	     ::load(si, scs_pt_) &&
	     ::load(si, degrees_rotated_) &&
	     ::load(si, vertical_));
  return ok;
}


//----------------------------------------------------------------
// the_keybd_event_t::tran_down_
// 
unsigned int
the_keybd_event_t::tran_down_ = ~0;

//----------------------------------------------------------------
// the_keybd_event_t::tran_up_
// 
unsigned int
the_keybd_event_t::tran_up_ = ~0;

//----------------------------------------------------------------
// the_keybd_event_t::setup_transition_detectors
// 
void
the_keybd_event_t::setup_transition_detectors(int tran_down,
					      int tran_up)
{
  tran_down_ = tran_down;
  tran_up_ = tran_up;
}

//----------------------------------------------------------------
// the_keybd_event_t::the_keybd_event_t
// 
the_keybd_event_t::the_keybd_event_t(the_view_t * widget,
				     unsigned int key,
				     unsigned int tran,
				     unsigned int mods,
				     bool autorepeat):
  the_input_device_event_t(widget),
  key_(key),
  tran_(tran),
  mods_(mods),
  autorepeat_(autorepeat)
{}

//----------------------------------------------------------------
// the_keybd_event_t::save
// 
void
the_keybd_event_t::save(std::ostream & so) const
{
  if (!is_open(so)) return;
  
  the_input_device_event_t::save(so, "the_keybd_event_t");
  ::save(so, key_);
  ::save(so, tran_);
  ::save(so, mods_);
  ::save(so, autorepeat_);
}

//----------------------------------------------------------------
// the_keybd_event_t::load
// 
bool
the_keybd_event_t::load(std::istream & si, const std::string & magic)
{
  if (!the_input_device_event_t::load(si, magic, "the_keybd_event_t"))
  {
    return false;
  }
  
  bool ok = (::load(si, key_) &&
	     ::load(si, tran_) &&
	     ::load(si, mods_) &&
	     ::load(si, autorepeat_));
  return ok;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & sout, const the_keybd_event_t & ke)
{
  sout << "the_keybd_event_t: " << endl;
  
  if (ke.key() < 128)
  {
    sout << "key_    = " << ke.key() << " (" << char(ke.key()) << ')' << endl;
  }
  else
  {
    sout << "key_    = " << ke.key() << endl;
  }
  
  sout << "tran_   = " << ke.tran() << endl
       << "mods_   = " << ke.mods() << endl
       << "widget_ = " << ke.widget() << endl
       << "autorepeat_ = " << ke.autorepeat() << endl;
  
  return sout;
}


//----------------------------------------------------------------
// the_wacom_event_t::the_wacom_event_t
// 
the_wacom_event_t::the_wacom_event_t(the_view_t * widget,
				     the_tablet_tool_t tool,
				     unsigned long int tool_id,
				     const p2x1_t & scs_pt,
				     const p2x1_t & tilt,
				     float pressure,
				     float tangential_pressure,
				     float rotation,
				     float z_position):
  the_input_device_event_t(widget),
  tool_(tool),
  tool_id_(tool_id),
  scs_pt_(scs_pt),
  tilt_(tilt),
  pressure_(pressure),
  tangential_pressure_(tangential_pressure),
  rotation_(rotation),
  z_position_(z_position)
{}

//----------------------------------------------------------------
// the_wacom_event_t::save
// 
void
the_wacom_event_t::save(std::ostream & so) const
{
  if (!is_open(so)) return;
  
  the_input_device_event_t::save(so, "the_wacom_event_t");

  int tool = tool_;
  ::save(so, tool);
  
  uint64_t tool_id = tool_id_;
  ::save_address(so, tool_id);
  
  ::save(so, scs_pt_);
  ::save(so, tilt_);
  ::save(so, pressure_);
  ::save(so, tangential_pressure_);
  ::save(so, rotation_);
  ::save(so, z_position_);
}

//----------------------------------------------------------------
// the_wacom_event_t::load
// 
bool
the_wacom_event_t::load(std::istream & si, const std::string & magic)
{
  if (!the_input_device_event_t::load(si, magic, "the_wacom_event_t"))
  {
    return false;
  }
  
  int tool = 0;
  uint64_t tool_id = 0;
  bool ok = (::load(si, tool) &&
	     ::load_address(si, tool_id) &&
	     ::load(si, scs_pt_) &&
	     ::load(si, tilt_) &&
	     ::load(si, pressure_) &&
	     ::load(si, tangential_pressure_) &&
	     ::load(si, rotation_) &&
	     ::load(si, z_position_));
  tool_ = (the_tablet_tool_t)tool;
  tool_id_ = (unsigned long int)tool_id;
  return ok;
}

//----------------------------------------------------------------
// the_wacom_event_t::dump
// 
void
the_wacom_event_t::dump(ostream & sout, unsigned int indent) const
{
  const char * tool_str = NULL;
  switch (tool_)
  {
    case THE_TABLET_PEN_E:
      tool_str = "Pen";
      break;
      
    case THE_TABLET_ERASER_E:
      tool_str = "Eraser";
      break;
      
    case THE_TABLET_CURSOR_E:
      tool_str = "Cursor";
      break;
      
    default:
      tool_str = "unknown tablet tool";
      break;
  }
  
  sout << INDSCP << "the_wacom_event_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "tool                = " << tool_str << endl
       << INDSTR << "tool_id             = " << int(tool_id_) << endl
       << INDSTR << "scs_pt              = " << scs_pt_ << endl
       << INDSTR << "tilt                = " << tilt_ << endl
       << INDSTR << "pressure            = " << pressure_ << endl
       << INDSTR << "tangential pressure = " << tangential_pressure_ << endl
       << INDSTR << "rotation            = " << rotation_ << endl
       << INDSTR << "Z position          = " << z_position_ << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & sout, const the_wacom_event_t & we)
{
  we.dump(sout);
  return sout;
}
