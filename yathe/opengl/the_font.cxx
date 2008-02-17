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


// File         : the_font.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : An OpenGL bitmap font class.

// local includes:
#include "opengl/the_font.hxx"
#include "opengl/OpenGLCapabilities.h"
#include "math/the_color.hxx"


//----------------------------------------------------------------
// the_font_t::print
// 
void
the_font_t::print(const the_text_t & str,
		  const p3x1_t & pos,
		  const the_color_t & color) const
{
  glColor4fv(color.rgba());
  glRasterPos3fv(pos.data());
  
  const unsigned int str_len = str.size();
  if (dl_offset_ != 0)
  {
    the_scoped_gl_attrib_t push_attr(GL_LIST_BIT);
    {
      glListBase(dl_offset_);
      glCallLists(str_len, GL_UNSIGNED_BYTE, (GLubyte *)(str.text()));
    }
  }
  else
  {
    for (unsigned int i = 0; i < str_len; i++)
    {
      draw_bitmap(int(str.operator[](i)));
    }
  }
}

//----------------------------------------------------------------
// the_font_t::print
// 
void
the_font_t::print(const the_text_t & str,
		  const p3x1_t & pos,
		  const the_color_t & font_color,
		  const the_color_t & mask_color) const
{
  print_mask(str, pos, mask_color);
  print(str, pos, font_color);
}

//----------------------------------------------------------------
// the_font_t::print_mask
// 
void
the_font_t::print_mask(const the_text_t & str,
		       const p3x1_t & pos,
		       const the_color_t & color) const
{
  glColor4fv(color.rgba());
  glRasterPos3fv(pos.data());
  
  const unsigned int str_len = str.size();
  for (unsigned int i = 0; i < str_len; i++)
  {
    draw_bitmap_mask(int(str.operator[](i)));
  }
}
