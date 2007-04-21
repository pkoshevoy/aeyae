// File         : the_view.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  : The OpenGL document view.

#ifndef THE_FLTK_VIEW_HXX_
#define THE_FLTK_VIEW_HXX_

// local includes:
#include "opengl/the_view.hxx"

// FLTK includes:
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Enumerations.H>


//----------------------------------------------------------------
// the_fltk_view_t
// 
class the_fltk_view_t : public Fl_Gl_Window,
			public the_view_t
{
public:
  // Constructor for the main view:
  the_fltk_view_t(int w,
		  int h,
		  const char * label,
		  const the_view_mgr_orientation_t & o = THE_ISOMETRIC_VIEW_E,
		  void * shared_context = NULL);
  
  // virtual:
  bool is_hidden()
  { return !Fl_Gl_Window::shown(); }
  
  // virtual:
  void set_focus()
  { Fl_Gl_Window::take_focus(); }
  
  // virtual:
  void refresh()
  { Fl_Gl_Window::redraw(); }
  
  // virtual:
  bool gl_context_is_valid() const
  { return Fl_Gl_Window::valid(); }
  
  // virtual:
  void gl_make_current()
  { Fl_Gl_Window::make_current(); }
  
  // virtual:
  void gl_done_current()
  {
    // FIXME: Fl_Gl_Window::invalidate();
  }
  
  // virtual:
  void change_cursor(const the_cursor_id_t & cursor_id);
  
  // virtual:
  void resize(int x, int y, int w, int h);
  void draw();
  
  // virtual:
  int handle(int event);
};


#endif // THE_FLTK_VIEW_HXX_
