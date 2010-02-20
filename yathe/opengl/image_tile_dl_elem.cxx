// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : image_tile_dl_elem.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a display list element for the image tiles

// GLEW includes:
#define GLEW_STATIC 1
#include <GL/glew.h>

// local includes:
#include "opengl/image_tile_dl_elem.hxx"
#include "opengl/the_gl_context.hxx"

// forward declarations:
#ifdef USE_CG
extern CGcontext cg_context;
extern CGprofile cg_profile;
#endif

//----------------------------------------------------------------
// DEBUG_TEXTURE_IDS
// 
// #define DEBUG_TEXTURE_IDS


//----------------------------------------------------------------
// image_tile_dl_elem_t::image_tile_dl_elem_t
// 
image_tile_dl_elem_t::image_tile_dl_elem_t(const image_tile_generator_t & data,
					   GLenum min_filter,
					   GLenum mag_filter):
  data_(data),
  min_filter_(min_filter),
  mag_filter_(mag_filter)
#ifdef USE_CG
  ,fragment_program_(NULL)
#endif
{
  p3x1_t min(float(data_.origin_x_),
	     float(data_.origin_y_),
	     0);
  v3x1_t ext(float(data_.spacing_x_ * data_.w_),
	     float(data_.spacing_y_ * data_.h_),
	     0);
  bbox_ << min << min + ext;
}

#ifdef USE_CG
//----------------------------------------------------------------
// image_tile_dl_elem_t::image_tile_dl_elem_t
// 
image_tile_dl_elem_t::image_tile_dl_elem_t(const image_tile_generator_t & data,
					   GLenum min_filter,
					   GLenum mag_filter,
					   const CGprogram * fragment_program):
  data_(data),
  min_filter_(min_filter),
  mag_filter_(mag_filter),
  fragment_program_(fragment_program)
{
  p3x1_t min(float(data_.origin_x_),
	     float(data_.origin_y_),
	     0);
  v3x1_t ext(float(data_.spacing_x_ * data_.w_),
	     float(data_.spacing_y_ * data_.h_),
	     0);
  bbox_ << min << min + ext;
}
#endif

