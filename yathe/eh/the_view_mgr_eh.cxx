// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_view_mgr_eh.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Mon Jul  1 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The OpenGL view manager mouse/keyboard/tablet event handler.

// local includes:
#include "eh/the_view_mgr_eh.hxx"
#include "ui/the_trail.hxx"
#include "doc/the_procedure.hxx"
#include "opengl/the_view.hxx"
#include "opengl/the_view_mgr.hxx"
#include "math/the_bbox.hxx"

// system includes:
#include <iostream>

using std::cerr;
using std::endl;


//----------------------------------------------------------------
// the_view_mgr_eh_t::the_view_mgr_eh_t
// 
the_view_mgr_eh_t::the_view_mgr_eh_t():
  the_input_device_eh_t(),
  view_mgr_eh_state_(THE_VIEW_MGR_EH_HAS_NOTHING_E),
  which_axis_(AXIS_UNDEFINED_E),
  pick_depth_(0.0),
  allow_spin_(true),
  allow_zoom_(true),
  allow_pan_(true),
  spin_constraint_(SPIN_UNCONSTRAINED_E)
{}

//----------------------------------------------------------------
// the_view_mgr_eh_t::~the_view_mgr_eh_t
// 
the_view_mgr_eh_t::~the_view_mgr_eh_t()
{}

//----------------------------------------------------------------
// the_view_mgr_eh_t::view_handler
// 
bool
the_view_mgr_eh_t::view_handler(the_view_t * view)
{
  view->refresh();
  return true;
}

//----------------------------------------------------------------
// the_view_mgr_eh_t::is_modifier_active_locally
// 
bool
the_view_mgr_eh_t::is_modifier_active_locally(the_eh_modifier_t mod) const
{
  switch (mod)
  {
    case THE_EH_MOD_VIEW_SPIN_E:
      return ((state() != THE_EH_NOT_READY_E) &&
	      THE_MOUSE.verify_pressed_btns(THE_MOUSE.l_btn()) &&
	      THE_KEYBD.verify_pressed_keys(THE_KEYBD.ascii('C')));
      
    case THE_EH_MOD_VIEW_ZOOM_E:
      return
	((state() != THE_EH_NOT_READY_E) &&
	 ((THE_MOUSE.verify_pressed_btns(THE_MOUSE.l_btn()) &&
	   THE_KEYBD.verify_pressed_keys(THE_KEYBD.ascii('Z'))) ||
	  (THE_MOUSE.verify_pressed_btns(THE_MOUSE.m_btn()) &&
	   THE_KEYBD.verify_no_pressed_keys())));
      
    case THE_EH_MOD_VIEW_PAN_E:
      return ((state() != THE_EH_NOT_READY_E) &&
	      ((THE_MOUSE.verify_pressed_btns(THE_MOUSE.l_btn()) &&
		THE_KEYBD.verify_pressed_keys(THE_KEYBD.ascii('X'))) ||
	       (THE_MOUSE.verify_pressed_btns(THE_MOUSE.l_btn()) &&
		THE_KEYBD.verify_no_pressed_keys())));
      
    case THE_EH_MOD_VIEWING_E:
      return (is_modifier_active(this, THE_EH_MOD_VIEW_SPIN_E) ||
	      is_modifier_active(this, THE_EH_MOD_VIEW_ZOOM_E) ||
	      is_modifier_active(this, THE_EH_MOD_VIEW_PAN_E));
      
    default:
      break;
  }
  
  return the_input_device_eh_t::is_modifier_active_locally(mod);
}

//----------------------------------------------------------------
// the_view_mgr_eh_t::ep_offset
// 
float
the_view_mgr_eh_t::ep_offset(the_view_t * view) const
{
  if (picks_.empty()) return 0.0;
  
  the_ray_t ep_ray(ep_origin(view), ep_normal(view));
  p3x1_t pick_pt = picks_.front().pick_pt();
  return ep_ray * pick_pt;
}

