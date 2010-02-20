// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_input_device_eh.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Base class for mouse, keyboard and tablet device
//                event handlers.


#ifndef THE_INPUT_DEVICE_EH_HXX_
#define THE_INPUT_DEVICE_EH_HXX_

// system includes:
#include <list>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

// local includes:
#include "eh/the_modifier.hxx"
#include "eh/the_input_device_event.hxx"
#include "eh/the_mouse_device.hxx"
#include "eh/the_keybd_device.hxx"
#include "eh/the_wacom_device.hxx"
#include "ui/the_document_ui.hxx"
#include "opengl/the_view.hxx"
#include "opengl/the_view_mgr.hxx"
#include "math/the_coord_sys.hxx"
#include "math/the_plane.hxx"
#include "math/the_ray.hxx"
#include "opengl/the_disp_list.hxx"
#include "utils/the_unique_list.hxx"
#include "utils/instance_method_call.hxx"

// forward declarations:
class the_input_device_eh_t;


// Several enumerations used for event handler state keeping.
typedef enum the_eh_state
{
  THE_EH_NOT_READY_E,
  THE_EH_HAS_CLICK_E,
  THE_EH_END_CLICK_E,
  THE_EH_1ST_DRAG_E,
  THE_EH_HAS_DRAG_E,
  THE_EH_END_DRAG_E
} the_eh_state_t;


//----------------------------------------------------------------
// the_eh_stack_t
// 
class the_eh_stack_t : public std::list<the_input_device_eh_t *>
{
  friend class the_input_device_eh_t;
  
public:
  the_eh_stack_t();
  ~the_eh_stack_t();
  
  // add another event handler to the stack:
  void push(the_input_device_eh_t * handler);
  
  // remove the top event handler from the stack:
  the_input_device_eh_t * pop();
  
  // the event preprocessors:
  bool mouse_cb(const the_mouse_event_t & e);
  bool wheel_cb(const the_wheel_event_t & e);
  bool keybd_cb(const the_keybd_event_t & e);
  bool wacom_cb(const the_wacom_event_t & e);
  bool view_cb(the_view_t * view) const;
  
  // traverse the stack asking each event handler whether a given
  // modifier is currently active, until an affirmative response is
  // encountered, or the stop_here event handler is reached:
  bool is_modifier_active(const the_input_device_eh_t * stop_here,
			  the_eh_modifier_t mod) const;
  
  // functions used to save a copy of the most recent event:
  inline void save_mouse_event(const the_mouse_event_t & e)
  {
    most_recent_mouse_event_ = e;
    most_recent_event_ = &most_recent_mouse_event_;
  }
  
  inline void save_wheel_event(const the_wheel_event_t & e)
  {
    most_recent_wheel_event_ = e;
    most_recent_event_ = &most_recent_wheel_event_;
  }
  
  inline void save_keybd_event(const the_keybd_event_t & e)
  {
    most_recent_keybd_event_ = e;
    most_recent_event_ = &most_recent_keybd_event_;
  }
  
  inline void save_wacom_event(const the_wacom_event_t & e)
  {
    most_recent_wacom_event_ = e;
    most_recent_event_ = &most_recent_wacom_event_;
  }
  
  // const accessors to the most recent events:
  inline const the_mouse_event_t & most_recent_mouse_event() const
  { return most_recent_mouse_event_; }
  
  inline const the_wheel_event_t & most_recent_wheel_event() const
  { return most_recent_wheel_event_; }
  
  inline const the_keybd_event_t & most_recent_keybd_event() const
  { return most_recent_keybd_event_; }
  
  inline const the_wacom_event_t & most_recent_wacom_event() const
  { return most_recent_wacom_event_; }
  
  inline const the_input_device_event_t * most_recent_event() const
  { return most_recent_event_; }
  
protected:
  // copies of most recent events in each event category:
  the_mouse_event_t most_recent_mouse_event_;
  the_wheel_event_t most_recent_wheel_event_;
  the_keybd_event_t most_recent_keybd_event_;
  the_wacom_event_t most_recent_wacom_event_;
  
