// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : glsl.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Nov 26 12:18:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a thin wrapper for GLEW

#ifndef GLSL_HXX_
#define GLSL_HXX_

// local includes:
#include "opengl/OpenGLCapabilities.h"

// system includes:
#include <iostream>


//----------------------------------------------------------------
// glsl_init
// 
extern bool glsl_init();

//----------------------------------------------------------------
// glsl_has_vertex_program
// 
extern bool glsl_has_vertex_program();

//----------------------------------------------------------------
// glsl_has_fragment_program
// 
extern bool glsl_has_fragment_program();

//----------------------------------------------------------------
// glsl_get_shader
//
// Compile a fragment or vertex shader and return a handle:
// 
extern GLuint
glsl_get_shader(const char * source_code, bool fragment);

//----------------------------------------------------------------
// glsl_get_vertex_shader
//
// Compile the shader and return a handle:
// 
inline GLuint glsl_get_vertex_shader(const char * source_code)
{ return glsl_get_shader(source_code, false); }

//----------------------------------------------------------------
// glsl_get_fragment_shader
// 
// Compile the shader and return a handle:
// 
inline GLuint glsl_get_fragment_shader(const char * source_code)
{ return glsl_get_shader(source_code, true); }

//----------------------------------------------------------------
// glsl_get_program
// 
// Link the shaders into a shader program and return the program handle:
// 
extern GLuint
glsl_get_program(const GLuint * shaders, unsigned int num_shaders);

//----------------------------------------------------------------
// glsl_get_vertex_program
// 
extern GLuint
glsl_get_vertex_program(const char * source_code);

//----------------------------------------------------------------
// glsl_get_fragment_program
// 
extern GLuint
glsl_get_fragment_program(const char * source_code);

//----------------------------------------------------------------
// glsl_use_program
// 
extern void
glsl_use_program(GLuint handle);

//----------------------------------------------------------------
// glsl_uni_addr
//
// return location of a uniform variable
// 
extern GLint
glsl_uni_addr(GLuint program, const char * name);

//----------------------------------------------------------------
// glsl_uni_1i
// 
extern void
glsl_uni_1i(GLint addr, GLint value);

//----------------------------------------------------------------
// glsl_uni_2f
// 
extern void
glsl_uni_2f(GLint addr, GLfloat v0, GLfloat v1);

//----------------------------------------------------------------
// glsl_uni_3f
// 
extern void
glsl_uni_3f(GLint addr, GLfloat v0, GLfloat v1, GLfloat v2);

//----------------------------------------------------------------
// glsl_texture_image_units
//
// Return the number of available texture image_units:
// 
extern GLint
glsl_texture_image_units();

//----------------------------------------------------------------
// glsl_activate_texture_unit
// 
// Set a given texture unit active:
// 
extern void
glsl_activate_texture_unit(GLint index);

//----------------------------------------------------------------
// glsl_log
// 
// dump the information log for a shader or program into a
// given stream.
// 
extern void
glsl_log(std::ostream & so, GLuint handle, bool shader);

//----------------------------------------------------------------
// glsl_shader_log
// 
inline void glsl_shader_log(std::ostream & so, GLuint handle)
{ glsl_log(so, handle, true); }

//----------------------------------------------------------------
// glsl_program_log
// 
inline void glsl_program_log(std::ostream & so, GLuint handle)
{ glsl_log(so, handle, false); }


#endif // GLSL_HXX_
