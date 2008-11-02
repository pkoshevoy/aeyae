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


// File         : the_disp_list.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The OpenGL diplay list and various display list elements.

#ifndef THE_DISP_LIST_HXX_
#define THE_DISP_LIST_HXX_

// system includes:
#include <list>
#include <vector>
#include <assert.h>

// local includes:
#include "math/the_bbox.hxx"
#include "math/v3x1p3x1.hxx"
#include "math/m4x4.hxx"
#include "math/the_plane.hxx"
#include "math/the_color.hxx"
#include "math/the_color_blend.hxx"
#include "opengl/OpenGLCapabilities.h"
#include "opengl/the_gl_context.hxx"
#include "opengl/the_symbols.hxx"
#include "opengl/the_ascii_font.hxx"
#include "opengl/the_point_symbols.hxx"
#include "opengl/the_vertex.hxx"
#include "utils/the_text.hxx"
#include "utils/the_indentation.hxx"

// forward declarations:
class the_ep_grid_t;
class the_view_volume_t;
class the_disp_list_t;


//----------------------------------------------------------------
// the_dl_elem_t
// 
class the_dl_elem_t
{
public:
  virtual ~the_dl_elem_t() {}
  
  // draw the element:
  virtual void draw() const = 0;
  
  // add the dimensions of this element to the bounding box:
  virtual void update_bbox(the_bbox_t & bbox) const = 0;
};

//----------------------------------------------------------------
// the_point_dl_elem_t
// 
class the_point_dl_elem_t : public the_dl_elem_t
{
public:
  the_point_dl_elem_t(const p3x1_t & p = p3x1_t(0.0, 0.0, 0.0),
		      const the_color_t & c = the_color_t(1.0, 1.0, 1.0, 1.0));
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
  // reinitialize this line segment:
  inline void reset(const p3x1_t & pt)
  { pt_ = pt; }
  
  // selection test:
  bool pick(const the_view_volume_t & pick_volume,
	    float & volume_axis_parameter,
	    float & distance_to_volume_axis) const;
  
  // accessors:
  inline const p3x1_t & pt() const
  { return pt_; }
  
  inline const the_color_t & color() const
  { return color_; }
  
  inline void set_color(const the_color_t & color)
  { color_ = color; }
  
protected:
  p3x1_t pt_;
  the_color_t color_;
};

//----------------------------------------------------------------
// the_line_strip_dl_elem_t
// 
class the_line_strip_dl_elem_t : public the_dl_elem_t
{
public:
  the_line_strip_dl_elem_t(const the_color_t & c0 =
			   the_color_t(0.5, 0.5, 0.5, 1.0),
			   const the_color_t & c1 =
			   the_color_t(0.0, 0.0, 0.0, 1.0),
			   const float & line_width = 1.0);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
  // accessors:
  inline const std::list<p3x1_t> & pt() const { return pt_; }
  inline       std::list<p3x1_t> & pt()       { return pt_; }
  
  inline const the_color_t & color(unsigned int i) const
  { return color_[i]; }
  
  inline void set_color(unsigned int i, const the_color_t & color)
  { color_[i] = color; }
  
  inline void set_line_width(const float & line_width)
  { line_width_ = line_width; }
  
protected:
  std::list<p3x1_t> pt_;
  the_color_t color_[2];
  float line_width_;
};

//----------------------------------------------------------------
// the_line_dl_elem_t
// 
class the_line_dl_elem_t : public the_dl_elem_t
{
public:
  the_line_dl_elem_t(const p3x1_t & a = p3x1_t(0.0, 0.0, 0.0),
		     const p3x1_t & b = p3x1_t(1.0, 1.0, 1.0),
		     const the_color_t & c = the_color_t(0.5, 0.5, 0.5, 1.0),
		     const float & line_width = 1.0);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
  // reinitialize this line segment:
  void reset(const p3x1_t & a, const p3x1_t & b);
  
  // selection test:
  bool pick(const the_view_volume_t & pick_volume,
	    float & volume_axis_parameter,
	    float & distance_to_volume_axis,
	    float & line_parameter) const;
  
  // accessors:
  inline const p3x1_t & pt_a() const
  { return pt_a_; }
  
  inline const p3x1_t & pt_b() const
  { return pt_b_; }
  
  inline const the_color_t & color() const
  { return color_; }
  
