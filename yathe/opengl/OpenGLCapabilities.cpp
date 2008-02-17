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


// File         : OpenGLCapabilities.cpp
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Aug 1 23:14:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : Helper functions for working with OpenGL.

// local includes:
#include "opengl/OpenGLCapabilities.h"
#include "opengl/the_view.hxx"
#include "ui/the_document_ui.hxx"
#include "utils/the_dynamic_array.hxx"

// system includes:
#include <iostream>
#include <string>
#include <vector>
#include <list>


// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// the_scoped_gl_attrib_t::the_scoped_gl_attrib_t
// 
the_scoped_gl_attrib_t::the_scoped_gl_attrib_t(GLbitfield mask):
  applied_(false)
{
  // return;
  
  mask = mask ^ GL_LINE_BIT;
  mask = mask ^ GL_ENABLE_BIT;
  if (!mask) return;
  
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    glPushAttrib(mask);
    err = glGetError();
    
    if (err == GL_NO_ERROR)
    {
      applied_ = true;
    }
    else
    {
      const GLubyte * str = gluErrorString(err);
      cerr << "GL_ERROR: " << str << endl;
    }
  }
}

//----------------------------------------------------------------
// the_scoped_gl_attrib_t::~the_scoped_gl_attrib_t
// 
the_scoped_gl_attrib_t::~the_scoped_gl_attrib_t()
{
  if (applied_)
  {
    glPopAttrib();
  }
}


//----------------------------------------------------------------
// the_scoped_gl_client_attrib_t::the_scoped_gl_client_attrib_t
// 
the_scoped_gl_client_attrib_t::the_scoped_gl_client_attrib_t(GLbitfield mask):
  applied_(false)
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    glPushClientAttrib(mask);
    err = glGetError();
    
    if (err == GL_NO_ERROR)
    {
      applied_ = true;
    }
    else
    {
      const GLubyte * str = gluErrorString(err);
      cerr << "GL_ERROR: " << str << endl;
    }
  }
}

//----------------------------------------------------------------
// the_scoped_gl_client_attrib_t::~the_scoped_gl_client_attrib_t
// 
the_scoped_gl_client_attrib_t::~the_scoped_gl_client_attrib_t()
{
  if (applied_)
  {
    glPopClientAttrib();
  }
}


//----------------------------------------------------------------
// the_scoped_gl_matrix_t::the_scoped_gl_matrix_t
// 
the_scoped_gl_matrix_t::the_scoped_gl_matrix_t(GLenum mode):
  applied_(false)
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    glMatrixMode(mode);
    glPushMatrix();
    err = glGetError();
    
    if (err == GL_NO_ERROR)
    {
      applied_ = true;
    }
    else
    {
      const GLubyte * str = gluErrorString(err);
      cerr << "GL_ERROR: " << str << endl;
    }
  }
}

//----------------------------------------------------------------
// the_scoped_gl_matrix_t::
// 
the_scoped_gl_matrix_t::~the_scoped_gl_matrix_t()
{
  if (applied_)
  {
    glPopMatrix();
  }
}


//----------------------------------------------------------------
// OpenGLCapabilities::OpenGLCapabilities
// 
OpenGLCapabilities::OpenGLCapabilities()
{
  const GLubyte * vendor = glGetString(GL_VENDOR);
  assert(vendor != NULL);
  vendor_.assign((const char *)(vendor));
  
  const GLubyte * version = glGetString(GL_VERSION);
  assert(version != NULL);
  version_.assign((const char *)(version));
  
  const GLubyte * renderer = glGetString(GL_RENDERER);
  assert(renderer != NULL);
  renderer_.assign((const char *)(renderer));
  
  const GLubyte * extensions = glGetString(GL_EXTENSIONS);
  assert(extensions != NULL);
  extensions_.assign((const char *)extensions);
  
  std::list<char> char_list;
  std::list<std::string> ext_list;
  for (unsigned int i = 0;; i++)
  {
    const char c = char(extensions_[i]);
    if (c == ' ' || c == '\0')
    {
      std::string ext(char_list.begin(), char_list.end());
      char_list.clear();
      ext_list.push_back(ext);
      if (c == '\0') break;
    }
    else
    {
      char_list.push_back(c);
    }
  }
  extension_array_.assign(ext_list.begin(), ext_list.end());
  
  max_texture_ = 64;
  hardware_mipmap_ = checkExtension("GL_SGIS_generate_mipmap");
  compressed_textures_ = checkExtension("GL_ARB_texture_compression");
}

//----------------------------------------------------------------
// OpenGLCapabilities::checkExtension
// 
bool
OpenGLCapabilities::checkExtension(const char * query) const
{
  if (query != NULL)
  {
    std::string extension(query);
    for (unsigned int i = 0; i < extension_array_.size(); i++)
    {
      if (extension_array_[i] == extension)
      {
	return true;
      }
    }
  }
  
  return false;
}

//----------------------------------------------------------------
// OpenGLCapabilities::maxTextureSize
// 
unsigned int
OpenGLCapabilities::maxTextureSize(GLenum internal_format,
				   GLenum format,
				   GLenum type,
				   GLint border)
{
  GLuint size = 64;
  for (int i = 0; i < 8; i++, size *= 2)
  {
    glTexImage2D(GL_PROXY_TEXTURE_2D,
		 0, // level
		 internal_format,
		 size * 2, // width
		 size * 2, // height
		 border,
		 format,
		 type,
		 NULL);// texels
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) break;
    
    GLint width = 0;
    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
			     0, // level
			     GL_TEXTURE_WIDTH,
			     &width);
    if (width != GLint(size * 2)) break;
  }
  
  return size;
}

//----------------------------------------------------------------
// OpenGLCapabilities::dump
// 
void
OpenGLCapabilities::dump() const
{
  cerr << "OpenGL vendor: " << vendor_ << endl
       << "OpenGL version: " << version_ << endl
       << "OpenGL renderer: " << renderer_ << endl
       << "OpenGL extensions: " << endl;
  for (unsigned int i = 0; i < extension_array_.size(); i++)
  {
    cerr << extension_array_[i] << endl;
  }
  
  cerr << endl
       << "MAX TILE SIZE: " << max_texture_ << endl
       << "HARDWARE MIPMAP: " << hardware_mipmap_ << endl
       << "COMPRESSED TEXTURES: " << compressed_textures_ << endl;
}

//----------------------------------------------------------------
// OpenGL
// 
OpenGLCapabilities & OpenGL()
{
  static OpenGLCapabilities * open_gl_ = NULL;
  if (open_gl_ == NULL)
  {
    // FIXME: this should probably be done somewhere else:
    the_document_ui_t::doc_ui()->shared()->gl_make_current();
    
    open_gl_ = new OpenGLCapabilities();
  }
  
  return *open_gl_;
}

//----------------------------------------------------------------
// OpenGL
// 
OpenGLCapabilities &
OpenGL(unsigned int view_id, the_view_t * view)
{
  static the_dynamic_array_t<OpenGLCapabilities *> open_gl_(16, 16, NULL);
  
  if (view != NULL)
  {
    if (open_gl_.size() <= view_id)
    {
      open_gl_.resize(view_id + 1);
    }
    
    view->gl_make_current();
    delete open_gl_[view_id];
    open_gl_[view_id] = new OpenGLCapabilities();
  }
  
  assert(open_gl_.size() > view_id);
  assert(open_gl_[view_id] != NULL);
  return *(open_gl_[view_id]);
}
