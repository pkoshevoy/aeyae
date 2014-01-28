// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : qimage_pixel_converter.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : A pixel converter used to generate OpenGL textures
//                from Qt images.

// the includes:
#include <Qt3/qimage_pixel_converter.hxx>


//----------------------------------------------------------------
// qimage_pixel_converter_t::qimage_pixel_converter_t
//
qimage_pixel_converter_t::
qimage_pixel_converter_t(const QImage & image,
			 const size_t & src_bytes_per_pixel,
			 const size_t & dst_bytes_per_pixel,
			 const GLenum & format):
  pixel_converter_t(),
  image_(image),
  src_origin_(const_cast<QImage &>(image).bits()),
  src_bytes_per_pixel_(src_bytes_per_pixel),
  dst_bytes_per_pixel_(dst_bytes_per_pixel),
  src_bytes_per_line_(image.bytesPerLine()),
  format_(format)
{}

//----------------------------------------------------------------
// qimage_pixel_converter_t::operator
//
void
qimage_pixel_converter_t::operator() (unsigned char * dst_addr,
				      const unsigned char * src_addr,
				      const size_t & src_bytes_to_read) const
{
#ifdef __BIG_ENDIAN__
  #define RGBA_R 3
  #define RGBA_G 2
  #define RGBA_B 1
  #define RGBA_A 0

  #define BGRA_B 3
  #define BGRA_G 2
  #define BGRA_R 1
  #define BGRA_A 0
#else
  #define RGBA_R 0
  #define RGBA_G 1
  #define RGBA_B 2
  #define RGBA_A 3

  #define BGRA_B 0
  #define BGRA_G 1
  #define BGRA_R 2
  #define BGRA_A 3
#endif

  size_t steps = src_bytes_to_read / src_bytes_per_pixel_;
  for (size_t i = 0; i < steps; i++)
  {
    const unsigned char * src = src_addr + i * src_bytes_per_pixel_;
    unsigned char * dst = dst_addr + i * dst_bytes_per_pixel_;

    size_t src_offset = src - src_origin_;
    int y = src_offset / src_bytes_per_line_;
    int x = (src_offset % src_bytes_per_line_) / src_bytes_per_pixel_;

    QRgb argb = image_.pixel(x, y);
    switch (format_)
    {
      case GL_LUMINANCE:
      {
	int gray = qGray(argb);
	dst[0] = gray;
      }
      break;

      case GL_LUMINANCE_ALPHA:
      {
	int gray = qGray(argb);
	int alpha = qAlpha(argb);
	if (dst_bytes_per_pixel_ == 1)
	{
	  encode_la(gray, alpha, dst[0]);
	  // dst[0] = gray;
	}
	else
	{
	  dst[0] = gray;
	  dst[1] = alpha;
	}
      }
      break;

      case GL_RGB:
      {
	dst[RGBA_R] = (unsigned char)(qRed(argb));
	dst[RGBA_G] = (unsigned char)(qGreen(argb));
	dst[RGBA_B] = (unsigned char)(qBlue(argb));
      }
      break;

      case GL_RGBA:
      {
	dst[RGBA_R] = (unsigned char)(qRed(argb));
	dst[RGBA_G] = (unsigned char)(qGreen(argb));
	dst[RGBA_B] = (unsigned char)(qBlue(argb));
	dst[RGBA_A] = (unsigned char)(qAlpha(argb));
      }
      break;

      case GL_BGRA_EXT:
      {
	dst[BGRA_R] = (unsigned char)(qRed(argb));
	dst[BGRA_G] = (unsigned char)(qGreen(argb));
	dst[BGRA_B] = (unsigned char)(qBlue(argb));
	dst[BGRA_A] = (unsigned char)(qAlpha(argb));
      }
      break;

      default:
	assert(0);
    }
  }

#undef RGBA_R
#undef RGBA_G
#undef RGBA_B
#undef RGBA_A

#undef BGRA_R
#undef BGRA_G
#undef BGRA_B
#undef BGRA_A
}
