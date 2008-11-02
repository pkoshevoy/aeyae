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


// File         : the_symbols.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : OpenGL 2D point symbols bitmap font.

// local includes:
#include "opengl/the_symbols.hxx"
#include "opengl/OpenGLCapabilities.h"
#include "math/the_color.hxx"


//----------------------------------------------------------------
// the_symbols_t::draw
// 
void
the_symbols_t::draw(const the_color_t & color,
		    const p3x1_t & pos,
		    const unsigned int & id) const
{
  glColor4fv(color.rgba());
  move(pos);
  draw(id);
}

//----------------------------------------------------------------
// the_symbols_t::draw
// 
void
the_symbols_t::draw(const unsigned int & id,
		    const p3x1_t & pos,
		    const the_color_t & color,
		    const the_color_t & mask_color) const
{
  static const int offset[][2] = {
    { -1,  0 },
    {  1,  0 },
    {  0, -1 },
    {  0,  1 },
    { -1, -1 },
    {  1, -1 },
    { -1,  1 },
    {  1,  1 }
  };

  glColor4fv(mask_color.rgba());
  if (bitmap_mask(id) != NULL)
  {
    // draw the mask:
    draw_bitmap_mask(id);
  }
  else
  {
    // simulate a mask:
    for (unsigned int i = 0; i < 4; i++)
    {
      move(pos);
      draw_bitmap(id, offset[i][0], offset[i][1]);
    }
  }
  
  // draw the symbol:
  glColor4fv(color.rgba());
  move(pos);
  draw(id);
}

//----------------------------------------------------------------
// the_symbols_t::draw
// 
void
the_symbols_t::draw(const unsigned int & id) const
{
  if (dl_offset_ != 0)
  {
    glCallList(dl_offset_ + id);
  }
  else
  {
    draw_bitmap(id);
  }
}

//----------------------------------------------------------------
// the_symbols_t::move
// 
void
the_symbols_t::move(const p3x1_t & pos) const
{
  glRasterPos3fv(pos.data());
}

//----------------------------------------------------------------
// the_symbols_t::compile
// 
void
the_symbols_t::compile()
{
  if (dl_offset_ != 0) return;
  
  dl_offset_ = glGenLists(size());
  if (dl_offset_ == 0) return;
  
  for (unsigned int i = 0; i < size(); i++)
  {
    glNewList(dl_offset_ + i, GL_COMPILE);
    draw_bitmap(i);
    glEndList();
  }
  
  FIXME_OPENGL("the_symbols_t::compile");
}

//----------------------------------------------------------------
// the_symbols_t::decompile
// 
void
the_symbols_t::decompile()
{
  if (dl_offset_ == 0) return;
  glDeleteLists(dl_offset_, size());
  dl_offset_ = 0;
  
  FIXME_OPENGL("the_symbols_t::decompile");
}

//----------------------------------------------------------------
// the_symbols_t::draw_bitmap
// 
void
the_symbols_t::draw_bitmap(const unsigned int & id,
			   const int & offset_x,
			   const int & offset_y) const
{
  unsigned char * bmp = bitmap(id);
  if (bmp == NULL) return;
  
  the_scoped_gl_client_attrib_t push_client_attr(GL_UNPACK_ALIGNMENT);
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(width(),
	     height(),
	     x_origin() + offset_x,
	     y_origin() + offset_y,
	     x_step(),
	     y_step(),
	     bmp);
    
    FIXME_OPENGL("the_symbols_t::draw_bitmap");
  }
}

//----------------------------------------------------------------
// the_symbols_t::draw_bitmap_mask
// 
void
the_symbols_t::draw_bitmap_mask(const unsigned int & id) const
{
  unsigned char * bmp = bitmap_mask(id);
  if (bmp == NULL) return;
  
  the_scoped_gl_client_attrib_t push_client_attr(GL_UNPACK_ALIGNMENT);
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBitmap(width() + 2,
	     height() + 2,
	     x_origin() + 1,
	     y_origin() + 1,
	     x_step(),
	     y_step(),
	     bmp);
    
    FIXME_OPENGL("the_symbols_t::draw_bitmap_mask");
  }
}