  // pointer to the most recent event:
  the_input_device_event_t * most_recent_event_;
  
  DECLARE_INSTANCE(the_eh_stack_t, instance_);
};


//----------------------------------------------------------------
// the_input_device_eh_t
// 
// Mouse/keyboard event handler base class:
class the_input_device_eh_t
{
  friend class the_eh_stack_t;
  
private:
  the_input_device_eh_t(const the_input_device_eh_t & eh);
  the_input_device_eh_t & operator = (const the_input_device_eh_t & eh);
  
public:
  the_input_device_eh_t();
  virtual ~the_input_device_eh_t();
  
  // A helper function used to identify event handlers:
  virtual const char * name() const = 0;
  
  // accessor to the event handler display list:
  inline const the_disp_list_t & dl() const
  { return dl_; }
  
  // this controls which views the event handler will get events from:
  void attach_view(the_view_t * view);
  void detach_view(the_view_t * view);
  
  // Install/Uninstall a callback for this handler:
  void install();
  void uninstall();
  
  // save a pointer to the view that originated the latest event:
  inline void save_event_originator(the_view_t * event_originator)
  { most_recent_view_ = event_originator; }
  
  // These functions will perform generic event processing and will call
  // the processor(..) to perform an appropriate action for the event.
  virtual bool mouse_handler();
  virtual bool wheel_handler();
  virtual bool keybd_handler();
  virtual bool wacom_handler();
  virtual bool view_handler(the_view_t * view);
  
  // These functions allow us to test if this handler is currently installed:
  inline bool is_toplevel() const  { return top_level_; }
  inline bool is_installed() const { return installed_; }
  
  // pointer to the view from which the most recent event originated:
  inline the_view_t * most_recent_view() const
  { return most_recent_view_; }
  
  // low-level recent event accessors:
  inline const the_mouse_event_t & most_recent_mouse_event() const
  { return most_recent_view()->eh_stack().most_recent_mouse_event(); }
  
  inline const the_wheel_event_t & most_recent_wheel_event() const
  { return most_recent_view()->eh_stack().most_recent_wheel_event(); }
  
  inline const the_keybd_event_t & most_recent_keybd_event() const
  { return most_recent_view()->eh_stack().most_recent_keybd_event(); }
  
  inline const the_wacom_event_t & most_recent_wacom_event() const
  { return most_recent_view()->eh_stack().most_recent_wacom_event(); }
  
  // most recent event accessors:
  inline const the_input_device_event_t * most_recent_event() const
  { return most_recent_view()->eh_stack().most_recent_event(); }
  
  // pointer to the view which this event handler is currently servicing:
  inline the_view_t & view() const
  { assert(most_recent_view() != NULL); return *most_recent_view(); }
  
  // return current state of the event handler:
  inline the_eh_state_t state() const { return state_; }
  
  // accessors to drag vector start/end points (expressed in screen coord sys):
  inline const p2x1_t & a_scs() const { return a_scs_; }
  inline const p2x1_t & b_scs() const { return b_scs_; }
  
  // Returns a_scs_ or b_scs_ depending on the state. This point is experssed
  // in the Screen Coordinate System:
  inline const p2x1_t & scs_pt() const
  { return most_recent_mouse_event().scs_pt(); }
  
  // helper function used to find the projection of a given point
  // expressed in the Screen Coordinate System onto the edit plane:
  inline bool
  scs_pt_to_wcs_pt(const p2x1_t & scs_pt, p3x1_t & wcs_pt) const
  {
    const the_ray_t ray(view().view_mgr().view_volume().to_ray(scs_pt));
    float param = FLT_MAX;
    if (edit_plane().intersect(ray, param) == false) return false;
    
    wcs_pt = ray * param;
    return true;
  }
  
  // use these to recalculate the drag vector start point location
  // and direction (in case edit plane normal/origin changes):
  virtual bool update_a_wcs();
  virtual bool update_drag_wcs();
  
  // helper for dealing with drag events:
  bool has_drag() const;
  
