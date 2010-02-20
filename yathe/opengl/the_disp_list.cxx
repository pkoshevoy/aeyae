// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_disp_list.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The OpenGL diplay list and various display list elements.

// local includes:
#include "opengl/the_disp_list.hxx"
#include "opengl/the_gl_context.hxx"
#include "opengl/the_ep_grid.hxx"
#include "math/the_ray.hxx"
#include "math/the_view_volume.hxx"
#include "utils/the_utils.hxx"

// system includes:
#include <assert.h>
#include <math.h>
#include <algorithm>
#include <limits>


//----------------------------------------------------------------
// the_point_dl_elem_t::the_point_dl_elem_t
// 
the_point_dl_elem_t::the_point_dl_elem_t(const p3x1_t & p,
					 const the_color_t & c):
  the_dl_elem_t(),
  pt_(p),
  color_(c)
{}

//----------------------------------------------------------------
// the_point_dl_elem_t::draw
// 
void
the_point_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    glColor4fv(color_.rgba());
    glBegin(GL_POINTS);
    glVertex3fv(pt_.data());
    glEnd();
  }
}

//----------------------------------------------------------------
// the_point_dl_elem_t::update_bbox
// 
void
the_point_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox << pt_;
}

//----------------------------------------------------------------
// the_point_dl_elem_t::pick
// 
bool
the_point_dl_elem_t::pick(const the_view_volume_t & pick_volume,
			  float & volume_axis_parameter,
			  float & distance_to_volume_axis) const
{
  volume_axis_parameter = pick_volume.depth_of_wcs_pt(pt_);
  
  the_cyl_uv_csys_t uv_frame;
  pick_volume.uv_frame_at_depth(volume_axis_parameter, uv_frame);
  
  float angle;
  uv_frame.wcs_to_lcs(pt_, distance_to_volume_axis, angle);
  
  return (distance_to_volume_axis <= 1.0 &&
	  volume_axis_parameter >= 0.0 &&
	  volume_axis_parameter <= 1.0);
}


//----------------------------------------------------------------
// the_line_strip_dl_elem_t::the_line_strip_dl_elem_t
// 
the_line_strip_dl_elem_t::the_line_strip_dl_elem_t(const the_color_t & c0,
						   const the_color_t & c1,
						   const float & line_width):
  the_dl_elem_t(),
  line_width_(line_width)
{
  color_[0] = c0;
  color_[1] = c1;
}

//----------------------------------------------------------------
// the_line_strip_dl_elem_t::draw
// 
void
the_line_strip_dl_elem_t::draw() const
{
  if (pt_.empty()) return;
  
  the_scoped_gl_attrib_t push_attr(GL_LINE_BIT | GL_ENABLE_BIT);
  {
    glLineWidth(line_width_);
    glDisable(GL_LIGHTING);
    glDisable(GL_LINE_SMOOTH);
    
#if 1
    glBegin(GL_LINE_STRIP);
    {
      unsigned int index = 0;
      for (std::list<p3x1_t>::const_iterator
	     i = pt_.begin(); i != pt_.end(); ++i, index++)
      {
	glColor4fv(color_[(index % 4) / 2].rgba());
	glVertex3fv((*i).data());
      }
    }
#else
    glBegin(GL_LINES);
    {
      unsigned int index = 0;
      const p3x1_t * prev = &(pt_.front());
      for (std::list<p3x1_t>::const_iterator
	     i = ++(pt_.begin()); i != pt_.end(); ++i, index++)
      {
	const p3x1_t & next = *i;
	
	glColor4fv(color_[index % 2].rgba());
	glVertex3fv(prev->data());
	glVertex3fv(next.data());
	prev = &next;
      }
    }
#endif
    glEnd();
  }
}

//----------------------------------------------------------------
// the_line_strip_dl_elem_t::update_bbox
// 
void
the_line_strip_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  for (std::list<p3x1_t>::const_iterator i = pt_.begin(); i != pt_.end(); ++i)
  {
    bbox << *i;
  }
}


//----------------------------------------------------------------
// the_line_dl_elem_t::the_line_dl_elem_t
// 
the_line_dl_elem_t::the_line_dl_elem_t(const p3x1_t & a,
				       const p3x1_t & b,
				       const the_color_t & c,
				       const float & line_width):
  the_dl_elem_t(),
  pt_a_(a),
  pt_b_(b),
  color_(c),
  line_width_(line_width)
{}

//----------------------------------------------------------------
// the_line_dl_elem_t::draw
// 
void
the_line_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_LINE_BIT | GL_ENABLE_BIT);
  {
    glLineWidth(line_width_);
    glDisable(GL_LIGHTING);
    glColor4fv(color_.rgba());
    glBegin(GL_LINES);
    {
      glVertex3fv(pt_a_.data());
      glVertex3fv(pt_b_.data());
    }
    glEnd();
  }
}

//----------------------------------------------------------------
// the_line_dl_elem_t::update_bbox
// 
void
the_line_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox << pt_a_ << pt_b_;
}