  inline void set_color(const the_color_t & color)
  { color_ = color; }
  
  inline void set_line_width(const float & line_width)
  { line_width_ = line_width; }
  
protected:
  p3x1_t pt_a_;
  p3x1_t pt_b_;
  the_color_t color_;
  float line_width_;
};

//----------------------------------------------------------------
// the_gradient_line_dl_elem_t
// 
class the_gradient_line_dl_elem_t : public the_line_dl_elem_t
{
public:
  the_gradient_line_dl_elem_t
  (const p3x1_t & a = p3x1_t(0.0, 0.0, 0.0),
   const p3x1_t & b = p3x1_t(1.0, 1.0, 1.0),
   const the_color_t & c = the_color_t(0.0, 0.0, 0.0, 1.0),
   const the_color_t & d = the_color_t(1.0, 1.0, 1.0, 1.0),
   const float & line_width = 1.0);
  
  // virtual:
  void draw() const;
  
  // accessors:
  inline const the_color_t & end_color() const
  { return end_color_; }
  
  inline void set_end_color(const the_color_t & color)
  { end_color_ = color; }
  
protected:
  the_color_t end_color_;
};

//----------------------------------------------------------------
// the_triangle_dl_elem_t
// 
class the_triangle_dl_elem_t : public the_dl_elem_t
{
public:
  the_triangle_dl_elem_t(const std::vector<the_vertex_t> & points,
			 const bool & calc_normal = false);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  // triangle vertices:
  std::vector<the_vertex_t> pt_;
};


//----------------------------------------------------------------
// the_color_triangle_dl_elem_t
// 
class the_color_triangle_dl_elem_t : public the_triangle_dl_elem_t
{
public:
  the_color_triangle_dl_elem_t(const the_color_t & color,
			       const std::vector<the_vertex_t> & points,
			       const bool & calc_normal = false);
  
  // virtual:
  void draw() const;
  
protected:
  // triangle color:
  the_color_t color_;
};


//----------------------------------------------------------------
// the_polygon_dl_elem_t
// 
class the_polygon_dl_elem_t : public the_dl_elem_t
{
public:
  the_polygon_dl_elem_t(const std::vector<the_vertex_t> & points,
			const bool & calc_normal = false);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
  // split this polygon in to two pieces, one on the negative side
  // of the reference plane, and another on the positive:
  void split(const the_plane_t & plane,
	     std::list<the_vertex_t> & neg_list,
	     std::list<the_vertex_t> & pos_list);
  
  // for debugging purposes:
  void dump() const;
  
  // accessor:
  inline const std::vector<the_vertex_t> & pt() const
  { return pt_; }
  
protected:
  // polygon vertices:
  std::vector<the_vertex_t> pt_;
};


//----------------------------------------------------------------
// the_color_polygon_dl_elem_t
// 
class the_color_polygon_dl_elem_t : public the_polygon_dl_elem_t
{
public:
  the_color_polygon_dl_elem_t(const the_color_t & color,
			      const std::vector<the_vertex_t> & points,
			      const bool & calc_normal = false);
  
  // virtual:
  void draw() const;
  
protected:
  // polygon color:
  the_color_t color_;
};


//----------------------------------------------------------------
// the_text_dl_elem_t
// 
class the_text_dl_elem_t : public the_point_dl_elem_t
{
public:
  the_text_dl_elem_t(const p3x1_t & position = p3x1_t(0.0, 0.0, 0.0),
		     const the_color_t & color = the_color_t::WHITE,
		     const char * text = ""):
    the_point_dl_elem_t(position, color),
    text_(text)
  {}
  
  // virtual:
  void draw() const;
  
  // accessor:
  inline void set_text(const char * text)
  { text_.assign(text); }
  
protected:
  the_text_t text_;
};

//----------------------------------------------------------------
// the_masked_text_dl_elem_t
// 
class the_masked_text_dl_elem_t : public the_text_dl_elem_t
{
public:
  the_masked_text_dl_elem_t(const p3x1_t & position = p3x1_t(0.0, 0.0, 0.0),
			    const the_color_t & color = the_color_t::WHITE,
			    const the_color_t & mask = the_color_t::BLACK,
			    const char * text = ""):
    the_text_dl_elem_t(position, color, text),
    mask_color_(mask)
  {}
  
  // virtual:
  void draw() const;
  