//----------------------------------------------------------------
// the_view_mgr_eh_t::processor
// 
bool
the_view_mgr_eh_t::
processor(the_input_device_eh_t::the_event_type_t event_type)
{
  the_view_mgr_eh_state_t prev_state = view_mgr_eh_state_;
  
  // viewing is done via keyboard and mouse, ignore any other events:
  if (event_type != THE_MOUSE_EVENT_E &&
      event_type != THE_KEYBD_EVENT_E &&
      event_type != THE_WHEEL_EVENT_E)
  {
    return false;
  }
  
  // FIXME: this should be done when the event handler is installed:
  helper_.set_view(&view());
  
  if (event_type == THE_WHEEL_EVENT_E &&
      view_mgr_eh_state_ == THE_VIEW_MGR_EH_HAS_NOTHING_E)
  {
    the_trail_t::milestone_t milestone;
    
    // handle the linked views:
    std::list<view_mgr_helper_t> views = linked_views_;
    views.push_front(helper_);
    
    for (std::list<view_mgr_helper_t>::iterator i = views.begin();
	 i != views.end(); ++i)
    {
      view_mgr_helper_t & helper = *i;
      
      // reset the view manager anchor:
      helper.set_anchor();
      
      the_view_mgr_t & view_mgr = helper.view_mgr();
      the_view_mgr_t & anchor = helper.anchor();
      the_view_mgr_t::callback_buffer_t buffer(&view_mgr);
      
      const the_wheel_event_t & we = most_recent_wheel_event();
      float vr = anchor.view_radius();
      double dr = we.degrees_rotated();
      
      if (dr > 0.0)
      {
	vr *= float((180.0 - dr) / 180.0);
      }
      else
      {
	vr *= float(180.0 / (180.0 + dr));
      }
      
      float vr_threshold = ~(view_mgr.lf() - view_mgr.la()) / 100.0f;
      if (vr > vr_threshold)
      {
	view_mgr.set_view_radius(vr);
	
	// make sure we did not move the look-from through the near plane:
	if (view_mgr.near_plane() == 0.0f)
	{
	  cerr << "WARNING: impossible viewing, restoring previous state:"
	       << endl
	       << "vr: " << view_mgr.view_radius() << endl
	       << "sr: " << view_mgr.scene_radius() << endl
	       << endl;
	  helper.restore();
	}
	
	helper.view().select_ep();
      }
    }
    
    return true;
  }
  
  if (event_type == THE_KEYBD_EVENT_E &&
      most_recent_keybd_event().autorepeat())
  {
    return true;
  }
  
  // get the pressed mouse button -- this is necessary since any
  // of the mouse buttons may be used for the view management, according
  // to the local active modifier function of other event handlers
  // higher up on the stack:
  std::list<the_mouse_btn_t> pressed_btns;
  unsigned int num_pressed_btns = THE_MOUSE.collect_pressed_btns(pressed_btns);
  const the_mouse_btn_t * pressed_btn = NULL;
  if (event_type == THE_MOUSE_EVENT_E && num_pressed_btns == 1)
  {
    pressed_btn = &(pressed_btns.front());
  }
  
  // A short cut to the view manager:
  the_view_mgr_t & view_mgr = helper_.view_mgr();
  
  // a callback buffer for the primary view manager:
  the_view_mgr_t::callback_buffer_t buffer(&view_mgr);
  
  // callback buffers for the linked view managers:
  std::list<the_view_mgr_t::callback_buffer_t> buffers;
  for (std::list<view_mgr_helper_t>::iterator i = linked_views_.begin();
       i != linked_views_.end(); ++i)
  {
    view_mgr_helper_t & helper = *i;
    buffers.push_back(the_view_mgr_t::callback_buffer_t(&(helper.view_mgr())));
  }
  
  if (view_mgr_eh_state_ == THE_VIEW_MGR_EH_HAS_NOTHING_E)
  {
    // reset the view manager anchor:
    helper_.set_anchor();
    
    // reset the linked view manager anchors:
    for (std::list<view_mgr_helper_t>::iterator i = linked_views_.begin();
	 i != linked_views_.end(); ++i)
    {
      view_mgr_helper_t & helper = *i;
      helper.set_anchor();
    }
    
    if (spin_allowed() &&
	is_modifier_active(this, THE_EH_MOD_VIEW_SPIN_E) &&
	pressed_btn != NULL &&
	pressed_btn->changed())
    {
      the_trail_t::milestone_t milestone;
      
      // move look-from point around the look-at point:
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_SPIN_E;
      
      p3x1_t view_radius_pt = (helper_.anchor().la() +
			       helper_.anchor().view_radius() *
			       !(helper_.anchor().lf() -
				 helper_.anchor().la()));
      pick_depth_ =
	helper_.anchor().view_volume().depth_of_wcs_pt(view_radius_pt);
    }
    else if (zoom_allowed() &&
	     is_modifier_active(this, THE_EH_MOD_VIEW_ZOOM_E) &&
	     pressed_btn != NULL &&
	     pressed_btn->changed())
    {
      the_trail_t::milestone_t milestone;
      
      // zoom in/out (move look-from point toward/away from the look-at point):
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_ZOOM_E;
      
      which_axis_ = AXIS_UNDEFINED_E;
    }
    else if (pan_allowed() &&
	     is_modifier_active(this, THE_EH_MOD_VIEW_PAN_E) &&
	     pressed_btn != NULL &&
	     pressed_btn->changed())
    {
      the_trail_t::milestone_t milestone;
      
      // move look-from and look-at simultaneously:
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_PAN_E;
      
      ray_a_ = view_mgr.view_volume().to_ray(scs_pt());
      
      // check if the user whats to pan about particular geometry:
      picks_.pick(view(), scs_pt());
      if (!picks_.empty())
      {
	pick_depth_ = picks_.front().data().depth();
      }
      else
      {
	pick_depth_ = ray_a_ * view_mgr.la();
      }
      
      start_pnt_ = ray_a_ * pick_depth_;
    }
  }
  else if (view_mgr_eh_state_ == THE_VIEW_MGR_EH_HAS_SPIN_E)
  {
    if (spin_allowed() &&
	is_modifier_active(this, THE_EH_MOD_VIEW_SPIN_E))
    {
      the_trail_t::milestone_t milestone;
      
      // handle the linked views:
      std::list<view_mgr_helper_t> views = linked_views_;
      views.push_front(helper_);
      
      for (std::list<view_mgr_helper_t>::iterator i = views.begin();
	   i != views.end(); ++i)
      {
	view_mgr_helper_t & helper = *i;
	the_view_mgr_t & view_mgr = helper.view_mgr();
	the_view_mgr_t & anchor = helper.anchor();
	
	the_view_volume_t view_vol;
	view_mgr.setup_view_volume(view_vol);
	
	the_view_t & view = helper.view();
	const the_plane_t & ep = view.active_ep();
	
	if (spin_constraint() == SPIN_EDIT_PLANE_E)
	{
	  const the_ray_t a_ray(view_vol.to_ray(a_scs()));
	  const the_ray_t b_ray(view_vol.to_ray(b_scs()));
	  float ta = FLT_MAX;
	  float tb = FLT_MAX;
	  if (!ep.intersect(a_ray, ta) ||
	      !ep.intersect(b_ray, tb))
	  {
	    continue;
	  }
	  
	  p3x1_t pa_wcs = a_ray * ta;
	  p3x1_t pb_wcs = b_ray * tb;
	  
	  // find center of rotation on the edit plane:
	  const the_ray_t lfla_ray = view_mgr.lfla_ray();
	  
	  float ray_ep_param = FLT_MAX;
	  if (!ep.intersect(lfla_ray, ray_ep_param))
	  {
	    continue;
	  }

	  the_coord_sys_t cs(ep.cs());
	  cs.origin() = lfla_ray * ray_ep_param;
	  
	  // NOTE: cylindrical coordinates should be interpreted as follows:
	  // 
	  // 0: radius, distance from the Z origin
	  // 1: angle, measured in XY plane from X axis
	  // 2: height, measured from XY plane along the Z axis
	  
	  p3x1_t a_cyl;
	  p3x1_t b_cyl;
	  cs.wcs_to_cyl(pa_wcs, a_cyl);
	  cs.wcs_to_cyl(pb_wcs, b_cyl);
	  
	  float rotation_angle = b_cyl[1] - a_cyl[1];
	  
	  // update look from point and up vector:
	  p3x1_t lf_cyl;
	  cs.wcs_to_cyl(anchor.lf(), lf_cyl);
	  lf_cyl[1] -= rotation_angle;
	  
	  p3x1_t up_cyl;
	  cs.wcs_to_cyl(anchor.lf() + anchor.up(), up_cyl);
	  up_cyl[1] -= rotation_angle;
	  
	  p3x1_t lf_wcs;
	  cs.cyl_to_wcs(lf_cyl, lf_wcs);
	  
	  p3x1_t up_wcs;
	  cs.cyl_to_wcs(up_cyl, up_wcs);
	  
	  v3x1_t up_vec = !(up_wcs - lf_wcs);
	  
	  view_mgr.set_lf(lf_wcs);
	  view_mgr.set_up(up_vec);
	  view.select_ep();
	}
	else if (spin_constraint() == SPIN_UNCONSTRAINED_E)
	{
	  const p3x1_t & la = view_mgr.la();
	  const p3x1_t & lf = view_mgr.lf();
	  
	  p3x1_t a = view_vol.to_wcs(p3x1_t(a_scs(), pick_depth_));
	  p3x1_t b = view_vol.to_wcs(p3x1_t(b_scs(), pick_depth_));
	  v3x1_t d = b - a;
	  
	  float lfla_dist =
	    ~(anchor.la() - anchor.lf());
	  
	  p3x1_t new_lf =
	    lf - 3.0f * (lfla_dist / anchor.view_radius()) * d;
	  
	  new_lf = la + lfla_dist * (!(new_lf - la));
	  
	  // calculate new up vector:
	  v3x1_t u = !(la - lf);
	  v3x1_t v = view_mgr.up();
	  v3x1_t w = !(u % v);
	  u = !(la - new_lf);
	  v = !(w % u);
	  
	  // apply the changes:
	  view_mgr.set_lf(new_lf);
	  view_mgr.set_up(v);
	  view.select_ep();
	}
      }
      
      if (spin_constraint() == SPIN_UNCONSTRAINED_E)
      {
	reset_anchor();
      }
    }
    else
    {
      the_trail_t::milestone_t milestone;
      
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_NOTHING_E;
      if (has_drag()) reset_anchor();
    }
  }
  else if (view_mgr_eh_state_ == THE_VIEW_MGR_EH_HAS_ZOOM_E)
  {
    if (zoom_allowed() &&
	is_modifier_active(this, THE_EH_MOD_VIEW_ZOOM_E))
    {
      the_trail_t::milestone_t milestone;
      
      if (which_axis_ == AXIS_UNDEFINED_E)
      {
	float dx = fabs(b_scs().x() - a_scs().x());
	float dy = fabs(b_scs().y() - a_scs().y());
	if (dx > dy) axis_.assign(1.0, 0.0, 0.0);
	else axis_.assign(0.0, 1.0, 0.0);
	which_axis_ = AXIS_DEFINED_E;
      }
      
      v3x1_t drag(helper_.anchor().view_radius() * (b_scs().x() - a_scs().x()),
		  helper_.anchor().view_radius() * (b_scs().y() - a_scs().y()),
		  0.0);
      
      // handle the linked views:
      std::list<view_mgr_helper_t> views = linked_views_;
      views.push_front(helper_);
      
      for (std::list<view_mgr_helper_t>::iterator i = views.begin();
	   i != views.end(); ++i)
      {
	view_mgr_helper_t & helper = *i;
	the_view_mgr_t & view_mgr = helper.view_mgr();
	the_view_mgr_t & anchor = helper.anchor();
	
	float magn = anchor.view_radius() + (axis_ * drag);
	if (magn > ~(view_mgr.lf() - view_mgr.la()) / 100.0)
	{
	  view_mgr.set_view_radius(magn);
	  
	  // make sure we did not move the look-from through the near plane:
	  if (view_mgr.near_plane() == 0.0)
	  {
	    cerr << "WARNING: impossible viewing, restoring previous state:"
		 << endl
		 << "vr: " << view_mgr.view_radius() << endl
		 << "sr: " << view_mgr.scene_radius() << endl
		 << endl;
	    helper.restore();
	  }
	  
	  helper.view().select_ep();
	}
      }
    }
    else
    {
      the_trail_t::milestone_t milestone;
      
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_NOTHING_E;
      if (has_drag()) reset_anchor();
    }
  }
  else if (view_mgr_eh_state_ == THE_VIEW_MGR_EH_HAS_PAN_E)
  {
    /*
    static int counter = 0;
    cerr << "has pan, got a mouse event: " << counter << endl;
    counter++;
    */
    
    if (pan_allowed() &&
	is_modifier_active(this, THE_EH_MOD_VIEW_PAN_E))
    {
      the_trail_t::milestone_t milestone;
      
      // handle the linked views:
      std::list<view_mgr_helper_t> views = linked_views_;
      views.push_front(helper_);
      
      for (std::list<view_mgr_helper_t>::iterator i = views.begin();
	   i != views.end(); ++i)
      {
	view_mgr_helper_t & helper = *i;
	the_view_mgr_t & view_mgr = helper.view_mgr();
	the_view_mgr_t & anchor = helper.anchor();
	
	the_view_volume_t vol;
	anchor.setup_view_volume(vol);
	
	the_view_t & view = helper.view();
	const the_plane_t & ep = view.active_ep();
	
	const the_ray_t a_ray(vol.to_ray(a_scs()));
	const the_ray_t b_ray(vol.to_ray(b_scs()));
	float ta = FLT_MAX;
	float tb = FLT_MAX;
	if (!ep.intersect(a_ray, ta) ||
	    !ep.intersect(b_ray, tb))
	{
	  continue;
	}
	
	p3x1_t a = a_ray * ta;
	p3x1_t b = b_ray * tb;
	v3x1_t v = b - a;
	
	view_mgr.set_lf(anchor.lf() - v);
	view_mgr.set_la(anchor.la() - v);
	
	the_bbox_t documents_bbox;
	helper.view().calc_bbox(documents_bbox);
	if ((documents_bbox.is_empty() == false) &&
	    (documents_bbox.is_singular() == false))
	{
	  float model_radius = documents_bbox.wcs_radius(view_mgr.la());
	  view_mgr.set_scene_radius(model_radius);
	}
	
	// make sure we did not move the look-from through the near plane:
	if (view_mgr.near_plane() == 0.0)
	{
	  helper.restore();
	}
	
	view.select_ep();
      }
    }
    else
    {
      the_trail_t::milestone_t milestone;
      
      view_mgr_eh_state_ = THE_VIEW_MGR_EH_HAS_NOTHING_E;
      if (has_drag()) reset_anchor();
    }
  }
  else
  {
    // unknown state:
    assert(false);
  }
  
  // update the cursor:
  switch (view_mgr_eh_state_)
  {
    case THE_VIEW_MGR_EH_HAS_SPIN_E:
      view().change_cursor(THE_SPIN_CURSOR_E);
      break;
      
    case THE_VIEW_MGR_EH_HAS_ZOOM_E:
      view().change_cursor(THE_ZOOM_CURSOR_E);
      break;
      
    case THE_VIEW_MGR_EH_HAS_PAN_E:
      view().change_cursor(THE_PAN_CURSOR_E);
      break;
      
    default:
      view().change_cursor(THE_DEFAULT_CURSOR_E);
      if (view_mgr_eh_state_ == prev_state)
      {
	return false;
      }
      break;
  }
  
  return true;
}
