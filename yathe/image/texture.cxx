// File         : texture.cxx
// Author       : Paul A. Koshevoy
// Created      : Sun Sep 3 18:15:00 MDT 2006
// Copyright    : (C) 2006
// License      : GPL.
// Description  : a texture convenience class

// local includes:
#include "image/texture.hxx"
#include "opengl/OpenGLCapabilities.h"

// system includes:
#include <assert.h>
#include <iostream>

// namespace access:
using std::cerr;
using std::endl;


//----------------------------------------------------------------
// texture_base_t::texture_base_t
// 
texture_base_t::texture_base_t(GLenum type,
			       GLint internal_format,
			       GLenum format,
			       GLsizei width,
			       GLsizei height,
			       GLint border,
			       GLint alignment,
			       GLint row_length,
			       GLint skip_pixels,
			       GLint skip_rows,
			       GLboolean swap_bytes,
			       GLboolean lsb_first):
  type_(type),
  internal_format_(internal_format),
  format_(format),
  width_(width),
  height_(height),
  border_(border),
  alignment_(alignment),
  row_length_(row_length),
  skip_pixels_(skip_pixels),
  skip_rows_(skip_rows),
  swap_bytes_(swap_bytes),
  lsb_first_(lsb_first)
{
  assert(skip_pixels_ >= 0);
  assert(skip_rows_ >= 0);
}

//----------------------------------------------------------------
// texture_base_t::~texture_base_t
// 
texture_base_t::~texture_base_t()
{}

//----------------------------------------------------------------
// texture_base_t::setup
// 
bool
texture_base_t::setup(const GLuint & texture_id) const
{
  glBindTexture(GL_TEXTURE_2D, texture_id);
  if (!glIsTexture(texture_id)) return false;
  
  FIXME_OPENGL("texture_base_t::setup");
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       internal_format_,
	       width_,
	       height_,
	       border_,
	       format_,
	       type_,
	       NULL);
  
  GLenum err = glGetError();
  if (err != GL_NO_ERROR)
  {
#ifndef NDEBUG
    const GLubyte * str = gluErrorString(err);
    cerr << "ERROR: OpenGL: " << str << endl;
#endif
    
    return false;
  }
  
  static const GLclampf priority = 1;
  glPrioritizeTextures(1, &texture_id, &priority);
  
  return is_valid(texture_id);
}

//----------------------------------------------------------------
// texture_base_t::is_valid
// 
bool
texture_base_t::is_valid(const GLuint & texture_id) const
{
  glBindTexture(GL_TEXTURE_2D, texture_id);
  if (!glIsTexture(texture_id)) return false;
  
  // make sure we were able to allocate the necesary chunk of memory:
  GLsizei w = 0;
  GLsizei h = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
  glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
  return (w == width_ && h == height_);
}

