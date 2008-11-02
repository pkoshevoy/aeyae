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


// File         : la_pixel_converter.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Sep 24 16:54:00 MDT 2006
// Copyright    : (C) 2006
// License      : MIT
// Description  : luminance-alpha pixel converter -- given 2 separate
//                images (grayscale data image and a mask image) generate
//                one image that interleaves the grayscale and mask data.
//                The output pixel may be 2 bytes or 1 byte long, depending
//                on whether compression was used.
// 
#ifndef LA_PIXEL_CONVERTER_HXX_
#define LA_PIXEL_CONVERTER_HXX_

// the includes:
#include <image/image_tile_generator.hxx>


//----------------------------------------------------------------
// la_pixel_converter_t
//
template <class TImage, class TMask>
class la_pixel_converter_t : public pixel_converter_t
{
public:
  la_pixel_converter_t(const unsigned char * src_origin,
		       const unsigned char * msk_origin,
		       const bool compressed = false):
    src_origin_(src_origin),
    msk_origin_(msk_origin),
    src_bytes_per_pixel_(sizeof(typename TImage::PixelType)),
    msk_bytes_per_pixel_(sizeof(typename TMask::PixelType)),
    compressed_(compressed)
  {
    src_pixel_bytes_ = &src_pixel_;
    msk_pixel_bytes_ = &msk_pixel_;
  }
  
  inline size_t dst_bytes_per_pixel() const
  { return compressed_ ? 1 : 2; }
  
  // virtual:
  void operator() (unsigned char * dst_addr,
		   const unsigned char * src_addr,
		   const size_t & src_bytes_to_read) const
  {
    typedef typename TImage::PixelType ipix_t;
    typedef typename TMask::PixelType mpix_t;
    
    size_t dst_bytes_per_pixel = this->dst_bytes_per_pixel();
    
    size_t steps = src_bytes_to_read / src_bytes_per_pixel_;
    for (size_t i = 0; i < steps; i++)
    {
      unsigned char * dst = dst_addr + i * dst_bytes_per_pixel;
      const unsigned char * src = src_addr + i * src_bytes_per_pixel_;
      
      size_t offset = src - src_origin_;
      const unsigned char * msk = msk_origin_ + offset;
      
      // get the image and mask pixels:
      memcpy(src_pixel_bytes_, src, src_bytes_per_pixel_);
      memcpy(msk_pixel_bytes_, msk, msk_bytes_per_pixel_);
      
      // clamp the pixels into the valid range for a luminance alpha texture:
      src_pixel_ = ipix_t(std::min(255, std::max(0, int(src_pixel_))));
      msk_pixel_ = mpix_t(std::min(255, std::max(0, int(msk_pixel_))));
      
      // set the destination pixel:
      if (compressed_)
      {
	encode_la(src_pixel_, msk_pixel_, dst[0]);
      }
      else
      {
	dst[0] = (unsigned char)(src_pixel_);
	dst[1] = (unsigned char)(msk_pixel_);
      }
    }
  }
  
  // reference to the source image and image mask:
  const unsigned char * src_origin_;
  const unsigned char * msk_origin_;
  
  // basic image stats:
  const size_t src_bytes_per_pixel_;
  const size_t msk_bytes_per_pixel_;
  
  // these are used to convert the pixel data:
  mutable typename TImage::PixelType src_pixel_;
  mutable typename TMask::PixelType  msk_pixel_;
  unsigned char * src_pixel_bytes_;
  unsigned char * msk_pixel_bytes_;
  
  bool compressed_;
};


//----------------------------------------------------------------
// convert_tile
// 
template <typename TImage, typename TMask>
void
convert_tile(image_tile_generator_t & generator,
	     const TImage * tile,
	     const TMask * tile_mask,
	     const bool & compress)
{
  typename TImage::SizeType max_sz =
    tile->GetLargestPossibleRegion().GetSize();
  typename TImage::PointType origin = tile->GetOrigin();
  typename TImage::SpacingType spacing = tile->GetSpacing();
  generator.layout(max_sz[0],
		   max_sz[1],
		   origin[0],
		   origin[1],
		   spacing[0],
		   spacing[1]);
  
  const unsigned char * src = tile->GetBufferPointer();
  const unsigned char * msk = tile_mask->GetBufferPointer();
  
  la_pixel_converter_t<TImage, TMask> pixel_converter(src, msk, compress);
  size_t src_bytes_per_pixel = sizeof(typename TImage::PixelType);
  size_t dst_bytes_per_pixel = pixel_converter.dst_bytes_per_pixel();
  
  generator.
    convert_and_pad(src,			// source image buffer pointer
		    src_bytes_per_pixel,	// alignment
		    src_bytes_per_pixel,	// source image bytes per pixel
		    dst_bytes_per_pixel,	// destination bytes per pixel
		    pixel_converter);
}


#endif // LA_PIXEL_CONVERTER_HXX_
