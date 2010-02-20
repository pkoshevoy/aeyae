// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_gl_context.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Jan 31 11:07:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : An OpenGL rendering context wrapper class.

#ifndef THE_GL_CONTEXT_HXX_
#define THE_GL_CONTEXT_HXX_

// forward declaration:
class the_view_t;


//----------------------------------------------------------------
// the_gl_context_t
// 
// Abstract class for toolkit-independent opengl context API.
// A subclass must be made for each toolkit.
// 
class the_gl_context_t
{
public:
  the_gl_context_t(the_view_t * view = NULL);
  ~the_gl_context_t();
  
  void make_current();
  void done_current();
  bool is_valid() const;
  void invalidate();
  
  static the_gl_context_t current();
  
protected:
  the_view_t * view_;
};


#endif // THE_GL_CONTEXT_HXX_
