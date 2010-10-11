// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_view.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A portable OpenGL view widget interface class.

// system includes:
#include <assert.h>

// local includes:
#include "opengl/the_view.hxx"
#include "opengl/the_appearance.hxx"
#include "opengl/the_disp_list.hxx"
#include "opengl/the_ep_grid.hxx"
#include "opengl/the_gl_context.hxx"
#include "doc/the_document.hxx"
#include "doc/the_procedure.hxx"
#include "eh/the_input_device_eh.hxx"
#include "ui/the_trail.hxx"
#include "utils/instance_method_call.hxx"


//----------------------------------------------------------------
// the_view_t::latest_view_
// 
the_view_t *
the_view_t::latest_view_ = NULL;


//----------------------------------------------------------------
// the_view_t::edit_plane_
// 
const the_plane_t
the_view_t::edit_plane_[] =
{
  // THE_FRONT_EDIT_PLANE_E:
  the_plane_t(the_coord_sys_t(v3x1_t(0, 1, 0),
			      v3x1_t(0, 0, 1),
			      v3x1_t(1, 0, 0),
			      p3x1_t(0, 0, 0))),
  
  // THE_SIDE_EDIT_PLANE_E:
  the_plane_t(the_coord_sys_t(v3x1_t(0, 0, 1),
			      v3x1_t(1, 0, 0),
			      v3x1_t(0, 1, 0),
			      p3x1_t(0, 0, 0))),
  
  // THE_TOP_EDIT_PLANE_E:
  the_plane_t(the_coord_sys_t(v3x1_t(1, 0, 0),
			      v3x1_t(0, 1, 0),
			      v3x1_t(0, 0, 1),
			      p3x1_t(0, 0, 0)))
};

//----------------------------------------------------------------
// the_view_t::the_view_t
// 
the_view_t::the_view_t(const char * name,
		       const the_view_mgr_orientation_t & orientation):
  name_(name),
  view_mgr_(NULL),
  stereoscopic_(false),
  local_aa_(true),
  local_dq_(false),
  local_pp_(false),
  active_ep_id_(THE_TOP_EDIT_PLANE_E),
  eh_stack_(NULL),
  doc_so_(NULL)
{
  DEFINE_INSTANCE(instance_, the_view_t, this);
  RECORD_INSTANCE(instance_);
  
  // create a view manager:
  if (local_pp_)
  {
    view_mgr_ = new the_persp_view_mgr_t(orientation);
  }
  else
  {
    view_mgr_ = new the_ortho_view_mgr_t(orientation);
  }
  
  // setup a view manager callback:
  view_mgr_->set_callback(view_mgr_cb, this);
  
  // create an event handler stack:
  eh_stack_ = new the_eh_stack_t();
  
  // select an active plane:
  select_ep();
}

//----------------------------------------------------------------
// the_view_t::~the_view_t
// 
the_view_t::~the_view_t()
{
  delete view_mgr_;
  delete eh_stack_;
}

//----------------------------------------------------------------
// the_view_t::view_mgr_cb
// 
void
the_view_t::view_mgr_cb(void * data)
{
  the_view_t * view = (the_view_t *)(data);
  view->eh_stack().view_cb(view);
}

//----------------------------------------------------------------
// the_view_t::set_stereoscopic
// 
void
the_view_t::set_stereoscopic(bool stereoscopic)
{
  if (stereoscopic_ != stereoscopic)
  {
    stereoscopic_ = stereoscopic;
    refresh();
  }
}

//----------------------------------------------------------------
// the_view_t::is_stereoscopic
// 
bool
the_view_t::is_stereoscopic() const
{
  return stereoscopic_;
}

