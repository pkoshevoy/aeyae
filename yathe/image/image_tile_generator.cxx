// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : image_tile_generator.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : a helper class for splitting an image into a set
//                of tiles that may be used as OpenGL textures.

// local includes:
#include "image/image_tile_generator.hxx"
#include "image/image_tile.hxx"
#include "image/texture_data.hxx"
#include "image/texture.hxx"
#include "thread/the_terminator.hxx"
#include "utils/the_exception.hxx"
#include "utils/the_text.hxx"
#include "utils/the_utils.hxx"

// system includes:
#include <math.h>


//----------------------------------------------------------------
// pot_less_than_or_equal
//
static size_t
pot_less_than_or_equal(size_t in)
{
  static const unsigned char bits = sizeof(size_t) << 3;

  size_t prev = 0;
  size_t curr = 1;

  for (unsigned char i = 1; i < bits && curr < in; i++)
  {
    prev = curr;
    curr = curr << 1;
  }

  return (curr > in) ? prev : curr;
}


//----------------------------------------------------------------
// image_tile_generator_t::image_tile_generator_t
//
image_tile_generator_t::image_tile_generator_t():
  w_(0),
  h_(0),
  w_pad_(0),
  h_pad_(0),
  w_odd_(0),
  h_odd_(0),
  origin_x_(0),
  origin_y_(0),
  spacing_x_(1),
  spacing_y_(1),
  bytes_per_pixel_(0)
{
  // cerr << this << " -- created tile generator" << endl;
}

//----------------------------------------------------------------
// image_tile_generator_t::~image_tile_generator_t
//
image_tile_generator_t::~image_tile_generator_t()
{
  // cerr << this << " -- deleted tile generator" << endl;
}

//----------------------------------------------------------------
// image_tile_generator_t::layout
//
// calculate the image padding and generate the tile layout
//
void
image_tile_generator_t::layout(const size_t w,
			       const size_t h,
			       const double origin_x,
			       const double origin_y,
			       const double spacing_x,
			       const double spacing_y)
{
  // unpadded image dimensions:
  w_ = w;
  h_ = h;

  // additional padding due to odd image dimensions:
  w_odd_ = (size_t)(w_ & 0x01);
  h_odd_ = (size_t)(h_ & 0x01);

  // padded image dimensions:
  w_pad_ = w_ + 2 + w_odd_;
  h_pad_ = h_ + 2 + h_odd_;

  // update the origin and pixel spacing:
  origin_x_ = origin_x;
  origin_y_ = origin_y;
  spacing_x_ = spacing_x;
  spacing_y_ = spacing_y;

  // update the bounding box:
  bbox_.clear();
  bbox_ << p3x1_t(float(origin_x_),
		  float(origin_y_),
		  0)
	<< p3x1_t(float(origin_x_ + spacing_x_ * double(w_)),
		  float(origin_y_ + spacing_y_ * double(h_)),
		  0);
}

//----------------------------------------------------------------
// image_tile_generator_t::allocate
//
void
image_tile_generator_t::allocate(const unsigned int & bytes_per_pixel)
{
  double bytes = double(w_pad_) * double(h_pad_) * double(bytes_per_pixel);
  size_t bytes_int = w_pad_ * h_pad_ * bytes_per_pixel;

  if (bytes > double(bytes_int))
  {
    static const double _1MB = 1024.0 * 1024.0;
    double size_MB = bytes / _1MB;
    std::ostringstream os;
    os << "integer overflow detected when calculating image buffer size: "
       << size_MB << "MB" << endl;
    the_exception_t e(os.str().c_str());
    throw e;
  }

  // allocate the new buffer -- may throw an exception:
  // cerr << "FIXME: allocating " << bytes_int << endl;

  boost::shared_ptr<texture_data_t> buffer(new texture_data_t(bytes_int));
  buffer_ = buffer;
  bytes_per_pixel_ = bytes_per_pixel;

  // fill the new buffer with zeros:
  memset(buffer_->data(), 0x00, bytes_int);
}