//----------------------------------------------------------------
// the_line_dl_elem_t::reset
// 
void
the_line_dl_elem_t::reset(const p3x1_t & a, const p3x1_t & b)
{
  pt_a_ = a;
  pt_b_ = b;
}

//----------------------------------------------------------------
// the_line_dl_elem_t::pick
// 
bool
the_line_dl_elem_t::pick(const the_view_volume_t & pick_volume,
			 float & volume_axis_parameter,
			 float & distance_to_volume_axis,
			 float & line_parameter) const
{
  the_ray_t pick_ray = pick_volume.to_ray(p2x1_t(0.0, 0.0));
  the_ray_t line_ray(pt_a_, pt_b_ - pt_a_);
  float wcs_distance;
  if (!pick_ray.intersect(line_ray,
			  volume_axis_parameter,
			  line_parameter,
			  wcs_distance)) return false;
  
  if (line_parameter < 0.0 ||
      line_parameter > 1.0 ||
      volume_axis_parameter < 0.0 ||
      volume_axis_parameter > 1.0) return false;
  
  // make sure the distance to volume axis is within tolerance:
  p3x1_t cyl_pt;
  pick_volume.wcs_to_cyl(line_ray * line_parameter, cyl_pt);
  distance_to_volume_axis = cyl_pt.x();
  
  return (distance_to_volume_axis <= 1.0);
}


//----------------------------------------------------------------
// the_gradient_line_dl_elem_t::the_gradient_line_dl_elem_t
// 
the_gradient_line_dl_elem_t::
the_gradient_line_dl_elem_t(const p3x1_t & a,
			    const p3x1_t & b,
			    const the_color_t & c,
			    const the_color_t & d,
			    const float & line_width):
  the_line_dl_elem_t(a, b, c, line_width),
  end_color_(d)
{}

//----------------------------------------------------------------
// the_gradient_line_dl_elem_t::draw
// 
void the_gradient_line_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_LINE_BIT | GL_ENABLE_BIT);
  {
    glLineWidth(line_width_);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINES);
    {
      glColor4fv(color_.rgba());
      glVertex3fv(pt_a_.data());
      glColor4fv(end_color_.rgba());
      glVertex3fv(pt_b_.data());
    }
    glEnd();
  }
}


//----------------------------------------------------------------
// draw_vertex
// 
static void
draw_vertex(const the_vertex_t & vertex)
{
  glNormal3fv(vertex.vn.data());
  glTexCoord2fv(vertex.vt.data());
  glVertex3fv(vertex.vx.data());
}

//----------------------------------------------------------------
// the_triangle_dl_elem_t::the_triangle_dl_elem_t
// 
the_triangle_dl_elem_t::
the_triangle_dl_elem_t(const std::vector<the_vertex_t> & points,
		       const bool & calc_normal):
  the_dl_elem_t(),
  pt_(points)
{
  if (calc_normal)
  {
    v3x1_t x_axis = !(pt_[1].vx - pt_[0].vx);
    v3x1_t y_axis_candidate = !(pt_[2].vx - pt_[1].vx);
    v3x1_t normal = !(x_axis % y_axis_candidate);
    
    pt_[0].vn = normal;
    pt_[1].vn = normal;
    pt_[2].vn = normal;
  }
}

//----------------------------------------------------------------
// the_triangle_dl_elem_t::draw
// 
void
the_triangle_dl_elem_t::draw() const
{
  glBegin(GL_TRIANGLES);
  
  draw_vertex(pt_[0]);
  draw_vertex(pt_[1]);
  draw_vertex(pt_[2]);
  
  glEnd();
}

//----------------------------------------------------------------
// the_triangle_dl_elem_t::update_bbox
// 
void
the_triangle_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox << pt_[0].vx
       << pt_[1].vx
       << pt_[2].vx;
}


//----------------------------------------------------------------
// the_color_triangle_dl_elem_t::the_color_triangle_dl_elem_t
// 
the_color_triangle_dl_elem_t::
the_color_triangle_dl_elem_t(const the_color_t & color,
			     const std::vector<the_vertex_t> & points,
			     const bool & calc_normal):
  the_triangle_dl_elem_t(points, calc_normal),
  color_(color)
{}

//----------------------------------------------------------------
// the_color_triangle_dl_elem_t::draw
// 
void
the_color_triangle_dl_elem_t::draw() const
{
  glMaterialfv(GL_FRONT, GL_DIFFUSE, color_.rgba());
  glMaterialfv(GL_BACK,  GL_DIFFUSE, color_.rgba());
  
  the_triangle_dl_elem_t::draw();
}