//----------------------------------------------------------------
// image_tile_dl_elem_t::~image_tile_dl_elem_t
// 
image_tile_dl_elem_t::~image_tile_dl_elem_t()
{
  size_t num_textures = texture_id_.size();
  if (num_textures == 0) return;
  
  the_gl_context_t current(the_gl_context_t::current());
  if (context_.is_valid())
  {
    context_.make_current();
  }
  
  GLuint * texture_ids = &(texture_id_[0]);
  
#ifdef DEBUG_TEXTURE_IDS
  cerr << this << ", deleting textures:";
#endif // DEBUG_TEXTURE_IDS
  
  for (size_t i = 0; i < num_textures; i++)
  {
#ifdef DEBUG_TEXTURE_IDS
    cerr << ' ' << texture_ids[i];
#endif // DEBUG_TEXTURE_IDS
    
    if (!glIsTexture(texture_ids[i]))
    {
      assert(false);
    }
  }
#ifdef DEBUG_TEXTURE_IDS
  cerr << endl;
#endif // DEBUG_TEXTURE_IDS
  
  glDeleteTextures((GLsizei)num_textures, texture_ids);
  FIXME_OPENGL("image_tile_dl_elem_t::~image_tile_dl_elem_t");
  
  if (current.is_valid())
  {
    current.make_current();
  }
  
  // forget the context:
  context_.invalidate();
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::setup_textures
// 
void
image_tile_dl_elem_t::setup_textures() const
{
  // shortcuts:
  const image_tile_t * tiles = &(data_.tiles_[0]);
  const size_t num_tiles = data_.tiles_.size();
  
  bool must_init = texture_id_.empty() && num_tiles > 0;
  if (must_init)
  {
    texture_id_.resize(num_tiles);
    texture_ok_.resize(num_tiles);
  }
  
  // shortcut to texture ids:
  GLuint * texture_ids = &(texture_id_[0]);
  
  // number of available texture units:
  GLint num_texture_units = 0;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_texture_units);
  PERROR_OPENGL("number of texture units");
  
  // upload the texture data as necessary:
  if (must_init)
  {
    // store the context:
    context_ = the_gl_context_t::current();
    FIXME_OPENGL("switch context");
    
    // allocate the texture objects:
    glGenTextures((GLsizei)num_tiles, texture_ids);
    FIXME_OPENGL("image_tile_dl_elem_t::draw");
    
    // FIXME:
#ifdef DEBUG_TEXTURE_IDS
    cerr << this << ", generating textures:";
    for (size_t i = 0; i < num_tiles; i++)
    {
      cerr << ' ' << texture_ids[i];
    }
    cerr << endl;
#endif
    
    // setup the textures:
    for (size_t i = 0; i < num_tiles; i++)
    {
#ifdef USE_CG
      if (has_fragment_program())
      {
	glActiveTexture(GL_TEXTURE0 + i % num_texture_units);
      }
#endif
      
      const image_tile_t & tile = tiles[i];
      texture_ok_[i] = tile.texture_->setup(texture_ids[i]);
    }
    
    // upload the texture data:
    if (upload_.empty())
    {
      for (size_t i = 0; i < num_tiles; i++)
      {
	if (!texture_ok_[i]) continue;
	
	const image_tile_t & tile = tiles[i];
	tile.texture_->upload(texture_ids[i]);
      }
    }
  }
  
  if (!upload_.empty())
  {
    while (!upload_.empty())
    {
      image_tile_t::quad_t quad = remove_head(upload_);
      for (size_t i = 0; i < num_tiles; i++)
      {
	if (!texture_ok_[i]) continue;
	
	const image_tile_t & tile = tiles[i];
	tile.texture_->upload(texture_ids[i],
			      quad.x_,
			      quad.y_,
			      quad.w_,
			      quad.h_);
	// cerr << "upload " << quad << endl;
      }
    }
  }
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::draw
// 
void
image_tile_dl_elem_t::draw() const
{
  draw(draw_tile, this, the_color_t::WHITE, true);
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::draw
// 
void
image_tile_dl_elem_t::draw(draw_tile_cb_t draw_tile_cb,
			   const void * draw_tile_cb_data,
			   const the_color_t & color,
			   const bool & use_textures) const
{
  // shortcuts:
  const image_tile_t * tiles = &(data_.tiles_[0]);
  const size_t num_tiles = data_.tiles_.size();
  
  setup_textures();
  
#ifdef USE_CG
  CGparameter texture_param = 0;
  CGparameter tile_size_param = 0;
  CGparameter antialias_param = 0;
  CGparameter tint_param = 0;
  CGparameter mask_param = 0;
  
  if (has_fragment_program())
  {
    cgGLLoadProgram(*fragment_program_);
    texture_param = cgGetNamedParameter(*fragment_program_, "tile_texture");
    tile_size_param = cgGetNamedParameter(*fragment_program_, "tile_size");
    antialias_param = cgGetNamedParameter(*fragment_program_, "antialias");
    tint_param = cgGetNamedParameter(*fragment_program_, "tint");
    mask_param = cgGetNamedParameter(*fragment_program_, "mask");
    
    FIXME_OPENGL("GPU Program ON");
  }
  
  // check whether fragments should be anti-aliased:
  bool antialias = min_filter_ != GL_NEAREST;
#endif
  
  // number of available texture units:
  GLint num_texture_units = 0;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_texture_units);
  PERROR_OPENGL("number of texture units");
  
  // draw the tiles:
  for (size_t i = 0; i < num_tiles; i++)
  {
    if (!texture_ok_[i]) continue;
    
    const image_tile_t & tile = tiles[i];
    
#ifdef USE_CG
    if (has_fragment_program())
    {
      glActiveTexture(GL_TEXTURE0 + i % num_texture_units);
      
      cgGLBindProgram(*fragment_program_);
      cgGLEnableProfile(cg_profile);
      
      cgGLSetTextureParameter(texture_param, texture_id_[i]);
      cgGLEnableTextureParameter(texture_param);
      
      cgGLSetParameter2f(tile_size_param,
			 float(tile.texture_->width_),
			 float(tile.texture_->height_));
      
      if (antialias_param != 0)
      {
	cgGLSetParameter1f(antialias_param,
			   float(antialias));
      }
      
      if (tint_param != 0)
      {
	cgGLSetParameter3f(tint_param,
			   color[0],
			   color[1],
			   color[2]);
      }
      
      if (mask_param != 0)
      {
	cgGLSetParameter1f(mask_param, float(!use_textures));
      }
      
      // the fragment shader will handle anti-aliasing if necessary:
      draw(draw_tile_cb,
	   draw_tile_cb_data,
	   i,
	   color,
	   use_textures,
	   texture_id_[i],
	   GL_NEAREST,
	   GL_NEAREST);
      
      cgGLDisableTextureParameter(texture_param);
      cgGLDisableProfile(cg_profile);
    }
    else
#endif
    {
      draw(draw_tile_cb,
	   draw_tile_cb_data,
	   i,
	   color,
	   use_textures,
	   texture_id_[i],
	   min_filter_,
	   mag_filter_);
    }
  }
  
#ifdef USE_CG
  if (has_fragment_program())
  {
    cgGLUnbindProgram(cg_profile);
    FIXME_OPENGL("GPU Program OFF");
    
    // restore classic OpenGL texturing:
    glActiveTexture(GL_TEXTURE0);
  }
#endif
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::draw
// 
void
image_tile_dl_elem_t::draw(draw_tile_cb_t draw_tile_cb,
			   const void * draw_tile_cb_data,
			   const size_t & tile_index,
			   const the_color_t & color,
			   const bool & use_textures,
			   const GLuint & texture_id,
			   const GLint & mag_filter,
			   const GLint & min_filter) const
{
  /*
  glPushAttrib(GL_TEXTURE_BIT |
	       GL_ENABLE_BIT |
	       GL_POLYGON_BIT |
	       GL_COLOR_BUFFER_BIT);
  */
  the_scoped_gl_attrib_t push_attr(GL_ALL_ATTRIB_BITS);
  {
    // if (use_textures)
    {
      glEnable(GL_TEXTURE_2D);
      data_.tiles_[tile_index].texture_->apply(texture_id);
    }
    // else
    {
      glColor4f(1, 1, 1, 1);
    }
    
    if (color[3] == 0.0)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
    }
    else
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    
    // setup magnification/minification filtering:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    
#if 0
    if (has_fragment_program() && use_textures)
    {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }
    else
    {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    glDisable(GL_LIGHTING);
    // glColor4f(1, 1, 1, 1);
    glColor4fv(color.rgba());
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0, 1.0);
    draw_tile_cb(draw_tile_cb_data, tile_index);
    
    glColor4f(drand(), drand(), drand(), 1);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonOffset(1.0, 1.0);
    glLineWidth(1);
    
#else // DEBUG_TEXTURES
    if (has_fragment_program() && use_textures)
    {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    }
    else
    {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3fv(color.rgba());
    
#endif // DEBUG_TEXTURES
    
    draw_tile_cb(draw_tile_cb_data, tile_index);
  }
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::draw_tile
// 
void
image_tile_dl_elem_t::draw_tile(const void * data,
				const size_t & tile_index)
{
  const image_tile_dl_elem_t * image = (const image_tile_dl_elem_t *)(data);
  const image_tile_t & tile = image->data_.tiles_[tile_index];
  /*
#ifdef USE_CG
  if (image->has_fragment_program())
  {
    const CGprogram & fragment_program = *(image->fragment_program_);
    
    CGparameter texture_param = 0;
    CGparameter tile_size_param = 0;
    CGparameter antialias_param = 0;
    
    cgGLLoadProgram(fragment_program);
    texture_param = cgGetNamedParameter(fragment_program, "tile_texture");
    tile_size_param = cgGetNamedParameter(fragment_program, "tile_size");
    antialias_param = cgGetNamedParameter(fragment_program, "antialias");
    
    FIXME_OPENGL("GPU Program ON");
  
    // check whether fragments should be anti-aliased:
    bool antialias = image->min_filter_ != GL_NEAREST;
    
    GLint num_texture_units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_texture_units);
    glActiveTexture(GL_TEXTURE0 + tile_index % num_texture_units);
    
    cgGLBindProgram(fragment_program);
    cgGLEnableProfile(cg_profile);
    
    cgGLSetTextureParameter(texture_param, image->texture_id_[tile_index]);
    cgGLEnableTextureParameter(texture_param);
    
    cgGLSetParameter2f(tile_size_param,
		       float(tile.texture_->width_),
		       float(tile.texture_->height_));
    
    cgGLSetParameter1f(antialias_param,
		       float(antialias));
  }
#endif
  */
  
  glBegin(GL_QUADS);
  {
    glTexCoord2f(tile.s0_, tile.t0_);
    glVertex2fv(tile.corner_[0].data());
    
    glTexCoord2f(tile.s1_, tile.t0_);
    glVertex2fv(tile.corner_[1].data());
    
    glTexCoord2f(tile.s1_, tile.t1_);
    glVertex2fv(tile.corner_[2].data());
    
    glTexCoord2f(tile.s0_, tile.t1_);
    glVertex2fv(tile.corner_[3].data());
  }
  glEnd();
}

//----------------------------------------------------------------
// image_tile_dl_elem_t::update_bbox
// 
void
image_tile_dl_elem_t::update_bbox(the_bbox_t & bbox) const
{
  bbox += bbox_;
}
