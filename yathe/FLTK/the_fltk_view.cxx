// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_fltk_view.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2004/08/30 23:14
// Copyright    : (C) 2002
// License      : MIT
// Description  : FLTK wrapper for the OpenGL view widget.

// system includes:
#include <assert.h>

// local includes:
#include "FLTK/the_fltk_view.hxx"
#include "FLTK/the_fltk_input_device_event.hxx"
#include "ui/the_trail.hxx"
#include "eh/the_input_device_eh.hxx"


//----------------------------------------------------------------
// the_fltk_view_t::the_fltk_view_t
// 
the_fltk_view_t::
the_fltk_view_t(int w,
		int h,
		const char * label,
		const the_view_mgr_orientation_t & orientation,
		void * shared_context):
  Fl_Gl_Window(w, h, label),
  the_view_t(label, orientation)
{
  if (shared_context != NULL)
  {
    // use the shared context:
    context(shared_context, false);
  }
}

//----------------------------------------------------------------
// the_fltk_view_t::change_cursor
// 
void
the_fltk_view_t::change_cursor(const the_cursor_id_t & cursor_id)
{
  // FIXME:
}

//----------------------------------------------------------------
// the_fltk_view_t::resize
// 
void
the_fltk_view_t::resize(int x, int y, int w, int h)
{
  gl_resize(w, h);
  Fl_Gl_Window::resize(x, y, w, h);
}

//----------------------------------------------------------------
// the_fltk_view_t::draw
// 
void
the_fltk_view_t::draw()
{
  if (!valid())
  {
    gl_setup();
  }
  
  gl_paint();
}

//----------------------------------------------------------------
// the_fltk_view_t::handle
// 
int
the_fltk_view_t::handle(int event)
{
  switch (event)
  {
    case FL_ENTER:
    case FL_LEAVE:
    case FL_FOCUS:
    case FL_UNFOCUS:
      return 1;
      
    case FL_KEYDOWN:
    case FL_KEYUP:
    {
      the_input_device_t::advance_time_stamp();
      the_keybd_event_t ke = the_keybd_event(this, event);
      THE_KEYBD.update(ke);
      bool ok = eh_stack_->keybd_cb(ke);
      // if (ok) redraw();
      // dump(THE_MOUSE);
      // dump(THE_KEYBD);
      return ok;
    }
    
    case FL_RELEASE:
    case FL_PUSH:
    case FL_DRAG:
    case FL_MOVE:
    {
      the_input_device_t::advance_time_stamp();
      the_mouse_event_t me = the_mouse_event(this, event);
      THE_MOUSE.update(me);
      THE_KEYBD.update(me);
      eh_stack_->mouse_cb(me);
      
      if (event != FL_MOVE)
      {
	// dump(THE_MOUSE);
	// dump(THE_KEYBD);
      }
      
      return 1;
    }
    
    case FL_MOUSEWHEEL:
   {
      the_input_device_t::advance_time_stamp();
      the_wheel_event_t we = the_wheel_event(this, event);
      eh_stack_->wheel_cb(we);
      // dump(THE_MOUSE);
      // dump(THE_KEYBD);
      return 1;
    }
      
    default:
      break;
  }
  
  return Fl_Gl_Window::handle(event);
}
