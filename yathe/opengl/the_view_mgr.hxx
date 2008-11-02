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


// File         : the_view_mgr.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Helper classes used to setup/evaluate the view volume.

#ifndef THE_VIEW_MGR_HXX_
#define THE_VIEW_MGR_HXX_

// local includes:
#include "opengl/the_view_mgr_orientation.hxx"
#include "math/the_view_volume.hxx"
#include "math/the_bbox.hxx"

#ifndef NOUI
#include "opengl/OpenGLCapabilities.h"
#endif // NOUI

// system includes:
#include <iostream>

// forward declarations:
class the_persp_view_mgr_t;
class the_ortho_view_mgr_t;


/*
  How this view manager works:
  
  |   . ^^^^^^^/.   |
  | .'      SR/  `. |
  |/   . '^^^/ .   \|
  |   /     /   \   |              LF (eye)
  |  |   LA+-----|--|--------------+
  |   \    |VR  /   |
  |\   ` ..|.. '   /|
  | `.           .' |
  |   ` -.....- '   |
  FAR               NEAR
  clipping          clipping
  plane             plane (the screen rests on this plane)
  
  LA - look-at point.
  LF - look-from point.
  VR - view radius.
  SR - scene radius.
  MR - model radius.
  
  Scene radius is the radius of the model bounding box,
  calculated with the center located at look-at.
  
  View radius is defined as a distance between LA and a
  point on the LF-LA vector at a fixed parameter value
  (in our case - 1/7th of the vector).
  
  The center of the model bounding box does not have to coincide
  with the look-at point. When it does, the model appears at
  the center of the view port.
  
  Zoom is defined as MR/VR.
  To zoom in - make VR smaller than MR (move LF towards LA).
  To zoom out - make VR larger than MR (move LF away from LA).
  
  The screen is laid out as follows:
  
      (0, 0)             (1, 0)
	UL-----------------UR
	 |                 |
	 |       V up      |
	 |        |        |
	 |        Z-- X    |
	 |        |        |
	 |        Y        |
	 |                 |
	LL-----------------LR
      (0, 1)             (1, 1)
  
  UL - upper left corner.
  UR - upper right corner.
  LL - lower left corner.
  LR - lower right corner.
  
  The viewing coordinate system is defined as follows:
  
  X = UR - UL // to the right.
  Y = LL - UL // down.
  Z = LA - LF // into the screen.
  O = LF      // the look from point.
*/

//----------------------------------------------------------------
// the_view_mgr_cb_t
// 
// The view manager callback function type:
// 
typedef void(*the_view_mgr_cb_t)(void *);

//----------------------------------------------------------------
// the_view_mgr_t
// 
class the_view_mgr_t
{
  friend bool save(std::ostream & stream, const the_view_mgr_t * view_mgr);
  friend bool load(std::istream & stream, the_view_mgr_t *& view_mgr);
  
public:

  the_view_mgr_t(const the_view_mgr_orientation_t & orientation,
		 the_view_mgr_cb_t cb,
		 void * data);
  virtual ~the_view_mgr_t() {}
  
  // clone yourself (used for undo):
  virtual the_view_mgr_t * clone() const = 0;
  
  // construct a new view of different type and return it:
  virtual the_view_mgr_t * other() const = 0;
  
  // optional callback specification:
  void set_callback(the_view_mgr_cb_t cb = NULL, void * data = NULL);
  
  //----------------------------------------------------------------
  // callback_suppressor_t
  // 
  class callback_suppressor_t
  {
  public:
    callback_suppressor_t(the_view_mgr_t * view_mgr):
      view_mgr_(view_mgr)
    { view_mgr_->set_callback_enabled(false); }

    ~callback_suppressor_t()
    { view_mgr_->set_callback_enabled(true); }
    
    the_view_mgr_t * view_mgr_;
  };
  
  // callback control:
  void set_callback_enabled(const bool & enable);
  
  //----------------------------------------------------------------
  // callback_buffer_t
  // 
  class callback_buffer_t
  {
  public:
    callback_buffer_t(const callback_buffer_t & b):
      view_mgr_(b.view_mgr_)
    { view_mgr_->set_callback_buffering(true); }
    
    callback_buffer_t & operator = (const callback_buffer_t & b)
    {
      if (view_mgr_ != NULL)
      {
	view_mgr_->set_callback_buffering(false);
      }
      
      view_mgr_ = b.view_mgr_;
      view_mgr_->set_callback_buffering(true);
      
      return *this;
    }
    
    callback_buffer_t(the_view_mgr_t * view_mgr):
      view_mgr_(view_mgr)
    { view_mgr_->set_callback_buffering(true); }
    
    ~callback_buffer_t()
    { view_mgr_->set_callback_buffering(false); }
    
    the_view_mgr_t * view_mgr_;
  };