//----------------------------------------------------------------
// image_tile_generator_t::pad
//
void
image_tile_generator_t::pad()
{
  the_terminator_t terminator("image_tile_generator_t::pad");

  // left, right padding offsets:
  const size_t src_offset_0 = bytes_per_pixel_;
  const size_t dst_offset_0 = 0;
  const size_t src_offset_1 = bytes_per_pixel_ * w_;
  const size_t dst_offset_1 = bytes_per_pixel_ * (w_ + 1);
  const size_t bytes_to_copy_per_line = bytes_per_pixel_ * w_;
  const size_t bytes_per_line = bytes_per_pixel_ * w_pad_;

  unsigned char * dst = buffer_->data();
  const unsigned char * src = dst;

  // pad on the top:
  {
    size_t src_line_addr = bytes_per_line;
    size_t dst_line_addr = 0;

    // left:
    memcpy(dst + dst_line_addr + dst_offset_0,
	   src + src_line_addr + src_offset_0,
	   bytes_per_pixel_);

    // center:
    memcpy(dst + dst_line_addr + dst_offset_0 + bytes_per_pixel_,
	   src + src_line_addr + src_offset_0,
	   bytes_to_copy_per_line);

    // right:
    memcpy(dst + dst_line_addr + dst_offset_1,
	   src + src_line_addr + src_offset_1,
	   bytes_per_pixel_);
  }

  // take care of the center of the image:
  for (size_t j = 0; j < h_; j++)
  {
    terminator.terminate_on_request();
    size_t line_addr = bytes_per_line * (j + 1);

    // pad on the left:
    memcpy(dst + line_addr + dst_offset_0,
	   src + line_addr + src_offset_0,
	   bytes_per_pixel_);

    // pad on the right:
    memcpy(dst + line_addr + dst_offset_1,
	   src + line_addr + src_offset_1,
	   bytes_per_pixel_);
  }

  // pad on the bottom:
  {
    size_t src_line_addr = bytes_per_line * h_;
    size_t dst_line_addr = bytes_per_line * (h_ + 1);

    // pad on the left:
    memcpy(dst + dst_line_addr + dst_offset_0,
	   src + src_line_addr + src_offset_0,
	   bytes_per_pixel_);

    // copy the center:
    memcpy(dst + dst_line_addr + dst_offset_0 + bytes_per_pixel_,
	   src + src_line_addr + src_offset_0,
	   bytes_to_copy_per_line);

    // pad on the right:
    memcpy(dst + dst_line_addr + dst_offset_1,
	   src + src_line_addr + src_offset_1,
	   bytes_per_pixel_);
  }
}

//----------------------------------------------------------------
// image_tile_generator_t::convert_and_pad
//
// generate a padded image, each pixel will be converted:
void
image_tile_generator_t::
convert_and_pad(const unsigned char * src,
		const unsigned int & src_alignment,
		const unsigned int & src_bytes_per_pixel,
		const unsigned int & dst_bytes_per_pixel,
		const pixel_converter_t & convert)
{
  the_terminator_t terminator("image_tile_generator_t::convert_and_pad");

  const size_t src_padding_bytes =
    (src_alignment - (src_bytes_per_pixel * w_) % src_alignment) %
    src_alignment;

  const size_t src_bytes_per_line =
    src_bytes_per_pixel * w_ + src_padding_bytes;

  const size_t dst_bytes_per_line =
    dst_bytes_per_pixel * w_pad_;

  allocate(dst_bytes_per_pixel);
  unsigned char * dst = buffer_->data();

  // left, right padding offsets:
  const size_t src_offset_0 = 0;
  const size_t dst_offset_0 = 0;
  const size_t src_offset_1 = src_bytes_per_pixel * (w_ - 1);
  const size_t dst_offset_1 = dst_bytes_per_pixel * (w_ + 1);
  const size_t src_bytes_to_copy_per_line = src_bytes_per_pixel * w_;

  // pad on the top:
  {
    size_t src_line_addr = 0;
    size_t dst_line_addr = 0;

    // left:
    convert(dst + dst_line_addr + dst_offset_0,
	    src + src_line_addr + src_offset_0,
	    src_bytes_per_pixel);

    // center:
    convert(dst + dst_line_addr + dst_offset_0 + dst_bytes_per_pixel,
	    src + src_line_addr + src_offset_0,
	    src_bytes_to_copy_per_line);

    // right:
    convert(dst + dst_line_addr + dst_offset_1,
	    src + src_line_addr + src_offset_1,
	    src_bytes_per_pixel);
  }

  // take care of the center of the image:
  for (size_t j = 0; j < h_; j++)
  {
    terminator.terminate_on_request();

    size_t src_line_addr = src_bytes_per_line * j;
    size_t dst_line_addr = dst_bytes_per_line * (j + 1);

    // pad on the left:
    convert(dst + dst_line_addr + dst_offset_0,
	    src + src_line_addr + src_offset_0,
	    src_bytes_per_pixel);

    // copy the center
    convert(dst + dst_line_addr + dst_offset_0 + dst_bytes_per_pixel,
	    src + src_line_addr + src_offset_0,
	    src_bytes_to_copy_per_line);

    // pad on the right:
    convert(dst + dst_line_addr + dst_offset_1,
	    src + src_line_addr + src_offset_1,
	    src_bytes_per_pixel);
  }

  // pad on the bottom:
  {
    size_t src_line_addr = src_bytes_per_line * (h_ - 1);
    size_t dst_line_addr = dst_bytes_per_line * (h_ + 1);

    // pad on the left:
    convert(dst + dst_line_addr + dst_offset_0,
	    src + src_line_addr + src_offset_0,
	    src_bytes_per_pixel);

    // copy the center:
    convert(dst + dst_line_addr + dst_offset_0 + dst_bytes_per_pixel,
	    src + src_line_addr + src_offset_0,
	    src_bytes_to_copy_per_line);

    // pad on the right:
    convert(dst + dst_line_addr + dst_offset_1,
	    src + src_line_addr + src_offset_1,
	    src_bytes_per_pixel);
  }
}

