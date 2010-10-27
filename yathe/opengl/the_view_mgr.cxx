// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_view_mgr.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Helper classes used to setup/evaluate the view volume.

// local includes:
#include "opengl/the_view_mgr.hxx"
#include "math/the_coord_sys.hxx"
#include "ui/the_desktop_metrics.hxx"

// system includes:
#include <algorithm>
#include <utility>
#include <limits>


//----------------------------------------------------------------
// the_view_mgr_t::the_view_mgr_t
// 
the_view_mgr_t::the_view_mgr_t(const the_view_mgr_orientation_t & orientation,
			       the_view_mgr_cb_t cb,
			       void * data):
  initial_orientation_(orientation),
  initial_zoom_(1.0),
  la_(0.0, 0.0, 0.0),
#ifndef NOUI
  stereo_(NOT_STEREOSCOPIC_E),
  cursor_id_(THE_BLANK_CURSOR_E),
#endif
  width_(1.0),
  height_(1.0),
  scene_radius_(1.0),
  callback_(cb),
  callback_data_(data),
  callback_disabled_(0),
  callback_buffering_(0),
  callback_buffer_size_(0)
{
  view_radius_ = initial_zoom_ * scene_radius_;
  
  lf_ = la_ + (lfla_vr_ratio() * view_radius_ *
	       THE_ORIENTATION_LF[initial_orientation_]);
  
  up_ = THE_ORIENTATION_UP[initial_orientation_];
}

//----------------------------------------------------------------
// the_view_mgr_t::set_callback
// 
void
the_view_mgr_t::set_callback(the_view_mgr_cb_t cb, void * data)
{
  callback_ = cb;
  callback_data_ = data;
}

