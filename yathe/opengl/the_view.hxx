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


// File         : the_view.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A portable OpenGL view widget interface class.

#ifndef THE_VIEW_HXX_
#define THE_VIEW_HXX_

// local includes:
#include "ui/the_cursor.hxx"
#include "doc/the_document_so.hxx"
#include "doc/the_procedure.hxx"
#include "opengl/the_view_mgr.hxx"
#include "opengl/the_disp_list.hxx"
#include "opengl/the_gl_context.hxx"
#include "math/the_plane.hxx"
#include "math/the_bbox.hxx"
#include "math/the_coord_sys.hxx"
#include "utils/instance_method_call.hxx"

// system includes:
#include <iostream>

// forward declarations:
class the_input_device_eh_t;
class the_eh_stack_t;
class the_document_t;


//----------------------------------------------------------------
// the_edit_plane_t
// 
typedef enum
{
  THE_FRONT_EDIT_PLANE_E, // plane = YZ, normal = X
  THE_SIDE_EDIT_PLANE_E,  // plane = ZX, normal = Y
  THE_TOP_EDIT_PLANE_E    // plane = XY, normal = Z
} the_edit_plane_id_t;


//----------------------------------------------------------------
// the_view_t
// 
class the_view_t
{
  friend bool save(std::ostream & stream, const the_view_t * view);
  friend bool load(std::istream & stream, the_view_t * view);
  friend class the_main_window_t;
  
public:
  // Constructor for the main view:
  the_view_t(const char * name,
	     const the_view_mgr_orientation_t & o = THE_ISOMETRIC_VIEW_E);
  
  // view name accessors:
  const the_text_t & name() const
  { return name_; }
  
  void set_name(const the_text_t & name)
  { name_ = name; }
  
  // Destructor for the main view:
  virtual ~the_view_t();
  
  // view manager accessors:
  inline the_view_mgr_t & view_mgr()
  { return *view_mgr_; }
  
  inline const the_view_mgr_t & view_mgr() const
  { return *view_mgr_; }
  
  // view manager callback functions:
  static void view_mgr_cb(void *);
  
  // plane currently active in this canvas:
  inline const the_plane_t & active_ep() const
  { return edit_plane_[active_ep_id_]; }
  
  // accessor to the edit planes:
  inline static const the_plane_t & edit_plane(the_edit_plane_id_t ep_id)
  { return edit_plane_[ep_id]; }
  
  // antialiasing controls:
  void aa_disable();
  void aa_enable();
  
  inline bool local_aa() const
  { return local_aa_; }
  
  inline void toggle_aa()
  { if (local_aa_) aa_disable(); else aa_enable(); }
  
  inline void apply_aa(const bool & aa)
  { if (aa) aa_enable(); else aa_disable(); }
  
  // depth-queing controls:
  void dq_disable();
  void dq_enable();
  
  inline bool local_dq() const
  { return local_dq_; }
  
  inline void toggle_dq()
  { if (local_dq_) dq_disable(); else dq_enable(); }
  
  inline void apply_dq(const bool & dq)
  { if (dq) dq_enable(); else dq_disable(); }
  
  // perspective depth-queing controls:
  void pp_disable();
  void pp_enable();
  
  inline bool local_pp() const
  { return local_pp_; }
  
  inline void toggle_pp()
  { if (local_pp_) pp_disable(); else pp_enable(); }
  
  inline void apply_pp(const bool & pp)
  { if (pp) pp_enable(); else pp_disable(); }
  
  // restore view manager orientation to the default:
  void restore_orientation();
  
  // select an edit plane base on current viewpoint orientation:
  bool select_ep();
  
  // accessor to the event handler stack for this view:
  inline const the_eh_stack_t & eh_stack() const
  { return *eh_stack_; }
  
  // associate/deassociate an event handler with this view:
  void attach_eh(the_input_device_eh_t * eh);
  void detach_eh(the_input_device_eh_t * eh);
  
  // calculate the bounding box of the documents associated with the
  // event handlers currently installed in this view:
  void calc_bbox(the_bbox_t & bbox) const;
  
  // accessors to the event handler display lists:
  inline the_disp_list_t & dl_eh_3d() { return dl_eh_3d_; }
  inline the_disp_list_t & dl_eh_2d() { return dl_eh_2d_; }
  
  // call this to let the view know which document to display (can be NULL):
  inline void assign_document(const the_document_so_t * doc_so)
  { doc_so_ = doc_so; }
  
  // accessor to the document being displayed in this view:
  inline the_document_t * document() const
  {
    if (doc_so_ == NULL) return NULL;
    return doc_so_->document();
  }
  
  // accessors/manipulators for the list of primitives displayed in this view:
  inline const std::list<the_primitive_t *> & primitives() const
  { return primitives_; }
  
  inline void add_primitives(const std::list<the_primitive_t *> & p)
  { primitives_.insert(primitives_.end(), p.begin(), p.end()); }
  
  inline void add_primitive(the_primitive_t * p)
  { primitives_.push_back(p); }
  
  inline void clear_primitives()
  { primitives_.clear(); }
  
  // the derived class must override these:
  virtual bool is_hidden() = 0;
  virtual void set_focus() = 0;
  virtual void refresh() = 0;
  virtual bool gl_context_is_valid() const = 0;
  virtual void gl_make_current() = 0;
  virtual void gl_done_current() = 0;
  
  // accessor to the most context of the view that was current most recently:
  static the_gl_context_t gl_latest_context()
  { return the_gl_context_t(latest_view_); }
  
  inline the_gl_context_t gl_context() const
  { return the_gl_context_t(const_cast<the_view_t *>(this)); }
  
  // OpenGL helpers:
  void gl_setup();
  void gl_resize(int width, int height);
  void gl_paint();
  
  // UI helpers:
  virtual void change_cursor(const the_cursor_id_t & cursor_id) = 0;
  
  // shortcuts:
  inline int width() const
  { return int(view_mgr_->width()); }
  
  inline int height() const
  { return int(view_mgr_->height()); }
  
  // this pointer will be updated each time a view is repainted:
  static the_view_t * latest_view_;
  
protected:
  // disable default constructor:
  the_view_t();
  
  // view name:
  the_text_t name_;
  
  // the view point manager:
  the_view_mgr_t * view_mgr_;
  
  // display control flags:
  bool local_aa_; // antialiasing.
  bool local_dq_; // fog depth-queing.
  bool local_pp_; // perspective depth-queing.
  
  // These display lists are used by the event handlers for user feedback:
  the_disp_list_t dl_eh_3d_;
  
  // This display list does not get compiled, and is cleared after
  // every repaint, it is the event handlers responsibility to re-fill
  // it as necessary. The contents of this display list are assumed to be
  // 2D only on the [0, width] x [0, height] UV domain:
  the_disp_list_t dl_eh_2d_;
  
  // built in edit planes:
  static const the_plane_t edit_plane_[3];
  
  // ID of the active edit plane in this view:
  the_edit_plane_id_t active_ep_id_;
  
  // the event handler stack for this view:
  the_eh_stack_t * eh_stack_;
  
  // pointer to the document state object:
  const the_document_so_t * doc_so_;
  
  // a list of primitives displayed in this view; the list is updated by
  // the_procedure_t::draw(..) function:
  std::list<the_primitive_t *> primitives_;
  
  DECLARE_INSTANCE(the_view_t, instance_);
};


#endif // THE_VIEW_HXX_
