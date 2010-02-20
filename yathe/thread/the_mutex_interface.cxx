// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_mutex_interface.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Feb 21 08:22:00 MST 2007
// Copyright    : (C) 2007
// License      : MIT
// Description  : An abstract mutex class interface.

// local includes:
#include "thread/the_mutex_interface.hxx"

// system includes:
#include <stddef.h>


//----------------------------------------------------------------
// the_mutex_interface_t::creator_
// 
the_mutex_interface_t::creator_t
the_mutex_interface_t::creator_ = NULL;

//----------------------------------------------------------------
// the_mutex_interface_t::~the_mutex_interface_t
// 
the_mutex_interface_t::~the_mutex_interface_t()
{}

//----------------------------------------------------------------
// the_mutex_interface_t::set_creator
// 
void
the_mutex_interface_t::set_creator(the_mutex_interface_t::creator_t creator)
{
  creator_ = creator;
}

//----------------------------------------------------------------
// the_mutex_interface_t::create
// 
the_mutex_interface_t *
the_mutex_interface_t::create()
{
  if (creator_ == NULL) return NULL;
  return creator_();
}