  // accessor:
  inline void set_mask_color(const the_color_t & mask_color)
  { mask_color_ = mask_color; }
  
private:
  the_color_t mask_color_;
};

//----------------------------------------------------------------
// the_symbol_dl_elem_t
// 
class the_symbol_dl_elem_t : public the_point_dl_elem_t
{
public:
  the_symbol_dl_elem_t(const p3x1_t & position = p3x1_t(0.0, 0.0, 0.0),
		       const the_color_t & color = the_color_t::WHITE,
		       const the_symbols_t & syms = THE_POINT_SYMBOLS,
		       const unsigned int & id = THE_FILLED_CIRCLE_SYMBOL_E):
    the_point_dl_elem_t(position, color),
    symbols_(&syms),
    symbol_id_(id)
  {}
  
  // virtual:
  void draw() const;
  
  // accessors:
  void set_symbols(const the_symbols_t & syms)
  { symbols_ = &syms; }
  
  void set_symbol_id(const unsigned int & id)
  { symbol_id_ = id; }
  
protected:
  // a set of symbols:
  const the_symbols_t * symbols_;
  
  // id of the symbol to be drawn:
  unsigned int symbol_id_;
};

//----------------------------------------------------------------
// the_masked_symbol_dl_elem_t
// 
class the_masked_symbol_dl_elem_t : public the_symbol_dl_elem_t
{
public:
  the_masked_symbol_dl_elem_t(const p3x1_t & position = p3x1_t(0.0, 0.0, 0.0),
			      const the_color_t & color = the_color_t::WHITE,
			      const the_color_t & mask = the_color_t::WHITE,
			      const the_symbols_t & syms = THE_POINT_SYMBOLS,
			      const unsigned int & id =
			      THE_FILLED_CIRCLE_SYMBOL_E):
    the_symbol_dl_elem_t(position, color, syms, id),
    mask_color_(mask)
  {}
  
  // virtual:
  void draw() const;
  
  // accessor:
  inline void set_mask_color(const the_color_t & mask_color)
  { mask_color_ = mask_color; }
  
private:
  the_color_t mask_color_;
};


//----------------------------------------------------------------
// the_arrow_dl_elem_t
// 
class the_arrow_dl_elem_t : public the_line_dl_elem_t
{
public:
  the_arrow_dl_elem_t() {}
  
  the_arrow_dl_elem_t(const the_view_volume_t & volume,
		      const float & window_width,
		      const float & window_height,
		      const p3x1_t & pt_a,
		      const p3x1_t & pt_b,
		      const v3x1_t & norm,
		      const the_color_t & shaft_color,
		      const the_color_t & blade_color,
		      const unsigned int & num_blades = 2,
		      const float & line_width = 1.0):
    the_line_dl_elem_t(pt_a, pt_b, shaft_color, line_width),
    norm_(norm),
    blade_color_(blade_color),
    num_blades_(num_blades)
  { set_stripe_len(volume, window_width, window_height); }
  
  the_arrow_dl_elem_t(const p3x1_t & pt_a,
		      const p3x1_t & pt_b,
		      const v3x1_t & norm,
		      const the_color_t & shaft_color,
		      const the_color_t & blade_color,
		      const unsigned int & num_stripes = 5,
		      const unsigned int & num_blades = 2,
		      const float & line_width = 1.0):
    the_line_dl_elem_t(pt_a, pt_b, shaft_color, line_width),
    norm_(norm),
    blade_color_(blade_color),
    num_blades_(num_blades)
  { set_num_stripes(num_stripes); }
  
  the_arrow_dl_elem_t(const p3x1_t & pt_a,
		      const p3x1_t & pt_b,
		      const the_color_t & shaft_color,
		      const the_color_t & blade_color,
		      const unsigned int & num_blades,
		      const float & stripe_len,
		      const float & line_width = 1.0):
    the_line_dl_elem_t(pt_a, pt_b, shaft_color, line_width),
    norm_((!(pt_b - pt_a)).normal()),
    blade_color_(blade_color),
    num_blades_(num_blades),
    stripe_len_(stripe_len)
  {}
  
  inline void reset(const p3x1_t & pt_a,
		    const p3x1_t & pt_b,
		    const v3x1_t & norm)
  {
    the_line_dl_elem_t::reset(pt_a, pt_b);
    norm_ = norm;
  }
  
  // virtual:
  void draw() const;
  
