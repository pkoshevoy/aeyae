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


// File         : the_grid.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jan 30 14:31:00 MDT 2005
// Copyright    : (C) 2005
// License      : MIT
// Description  : A uniform grid class (4-connected vertex mesh).

// local includes:
#include "geom/the_grid.hxx"
#include "geom/the_polyline.hxx"
#include "math/the_aa_bbox.hxx"
#include "opengl/the_view_mgr.hxx"
#include "sel/the_pick_rec.hxx"


//----------------------------------------------------------------
// the_grid_t::the_grid_t
// 
the_grid_t::the_grid_t():
  the_primitive_t()
{
  init();
}

//----------------------------------------------------------------
// the_grid_t::regenerate
// 
bool
the_grid_t::regenerate()
{
  regenerated_ = true;
  return regenerated_;
}

//----------------------------------------------------------------
// the_grid_t::intersect
// 
bool
the_grid_t::intersect(const the_view_volume_t & volume,
		      std::list<the_pick_data_t> & data) const
{
  the_polyline_geom_t polyline;
  
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  // find the closest row:
  unsigned int closest_row = UINT_MAX;
  the_deviation_min_t row_s_rs(FLT_MAX, FLT_MAX);
  
  for (unsigned int i = 0; i < rows; i++)
  {
    polyline.reset(grid_[i], u_, u_);
    
    std::list<the_deviation_min_t> minima;
    if (polyline.intersect(volume, minima))
    {
      const the_deviation_min_t & s_rs = *(minima.begin());
      if (s_rs.r_ < row_s_rs.r_)
      {
	row_s_rs = s_rs;
	closest_row = i;
      }
    }
  }
  
  // find the closest column:
  std::vector<p3x1_t> column(rows);
  unsigned int closest_col = UINT_MAX;
  the_deviation_min_t col_s_rs(FLT_MAX, FLT_MAX);
  
  for (unsigned int j = 0; j < cols; j++)
  {
    for (unsigned int i = 0; i < rows; i++)
    {
      column[i] = grid_[i][j];
    }
    
    polyline.reset(column, v_, v_);
    
    std::list<the_deviation_min_t> minima;
    if (polyline.intersect(volume, minima))
    {
      const the_deviation_min_t & s_rs = *(minima.begin());
      if (s_rs.r_ < col_s_rs.r_)
      {
	col_s_rs = s_rs;
	closest_col = j;
      }
    }
  }
  
  // check for failed selection:
  if (closest_row == UINT_MAX && closest_col == UINT_MAX) return false;
  
  p3x1_t wcs_pt;
  if (row_s_rs.r_ <= col_s_rs.r_)
  {
    polyline.reset(grid_[closest_row], u_, u_);
    polyline.position(row_s_rs.s_, wcs_pt);
  }
  else
  {
    for (unsigned int i = 0; i < rows; i++)
    {
      column[i] = grid_[i][closest_col];
    }
    
    polyline.reset(column, v_, v_);
    polyline.position(col_s_rs.s_, wcs_pt);
  }
  
  p3x1_t cyl_pt;
  volume.wcs_to_cyl(wcs_pt, cyl_pt);

  the_grid_ref_t * ref =
    new the_grid_ref_t(id(),
		       row_col_t(closest_row,
				 closest_col),
		       p2x1_t(col_s_rs.s_,
			      row_s_rs.s_));
  
  data.push_back(the_pick_data_t(cyl_pt, ref));
  return true;
}

