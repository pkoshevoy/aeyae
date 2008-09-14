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


// File         : the_input_device_eh.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : Base class for mouse, keyboard and tablet device
//                event handlers.

// local includes:
#include "eh/the_input_device_eh.hxx"
#include "ui/the_trail.hxx"
#include "math/the_ray.hxx"
#include "math/the_coord_sys.hxx"
#include "opengl/the_appearance.hxx"
#include "opengl/the_view.hxx"
#include "utils/the_indentation.hxx"
#include "utils/the_utils.hxx"
#include "utils/instance_method_call.hxx"


//----------------------------------------------------------------
// operator <<
// 
ostream &
operator << (ostream & stream, the_eh_state_t s)
{
  switch (s)
  {
    case THE_EH_NOT_READY_E: return stream << "THE_EH_NOT_READY_E";
    case THE_EH_HAS_CLICK_E: return stream << "THE_EH_HAS_CLICK_E";
    case THE_EH_END_CLICK_E: return stream << "THE_EH_END_CLICK_E";
    case THE_EH_1ST_DRAG_E:  return stream << "THE_EH_1ST_DRAG_E";
    case THE_EH_HAS_DRAG_E:  return stream << "THE_EH_HAS_DRAG_E";
    case THE_EH_END_DRAG_E:  return stream << "THE_EH_END_DRAG_E";
    default:                 break;
  }
  
  assert(false);
  return stream << "THE_EH_BAD_STATE:" << (int)s;
}


//----------------------------------------------------------------
// the_eh_stack_t::the_eh_stack_t
// 
the_eh_stack_t::the_eh_stack_t():
  std::list<the_input_device_eh_t *>(),
  most_recent_event_(NULL)
{
  // cerr << "eh_stack constructor called: " << this << endl;
  SCOPED_INSTANCE_INIT(the_eh_stack_t);
  RECORD_INSTANCE(instance_);
  
  static bool methods_registered = false;
  if (!methods_registered)
  {
    METHOD_REGISTER_ARG1(the_eh_stack_t, mouse_cb, the_mouse_event_t);
    METHOD_REGISTER_ARG1(the_eh_stack_t, wheel_cb, the_wheel_event_t);
    METHOD_REGISTER_ARG1(the_eh_stack_t, keybd_cb, the_keybd_event_t);
    METHOD_REGISTER_ARG1(the_eh_stack_t, wacom_cb, the_wacom_event_t);
    methods_registered = true;
  }
}

//----------------------------------------------------------------
// the_eh_stack_t::~the_eh_stack_t
// 
the_eh_stack_t::~the_eh_stack_t()
{
  // cerr << "eh_stack destructor called: " << this << endl;
}

//----------------------------------------------------------------
// the_eh_stack_t::push
// 
void
the_eh_stack_t::push(the_input_device_eh_t * eh)
{
  if (!empty())
  {
    front()->set_toplevel(false);
  }
  
  eh->set_toplevel(true);
  push_front(eh);
}

//----------------------------------------------------------------
// the_eh_stack_t::pop
// 
the_input_device_eh_t *
the_eh_stack_t::pop()
{
  if (empty()) return NULL;
  
  the_input_device_eh_t * old_eh = remove_head(*this);
  
  old_eh->set_toplevel(false);
  if (!empty()) front()->set_toplevel(true);
  return old_eh;
}

