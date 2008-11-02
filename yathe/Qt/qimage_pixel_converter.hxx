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
#include <QImage>


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