//----------------------------------------------------------------
// image_tile_generator_t::make_tiles
//
void
image_tile_generator_t::make_tiles(const GLenum & data_type,
				   const GLenum & format_internal,
				   const GLenum & format,
				   const size_t max_texture,
				   double min_x,
				   double min_y,
				   double max_x,
				   double max_y)
{
  // sanitize the parameters to be within legal bounds:
  double image_max_x = origin_x_ + spacing_x_ * double(w_);
  double image_max_y = origin_y_ + spacing_y_ * double(h_);

#if 0
  cerr << endl
       << "image_max_x: " << image_max_x << endl
       << "image_max_y: " << image_max_y << endl
       << "min_x: " << min_x << endl
       << "max_x: " << max_x << endl
       << "min_y: " << min_y << endl
       << "max_y: " << max_y << endl;
#endif

  min_x = std::max(origin_x_, std::min(image_max_x, min_x));
  max_x = std::max(origin_x_, std::min(image_max_x, max_x));
  min_y = std::max(origin_y_, std::min(image_max_y, min_y));
  max_y = std::max(origin_y_, std::min(image_max_y, max_y));

  size_t w = std::min(w_, (size_t)(ceil((max_x - min_x) / spacing_x_)));
  size_t h = std::min(h_, (size_t)(ceil((max_y - min_y) / spacing_y_)));

  size_t w_odd = (size_t)(w & 0x01);
  size_t h_odd = (size_t)(h & 0x01);

  size_t w_pad = w + 2 + w_odd;
  size_t h_pad = h + 2 + h_odd;

  size_t x_offset = int(floor((min_x - origin_x_) / spacing_x_));
  size_t y_offset = int(floor((min_y - origin_y_) / spacing_y_));

  int x_shift = std::max(0, int(x_offset + w_pad - w_pad_));
  int y_shift = std::max(0, int(y_offset + h_pad - h_pad_));

  if (x_shift > 1 || y_shift > 1)
  {
#if 0
  cerr << endl
       << "origin_x_: " << origin_x_ << endl
       << "origin_y_: " << origin_y_ << endl
       << "spacing_x_: " << spacing_x_ << endl
       << "spacing_y_: " << spacing_y_ << endl
       << "w_: " << w_ << endl
       << "h_: " << h_ << endl
       << "w_pad_: " << w_pad_ << endl
       << "h_pad_: " << h_pad_ << endl
       << "w_odd_: " << w_odd_ << endl
       << "h_odd_: " << h_odd_ << endl
       << "image_max_x: " << image_max_x << endl
       << "image_max_y: " << image_max_y << endl
       << "min_x: " << min_x << endl
       << "min_y: " << min_y << endl
       << "max_x: " << max_x << endl
       << "max_y: " << max_y << endl
       << "w: " << w << endl
       << "h: " << h << endl
       << "w_odd: " << w_odd << endl
       << "h_odd: " << h_odd << endl
       << "w_pad: " << w_pad << endl
       << "h_pad: " << h_pad << endl
       << "x_offset: " << x_offset << endl
       << "y_offset: " << y_offset << endl
       << "x_shift: " << x_shift << endl
       << "y_shift: " << y_shift << endl;
#endif
  }

  assert(x_shift <= 1);
  assert(y_shift <= 1);

  x_offset -= x_shift;
  y_offset -= y_shift;

  // calculate x_min, x_max image coordinates for each tile:
  the_dynamic_array_t<the_duplet_t<size_t> > x;
  if (min_x <= max_x)
  {
    size_t w0 = w_pad;
    while (true)
    {
      the_duplet_t<size_t> xw;
      xw[0] = x_offset;
      xw[1] = std::min(max_texture, pot_less_than_or_equal(w0));
      x.append(xw);

      if (xw[1] == w0) break;
      x_offset += (xw[1] - 2);
      w0 -= (xw[1] - 2);
    }
  }

  // calculate y_min, y_max image coordinates for each tile:
  the_dynamic_array_t<the_duplet_t<size_t> > y;
  if (min_y <= max_y)
  {
    size_t h0 = h_pad;
    while (true)
    {
      the_duplet_t<size_t> yh;
      yh[0] = y_offset;
      yh[1] = std::min(max_texture, pot_less_than_or_equal(h0));
      y.append(yh);

      if (yh[1] == h0) break;
      y_offset += (yh[1] - 2);
      h0 -= (yh[1] - 2);
    }
  }

  // shortcut:
  const unsigned char * data = buffer_->data();

  // setup the tiles:
  const size_t rows = y.size();
  const size_t cols = x.size();
  tiles_.resize(rows * cols);
  /*
  cerr << endl
       << "rows: " << rows << endl
       << "cols: " << cols << endl;
  */

  int alignment = 1;
  switch (bytes_per_pixel_)
  {
    case 2:
    case 4:
    case 8:
      alignment = bytes_per_pixel_;
      break;

    default:
      break;
  }

  int scale = 1;
  if (format_internal == GL_LUMINANCE && bytes_per_pixel_ != 1)
  {
    scale = bytes_per_pixel_;
    alignment = 1;
  }

  for (size_t j = 0; j < rows; j++)
  {
    for (size_t i = 0; i < cols; i++)
    {
      image_tile_t & tile = tiles_[j * cols + i];

      double x0 = origin_x_ + spacing_x_ * double(x[i][0]);
      double y0 = origin_y_ + spacing_y_ * double(y[j][0]);
      double x1 = x0 + spacing_x_ * double(x[i][1] - 2);
      double y1 = y0 + spacing_y_ * double(y[j][1] - 2);

      tile.s0_ = GLfloat(double(1) / double(x[i][1]));
      tile.s1_ = GLfloat(1.0 - double(1) / double(x[i][1]));
      tile.t0_ = GLfloat(1.0 / double(y[j][1]));
      tile.t1_ = GLfloat(1.0 - 1.0 / double(y[j][1]));

      if (x_shift == 0 && i + 1 == cols)
      {
	x1 -= spacing_x_ * double(w_odd);
	tile.s1_ = GLfloat(1.0 - double(1 * (1 + w_odd)) / double(x[i][1]));
      }
      else if (x_shift > 0 && i == 0)
      {
	x0 += spacing_x_ * double(x_shift);
	tile.s0_ = GLfloat(double(1 * (1 + x_shift)) / double(x[i][1]));
      }

      if (y_shift == 0 && j + 1 == rows)
      {
	y1 -= spacing_y_ * double(h_odd);
	tile.t1_ = GLfloat(1.0 - double(1 + h_odd) / double(y[j][1]));
      }
      else if (y_shift > 0 && j == 0)
      {
	y0 += spacing_y_ * double(y_shift);
	tile.t0_ = GLfloat(double(1 + y_shift) / double(y[j][1]));
      }

      tile.corner_[0].assign(float(x0), float(y0), 0);
      tile.corner_[1].assign(float(x1), float(y0), 0);
      tile.corner_[2].assign(float(x1), float(y1), 0);
      tile.corner_[3].assign(float(x0), float(y1), 0);

      typedef boost::shared_ptr<const_texture_data_t> data_ptr_t;
      data_ptr_t texture_data(new const_texture_data_t(data));

      tile.texture_ =
	boost::shared_ptr<texture_base_t>
	(new texture_t<data_ptr_t>(texture_data,
				   data_type,		// OpenGL data type
				   format_internal,	// OpenGL internal fmt
				   format,		// external data fmt
				   scale *
				   (GLsizei)(x[i][1]),	// width
				   (GLsizei)(y[j][1]),	// height
				   0,			// border
				   alignment,		// alignment
				   (GLint)
				   (scale * w_pad_),	// row_length
				   scale *
				   (GLint)(x[i][0]),	// skip_pixels
				   (GLint)(y[j][0])));	// skip_rows
    }
  }
}