//----------------------------------------------------------------
// the_polygon_dl_elem_t::the_polygon_dl_elem_t
// 
the_polygon_dl_elem_t::
the_polygon_dl_elem_t(const std::vector<the_vertex_t> & points,
		      const bool & calc_normal):
  the_dl_elem_t(),
  pt_(points)
{
  if (calc_normal)
  {
    size_t pt_index[] =
    {
      0,
      std::numeric_limits<size_t>::max(),
      std::numeric_limits<size_t>::max()
    };
    
    // shortcut to the array size:
    const size_t & num_pts = pt_.size();
    
    // find a point such that it is not coincident with the initial point:
    for (size_t i = pt_index[0] + 1; i < num_pts; i++)
    {
      float dist_sqrd = (pt_[i].vx - pt_[pt_index[0]].vx).norm_sqrd();
      if (dist_sqrd > 0.0)
      {
	pt_index[1] = i;
	break;
      }
    }
    
    // sanity check:
    if (pt_index[1] == std::numeric_limits<size_t>::max())
    {
      dump();
      assert(false);
    }
    
    v3x1_t x_axis = !(pt_[pt_index[1]].vx - pt_[pt_index[0]].vx);
    
    // find a point that would have the smallest dot product with the vector
    // formed by the first two points:
    float min_cos_theta = FLT_MAX;
    for (size_t j = pt_index[1] + 1; j < num_pts; j++)
    {
      v3x1_t vec = !(pt_[j].vx - pt_[pt_index[1]].vx);
      float cos_theta = fabs(vec * x_axis);
      if (cos_theta < min_cos_theta)
      {
	min_cos_theta = cos_theta;
	pt_index[2] = j;
      }
    }
    
    // sanity check:
    if (pt_index[2] == std::numeric_limits<size_t>::max())
    {
      dump();
      assert(false);
    }
    
    v3x1_t y_axis_candidate = !(pt_[pt_index[2]].vx - pt_[pt_index[1]].vx);
    v3x1_t normal = !(x_axis % y_axis_candidate);
    
    for (size_t k = 0; k < num_pts; k++)
    {
      pt_[k].vn = normal;
    }
  }
}

//----------------------------------------------------------------
// the_polygon_dl_elem_t::draw
// 
void
the_polygon_dl_elem_t::draw() const
{
  glBegin(GL_POLYGON);
  
  const size_t & num_pts = pt_.size();
  for (size_t i = 0; i < num_pts; i++)
  {
    draw_vertex(pt_[i]);
  }
  
  glEnd();
}

//----------------------------------------------------------------
// the_polygon_dl_elem_t::update_bbox
// 
void
the_polygon_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  const size_t & num_pts = pt_.size();
  for (size_t i = 0; i < num_pts; i++)
  {
    bbox << pt_[i].vx;
  }
}

//----------------------------------------------------------------
// the_polygon_dl_elem_t::split
// 
void
the_polygon_dl_elem_t::split(const the_plane_t & plane,
			     std::list<the_vertex_t> & neg_list,
			     std::list<the_vertex_t> & pos_list)
{
  const size_t & num_points = pt_.size();
  
  // find distance from each point to the plane:
  std::vector<float> point_plane_dist(num_points);
  for (size_t i = 0; i < num_points; i++)
  {
    point_plane_dist[i] = plane.dist(pt_[i].vx);
  }
  
  for (size_t i = 0; i < num_points; i++)
  {
    size_t j = (i + 1) % num_points;
    
    const float & di = point_plane_dist[i];
    const float & dj = point_plane_dist[j];
    
    const the_vertex_t & pi = pt_[i];
    const the_vertex_t & pj = pt_[j];
    
    if (di < 0.0)
    {
      neg_list.push_back(pi);
    }
    else
    {
      pos_list.push_back(pi);
    }
    
    // find edge/plane intersections:
    if ((di < 0.0 && dj >= 0.0) ||
	(dj < 0.0 && di >= 0.0))
    {
      float pipj_x_plane_param = fabs(di) / (fabs(di) + fabs(dj));
      the_vertex_t px;
      the_vertex_t::interpolate(pi, pj, pipj_x_plane_param, px);
      neg_list.push_back(px);
      pos_list.push_back(px);
    }
  }
}

//----------------------------------------------------------------
// the_polygon_dl_elem_t::dump
// 
void
the_polygon_dl_elem_t::dump() const
{
  for (size_t i = 0; i < pt_.size(); i++)
  {
    cerr << pt_[i].vx << endl;
  }
}


//----------------------------------------------------------------
// the_color_polygon_dl_elem_t::the_color_polygon_dl_elem_t
// 
the_color_polygon_dl_elem_t::
the_color_polygon_dl_elem_t(const the_color_t & color,
			    const std::vector<the_vertex_t> & points,
			    const bool & calc_normal):
  the_polygon_dl_elem_t(points, calc_normal),
  color_(color)
{}

//----------------------------------------------------------------
// the_color_polygon_dl_elem_t::draw
// 
void
the_color_polygon_dl_elem_t::draw() const
{
  glMaterialfv(GL_FRONT, GL_DIFFUSE, color_.rgba());
  glMaterialfv(GL_BACK,  GL_DIFFUSE, color_.rgba());
  
  the_polygon_dl_elem_t::draw();
}


//----------------------------------------------------------------
// the_text_dl_elem_t::draw
// 
void
the_text_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    THE_ASCII_FONT.print(text_, pt_, color_);
  }
}


//----------------------------------------------------------------
// the_masked_text_dl_elem_t::draw
// 
void
the_masked_text_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    THE_ASCII_FONT.print(text_, pt_, color_, mask_color_);
  }
}


