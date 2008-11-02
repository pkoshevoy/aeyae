// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: t -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

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


// File         : the_primitive.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Implementation of the document primitive object.

// local includes:
#include "doc/the_primitive.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_utils.hxx"

#ifndef NOUI
#include "opengl/the_appearance.hxx"
#endif // NOUI 


//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & strm, the_primitive_state_t state)
{
  switch (state)
  {
    case THE_REGULAR_STATE_E:	return strm << "THE_REGULAR_STATE_E";
    case THE_HILITED_STATE_E:	return strm << "THE_HILITED_STATE_E";
    case THE_SELECTED_STATE_E:	return strm << "THE_SELECTED_STATE_E";
    case THE_FAILED_STATE_E:	return strm << "THE_FAILED_STATE_E";
      
    default:
      assert(false);
  }
  
  return strm;
}


//----------------------------------------------------------------
// the_primitive_t::the_primitive_t
// 
the_primitive_t::the_primitive_t():
  registry_(NULL),
  id_(UINT_MAX),
  regenerated_(false)
{}

//----------------------------------------------------------------
// the_primitive_t::the_primitive_t
// 
the_primitive_t::the_primitive_t(const the_primitive_t & primitive):
  registry_(primitive.registry_),
  id_(primitive.id_),
  regenerated_(primitive.regenerated_),
  current_state_(primitive.current_state_),
  direct_dependents_(primitive.direct_dependents_),
  direct_supporters_(primitive.direct_supporters_)
{}

//----------------------------------------------------------------
// the_primitive_t::~the_primitive_t
// 
the_primitive_t::~the_primitive_t()
{}

//----------------------------------------------------------------
// the_primitive_t::added_to_the_registry
// 
void
the_primitive_t::added_to_the_registry(the_registry_t * registry,
				       const unsigned int & id)
{
  // sanity check:
  assert(id_ == UINT_MAX);
  assert(direct_dependents_.size() == 0);
  
  registry_ = registry;
  id_ = id;
}

//----------------------------------------------------------------
// the_primitive_t::removed_from_the_registry
// 
void
the_primitive_t::removed_from_the_registry()
{
  while (direct_supporters_.size() != 0)
  {
    breakdown_supporter_dependent(registry_,
				  direct_supporters_.back(),
				  id_);
  }
  
  while (direct_dependents_.size() != 0)
  {
    unsigned int dep_id = direct_dependents_.front();
    the_primitive_t * dep = primitive(dep_id);
    registry_->del(dep);
    delete dep;
  }
  
  registry_ = NULL;
  id_ = UINT_MAX;
}

//----------------------------------------------------------------
// the_primitive_t::supports
// 
// check wether this primitive supports a given primitive:
bool
the_primitive_t::supports(const unsigned int & id) const
{
  // obviously, each primitive supports itself ;-)
  if (id == id_) return true;
  
  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    const unsigned int & dep_id = *i;
    the_primitive_t * p = primitive(dep_id);
    if (p->supports(id)) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_primitive_t::dependents
// 
void
the_primitive_t::dependents(std::list<unsigned int> & dependents) const
{
  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    dependents.push_back(*i);
    primitive(*i)->dependents(dependents);
  }
}

//----------------------------------------------------------------
// the_primitive_t::supporters
// 
void
the_primitive_t::supporters(std::list<unsigned int> & supporters) const
{
  for (std::list<unsigned int>::const_iterator i = direct_supporters_.begin();
       i != direct_supporters_.end(); ++i)
  {
    supporters.push_back(*i);
    primitive(*i)->supporters(supporters);
  }
}

//----------------------------------------------------------------
// the_primitive_t::request_regeneration
// 
void
the_primitive_t::request_regeneration()
{
  regenerated_ = false;
  
  for (std::list<unsigned int>::const_iterator i = direct_dependents_.begin();
       i != direct_dependents_.end(); ++i)
  {
    primitive(*i)->request_regeneration();
  }
}

#ifndef NOUI
//----------------------------------------------------------------
// the_primitive_t::color
// 
the_color_t
the_primitive_t::color() const
{
  return THE_APPEARANCE.palette().point()[current_state()];
}
#endif // NOUI

//----------------------------------------------------------------
// the_primitive_t::save
// 
bool
the_primitive_t::save(std::ostream & stream) const
{
  ::save(stream, the_primitive_t::id_);
  ::save(stream, the_primitive_t::direct_dependents_);
  ::save(stream, the_primitive_t::direct_supporters_);
  return true;
}

//----------------------------------------------------------------
// the_primitive_t::load
// 
bool
the_primitive_t::load(std::istream & stream)
{
  registry_ = NULL;
  regenerated_ = false;
  
  ::load(stream, the_primitive_t::id_);
  ::load(stream, the_primitive_t::direct_dependents_);
  ::load(stream, the_primitive_t::direct_supporters_);
  return true;
}

//----------------------------------------------------------------
// the_primitive_t::dump
// 
void
the_primitive_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_primitive_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "registry_ = " << registry_ << endl
       << INDSTR << "id_ = " << id_ << endl
       << INDSTR << "regenerated_ = " << regenerated_ << endl
       << INDSTR << "current_state_ = " << endl
       << INDSTR << "direct_dependents_ = " << direct_dependents_ << endl
       << INDSTR << "direct_supporters_ = " << direct_supporters_ << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & s, const the_primitive_t & p)
{
  p.dump(s);
  return s;
}

//----------------------------------------------------------------
// establish_supporter_dependent
// 
void
establish_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id)
{
  the_primitive_t * supporter = registry->elem(supporter_id);
  if (supporter == NULL) return;
  
  the_primitive_t * dependent = registry->elem(dependent_id);
  if (dependent == NULL) return;
  
  supporter->add_dependent(dependent_id);
  dependent->add_supporter(supporter_id);
}

//----------------------------------------------------------------
// breakdown_supporter_dependent
// 
void
breakdown_supporter_dependent(the_registry_t * registry,
			      const unsigned int & supporter_id,
			      const unsigned int & dependent_id)
{
  the_primitive_t * supporter = registry->elem(supporter_id);
  if (supporter == NULL) return;
  
  the_primitive_t * dependent = registry->elem(dependent_id);
  if (dependent == NULL) return;
  
  supporter->del_dependent(dependent_id);
  dependent->del_supporter(supporter_id);
}
