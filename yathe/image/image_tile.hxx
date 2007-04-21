// File         : image_tile.hxx
// Author       : Paul A. Koshevoy
// Created      : Sun Sep 3 18:01:00 MDT 2006
// Copyright    : (C) 2006
// License      : GPL.
// Description  : a textured image tile convenience class

#ifndef IMAGE_TILE_HXX_
#define IMAGE_TILE_HXX_

// system includes:
#include <iostream>

// Boost includes:
#include <boost/shared_ptr.hpp>

// local includes:
#include "opengl/OpenGLCapabilities.h"
#include "utils/the_indentation.hxx"
#include "math/v3x1p3x1.hxx"

// namespace access:
using std::ostream;
using std::endl;

// forward declarations:
class texture_base_t;
class the_bbox_t;


//----------------------------------------------------------------
// image_tile_t
//
// a texture mapped OpenGL quadrilateral.
//
class image_tile_t
{
public:
  image_tile_t();
  ~image_tile_t();
  
  // OpenGL helper:
  void draw(const GLuint & texture_id,
	    const GLint & mag_filter = GL_NEAREST,
	    const GLint & min_filter = GL_NEAREST) const;
  
  // helper:
  void update_bbox(the_bbox_t & bbox) const;
  
  //----------------------------------------------------------------
  // quad_t
  // 
  class quad_t
  {
  public:
    inline void dump(ostream & strm, unsigned int indent = 0) const
    {
      strm << INDSCP << "quad_t(" << (void *)this << ")" << endl
	   << INDSCP << "{" << endl
	   << INDSTR << "x_ = " << x_ << endl
	   << INDSTR << "y_ = " << y_ << endl
	   << INDSTR << "w_ = " << w_ << endl
	   << INDSTR << "h_ = " << h_ << endl
	   << INDSCP << "}" << endl;
    }
    
    GLint x_;
    GLint y_;
    GLsizei w_;
    GLsizei h_;
  };
  
  // quad coordinates:
  p3x1_t corner_[4];
  
  // texture coordinates:
  GLfloat s0_;
  GLfloat s1_;
  GLfloat t0_;
  GLfloat t1_;
  
  // the texture:
  boost::shared_ptr<texture_base_t> texture_;
};


//----------------------------------------------------------------
// operator <<
// 
inline ostream &
operator << (ostream & strm, const image_tile_t::quad_t & quad)
{
  quad.dump(strm);
  return strm;
}


#endif // IMAGE_TILE_HXX_
