// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : OpenGLCapabilities.h
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Tue Aug 1 23:14:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : Helper functions for working with OpenGL.

#ifndef OPENGL_CAPABILITIES_HXX_
#define OPENGL_CAPABILITIES_HXX_

// system includes:
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

// OpenGL includes:
#include <GL/glew.h>
#if defined(__APPLE__)
#  include <AGL/agl.h>
#  include <AGL/aglRenderers.h>
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <OpenGL/glext.h>
#else
#  if defined(_WIN32)
#    include <windows.h>
#  endif
#  include <GL/gl.h>
#  include <GL/glu.h>
#  if !defined(_WIN32)
#    include <GL/glext.h>
#  endif
#endif

// forward declarations:
class the_view_t;


//----------------------------------------------------------------
// DEBUG_TEXTURES
// 
// #define DEBUG_TEXTURES


//----------------------------------------------------------------
// the_scoped_gl_attrib_t
// 
class the_scoped_gl_attrib_t
{
public:
  the_scoped_gl_attrib_t(GLbitfield mask);
  ~the_scoped_gl_attrib_t();
  
  bool applied_;
};

//----------------------------------------------------------------
// the_scoped_gl_client_attrib_t
// 
class the_scoped_gl_client_attrib_t
{
public:
  the_scoped_gl_client_attrib_t(GLbitfield mask);
  ~the_scoped_gl_client_attrib_t();
  
  bool applied_;
};

//----------------------------------------------------------------
// the_scoped_gl_matrix_t
// 
class the_scoped_gl_matrix_t
{
public:
  the_scoped_gl_matrix_t(GLenum mode);
  ~the_scoped_gl_matrix_t();
  
  bool applied_;
};


//----------------------------------------------------------------
// OpenGLCapabilities
// 
class OpenGLCapabilities
{
public:
  OpenGLCapabilities();
  
  bool checkExtension(const char * query) const;
  
  // determine max 2D texture size (width or height):
  static unsigned int maxTextureSize(GLenum internal_format,
				     GLenum format,
				     GLenum type,
				     GLint border = 0);
  
  // for debugging:
  void dump() const;
  
  // capabilities:
  std::string vendor_;
  std::string version_;
  std::string renderer_;
  std::string extensions_;
  std::vector<std::string> extension_array_;
  
  // flag indicating whether hardware mipmap generation is supported:
  bool hardware_mipmap_;
  
  // flag indicating whether the compressed textures are supported:
  bool compressed_textures_;
  
  // max 2D texture size (width or height):
  GLuint max_texture_;
};


//----------------------------------------------------------------
// OpenGL
// 
// Return the cached OpenGL capabilities
// for the shared OpenGL context.
// 
extern OpenGLCapabilities & OpenGL();


//----------------------------------------------------------------
// OpenGL
// 
// Return the cached OpenGL capabilities for a given view.
// The capabilities can be re-cached by specifying a non-NULL
// view pointer. Trying to access OpenGL capabilities for
// a view prior to caching them will trigger an assertion.
// 
extern OpenGLCapabilities &
OpenGL(unsigned int view_id, the_view_t * view = NULL);


//----------------------------------------------------------------
// FIXME_OPENGL
// 
#define FIXME_OPENGL(x)
#if 1
#ifndef NDEBUG
#undef FIXME_OPENGL
#define FIXME_OPENGL(x)                                         \
  {                                                             \
    int i = 0;                                                  \
    while (true)                                                \
    {                                                           \
      GLenum err = glGetError();                                \
      if (err == GL_NO_ERROR) break;                            \
      const GLubyte * str = gluErrorString(err);                \
      std::cerr << x << std::setw(3) << i << ". FIXME: "        \
                << __FILE__ << ':' << __LINE__                  \
                << ", OPENGL: " << str                          \
                << ", error " << err << std::endl;              \
      i++;                                                      \
    }                                                           \
    /* it's a quick way to die: */                              \
    static unsigned char * null = NULL;                         \
    if (false && i > 0) null[0] = 0xFF;                         \
  }
#else // NDEBUG
#define FIXME_OPENGL(x)
#endif // NDEBUG

#undef PERROR_OPENGL
#define PERROR_OPENGL(x)                                        \
  {                                                             \
    int i = 0;                                                  \
    while (true)                                                \
    {                                                           \
      GLenum err = glGetError();                                \
      if (err == GL_NO_ERROR) break;                            \
      const GLubyte * str = gluErrorString(err);                \
      std::cerr << x << std::setw(3) << i << ". FIXME: "        \
                << __FILE__ << ':' << __LINE__                  \
                << ", OPENGL: " << str                          \
                << ", error " << err << std::endl;              \
      i++;                                                      \
    }                                                           \
  }

#endif // 0


#endif // OPENGL_CAPABILITIES_HXX_
