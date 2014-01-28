// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : glsl.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 26 12:20:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a thin wrapper for GLEW

// GLEW includes:
#define GLEW_STATIC 1
#include <GL/glew.h>

// local includes:
#include "opengl/glsl.hxx"
#include "utils/the_text.hxx"

// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// glsl_init
//
bool
glsl_init()
{
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    cerr << "GLEW init failed: " << glewGetErrorString(err) << endl;
    return false;
  }

  return true;
}

//----------------------------------------------------------------
// glsl_has_vertex_program
//
bool
glsl_has_vertex_program()
{
  return GLEW_VERSION_2_0 == GL_TRUE;
}

//----------------------------------------------------------------
// glsl_has_fragment_program
//
bool
glsl_has_fragment_program()
{
  return GLEW_VERSION_2_0 == GL_TRUE;
}

//----------------------------------------------------------------
// glsl_get_shader
//
GLuint
glsl_get_shader(const char * source_code, bool fragment)
{

#ifndef NDEBUG
  cerr << "compiling shader: " << endl << source_code << endl;
#endif

  GLuint handle =
    fragment
    ? glCreateShader(GL_FRAGMENT_SHADER)
    : glCreateShader(GL_VERTEX_SHADER);
  FIXME_OPENGL("glCreateShader");

  if (handle != 0)
  {
    glShaderSource(handle, 1, &source_code, NULL);
    FIXME_OPENGL("glShaderSource");

    // compile the shader source code:
    glCompileShader(handle);

    GLint shader_compiled;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &shader_compiled);

    if (shader_compiled == GL_FALSE)
    {
      glsl_shader_log(cerr, handle);
      FIXME_OPENGL("glCompileShader");

      glDeleteShader(handle);
      handle = 0;
    }
  }

  return handle;
}

//----------------------------------------------------------------
// glsl_get_program
//
GLuint
glsl_get_program(const GLuint * shaders, unsigned int num_shaders)
{
  GLuint handle = glCreateProgram();
  FIXME_OPENGL("glCreateProgram");

  if (handle != 0)
  {
    for (unsigned int i = 0; i < num_shaders; i++)
    {
      glAttachShader(handle, shaders[i]);
      FIXME_OPENGL(the_text_t("glAttachShader ") + the_text_t::number(i));
    }

    // link the shaders into an executable:
    glLinkProgram(handle);

    GLint program_linked;
    glGetProgramiv(handle, GL_LINK_STATUS, &program_linked);

    if (program_linked == GL_FALSE)
    {
      glsl_program_log(cerr, handle);
      FIXME_OPENGL("glLinkProgram");

      glDeleteProgram(handle);
      handle = 0;
    }
  }

  return handle;
}

//----------------------------------------------------------------
// glsl_get_vertex_program
//
GLuint
glsl_get_vertex_program(const char * source_code)
{
  if (!glsl_has_vertex_program())
  {
    return 0;
  }

  GLuint shader_handle = glsl_get_vertex_shader(source_code);
  if (shader_handle == 0)
  {
    return 0;
  }

  GLuint handle = glsl_get_program(&shader_handle, 1);
  glDeleteShader(shader_handle);

  return handle;
}

//----------------------------------------------------------------
// glsl_get_fragment_program
//
GLuint
glsl_get_fragment_program(const char * source_code)
{
  if (!glsl_has_fragment_program())
  {
    return 0;
  }

  GLuint shader_handle = glsl_get_fragment_shader(source_code);
  if (shader_handle == 0)
  {
    return 0;
  }

  GLuint handle = glsl_get_program(&shader_handle, 1);
  glDeleteShader(shader_handle);

  return handle;
}

//----------------------------------------------------------------
// glsl_use_program
//
void
glsl_use_program(GLuint handle)
{
  glUseProgram(handle);
  FIXME_OPENGL("glsl_use_program");
}

//----------------------------------------------------------------
// glsl_uni_addr
//
GLint
glsl_uni_addr(GLuint program, const char * name)
{
  GLint addr = glGetUniformLocation(program, name);
  if (addr == -1)
  {
    cerr << "uniform named \"" << name << "\" not found" << endl;
  }

  FIXME_OPENGL(the_text_t("glsl_uni_addr ") + name);
  return addr;
}

//----------------------------------------------------------------
// glsl_uni_1i
//
void
glsl_uni_1i(GLint addr, GLint value)
{
  glUniform1i(addr, value);
}

//----------------------------------------------------------------
// glsl_uni_2f
//
void
glsl_uni_2f(GLint addr, GLfloat v0, GLfloat v1)
{
  glUniform2f(addr, v0, v1);
}

//----------------------------------------------------------------
// glsl_uni_3f
//
void
glsl_uni_3f(GLint addr, GLfloat v0, GLfloat v1, GLfloat v2)
{
  glUniform3f(addr, v0, v1, v2);
}


//----------------------------------------------------------------
// glsl_texture_image_units
//
GLint
glsl_texture_image_units()
{
  GLint n_units = 0;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &n_units);
  FIXME_OPENGL("glsl_texture_image_units");
  return n_units;
}

//----------------------------------------------------------------
// glsl_activate_texture_unit
//
// Set a given texture unit active:
//
void
glsl_activate_texture_unit(GLint index)
{
  glActiveTexture(GL_TEXTURE0 + index);
  FIXME_OPENGL("glActiveTexture");
}

//----------------------------------------------------------------
// glsl_log
//
void
glsl_log(std::ostream & so, GLuint handle, bool shader)
{
  GLint buffer_size = 0;
  the_text_t info;

  if (shader)
  {
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &buffer_size);
  }
  else
  {
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &buffer_size);
  }

  if (buffer_size <= 0) return;

  info.fill('\0', buffer_size);

  if (shader)
  {
    glGetShaderInfoLog(handle, buffer_size, NULL, &(info[0]));
    so << "shader log: " << endl << info << endl;
  }
  else
  {
    glGetProgramInfoLog(handle, buffer_size, NULL, &(info[0]));
    so << "program log: " << endl << info << endl;
  }
}