  // callback buffering control:
  void set_callback_buffering(const bool & enable);
  
  // execute the callback (only if callback buffering is disabled):
  inline void callback() const
  {
    if (callback_ == NULL) return;
    if (callback_disabled_ > 0) return;
    
    if (callback_buffering_ == 0)
    {
      callback_(callback_data_);
      callback_buffer_size_ = 0;
    }
    else
    {
      callback_buffer_size_++;
    }
  }
  
  // accessor to the initial orientation of the view point:
  inline const the_view_mgr_orientation_t & initial_orientation() const
  { return initial_orientation_; }
  
  inline void set_initial_vp_orientation(const the_view_mgr_orientation_t & o)
  { initial_orientation_ = o; }
  
  // accessors to the initial zoom value:
  inline const float & initial_zoom() const
  { return initial_zoom_; }
  
  inline void set_initial_zoom(const float & initial_zoom)
  { initial_zoom_ = initial_zoom; }
  
  // the ratio of view-radius to the distance between lf-la points:
  inline float lfla_vr_ratio() const { return 7.0; }
  
  // restore each aspect of viewing to default:
  void restore_spin();
  void restore_zoom(const the_bbox_t & bbox);
  void restore_pan(const the_bbox_t & bbox);
  
  // reset the view to default orientation (restores LF/LA/UP):
  void reset(const the_bbox_t & bbox);
  
  // update the view setup:
  void update_scene_radius(const the_bbox_t & bbox);
  void update_view_radius(const the_bbox_t & bbox);
  
  // update look-from, look-at, up vector, width/height viewport data:
  void set_lf(const p3x1_t & lf);
  void set_la(const p3x1_t & la);
  void set_up(const v3x1_t & up);
  void resize(unsigned int w, unsigned int h);
  
  // set the viewsphere radius (zoom in/out):
  void set_view_radius(const float & vr);
  void set_scene_radius(const float & sr);
  void set_zoom(const float & zoom);
  
#ifndef NOUI
  // reset OpenGL viewing transformations to identity:
  void reset_opengl_viewing() const;
  
  // setup glFrustum for 2D viewing:
  void setup_opengl_2d_viewing(const p2x1_t & ll, const p2x1_t & ur) const;
  
  // setup glFrustum for specific near/far clipping plane values:
  virtual bool setup_opengl_3d_viewing(float near_plane,
				       float far_plane) const = 0;
  
  // setup glFrustum for near/far clipping planes dependent on the
  // current scene radius:
  inline bool setup_opengl_3d_viewing() const
  { return setup_opengl_3d_viewing(near_plane(), far_plane()); }
#endif // NOUI
  
  // setup a view volume:
  virtual void setup_view_volume(the_view_volume_t & view_volume) const = 0;
  
  // return a view volume snapshot of the current state of the view manager:
  inline const the_view_volume_t view_volume() const
  {
    the_view_volume_t view_volume;
    setup_view_volume(view_volume);
    return view_volume;
  }
  
  // setup a pick volume:
  void setup_pick_volume(the_view_volume_t & pick_volume,
			 const p2x1_t & scs_pt,
			 const float & pick_radius_pixels = 7.0) const;
  
  // return a pick volume:
  inline const the_view_volume_t
  pick_volume(const p2x1_t & scs_pt,
	      const float & pick_radius_pixels = 7.0) const
  {
    the_view_volume_t pick_volume;
    setup_pick_volume(pick_volume, scs_pt, pick_radius_pixels);
    return pick_volume;
  }
  
  // look from point:
  inline const p3x1_t & lf() const { return lf_; }
  
  // look at point:
  inline const p3x1_t & la() const { return la_; }
  
  // vector up:
  inline const v3x1_t & up() const { return up_; }
  
  // distance along normalized Z axis from LF to the near clipping plane:
  float near_plane() const;
  
  // distance along normalized Z axis from LF to the far clipping plane:
  float far_plane() const;
  
  // ensure that there is some distance between the near and far,
  // and check that they are usable (non-negative), otherwise clamp
  // them to some salvageable value so that at least some partial
  // scene can be rendered:
  static bool salvage_near_far(float & near_plane, float & far_plane);
  
  // construct a bounding box, such that when it is used on display
  // lists, its' minimum and maximum Z coordinates will correspond
  // exactly to the near and far clipping plane parameters necessary
  // to display the contents of the display list:
  const the_bbox_t near_far_bbox() const;
  
  // the real window dimensions:
  inline float width() const  { return width_; }
  inline float height() const { return height_; }
  
  // view sphere dimensions:
  inline float view_radius() const   { return view_radius_; }
  inline float view_diameter() const { return 2 * view_radius_; }
  
  // scene sphere dimensions:
  inline float scene_radius() const { return scene_radius_; }
  
