// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_ep_grid.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The edit plane grid class
//                provides basic theme/skin support for the OpenGL view widget.

// system includes:
#include <math.h>
#include <algorithm>
#include <vector>

// local includes:
#include "opengl/the_ep_grid.hxx"
#include "opengl/the_view_mgr.hxx"
#include "opengl/the_palette.hxx"
#include "opengl/the_disp_list.hxx"


//----------------------------------------------------------------
// the_ep_grid_t::ep_radius_scale
// 
float
the_ep_grid_t::ep_radius_scale() const
{
  return std::max(view_mgr_.scale_x(), view_mgr_.scale_y());
}

//----------------------------------------------------------------
// the_ep_grid_t::lcs_ep_center
// 
p2x1_t
the_ep_grid_t::lcs_ep_center() const
{
  return plane_ * view_mgr_.la();
}

//----------------------------------------------------------------
// the_ep_grid_t::wcs_ep_center
// 
p3x1_t
the_ep_grid_t::wcs_ep_center() const
{
  return plane_ * lcs_ep_center();
}

//----------------------------------------------------------------
// the_ep_grid_t::ep_radius
// 
float
the_ep_grid_t::ep_radius() const
{
  return ((view_mgr_.lfla_ray() * wcs_ep_center()) *
	  view_mgr_.view_radius() *
	  ep_radius_scale());
}

//----------------------------------------------------------------
// the_ep_grid_t::calc_ep_step_domain
// 
void
the_ep_grid_t::calc_ep_step_domain(int domain_u[2],
				   int domain_v[2]) const
{
  p2x1_t lcs_center = lcs_ep_center();
  float radius = ep_radius();
  float step_size = fixed_step_size();
  
  // horizontal step domain of the grid:
  domain_u[0] = int(ceil((lcs_center.x() - radius) / step_size + 0.05));
  domain_u[1] = int(floor((lcs_center.x() + radius) / step_size - 0.05));
  
  // vertical step domain of the grid:
  domain_v[0] = int(ceil((lcs_center.y() - radius) / step_size + 0.05));
  domain_v[1] = int(floor((lcs_center.y() + radius) / step_size - 0.05));
}

//----------------------------------------------------------------
// the_ep_grid_t::zoom_exact
// 
float
the_ep_grid_t::zoom_exact() const
{
  return (exact_step_size() / default_step_size());
}

//----------------------------------------------------------------
// the_ep_grid_t::zoom_prev
// 
float
the_ep_grid_t::zoom_prev() const
{
  float a = floorf(logf(zoom_exact()) / logf(2.0f));
  return powf(2.0f, a);
}

//----------------------------------------------------------------
// the_ep_grid_t::zoom_next
// 
float
the_ep_grid_t::zoom_next() const
{
  return (2.0f * zoom_prev());
}

//----------------------------------------------------------------
// the_ep_grid_t::zoom_blend
// 
float
the_ep_grid_t::zoom_blend() const
{
  float c = zoom_exact();
  float a = zoom_prev();
  float b = zoom_next();
  return ((c - a) / (b - a));
}

//----------------------------------------------------------------
// the_ep_grid_t::exact_step_size
// 
float
the_ep_grid_t::exact_step_size() const
{
  return view_mgr_.view_diameter() / number_of_exact_steps();
}

//----------------------------------------------------------------
// the_ep_grid_t::fixed_step_size
// 
float
the_ep_grid_t::fixed_step_size() const
{
  return (default_step_size() * zoom_prev());
}

//----------------------------------------------------------------
// the_ep_grid_t::number_of_exact_steps
// 
float
the_ep_grid_t::number_of_exact_steps() const
{
  return floorf(0.5f + view_mgr_.view_diameter() /
	       (view_mgr_.wcs_units_per_inch() * default_step_size()));
}

//----------------------------------------------------------------
// the_ep_grid_t::number_of_fixed_steps
// 
float
the_ep_grid_t::number_of_fixed_steps() const
{
  return floorf(0.5f + view_mgr_.view_diameter() / fixed_step_size());
}