//----------------------------------------------------------------
// the_symbol_dl_elem_t::draw
// 
void
the_symbol_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    symbols_->draw(color_, pt_, symbol_id_);
    // cerr << "draw: " << pt_ << endl;
  }
}


//----------------------------------------------------------------
// the_masked_symbol_dl_elem_t::draw
// 
void
the_masked_symbol_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    symbols_->draw(symbol_id_, pt_, color_, mask_color_);
  }
}


//----------------------------------------------------------------
// the_arrow_dl_elem_t::draw
// 
void
the_arrow_dl_elem_t::draw() const
{
  static const float two_pi = float(M_PI * 2.0);
  if ((pt_a_ - pt_b_).norm_sqrd() <= THE_EPSILON) return;
  
  p3x1_t pt_c = (pt_b_ + ((0.78f * stripe_len_) /
			  (~(pt_a_ - pt_b_)) * (pt_a_ - pt_b_)));
  
  // use a color zebra to draw the arrow blades:
  const the_color_t * zebra[] =
  {
    &blade_color_,
    &color_
  };
  
  the_scoped_gl_attrib_t push_attr(GL_LINE_BIT | GL_ENABLE_BIT);
  {
    glLineWidth(the_line_dl_elem_t::line_width_);
    glDisable(GL_LIGHTING);
    glEnable(GL_POLYGON_SMOOTH);
    
    // draw the arrow shaft:
    v3x1_t v = pt_a_ - pt_b_;
    v3x1_t v_unit = !v;
    unsigned int num_stripes = (unsigned int)(~v / stripe_len_);
    p3x1_t prev_pt = pt_b_;
    for (unsigned int i = 0; i < num_stripes; i++)
    {
      p3x1_t curr_pt = prev_pt + v_unit * stripe_len_;
      
      glColor4fv(zebra[i % 2]->rgba());
      glBegin(GL_LINES);
      {
	glVertex3fv(prev_pt.data());
	glVertex3fv(curr_pt.data());
      }
      glEnd();
      
      prev_pt = curr_pt;
    }
    
    glColor4fv(zebra[num_stripes % 2]->rgba());
    glBegin(GL_LINES);
    {
      glVertex3fv(prev_pt.data());
      glVertex3fv(pt_a_.data());
    }
    glEnd();
    
    // draw the blades:
    v3x1_t cs_z = !(pt_a_ - pt_b_);
    v3x1_t cs_y = !norm_;
    v3x1_t cs_x = !(cs_y % cs_z);
    the_cyl_coord_sys_t cyl_cs(cs_x, cs_y, cs_z, pt_b_);
    
    float radius = 0.3f * stripe_len_;
    
    for (unsigned int i = 0; i < num_blades_; i++)
    {
      glColor4fv(zebra[i % 2]->rgba());
      
      float angle = ((float)(i) * two_pi) / (float)(num_blades_);
      p3x1_t blade_pt = cyl_cs.to_wcs(p3x1_t(radius, angle, stripe_len_));
      
      glBegin(GL_POLYGON);
      {
	glVertex3fv(pt_b_.data());
	glVertex3fv(blade_pt.data());
	glVertex3fv(pt_c.data());
      }
      glEnd();
    }
  }
}

//----------------------------------------------------------------
// the_arrow_dl_elem_t::set_stripe_len
//
void
the_arrow_dl_elem_t::set_stripe_len(const the_view_volume_t & view_volume,
				    const float & window_width,
				    const float & window_height)
{
  float diagonal = sqrt(window_width * window_width +
			 window_height * window_height);
  float scs_stripe_size = 16.0f / diagonal;
  p2x1_t scs_pt_a = view_volume.to_scs(pt_a_);
  p2x1_t scs_pt_b = scs_pt_a + v2x1_t(scs_stripe_size, scs_stripe_size);
  float depth = std::max(view_volume.depth_of_wcs_pt(pt_a_), float(0));
  p3x1_t wcs_pt_b = view_volume.to_wcs(p3x1_t(scs_pt_b, depth));
  stripe_len_ = ~(wcs_pt_b - pt_a_);
}


//----------------------------------------------------------------
// the_scs_arrow_dl_elem_t::the_scs_arrow_dl_elem_t
// 
the_scs_arrow_dl_elem_t::
the_scs_arrow_dl_elem_t(const float &      window_width,
			const float &      window_height,
			const p2x1_t &      pt_a,
			const p2x1_t &      pt_b,
			const the_color_t & color,
			const float &      pixels_blade_length):
  the_dl_elem_t(),
  window_width_(window_width),
  window_height_(window_height),
  pt_a_(pt_a),
  pt_b_(pt_b),
  color_(color),
  blade_length_(pixels_blade_length)
{}

