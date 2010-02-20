// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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
  the_text_t tmp(str);
  tmp.to_ascii();
  
  glColor4fv(color.rgba());
  glRasterPos3fv(pos.data());
  
  const size_t str_len = str.size();
  if (dl_offset_ != 0)
  {
    the_scoped_gl_attrib_t push_attr(GL_LIST_BIT);
    {
      glListBase(dl_offset_);
      glCallLists((GLsizei)str_len, GL_UNSIGNED_BYTE, (GLubyte *)(tmp.text()));
    }
  }
  else
  {
    for (size_t i = 0; i < str_len; i++)
    {
      draw_bitmap(int(tmp.operator[](i)));
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
  the_text_t tmp(str);
  tmp.to_ascii();
  
  print_mask(tmp, pos, mask_color);
  print(tmp, pos, font_color);
}

//----------------------------------------------------------------
// the_font_t::print_mask
// 
void
the_font_t::print_mask(const the_text_t & str,
		       const p3x1_t & pos,
		       const the_color_t & color) const
{
  the_text_t tmp(str);
  tmp.to_ascii();
  
  glColor4fv(color.rgba());
  glRasterPos3fv(pos.data());
  
  const size_t str_len = str.size();
  for (size_t i = 0; i < str_len; i++)
  {
    draw_bitmap_mask(int(tmp.operator[](i)));
  }
}
