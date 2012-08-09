// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Feb 21 15:34:06 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PIXEL_FORMAT_TRAITS_H_
#define YAE_PIXEL_FORMAT_TRAITS_H_

// yae includes:
#include <yaeAPI.h>
#include <yaePixelFormats.h>


namespace yae
{

  namespace pixelFormat
  {
    //----------------------------------------------------------------
    // TFlags
    //
    enum TFlags
    {
      kLE = 1 << 0,      // little-endian
      kBE = 1 << 1,      // big-endian
      kAlpha = 1 << 2,   // has an alpha channel
      kYUV = 1 << 3,     // has color (YUV)
      kRGB = 1 << 4,     // has color (RGB)
      kPacked = 1 << 5,  // has a plane with interleaved channel samples
      kPlanar = 1 << 6,  // has a contiguous plane per channel samples
      kPaletted = 1 << 7 // requires a color palette
    };

    enum { kColor = (kYUV | kRGB) };

#ifdef __BIG_ENDIAN__
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
      unsigned char flags_;

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
      // number of least significant bits to right-shift
      // to get the channel sample value is not stored
      // since this can be derived from existing properties:
      //
      // rshift = stride - lshift - depth
      unsigned char rshift(unsigned char channel) const;

      // return number of contiguous sample planes, pass back
      // sample set stride (in bits) per sample plane:
      unsigned char getPlanes(unsigned char stride[4]) const;
    };

    //----------------------------------------------------------------
    // getTraits
    //
    YAE_API const Traits * getTraits(TPixelFormatId id);
  }
}


#endif // YAE_PIXEL_FORMAT_TRAITS_H_