  // accessors:
  inline void set_blade_color(const the_color_t & blade_color)
  { blade_color_ = blade_color; }
  
  inline void set_num_blades(const unsigned int & num_blades)
  { num_blades_ = num_blades; }
  
  inline void set_stripe_len(const float & stripe_len)
  { stripe_len_ = stripe_len; }
  
  // helper functions:
  inline void set_num_stripes(const unsigned int & num_stripes)
  {
    assert(num_stripes != 0);
    stripe_len_ = ~(pt_b_ - pt_a_) / float(num_stripes);
  }
  
  void set_stripe_len(const the_view_volume_t & view_volume,
		      const float & window_width,
		      const float & window_height);
  
protected:
  v3x1_t norm_;
  
  the_color_t blade_color_;
  
  unsigned int num_blades_;
  float stripe_len_;
};


//----------------------------------------------------------------
// the_scs_arrow_dl_elem_t
// 
class the_scs_arrow_dl_elem_t : public the_dl_elem_t
{
public:
  the_scs_arrow_dl_elem_t(const float &      window_width,
			  const float &      window_height,
			  const p2x1_t &      pt_a,
			  const p2x1_t &      pt_b,
			  const the_color_t & color,
			  const float &      pixels_blade_length = 11.0);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  the_scs_arrow_dl_elem_t();
  
  float window_width_;
  float window_height_;
  
  p2x1_t pt_a_;
  p2x1_t pt_b_;
  
  the_color_t color_;
  
  float blade_length_;
};


//----------------------------------------------------------------
// the_height_map_dl_elem_t
// 
class the_height_map_dl_elem_t : public the_dl_elem_t
{
public:
  the_height_map_dl_elem_t(const std::vector< std::vector<p3x1_t> > & vertex,
			   const std::vector<the_color_t> & color);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
private:
  // disable default constructor:
  the_height_map_dl_elem_t();
  
protected:
  // references to external surface data:
  const std::vector< std::vector<p3x1_t> > & vertex_;
  
  // surface colors (going from low point color to high point color):
  the_color_blend_t color_blend_;
  
  // cached surface data bounding box (updated during update_bbox):
  mutable the_bbox_t bbox_;
};


//----------------------------------------------------------------
// the_tex_surf_data_dl_elem_t
// 
class the_tex_surf_data_dl_elem_t : public the_dl_elem_t
{
public:
  the_tex_surf_data_dl_elem_t(const std::vector< std::vector<p3x1_t> > & vert,
			      const std::vector< std::vector<v3x1_t> > & norm,
			      const std::vector<the_color_t> & color);
  
  ~the_tex_surf_data_dl_elem_t();
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  the_tex_surf_data_dl_elem_t();
  
  // references to external surface data:
  const std::vector< std::vector<p3x1_t> > & vertex_;
  const std::vector< std::vector<v3x1_t> > & normal_;
  
  // surface colors (going from low point color to high point color):
  std::vector<the_rgba_word_t> color_;
  
  // texture name:
  GLuint texture_id_;
  
  // cached surface data bounding box (updated during update_bbox):
  mutable the_bbox_t bbox_;
};


//----------------------------------------------------------------
// the_coord_sys_dl_elem_t
// 
class the_coord_sys_dl_elem_t : public the_dl_elem_t
{
public:
  the_coord_sys_dl_elem_t(const the_view_volume_t & volume,
			  const float & window_width,
			  const float & window_height,
			  const the_coord_sys_t & cs,
			  const the_color_t & blade_color,
			  const the_color_t & shaft_color,
			  const char * x_label = "X",
			  const char * y_label = "Y",
			  const char * z_label = "Z",
			  const float & sx = float(1),
			  const float & sy = float(1),
			  const float & sz = float(1));
  
  the_coord_sys_dl_elem_t(const the_coord_sys_t & cs,
			  const the_color_t & blade_color,
			  const the_color_t & shaft_color,
			  const char * x_label = "X",
			  const char * y_label = "Y",
			  const char * z_label = "Z");
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  the_coord_sys_dl_elem_t();
  
  // the coordinate system arrows:
  the_arrow_dl_elem_t x_arrow_;
  the_arrow_dl_elem_t y_arrow_;
  the_arrow_dl_elem_t z_arrow_;
  
  the_text_t x_label_;
  the_text_t y_label_;
  the_text_t z_label_;
  
