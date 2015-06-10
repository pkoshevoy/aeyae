// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : qimage_pixel_converter.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : A pixel converter used to generate OpenGL textures
//                from Qt images.

#ifndef QIMAGE_PIXEL_CONVERTER_HXX_
#define QIMAGE_PIXEL_CONVERTER_HXX_

// the includes:
#include <image/image_tile_generator.hxx>
#include <opengl/OpenGLCapabilities.h>

// Qt includes:
#include <qimage.h>


//----------------------------------------------------------------
// qimage_pixel_converter_t
//
class qimage_pixel_converter_t : public pixel_converter_t
{
public:
  qimage_pixel_converter_t(const QImage & image,
			   const size_t & src_bytes_per_pixel,
			   const size_t & dst_bytes_per_pixel,
			   const GLenum & format);

  // virtual:
  void operator() (unsigned char * dst_addr,
		   const unsigned char * src_addr,
		   const size_t & src_bytes_to_read) const;

  // a reference to the image:
  const QImage & image_;
  const unsigned char * src_origin_;
  const size_t src_bytes_per_pixel_;
  const size_t dst_bytes_per_pixel_;
  const size_t src_bytes_per_line_;

  // OpenGL format of the destination image:
  const GLenum format_;
};


#endif // QIMAGE_PIXEL_CONVERTER_HXX_
