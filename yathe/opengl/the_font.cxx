// File         : the_font.cxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

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
    glPushAttrib(GL_LIST_BIT);
    {
      glListBase(dl_offset_);
      glCallLists(str_len, GL_UNSIGNED_BYTE, (GLubyte *)(str.text()));
    }
    glPopAttrib();
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
