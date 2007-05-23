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
  return registry()->elem(id) != NULL;
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
  
  registry()->elem(id)->set_current_state(THE_SELECTED_STATE_E);
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
  
  registry()->elem(id)->clear_state(THE_SELECTED_STATE_E);
  return true;
}
