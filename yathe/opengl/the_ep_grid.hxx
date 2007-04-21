// File         : the_ep_grid.hxx
// Author       : Paul A. Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : GPL.
// Description  :

#ifndef THE_EP_GRID_HXX_
#define THE_EP_GRID_HXX_

// local includes:
#include "math/the_color_blend.hxx"
#include "math/the_plane.hxx"

// forward declarations:
class the_view_mgr_t;
class the_palette_t;
class the_disp_list_t;


//----------------------------------------------------------------
// the_ep_grid_t
// 
class the_ep_grid_t
{
public:
  the_ep_grid_t(const the_view_mgr_t & view_mgr,
		const the_plane_t & plane,
		const the_palette_t & palette):
    view_mgr_(view_mgr),
    plane_(plane),
    palette_(palette)
  {}
  
  virtual ~the_ep_grid_t()
  {}
  
  // draw the edit plane grid:
  virtual void draw() const;
  
  // size of the default grid step measured in WCS units:
  virtual float default_step_size() const
  { return 0.2; }
  
  // the edit plane radius scale:
  virtual float ep_radius_scale() const;
  
  // helper functions:
  p2x1_t lcs_ep_center() const;
  p3x1_t wcs_ep_center() const;
  float ep_radius() const;
  
  void calc_ep_step_domain(int domain_u[2], int domain_v[2]) const;
  
  float zoom_exact() const;
  float zoom_prev() const;
  float zoom_next() const;
  float zoom_blend() const;
  
  float exact_step_size() const;
  float fixed_step_size() const;
  
  float number_of_exact_steps() const;
  float number_of_fixed_steps() const;
  
protected:
  // disable the default constructor:
  the_ep_grid_t();
  
  // draw the edit plane grid here:
  virtual void populate_disp_list(the_disp_list_t & dl) const = 0;
  
  // the view manager reference:
  const the_view_mgr_t & view_mgr_;
  
  // the coordinate system of the edit plane for which to draw the grid:
  the_plane_t plane_;
  
  // reference to the color palette:
  const the_palette_t & palette_;
};


//----------------------------------------------------------------
// the_original_ep_grid_t
// 
class the_original_ep_grid_t : public the_ep_grid_t
{
public:
  the_original_ep_grid_t(const the_view_mgr_t & view_mgr,
			 const the_plane_t & plane,
			 const the_palette_t & palette);
  
  // virtual: size of the default grid step measured in WCS units:
  float default_step_size() const
  { return 0.25; }
  
  // virtual: the edit plane radius scale:
  float ep_radius_scale() const
  { return 0.7 * the_ep_grid_t::ep_radius_scale(); }
  
protected:
  // disable the default constructor:
  the_original_ep_grid_t();
  
  // virtual: draw the edit plane grid here:
  void populate_disp_list(the_disp_list_t & dl) const;
  
  // color blend used to draw the grid:
  the_color_blend_t blend_;
};


//----------------------------------------------------------------
// the_quad_ep_grid_t
// 
class the_quad_ep_grid_t : public the_ep_grid_t
{
public:
  the_quad_ep_grid_t(const the_view_mgr_t & view_mgr,
		     const the_plane_t & plane,
		     const the_palette_t & palette);
  
protected:
  // disable the default constructor:
  the_quad_ep_grid_t();
  
  // virtual: draw the edit plane grid here:
  void populate_disp_list(the_disp_list_t & dl) const;
  
  // color blend used to draw the grid:
  the_color_blend_t blend_;
};


//----------------------------------------------------------------
// the_ampad_ep_grid_t
// 
class the_ampad_ep_grid_t : public the_ep_grid_t
{
public:
  the_ampad_ep_grid_t(const the_view_mgr_t & view_mgr,
		      const the_plane_t & plane,
		      const the_palette_t & palette);
  
protected:
  // disable the default constructor:
  the_ampad_ep_grid_t();
  
  // virtual: draw the edit plane grid here:
  void populate_disp_list(the_disp_list_t & dl) const;
  
  // color gradients used to draw the grids:
  the_color_blend_t light_gradient_;
  the_color_blend_t dark_gradient_;
};


//----------------------------------------------------------------
// the_ep_grid_csys_t
// 
class the_ep_grid_csys_t : public the_ep_grid_t
{
public:
  the_ep_grid_csys_t(const the_view_mgr_t & view_mgr,
		      const the_plane_t & plane,
		      const the_palette_t & palette);
  
  // virtual: draw the edit plane grid:
  void draw() const;
  
protected:
  // virtual:
  void populate_disp_list(the_disp_list_t &) const
  {}
  
  // helper function:
  void draw_csys(const the_coord_sys_t & cs,
		 const float & step_size,
		 const float & alpha,
		 the_disp_list_t & dl) const;
};


#endif // THE_EP_GRID_HXX_