//----------------------------------------------------------------
// the_grid_t::dl_elem
// 
the_dl_elem_t *
the_grid_t::dl_elem() const
{
  dl_.clear();
  
  // mimic the polyline behaviour:
  the_color_t c(THE_APPEARANCE.palette().curve()[current_state()]);
  /*
  c += the_color_t::WHITE;
  c *= 0.5;
  */
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  // draw the rows:
  for (unsigned int i = 0; i < rows; i++)
  {
    const std::vector<p3x1_t> & row = grid_[i];
    for (unsigned int j = 1; j < cols; j++)
    {
      dl_.push_back(new the_line_dl_elem_t(row[j - 1], row[j], c));
    }
  }
  
  // draw the columns:
  for (unsigned int i = 1; i < rows; i++)
  {
    for (unsigned int j = 0; j < cols; j++)
    {
      dl_.push_back(new the_line_dl_elem_t(grid_[i - 1][j], grid_[i][j], c));
    }
  }
  
  // draw the points:
  if (has_state(THE_SELECTED_STATE_E))
  {
    for (unsigned int i = 0; i < rows; i++)
    {
      for (unsigned int j = 0; j < cols; j++)
      {
	the_primitive_state_t state =
	  active_[i][j] ? THE_SELECTED_STATE_E : THE_REGULAR_STATE_E;
	
	dl_.push_back(new the_symbol_dl_elem_t
		      (grid_[i][j],
		       THE_APPEARANCE.palette().point()[state],
		       THE_POINT_SYMBOLS,
		       THE_SMALL_FILLED_CIRCLE_SYMBOL_E));
      }
    }
  }
  
  return new the_disp_list_dl_elem_t(dl_);
}

//----------------------------------------------------------------
// the_grid_t::init
// 
void
the_grid_t::init(const p3x1_t & ul, const p3x1_t & lr)
{
  u_.resize(2);
  v_.resize(2);
  resize(grid_, 2, 2);
  resize(active_, 2, 2);
  
  u_[0] = 0.0;
  u_[1] = 1.0;
  v_[0] = 0.0;
  v_[1] = 1.0;
  
  the_aa_bbox_t bbox;
  bbox << ul << lr;
  
  if (bbox.is_singular())
  {
    grid_[0][0] = ul;
    grid_[0][1] = ul;
    grid_[1][0] = ul;
    grid_[1][1] = ul;
  }
  else if (bbox.is_linear())
  {
    grid_[0][0] = lr;
    grid_[0][1] = ul;
    grid_[1][0] = lr;
    grid_[1][1] = ul;
  }
  else
  {
    unsigned int sd = bbox.smallest_dimension();
    unsigned int d1 = bbox.largest_dimension();
    unsigned int d2 = 3 - d1 - sd;
    
    grid_[0][0] = lr;
    grid_[0][1] = ul;
    grid_[1][0] = lr;
    grid_[1][1] = ul;
    
    grid_[0][0][d1] = ul[d1];
    grid_[0][0][d2] = lr[d2];
    
    grid_[1][1][d1] = lr[d1];
    grid_[1][1][d2] = ul[d2];
  }
  
  active_[0][0] = false;
  active_[0][1] = false;
  active_[1][0] = false;
  active_[1][1] = false;
  
  regenerated_ = false;
}

//----------------------------------------------------------------
// find_segment_index
// 
static unsigned int
find_containing_segment(const std::vector<float> & data,
			const float & t,
			const float & eps = THE_EPSILON)
{
  const unsigned int & n = data.size();
  for (unsigned int i = 1; i < n; i++)
  {
    if ((t - data[i - 1] > eps) && (data[i] - t > eps))
    {
      return i - 1;
    }
  }
  
  return UINT_MAX;
}

//----------------------------------------------------------------
// ab_average
// 
// ab = a + t * (b - a);
// 
static void
interpolate(const std::vector<p3x1_t> & a,
	    const std::vector<p3x1_t> & b,
	    const float & t,
	    std::vector<p3x1_t> & ab)
{
  const unsigned int & n = a.size();
  assert(n == b.size());
  
  ab.resize(n);
  for (unsigned int i = 0; i < n; i++)
  {
    ab[i] = a[i] + t * (b[i] - a[i]);
  }
}

//----------------------------------------------------------------
// get_active_cols
// 
static void
get_active_cols(const std::vector<std::vector<bool> > & active,
		std::vector<bool> & active_cols)
{
  active_cols = active[0];
  
  const unsigned int & rows = active.size();
  for (unsigned int i = 1; i < rows; i++)
  {
    const std::vector<bool> & row = active[i];
    const unsigned int & cols = row.size();
    for (unsigned int j = 0; j < cols; j++)
    {
      active_cols[j] = active_cols[j] && row[j];
    }
  }
}

