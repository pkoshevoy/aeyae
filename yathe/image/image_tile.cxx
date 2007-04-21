// File         : image_tile.cxx
// Author       : Paul A. Koshevoy
// Created      : Sun Sep 3 18:04 MDT 2006
// Copyright    : (C) 2006
// License      : GPL.
// Description  : a textured image tile convenience class

// local includes:
#include "image/image_tile.hxx"
#include "image/texture.hxx"
#include "math/the_bbox.hxx"
#include "math/the_color.hxx"
#include "opengl/OpenGLCapabilities.h"
#include "utils/the_utils.hxx"
#include "utils/the_indentation.hxx"



//----------------------------------------------------------------
// draw_tile
// 
// FIXME: eventually I'll have to benchmark to see what is faster -
// drawing one quad or a set of triangle strips.
// 
static void
draw_tile(const p3x1_t * corners,
	  const GLfloat & s0,
	  const GLfloat & s1,
	  const GLfloat & t0,
	  const GLfloat & t1)
{
  glBegin(GL_QUADS);
  {
    glTexCoord2f(s0, t0);
    glVertex2fv(corners[0].data());
    
    glTexCoord2f(s1, t0);
    glVertex2fv(corners[1].data());
    
    glTexCoord2f(s1, t1);
    glVertex2fv(corners[2].data());
    
    glTexCoord2f(s0, t1);
    glVertex2fv(corners[3].data());
  }
  glEnd();
}


//----------------------------------------------------------------
// image_tile_t::image_tile_t
// 
image_tile_t::image_tile_t():
  s0_(0),
  s1_(0),
  t0_(0),
  t1_(0)
{}

//----------------------------------------------------------------
// image_tile_t::~image_tile_t
// 
image_tile_t::~image_tile_t()
{}

//----------------------------------------------------------------
// image_tile_t::draw
// 
void
image_tile_t::draw(const GLuint & texture_id,
		   const GLint & mag_filter,
		   const GLint & min_filter) const
{
  glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
  {
    glEnable(GL_TEXTURE_2D);
    
    texture_->apply(texture_id);
    
    // setup magnification/minification filtering:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    FIXME_OPENGL("image_tile_t::draw A");
    
    // FIXME: #define DEBUG_TEXTURES
#ifdef DEBUG_TEXTURES
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_LIGHTING);
    glColor4f(1, 1, 1, 1);
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonOffset(1.0, 1.0);
    draw_tile(corner_, s0_, s1_, t0_, t1_);
    
    glColor4f(drand(), drand(), drand(), 1);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonOffset(1.0, 1.0);
    glLineWidth(1);
    
#else // DEBUG_TEXTURES
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
#endif // DEBUG_TEXTURES
    FIXME_OPENGL("image_tile_t::draw B");
    
    draw_tile(corner_, s0_, s1_, t0_, t1_);
  }
  glPopAttrib();
  FIXME_OPENGL("image_tile_t::draw C");
}

//----------------------------------------------------------------
// image_tile_t::update_bbox
// 
void
image_tile_t::update_bbox(the_bbox_t & bbox) const
{
  bbox << corner_[0]
       << corner_[1]
       << corner_[2]
       << corner_[3];
}