//----------------------------------------------------------------
// the_ep_grid_t::draw
// 
void
the_ep_grid_t::draw() const
{
  // display list that will contain the edit plane primitives:
  the_disp_list_t dl(view_mgr_.near_far_bbox());
  
  // collect display primitives (specific to each subclass):
  populate_disp_list(dl);
  
  // setup OpenGL near/far clipping planes:
  const the_bbox_t & dl_bbox = dl.bbox();
  view_mgr_.reset_opengl_viewing();
  view_mgr_.setup_opengl_3d_viewing(dl_bbox.lcs_min().z(),
				    dl_bbox.lcs_max().z());
  
  // execute the display list:
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT | GL_LINE_BIT);
  {
#if 1
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
#else
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
#endif
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    dl.draw();
  }
}


//----------------------------------------------------------------
// the_original_ep_grid_t::the_original_ep_grid_t
// 
the_original_ep_grid_t::the_original_ep_grid_t(const the_view_mgr_t & view_mgr,
					       const the_plane_t & plane,
					       const the_palette_t & palette):
  the_ep_grid_t(view_mgr, plane, palette),
  blend_(2)
{
  the_color_t grid_color = palette_.ep()[THE_DARK_EP_GRID_E];
  the_color_t fade_color = 0.5 * (palette_.bg()[THE_UL_CORNER_E] +
				  palette_.bg()[THE_LL_CORNER_E]);
  
  blend_.color()[0] = grid_color;
  blend_.color()[1] = fade_color;
}

//----------------------------------------------------------------
// the_original_ep_grid_t::draw
// 
void
the_original_ep_grid_t::populate_disp_list(the_disp_list_t & dl) const
{
  const the_coord_sys_t & plane_cs = plane_.cs();
  
  p2x1_t lcs_center = lcs_ep_center();
  float radius = ep_radius();
  
  float step_size = fixed_step_size();
  float blend = zoom_blend();
  
  // calculate the horizontal/vertical step domain of the edit plane grid:
  int domain_u[] = { 0, 0 };
  int domain_v[] = { 0, 0 };
  calc_ep_step_domain(domain_u, domain_v);
  
  float steps = float(domain_u[1] - domain_u[0]);
  
  for (int i = domain_u[0]; i <= domain_u[1]; i++)
  {
    p3x1_t a(step_size * float(i), 0.0, 0.0);
    p3x1_t b(step_size * float(i), step_size * float(steps), 0.0);
    v3x1_t ab = b - a;
    
    for (int j = domain_v[0]; j <= domain_v[1]; j++)
    {
      float r = float(steps);
      float t = (r == 0.0f) ? 0.0f : float(j) / r;
      p3x1_t x = a + t * ab;
      
      float c = blend;
      if ((abs(j) % 2 == 0) && (abs(i) % 2 == 0))
      {
	c = std::min(c, float(1) - c);
      }
      
      the_color_t color = blend_(c);
      dl.push_back(new the_point_dl_elem_t(plane_cs.to_wcs(x), color));
    }
  }
  
  // draw the edit plane outline:
  const the_color_t & color = palette_.ep()[THE_DARK_EP_GRID_E];
  
  p3x1_t ul = plane_ * (lcs_center + v2x1_t(-radius,  radius));
  p3x1_t ur = plane_ * (lcs_center + v2x1_t( radius,  radius));
  p3x1_t lr = plane_ * (lcs_center + v2x1_t( radius, -radius));
  p3x1_t ll = plane_ * (lcs_center + v2x1_t(-radius, -radius));
  
  dl.push_back(new the_line_dl_elem_t(ul, ur, color));
  dl.push_back(new the_line_dl_elem_t(ur, lr, color));
  dl.push_back(new the_line_dl_elem_t(lr, ll, color));
  dl.push_back(new the_line_dl_elem_t(ll, ul, color));
}


