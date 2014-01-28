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
{
  p3x1_t min(float(data_.origin_x_),
	     float(data_.origin_y_),
	     0);
  v3x1_t ext(float(data_.spacing_x_ * data_.w_),
	     float(data_.spacing_y_ * data_.h_),
	     0);
  bbox_ << min << min + ext;
}

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

  // number of available texture units:
  GLint num_texture_units = 0;
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &num_texture_units);
  PERROR_OPENGL("number of texture units");

  // draw the tiles:
  for (size_t i = 0; i < num_tiles; i++)
  {
    if (!texture_ok_[i]) continue;

    const image_tile_t & tile = tiles[i];

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

    // don't allow fragments less than half opaque:
    // glEnable(GL_ALPHA_TEST);
    // glAlphaFunc(GL_GREATER, 0.5f);

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
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

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

//----------------------------------------------------------------
// image_tile_dl_elem_t::get_texture_info
//
bool
image_tile_dl_elem_t::get_texture_info(GLenum & data_type,
                                       GLenum & format_internal,
                                       GLenum & format) const
{
  if (!data_.tiles_.empty())
  {
    boost::shared_ptr<texture_base_t> t = data_.tiles_.front().texture_;
    const texture_base_t * texture = t.get();
    data_type = texture->type_;
    format_internal = texture->internal_format_;
    format = texture->format_;
    return true;
  }

  return false;
}