//----------------------------------------------------------------
// get_active_rows
// 
static void
get_active_rows(const std::vector<std::vector<bool> > & active,
		std::vector<bool> & active_rows)
{
  const unsigned int & rows = active.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    active_rows[i] = active[i][0];
    
    const std::vector<bool> & row = active[i];
    const unsigned int & cols = row.size();
    for (unsigned int j = 1; j < cols; j++)
    {
      active_rows[i] = active_rows[i] && row[j];
    }
  }
}

//----------------------------------------------------------------
// assign_value_to_row
// 
template <class T>
static void
assign_value_to_row(std::vector<std::vector<T> > & grid,
		    const unsigned int & row_index,
		    const T & value)
{
  std::vector<T> & row = grid[row_index];
  const unsigned int & cols = row.size();
  for (unsigned int i = 0; i < cols; i++)
  {
    row[i] = value;
  }
}

//----------------------------------------------------------------
// assign_value_to_col
// 
template <class T>
static void
assign_value_to_col(std::vector<std::vector<T> > & grid,
		    const unsigned int & col_index,
		    const T & value)
{
  const unsigned int & rows = grid.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    grid[i][col_index] = value;
  }
}

//----------------------------------------------------------------
// the_grid_t::insert_row
// 
bool
the_grid_t::insert_row(const float & v)
{
  unsigned int seg = find_containing_segment(v_, v);
  if (seg == UINT_MAX) return false;
  
  v_.insert(v_.begin() + seg + 1, v);
  const float t = (v - v_[seg]) / (v_[seg + 2] - v_[seg]);
  
  grid_.insert(grid_.begin() + seg + 1, std::vector<p3x1_t>(0));
  interpolate(grid_[seg], grid_[seg + 2], t, grid_[seg + 1]);
  
  std::vector<bool> active_cols;
  get_active_cols(active_, active_cols);
  active_.insert(active_.begin() + seg + 1, std::vector<bool>(0));
  active_[seg + 1] = active_cols;
  
  return true;
}

//----------------------------------------------------------------
// the_grid_t::insert_col
// 
bool
the_grid_t::insert_col(const float & u)
{
  unsigned int seg = find_containing_segment(u_, u);
  if (seg == UINT_MAX) return false;
  
  u_.insert(u_.begin() + seg + 1, u);
  const float t = (u - u_[seg]) / (u_[seg + 2] - u_[seg]);
  
  std::vector<bool> active_rows;
  get_active_rows(active_, active_rows);
  
  const unsigned int & rows = grid_.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    grid_[i].insert(grid_[i].begin() + seg + 1, p3x1_t());
    const p3x1_t & a = grid_[i][seg];
    const p3x1_t & b = grid_[i][seg + 2];
    grid_[i][seg + 1] = a + t * (b - a);
    
    active_[i].insert(active_[i].begin() + seg + 1, active_rows[i]);
  }
  
  return true;
}

//----------------------------------------------------------------
// the_grid_t::remove_row
// 
bool
the_grid_t::remove_row(const unsigned int & row)
{
  const unsigned int old_rows = v_.size();
  if (old_rows < 3) return false;
  
  active_.erase(active_.begin() + row);
  grid_.erase(grid_.begin() + row);
  v_.erase(v_.begin() + row);
  return true;
}

//----------------------------------------------------------------
// the_grid_t::remove_col
// 
bool
the_grid_t::remove_col(const unsigned int & col)
{
  const unsigned int old_cols = u_.size();
  if (old_cols < 3) return false;
  
  const unsigned int & rows = v_.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    active_[i].erase(active_[i].begin()+ col);
    grid_[i].erase(grid_[i].begin() + col);
  }
  u_.erase(u_.begin() + col);
  return false;
}

