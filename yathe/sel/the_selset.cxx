// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_selset.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sat Jan 22 13:03:00 MDT 2005
// Copyright    : (C) 2005
// License      : MIT
// Description  : A basic selection set.

// the includes:
#include <sel/the_selset.hxx>
#include <ui/the_document_ui.hxx>
#include <doc/the_document.hxx>


//----------------------------------------------------------------
// the_selset_traits_t::clone
//
the_selset_traits_t::super_t *
the_selset_traits_t::clone() const
{
  return new the_selset_traits_t(*this);
}

//----------------------------------------------------------------
// the_selset_traits_t::is_valid
//
bool
the_selset_traits_t::is_valid(const unsigned int & id) const
{
  the_primitive_t * prim = registry()->elem<the_primitive_t>(id);
  return (prim != NULL);
}

//----------------------------------------------------------------
// the_selset_traits_t::activate
//
bool
the_selset_traits_t::activate(unsigned int & id) const
{
  if (!is_valid(id))
  {
    return false;
  }

  the_primitive_t * prim = registry()->elem<the_primitive_t>(id);
  prim->set_current_state(THE_SELECTED_STATE_E);
  return true;
}

//----------------------------------------------------------------
// the_selset_traits_t::deactivate
//
bool
the_selset_traits_t::deactivate(unsigned int & id) const
{
  if (!is_valid(id))
  {
    return false;
  }

  the_primitive_t * prim = registry()->elem<the_primitive_t>(id);
  prim->clear_state(THE_SELECTED_STATE_E);
  return true;
}