//----------------------------------------------------------------
// the_scs_arrow_dl_elem_t::draw
// 
void
the_scs_arrow_dl_elem_t::draw() const
{
  if ((pt_a_ - pt_b_).norm_sqrd() <= THE_EPSILON) return;
  
  p3x1_t pt_a(pt_a_.x() * window_width_, pt_a_.y() * window_height_, 0.0);
  p3x1_t pt_b(pt_b_.x() * window_width_, pt_b_.y() * window_height_, 0.0);
  
  // draw the blades:
  v3x1_t cs_z = !(pt_a - pt_b);
  v3x1_t cs_y(0.0, 0.0, 1.0);
  v3x1_t cs_x = !(cs_y % cs_z);
  the_cyl_coord_sys_t cyl_cs(cs_x, cs_y, cs_z, pt_b);
  
  float radius = 0.3f * blade_length_;
  p3x1_t blade_a = cyl_cs.to_wcs(p3x1_t(radius, 0, blade_length_));
  p3x1_t blade_b = cyl_cs.to_wcs(p3x1_t(radius, float(M_PI), blade_length_));
  
  // draw the arrow:
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    glColor4fv(color_.rgba());
    glBegin(GL_LINES);
    {
      glVertex3fv(pt_a.data());
      glVertex3fv(pt_b.data());
      glVertex3fv(blade_a.data());
      glVertex3fv(pt_b.data());
      glVertex3fv(pt_b.data());
      glVertex3fv(blade_b.data());
    }
    glEnd();
  }
}

//----------------------------------------------------------------
// the_scs_arrow_dl_elem_t::update_bbox
// 
void
the_scs_arrow_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox << pt_a_ << pt_b_;
}


//----------------------------------------------------------------
// the_height_map_dl_elem_t::the_height_map_dl_elem_t
// 
the_height_map_dl_elem_t::
the_height_map_dl_elem_t(const std::vector< std::vector<p3x1_t> > & vertex,
			 const std::vector<the_color_t> & color):
  vertex_(vertex)
{
  color_blend_.color() = color;
}

//----------------------------------------------------------------
// the_height_map_dl_elem_t::draw
// 
void
the_height_map_dl_elem_t::draw() const
{
  p3x1_t min = bbox_.wcs_min();
  p3x1_t max = bbox_.wcs_max();
  float z_min = min.z();
  float z_max = max.z();
  float z_range = (z_max - z_min);
  
  size_t rows = vertex_.size();
  size_t cols = vertex_[0].size();
  
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT | GL_POLYGON_BIT);
  {
    glDisable(GL_LIGHTING);
    glEnable(GL_POLYGON_OFFSET_FILL);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0, 1.0);
    
    for (size_t i = 1; i < rows; i++)
    {
      glBegin(GL_QUAD_STRIP);
      for (size_t j = 0; j < cols; j++)
      {
	const p3x1_t & a = vertex_[i - 1][j];
	float ta = (a.z() - z_min) / z_range;
	glColor4fv(color_blend_(ta).rgba());
	glVertex3fv(a.data());
	
	const p3x1_t & b = vertex_[i][j];
	float tb = (b.z() - z_min) / z_range;
	glColor4fv(color_blend_(tb).rgba());
	glVertex3fv(b.data());
      }
      glEnd();
    }
    
    glColor4fv(the_color_t::BLACK.rgba());
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    for (size_t i = 1; i < rows; i++)
    {
      glBegin(GL_QUAD_STRIP);
      for (size_t j = 0; j < cols; j++)
      {
	glVertex3fv(vertex_[i - 1][j].data());
	glVertex3fv(vertex_[i][j].data());
      }
      glEnd();
    }
  }
}

//----------------------------------------------------------------
// the_height_map_dl_elem_t::update_bbox
// 
void
the_height_map_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  size_t rows = vertex_.size();
  size_t cols = vertex_[0].size();
  for (size_t i = 0; i < rows; i++)
  {
    for (size_t j = 0; j < cols; j++)
    {
      bbox << vertex_[i][j];
    }
  }
  
  // cache the results:
  bbox_ = bbox;
}

//----------------------------------------------------------------
// the_tex_surf_data_dl_elem_t::the_tex_surf_data_dl_elem_t
// 
the_tex_surf_data_dl_elem_t::
the_tex_surf_data_dl_elem_t(const std::vector< std::vector<p3x1_t> > & vertex,
			    const std::vector< std::vector<v3x1_t> > & normal,
			    const std::vector<the_color_t> & color):
  vertex_(vertex),
  normal_(normal),
  color_(color.size()),
  texture_id_(UINT_MAX)
{
  for (size_t i = 0; i < color.size(); i++)
  {
    color_[i] = the_rgba_word_t(color[i]);
  }

  the_scoped_gl_client_attrib_t push_client_attr(GL_UNPACK_ALIGNMENT);
  {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_1D, texture_id_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage1D(GL_TEXTURE_1D,
		 0,
		 GL_RGBA8,
		 (GLsizei)(color_.size()),
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_INT,
		 &(color_[0]));
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  }
}

//----------------------------------------------------------------
// the_tex_surf_data_dl_elem_t::~the_tex_surf_data_dl_elem_t
// 
the_tex_surf_data_dl_elem_t::~the_tex_surf_data_dl_elem_t()
{
  glDeleteTextures(1, &texture_id_);
}