//----------------------------------------------------------------
// closest_elem
// 
static unsigned int
closest_elem(const std::vector<float> & data, const float & v)
{
  unsigned int i_min = UINT_MAX;
  float d_min = FLT_MAX;
  
  const unsigned int & rows = data.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    float d = fabs(data[i] - v);
    if (d < d_min)
    {
      i_min = i;
      d_min = d;
    }
    
    if (d > d_min) break;
  }
  
  return i_min;
}

//----------------------------------------------------------------
// the_grid_t::closest_row
// 
unsigned int
the_grid_t::closest_row(const float & v) const
{
  return closest_elem(v_, v);
}

//----------------------------------------------------------------
// the_grid_t::closest_col
// 
unsigned int
the_grid_t::closest_col(const float & u) const
{
  return closest_elem(u_, u);
}

//----------------------------------------------------------------
// the_grid_t::row_more_quads
//
// increase the number of columns per row:
// 
void
the_grid_t::row_more_quads()
{
  const unsigned int old_cols = u_.size();
  const unsigned int new_cols = old_cols * 2 - 1;
  std::vector<p3x1_t> new_row(new_cols);
  std::vector<bool>   new_act(new_cols);
  
  const unsigned int & rows = grid_.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    std::vector<p3x1_t> & old_row = grid_[i];
    std::vector<bool> &   old_act = active_[i];
    
    for (unsigned int j = 0; j < old_cols; j++)
    {
      new_row[j * 2] = old_row[j];
      new_act[j * 2] = old_act[j];
    }
    
    for (unsigned int j = 1; j < old_cols; j++)
    {
      new_row[j * 2 - 1] = 0.5 * (old_row[j - 1] + old_row[j]);
      new_act[j * 2 - 1] = old_act[j - 1] && old_act[j];
    }
    
    old_row = new_row;
    old_act = new_act;
  }
  
  std::vector<float> new_u(new_cols);
  for (unsigned int j = 0; j < old_cols; j++)
  {
    new_u[j * 2] = u_[j];
  }
  
  for (unsigned int j = 1; j < old_cols; j++)
  {
    new_u[j * 2 - 1] = 0.5 * (u_[j - 1] + u_[j]);
  }
  
  u_ = new_u;
}

//----------------------------------------------------------------
// the_grid_t::col_more_quads
// 
// increase the number of rows per column:
// 
void
the_grid_t::col_more_quads()
{
  const unsigned int old_rows = v_.size();
  const unsigned int new_rows = old_rows * 2 - 1;
  
  std::vector<std::vector<p3x1_t> > new_grid(new_rows);
  std::vector<std::vector<bool> > new_active(new_rows);
  std::vector<float> new_v(new_rows);
  
  for (unsigned int j = 0; j < old_rows; j++)
  {
    new_grid[j * 2] = grid_[j];
    new_active[j * 2] = active_[j];
    new_v[j * 2] = v_[j];
  }
  
  const unsigned int & cols = u_.size();
  for (unsigned int j = 1; j < old_rows; j++)
  {
    new_active[j * 2 - 1].resize(cols);
    for (unsigned int i = 0; i < cols; i++)
    {
      new_active[j * 2 - 1][i] = active_[j - 1][i] && active_[j][i];
    }
    
    interpolate(grid_[j - 1], grid_[j], 0.5, new_grid[j * 2 - 1]);
    new_v[j * 2 - 1] = 0.5 * (v_[j - 1] + v_[j]);
  }
  
  grid_ = new_grid;
  active_ = new_active;
  v_ = new_v;
}