//----------------------------------------------------------------
// texture_base_t::upload
// 
void
texture_base_t::upload(const GLuint & texture_id,
		       GLint x,
		       GLint y,
		       GLsizei w,
		       GLsizei h) const
{
  if (!is_valid(texture_id)) return;
  
  const GLint & x_min = skip_pixels_;
  const GLint & y_min = skip_rows_;
  GLint x_max = x_min + width_ - 1;
  GLint y_max = y_min + height_ - 1;
  
  GLint x1 = x + w - 1;
  GLint y1 = y + h - 1;
  
  if (x > x_max || x1 < x_min ||
      y > y_max || y1 < y_min)
  {
    // the region falls entirely outside the texture:
    return;
  }
  
  GLint x0 = std::min(x_max, std::max(x_min, x));
  x1 = std::min(x_max, std::max(x_min, x1));
  
  GLint y0 = std::min(y_max, std::max(y_min, y));
  y1 = std::min(y_max, std::max(y_min, y1));
  
  GLint offset_x = x0 - x_min;
  GLint offset_y = y0 - y_min;
  GLsizei cropped_w = x1 - x0 + 1;
  GLsizei cropped_h = y1 - y0 + 1;
  
  // FIXME: #define DEBUG_TEXTURES
#ifdef DEBUG_TEXTURES
  cerr << "texture id: " << texture_id << endl
       << "type: " << type_ << endl
       << "internal format: " << internal_format_ << endl
       << "format: " << format_ << endl
       << "width: " << width_ << endl
       << "height: " << height_ << endl
       << "border: " << border_ << endl
       << "alignment: " << alignment_ << endl
       << "row length: " << row_length_ << endl
       << "skip pixels: " << skip_pixels_ << endl
       << "skip rows: " << skip_rows_ << endl
       << "swap bytes: " << int(swap_bytes_) << endl
       << "lsb first: " << int(lsb_first_) << endl
       << endl;
  
  cerr << "x_min: " << x_min << endl
       << "x_max: " << x_max << endl
       << "y_min: " << y_min << endl
       << "y_max: " << y_max << endl
       << "x0: " << x0 << endl
       << "x1: " << x1 << endl
       << "y0: " << y0 << endl
       << "y1: " << y1 << endl
       << "x: " << x << endl
       << "y: " << y << endl
       << "w: " << w << endl
       << "h: " << h << endl
       << "offset_x: " << offset_x << endl
       << "offset_y: " << offset_y << endl
       << "cropped_w: " << cropped_w << endl
       << "cropped_h: " << cropped_h << endl
       << endl;
  
#endif // DEBUG_TEXTURES
  
  glBindTexture(GL_TEXTURE_2D, texture_id);
  
  glPushClientAttrib(GL_UNPACK_ALIGNMENT);
  {
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length_);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels_ + offset_x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows_ + offset_y);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, swap_bytes_);
    glPixelStorei(GL_UNPACK_LSB_FIRST, lsb_first_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment_);
    
    glTexSubImage2D(GL_TEXTURE_2D,
		    0,
		    offset_x,
		    offset_y,
		    cropped_w,
		    cropped_h,
		    format_,
		    type_,
		    texture());
    // FIXME_OPENGL("texture_base_t::upload");
  }
  glPopClientAttrib();
}

//----------------------------------------------------------------
// texture_base_t::apply
// 
void
texture_base_t::apply(const GLuint & texture_id) const
{
  if (!is_valid(texture_id)) return;
  
  // setup texture clamping:
  if (border_ != 0)
  {
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // FIXME: the texture class hasn't been tested recently with border = 1:
    static const GLfloat border[] = { 0, 0, 0, 1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  
  glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_FALSE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
}

//----------------------------------------------------------------
// texture_base_t::debug
// 
void
texture_base_t::debug() const
{
  GLint bytes_per_pixel = 1;
  switch (format_)
  {
    case GL_LUMINANCE:
      bytes_per_pixel = 1;
      break;
      
    case GL_LUMINANCE_ALPHA:
      bytes_per_pixel = 2;
      break;
      
    case GL_RGB:
      bytes_per_pixel = 3;
      break;
      
    case GL_RGBA:
    case GL_BGRA_EXT:
      bytes_per_pixel = 4;
      break;
      
    default:
      assert(0);
      break;
  }
  
  const GLint padding_bytes =
    (alignment_ - (bytes_per_pixel * row_length_) % alignment_) % alignment_;
  cerr << "FIXME: padding bytes: " << padding_bytes << endl;
  
  const GLint bytes_per_line = bytes_per_pixel * row_length_ + padding_bytes;
  
  // make sure the texture memory is allocated correctly:
  GLubyte * tex = const_cast<GLubyte *>(texture());
  for (GLint j = 0; j < height_; j++)
  {
    GLint y = skip_rows_ + j;
    for (GLint i = 0; i < width_; i++)
    {
      GLint x = skip_pixels_ + i;
      
      GLubyte * byte = tex + bytes_per_line * y + bytes_per_pixel * x;
      GLubyte val = GLubyte(255.0 * double(j + i) / double(width_ + height_));
      for (GLint k = 0; k < bytes_per_pixel; k++)
      {
	byte[k] = val;
      }
    }
  }
}