//----------------------------------------------------------------
// the_tex_surf_data_dl_elem_t::draw
// 
void
the_tex_surf_data_dl_elem_t::draw() const
{
  glBindTexture(GL_TEXTURE_1D, texture_id_);
  float z_min = bbox_.wcs_min().z();
  float z_max = bbox_.wcs_max().z();
  float z_range = (z_max - z_min);
  
  size_t rows = vertex_.size();
  size_t cols = vertex_[0].size();
  
  glMaterialfv(GL_FRONT, GL_DIFFUSE, the_color_t::WHITE.rgba());
  glMaterialfv(GL_BACK,  GL_DIFFUSE, the_color_t::WHITE.rgba());
  
  the_scoped_gl_attrib_t push_attr(GL_TEXTURE_BIT);
  {
    glEnable(GL_TEXTURE_1D);
    for (size_t i = 1; i < rows; i++)
    {
      glBegin(GL_QUAD_STRIP);
      for (size_t j = 0; j < cols; j++)
      {
	const p3x1_t & va = vertex_[i - 1][j];
	const v3x1_t & na = normal_[i - 1][j];
	float ta = (va.z() - z_min) / z_range;
	// glTexCoord2f(ta, 0.0);
	glTexCoord1f(ta);
	glNormal3fv(na.data());
	glVertex3fv(va.data());
	
	const p3x1_t & vb = vertex_[i][j];
	const v3x1_t & nb = normal_[i][j];
	float tb = (vb.z() - z_min) / z_range;
	// glTexCoord2f(tb, 0.0);
	glTexCoord1f(tb);
	glNormal3fv(nb.data());
	glVertex3fv(vb.data());
      }
      glEnd();
    }
  }
}

//----------------------------------------------------------------
// the_tex_surf_data_dl_elem_t::update_bbox
// 
void
the_tex_surf_data_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  size_t rows = vertex_.size();
  size_t cols = vertex_[0].size();
  for (size_t i = 0; i < rows; i++)
  {
    for (size_t j = 0; j < cols; j++)
    {
      bbox << vertex_[i][j];
    }
  }
  
  // cache the results:
  bbox_ = bbox;
}


//----------------------------------------------------------------
// the_coord_sys_dl_elem_t::the_coord_sys_dl_elem_t
// 
the_coord_sys_dl_elem_t::
the_coord_sys_dl_elem_t(const the_view_volume_t & volume,
			const float & window_width,
			const float & window_height,
			const the_coord_sys_t & cs,
			const the_color_t & blade_color,
			const the_color_t & shaft_color,
			const char * x_label,
			const char * y_label,
			const char * z_label,
			const float & sx,
			const float & sy,
			const float & sz):
  the_dl_elem_t(),
  x_arrow_(volume,
	   window_width,
	   window_height,
	   cs.origin(),
	   cs.origin() + cs.x_axis() * sx,
	   cs.y_axis(),
	   shaft_color,
	   blade_color,
	   0),
  y_arrow_(volume,
	   window_width,
	   window_height,
	   cs.origin(),
	   cs.origin() + cs.y_axis() * sy,
	   cs.z_axis(),
	   shaft_color,
	   blade_color,
	   0),
  z_arrow_(volume,
	   window_width,
	   window_height,
	   cs.origin(),
	   cs.origin() + cs.z_axis() * sz,
	   cs.x_axis(),
	   shaft_color,
	   blade_color,
	   0),
  x_label_(x_label),
  y_label_(y_label),
  z_label_(z_label),
  color_(blade_color)
{}

//----------------------------------------------------------------
// the_coord_sys_dl_elem_t::the_coord_sys_dl_elem_t
// 
the_coord_sys_dl_elem_t::
the_coord_sys_dl_elem_t(const the_coord_sys_t & cs,
			const the_color_t & blade_color,
			const the_color_t & shaft_color,
			const char * x_label,
			const char * y_label,
			const char * z_label):
  the_dl_elem_t(),
  x_arrow_(cs.origin(),
	   cs.origin() + cs.x_axis(),
	   cs.y_axis(),
	   shaft_color,
	   blade_color,
	   5,
	   0),
  y_arrow_(cs.origin(),
	   cs.origin() + cs.y_axis(),
	   cs.z_axis(),
	   shaft_color,
	   blade_color,
	   5,
	   0),
  z_arrow_(cs.origin(),
	   cs.origin() + cs.z_axis(),
	   cs.x_axis(),
	   shaft_color,
	   blade_color,
	   5,
	   0),
  x_label_(x_label),
  y_label_(y_label),
  z_label_(z_label),
  color_(blade_color)
{}

//----------------------------------------------------------------
// the_coord_sys_dl_elem_t::draw
// 
void
the_coord_sys_dl_elem_t::draw() const
{
  x_arrow_.draw();
  y_arrow_.draw();
  z_arrow_.draw();
  
  const p3x1_t & origin = x_arrow_.pt_a();
  float scale = 1.05f;
  v3x1_t x_axis = scale * (x_arrow_.pt_b() - origin);
  v3x1_t y_axis = scale * (y_arrow_.pt_b() - origin);
  v3x1_t z_axis = scale * (z_arrow_.pt_b() - origin);
  
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    THE_ASCII_FONT.print(x_label_, origin + x_axis, color_);
    THE_ASCII_FONT.print(y_label_, origin + y_axis, color_);
    THE_ASCII_FONT.print(z_label_, origin + z_axis, color_);
  }
}