//----------------------------------------------------------------
// image_tile_generator_t::set_origin
//
void
image_tile_generator_t::set_origin(double ox, double oy)
{
  double dx = ox - origin_x_;
  double dy = oy - origin_y_;
  if (dx == 0.0 && dy == 0.0) return;

  layout(w_, h_, ox, oy, spacing_x_, spacing_y_);
  size_t num_tiles = tiles_.size();
  for (size_t i = 0; i < num_tiles; i++)
  {
    image_tile_t & tile = tiles_[i];
    for (size_t j = 0; j < 4; j++)
    {
      tile.corner_[j][0] += float(dx);
      tile.corner_[j][1] += float(dy);
    }
  }
}

//----------------------------------------------------------------
// image_tile_generator_t::flip
//
void
image_tile_generator_t::flip()
{
  the_terminator_t terminator("image_tile_generator_t::flip");

  size_t tmp_h = (h_ + 2);
  size_t tmp_w = (w_ + 2) / 2;

  size_t bytes_per_line = w_pad_ * bytes_per_pixel_;
  size_t addr_0 = 0;
  size_t addr_1 = (w_ + 2) * bytes_per_pixel_;

  texture_data_t pixel(bytes_per_pixel_);
  unsigned char * pixel_bytes = pixel.data();
  unsigned char * data = buffer_->data();

  for (size_t j = 0; j < tmp_h; j++)
  {
    terminator.terminate_on_request();
    size_t line_addr = bytes_per_line * j;

    for (size_t i = 0; i < tmp_w; i++)
    {
      size_t shift = i * bytes_per_pixel_;
      memcpy(pixel_bytes,
	     data + line_addr + addr_0 + shift,
	     bytes_per_pixel_);
      memcpy(data + line_addr + addr_0 + shift,
	     data + line_addr + addr_1 - shift,
	     bytes_per_pixel_);
      memcpy(data + line_addr + addr_1 - shift,
	     pixel_bytes,
	     bytes_per_pixel_);
    }
  }
}

