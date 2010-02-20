// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_wacom_device.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2001/06/24 16:47:24
// Copyright    : (C) 2001, 2002, 2003
// License      : MIT
// Description  : wacom tablet device abstraction classes.

// local includes:
#include "eh/the_wacom_device.hxx"
#include "ui/the_trail.hxx"


//----------------------------------------------------------------
// the_wacom_device_t::the_wacom_device_t
// 
the_wacom_device_t::the_wacom_device_t():
  the_input_device_t(),
  tool_id_(0),
  scs_pt_(0.0, 0.0),
  pressure_(0.0)
{}

//----------------------------------------------------------------
// the_wacom_device_t::update
// 
void
the_wacom_device_t::update(const the_wacom_event_t & e)
{
  advance_time_stamp();
  
  if (tool_id_ == e.tool_id())
  {
    // make sure the event is genuinely new:
    float delta = (e.scs_pt() - scs_pt_).norm_sqrd();
    float dpres = e.pressure() - pressure_;
    
    // FIXME: hardcoded thresholds:
    if (delta < 1e-5f && dpres < 2e-2f)
    {
      return;
    }
    
    // cerr << "delta: " << delta << ", dpres: " << dpres << endl;
  }
  
  tool_id_  = e.tool_id();
  scs_pt_   = e.scs_pt();
  pressure_ = e.pressure();
  
  update_time_stamp();
}


//----------------------------------------------------------------
// the_wacom_t::update
// 
void
the_wacom_t::update(const the_wacom_event_t & te)
{
  switch (te.tool())
  {
    case THE_TABLET_PEN_E:
      pencil_.update(te);
      return;
      
    case THE_TABLET_ERASER_E:
      eraser_.update(te);
      return;
      
    case THE_TABLET_CURSOR_E:
      cursor_.update(te);
      return;
      
    default:
      cerr << "FIXME: the_wacom_t::update: unknown tool: " << te << endl;
      return;
  }
}