  // return the radius of the plane at a given depth
  // along the normalized (la - lf) vector:
  virtual float viewport_plane_radius(float depth) const = 0;
  
  // functions calculating the current screen aspect ratio:
  inline float scale_x() const
  {
    if (width_ > height_) return width_ / height_;
    return 1;
  }
  
  inline float scale_y() const
  {
    if (width_ > height_) return 1;
    return height_ / width_;
  }
  
  // generic information one can expect to need to know about the view:
  static float pixels_per_millimeter();
  static float pixels_per_inch();
  
  // the units here are WCS units on the near plane (I think - 20040802):
  inline float wcs_units_per_pixel() const
  { return view_diameter() / (width_ / scale_x()); }
  
  inline float wcs_units_per_millimeter() const
  { return wcs_units_per_pixel() * pixels_per_millimeter(); }
  
  inline float wcs_units_per_inch() const
  { return wcs_units_per_pixel() * pixels_per_inch(); }
  
  // ray from look-from point to the look-at point:
  inline const the_ray_t lfla_ray() const
  { return the_ray_t(lf_, la_ - lf_); }
  
  // normalized ray from look-from point to the look-at point:
  inline const the_ray_t lfla_unit_ray() const
  { return the_ray_t(lf_, !(la_ - lf_)); }
  
  // for debugging:
  void dump(std::ostream & stream) const;
  
  // initial orientation of the view point (defaults to isometric):
  the_view_mgr_orientation_t initial_orientation_;
  
  // default zoom factor:
  float initial_zoom_;
  
  // Viewing setup:
  p3x1_t lf_; // look from point.
  p3x1_t la_; // look at point.
  v3x1_t up_; // vector up (so we know which way is up).    
  float width_; // screen width.
  float height_; // screen height.
  float view_radius_; // view sphere radius.
  float scene_radius_; // model view sphere radius.
  
  // if the callback is not NULL it will be called every time the view changes,
  // depending on callback buffering:
  the_view_mgr_cb_t callback_;
  void * callback_data_;
  
  // flag controlling whether the callback will be executed --
  // when the flag is non zero the callback is disabled:
  unsigned int callback_disabled_;
  
  // flag controlling callback buffering -- turning off callback
  // buffering when the buffer size is non-zero will trigger the callback:
  unsigned int callback_buffering_;
  
  // current buffer size:
  mutable unsigned int callback_buffer_size_;
  
  // a cached bounding box of the scene:
  the_bbox_t scene_bbox_;
};


//----------------------------------------------------------------
// the_persp_view_mgr_t:
// 
class the_persp_view_mgr_t: public the_view_mgr_t
{
public:
  the_persp_view_mgr_t(const the_view_mgr_orientation_t & init =
		       THE_ISOMETRIC_VIEW_E,
		       the_view_mgr_cb_t cb = NULL,
		       void * data = NULL);
  
  the_persp_view_mgr_t(const the_persp_view_mgr_t & vm);
  the_persp_view_mgr_t(const the_ortho_view_mgr_t & vm);
  
  // virtual: clone yourself (used for undo):
  the_view_mgr_t * clone() const
  { return new the_persp_view_mgr_t(*this); }
  
  // virtual: construct a new view of orthonormal type and return it:
  the_view_mgr_t * other() const;
  
#ifndef NOUI
  // virtual: Open/GL stuff:
  bool setup_opengl_3d_viewing(float near_plane,
			       float far_plane) const;
#endif // NOUI
  
  // virtual:
  void setup_view_volume(the_view_volume_t & view_volume) const;
  
  // virtual:
  float viewport_plane_radius(float depth) const;
};


//----------------------------------------------------------------
// the_ortho_view_mgr_t:
// 
class the_ortho_view_mgr_t: public the_view_mgr_t
{
public:
  the_ortho_view_mgr_t(const the_view_mgr_orientation_t & init =
		       THE_ISOMETRIC_VIEW_E,
		       the_view_mgr_cb_t cb = NULL,
		       void * data = NULL);
  
  the_ortho_view_mgr_t(const the_ortho_view_mgr_t & vm);
  the_ortho_view_mgr_t(const the_persp_view_mgr_t & vm);
  
  // virtual: clone yourself (used for undo):
  the_view_mgr_t * clone() const
  { return new the_ortho_view_mgr_t(*this); }
  
  // virtual: construct a new view of perspective type and return it:
  the_view_mgr_t * other() const;
  
#ifndef NOUI
  // virtual: Open/GL stuff:
  bool setup_opengl_3d_viewing(float near_plane,
			       float far_plane) const;
#endif // NOUI
  
  // virtual:
  void setup_view_volume(the_view_volume_t & view_volume) const;
  
  // virtual:
  float viewport_plane_radius(float depth) const;
};


#endif // THE_VIEW_MGR_HXX_
