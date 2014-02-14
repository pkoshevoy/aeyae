// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_grid.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jan 30 14:29:00 MDT 2005
// Copyright    : (C) 2005
// License      : MIT
// Description  : A uniform grid class (4-connected vertex mesh).

#ifndef THE_GRID_HXX_
#define THE_GRID_HXX_

// local includes:
#include "doc/the_primitive.hxx"
#include "doc/the_reference.hxx"
#include "math/v3x1p3x1.hxx"
#include "math/the_aa_bbox.hxx"
#include "opengl/the_disp_list.hxx"

// system includes:
#include <list>
#include <vector>


//----------------------------------------------------------------
// row_col_t
//
typedef the_duplet_t<size_t> row_col_t;


//----------------------------------------------------------------
// the_grid_t
//
// A quadrilateral grid.
//
class the_grid_t : public the_primitive_t
{
public:
  the_grid_t();
  the_grid_t(const the_grid_t & grid);

  // virtual:
  the_primitive_t * clone() const
  { return new the_grid_t(*this); }

  // virtual:
  const char * name() const
  { return "the_grid_t"; }

  // virtual:
  bool regenerate();

  // virtual: this is used during intersection/proximity testing for selection:
  bool intersect(const the_view_volume_t & volume,
		 std::list<the_pick_data_t> & data) const;

  // virtual:
  the_dl_elem_t * dl_elem() const;

  // setup a 2x2 grid with the following upper-left lower-right corners:
  void init(const p3x1_t & ul = p3x1_t(0.0, 1.0, 0.0),
	    const p3x1_t & lr = p3x1_t(1.0, 0.0, 0.0));

  // extend the number of rows/colums in the grid by inserting it internally:
  bool insert_row(const float & v);
  bool insert_col(const float & u);

  bool remove_row(const size_t & row);
  bool remove_col(const size_t & col);

  size_t closest_row(const float & v) const;
  size_t closest_col(const float & u) const;

  // double the number of quads per row/column:
  void row_more_quads();
  void col_more_quads();

  // reduce the number of quads per row/column in half:
  void row_less_quads();
  void col_less_quads();

  // extend/contract the grid by adding/removing rows/columns at the boundary:
  void resize_quads(const p3x1_t & from, const p3x1_t & to);

  // displace the actuve grid points from the anchor positions be a
  // given drag vector:
  void move(const v3x1_t & drag_wcs);

  // adjust absolute coordinate of active points:
  void move_absolute(const unsigned int & coord, const float & value);

  // displace one grid point:
  void move(const row_col_t & rc, const v3x1_t & drag_wcs);

  // anchor managment:
  void set_anchor() { anchor_ = grid_; }

  // check whether the given row/column/point is active:
  bool is_active(const row_col_t & rc) const;

  // activate the given row/column/point:
  void activate(const row_col_t & rc);
  void deactivate(const row_col_t & rc);
  void activate_all_points();
  void deactivate_all_points();

  // check whether the grid has any active points:
  bool has_active_points() const;

  // calculate the bounding box of the grid:
  void calc_bbox(the_aa_bbox_t & bbox) const;

  // check whether this grid is singular (all points are the same):
  bool is_singular() const;

  // accessors:
  inline const std::vector<std::vector<p3x1_t> > & grid() const
  { return grid_; }

  inline const std::vector<std::vector<bool> > & active() const
  { return active_; }

  inline const std::vector<float> & u() const
  { return u_; }

  inline const std::vector<float> & v() const
  { return v_; }

  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);

private:
  std::vector<std::vector<p3x1_t> > anchor_;
  std::vector<std::vector<p3x1_t> > grid_;
  std::vector<std::vector<bool> > active_;

  std::vector<float> u_;
  std::vector<float> v_;

  mutable the_disp_list_t dl_;
};


//----------------------------------------------------------------
// the_grid_ref_t
//
class the_grid_ref_t : public the_reference_t
{
public:
  the_grid_ref_t(const unsigned int & id,
		 const row_col_t & row_col,
		 const p2x1_t & uv);

  // virtual: a method for cloning references (potential memory leak):
  the_grid_ref_t * clone() const
  { return new the_grid_ref_t(*this); }

  // virtual:
  const char * name() const
  { return "the_grid_ref_t"; }

  // virtual: calculate the 3D value of this reference:
  bool eval(the_registry_t * registry, p3x1_t & wcs_pt) const;

  // virtual: if possible, adjust this reference:
  bool move(the_registry_t * registry,
	    const the_view_mgr_t & view_mgr,
	    const p3x1_t & wcs_pt);

  // virtual: if possible, adjust this reference:
  bool reparameterize(the_registry_t * registry,
                      const p3x1_t & wcs_pt);

  // virtual: Equality test:
  bool equal(const the_reference_t * ref) const;

  // virtual:
  the_point_symbol_id_t symbol() const
  { return THE_CROSS_SYMBOL_E; }

  // virtual: For debugging:
  void dump(ostream & strm, unsigned int indent = 0) const;

  // accessors:
  inline const row_col_t & row_col() const
  { return row_col_; }

  inline void set_param(const row_col_t & row_col)
  { row_col_ = row_col; }

  // virtual: file io:
  bool save(std::ostream & stream) const;
  bool load(std::istream & stream);

protected:
  the_grid_ref_t();

  // the row/column at which the grid is being referenced:
  row_col_t row_col_;

  // the parameter coordinates of this reference:
  p2x1_t uv_;
};


#endif // THE_GRID_HXX_