//----------------------------------------------------------------
// the_grid_t::row_less_quads
// 
// throw away every other column, starting with the second column:
// 
void
the_grid_t::row_less_quads()
{
  const unsigned int old_cols = u_.size();
  if (old_cols % 2 == 0) return;
  
  const unsigned int new_cols = (old_cols + 1) / 2;
  std::vector<p3x1_t> new_row(new_cols);
  std::vector<bool>   new_act(new_cols);
  
  const unsigned int & rows = grid_.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    std::vector<p3x1_t> & old_row = grid_[i];
    std::vector<bool> &   old_act = active_[i];
    
    for (unsigned int j = 0; j < new_cols; j++)
    {
      new_row[j] = old_row[j * 2];
      new_act[j] = old_act[j * 2];
    }
    
    old_row = new_row;
    old_act = new_act;
  }
  
  std::vector<float> new_u(new_cols);
  for (unsigned int j = 0; j < new_cols; j++)
  {
    new_u[j] = u_[j * 2];
  }
  
  u_ = new_u;
}

//----------------------------------------------------------------
// the_grid_t::col_less_quads
// 
// throw away every other row, starting with the second row:
// 
void
the_grid_t::col_less_quads()
{
  const unsigned int old_rows = v_.size();
  if (old_rows % 2 == 0) return;
  
  const unsigned int new_rows = (old_rows + 1) / 2;
  std::vector<std::vector<p3x1_t> > new_grid(new_rows);
  std::vector<std::vector<bool> > new_active(new_rows);
  std::vector<float> new_v(new_rows);
  
  for (unsigned int j = 0; j < new_rows; j++)
  {
    new_grid[j] = grid_[j * 2];
    new_active[j] = active_[j * 2];
    new_v[j] = v_[j * 2];
  }
  
  grid_ = new_grid;
  active_ = new_active;
  v_ = new_v;
}

//----------------------------------------------------------------
// the_grid_t::resize_quads
// 
void
the_grid_t::resize_quads(const p3x1_t & from, const p3x1_t & to)
{
  // grow/shrink the size of the grid by dragging its boundary:
}

//----------------------------------------------------------------
// the_grid_t::move
// 
void
the_grid_t::move(const v3x1_t & drag_wcs)
{
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  for (unsigned int i = 0; i < rows; i++)
  {
    for (unsigned int j = 0; j < cols; j++)
    {
      if (active_[i][j])
      {
	grid_[i][j] = anchor_[i][j] + drag_wcs;
      }
    }
  }
}

//----------------------------------------------------------------
// the_grid_t::move_absolute
// 
void
the_grid_t::move_absolute(const unsigned int & coord, const float & value)
{
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  for (unsigned int i = 0; i < rows; i++)
  {
    for (unsigned int j = 0; j < cols; j++)
    {
      if (active_[i][j])
      {
	grid_[i][j][coord] = value;
      }
    }
  }
}

//----------------------------------------------------------------
// the_grid_t::move
// 
void
the_grid_t::move(const row_col_t & rc, const v3x1_t & drag_wcs)
{
  const unsigned int & r = rc[0];
  const unsigned int & c = rc[1];
  if (r == UINT_MAX || c == UINT_MAX) return;
  
  grid_[r][c] = anchor_[r][c] + drag_wcs;
}

//----------------------------------------------------------------
// the_grid_t::is_active
// 
bool
the_grid_t::is_active(const row_col_t & rc) const
{
  const unsigned int & r = rc[0];
  const unsigned int & c = rc[1];
  
  if (r != UINT_MAX && c != UINT_MAX)
  {
    return active_[r][c];
  }
  else if (r != UINT_MAX)
  {
    const std::vector<bool> & row = active_[r];
    bool active = row[0];
    
    const unsigned int & cols = u_.size();
    for (unsigned int i = 1; i < cols; i++)
    {
      active = active & row[i];
    }
    
    return active;
  }
  else if (c != UINT_MAX)
  {
    bool active = active_[0][c];
    
    const unsigned int & rows = v_.size();
    for (unsigned int i = 1; i < rows; i++)
    {
      active = active & active_[i][c];
    }
    
    return active;
  }
  
  return false;
}