//----------------------------------------------------------------
// the_quad_ep_grid_t::the_quad_ep_grid_t
// 
the_quad_ep_grid_t::the_quad_ep_grid_t(const the_view_mgr_t & view_mgr,
				       const the_plane_t & plane,
				       const the_palette_t & palette):
  the_ep_grid_t(view_mgr, plane, palette),
  blend_(2)
{
  the_color_t grid_color = palette_.ep()[THE_DARK_EP_GRID_E];
  the_color_t fade_color = 0.5 * (palette_.bg()[THE_UL_CORNER_E] +
				  palette_.bg()[THE_LL_CORNER_E]);
  
  blend_.color()[0] = grid_color;
  blend_.color()[1] = fade_color;
}

//----------------------------------------------------------------
// the_quad_ep_grid_t::draw
// 
void
the_quad_ep_grid_t::populate_disp_list(the_disp_list_t & dl) const
{
  const the_coord_sys_t & plane_cs = plane_.cs();
  
  p2x1_t lcs_center = lcs_ep_center();
  float radius = ep_radius();
  
  float step_size = fixed_step_size();
  float blend = zoom_blend();
  
  // calculate the horizontal/vertical step domain of the edit plane grid:
  int domain_u[] = { 0, 0 };
  int domain_v[] = { 0, 0 };
  calc_ep_step_domain(domain_u, domain_v);
  
  // columns:
  for (int i = domain_u[0]; i <= domain_u[1]; i++)
  {
    float t = blend;
    if (abs(i) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = blend_(t);
    
    p3x1_t a(step_size * float(i), lcs_center.y() - radius, 0.0);
    p3x1_t b(step_size * float(i), lcs_center.y() + radius, 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
  
  // rows:
  for (int j = domain_v[0]; j <= domain_v[1]; j++)
  {
    float t = blend;
    if (abs(j) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = blend_(t);
    
    p3x1_t a(lcs_center.x() - radius, step_size * float(j), 0.0);
    p3x1_t b(lcs_center.x() + radius, step_size * float(j), 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
}


//----------------------------------------------------------------
// the_ampad_ep_grid_t::the_ampad_ep_grid_t
// 
the_ampad_ep_grid_t::the_ampad_ep_grid_t(const the_view_mgr_t & view_mgr,
					 const the_plane_t & plane,
					 const the_palette_t & palette):
  the_ep_grid_t(view_mgr, plane, palette),
  light_gradient_(2),
  dark_gradient_(2)
{
  light_gradient_.color()[0] = palette_.ep()[THE_LIGHT_EP_GRID_E];
  light_gradient_.color()[1] = palette_.bg()[THE_LL_CORNER_E];
  
  dark_gradient_.color()[0] = palette_.ep()[THE_DARK_EP_GRID_E];
  dark_gradient_.color()[1] = palette_.bg()[THE_LL_CORNER_E];
}

//----------------------------------------------------------------
// the_ampad_ep_grid_t::draw
// 
void
the_ampad_ep_grid_t::populate_disp_list(the_disp_list_t & dl) const
{
  const the_coord_sys_t & plane_cs = plane_.cs();
  
  p2x1_t lcs_center = lcs_ep_center();
  float radius = ep_radius();
  
  float step_size = fixed_step_size();
  float blend = zoom_blend();
  
  // calculate the horizontal/vertical step domain of the edit plane grid:
  int domain_u[] = { 0, 0 };
  int domain_v[] = { 0, 0 };
  calc_ep_step_domain(domain_u, domain_v);
  
  // light columns:
  for (int i = domain_u[0]; i <= domain_u[1]; i++)
  {
    if (abs(i) % 5 == 0) continue;
    
    float t = blend;
    if (abs(i) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = light_gradient_(t);
    
    p3x1_t a(step_size * float(i), lcs_center.y() - radius, 0.0);
    p3x1_t b(step_size * float(i), lcs_center.y() + radius, 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
  
  // light rows:
  for (int j = domain_v[0]; j <= domain_v[1]; j++)
  {
    if (abs(j) % 5 == 0) continue;
    
    float t = blend;
    if (abs(j) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = light_gradient_(t);
    
    p3x1_t a(lcs_center.x() - radius, step_size * float(j), 0.0);
    p3x1_t b(lcs_center.x() + radius, step_size * float(j), 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
  
  // dark columns:
  for (int i = domain_u[0]; i <= domain_u[1]; i++)
  {
    if (abs(i) % 5 != 0) continue;
    
    float t = blend;
    if (abs(i) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = dark_gradient_(t);
    
    p3x1_t a(step_size * float(i), lcs_center.y() - radius, 0.0);
    p3x1_t b(step_size * float(i), lcs_center.y() + radius, 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
  
  // dark rows:
  for (int j = domain_v[0]; j <= domain_v[1]; j++)
  {
    if (abs(j) % 5 != 0) continue;
    
    float t = blend;
    if (abs(j) % 2 == 0) t = std::min(t, float(1) - t);
    the_color_t color = dark_gradient_(t);
    
    p3x1_t a(lcs_center.x() - radius, step_size * float(j), 0.0);
    p3x1_t b(lcs_center.x() + radius, step_size * float(j), 0.0);
    dl.push_back(new the_line_dl_elem_t(plane_cs.to_wcs(a),
					plane_cs.to_wcs(b),
					color));
  }
}


//----------------------------------------------------------------
// the_ep_grid_csys_t::the_ep_grid_csys_t
// 
the_ep_grid_csys_t::the_ep_grid_csys_t(const the_view_mgr_t & view_mgr,
				       const the_plane_t & plane,
				       const the_palette_t & palette):
  the_ep_grid_t(view_mgr, plane, palette)
{}

//----------------------------------------------------------------
// the_ep_grid_csys_t::draw
// 
void
the_ep_grid_csys_t::draw() const
{
  // display list that will contain the edit plane primitives:
  the_disp_list_t dl(view_mgr_.near_far_bbox());
  
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT | GL_LINE_BIT);
  {
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glDisable(GL_DEPTH_TEST);
    
    float step_size = fixed_step_size();
    float blend = zoom_blend();
    
    // setup the display list:
    // const the_coord_sys_t & plane_cs = plane_.cs();
    const the_coord_sys_t plane_cs;
    draw_csys(plane_cs, step_size, 1.0f - blend, dl);
    draw_csys(plane_cs, step_size * 2.0f, blend, dl);
    
    // setup OpenGL near/far clipping planes:
    const the_bbox_t & dl_bbox = dl.bbox();
    view_mgr_.reset_opengl_viewing();
    view_mgr_.setup_opengl_3d_viewing(dl_bbox.lcs_min().z(),
				      dl_bbox.lcs_max().z());
    
    // execute the display list:
    dl.draw();
  }
}

//----------------------------------------------------------------
// the_ep_grid_csys_t::draw_csys
// 
void
the_ep_grid_csys_t::draw_csys(const the_coord_sys_t & cs,
			      const float & step_size,
			      const float & alpha,
			      the_disp_list_t & dl) const
{
  const char * xyz[] = { "X", "Y", "Z" };
  
  for (unsigned int i = 0; i < 3; i++)
  {
    std::vector<p3x1_t> axis_pt(6);
    for (unsigned int p = 0; p < axis_pt.size(); p++)
    {
      p3x1_t lcs_pt(0.0, 0.0, 0.0);
      lcs_pt.data()[i] = float(p) * step_size;
      axis_pt[p] = cs.to_wcs(lcs_pt);
    }
    
    for (unsigned int s = 0; s < axis_pt.size() - 1; s++)
    {
      the_color_t color = palette_.cs()[s % 2];
      color[3] = alpha;
      dl.push_back(new the_line_dl_elem_t(axis_pt[s],
					  axis_pt[s + 1],
					  color,
					  2));
    }
    
    p3x1_t lcs_pt(0.0f, 0.0f, 0.0f);
    lcs_pt.data()[i] = (float(axis_pt.size() - 1) + 0.25f) * step_size;
    the_color_t color = palette_.cs()[0];
    color[3] = alpha;
    dl.push_back(new the_text_dl_elem_t(cs.to_wcs(lcs_pt),
					color,
					xyz[i]));
  }
}