//----------------------------------------------------------------
// the_coord_sys_dl_elem_t::update_bbox
// 
void
the_coord_sys_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  x_arrow_.update_bbox(bbox);
  y_arrow_.update_bbox(bbox);
  z_arrow_.update_bbox(bbox);
}


//----------------------------------------------------------------
// the_bbox_dl_elem_t::the_bbox_dl_elem_t
// 
the_bbox_dl_elem_t::the_bbox_dl_elem_t(const the_bbox_t & bbox,
				       const the_color_t & color):
  the_dl_elem_t(),
  bbox_(bbox),
  color_(color)
{}

//----------------------------------------------------------------
// the_bbox_dl_elem_t::draw
// 
void
the_bbox_dl_elem_t::draw() const
{
  p3x1_t corner[8];
  bbox_.wcs_corners(corner);
  
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glDisable(GL_LIGHTING);
    glColor4fv(color_.rgba());
    
    glBegin(GL_LINE_LOOP);
    {
      glVertex3fv(corner[0].data());
      glVertex3fv(corner[1].data());
      glVertex3fv(corner[2].data());
      glVertex3fv(corner[3].data());
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
      glVertex3fv(corner[4].data());
      glVertex3fv(corner[5].data());
      glVertex3fv(corner[6].data());
      glVertex3fv(corner[7].data());
    }
    glEnd();
    
    glBegin(GL_LINES);
    {
      glVertex3fv(corner[0].data());
      glVertex3fv(corner[4].data());
      glVertex3fv(corner[1].data());
      glVertex3fv(corner[5].data());
      glVertex3fv(corner[2].data());
      glVertex3fv(corner[6].data());
      glVertex3fv(corner[3].data());
      glVertex3fv(corner[7].data());
    }
    glEnd();
  }
}

//----------------------------------------------------------------
// the_bbox_dl_elem_t::update_bbox
// 
void
the_bbox_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox += bbox_;
}


//----------------------------------------------------------------
// the_ep_grid_dl_elem_t::the_ep_grid_dl_elem_t
// 
the_ep_grid_dl_elem_t::the_ep_grid_dl_elem_t(the_ep_grid_t * ep_grid):
  the_dl_elem_t(),
  ep_grid_(ep_grid)
{}

//----------------------------------------------------------------
// the_ep_grid_dl_elem_t::~the_ep_grid_dl_elem_t
// 
the_ep_grid_dl_elem_t::~the_ep_grid_dl_elem_t()
{
  delete ep_grid_;
  ep_grid_ = NULL;
}

//----------------------------------------------------------------
// the_ep_grid_dl_elem_t::draw
// 
void
the_ep_grid_dl_elem_t::draw() const
{
  ep_grid_->draw();
}


//----------------------------------------------------------------
// the_view_volume_dl_elem_t::the_view_volume_dl_elem_t
// 
the_view_volume_dl_elem_t::
the_view_volume_dl_elem_t(const the_view_volume_t & view_volume):
  view_volume_(new the_view_volume_t(view_volume))
{}

//----------------------------------------------------------------
// the_view_volume_dl_elem_t::~the_view_volume_dl_elem_t
// 
the_view_volume_dl_elem_t::~the_view_volume_dl_elem_t()
{
  delete view_volume_;
  view_volume_ = NULL;
}

//----------------------------------------------------------------
// the_view_volume_dl_elem_t::draw
// 
void
the_view_volume_dl_elem_t::draw() const
{
  const the_color_t zebra[] =
  {
    the_color_t::RED,
    the_color_t::WHITE
  };
  
  const float two_pi = float(2.0 * M_PI);
  const unsigned int num_seg = 10;
  const float half_seg = 1.0f / float(num_seg) * float(M_PI);
  
  for (unsigned int i = 0; i < num_seg; i++)
  {
    const float a = half_seg + (float(i) /
				 float(num_seg)) * two_pi;
    const float b = half_seg + (float((i + 1) % num_seg) /
				 float(num_seg)) * two_pi;
    
    const unsigned int num_layers = 2;
    for (unsigned int j = 0; j < num_layers; j++)
    {
      std::vector<the_vertex_t> pt(4);
      
      float h0 = float(j) / float(num_layers);
      float h1 = float(j + 1) / float(num_layers);
      
      view_volume_->cyl_to_wcs(p3x1_t(1.0, a, h0), pt[0].vx);
      view_volume_->cyl_to_wcs(p3x1_t(1.0, b, h0), pt[1].vx);
      view_volume_->cyl_to_wcs(p3x1_t(1.0, b, h1), pt[2].vx);
      view_volume_->cyl_to_wcs(p3x1_t(1.0, a, h1), pt[3].vx);
      
      the_color_polygon_dl_elem_t face(zebra[((i % 2) + j) % 2], pt);
      face.draw();
    }
  }
}


//----------------------------------------------------------------
// the_disp_list_dl_elem_t::the_disp_list_dl_elem_t
// 
the_disp_list_dl_elem_t::
the_disp_list_dl_elem_t(const the_disp_list_t & display_list):
  the_dl_elem_t(),
  dl_(display_list)
{}