//----------------------------------------------------------------
// set_active
// 
static void
set_active(std::vector<std::vector<bool> > & grid,
	   const row_col_t & rc,
	   const bool & active)
{
  const unsigned int & r = rc[0];
  const unsigned int & c = rc[1];
  
  if (r != UINT_MAX && c != UINT_MAX)
  {
    grid[r][c] = active;
  }
  else if (r != UINT_MAX)
  {
    std::vector<bool> & row = grid[r];
    const unsigned int & cols = row.size();
    for (unsigned int i = 0; i < cols; i++) row[i] = active;
  }
  else if (c != UINT_MAX)
  {
    const unsigned int & rows = grid.size();
    for (unsigned int i = 0; i < rows; i++) grid[i][c] = active;
  }
}

//----------------------------------------------------------------
// the_grid_t::activate
// 
void
the_grid_t::activate(const row_col_t & rc)
{
  ::set_active(active_, rc, true);
}

//----------------------------------------------------------------
// the_grid_t::deactivate
// 
void
the_grid_t::deactivate(const row_col_t & rc)
{
  ::set_active(active_, rc, false);
}

//----------------------------------------------------------------
// set_active
// 
static void
set_active(std::vector<std::vector<bool> > & grid, const bool & active)
{
  const unsigned int & rows = grid.size();
  for (unsigned int i = 0; i < rows; i++)
  {
    std::vector<bool> & row = grid[i];
    
    const unsigned int & cols = row.size();
    for (unsigned int j = 0; j < cols; j++)
    {
      row[j] = active;
    }
  }
}

//----------------------------------------------------------------
// the_grid_t::activate_all_points
// 
void
the_grid_t::activate_all_points()
{
  ::set_active(active_, true);
}

//----------------------------------------------------------------
// the_grid_t::deactivate_all_points
// 
void
the_grid_t::deactivate_all_points()
{
  ::set_active(active_, false);
}

//----------------------------------------------------------------
// the_grid_t::has_active_points
// 
bool
the_grid_t::has_active_points() const
{
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  for (unsigned int i = 0; i < rows; i++)
  {
    const std::vector<bool> & row = active_[i];
    
    for (unsigned int j = 0; j < cols; j++)
    {
      if (row[j]) return true;
    }
  }
  
  return false;
}

//----------------------------------------------------------------
// the_grid_t::calc_bbox
// 
void
the_grid_t::calc_bbox(the_aa_bbox_t & bbox) const
{
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  for (unsigned int i = 0; i < rows; i++)
  {
    const std::vector<p3x1_t> & row = grid_[i];

    for (unsigned int j = 0; j < cols; j++)
    {
      bbox << row[j];
    }
  }
}

//----------------------------------------------------------------
// the_grid_t::is_singular
// 
bool
the_grid_t::is_singular() const
{
  const unsigned int & rows = v_.size();
  const unsigned int & cols = u_.size();
  
  const p3x1_t ref(grid_[0][0]);
  for (unsigned int i = 1; i < rows * cols; i++)
  {
    const unsigned int r = i / rows;
    const unsigned int c = i % rows;
    
    if (ref != grid_[r][c]) return false;
  }
  
  return true;
}

//----------------------------------------------------------------
// the_grid_t::save
// 
bool
the_grid_t::save(std::ostream & stream) const
{
  ::save(stream, grid_);
  ::save(stream, u_);
  ::save(stream, v_);
  return the_primitive_t::save(stream);
}

//----------------------------------------------------------------
// the_grid_t::load
// 
bool
the_grid_t::load(std::istream & stream)
{
  ::load(stream, grid_);
  ::load(stream, u_);
  ::load(stream, v_);
  
  anchor_ = grid_;
  ::assign(active_, v_.size(), u_.size(), false);
  
  return the_primitive_t::load(stream);
}


//----------------------------------------------------------------
// the_grid_ref_t::the_grid_ref_t
// 
the_grid_ref_t::the_grid_ref_t(const unsigned int & id,
			       const row_col_t & row_col,
			       const p2x1_t & uv):
  the_reference_t(id),
  row_col_(row_col),
  uv_(uv)
{}

