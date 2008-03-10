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


// File         : the_appearance.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The appearance class in combination with the palette class
//                provides basic theme/skin support for the OpenGL view widget.

// local includes:
#include "opengl/the_appearance.hxx"
#include "opengl/the_view.hxx"
#include "doc/the_document.hxx"
#include "doc/the_procedure.hxx"
#include "eh/the_input_device_eh.hxx"
#include "utils/the_text.hxx"


//----------------------------------------------------------------
// the_appearance_t::draw_background
// 
void
the_appearance_t::draw_background(the_view_t & view) const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT | GL_POLYGON_BIT);
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // draw the background:
    view.view_mgr().reset_opengl_viewing();
    view.view_mgr().setup_opengl_2d_viewing(p2x1_t(0.0, 0.0),
					    p2x1_t(1.0, 1.0));
    glBegin(GL_QUADS);
    {
      glColor4fv(palette_.bg()[THE_UL_CORNER_E].rgba());
      glVertex2f(0.0, 1.0);
    
      glColor4fv(palette_.bg()[THE_UR_CORNER_E].rgba());
      glVertex2f(1.0, 1.0);
      
      glColor4fv(palette_.bg()[THE_LR_CORNER_E].rgba());
      glVertex2f(1.0, 0.0);
      
      glColor4fv(palette_.bg()[THE_LL_CORNER_E].rgba());
      glVertex2f(0.0, 0.0);
    }
    glEnd();
  }
}

//----------------------------------------------------------------
// the_appearance_t::draw_coordinate_system
// 
void
the_appearance_t::draw_coordinate_system(the_view_t & view) const
{
  const the_eh_stack_t & eh_stack = view.eh_stack();
  
  if (eh_stack.empty())
  {
    the_ep_grid_csys_t(view.view_mgr(),
		       view.active_ep(),
		       palette()).draw();
  }
  else
  {
    the_ep_grid_csys_t(view.view_mgr(),
		       eh_stack.front()->edit_plane(&view),
		       palette()).draw();
  }
}

//----------------------------------------------------------------
// the_appearance_t::draw_view_label
// 
void
the_appearance_t::draw_view_label(the_view_t & view) const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_DEPTH_TEST);
    // draw the view label:
    view.view_mgr().reset_opengl_viewing();
    view.view_mgr().setup_opengl_2d_viewing(p2x1_t(0, view.height()),
					    p2x1_t(view.width(), 0));
    
    p3x1_t pos(THE_ASCII_FONT.x_step(),
	       1.2f * float(THE_ASCII_FONT.height()),
	       0);
    
    the_masked_text_dl_elem_t(pos,
			      palette_.text(),
			      palette_.mask(),
			      view.name()).draw();
  }
}

//----------------------------------------------------------------
// the_original_appearance_t::draw_edit_plane
// 
void
the_original_appearance_t::draw_edit_plane(the_view_t & view) const
{
  const the_eh_stack_t & eh_stack = view.eh_stack();
  
  if (eh_stack.empty())
  {
    the_original_ep_grid_t(view.view_mgr(),
			   view.active_ep(),
			   palette()).draw();
  }
  else
  {
    the_original_ep_grid_t(view.view_mgr(),
			   eh_stack.front()->edit_plane(&view),
			   palette()).draw();
  }
}


//----------------------------------------------------------------
// the_ampad_appearance_t::draw_edit_plane
// 
void
the_ampad_appearance_t::draw_edit_plane(the_view_t & view) const
{
  const the_eh_stack_t & eh_stack = view.eh_stack();
  
  if (eh_stack.empty())
  {
    the_ampad_ep_grid_t(view.view_mgr(),
			view.active_ep(),
			palette()).draw();
  }
  else
  {
    the_ampad_ep_grid_t(view.view_mgr(),
			eh_stack.front()->edit_plane(&view),
			palette()).draw();
  }
}


//----------------------------------------------------------------
// the_generic_appearance_t::draw_edit_plane
// 
void
the_generic_appearance_t::draw_edit_plane(the_view_t & view) const
{
  const the_eh_stack_t & eh_stack = view.eh_stack();
  
  if (eh_stack.empty())
  {
    the_quad_ep_grid_t(view.view_mgr(),
		       view.active_ep(),
		       palette()).draw();
  }
  else
  {
    the_quad_ep_grid_t(view.view_mgr(),
		       eh_stack.front()->edit_plane(&view),
		       palette()).draw();
  }
}


//----------------------------------------------------------------
// THE_DEFAULT_APPEARANCE
// 
// static const the_original_appearance_t THE_DEFAULT_APPEARANCE;
// static const the_generic_appearance_t THE_DEFAULT_APPEARANCE;
static const the_ampad_appearance_t THE_DEFAULT_APPEARANCE;

//----------------------------------------------------------------
// THE_APPEARANCE
// 
const the_appearance_t & THE_APPEARANCE = THE_DEFAULT_APPEARANCE;
