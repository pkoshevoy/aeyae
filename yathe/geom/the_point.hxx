// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_point.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The 3D point primitive.

#ifndef THE_POINT_HXX_
#define THE_POINT_HXX_

// system includes:
#include <list>

// local includes:
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "sel/the_pick_filter.hxx"
#include "math/v3x1p3x1.hxx"
#include "opengl/the_point_symbols.hxx"


//----------------------------------------------------------------
// the_point_t
//
// Base class for the Vertex datatypes:
class the_point_t : public the_primitive_t
{
public:
  the_point_t():
    anchor_(FLT_MAX, FLT_MAX, FLT_MAX),
    weight_(1.0)
  {}

  // accessors to the weight associated with this point:
  inline const float & weight() const
  { return weight_; }

  inline void set_weight(const float & weight)
  {
    weight_ = weight;
    the_graph_node_t::request_regeneration(this);
  }

  // accessor to the Euclidian coordinates of this point:
  virtual const p3x1_t & value() const = 0;

  // these functions can be used to move the point:
  virtual bool set_value(const the_view_mgr_t & view_mgr,
			 const p3x1_t & wcs_pt) = 0;

  inline bool move(const the_view_mgr_t & view_mgr,
		   const v3x1_t & wcs_vec)
  { return set_value(view_mgr, anchor_ + wcs_vec); }

  // anchor point managment:
  void set_anchor()
  { anchor_ = value(); }

  const p3x1_t & anchor() const
  { return anchor_; }

  // return the symbol used to display this point:
  virtual the_point_symbol_id_t symbol() const = 0;

  // virtual: this is used during intersection/proximity testing:
  bool intersect(const the_view_volume_t & volume,
		 std::list<the_pick_data_t> & data) const;

  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);

  // virtual: For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;

protected:
  p3x1_t anchor_;
  float weight_;
};


//----------------------------------------------------------------
// the_hard_point_t
//
class the_hard_point_t : public the_point_t
{
public:
  the_hard_point_t():
    the_point_t(),
    value_(0.0, 0.0, 0.0)
  {}

  the_hard_point_t(const p3x1_t & v):
    the_point_t(),
    value_(v)
  {}

  // virtual:
  the_primitive_t * clone() const
  { return new the_hard_point_t(*this); }

  // virtual:
  const char * name() const
  { return "the_hard_point_t"; }

  // virtual:
  bool regenerate()
  { return true; }

  const p3x1_t & value() const
  { return value_; }

  // virtual:
  bool set_value(const the_view_mgr_t & view_mgr,
		 const p3x1_t & wcs_pt);

  // virtual:
  the_point_symbol_id_t symbol() const
  { return THE_SMALL_FILLED_CIRCLE_SYMBOL_E; }

  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);

  // virtual: For debugging, dumps the value
  void dump(ostream & strm, unsigned int indent = 0) const;

private:
  p3x1_t value_;
};


//----------------------------------------------------------------
// the_soft_point_t
//
// Soft Vertex datatype:
class the_soft_point_t : public the_point_t
{
public:
  the_soft_point_t();
  the_soft_point_t(const the_reference_t & ref);
  the_soft_point_t(const the_soft_point_t & point);
  ~the_soft_point_t();

  // virtual:
  the_primitive_t * clone() const
  { return new the_soft_point_t(*this); }

  // virtual:
  const char * name() const
  { return "the_soft_point_t"; }

  // virtual:
  void added_to_the_registry(the_registry_t * registry,
			     const unsigned int & id);

  // accessor to the stored reference:
  inline the_reference_t * ref() const
  { return ref_; }

  // virtual:
  bool regenerate();

  const p3x1_t & value() const
  { return value_; }

  // virtual:
  bool set_value(const the_view_mgr_t & view_mgr,
		 const p3x1_t & wcs_pt);

  // virtual:
  the_point_symbol_id_t symbol() const;

  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);

  // virtual: For debugging, dumps the value:
  void dump(ostream & strm, unsigned int indent = 0) const;

private:
  // the reference (the part that makes this point soft):
  the_reference_t * ref_;

  // cached value of the point:
  p3x1_t value_;
};


//----------------------------------------------------------------
// the_point_ref_t
//
// Reference to a vertex:
class the_point_ref_t : public the_reference_t
{
public:
  the_point_ref_t(unsigned int id = ~0);

  // virtual: a method for cloning references (potential memory leak):
  the_reference_t * clone() const
  { return new the_point_ref_t(*this); }

  // virtual:
  const char * name() const
  { return "the_point_ref_t"; }

  // virtual: Calculate the 3D value of this reference:
  bool eval(the_registry_t * registry, p3x1_t & pt) const;

  // virtual:
  bool move(the_registry_t * /* registry */,
	    const the_view_mgr_t & /* view_mgr */,
	    const p3x1_t & /* wcs_pt */)
  { return false; }

  // virtual:
  the_point_symbol_id_t symbol() const
  { return THE_CORNERS_SYMBOL_E; }

  // virtual: For debugging, dumps the seg:
  void dump(ostream & strm, unsigned int indent = 0) const;
};


//----------------------------------------------------------------
// the_point_pick_filter_t
//
class the_point_pick_filter_t : public the_pick_filter_t
{
public:
  // virtual:
  bool allow(const the_registry_t * registry, const unsigned int & id) const
  {
    the_point_t * point = registry->elem<the_point_t>(id);
    return (point != NULL);
  }
};


#endif // THE_POINT_HXX_