//----------------------------------------------------------------
// the_grid_ref_t::eval
// 
bool
the_grid_ref_t::eval(the_registry_t * registry,
		     p3x1_t & wcs_pt) const
{
  const the_grid_t * prim = registry->elem<the_grid_t>(id());
  if (prim == NULL) return false;
  
  const std::vector<std::vector<p3x1_t> > & grid = prim->grid();
  const std::vector<float> & grid_u = prim->u();
  const std::vector<float> & grid_v = prim->v();
  
  const unsigned int & row = row_col_[0];
  const unsigned int & col = row_col_[1];
  const float & u = uv_[0];
  const float & v = uv_[1];
  
  if (row != UINT_MAX && col != UINT_MAX)
  {
    // FIXME: maybe I should perform a bilinear interpolation instead of
    // going for the closest corner point?
    wcs_pt = grid[row][col];
  }
  else if (row != UINT_MAX)
  {
    const unsigned int ref_col = find_containing_segment(grid_u, u);
    
    float t = (u - grid_u[ref_col]) / (grid_u[ref_col + 1] - grid_u[ref_col]);
    wcs_pt = grid[row][ref_col] + t * (grid[row][ref_col + 1] -
				       grid[row][ref_col]);
  }
  else // col != UINT_MAX
  {
    const unsigned int ref_row = find_containing_segment(grid_v, v);
    
    float t = (v - grid_v[ref_row]) / (grid_v[ref_row + 1] - grid_v[ref_row]);
    wcs_pt = grid[ref_row][col] + t * (grid[ref_row + 1][col] -
				       grid[ref_row][col]);
  }
  
  return true;
}

//----------------------------------------------------------------
// the_grid_ref_t::move
// 
bool
the_grid_ref_t::move(the_registry_t * registry,
		     const the_view_mgr_t & view_mgr,
		     const p3x1_t & wcs_pt)
{
  const the_grid_t * grid = registry->elem<the_grid_t>(id());
  if (grid == NULL) return false;
  
  the_view_volume_t volume = view_mgr.view_volume();
  p2x1_t scs_pt = volume.to_scs(wcs_pt);
  view_mgr.setup_pick_volume(volume,
			     scs_pt,
			     std::max(view_mgr.width(),
				      view_mgr.height()) * 0.5);

  std::list<the_pick_data_t> data;
  if (!grid->intersect(volume, data)) return false;
  
  const the_grid_ref_t * grid_ref =
    dynamic_cast<const the_grid_ref_t *>((*(data.begin())).ref());
  
  *this = *grid_ref;
  return true;
}

//----------------------------------------------------------------
// the_grid_ref_t::equal
// 
bool
the_grid_ref_t::equal(const the_reference_t * ref) const
{
  if (!the_reference_t::equal(ref)) return false;
  
  const the_grid_ref_t * grid_ref =
    dynamic_cast<const the_grid_ref_t *>(ref);
  if (grid_ref == NULL) return false;
  
  return ((grid_ref->row_col_ == row_col_) && (grid_ref->uv_ == uv_));
}

//----------------------------------------------------------------
// the_grid_ref_t::dump
// 
void
the_grid_ref_t::dump(ostream & strm, unsigned int indent) const
{
  strm << INDSCP << "the_grid_ref_t(" << (void *)this << ")" << endl
       << INDSCP << "{" << endl;
  the_reference_t::dump(strm, INDNXT);
  strm << INDSTR << "row_col_ = " << row_col_ << ";" << endl
       << INDSTR << "uv_ = " << uv_ << ";" << endl
       << INDSCP << "}" << endl << endl;
}

//----------------------------------------------------------------
// the_grid_ref_t::save
// 
bool
the_grid_ref_t::save(std::ostream & stream) const
{
  ::save(stream, row_col_);
  ::save(stream, uv_);
  return the_reference_t::save(stream);
}

//----------------------------------------------------------------
// the_grid_ref_t::load
// 
bool
the_grid_ref_t::load(std::istream & stream)
{
  ::load(stream, row_col_);
  ::load(stream, uv_);
  return the_reference_t::load(stream);
}
