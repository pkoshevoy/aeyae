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


// File         : the_view_mgr_eh.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : The OpenGL view manager mouse/keyboard/tablet event handler.

#ifndef THE_VIEW_MGR_EH_HXX_
#define THE_VIEW_MGR_EH_HXX_

// local includes:
#include "eh/the_input_device_eh.hxx"
#include "math/the_ray.hxx"
#include "sel/the_pick_list.hxx"


//----------------------------------------------------------------
// the_view_mgr_eh_state_t
// 
typedef enum
{
  THE_VIEW_MGR_EH_HAS_NOTHING_E,
  THE_VIEW_MGR_EH_HAS_SPIN_E,
  THE_VIEW_MGR_EH_HAS_ZOOM_E,
  THE_VIEW_MGR_EH_HAS_PAN_E
} the_view_mgr_eh_state_t;


//----------------------------------------------------------------
// view_mgr_helper_t
// 
class view_mgr_helper_t
{
public:
  view_mgr_helper_t(the_view_t * view = NULL,
		    the_view_mgr_t * anchor = NULL):
    view_(view),
    anchor_((anchor == NULL) ? NULL : anchor->clone())
  {}
  
  view_mgr_helper_t(const view_mgr_helper_t & helper):
    view_(NULL),
    anchor_(NULL)
  { *this = helper; }
  
  ~view_mgr_helper_t()
  {
    delete anchor_;
    
#ifndef NDEBUG
    view_ = NULL;
    anchor_ = NULL;
#endif
  }
  
  // assignment operator:
  view_mgr_helper_t & operator = (const view_mgr_helper_t & helper)
  {
    view_ = helper.view_;
    anchor_ = (helper.anchor_ == NULL) ? NULL : helper.anchor_->clone();
    return *this;
  }
  
  // equality test operator:
  inline bool operator == (const view_mgr_helper_t & helper) const
  { return view_ == helper.view_; }
  
  // view accessors:
  inline const the_view_t & view() const
  { return *view_; }
  
  inline the_view_t & view()
  { return *view_; }
  
  inline void set_view(the_view_t * view)
  { view_ = view; }
  
  // view manager accessors:
  inline const the_view_mgr_t & view_mgr() const
  { return view_->view_mgr(); }
  
  inline the_view_mgr_t & view_mgr()
  { return view_->view_mgr(); }
  
  // anchor accessors:
  inline const the_view_mgr_t & anchor() const
  { return *anchor_; }
  
  inline the_view_mgr_t & anchor()
  { return *anchor_; }
  
  inline void set_anchor()
  {
    delete anchor_;
    anchor_ = view_->view_mgr().clone();
  }
  
  // restore the view manager from the anchor:
  inline void restore()
  { view_->view_mgr() = *anchor_; }
  
protected:
  the_view_t * view_;
  the_view_mgr_t * anchor_;
};


//----------------------------------------------------------------
// the_view_mgr_eh_t
// 
// This class handles top level view point manipulation:
class the_view_mgr_eh_t : public the_input_device_eh_t
{
public:
  typedef enum { AXIS_UNDEFINED_E, AXIS_DEFINED_E } axis_t;
  
  the_view_mgr_eh_t();
  virtual ~the_view_mgr_eh_t();
  
  // virtual:
  const char * name() const { return "the_view_mgr_eh_t"; }
  
  // virtual:
  bool view_handler(the_view_t * view);
  
  // virtual: check if a given modifier is currently active:
  bool is_modifier_active_locally(the_eh_modifier_t mod) const;
  
  // virtual:
  float ep_offset(the_view_t * view) const;
  
  // these flags control how much interactivity is presented to the user:
  inline void allow_spin(const bool & enable)
  { allow_spin_ = enable; }
  
  inline const bool & spin_allowed() const
  { return allow_spin_; }
  
  inline void allow_zoom(const bool & enable)
  { allow_zoom_ = enable; }
  
  inline const bool & zoom_allowed() const
  { return allow_zoom_; }
  
  inline void allow_pan(const bool & enable)
  { allow_pan_ = enable; }
  
  inline const bool & pan_allowed() const
  { return allow_pan_; }
  
  // linked view accessors:
  inline void add_linked_view(the_view_t * v)
  { linked_views_.push_back(view_mgr_helper_t(v)); }
  
  inline void del_linked_view(the_view_t * v)
  { linked_views_.remove(view_mgr_helper_t(v)); }
  
  inline const the_view_mgr_eh_state_t & view_mgr_eh_state() const
  { return view_mgr_eh_state_; }
  
protected:
  // virtual:
  bool must_update_drag_wcs() const { return false; }
  
  // virtual: Override the processor function.
  bool processor(the_input_device_eh_t::the_event_type_t event_type);
  
private:
  // internal state of the view point event handler:
  the_view_mgr_eh_state_t view_mgr_eh_state_;
  
  // a wrapper used to simplify internal interaction with the view manager:
  view_mgr_helper_t helper_;
  
  // SPIN: previous point on the near plane:
  p3x1_t prev_pt_;
  
  // ZOOM: which axis is to be used to decide the zoom in/out:
  axis_t which_axis_;
  
  // ZOOM: coordinate system axis used to decide the zoom in/out:
  v3x1_t axis_;
  
  // PAN: start point of the vector the LF/LA translation vector.
  p3x1_t start_pnt_;
  the_ray_t ray_a_;
  the_ray_t ray_b_;
  
  // SPIN & PAN: depth of the object that was picked:
  float pick_depth_;
  
  // a list of geometry that happened to be under the mouse during the pick:
  the_pick_list_t picks_;
  
  // these flags control how much interactivity is presented to the user:
  bool allow_spin_;
  bool allow_zoom_;
  bool allow_pan_;
  
  // a list of views linked to the view controlled by this view manager
  // event handler:
  std::list<view_mgr_helper_t> linked_views_;
};


#endif // THE_VIEW_MGR_EH_HXX_