  // color of the axis labels:
  the_color_t color_;
};


//----------------------------------------------------------------
// the_bbox_dl_elem_t
// 
class the_bbox_dl_elem_t : public the_dl_elem_t
{
public:
  the_bbox_dl_elem_t(const the_bbox_t & bbox, const the_color_t & color);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
protected:
  the_bbox_dl_elem_t();
  
  // the bounding box:
  the_bbox_t bbox_;
  
  // color of the bounding box:
  the_color_t color_;
};


//----------------------------------------------------------------
// the_ep_grid_dl_elem_t
// 
class the_ep_grid_dl_elem_t : public the_dl_elem_t
{
public:
  the_ep_grid_dl_elem_t(the_ep_grid_t * ep_grid);
  ~the_ep_grid_dl_elem_t();
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t &) const {}
  
protected:
  the_ep_grid_dl_elem_t();
  
  // data members:
  the_ep_grid_t * ep_grid_;
};


//----------------------------------------------------------------
// the_view_volume_dl_elem_t
// 
class the_view_volume_dl_elem_t : public the_dl_elem_t
{
public:
  the_view_volume_dl_elem_t(const the_view_volume_t & view_volume);
  ~the_view_volume_dl_elem_t();
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t &) const {}
  
protected:
  the_view_volume_dl_elem_t();
  
  the_view_volume_t * view_volume_;
};


//----------------------------------------------------------------
// the_disp_list_dl_elem_t
// 
class the_disp_list_dl_elem_t : public the_dl_elem_t
{
public:
  the_disp_list_dl_elem_t(const the_disp_list_t & disp_list);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
private:
  // disable default constructor:
  the_disp_list_dl_elem_t();
  
protected:
  // references to some other display list:
  const the_disp_list_t & dl_;
};


//----------------------------------------------------------------
// the_instance_dl_elem_t
// 
class the_instance_dl_elem_t : public the_disp_list_dl_elem_t
{
public:
  the_instance_dl_elem_t(const the_disp_list_t & disp_list,
			 const m4x4_t & lcs_to_wcs);
  
  // virtual:
  void draw() const;
  void update_bbox(the_bbox_t & bbox) const;
  
private:
  // disable default constructor:
  the_instance_dl_elem_t();
  
protected:
  // the transform matrix applied to the mesh:
  const m4x4_t lcs_to_wcs_;
};


//----------------------------------------------------------------
// the_memory_managed_dl_elem_t
//
// It's a wrapper around another display element,
// in order to avoid the other display element being
// destroyed:
// 
class the_memory_managed_dl_elem_t : public the_dl_elem_t
{
public:
  the_memory_managed_dl_elem_t(const the_dl_elem_t & e):
    dl_elem_(e)
  {}
  
  // virtual:
  void draw() const
  { dl_elem_.draw(); }
  
  // virtual:
  void update_bbox(the_bbox_t & bbox) const
  { dl_elem_.update_bbox(bbox); }
  
private:
  // disable default constructor:
  the_memory_managed_dl_elem_t();
  
protected:
  const the_dl_elem_t & dl_elem_;
};


//----------------------------------------------------------------
// the_disp_list_t
// 
class the_disp_list_t : public std::list<the_dl_elem_t *>
{
public:
  the_disp_list_t();
  the_disp_list_t(const the_bbox_t & bbox);
  ~the_disp_list_t();
  
  // delete all elements in this list, clear the list and the bounding box:
  void clear();
  
  bool push_back(the_dl_elem_t * const & elem);
  
  // Draw the elements:
  void draw() const;
  
  // create an OpenGL display list:
  void compile(GLenum mode = GL_COMPILE);
  
  // execute the OpenGL display list:
  void execute() const;
  
  // bounding box of this display list:
  inline const the_bbox_t & bbox() const
  { return bbox_; }
  
  // accessor to this display lists id:
  inline unsigned int list_id() const
  { return list_id_; }
  
protected:
  // the OpenGL context assiciated with this list:
  mutable the_gl_context_t context_;
  
  // starting display list id offset:
  unsigned int list_id_;
  
  // this flag verifies whether the compiled OpenGL display list
  // is up to date with the current contents of this display list:
  bool up_to_date_;
  
  // bounding box of geometry stored in the list:
  the_bbox_t bbox_;
};


#endif // THE_DISP_LIST_HXX_