//----------------------------------------------------------------
// the_view_t::aa_disable
// 
void
the_view_t::aa_disable()
{
  if (!local_aa_) return;
  
  local_aa_ = false;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::aa_enable
// 
void
the_view_t::aa_enable()
{
  if (local_aa_) return;
  
  local_aa_ = true;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::dq_disable
// 
void
the_view_t::dq_disable()
{
  if (!local_dq_) return;
  
  local_dq_ = false;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::dq_enable
// 
void
the_view_t::dq_enable()
{
  if (local_dq_) return;
  
  local_dq_ = true;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::pp_disable
// 
void
the_view_t::pp_disable()
{
  the_ortho_view_mgr_t * current =
    dynamic_cast<the_ortho_view_mgr_t *>(view_mgr_);
  
  if ((local_pp_ == false) && (current != NULL)) return;
  
  the_view_mgr_t * ortho_view = view_mgr_->other();
  delete view_mgr_;
  view_mgr_ = ortho_view;
  
  local_pp_ = false;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::pp_enable
// 
void
the_view_t::pp_enable()
{
  the_persp_view_mgr_t * current =
    dynamic_cast<the_persp_view_mgr_t *>(view_mgr_);
  
  if (local_pp_ && (current != NULL)) return;
  
  the_view_mgr_t * persp_view = view_mgr_->other();
  delete view_mgr_;
  view_mgr_ = persp_view;
  
  local_pp_ = true;
  refresh();
}

//----------------------------------------------------------------
// the_view_t::restore_orientation
// 
void
the_view_t::restore_orientation()
{
  the_view_mgr_t::callback_buffer_t buffer(&view_mgr());
  
  // restore the view manager:
  the_bbox_t documents_bbox;
  calc_bbox(documents_bbox);
  view_mgr().reset(documents_bbox);
  select_ep();
}

//----------------------------------------------------------------
// the_view_t::select_ep
// 
// The edit plane grid is more intuitive when it appears
// that the grid at the top of the screen is farther away from
// the viewer that at the bottom of the screen (the edit plane
// is leaning back). It is also intuitive when the edit plane
// is exactly perpendicular to the view vector.
// 
// The method below will try to optimize the active edit plane
// selection to satisfy these requirements.
// 
bool
the_view_t::select_ep()
{
  // midpoint between upper-left and upper-right corners:
  v3x1_t la = !(view_mgr().la() - view_mgr().lf());
  v3x1_t up = !(view_mgr().up());
  p3x1_t lf = view_mgr().lf();
  
  the_edit_plane_id_t prev_ep_id = active_ep_id_;
  the_edit_plane_id_t best_ep_id = prev_ep_id;
  
  float best_result = -FLT_MAX;
  float result[] = { 0.0f, 0.0f, 0.0f };
  
  for (unsigned int i = 0; i <= THE_TOP_EDIT_PLANE_E; i++)
  {
    const the_plane_t & ep = edit_plane_[i];
    
    // find the vector from the edit plane toward the top-center point:
    v3x1_t ep_lf = !(ep - lf);
    float a = -(ep_lf * la);
    float b = 0.5f * (1.0f + fabsf(a)) * (ep_lf * up);
    result[i] = a + b;
    
    if (best_result > result[i]) continue;
    
    best_result = result[i];
    best_ep_id = (the_edit_plane_id_t)i;
  }
  
  if ((best_result - result[active_ep_id_]) / best_result > 0.3)
  {
    active_ep_id_ = best_ep_id;
  }
  
  return (prev_ep_id != active_ep_id_);
}

//----------------------------------------------------------------
// the_view_t::attach_eh
// 
void
the_view_t::attach_eh(the_input_device_eh_t * eh)
{
  eh_stack_->push(eh);
}

//----------------------------------------------------------------
// the_view_t::detach_eh
// 
void
the_view_t::detach_eh(the_input_device_eh_t * eh)
{
  assert(std::find(eh_stack_->begin(), eh_stack_->end(), eh) !=
	 eh_stack_->end());
  eh_stack_->remove(eh);
}

//----------------------------------------------------------------
// the_view_t::calc_bbox
//
void
the_view_t::calc_bbox(the_bbox_t & bbox) const
{
  if (document() == NULL) return;
  document()->calc_bbox(*this, bbox);
}

//----------------------------------------------------------------
// the_view_t::gl_setup
// 
void
the_view_t::gl_setup()
{
  the_scoped_variable_t<the_view_t *>(latest_view_, this, NULL);
  
  glShadeModel(GL_SMOOTH);
  glClearDepth(1);
  glClearStencil(0);
  glClearAccum(0, 0, 0, 1);
  glClearColor(0, 0, 0, 1);
  
  // common material properties:
  static const float mat_ambient[]   = { 0.1f, 0.1f, 0.1f, 1.0f };
  static const float mat_specular[]  = { 0.7f, 0.7f, 0.7f, 1.0f };
  static const float mat_shininess[] = { 7.0f };
  glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
  glMaterialfv(GL_BACK,  GL_AMBIENT,   mat_ambient);
  glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
  glMaterialfv(GL_BACK,  GL_SPECULAR,  mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glMaterialfv(GL_BACK,  GL_SHININESS, mat_shininess);
  FIXME_OPENGL("the_view_t::initializeGL A");
  
  // light properties:
  static const float light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
  static const float light_specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };
  static const float light_ambient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
  FIXME_OPENGL("the_view_t::initializeGL B");
  
  static const float global_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  FIXME_OPENGL("the_view_t::initializeGL C");
  
  glEnable(GL_LIGHT0);
  FIXME_OPENGL("the_view_t::initializeGL D");
  
  static const float fog_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  glFogi(GL_FOG_MODE, GL_LINEAR);
  glFogfv(GL_FOG_COLOR, fog_color);
  glHint(GL_FOG_HINT, GL_NICEST);
  glDepthFunc(GL_LESS);
  glLineWidth(1.0f);
}

//----------------------------------------------------------------
// int
// 
void
the_view_t::gl_resize(int w, int h)
{
  the_scoped_variable_t<the_view_t *>(latest_view_, this, NULL);
  
  view_mgr().resize(w, h);
}

//----------------------------------------------------------------
// the_view_t::gl_paint
// 
void
the_view_t::gl_paint()
{
  if (width() == 0 || height() == 0) return;
  
  the_scoped_variable_t<the_view_t *> latest_view(latest_view_, this, NULL);
  
  // disable view manager callbacks when repainting -- we don't
  // want to trigger recursive repaints:
  the_view_mgr_t::callback_suppressor_t suppressor(&view_mgr());
  
  // calculate the bounding box of all the documents that this
  // view is currently displaying:
  the_bbox_t documents_bbox;
  calc_bbox(documents_bbox);
  
  // update the view manager:
  view_mgr().update_scene_radius(documents_bbox);

  std::list<the_view_mgr_t::stereoscopic_t> render_mode;
  if (!stereoscopic_)
  {
    render_mode.push_back(the_view_mgr_t::NOT_STEREOSCOPIC_E);
  }
  else
  {
    render_mode.push_back(the_view_mgr_t::STEREOSCOPIC_LEFT_EYE_E);
    render_mode.push_back(the_view_mgr_t::STEREOSCOPIC_RIGHT_EYE_E);
  }
  
  for (std::list<the_view_mgr_t::stereoscopic_t>::const_iterator
         i = render_mode.begin(); i != render_mode.end(); ++i)
  {
    the_view_mgr_t::stereoscopic_t stereo = *i;
    view_mgr().set_stereoscopic(stereo);
    
#if 1
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // draw the background:
    THE_APPEARANCE.draw_background(*this);
    FIXME_OPENGL("the_view_t::paintGL A");
#endif
    
    // check whether antialiasing/fog are allowed:
    bool enable_gfx_effects = true;
    if (eh_stack().empty() == false)
    {
      enable_gfx_effects = !eh_stack().front()->has_drag();
    }
    
#if 1
    // setup antialiasing:
    if (local_aa_ && enable_gfx_effects)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
      glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      FIXME_OPENGL("the_view_t::paintGL B");
    }
    else
    {
      glDisable(GL_BLEND);
      glDisable(GL_LINE_SMOOTH);
      FIXME_OPENGL("the_view_t::paintGL C");
    }
    
    // draw the edit plane:
    THE_APPEARANCE.draw_edit_plane(*this);
    FIXME_OPENGL("the_view_t::paintGL D");
#endif
    
    // setup 3D viewing:
    view_mgr().reset_opengl_viewing();
    view_mgr().setup_opengl_3d_viewing();
    FIXME_OPENGL("the_view_t::paintGL E");
    
    // setup fog (for depth cue):
    if (local_dq_ && enable_gfx_effects &&
        !(documents_bbox.is_empty() || documents_bbox.is_singular()))
    {
      the_coord_sys_t vcs(documents_bbox.wcs_center(),
                          !(view_mgr().la() - view_mgr().lf()),
                          !(view_mgr().up()));
      
      the_bbox_t bbox(vcs);
      bbox += documents_bbox;
      
      // don't set up fog when the bounding box is flat:
      if (bbox.is_spacial())
      {
        the_ray_t view_ray(view_mgr().lf(), !(view_mgr().la() -
                                              view_mgr().lf()));
        
        float a = view_ray * bbox.wcs_center();
        float b = view_ray * bbox.wcs_max();
        
        glFogf(GL_FOG_START, a);
        glFogf(GL_FOG_END, a + 3.0f * (b - a));
        glEnable(GL_FOG);
      }
    }
    else
    {
      glDisable(GL_FOG);
    }
    FIXME_OPENGL("the_view_t::paintGL F");
    
    // setup directional lighting:
    // NOTE: OpenGL is weird, it wants the light "direction" to be
    // specified backwards. If you want light along (0, 0, 1) vector,
    // specify (0, 0, -1, 0) as the light position:
    v3x1_t light_dir = (view_mgr().lf() - view_mgr().la());
    p4x1_t light(light_dir.x(), light_dir.y(), light_dir.z(), 0.0);
    glLightfv(GL_LIGHT0, GL_POSITION, light.data());
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    FIXME_OPENGL("the_view_t::paintGL G");
    
    // draw the document:
    {
      glClear(GL_DEPTH_BUFFER_BIT);
      
      if (document() != NULL)
      {
        document()->draw(*this);
      }
    }
    
#if 1
    // when drawing event handler geometry depth testing is unnecessary:
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    // draw the event handler 3D geometry:
    {
      for (std::list<the_input_device_eh_t *>::const_iterator
             i = eh_stack_->begin(); i != eh_stack_->end(); ++i)
      {
        (*i)->dl().execute();
      }
    }
    
    // draw the event handler 3D geometry that is specific to this view:
    dl_eh_3d_.draw();
    
    // disable fog depth queing:
    glDisable(GL_FOG);
    
    // draw the coordinate system:
    THE_APPEARANCE.draw_coordinate_system(*this);
    FIXME_OPENGL("the_view_t::paintGL I");
    
    // draw whatever (2D) is needed by the event handler:
    view_mgr().reset_opengl_viewing();
    view_mgr().setup_opengl_2d_viewing(p2x1_t(0.0f, float(height())),
                                       p2x1_t(float(width()), 0.0f));
    FIXME_OPENGL("the_view_t::paintGL J");
    
    dl_eh_2d_.draw();
    FIXME_OPENGL("the_view_t::paintGL K");
    
    // draw the view label:
    THE_APPEARANCE.draw_view_label(*this);
    FIXME_OPENGL("the_view_t::paintGL L");
    
    // draw the pointing device (mouse) cursor, if required:
    if (view_mgr_->get_cursor_id() != THE_BLANK_CURSOR_E &&
        eh_stack().most_recent_event() &&
        eh_stack().most_recent_event()->widget() == this)
    {
      const the_input_device_event_t * e =
        eh_stack().most_recent_event();

      p2x1_t scs_pt = eh_stack().most_recent_mouse_event().scs_pt_;
      
      const the_pointer_device_event_t * pe =
        dynamic_cast<const the_pointer_device_event_t *>(e);
      if (pe)
      {
        scs_pt = pe->scs_pt();
      }
      
      p3x1_t cursor_pos(scs_pt.x() * float(width()),
                        scs_pt.y() * float(height()),
                        0.0f);
      
      bool data_is_bottom_up = false;
      the_cursor_t cursor(view_mgr_->get_cursor_id());
      the_bitmap_dl_elem_t(cursor_pos,
                           data_is_bottom_up,
                           cursor.icon_,
                           cursor.mask_,
                           cursor.w_,
                           cursor.h_,
                           cursor.x_,
                           cursor.y_).draw();
    }
#endif
  }
  
  dl_eh_3d_.clear();
  dl_eh_2d_.clear();
}
