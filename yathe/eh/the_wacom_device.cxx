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
