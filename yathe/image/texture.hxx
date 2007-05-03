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


// File         : texture.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 3 18:13:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a texture convenience class

#ifndef TEXTURE_HXX_
#define TEXTURE_HXX_

// local includes:
#include "opengl/OpenGLCapabilities.h"


//----------------------------------------------------------------
// texture_base_t
// 
class texture_base_t
{
public:
  texture_base_t(GLenum type = 0,
		 GLint internal_format = 0,
		 GLenum format = 0,
		 GLsizei width = 0,
		 GLsizei height = 0,
		 GLint border = 0,
		 GLint alignment = 4,
		 GLint row_length = 0,
		 GLint skip_pixels = 0,
		 GLint skip_rows = 0,
		 GLboolean swap_bytes = GL_FALSE,
		 GLboolean lsb_first = GL_FALSE);
  
  virtual ~texture_base_t();
  
  // texture data accessor:
  virtual const GLubyte * texture() const = 0;
  
  // use glTexImage to allocate the OpenGL texture, return true on success:
  bool setup(const GLuint & texture_id) const;
  
  // check whether this texture has been properly setup and
  // can be used successfully to upload the texture data:
  bool is_valid(const GLuint & texture_id) const;
  
  // use glTexSubImage to upload the specified image region
  // into the OpenGL texture memory:
  void upload(const GLuint & texture_id,
	      GLint x,
	      GLint y,
	      GLsizei w,
	      GLsizei h) const;
  
  // upload the entire image into the OpenGL texture memory:
  inline void upload(const GLuint & texture_id) const
  { upload(texture_id, skip_pixels_, skip_rows_, width_, height_); }
  
  // apply texture parameters:
  void apply(const GLuint & texture_id) const;
  
  // this is for debugging only:
  void debug() const;
  
  // common texture attributes:
  GLenum type_;
  GLint internal_format_;
  GLenum format_;
  
  GLsizei width_;
  GLsizei height_;
  GLint border_;
  
  GLint alignment_;
  GLint row_length_;
  GLint skip_pixels_;
  GLint skip_rows_;
  
  GLboolean swap_bytes_;
  GLboolean lsb_first_;
};


//----------------------------------------------------------------
// texture_t
//
template <typename TDataPtr>
class texture_t : public texture_base_t
{
public:
  typedef TDataPtr DataPtrType;
  
  texture_t(const TDataPtr & texture,
	    GLenum type = 0,
	    GLint internal_format = 0,
	    GLenum format = 0,
	    GLsizei width = 0,
	    GLsizei height = 0,
	    GLint border = 0,
	    GLint alignment = 4,
	    GLint row_length = 0,
	    GLint skip_pixels = 0,
	    GLint skip_rows = 0,
	    GLboolean swap_bytes = GL_FALSE,
	    GLboolean lsb_first = GL_FALSE):
    texture_base_t(type,
		   internal_format,
		   format,
		   width,
		   height,
		   border,
		   alignment,
		   row_length,
		   skip_pixels,
		   skip_rows,
		   swap_bytes,
		   lsb_first),
    texture_(texture)
  {}
  
  // virtual:
  const GLubyte * texture() const
  { return texture_->data(); }
  
  // the texture data:
  TDataPtr texture_;
};


#endif // TEXTURE_HXX_
