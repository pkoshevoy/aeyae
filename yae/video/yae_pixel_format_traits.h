// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Feb 21 15:34:06 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PIXEL_FORMAT_TRAITS_H_
#define YAE_PIXEL_FORMAT_TRAITS_H_

// aeyae:
#include "yae_video.h"
#include "yae_pixel_formats.h"


namespace yae
{

  namespace pixelFormat
  {
    //----------------------------------------------------------------
    // TFlags
    //
    enum TFlags
    {
      kLE = 1 << 0,        // little-endian
      kBE = 1 << 1,        // big-endian
      kAlpha = 1 << 2,     // has an alpha channel
      kYUV = 1 << 3,       // has color (YUV)
      kRGB = 1 << 4,       // has color (RGB)
      kXYZ = 1 << 5,       // has color (XYZ)
      kBayer = 1 << 6,     // has color (RGB), in 2x2 Bayer configuration
      kPacked = 1 << 7,    // has a plane with interleaved channel samples
      kPlanar = 1 << 8,    // has a contiguous plane per channel samples
      kPaletted = 1 << 9,  // requires a color palette
      kFloat = 1 << 10,    // IEEE-754 single precision float samples
    };

    enum { kColor = (kYUV | kRGB | kXYZ | kBayer) };

#if defined(__BIG_ENDIAN__) && __BIG_ENDIAN__
    enum { kNativeEndian = kBE };
#else
    enum { kNativeEndian = kLE };
#endif

    //----------------------------------------------------------------
    // Traits
    //
    struct YAE_API Traits
    {
      // format label:
      const char * name_;

      // additional info (big endian, little endian, etc...)
      unsigned int flags_;

      // number of pixel component channels:
      // 1 -- gray, alpha, or paletted
      // 2 -- gray and alpha
      // 3 -- color, YUV or RGB
      // 4 -- color and alpha, YUV or RGB
      unsigned char channels_;

      // horizontal chroma subsampling bounding box width,
      // specifies number of Y samples per UV sample,
      // set to 0 if there is no color:
      unsigned char chromaBoxW_;

      // horizontal chroma subsampling region height,
      // specifies number of Y samples per UV sample,
      // set to 0 if there is no color:
      unsigned char chromaBoxH_;

      // index of contiguous sample plane each channel belongs to:
      unsigned char plane_[4];

      // sample bit depth per channel:
      unsigned char depth_[4];

      // sample bit offset (left-shift bits) per channel:
      unsigned char lshift_[4];

      // bits to skip to reach next set of samples per channel:
      unsigned char stride_[4];

      // number of samples in the set per channel,
      // necessary for complex formats like UYYVYY411:
      unsigned char samples_[4];

      // NOTE:
      //
      //  * Bayer sample layout matrix (2 x 2)
      //  is stored in row-major order
      //
      //  * Bayer pixel format chroma box is always 2 x 2
      //    as is the Bayer sample layout matrix
      //
      //  * samples_ are always 1, 2, 1
      //    because Green is always represented twice
      //
      //  * lshift_, plane_, depth_, and stride_ correspond to the
      //    color component specified in the bayer_ matrix
      //
      //
      // Example of Bayer sample layout, BGGR:
      //
      //   even lines:  G R    1 0
      //    odd lines:  B G    2 1
      //
      //   bayer_ == [ 1, 0, 2, 1 ]
      //
      unsigned char bayer_[4];

      unsigned char datatype_bits_[4];
      unsigned char datatype_rpad_[4];
      unsigned char datatype_lpad_[4];

      // NOTE:
      // number of least significant bits to right-shift
      // to get the channel sample value is not stored
      // since this can be derived from existing properties:
      //
      // rshift = stride - lshift - depth
      unsigned char rshift(unsigned char channel) const;

      // return number of contiguous sample planes, pass back
      // sample set stride (in bits) per sample plane:
      unsigned char getPlanes(unsigned char stride[4]) const;

      inline bool is_packed() const
      { return ((flags_ & kPacked) == kPacked); }

      inline bool is_planar() const
      { return ((flags_ & kPlanar) == kPlanar); }
    };

    //----------------------------------------------------------------
    // getTraits
    //
    YAE_API const Traits * getTraits(TPixelFormatId id);
  }


  //----------------------------------------------------------------
  // pixelIntensity
  //
  // if pixel format is YUV -- return Y channel pixel value
  // if pixel format is RGB -- return RGB average pixel value
  //
  YAE_API double pixelIntensity(const int x,
                                const int y,
                                const unsigned char * data,
                                const std::size_t rowBytes,
                                const pixelFormat::Traits & ptts);

}


#endif // YAE_PIXEL_FORMAT_TRAITS_H_
