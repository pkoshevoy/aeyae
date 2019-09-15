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

    //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
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

    //! planar YUV, 9 bits per channel:
    kPixelFormatYUV420P9BE = 55,
    kPixelFormatYUV420P9LE = 56,
    kPixelFormatYUV422P9BE = 57,
    kPixelFormatYUV422P9LE = 58,
    kPixelFormatYUV444P9BE = 59,
    kPixelFormatYUV444P9LE = 60,

    //! planar YUV, 10 bits per channel:
    kPixelFormatYUV420P10BE = 61,
    kPixelFormatYUV420P10LE = 62,
    kPixelFormatYUV422P10BE = 63,
    kPixelFormatYUV422P10LE = 64,
    kPixelFormatYUV444P10BE = 65,
    kPixelFormatYUV444P10LE = 66,

    kPixelFormatRGBA64BE = 67,
    kPixelFormatRGBA64LE = 68,
    kPixelFormatBGRA64BE = 69,
    kPixelFormatBGRA64LE = 70,

    kPixelFormatGBRP = 71,

    kPixelFormatGBRP9BE = 72,
    kPixelFormatGBRP9LE = 73,
    kPixelFormatGBRP10BE = 74,
    kPixelFormatGBRP10LE = 75,
    kPixelFormatGBRP16BE = 76,
    kPixelFormatGBRP16LE = 77,

    //! planar YUVA, 9 bits per channel:
    kPixelFormatYUVA420P9BE = 78,
    kPixelFormatYUVA420P9LE = 79,
    kPixelFormatYUVA422P9BE = 80,
    kPixelFormatYUVA422P9LE = 81,
    kPixelFormatYUVA444P9BE = 82,
    kPixelFormatYUVA444P9LE = 83,

    //! planar YUVA, 10 bits per channel:
    kPixelFormatYUVA420P10BE = 84,
    kPixelFormatYUVA420P10LE = 85,
    kPixelFormatYUVA422P10BE = 86,
    kPixelFormatYUVA422P10LE = 87,
    kPixelFormatYUVA444P10BE = 88,
    kPixelFormatYUVA444P10LE = 89,

    //! planar YUVA, 16 bits per channel:
    kPixelFormatYUVA420P16BE = 90,
    kPixelFormatYUVA420P16LE = 91,
    kPixelFormatYUVA422P16BE = 92,
    kPixelFormatYUVA422P16LE = 93,
    kPixelFormatYUVA444P16BE = 94,
    kPixelFormatYUVA444P16LE = 95,

    //! packed XYZ:
    kPixelFormatXYZ12BE = 96,
    kPixelFormatXYZ12LE = 97,

    //! interleaved chroma YUV 4:2:2
    kPixelFormatNV16 = 98,
    kPixelFormatNV20BE = 99,
    kPixelFormatNV20LE = 100,

    //! packed YUV 4:2:2, 16bpp
    kPixelFormatYVYU422 = 101,

    //! luminance + alpha, 16 bits per channel:
    kPixelFormatYA16BE = 102,
    kPixelFormatYA16LE = 103,

    //! packed RGB 8:8:8, 32bpp, 0RGB 0RGB ...
    kPixelFormat0RGB = 104,

    //! packed RGB 8:8:8, 32bpp, RGB0 RGB0 ...
    kPixelFormatRGB0 = 105,

    //! packed BGR 8:8:8, 32bpp, 0BGR 0BGR ...
    kPixelFormat0BGR = 106,

    //! packed BGR 8:8:8, 32bpp, BGR0 BGR0 ...
    kPixelFormatBGR0 = 107,


    //! planar YUV 4:2:2, 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    kPixelFormatYUVA422P = 108,

    //! planar YUV 4:4:4, 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
    kPixelFormatYUVA444P = 109,

    //! planar YUV 4:2:0, 18bpp, (1 Cr & Cb sample per 2x2 Y samples)
    kPixelFormatYUV420P12BE = 110,
    kPixelFormatYUV420P12LE = 111,

    //! planar YUV 4:2:0, 21bpp, (1 Cr & Cb sample per 2x2 Y samples)
    kPixelFormatYUV420P14BE = 112,
    kPixelFormatYUV420P14LE = 113,

    //! planar YUV 4:2:2, 24bpp, (1 Cr & Cb sample per 2x1 Y samples)
    kPixelFormatYUV422P12BE = 114,
    kPixelFormatYUV422P12LE = 115,

    //! planar YUV 4:2:2, 28bpp, (1 Cr & Cb sample per 2x1 Y samples)
    kPixelFormatYUV422P14BE = 116,
    kPixelFormatYUV422P14LE = 117,

    //! planar YUV 4:4:4, 36bpp, (1 Cr & Cb sample per 1x1 Y samples)
    kPixelFormatYUV444P12BE = 118,
    kPixelFormatYUV444P12LE = 119,

    //! planar YUV 4:4:4, 42bpp, (1 Cr & Cb sample per 1x1 Y samples)
    kPixelFormatYUV444P14BE = 120,
    kPixelFormatYUV444P14LE = 121,

    //! planar GBR 4:4:4, 36bpp
    kPixelFormatGBRP12BE = 122,
    kPixelFormatGBRP12LE = 123,

    //! planar GBR 4:4:4, 42bpp
    kPixelFormatGBRP14BE = 124,
    kPixelFormatGBRP14LE = 125,

    //! planar GBRA 4:4:4:4, 32bpp
    kPixelFormatGBRAP = 126,

    //! planar GBRA 4:4:4:4, 64bpp
    kPixelFormatGBRAP16BE = 127,
    kPixelFormatGBRAP16LE = 128,

    //! planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG)
    kPixelFormatYUVJ411P = 129,

    //! Bayer, BGBG..(odd line), GRGR..(even line), 8 bits per channel
    kPixelFormatBayerBGGR8 = 130,

    //! Bayer, RGRG..(odd line), GBGB..(even line), 8 bits per channel
    kPixelFormatBayerRGGB8 = 131,

    //! Bayer, GBGB..(odd line), RGRG..(even line), 8 bits per channel
    kPixelFormatBayerGBRG8 = 132,

    //! Bayer, GRGR..(odd line), BGBG..(even line), 8 bits per channel
    kPixelFormatBayerGRBG8 = 133,

    //! Bayer, BGBG..(odd line), GRGR..(even line), 16 bits per channel
    kPixelFormatBayerBGGR16LE = 134,
    kPixelFormatBayerBGGR16BE = 135,

    //! Bayer, RGRG..(odd line), GBGB..(even line), 16 bits per channel
    kPixelFormatBayerRGGB16LE = 136,
    kPixelFormatBayerRGGB16BE = 137,

    //! Bayer, GBGB..(odd line), RGRG..(even line), 16 bits per channel
    kPixelFormatBayerGBRG16LE = 138,
    kPixelFormatBayerGBRG16BE = 139,

    //! Bayer, GRGR..(odd line), BGBG..(even line), 16 bits per channel
    kPixelFormatBayerGRBG16LE = 140,
    kPixelFormatBayerGRBG16BE = 141,

    //! planar YUV 4:4:0, 20bpp, (1 Cr & Cb sample per 1x2 Y samples)
    kPixelFormatYUV440P10LE = 142,
    kPixelFormatYUV440P10BE = 143,

    //! planar YUV 4:4:0, 24bpp, (1 Cr & Cb sample per 1x2 Y samples)
    kPixelFormatYUV440P12LE = 144,
    kPixelFormatYUV440P12BE = 145,

    //! packed AYUV 4:4:4, 64bpp (1 Cr & Cb sample per 1x1 Y & A samples)
    kPixelFormatAYUV64LE = 146,
    kPixelFormatAYUV64BE = 147,

    //! semi-planar YUV 4:2:0, 10bpp per component,
    //! data in the high bits, zero padding in the bottom 6 bits
    kPixelFormatP010LE = 148,
    kPixelFormatP010BE = 149,

    //! planar GBR 4:4:4:4 48bpp
    kPixelFormatGBRAP12BE = 150,
    kPixelFormatGBRAP12LE = 151,

    //! planar GBR 4:4:4:4 40bpp
    kPixelFormatGBRAP10BE = 152,
    kPixelFormatGBRAP10LE = 153,

    //! Y, 12bpp
    kPixelFormatGRAY12BE = 154,
    kPixelFormatGRAY12LE = 155,

    //! Y, 10bpp
    kPixelFormatGRAY10BE = 156,
    kPixelFormatGRAY10LE = 157,

    //! semi-planar YUV 4:2:0, 16bpp per component
    kPixelFormatP016LE = 158,
    kPixelFormatP016BE = 159,

    //! Y, 9bpp
    kPixelFormatGRAY9BE = 160,
    kPixelFormatGRAY9LE = 161,

    //! IEEE-754 single precision planar GBR 4:4:4, 96bpp
    kPixelFormatGBRPF32BE = 162,
    kPixelFormatGBRPF32LE = 163,

    //! IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp
    kPixelFormatGBRAPF32BE = 164,
    kPixelFormatGBRAPF32LE = 165,

    //! Y, 14bpp
    kPixelFormatGRAY14BE = 166,
    kPixelFormatGRAY14LE = 167,

    //! IEEE-754 single precision Y, 32bpp
    kPixelFormatGRAYF32BE = 168,
    kPixelFormatGRAYF32LE = 169,

    //! planar YUV 4:2:2, 24bpp, 12b alpha
    kPixelFormatYUVA422P12BE = 170,
    kPixelFormatYUVA422P12LE = 171,

    //! planar YUV 4:4:4, 36bpp, 12b alpha
    kPixelFormatYUVA444P12BE = 172,
    kPixelFormatYUVA444P12LE = 173,

    //! semi-planar YUV 4:4:4, 24bpp
    kPixelFormatNV24 = 174,

    //! semi-planar YVU 4:4:4, 24bpp
    kPixelFormatNV42 = 175,

  //----------------------------------------------------------------
  // YAE_NATIVE_ENDIAN
  //
