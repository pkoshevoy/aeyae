// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_pick_rec.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Wed Apr  7 16:07:00 MDT 2004
// Copyright    : (C) 2004
// License      : MIT
// Description  : A selection record for a user selected document primitve.

#ifndef THE_PICK_REC_HXX_
#define THE_PICK_REC_HXX_

// system includes:
#include <assert.h>

// local includes:
#include "math/v3x1p3x1.hxx"
#include "math/the_view_volume.hxx"
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"

// forward declarations:
class the_view_t;


//----------------------------------------------------------------
// the_pick_data_t
//
class the_pick_data_t
{
public:
  the_pick_data_t():
    vol_pt_(FLT_MAX, FLT_MAX, FLT_MAX),
    ref_(NULL)
  {}

  the_pick_data_t(const p3x1_t & vol_pt, the_reference_t * ref):
    vol_pt_(vol_pt),
    ref_(ref)
  {}

  the_pick_data_t(const the_pick_data_t & r):
    vol_pt_(r.vol_pt_),
    ref_(r.ref_->clone())
  {}

  ~the_pick_data_t()
  {
    delete ref_;
    ref_ = NULL;
  }

  the_pick_data_t & operator = (const the_pick_data_t & r)
  {
    if (this != &r)
    {
      vol_pt_ = r.vol_pt_;
      delete ref_;
      ref_ = r.ref_->clone();
    }

    return *this;
  }

  the_pick_data_t * clone() const
  { return new the_pick_data_t(*this); }

  // required for searching and sorting:
  bool operator == (const the_pick_data_t & d) const;
  bool operator <  (const the_pick_data_t & d) const;

  // helper: distance between the point on the object that was picked
  // and the axis of the selection volume:
  inline float radius() const
  { return vol_pt_.x(); }

  // helper: distance from the near clipping face of the selection volume
  // to the far clipping face:
  inline float depth() const
  { return vol_pt_.z(); }

  // helper accessor to the id of the primitive that was picked:
  inline unsigned int id() const
  { return (ref_ == NULL) ? UINT_MAX : ref_->id(); }

  // helper accessor to the primitive:
  template <class T> T * is(the_registry_t * registry) const
  { return (ref_ == NULL) ? NULL : registry->template elem<T>(ref_->id()); }

  // accessors:
  inline const p3x1_t & vol_pt() const
  { return vol_pt_; }

  inline const the_reference_t * ref() const
  { return ref_; }

  template <class ref_t> const ref_t * ref() const
  { return dynamic_cast<const ref_t *>(ref_); }

  // for debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  // the point expressed in the view volume coordinate system where the
  // intersection occured:
  p3x1_t vol_pt_;

  // a reference to the object that was intersected:
  the_reference_t * ref_;
};

//----------------------------------------------------------------
// operator <<
//
extern ostream &
operator << (ostream & strm, const the_pick_data_t & data);


//----------------------------------------------------------------
// the_pick_rec_t
//
class the_pick_rec_t
{
public:
  // default constructor:
  the_pick_rec_t():
    view_(NULL)
  {}

  // explicit constructor:
  the_pick_rec_t(the_view_t * view,
		 const the_view_volume_t & volume,
		 const the_pick_data_t & data):
    view_(view),
    volume_(volume),
    data_(data)
  {}

  // FIXME: what about records referencing the same object
  // at different locations?
  inline bool operator == (const the_pick_rec_t & pick) const
  { return data_.id() == pick.data_.id(); }

  // impose an ordering scheme on the records:
  bool operator < (const the_pick_rec_t & pick) const
  { return data_ < pick.data_; }

  // the location of the pick point in the world coordinate system:
  inline p3x1_t pick_pt() const
  {
    p3x1_t wcs_pt;
    volume_.cyl_to_wcs(data_.vol_pt(), wcs_pt);
    return wcs_pt;
  }

  // selection appearance controls:
  void set_current_state(the_registry_t * r, the_primitive_state_t s) const;
  void remove_current_state(the_registry_t * r, the_primitive_state_t s) const;
  void clear_current_state(the_registry_t * r) const;

  // accessors:
  inline the_view_t * view() const
  { return view_; }

  inline const the_view_volume_t & volume() const
  { return volume_; }

  inline const the_pick_data_t & data() const
  { return data_; }

  // for debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  // the view in which this primitive was picked:
  the_view_t * view_;

  // the volume that was used for the pick:
  the_view_volume_t volume_;

  // the basic data about the pick:
  the_pick_data_t data_;
};

//----------------------------------------------------------------
// operator <<
//
extern ostream &
operator << (ostream & strm, const the_pick_rec_t & pick);


#endif // THE_PICK_REC_HXX_