//----------------------------------------------------------------
// the_view_mgr_t::set_callback_enabled
// 
void
the_view_mgr_t::set_callback_enabled(const bool & enable)
{
  if (enable)
  {
    assert(callback_disabled_ > 0);
    callback_disabled_--;
  }
  else
  {
    callback_disabled_++;
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_callback_buffering
// 
void
the_view_mgr_t::set_callback_buffering(const bool & enable)
{
  if (enable)
  {
    callback_buffering_++;
  }
  else
  {
    assert(callback_buffering_ > 0);
    
    callback_buffering_--;
    if (callback_buffering_ == 0 && callback_buffer_size_ > 0)
    {
      callback();
    }
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::restore_spin
// 
void
the_view_mgr_t::restore_spin()
{
  lf_ = la_ + (lfla_vr_ratio() * view_radius_ *
	       THE_ORIENTATION_LF[initial_orientation_]);
  up_ = THE_ORIENTATION_UP[initial_orientation_];
  
  callback();
}

//----------------------------------------------------------------
// calc_scene_radius
// 
static double
calc_scene_radius(const the_bbox_t & bbox,
		  const double width,
		  const double height)
{
  if (bbox.is_empty() || bbox.is_singular())
  {
    return 1.0;
  }

#if 0
  return bbox.radius();
#else
  // project the bbox onto the viewport axis:
  the_bbox_t xy_bbox;
  xy_bbox += bbox;
  p3x1_t xy_min = xy_bbox.wcs_min();
  p3x1_t xy_max = xy_bbox.wcs_max();
  
  double w = xy_max[0] - xy_min[0];
  double h = xy_max[1] - xy_min[1];
  double wr = w / double(width);
  double hr = h / double(height);
  double scale_x = (width > height) ? width / height : 1.0;
  double scale_y = (width > height) ? 1.0 : height / width;
  
#if 0
  cerr << "width: " << width << endl
       << "height: " << height << endl
       << "w: " << w << endl
       << "h: " << h << endl
       << "wr: " << wr << endl
       << "hr: " << hr << endl
       << endl;
#endif
  
  double d = (wr > hr) ? w / scale_x : h / scale_y;
  return d / 2.0;
#endif
}

//----------------------------------------------------------------
// the_view_mgr_t::restore_zoom
// 
void
the_view_mgr_t::restore_zoom(const the_bbox_t & bbox)
{
  scene_bbox_ = bbox;
  scene_radius_ = float(calc_scene_radius(scene_bbox_, width_, height_));
  view_radius_ = initial_zoom_ * scene_radius_;
  
  lf_ = la_ + (lfla_vr_ratio() * view_radius_ *
	       THE_ORIENTATION_LF[initial_orientation_]);
  callback();
}

//----------------------------------------------------------------
// the_view_mgr_t::restore_pan
// 
void
the_view_mgr_t::restore_pan(const the_bbox_t & bbox)
{
  scene_bbox_ = bbox;
  p3x1_t center(0.0, 0.0, 0.0);
  if (scene_bbox_.is_empty() == false)
  {
    center = scene_bbox_.wcs_center();
  }
  
  v3x1_t pan = la_ - center;
  la_ -= pan;
  lf_ -= pan;
  
  callback();
}

//----------------------------------------------------------------
// the_view_mgr_t::reset
// 
void
the_view_mgr_t::reset(const the_bbox_t & bbox)
{
  // avoid multiple callbacks:
  callback_buffer_t buffer(this);

  scene_bbox_ = bbox;
  restore_pan(scene_bbox_);
  restore_spin();
  restore_zoom(scene_bbox_);
}

//----------------------------------------------------------------
// the_view_mgr_t::update_scene_radius
// 
void
the_view_mgr_t::update_scene_radius(const the_bbox_t & bbox)
{
  if (bbox.is_empty() || bbox.is_singular()) return;
  scene_bbox_ = bbox;
#if 1
  // for 3D
  float sr = scene_bbox_.wcs_radius(la_);
#else
  // for 2D
  float shift = ~(scene_bbox_.wcs_center() - la_);
  float sr = shift + float(calc_scene_radius(scene_bbox_, width_, height_));
#endif
  
  set_scene_radius(sr);
}

//----------------------------------------------------------------
// the_view_mgr_t::update_view_radius
// 
void
the_view_mgr_t::update_view_radius(const the_bbox_t & bbox)
{
  if (bbox.is_empty() || bbox.is_singular()) return;
  scene_bbox_ = bbox;
  
  // avoid multiple callbacks:
  callback_buffer_t buffer(this);

  v3x1_t lf_unit_vec = !(lf_ - la_);
  float zoom = scene_radius_ / view_radius_;
  
  update_scene_radius(scene_bbox_);
  view_radius_ = scene_radius_ / zoom;
  
  lf_ = la_ + lfla_vr_ratio() * view_radius_ * lf_unit_vec;
}

//----------------------------------------------------------------
// the_view_mgr_t::set_lf
// 
void
the_view_mgr_t::set_lf(const p3x1_t & LF)
{
  if (lf_ != LF)
  {
    lf_ = LF;
    callback();
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_la
// 
void
the_view_mgr_t::set_la(const p3x1_t & LA)
{
  if (la_ != LA)
  {
    la_ = LA;
    callback();
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_up
// 
void
the_view_mgr_t::set_up(const v3x1_t & UP)
{
  if (up_ != UP)
  {
    up_ = UP;
    callback();
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::resize
// 
void
the_view_mgr_t::resize(unsigned int w, unsigned int h)
{
  if (width_ != float(w) || height_ != float(h))
  {
    width_ =  float(w);
    height_ = float(h);
#if 0
    callback();
#else
    update_view_radius(scene_bbox_);
#endif
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_view_radius
// 
void
the_view_mgr_t::set_view_radius(const float & vr)
{
  static float vr_limit_min =
    ::sqrt(std::numeric_limits<float>::min() * 1.01f) / lfla_vr_ratio();
  
  static float vr_limit_max =
    ::sqrt(std::numeric_limits<float>::max() * 0.99f) / lfla_vr_ratio();
  
  if (view_radius_ != vr /* && vr >= vr_limit_min && vr <= vr_limit_max */)
  {
    // view_radius_ = vr;
    view_radius_ = std::max(std::min(vr, vr_limit_max), vr_limit_min);
    lf_ = (la_ - !(la_ - lf_) * lfla_vr_ratio() * view_radius_);
    callback();
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_scene_radius
// 
void
the_view_mgr_t::set_scene_radius(const float & sr)
{
  if (scene_radius_ != sr)
  {
    scene_radius_ = sr;
    callback();
  }
}

//----------------------------------------------------------------
// the_view_mgr_t::set_zoom
// 
void
the_view_mgr_t::set_zoom(const float & zoom)
{
  set_view_radius(scene_radius_ / zoom);
}

#ifndef NOUI
//----------------------------------------------------------------
// the_view_mgr_t::reset_opengl_viewing
// 
void
the_view_mgr_t::reset_opengl_viewing() const
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

//----------------------------------------------------------------
// the_view_mgr_t::setup_opengl_2d_viewing
// 
void
the_view_mgr_t::setup_opengl_2d_viewing(const p2x1_t & ll,
					const p2x1_t & ur) const
{
  if (stereo_ == STEREOSCOPIC_LEFT_EYE_E)
  {
    int w0 = int(width() / 2.0);
    glViewport(0, 0, w0, int(height()));
  }
  else if (stereo_ == STEREOSCOPIC_RIGHT_EYE_E)
  {
    int w0 = int(width() / 2.0);
    int w1 = int(width()) - w0;
    glViewport(w0, 0, w1, int(height()));
  }
  else
  {
    glViewport(0, 0, int(width()), int(height()));
  }
  
  glMatrixMode(GL_PROJECTION);
  gluOrtho2D(ll.x(), ur.x(), ll.y(), ur.y());
}
#endif // NOUI

//----------------------------------------------------------------
// the_view_mgr_t::setup_pick_volume
// 
void
the_view_mgr_t::setup_pick_volume(the_view_volume_t & pick_volume,
				  const p2x1_t & scs_pt,
				  const float & pick_radius_pixels) const
{
  the_view_volume_t view_volume;
  setup_view_volume(view_volume);
  
  const float x_offset = pick_radius_pixels / width();
  const float y_offset = pick_radius_pixels / height();
  view_volume.sub_volume(scs_pt,
			 scs_pt + v2x1_t(x_offset, y_offset),
			 pick_volume);
}

//----------------------------------------------------------------
// the_view_mgr_t::near_plane
// 
float
the_view_mgr_t::near_plane() const
{
  float lfla_dist = ~(la_ - lf_);
  float temp = lfla_dist - scene_radius_;
  
  if (temp <= 0.0f)
  {
    // make sure the clipping plane stays in front of the nose:
    temp = lfla_dist * 0.02f;
  }
  else if (temp == lfla_dist)
  {
    // avoid numerical precision problems:
    temp = lfla_dist * 0.99f;
  }
  
  return temp;
}

//----------------------------------------------------------------
// the_view_mgr_t::far_plane
// 
float
the_view_mgr_t::far_plane() const
{
  float lfla_dist = ~(la_ - lf_);
  float temp = lfla_dist + scene_radius_;
  
  if (temp == lfla_dist)
  {
    // avoid numerical precision problems:
    temp = lfla_dist * 1.01f;
  }
  
  return temp;
}

//----------------------------------------------------------------
// the_view_mgr_t::salvage_near_far
// 
bool
the_view_mgr_t::salvage_near_far(float & near_plane, float & far_plane)
{
  // make sure that the near clipping is salvageable:
  if (far_plane <= 0.0f) return false;
  if (far_plane < near_plane) return false;
  
  // try to salvage the near clipping plane:
  if (near_plane <= 0.0f) near_plane = 0.01f * far_plane;
  
  // separate the clipping planes:
  float offset = 0.01f * near_plane;
  far_plane  += offset;
  near_plane -= offset;
  
  return true;
}

//----------------------------------------------------------------
// the_view_mgr_t::near_far_bbox
// 
const the_bbox_t
the_view_mgr_t::near_far_bbox() const
{
  // constuct a coordinate system such that its' Z axis corresponds
  // to the look-at vector:
  const p3x1_t & origin = lf();
  v3x1_t z_axis = !(la() - lf());
  v3x1_t y_axis = -up();
  v3x1_t x_axis = !(y_axis % z_axis);
  return the_bbox_t(the_coord_sys_t(x_axis, y_axis, z_axis, origin));
}

//----------------------------------------------------------------
// the_view_mgr_t::pixels_per_millimeter
// 
float
the_view_mgr_t::pixels_per_millimeter()
{
  const the_desktop_metrics_t * dm = the_desktop_metrics();
  return dm->pixels_per_millimeter();
}

//----------------------------------------------------------------
// the_view_mgr_t::pixels_per_inch
// 
float
the_view_mgr_t::pixels_per_inch()
{
  const the_desktop_metrics_t * dm = the_desktop_metrics();
  return dm->pixels_per_inch();
}

//----------------------------------------------------------------
// the_view_mgr_t::dump
// 
void
the_view_mgr_t::dump(std::ostream & so) const
{
  so << "initial_orientation_	: " << initial_orientation_ << endl
     << "initial_zoom_		: " << initial_zoom_ << endl
     << "lf_ " << lf_ << endl
     << "la_ " << la_ << endl
     << "up_ " << up_ << endl
     << "width_		: " << width_ << endl
     << "height_	: " << height_ << endl
     << "view_radius_	: " << view_radius_ << endl
     << "scene_radius_	: " << scene_radius_ << endl;
}


//----------------------------------------------------------------
// the_persp_view_mgr_t::the_persp_view_mgr_t
// 
the_persp_view_mgr_t::
the_persp_view_mgr_t(const the_view_mgr_orientation_t & init,
		     the_view_mgr_cb_t cb,
		     void * data):
  the_view_mgr_t(init, cb, data)
{}

//----------------------------------------------------------------
// the_persp_view_mgr_t::the_persp_view_mgr_t
// 
the_persp_view_mgr_t::the_persp_view_mgr_t(const the_persp_view_mgr_t & vm):
  the_view_mgr_t(vm)
{}

//----------------------------------------------------------------
// the_persp_view_mgr_t::the_persp_view_mgr_t
// 
the_persp_view_mgr_t::the_persp_view_mgr_t(const the_ortho_view_mgr_t & vm):
  the_view_mgr_t(vm)
{}

//----------------------------------------------------------------
// the_persp_view_mgr_t::other
// 
the_view_mgr_t *
the_persp_view_mgr_t::other() const
{
  return new the_ortho_view_mgr_t(*this);
}

#ifndef NOUI
//----------------------------------------------------------------
// the_persp_view_mgr_t::setup_opengl_3d_viewing
// 
bool
the_persp_view_mgr_t::setup_opengl_3d_viewing(float near_plane,
					      float far_plane) const
{
  if (salvage_near_far(near_plane, far_plane) == false) return false;

  float vr = view_radius();
  if (stereo_ == STEREOSCOPIC_LEFT_EYE_E)
  {
    int w0 = int(width_ / 2.0);
    glViewport(0, 0, w0, (int)height_);
    
    v3x1_t la = !(la_ - lf_);
    v3x1_t le = (!(up_ % la)) * vr * 0.1f;
    p3x1_t lf = lf_ + le;
    gluLookAt(lf.x(), lf.y(), lf.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  else if (stereo_ == STEREOSCOPIC_RIGHT_EYE_E)
  {
    int w0 = int(width_ / 2.0);
    int w1 = int(width_) - w0;
    glViewport(w0, 0, w1, (int)height_);
  
    v3x1_t la = !(la_ - lf_);
    v3x1_t le = (!(up_ % la)) * vr * 0.1f;
    p3x1_t lf = lf_ - le;
    gluLookAt(lf.x(), lf.y(), lf.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  else
  {
    glViewport(0, 0, (int)width_, (int)height_);
    
    gluLookAt(lf_.x(), lf_.y(), lf_.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  
  glMatrixMode(GL_PROJECTION);
  glFrustum(-viewport_plane_radius(near_plane) * scale_x(), // left
	    viewport_plane_radius(near_plane)  * scale_x(), // right
	    -viewport_plane_radius(near_plane) * scale_y(), // bottom
	    viewport_plane_radius(near_plane)  * scale_y(), // top
	    near_plane,
	    far_plane);
  
#ifdef DEBUG_VIEWING
  cerr
    << "glFrustum(" << -viewport_plane_radius(near_plane) * scale_x() << ",\n"
    << "          " << viewport_plane_radius(near_plane)  * scale_x() << ",\n"
    << "          " << -viewport_plane_radius(near_plane) * scale_y() << ",\n"
    << "          " << viewport_plane_radius(near_plane)  * scale_y() << ",\n"
    << "          " << near_plane << ",\n"
    << "          " << far_plane << ")\n" << endl;
#endif // DEBUG_VIEWING
  
  return true;
}
#endif // NOUI

//----------------------------------------------------------------
// the_persp_view_mgr_t::setup_view_volume
// 
void
the_persp_view_mgr_t::setup_view_volume(the_view_volume_t & view_volume) const
{
  view_volume.setup(lf_,
		    la_,
		    up_,
		    near_plane(),
		    far_plane(),
		    viewport_plane_radius(near_plane()) * scale_x() * 2.0f,
		    viewport_plane_radius(near_plane()) * scale_y() * 2.0f,
		    true);
}

//----------------------------------------------------------------
// the_persp_view_mgr_t::viewport_plane_radius
// 
float
the_persp_view_mgr_t::viewport_plane_radius(float depth) const
{
  float t = depth / ~(la_ - lf_);
  return view_radius() * t;
}


//----------------------------------------------------------------
// the_ortho_view_mgr_t::the_ortho_view_mgr_t
// 
the_ortho_view_mgr_t::
the_ortho_view_mgr_t(const the_view_mgr_orientation_t & init,
		     the_view_mgr_cb_t cb,
		     void * data):
  the_view_mgr_t(init, cb, data)
{}

//----------------------------------------------------------------
// the_ortho_view_mgr_t::the_ortho_view_mgr_t
// 
the_ortho_view_mgr_t::the_ortho_view_mgr_t(const the_ortho_view_mgr_t & vm):
  the_view_mgr_t(vm)
{}

//----------------------------------------------------------------
// the_ortho_view_mgr_t::the_ortho_view_mgr_t
// 
the_ortho_view_mgr_t::the_ortho_view_mgr_t(const the_persp_view_mgr_t & vm):
  the_view_mgr_t(vm)
{}

//----------------------------------------------------------------
// the_ortho_view_mgr_t::other
// 
the_view_mgr_t *
the_ortho_view_mgr_t::other() const
{
  return new the_persp_view_mgr_t(*this);
}

#ifndef NOUI
//----------------------------------------------------------------
// the_ortho_view_mgr_t::setup_opengl_3d_viewing
// 
bool
the_ortho_view_mgr_t::setup_opengl_3d_viewing(float near_plane,
					      float far_plane) const
{
  if (salvage_near_far(near_plane, far_plane) == false) return false;
  
  float vr = view_radius();
  if (stereo_ == STEREOSCOPIC_LEFT_EYE_E)
  {
    int w0 = int(width_ / 2.0);
    glViewport(0, 0, w0, (int)height_);

    v3x1_t la = !(la_ - lf_);
    v3x1_t le = (!(up_ % la)) * vr * 0.1f;
    p3x1_t lf = lf_ + le;
    gluLookAt(lf.x(), lf.y(), lf.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  else if (stereo_ == STEREOSCOPIC_RIGHT_EYE_E)
  {
    int w0 = int(width_ / 2.0);
    int w1 = int(width_) - w0;
    glViewport(w0, 0, w1, (int)height_);

    v3x1_t la = !(la_ - lf_);
    v3x1_t le = (!(up_ % la)) * vr * 0.1f;
    p3x1_t lf = lf_ - le;
    gluLookAt(lf.x(), lf.y(), lf.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  else
  {
    glViewport(0, 0, (int)width_, (int)height_);
    
    gluLookAt(lf_.x(), lf_.y(), lf_.z(),
              la_.x(), la_.y(), la_.z(),
              up_.x(), up_.y(), up_.z());
  }
  
  glMatrixMode(GL_PROJECTION);
  glOrtho(-viewport_plane_radius(near_plane) * scale_x(), // left
	  viewport_plane_radius(near_plane)  * scale_x(), // right
	  -viewport_plane_radius(near_plane) * scale_y(), // bottom
	  viewport_plane_radius(near_plane)  * scale_y(), // top
	  near_plane,
	  far_plane);
  
#ifdef DEBUG_VIEWING
  cerr
    << "glOrtho(" << -viewport_plane_radius(near_plane) * scale_x() << ",\n"
    << "        " << viewport_plane_radius(near_plane)  * scale_x() << ",\n"
    << "        " << -viewport_plane_radius(near_plane) * scale_y() << ",\n"
    << "        " << viewport_plane_radius(near_plane)  * scale_y() << ",\n"
    << "        " << near_plane << ",\n"
    << "        " << far_plane << ")\n" << endl;
#endif // DEBUG_VIEWING
  
  return true;
}
#endif // NOUI

//----------------------------------------------------------------
// the_ortho_view_mgr_t::setup_view_volume
// 
void
the_ortho_view_mgr_t::setup_view_volume(the_view_volume_t & view_volume) const
{
  float n = near_plane();
  float f = far_plane();
  float r = viewport_plane_radius(n);
  float w = r * (scale_x() * 2.0f);
  float h = r * (scale_y() * 2.0f);
  view_volume.setup(lf_, la_, up_, n, f, w, h, false);
}

//----------------------------------------------------------------
// the_ortho_view_mgr_t::viewport_plane_radius
// 
float
the_ortho_view_mgr_t::viewport_plane_radius(float /* depth */) const
{
  return view_radius();
}
