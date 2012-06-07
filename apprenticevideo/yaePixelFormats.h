// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Feb 21 10:42:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PIXEL_FORMATS_H_
#define YAE_PIXEL_FORMATS_H_


namespace yae
{
  //----------------------------------------------------------------
  // TPixelFormatId
  // 
  enum TPixelFormatId
  {
    kInvalidPixelFormat = -1,

    //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    kPixelFormatYUV420P = 0,
    
    //! packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    kPixelFormatYUYV422 = 1,
    
    //! packed RGB 8:8:8, 24bpp, RGBRGB...
    kPixelFormatRGB24 = 2,
    
    //! packed RGB 8:8:8, 24bpp, BGRBGR...
    kPixelFormatBGR24 = 3,
    
    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    kPixelFormatYUV422P = 4,
    
    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    kPixelFormatYUV444P = 5,
    
    //! planar YUV 4:1:0, 9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    kPixelFormatYUV410P = 6,
    
    //! planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    kPixelFormatYUV411P = 7,
    
    //! Y, 8bpp
    kPixelFormatGRAY8 = 8,
    
    //! Y, 1bpp, 0 is white, 1 is black, in each byte pixels are
    //! ordered from the msb to the lsb
    kPixelFormatMONOWHITE = 9,
    
    //! Y, 1bpp, 0 is black, 1 is white, in each byte pixels are
    //! ordered from the msb to the lsb
    kPixelFormatMONOBLACK = 10,
    
    //! 8 bit with kPixelFormatRGB32 palette
    kPixelFormatPAL8 = 11,
    
    //! packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    kPixelFormatUYVY422 = 12,
    
    //! packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    kPixelFormatUYYVYY411 = 13,
    
    //! packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
    kPixelFormatBGR8 = 14,
    
    //! packed RGB 1:2:1 bitstream, 4bpp, (msb)1B 2G 1R(lsb), a byte
    //! contains two pixels, the first pixel in the byte is the one
    //! composed by the 4 msb bits
    kPixelFormatBGR4 = 15,
    
    //! packed RGB 1:2:1, 8bpp, (msb)1B 2G 1R(lsb)
    kPixelFormatBGR4_BYTE = 16,
    
    //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
    kPixelFormatRGB8 = 17,
    
    //! packed RGB 1:2:1 bitstream, 4bpp, (msb)1R 2G 1B(lsb), a byte
    //! contains two pixels, the first pixel in the byte is the one
    //! composed by the 4 msb bits
    kPixelFormatRGB4 = 18,
    
    //! packed RGB 1:2:1, 8bpp, (msb)1R 2G 1B(lsb)
    kPixelFormatRGB4_BYTE = 19,
    
    //! planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV
    //! components, which are interleaved (first byte U and the
    //! following byte V)
    kPixelFormatNV12 = 20,
    
    //! as above, but U and V bytes are swapped
    kPixelFormatNV21 = 21,
    
    //! packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    kPixelFormatARGB = 22,
    
    //! packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    kPixelFormatRGBA = 23,
    
    //! packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    kPixelFormatABGR = 24,
    
    //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    kPixelFormatBGRA = 25,
    
    //! Y, 16bpp, big-endian
    kPixelFormatGRAY16BE = 26,
    
    //! Y, 16bpp, little-endian
    kPixelFormatGRAY16LE = 27,
    
    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples)
    kPixelFormatYUV440P = 28,
    
    //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
    //! samples)
    kPixelFormatYUVA420P = 29,
    
    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as big-endian
    kPixelFormatRGB48BE = 30,
    
    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as little-endian
    kPixelFormatRGB48LE = 31,
    
    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
    kPixelFormatRGB565BE = 32,
    
    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
    kPixelFormatRGB565LE = 33,
    
    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
    //! most significant bit to 0
    kPixelFormatRGB555BE = 34,
    
    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
    //! most significant bit to 0
    kPixelFormatRGB555LE = 35,
    
    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
    kPixelFormatBGR565BE = 36,
    
    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
    kPixelFormatBGR565LE = 37,
    
    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
    //! most significant bit to 1
    kPixelFormatBGR555BE = 38,
    
    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
    //! most significant bit to 1
    kPixelFormatBGR555LE = 39,
    
    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! little-endian
    kPixelFormatYUV420P16LE = 40,
    
    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! big-endian
    kPixelFormatYUV420P16BE = 41,
    
    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! little-endian
    kPixelFormatYUV422P16LE = 42,
    
    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! big-endian
    kPixelFormatYUV422P16BE = 43,
    
    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! little-endian
    kPixelFormatYUV444P16LE = 44,
    
    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! big-endian
    kPixelFormatYUV444P16BE = 45,
    
    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
    //! most significant bits to 0
    kPixelFormatRGB444BE = 46,
    
    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
    //! most significant bits to 0
    kPixelFormatRGB444LE = 47,
    
    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
    //! most significant bits to 1
    kPixelFormatBGR444BE = 48,
    
    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
    //! most significant bits to 1
    kPixelFormatBGR444LE = 49,
    
    //! 8bit gray, 8bit alpha
    kPixelFormatY400A = 50,
    
    //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples), JPEG
    kPixelFormatYUVJ420P = 51,
    
    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    kPixelFormatYUVJ422P = 52,
    
    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    kPixelFormatYUVJ444P = 53,
    
    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples), JPEG
    kPixelFormatYUVJ440P = 54,

    //! planar YUV, 9-bits per channel:
    kPixelFormatYUV420P9 = 55,
    kPixelFormatYUV422P9 = 56,
    kPixelFormatYUV444P9 = 57,

    //! planar YUV, 10-bits per channel:
    kPixelFormatYUV420P10 = 58,
    kPixelFormatYUV422P10 = 59,
    kPixelFormatYUV444P10 = 60,
  };
}


#endif // YAE_PIXEL_FORMATS_H_