//----------------------------------------------------------------
// image_tile_generator_t::evaluate
//
bool
image_tile_generator_t::evaluate(double x,
				 double y,
				 const unsigned char * pixel[4],
				 double weight[4]) const
{
  int ix;
  int iy;
  double u1;
  double v1;
  if (!get_pixel_coords(x, y, ix, iy, u1, v1))
  {
    return false;
  }

  double u0 = 1.0 - u1;
  double v0 = 1.0 - v1;

  pixel[0] = scanline(iy) + int(bytes_per_pixel_) * ix;
  pixel[1] = scanline(iy) + int(bytes_per_pixel_) * (ix + 1);
  pixel[2] = scanline(iy + 1) + int(bytes_per_pixel_) * ix;
  pixel[3] = scanline(iy + 1) + int(bytes_per_pixel_) * (ix + 1);

#ifndef NDEBUG
  size_t data = size_t(buffer_->data());
  size_t size = buffer_->size();
  size_t last = data + size;
  size_t addr = size_t(pixel[3] + bytes_per_pixel_);
  assert(addr <= last);
  assert(size_t(pixel[0]) >= data);
#endif

#if 0
  weight[0] = v0 * u0;
  weight[1] = v0 * u1;
  weight[2] = v1 * u0;
  weight[3] = v1 * u1;
#else
  weight[0] = (ix > -1 && iy > -1) ? v0 * u0 : 0.0;
  weight[1] = (ix + 1 < int(w_) && iy > -1) ? v0 * u1 : 0.0;
  weight[2] = (ix > -1 && iy + 1 < int(h_)) ? v1 * u0 : 0.0;
  weight[3] = (ix + 1 < int(w_) && iy + 1 < int(h_)) ? v1 * u1 : 0.0;
#endif

  return true;
}

