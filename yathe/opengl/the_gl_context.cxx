// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_gl_context.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Fri Feb 9 22:10:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : An OpenGL rendering context wrapper class.

// system includes:
#include <stddef.h>

// local includes:
#include "opengl/the_gl_context.hxx"


//----------------------------------------------------------------
// the_gl_context_interface_t::current_
// 
the_gl_context_interface_t *
the_gl_context_interface_t::current_ = NULL;


//----------------------------------------------------------------
// the_gl_context_t::the_gl_context_t
// 
the_gl_context_t::the_gl_context_t(the_gl_context_interface_t * context):
  context_(context)
{}

//----------------------------------------------------------------
// the_gl_context_t::make_current
// 
void
the_gl_context_t::make_current()
{
  if (!context_)
  {
    return;
  }

  context_->gl_make_current();
}

//----------------------------------------------------------------
// the_gl_context_t::done_current
// 
void
the_gl_context_t::done_current()
{
  if (!context_)
  {
    return;
  }
  
  context_->gl_done_current();
}

//----------------------------------------------------------------
// the_gl_context_t::is_valid
// 
bool
the_gl_context_t::is_valid() const
{
  if (!context_)
  {
    return false;
  }
  
  return context_->gl_context_is_valid();
}

//----------------------------------------------------------------
// the_gl_context_t::invalidate
// 
void
the_gl_context_t::invalidate()
{
  context_ = NULL;
}

//----------------------------------------------------------------
// the_gl_context_t::current
// 
the_gl_context_t
the_gl_context_t::current()
{
  return the_gl_context_t(the_gl_context_interface_t::current_);
}
