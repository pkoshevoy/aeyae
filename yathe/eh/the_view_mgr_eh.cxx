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
  allow_pan_(true)
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
	vr *= (180.0 - we.degrees_rotated()) / 180.0;
      }
      else
      {
	vr *= 180.0 / (180.0 + we.degrees_rotated());
      }
      
      float vr_threshold = ~(view_mgr.lf() - view_mgr.la()) / 100.0;
      if (vr > vr_threshold)
      {
	view_mgr.set_view_radius(vr);
	
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
      prev_pt_ =
	helper_.anchor().view_volume().to_wcs(p3x1_t(a_scs(), pick_depth_));
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
      
      // FIXME: rewrite this to support linked views:
      
      const p3x1_t & la = view_mgr.la();
      const p3x1_t & lf = view_mgr.lf();
      
      p3x1_t b = view_mgr.view_volume().to_wcs(p3x1_t(b_scs(), pick_depth_));
      v3x1_t d = b - prev_pt_;
      
      float lfla_dist =
	~(helper_.anchor().la() - helper_.anchor().lf());
      
      p3x1_t new_lf =
	lf - 3.0 * (lfla_dist / helper_.anchor().view_radius()) * d;
      
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
      view().select_ep();
      
      prev_pt_ = view_mgr.view_volume().to_wcs(p3x1_t(b_scs(), pick_depth_));
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
      
      // calculate the displacement vector:
      ray_b_ = helper_.anchor().view_volume().to_ray(b_scs());
      p3x1_t finish_pnt = ray_b_ * pick_depth_;
      v3x1_t v = (start_pnt_ - finish_pnt);
      
      // handle the linked views:
      std::list<view_mgr_helper_t> views = linked_views_;
      views.push_front(helper_);
      
      for (std::list<view_mgr_helper_t>::iterator i = views.begin();
	   i != views.end(); ++i)
      {
	view_mgr_helper_t & helper = *i;
	the_view_mgr_t & view_mgr = helper.view_mgr();
	the_view_mgr_t & anchor = helper.anchor();
	
	view_mgr.set_lf(anchor.lf() + v);
	view_mgr.set_la(anchor.la() + v);
	
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
	
	helper.view().select_ep();
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