//----------------------------------------------------------------
// interpolate_luminance
//
// NOTE: this function assumes that luminance is
// passed in already initialized to 0, and that each of the
// four pixels consists of 1 byte -- luminance:
//
void
interpolate_luminance(double & luminance,
		      const unsigned char * pixel[4],
		      const double weight[4])
{
  luminance += weight[0] * double(pixel[0][0]);
  luminance += weight[1] * double(pixel[1][0]);
  luminance += weight[2] * double(pixel[2][0]);
  luminance += weight[3] * double(pixel[3][0]);
}

//----------------------------------------------------------------
// interpolate_luminance_alpha
//
// NOTE: this function assumes that luminance and alpha are
// passed in already initialized to 0, and that each of the
// four pixels consists of 2 bytes -- luminance and alpha:
//
void
interpolate_luminance_alpha(double & luminance,
			    double & alpha,
			    const unsigned char * pixel[4],
			    const double weight[4])
{
  luminance += weight[0] * double(pixel[0][0]);
  alpha += weight[0] * double(pixel[0][1]);

  luminance += weight[1] * double(pixel[1][0]);
  alpha += weight[1] * double(pixel[1][1]);

  luminance += weight[2] * double(pixel[2][0]);
  alpha += weight[2] * double(pixel[2][1]);

  luminance += weight[3] * double(pixel[3][0]);
  alpha += weight[3] * double(pixel[3][1]);
}


//----------------------------------------------------------------
// encode_la_v1
//
void
encode_la_v1(const unsigned char & l,
	     const unsigned char & a,
	     unsigned char & la)
{
  if (a != 0xFF)
  {
    unsigned char a_bin =
      (unsigned char)(floorf(float(a) * 15.0f / 255.0f));

    unsigned char l_bin =
      (unsigned char)(floorf(float(l) * (a_bin + 1) / 256.0f));

    la = (a_bin * (a_bin + 1)) / 2 + l_bin;
  }
  else
  {
    la = 120 + (unsigned char)(135.0f * float(l) / 255.0f);
  }
}

//----------------------------------------------------------------
// decode_la_v1
//
void
decode_la_v1(const unsigned char & la,
	     unsigned char & l,
	     unsigned char & a)
{
  if (la < 120)
  {
    // decode the alpha/luminance indices:
    unsigned char a_bin =
      (unsigned char)(std::min(14.0f,
			       floorf((sqrtf(1 + 8 * float(la)) - 1) / 2)));

    unsigned char offset = (a_bin * (a_bin + 1)) / 2;
    unsigned char l_bin = la - offset;

    l = (unsigned char)(256.0f * (float(l_bin) + 0.5) / float(a_bin + 1));
    a = (unsigned char)(255.0f * float(a_bin) / 15.0f);
  }
  else
  {
    l = (unsigned char)(255.0f * float(la - 120) / 135.0f);
    a = 255;
  }
}

//----------------------------------------------------------------
// encode_la_v2
//
void
encode_la_v2(const unsigned char & l,
	     const unsigned char & a,
	     unsigned char & la)
{
  if (a < 127)
  {
    la = 0;
  }
  else
  {
    la = 1 + (unsigned char)(254.0f * float(l) / 255.0f);
  }
}

//----------------------------------------------------------------
// decode_la_v2
//
void
decode_la_v2(const unsigned char & la,
	     unsigned char & l,
	     unsigned char & a)
{
  if (la < 1)
  {
    l = 0;
    a = 0;
  }
  else
  {
    l = (unsigned char)(255.0f * float(la - 1) / 254.0f);
    a = 255;
  }
}