//----------------------------------------------------------------
// the_eh_stack_t::mouse_cb
// 
bool
the_eh_stack_t::mouse_cb(const the_mouse_event_t & e)
{
  // save the call to the trail file:
  RECORD_CALL_ARG1(instance_, the_eh_stack_t, mouse_cb, the_mouse_event_t, e);
  
  e.widget()->set_focus();
  
  save_mouse_event(e);
  
  // Traverse the stack from the top until the event is processed:
  for (std::list<the_input_device_eh_t *>::iterator
	 i = begin(); i != end(); ++i)
  {
    (*i)->save_event_originator(dynamic_cast<the_view_t *>(e.widget()));
    if ((*i)->mouse_handler()) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_eh_stack_t::wheel_cb
// 
bool
the_eh_stack_t::wheel_cb(const the_wheel_event_t & e)
{
  // save the call to the trail file:
  RECORD_CALL_ARG1(instance_, the_eh_stack_t, wheel_cb, the_wheel_event_t, e);
  
  e.widget()->set_focus();
  
  save_wheel_event(e);
  
  // Traverse the stack from the top until the event is processed:
  for (std::list<the_input_device_eh_t *>::iterator
	 i = begin(); i != end(); ++i)
  {
    (*i)->save_event_originator(dynamic_cast<the_view_t *>(e.widget()));
    if ((*i)->wheel_handler()) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_eh_stack_t::keybd_cb
// 
bool
the_eh_stack_t::keybd_cb(const the_keybd_event_t & e)
{
  // save the call to the trail file:
  RECORD_CALL_ARG1(instance_, the_eh_stack_t, keybd_cb, the_keybd_event_t, e);
  
  // save the view pointer:
  save_keybd_event(e);
  
  // Traverse the stack from the top until the event is processed:  
  for (std::list<the_input_device_eh_t *>::iterator
	 i = begin(); i != end(); ++i)
  {
    (*i)->save_event_originator(dynamic_cast<the_view_t *>(e.widget()));
    if ((*i)->keybd_handler()) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_eh_stack_t::wacom_cb
// 
bool
the_eh_stack_t::wacom_cb(const the_wacom_event_t & e)
{
  // save the call to the trail file:
  RECORD_CALL_ARG1(instance_, the_eh_stack_t, wacom_cb, the_wacom_event_t, e);
  
  e.widget()->set_focus();
  
  save_wacom_event(e);
  
  // Traverse the stack from the top until the event is processed:
  for (std::list<the_input_device_eh_t *>::iterator
	 i = begin(); i != end(); ++i)
  {
    (*i)->save_event_originator(dynamic_cast<the_view_t *>(e.widget()));
    if ((*i)->wacom_handler()) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_eh_stack_t::view_cb
// 
bool
the_eh_stack_t::view_cb(the_view_t * view) const
{
  // setup the milestone markers:
  the_trail_t::milestone_t milestone;
  
  // Traverse the stack from the top:
  for (std::list<the_input_device_eh_t *>::const_iterator
	 i = begin(); i != end(); ++i)
  {
    if ((*i)->view_handler(view)) return true;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_eh_stack_t::is_modifier_active
// 
bool
the_eh_stack_t::
is_modifier_active(const the_input_device_eh_t * stop_here,
		   the_eh_modifier_t mod) const
{
  // Traverse the stack from the top until the event is processed:
  for (std::list<the_input_device_eh_t *>::const_iterator
	 i = begin(); i != end(); ++i)
  {
    const the_input_device_eh_t * eh = *i;
    if (eh->is_modifier_active_locally(mod)) return true;
    if (eh == stop_here) break;
  }
  
  return false;
}


//----------------------------------------------------------------
// the_input_device_eh_t::the_input_device_eh_t
// 
the_input_device_eh_t::the_input_device_eh_t():
  most_recent_view_(NULL),
  a_scs_(0.0, 0.0),
  b_scs_(0.0, 0.0),
  drag_threshold_mm_(1.0),
  threshold_exceeded_(false),
  state_(THE_EH_NOT_READY_E),
  a_wcs_(0.0, 0.0, 0.0),
  drag_wcs_(0.0, 0.0, 0.0),
  top_level_(false),
  installed_(false)
{}

//----------------------------------------------------------------
// the_input_device_eh_t::~the_input_device_eh_t
// 
the_input_device_eh_t::~the_input_device_eh_t()
{
  // WARNING: do not call uninstall() here because the derived
  // class has already been destroyed:
  assert(!is_installed());
}

//----------------------------------------------------------------
// the_input_device_eh_t::attach_view
// 
void
the_input_device_eh_t::attach_view(the_view_t * view)
{
  views_.push_back(view);
}

//----------------------------------------------------------------
// the_input_device_eh_t::detach_view
// 
void
the_input_device_eh_t::detach_view(the_view_t * view)
{
  views_.remove(view);
}

//----------------------------------------------------------------
// the_input_device_eh_t::install
// 
void
the_input_device_eh_t::install()
{
  if (is_installed()) return;
  
  // let yourself know that you are about to be installed:
  install_prolog();
  
  // push yourself onto the event handler stack of very view:
  for (std::list<the_view_t *>::iterator
	 i = views_.begin(); i != views_.end(); ++i)
  {
    (*i)->attach_eh(this);
  }
  
  // set the flag:
  installed_ = true;
  
  // let yourself know that you have just been installed:
  install_epilog();
}

//----------------------------------------------------------------
// the_input_device_eh_t::uninstall
// 
void
the_input_device_eh_t::uninstall()
{
  if (!is_installed()) return;
  
  // let yourself know that you are about to be un-installed:
  uninstall_prolog();
  
  // clear the display list:
  dl_.clear();
  
  // remove yourself from the event handler stack of very view:
  for (std::list<the_view_t *>::iterator
	 i = views_.begin(); i != views_.end(); ++i)
  {
    (*i)->detach_eh(this);
  }
  
  // clear the flag:
  installed_ = false;
  
  // let yourself know that you have just been un-installed:
  uninstall_epilog();
}

//----------------------------------------------------------------
// the_input_device_eh_t::mouse_handler
// 
bool
the_input_device_eh_t::mouse_handler()
{
  // NOTE: on down events, a_wcs_ will be calculated only after calling
  // processor(..). This ensures that the edit plane normal/origin will
  // be correct (processor may change internal state of the event handler
  // which may affect the edit plane information). The moral of the story:
  // do not rely on a_wcs_ in processor(..) on the down events.
  
  // perform generic event processing:
  switch (state_)
  {
    case THE_EH_NOT_READY_E:
    case THE_EH_END_CLICK_E:
    case THE_EH_END_DRAG_E:
      if ((THE_MOUSE.l_btn().changed() && THE_MOUSE.l_btn().down()) ||
	  (THE_MOUSE.m_btn().changed() && THE_MOUSE.m_btn().down()) ||
	  (THE_MOUSE.r_btn().changed() && THE_MOUSE.r_btn().down()))
      {
	state_ = THE_EH_HAS_CLICK_E;
      }
      else
      {
	state_ = THE_EH_NOT_READY_E;
	threshold_exceeded_ = false;
      }
      break;
      
    case THE_EH_HAS_CLICK_E:
      if (THE_MOUSE.l_btn().down() ||
	  THE_MOUSE.m_btn().down() ||
	  THE_MOUSE.r_btn().down())
      {
	if (!threshold_exceeded_)
	{
	  const p2x1_t a(a_scs_);
	  const p2x1_t b(scs_pt());
	  v2x1_t d = b - a;
	  
	  const the_view_mgr_t & vm = view().view_mgr();
	  d.x() *= vm.width();
	  d.y() *= vm.height();
	  
	  const float drag_pixels = d.norm();
	  const float drag_mm =
	    drag_pixels / the_view_mgr_t::pixels_per_millimeter();
	  
	  if (drag_mm < drag_threshold_mm_)
	  {
	    // FIXME: printf("drag: %f mm\n", drag_mm);
	    return true;
	  }
	  
	  // FIXME: printf("drag: %f mm - threshold exceeded\n\n", drag_mm);
	  threshold_exceeded_ = true;
	}
	
	state_ = THE_EH_1ST_DRAG_E;
      }
      else
      {
	state_ = THE_EH_END_CLICK_E;
      }
      break;
      
    case THE_EH_1ST_DRAG_E:
      state_ = THE_EH_HAS_DRAG_E;
    case THE_EH_HAS_DRAG_E:
      if (THE_MOUSE.l_btn().up() &&
	  THE_MOUSE.m_btn().up() &&
	  THE_MOUSE.r_btn().up())
      {
	state_ = THE_EH_END_DRAG_E;
      }
      break;
      
    default:
      assert(false);
  }
  
  b_scs_ = scs_pt();
  if ((state_ == THE_EH_NOT_READY_E) || (state_ == THE_EH_HAS_CLICK_E))
  {
    a_scs_ = scs_pt();
  }
  
  // ignore empty mouse drags:
  if ((state_ == THE_EH_NOT_READY_E) &&
      (most_recent_mouse_event().moving_))
  {
    return true;
  }
  
  // update the drag vector:
  if (state_ != THE_EH_NOT_READY_E)
  {
    if (must_update_drag_wcs())
    {
      if (update_drag_wcs() == false)
      {
	// no point in processing an event if the drag vector cannot be
	// calculated with current viewing:
	return false;
      }
    }
  }
  else
  {
    drag_wcs_.assign(0.0, 0.0, 0.0);
  }
  
  // call the processor:
  bool ret = processor(THE_MOUSE_EVENT_E);
  
  // calculate the start of the drag vector:
  if (state_ == THE_EH_HAS_CLICK_E)
  {
    if (must_update_drag_wcs()) update_drag_wcs();
  }
  
  return ret;
}

//----------------------------------------------------------------
// the_input_device_eh_t::wheel_handler
// 
bool
the_input_device_eh_t::wheel_handler()
{
  // call the processor:
  return processor(THE_WHEEL_EVENT_E);
}

//----------------------------------------------------------------
// the_input_device_eh_t::keybd_handler
// 
bool
the_input_device_eh_t::keybd_handler()
{
  // call the processor:
  return processor(THE_KEYBD_EVENT_E);
}

//----------------------------------------------------------------
// the_input_device_eh_t::wacom_handler
// 
bool
the_input_device_eh_t::wacom_handler()
{
  // call the processor:
  return processor(THE_WACOM_EVENT_E);
}

//----------------------------------------------------------------
// the_input_device_eh_t::view_handler
// 
bool
the_input_device_eh_t::view_handler(the_view_t * /* view */)
{
  return false;
}

//----------------------------------------------------------------
// the_input_device_eh_t::update_a_wcs
// 
bool
the_input_device_eh_t::update_a_wcs()
{
  if (scs_pt_to_wcs_pt(a_scs(), a_wcs_) == false)
  {
    cerr << "WARNING: " << name() << ": update_a_wcs FAILED" << endl;
    return false;
  }
  
  return true;
}

//----------------------------------------------------------------
// the_input_device_eh_t::update_drag_wcs
// 
bool
the_input_device_eh_t::update_drag_wcs()
{
  if (update_a_wcs() == false) return false;
  
  if (!has_drag())
  {
    drag_wcs_.assign(0.0, 0.0, 0.0);
    return true;
  }
  
  p3x1_t b_wcs;
  if (scs_pt_to_wcs_pt(b_scs(),  b_wcs) == false)
  {
    cerr << "WARNING: " << name() << ": update_drag_wcs FAILED" << endl;
    return false;
  }
  
  drag_wcs_ = b_wcs - a_wcs_;
  return true;
}

//----------------------------------------------------------------
// the_input_device_eh_t::has_drag
// 
bool
the_input_device_eh_t::has_drag() const
{
  return
    state_ == THE_EH_1ST_DRAG_E ||
    state_ == THE_EH_HAS_DRAG_E;
}

//----------------------------------------------------------------
// the_input_device_eh_t::curr_view_dir
// 
const v3x1_t 
the_input_device_eh_t::curr_view_dir() const
{
  const the_view_mgr_t & view_mgr = view().view_mgr();
  return !(view_mgr.la() - view_mgr.lf());
}

//----------------------------------------------------------------
// the_input_device_eh_t::best_ep_normal
// 
const v3x1_t
the_input_device_eh_t::best_ep_normal(const v3x1_t & vec_in_plane) const
{
  the_coord_sys_t some_cs;
  
  v3x1_t view_vec = curr_view_dir();
  v3x1_t path_vec = !vec_in_plane;
  
  // find the best edit plane normal vector:
  unsigned int best_vec_id = UINT_MAX;
  float best_dot_pr = 0.0;
  unsigned int adequate_vec_id = UINT_MAX;
  float adequate_dot_pr = 1.0;
  
  v3x1_t norm(0.0, 0.0, 0.0);
  for (unsigned int i = 0; i < 3; i++)
  {
    float dot_pr_0 = fabs(path_vec * some_cs[i]);
    if (dot_pr_0 < adequate_dot_pr)
    {
      adequate_vec_id = i;
      adequate_dot_pr = dot_pr_0;
    }
    
    if (dot_pr_0 < 0.708)
    {
      norm = path_vec % some_cs[i];
      float dot_pr_1 = fabs(view_vec * norm);
      if (dot_pr_1 > best_dot_pr)
      {
	best_dot_pr = dot_pr_1;
	best_vec_id = i;
      }
    }
  }
  
  if (best_vec_id == UINT_MAX)
  {
    // Uh - oh. The drag path is not suitable for current view point
    // orientation, let the event handler take care of the view point
    // problem (no-op is a good solution):
    
    best_vec_id = adequate_vec_id;
  }
  
  return !(path_vec % some_cs[best_vec_id]);
}

//----------------------------------------------------------------
// the_input_device_eh_t::ep_offset
// 
float
the_input_device_eh_t::ep_offset(the_view_t * /* view */) const
{
  return 0.0;
}

//----------------------------------------------------------------
// the_input_device_eh_t::ep_normal
// 
const v3x1_t
the_input_device_eh_t::ep_normal(the_view_t * view) const
{
  return view->active_ep().cs().z_axis();
}

//----------------------------------------------------------------
// the_input_device_eh_t::ep_origin
// 
const p3x1_t
the_input_device_eh_t::ep_origin(the_view_t * view) const
{
  return view->active_ep().cs().origin();
}

//----------------------------------------------------------------
// the_input_device_eh_t::edit_plane
// 
const the_plane_t
the_input_device_eh_t::edit_plane(the_view_t * view) const
{
  v3x1_t normal = ep_normal(view);
  return the_plane_t(the_coord_sys_t(ep_origin(view) +
				     ep_offset(view) * normal,
				     normal));
}

//----------------------------------------------------------------
// the_input_device_eh_t::is_modifier_active_locally
// 
bool
the_input_device_eh_t::
is_modifier_active_locally(the_eh_modifier_t modifier) const
{
  switch (modifier)
  {
    case THE_EH_MOD_NONE_E:
      return THE_KEYBD.verify_no_pressed_keys();
      
    default:
      break;
  }
  
  return false;
}

//----------------------------------------------------------------
// the_input_device_eh_t::set_toplevel
// 
void
the_input_device_eh_t::set_toplevel(bool toplevel)
{
  top_level_ = toplevel;
}

//----------------------------------------------------------------
// the_input_device_eh_t::reset_anchor
// 
void
the_input_device_eh_t::reset_anchor()
{
  if (!has_drag()) return;
  
  a_scs_ = b_scs_;
  state_ = THE_EH_HAS_CLICK_E;
}

//----------------------------------------------------------------
// the_input_device_eh_t::draw_scs_drag_vec
// 
void
the_input_device_eh_t::draw_scs_drag_vec()
{
  the_disp_list_t & dl_2d = view().dl_eh_2d();
  dl_2d.
    push_back(new the_scs_arrow_dl_elem_t(view().view_mgr().width(),
					  view().view_mgr().height(),
					  a_scs(),
					  scs_pt(),
					  THE_APPEARANCE.palette().scs_drag(),
					  11.0));
}

//----------------------------------------------------------------
// the_input_device_eh_t::draw_wcs_drag_vec
// 
void
the_input_device_eh_t::draw_wcs_drag_vec()
{
  the_disp_list_t & dl_3d = view().dl_eh_3d();
  p3x1_t b_wcs = update_drag_wcs() ? a_wcs_ + drag_wcs_ : a_wcs_;
  
  const the_color_t & color_a = THE_APPEARANCE.palette().wcs_drag()[0];
  const the_color_t & color_b = THE_APPEARANCE.palette().wcs_drag()[1];
  
  dl_3d.push_back(new the_symbol_dl_elem_t(a_wcs_,
					   color_a,
					   THE_POINT_SYMBOLS,
					   THE_DIAG_CROSS_SYMBOL_E));
  the_view_volume_t view_volume;
  view().view_mgr().setup_view_volume(view_volume);
  dl_3d.push_back(new the_arrow_dl_elem_t(view_volume,
					  view().view_mgr().width(),
					  view().view_mgr().height(),
					  a_wcs_,
					  b_wcs,
					  view().active_ep().cs().z_axis(),
					  color_a,
					  color_b,
					  4));
}