  // start/end points of the drag vector (expressed in the World Coord System):
  inline const p3x1_t & a_wcs() const { return a_wcs_; }
  inline const p3x1_t   b_wcs() const { return a_wcs_ + drag_wcs_; }
  
  // the drag vector (expressed in the World Coord System):
  inline const v3x1_t & drag_wcs() const { return drag_wcs_; }
  
  // an alternative for calculating the edit plane normal/origin:
  const v3x1_t curr_view_dir() const;
  
  // find the best edit plane normal given a vector in the plane:
  const v3x1_t best_ep_normal(const v3x1_t & vec_in_plane) const;
  
  // default edit plane offset, normal and origin:
  virtual       float ep_offset(the_view_t * view) const;
  virtual const v3x1_t ep_normal(the_view_t * view) const;
  virtual const p3x1_t ep_origin(the_view_t * view) const;
  
  // the edit plane:
  // origin = ep_origin + ep_offset * ep_normal
  // normal = ep_normal
  const the_plane_t edit_plane(the_view_t * view) const;
  
  // edit plane of the most recent view:
  inline const the_plane_t edit_plane() const
  { return edit_plane(most_recent_view()); }
  
  // check if a given modifier is currently active:
  virtual bool is_modifier_active_locally(the_eh_modifier_t mod) const;
  
  // check the event handler stack up to a point
  // whether a given modifier is currently active:
  inline bool is_modifier_active(const the_input_device_eh_t * stop_here,
				 the_eh_modifier_t mod) const
  { return most_recent_view()->eh_stack().is_modifier_active(stop_here, mod); }
  
protected:
  // these functions control whether events get delivered when the drag
  // vector can not be calculated due to poor edit plane positioning:
  virtual bool must_update_drag_wcs() const { return true; }
  
  //----------------------------------------------------------------
  // the_event_type_t
  // 
  typedef enum
  {
    THE_MOUSE_EVENT_E,
    THE_WHEEL_EVENT_E,
    THE_KEYBD_EVENT_E,
    THE_WACOM_EVENT_E
  } the_event_type_t;
  
  // This function must be overriden by the derived class.
  // It does the actual processing of data from the event,
  // be it click or drag or key press.
  virtual bool
  processor(the_input_device_eh_t::the_event_type_t event_type) = 0;
  
  // This function will be called automatically for the
  // top level event handler and the one immediately below it
  // after a push or a pop. It is useful for keeping state.
  // The default function ignores this information, and should
  // be rewritten by the classes that actually need this:
  virtual void set_toplevel(bool tl);
  
  // state: has_drag -> has_click, a_scs = b_scs:
  void reset_anchor();
  
  // These functions will be called from install/uninstall.
  // These are for convenience in order to let a handler know
  // that it will be (or has been) installed (or uninstalled):
  virtual void install_prolog() {}
  virtual void install_epilog() {}
  virtual void uninstall_prolog() {}
  virtual void uninstall_epilog() {}
  
  // draw drag vectors:
  void draw_scs_drag_vec();
  void draw_wcs_drag_vec();
  
  // a list of views on which this event handler can be installed:
  the::unique_list<the_view_t *> views_;
  
  // the originator of the most recent event:
  the_view_t * most_recent_view_;
  
  p2x1_t a_scs_; // anchor point, the first click point.
  p2x1_t b_scs_; // current point (after drag since the first click).
  
  // the user must move the mouse a minimum number of millimeters
  // before the event is reported as a drag event:
  float drag_threshold_mm_;
  bool threshold_exceeded_;
  
  // current state of the event handler:
  the_eh_state_t state_;
  
  // the start point of the drag vector:
  p3x1_t a_wcs_;
  
  // the drag vector expressed in the world coordinate system:
  v3x1_t drag_wcs_;
  
  bool top_level_; // is this the top-level event handler on the stack?
  bool installed_; // is this event handler installed?
  
  // event handler display list:
  the_disp_list_t dl_;
};


// For debugging purposes:
extern ostream &
operator << (ostream & stream, the_eh_state_t state);


#endif // THE_INPUT_DEVICE_EH_HXX_