//----------------------------------------------------------------
// the_disp_list_dl_elem_t::draw
// 
void
the_disp_list_dl_elem_t::draw() const
{
  dl_.execute();
}

//----------------------------------------------------------------
// the_disp_list_dl_elem_t::update_bbox
// 
void
the_disp_list_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox += dl_.bbox();
}


//----------------------------------------------------------------
// the_instance_dl_elem_t::the_instance_dl_elem_t
// 
the_instance_dl_elem_t::
the_instance_dl_elem_t(const the_disp_list_t & display_list,
		       const m4x4_t & lcs_to_wcs):
  the_disp_list_dl_elem_t(display_list),
  lcs_to_wcs_(lcs_to_wcs)
{}

//----------------------------------------------------------------
// the_instance_dl_elem_t::draw
// 
void
the_instance_dl_elem_t::draw() const
{
  the_scoped_gl_attrib_t push_attr(GL_ENABLE_BIT);
  {
    glEnable(GL_NORMALIZE);
    the_scoped_gl_matrix_t push_matrix(GL_MODELVIEW);
    {
      glMultMatrixf(lcs_to_wcs_.data());
      the_disp_list_dl_elem_t::draw();
    }
  }
}

//----------------------------------------------------------------
// the_instance_dl_elem_t::update_bbox
// 
void
the_instance_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  if (dl_.bbox().is_empty()) return;
  
  p3x1_t corners[8];
  dl_.bbox().wcs_corners(corners);
  
  for (unsigned int i = 0; i < 8; i++)
  {
    bbox << lcs_to_wcs_ * corners[i];
  }
}


//----------------------------------------------------------------
// the_disp_list_t::the_disp_list_t
// 
the_disp_list_t::the_disp_list_t():
  std::list<the_dl_elem_t *>(),
  list_id_(0),
  up_to_date_(false)
{}

//----------------------------------------------------------------
// the_disp_list_t::the_disp_list_t
// 
the_disp_list_t::the_disp_list_t(const the_bbox_t & bbox):
  std::list<the_dl_elem_t *>(),
  list_id_(0),
  up_to_date_(false),
  bbox_(bbox)
{}

//----------------------------------------------------------------
// the_disp_list_t::~the_disp_list_t
// 
the_disp_list_t::~the_disp_list_t()
{
  the_disp_list_t::clear();
}

//----------------------------------------------------------------
// the_disp_list_t::clear
// 
void
the_disp_list_t::clear()
{
  if (empty()) return;
  
  the_gl_context_t current(the_gl_context_t::current());
  if (context_.is_valid())
  {
    context_.make_current();
  }
  
  while (!empty())
  {
    delete remove_head(*this);
  }
  
  bbox_.clear();
  
  if (list_id_ != 0)
  {
    glDeleteLists(list_id_, 1);
    list_id_ = 0;
  }
  
  if (current.is_valid())
  {
    current.make_current();
  }
  
  // forget the context:
  context_.invalidate();
}

//----------------------------------------------------------------
// the_disp_list_t::push_back
// 
bool
the_disp_list_t::push_back(the_dl_elem_t * const & elem)
{
  if (elem == NULL) return false;
  
  std::list<the_dl_elem_t *>::push_back(elem);
  elem->update_bbox(bbox_);
  up_to_date_ = false;
  
  return true;
}

//----------------------------------------------------------------
// the_disp_list_t::draw
// 
void
the_disp_list_t::draw() const
{
  if (empty()) return;
  
  // store the context:
  context_ = the_gl_context_t::current();
  
  for (std::list<the_dl_elem_t *>::const_iterator i = begin(); i != end(); ++i)
  {
    the_dl_elem_t * e = *i;
    e->draw();
    FIXME_OPENGL("the_disp_list_t::draw");
  }
}

//----------------------------------------------------------------
// the_disp_list_t::compile
// 
void
the_disp_list_t::compile(GLenum mode)
{
  if (empty()) return;
  
  if (list_id_ == 0)
  {
    // try to allocate a display list:
    list_id_ = glGenLists(1);
  }
  
  if (list_id_ == 0 && mode == GL_COMPILE)
  {
    // if we couldn't get a display list, and that's all we wanted -- return:
    return;
  }
  
  if (list_id_ != 0)
  {
    // if we have a display list -- activate it:
    glNewList(list_id_, mode);
  }
  
  // draw the geometry:
  draw();
  
  if (list_id_ != 0)
  {
    // if we have a display list -- deactivate it:
    glEndList();
    up_to_date_ = true;
  }
}

//----------------------------------------------------------------
// the_disp_list_t::execute
// 
void
the_disp_list_t::execute() const
{
  if (empty()) return;
  
  if (list_id_ == 0)
  {
    draw();
  }
  else
  {
    if (!up_to_date_)
    {
      the_disp_list_t * fake = const_cast<the_disp_list_t *>(this);
      fake->compile(GL_COMPILE_AND_EXECUTE);
    }
    else
    {
      glCallList(list_id_);
    }
  }
}