//----------------------------------------------------------------
// encode_la
//
encode_la_fn_t encode_la = encode_la_v1;

//----------------------------------------------------------------
// decode_la
//
decode_la_fn_t decode_la = decode_la_v1;

//----------------------------------------------------------------
// interpolate_compressed_luminance_alpha
//
void
interpolate_compressed_luminance_alpha(double & luminance,
				       double & alpha,
				       const unsigned char * pixel[4],
				       const double weight[4])
{
  // temporaries:
  unsigned char l;
  unsigned char a;

  decode_la(pixel[0][0], l, a);
  luminance += weight[0] * double(l);
  alpha += weight[0] * double(a);

  decode_la(pixel[1][0], l, a);
  luminance += weight[1] * double(l);
  alpha += weight[1] * double(a);

  decode_la(pixel[2][0], l, a);
  luminance += weight[2] * double(l);
  alpha += weight[2] * double(a);

  decode_la(pixel[3][0], l, a);
  luminance += weight[3] * double(l);
  alpha += weight[3] * double(a);

  // get rid of the roundoff error:
  alpha = floor(alpha + 0.5);
}

//----------------------------------------------------------------
// encode_rgba16
//
void
encode_rgba16(const float * rgba, unsigned char * rgba16)
{
  unsigned char r = (unsigned char)(rgba[0] / 255.0f * 31.0f);
  unsigned char g = (unsigned char)(rgba[1] / 255.0f * 31.0f);
  unsigned char b = (unsigned char)(rgba[2] / 255.0f * 31.0f);
  unsigned char a = (rgba[3] < 128.0) ? 0 : 1;

  // shortcuts:
  unsigned char & b0 = rgba16[0];
  unsigned char & b1 = rgba16[1];

#if 1
  // encode R5-G3-g2-B5-A1
  b0 = (r << 3) | (g >> 2);
  b1 = ((g & 3) << 6) | (b << 1) | a;
#elif 0
  // encode R5-G3-B5-A1-g2
  b0 = (r << 3) | (g >> 2);
  b1 = (b << 3) | (a << 2) | (g & 3);
#else
  // encode A1-R5-g2-G3-B5
  b0 = (a << 7) | (r << 2) | (g & 3);
  b1 = ((g & 28) << 3) | b;
#endif
}

//----------------------------------------------------------------
// decode_rgba16
//
void
decode_rgba16(const unsigned char * rgba16, float * rgba)
{
  // shortcuts:
  const unsigned char & b0 = rgba16[0];
  const unsigned char & b1 = rgba16[1];

#if 1
  // decode R5-G3-g2-B5-A1
  /*
  unsigned char r = b0 / 8;
  unsigned char g0 = (b0 % 8);
  unsigned char g1 = b1 / 64;
  unsigned char b = (b1 / 2) % 32;
  unsigned char a = (b1 % 2);
  unsigned char g = g0 * 4 + g1;
  */
  unsigned char r  = b0 >> 3;
  unsigned char g0 = b0 & 7;
  unsigned char g1 = b1 >> 6;
  unsigned char g  = (g0 << 2) | g1;
  unsigned char b  = (b1 >> 1) & 31;
  unsigned char a  = b1 & 1;
#elif 0
  // decode R5-G3-B5-A1-g2
  unsigned char r = b0 / 8;
  unsigned char g0 = b0 % 8;
  unsigned char b = b1 / 8;
  unsigned char a = (b1 % 8) / 4;
  unsigned char g1 = b1 % 4;
  unsigned char g = g0 * 4 + g1;
#else
  // decode A1-R5-g2-G3-B5
  unsigned char a = b0 / 128;
  unsigned char r = (b0 % 128) / 4;
  unsigned char g1 = b0 % 4;
  unsigned char g0 = b1 / 32;
  unsigned char b = b1 % 32;
  unsigned char g = g0 * 4 + g1;
#endif

  static const float scale = 255.0f / 31.f;
  rgba[0] = floor(0.5f + float(r) * scale);
  rgba[1] = floor(0.5f + float(g) * scale);
  rgba[2] = floor(0.5f + float(b) * scale);
  rgba[3] = 255.0f * float(a);
}