#if defined(__BIG_ENDIAN__) && __BIG_ENDIAN__
#define YAE_NATIVE_ENDIAN(kPixelFormat)         \
  kPixelFormat = kPixelFormat ## BE
#else
#define YAE_NATIVE_ENDIAN(kPixelFormat)         \
  kPixelFormat = kPixelFormat ## LE
#endif
    YAE_NATIVE_ENDIAN (kPixelFormatGRAY16),
    YAE_NATIVE_ENDIAN (kPixelFormatRGB48),
    YAE_NATIVE_ENDIAN (kPixelFormatRGB565),
    YAE_NATIVE_ENDIAN (kPixelFormatRGB555),
    YAE_NATIVE_ENDIAN (kPixelFormatBGR565),
    YAE_NATIVE_ENDIAN (kPixelFormatBGR555),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV420P16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV422P16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV444P16),
    YAE_NATIVE_ENDIAN (kPixelFormatRGB444),
    YAE_NATIVE_ENDIAN (kPixelFormatBGR444),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV420P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV422P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV444P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV420P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV422P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV444P10),
    YAE_NATIVE_ENDIAN (kPixelFormatRGBA64),
    YAE_NATIVE_ENDIAN (kPixelFormatBGRA64),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRP9),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRP10),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRP16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA420P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA422P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA444P9),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA420P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA422P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA444P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA420P16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA422P16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA444P16),
    YAE_NATIVE_ENDIAN (kPixelFormatXYZ12),
    YAE_NATIVE_ENDIAN (kPixelFormatNV20),
    YAE_NATIVE_ENDIAN (kPixelFormatYA16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV420P12),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV420P14),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV422P12),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV422P14),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV444P12),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV444P14),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRP12),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRP14),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRAP16),
    YAE_NATIVE_ENDIAN (kPixelFormatBayerBGGR16),
    YAE_NATIVE_ENDIAN (kPixelFormatBayerRGGB16),
    YAE_NATIVE_ENDIAN (kPixelFormatBayerGBRG16),
    YAE_NATIVE_ENDIAN (kPixelFormatBayerGRBG16),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV440P10),
    YAE_NATIVE_ENDIAN (kPixelFormatYUV440P12),
    YAE_NATIVE_ENDIAN (kPixelFormatAYUV64),
    YAE_NATIVE_ENDIAN (kPixelFormatP010),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRAP12),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRAP10),
    YAE_NATIVE_ENDIAN (kPixelFormatGRAY12),
    YAE_NATIVE_ENDIAN (kPixelFormatGRAY10),
    YAE_NATIVE_ENDIAN (kPixelFormatP016),
    YAE_NATIVE_ENDIAN (kPixelFormatGRAY9),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRPF32),
    YAE_NATIVE_ENDIAN (kPixelFormatGBRAPF32),
    YAE_NATIVE_ENDIAN (kPixelFormatGRAY14),
    YAE_NATIVE_ENDIAN (kPixelFormatGRAYF32),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA422P12),
    YAE_NATIVE_ENDIAN (kPixelFormatYUVA444P12),
#undef YAE_NATIVE_ENDIAN
  };
}


#endif // YAE_PIXEL_FORMATS_H_
