// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_pick_rec.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Apr  7 16:07:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A selection record for a user selected document primitve.

// system includes:
#include <assert.h>

// local includes:
#include "sel/the_pick_rec.hxx"
#include "opengl/the_view.hxx"
#include "utils/the_indentation.hxx"


//----------------------------------------------------------------
// the_pick_data_t::operator ==
//
bool
the_pick_data_t::operator == (const the_pick_data_t & data) const
{
  if (ref_ != data.ref_) return false;
  if (ref_ != NULL && !ref_->equal(data.ref_)) return false;
  return vol_pt_ != data.vol_pt_;
}

//----------------------------------------------------------------
// the_pick_data_t::operator <
//
bool
the_pick_data_t::operator < (const the_pick_data_t & data) const
{
  // us depth as a tie braker when the pick radius is equal:
  if (radius() == data.radius())
  {
    return depth() < data.depth();
  }

  // selections closest to the view volume axis are more acurate and
  // therefore have higher priority:
  return radius() < data.radius();
}

//----------------------------------------------------------------
// the_pick_data_t::dump
//
void
the_pick_data_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_pick_data_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "vol_pt_ = " << vol_pt_ << ";" << endl
       << INDSTR << "ref_: " << endl;
  if (ref_ == NULL) strm << ref_ << ";" << endl;
  else ref_->dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & strm, const the_pick_data_t & data)
{
  data.dump(strm);
  return strm;
}


//----------------------------------------------------------------
// the_pick_rec_t::set_current_state
//
void
the_pick_rec_t::set_current_state(the_registry_t * r,
				  the_primitive_state_t state) const
{
  the_primitive_t * prim = data_.is<the_primitive_t>(r);
  if (prim == NULL) return;

  prim->set_current_state(state);
}

//----------------------------------------------------------------
// the_pick_rec_t::remove_current_state
//
void
the_pick_rec_t::remove_current_state(the_registry_t * r,
				     the_primitive_state_t state) const
{
  the_primitive_t * prim = data_.is<the_primitive_t>(r);
  if (prim == NULL) return;

  prim->clear_state(state);
}

//----------------------------------------------------------------
// the_pick_rec_t::clear_current_state
//
void
the_pick_rec_t::clear_current_state(the_registry_t * r) const
{
  the_primitive_t * prim = data_.is<the_primitive_t>(r);
  if (prim == NULL) return;

  prim->clear_current_state();
}

//----------------------------------------------------------------
// the_pick_rec_t::dump
//
void
the_pick_rec_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_pick_rec_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl
       << INDSTR << "view_ = " << view_ << ";" << endl
       << INDSTR << "volume_ = " << volume_ << ";" << endl
       << INDSTR << "data_: ";
  data_.dump(strm, INDNXT);
  strm << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// operator <<
//
ostream &
operator << (ostream & strm, const the_pick_rec_t & pick)
{
  pick.dump(strm);
  return strm;
}
