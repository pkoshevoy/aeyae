// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_fltk_input_device_event.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Aug 29 20:30:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : mouse, keyboard event wrapper class implementation.

// local includes:
#include "FLTK/the_fltk_input_device_event.hxx"
#include "FLTK/the_fltk_view.hxx"


//----------------------------------------------------------------
// the_mouse_event
// 
the_mouse_event_t
the_mouse_event(Fl_Widget * widget, const int & event)
{
  the_mouse_event_t me(dynamic_cast<the_fltk_view_t *>(widget));
  me.moving_ = (event == FL_DRAG || event == FL_MOVE);
  
  if (Fl::event_clicks() > 0)
  {
    me.double_click_ = true;
    Fl::event_clicks(0);
  }
  
  if (event == FL_PUSH || event == FL_RELEASE)
  {
    int button = Fl::event_button();
    switch (button)
    {
      case 1:	me.btns_ = FL_BUTTON1;	break;
      case 2:	me.btns_ = FL_BUTTON2;	break;
      case 3:	me.btns_ = FL_BUTTON3;	break;
      default:	me.btns_ = 0;
    }
  }
  else
  {
    me.btns_ = Fl::event_state() & FL_BUTTONS;
  }
  
  // record the transition:
  me.tran_ = event;
  
  me.mods_ = Fl::event_state() & (FL_SHIFT |
				  FL_CTRL |
				  FL_ALT |
				  FL_META |
				  FL_COMMAND);
  
  me.scs_pt_.assign(float(Fl::event_x()) / float(widget->w()),
		    float(Fl::event_y()) / float(widget->h()));
  
#if 0
  cerr << "FIXME: mouse: " << me.btns_
       << ", mods: " << me.mods_
       << ", tran: " << me.tran_
       << ", event " << event << endl;
#endif
  
  return me;
}

//----------------------------------------------------------------
// the_wheel_event
// 
the_wheel_event_t
the_wheel_event(Fl_Widget * widget, const int & event)
{
  int ex = Fl::event_x();
  int ey = Fl::event_y();
  p2x1_t scs_pt(float(ex) / float(widget->w()),
		float(ey) / float(widget->h()));
  
  bool vertical = true;
  int delta = Fl::event_dy();
  if (delta == 0)
  {
    vertical = false;
    delta = Fl::event_dx();
  }
  
  the_wheel_event_t we(dynamic_cast<the_fltk_view_t *>(widget),
		       0, // btns
		       0, // tran
		       Fl::event_state() & (FL_SHIFT |
					    FL_CTRL |
					    FL_ALT |
					    FL_META |
					    FL_COMMAND),
		       scs_pt,
		       double(delta * -15),
		       vertical);
#if 0
  cerr << "FIXME: wheel: " << we.degrees_rotated_ << endl;
#endif
  
  return we;
}

//----------------------------------------------------------------
// the_keybd_event
// 
the_keybd_event_t
the_keybd_event(Fl_Widget * widget, const int & event)
{
#if 0
  cerr << "FIXME: keybd: " << Fl::event_key();
  if (Fl::event_key() < 256)
  {
    cerr << "\t\'" << (unsigned char)(Fl::event_key()) << '\'';
  }
  
  cerr << ", event " << event << endl;
#endif
  
  the_keybd_event_t ke(dynamic_cast<the_fltk_view_t *>(widget),
		       Fl::event_key(),
		       event,
		       Fl::event_state() & (FL_SHIFT |
					    FL_CTRL |
					    FL_ALT |
					    FL_META |
					    FL_COMMAND));
  
  // FIXME: this is a workaround for the Shift + Alt keypress which
  // reports incorrect scan code for Alt:
  if (ke.key_ == 65511) ke.key_ = 65513;
  if (ke.key_ == 65512) ke.key_ = 65514;
  
  return ke;
}
