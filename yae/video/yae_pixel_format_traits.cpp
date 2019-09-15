// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Feb 21 15:36:48 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <string.h>

// aeyae:
#include "yae_pixel_formats.h"
#include "yae_pixel_format_traits.h"


namespace yae
{
  using namespace pixelFormat;

  //----------------------------------------------------------------
  // TraitsInit
  //
  struct TraitsInit : public Traits
  {
    friend class TraitsInitVector;

    TraitsInit()
    {
      memset(this, 0, sizeof(TraitsInit));
    }

    void calc_padding()
    {
      for (int i = 0; i < channels_; i++)
      {
        if (depth_[i] < 8)
        {
          continue;
        }

        datatype_bits_[i] =
          depth_[i] == 8 ? 8 :
          depth_[i] <= 16 ? 16 :
          32;

        datatype_rpad_[i] =
          (stride_[i] - lshift_[i] - depth_[i]) % datatype_bits_[i];

        datatype_lpad_[i] =
          datatype_bits_[i] - datatype_rpad_[i] - depth_[i];
      }
    }

  protected:
    inline TraitsInit & name(const char * name)
    {
      name_ = name;
      return *this;
    }

    inline TraitsInit & flags(unsigned int flags)
    {
      flags_ = flags;
      return *this;
    }

    inline TraitsInit & channels(unsigned char channels)
    {
      channels_ = channels;
      return *this;
    }

    inline TraitsInit & chromaBoxW(unsigned char chromaBoxW)
    {
      chromaBoxW_ = chromaBoxW;
      return *this;
    }

    inline TraitsInit & chromaBoxH(unsigned char chromaBoxH)
    {
      chromaBoxH_ = chromaBoxH;
      return *this;
    }

    inline TraitsInit & plane(unsigned char a,
                              unsigned char b = 0,
                              unsigned char c = 0,
                              unsigned char d = 0)
    {
      plane_[0] = a;
      plane_[1] = b;
      plane_[2] = c;
      plane_[3] = d;
      return *this;
    }

    inline TraitsInit & depth(unsigned char a,
                              unsigned char b = 0,
                              unsigned char c = 0,
                              unsigned char d = 0)
    {
      depth_[0] = a;
      depth_[1] = b;
      depth_[2] = c;
      depth_[3] = d;
      return *this;
    }

    inline TraitsInit & lshift(unsigned char a,
                               unsigned char b = 0,
                               unsigned char c = 0,
                               unsigned char d = 0)
    {
      lshift_[0] = a;
      lshift_[1] = b;
      lshift_[2] = c;
      lshift_[3] = d;
      return *this;
    }

    inline TraitsInit & stride(unsigned char a,
                               unsigned char b = 0,
                               unsigned char c = 0,
                               unsigned char d = 0)
    {
      stride_[0] = a;
      stride_[1] = b;
      stride_[2] = c;
      stride_[3] = d;
      return *this;
    }

    inline TraitsInit & samples(unsigned char a,
                                unsigned char b = 0,
                                unsigned char c = 0,
                                unsigned char d = 0)
    {
      samples_[0] = a;
      samples_[1] = b;
      samples_[2] = c;
      samples_[3] = d;
      return *this;
    }

    inline TraitsInit & bayer(unsigned char a_even,
                              unsigned char b_even,
                              unsigned char a_odd,
                              unsigned char b_odd)
    {
      bayer_[0] = a_even;
      bayer_[1] = b_even;
      bayer_[2] = a_odd;
      bayer_[3] = b_odd;
      return *this;
    }

    inline TraitsInit & reverseEndian()
    {
      flags_ ^= (kLE | kBE);
      return *this;
    }

    inline TraitsInit & addFlags(unsigned int flags)
    {
      flags_ |= flags;
      return *this;
    }
  };

  //----------------------------------------------------------------
  // TraitsInitVector
  //
  class TraitsInitVector : public std::vector<TraitsInit>
  {
  public:
    TraitsInitVector();

  private:
    TraitsInit & set(TPixelFormatId id)
    {
      if (size() <= std::size_t(id))
      {
        resize(id + 1);
      }

      return operator[](id);
    }

    TraitsInit copy(TPixelFormatId id) const
    {
      return operator[](id);
    }
  };

  //----------------------------------------------------------------
  // TraitsInitVector::TraitsInitVector
  //
  TraitsInitVector::TraitsInitVector()
  {
    //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    set(kPixelFormatYUV420P) = TraitsInit()
      .name( "YUV420P" )
      .flags( kYUV | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    set(kPixelFormatYUYV422) = TraitsInit()
      .name( "YUYV422" )
      .flags( kYUV | kPacked )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 0, 8, 24 )
      .stride( 16, 32, 32 )
      .samples( 1, 1, 1 );

    //! packed RGB 8:8:8, 24bpp, RGBRGB...
    set(kPixelFormatRGB24) = TraitsInit()
      .name( "RGB24" )
      .flags( kRGB | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 0, 8, 16 )
      .stride( 24, 24, 24 )
      .samples( 1, 1, 1 );

    //! packed RGB 8:8:8, 24bpp, BGRBGR...
    set(kPixelFormatBGR24) = copy(kPixelFormatRGB24)
      .name( "BGR24" )
      .lshift( 16, 8, 0 );

    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    set(kPixelFormatYUV422P) = copy(kPixelFormatYUV420P)
      .name( "YUV422P" )
      .chromaBoxH( 1 );

    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    set(kPixelFormatYUV444P) = copy(kPixelFormatYUV422P)
      .name( "YUV444P" )
      .chromaBoxW( 1 );

    //! planar YUV 4:1:0, 9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    set(kPixelFormatYUV410P) = TraitsInit()
      .name( "YUV410P" )
      .flags( kYUV | kPlanar )
      .channels( 3 )
      .chromaBoxW( 4 )
      .chromaBoxH( 4 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    set(kPixelFormatYUV411P) = TraitsInit()
      .name( "YUV411P" )
      .flags( kYUV | kPlanar )
      .channels( 3 )
      .chromaBoxW( 4 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    set(kPixelFormatYUVJ411P) = copy(kPixelFormatYUV411P)
      .name( "YUVJ411P" );

    //! Y, 8bpp
    set(kPixelFormatGRAY8) = TraitsInit()
      .name( "GRAY8" )
      .flags( kPlanar )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 8 )
      .lshift( 0 )
      .stride( 8 )
      .samples( 1 );

    //! Y, 1bpp, 0 is white, 1 is black, in each byte pixels are
    //! ordered from the msb to the lsb
    set(kPixelFormatMONOWHITE) = TraitsInit()
      .name( "MONOWHITE" )
      .flags( kPlanar )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 1 )
      .lshift( 0 )
      .stride( 1 )
      .samples( 1 );

    //! Y, 1bpp, 0 is black, 1 is white, in each byte pixels are
    //! ordered from the msb to the lsb
    set(kPixelFormatMONOBLACK) = copy(kPixelFormatMONOWHITE)
      .name( "MONOBLACK" );

    //! 8 bit with kPixelFormatRGB32 palette
    set(kPixelFormatPAL8) = copy(kPixelFormatGRAY8)
      .name( "PAL8" )
      .flags( kPlanar | kPaletted );

    //! packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    set(kPixelFormatUYVY422) = TraitsInit()
      .name( "UYVY422" )
      .flags( kYUV | kPacked )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 8, 0, 16 )
      .stride( 16, 32, 32 )
      .samples( 1, 1, 1 );

    //! packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    set(kPixelFormatUYYVYY411) = TraitsInit()
      .name( "UYYVYY411" )
      .flags( kYUV | kPacked )
      .channels( 3 )
      .chromaBoxW( 4 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 8, 0, 24 )
      .stride( 24, 48, 48 )
      .samples( 2, 1, 1 );

    //! packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
    set(kPixelFormatBGR8) = TraitsInit()
      .name( "BGR8" )
      .flags( kRGB | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 3, 3, 2 )
      .lshift( 5, 2, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! packed RGB 1:2:1 bitstream, 4bpp, (msb)1B 2G 1R(lsb), a byte
    //! contains two pixels, the first pixel in the byte is the one
    //! composed by the 4 msb bits
    set(kPixelFormatBGR4) = TraitsInit()
      .name( "BGR4" )
      .flags( kRGB | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 1, 2, 1 )
      .lshift( 3, 1, 0 )
      .stride( 4, 4, 4 )
      .samples( 1, 1, 1 );

    //! packed RGB 1:2:1, 8bpp, (msb)1B 2G 1R(lsb)
    set(kPixelFormatBGR4_BYTE) = copy(kPixelFormatBGR4)
      .name( "BGR4_BYTE" )
      .lshift( 7, 5, 4 )
      .stride( 8, 8, 8 );

    //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
    set(kPixelFormatRGB8) = copy(kPixelFormatBGR8)
      .name( "RGB8" )
      .lshift( 0, 3, 5 );

    //! packed RGB 1:2:1 bitstream, 4bpp, (msb)1R 2G 1B(lsb), a byte
    //! contains two pixels, the first pixel in the byte is the one
    //! composed by the 4 msb bits
    set(kPixelFormatRGB4) = copy(kPixelFormatBGR4)
      .name( "RGB4" )
      .lshift( 0, 1, 3 );

    //! packed RGB 1:2:1, 8bpp, (msb)1R 2G 1B(lsb)
    set(kPixelFormatRGB4_BYTE) = copy(kPixelFormatBGR4_BYTE)
      .name( "RGB4_BYTE" )
      .lshift( 4, 5, 7 );

    //! planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV
    //! channels, which are interleaved (first byte U and the
    //! following byte V)
    set(kPixelFormatNV12) = TraitsInit()
      .name( "NV12" )
      .flags( kYUV | kPacked | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 1 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 8 )
      .stride( 8, 16, 16 )
      .samples( 1, 1, 1 );

    //! as above, but U and V bytes are swapped
    set(kPixelFormatNV21) = copy(kPixelFormatNV12)
      .name( "NV21" )
      .lshift( 0, 8, 0 );

    //! packed ARGB 8:8:8:8, 32bpp, ARGB ARGB ...
    set(kPixelFormatARGB) = TraitsInit()
      .name( "ARGB" )
      .flags( kAlpha | kRGB | kPacked )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 8, 16, 24, 0 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    //! packed RGBA 8:8:8:8, 32bpp, RGBA RGBA ...
    set(kPixelFormatRGBA) = copy(kPixelFormatARGB)
      .name( "RGBA" )
      .lshift( 0, 8, 16, 24 );

    //! packed ABGR 8:8:8:8, 32bpp, ABGR ABGR ...
    set(kPixelFormatABGR) = copy(kPixelFormatARGB)
      .name( "ABGR" )
      .lshift( 24, 16, 8, 0 );

    //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    set(kPixelFormatBGRA) = copy(kPixelFormatARGB)
      .name( "BGRA" )
      .lshift( 16, 8, 0, 24 );

    //! Y, 16bpp, big-endian
    set(kPixelFormatGRAY16BE) = copy(kPixelFormatGRAY8)
      .name( "GRAY16BE" )
      .addFlags( kBE )
      .depth( 16 )
      .stride( 16 );

    //! same as above, but little-endian
    set(kPixelFormatGRAY16LE) = copy(kPixelFormatGRAY16BE)
      .reverseEndian()
      .name("GRAY16LE");

    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples)
    set(kPixelFormatYUV440P) = copy(kPixelFormatYUV420P)
      .name( "YUV440P" )
      .chromaBoxW( 1 )
      .chromaBoxH( 2 );

    //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
    //! samples)
    set(kPixelFormatYUVA420P) = copy(kPixelFormatYUV420P)
      .name( "YUVA420P" )
      .addFlags( kAlpha )
      .channels( 4 )
      .plane( 0, 1, 2, 3 )
      .depth( 8, 8, 8, 8 )
      .lshift( 0, 0, 0, 0 )
      .stride( 8, 8, 8, 8 )
      .samples( 1, 1, 1, 1 );

    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as big-endian
    set(kPixelFormatRGB48BE) = copy(kPixelFormatRGB24)
      .name( "RGB48BE" )
      .addFlags( kBE )
      .depth( 16, 16, 16 )
      .lshift( 0, 16, 32 )
      .stride( 48, 48, 48 );

    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as little-endian
    set(kPixelFormatRGB48LE) = copy(kPixelFormatRGB48BE)
      .reverseEndian()
      .name("RGB48LE");

    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
    set(kPixelFormatRGB565BE) = TraitsInit()
      .name( "RGB565BE" )
      .flags( kBE | kRGB | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 6, 5 )
      .lshift( 0, 5, 11 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
    set(kPixelFormatRGB565LE) = copy(kPixelFormatRGB565BE)
      .reverseEndian()
      .name("RGB565LE");

    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
    //! most significant bit to 0
    set(kPixelFormatRGB555BE) = copy(kPixelFormatRGB565BE)
      .name( "RGB555BE" )
      .depth( 5, 5, 5 )
      .lshift( 1, 6, 11 );

    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
    //! most significant bit to 0
    set(kPixelFormatRGB555LE) = copy(kPixelFormatRGB555BE)
      .reverseEndian()
      .name("RGB555LE");

    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
    set(kPixelFormatBGR565BE) = copy(kPixelFormatRGB565BE)
      .name( "BGR565BE" )
      .lshift( 11, 5, 0 );

    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
    set(kPixelFormatBGR565LE) = copy(kPixelFormatBGR565BE)
      .reverseEndian()
      .name("BGR565LE");

    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
    //! most significant bit to 1
    set(kPixelFormatBGR555BE) = copy(kPixelFormatRGB555BE)
      .name( "BGR555BE" )
      .lshift( 11, 6, 1 );

    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
    //! most significant bit to 1
    set(kPixelFormatBGR555LE) = copy(kPixelFormatBGR555BE)
      .reverseEndian()
      .name("BGR555LE");

    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! little-endian
    set(kPixelFormatYUV420P16LE) = copy(kPixelFormatYUV420P)
      .name( "YUV420P16LE" )
      .addFlags( kLE )
      .depth( 16, 16, 16 )
      .stride( 16, 16, 16 );

    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! big-endian
    set(kPixelFormatYUV420P16BE) = copy(kPixelFormatYUV420P16LE)
      .reverseEndian()
      .name("YUV420P16BE");

    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! little-endian
    set(kPixelFormatYUV422P16LE) = copy(kPixelFormatYUV420P16LE)
      .name( "YUV422P16LE" )
      .chromaBoxH( 1 );

    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! big-endian
    set(kPixelFormatYUV422P16BE) = copy(kPixelFormatYUV422P16LE)
      .reverseEndian()
      .name("YUV422P16BE");

    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! little-endian
    set(kPixelFormatYUV444P16LE) = copy(kPixelFormatYUV422P16LE)
      .name( "YUV444P16LE" )
      .chromaBoxW( 1 );

    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! big-endian
    set(kPixelFormatYUV444P16BE) = copy(kPixelFormatYUV444P16LE)
      .reverseEndian()
      .name("YUV444P16BE");

    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
    //! most significant bits to 0
    set(kPixelFormatRGB444BE) = copy(kPixelFormatRGB565BE)
      .name( "RGB444BE" )
      .depth( 4, 4, 4 )
      .lshift( 4, 8, 12 );

    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
    //! most significant bits to 0
    set(kPixelFormatRGB444LE) = copy(kPixelFormatRGB444BE)
      .reverseEndian()
      .name("RGB444LE");

    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
    //! most significant bits to 1
    set(kPixelFormatBGR444BE) = copy(kPixelFormatRGB444BE)
      .name( "BGR444BE" )
      .lshift( 12, 8, 4 );

    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
    //! most significant bits to 1
    set(kPixelFormatBGR444LE) = copy(kPixelFormatBGR444BE)
      .reverseEndian()
      .name("BGR444LE");

    //! 8bit gray, 8bit alpha
    set(kPixelFormatY400A) = TraitsInit()
      .name( "Y400A" )
      .flags( kAlpha | kPacked )
      .channels( 2 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0, 0 )
      .depth( 8, 8 )
      .lshift( 0, 8 )
      .stride( 16, 16 )
      .samples( 1, 1 );

    //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples), JPEG
    set(kPixelFormatYUVJ420P) = copy(kPixelFormatYUV420P)
      .name( "YUVJ420P" );

    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    set(kPixelFormatYUVJ422P) = copy(kPixelFormatYUV422P)
      .name( "YUVJ422P" );

    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    set(kPixelFormatYUVJ444P) = copy(kPixelFormatYUV444P)
      .name( "YUVJ444P" );

    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples), JPEG
    set(kPixelFormatYUVJ440P) = copy(kPixelFormatYUV440P)
      .name( "YUVJ440P" );

    //! planar YUV 4:2:0, 9-bits per channel:
    set(kPixelFormatYUV420P9BE) = copy(kPixelFormatYUV420P)
      .name( "YUV420P9BE" )
      .addFlags( kBE )
      .depth( 9, 9, 9 )
      .lshift( 7, 7, 7 )
      .stride( 16, 16, 16 );

    set(kPixelFormatYUV420P9LE) = copy(kPixelFormatYUV420P9BE)
      .reverseEndian()
      .name("YUV420P9LE");

    //! planar YUV 4:2:2, 9-bits per channel:
    set(kPixelFormatYUV422P9BE) = copy(kPixelFormatYUV420P9BE)
      .name( "YUV422P9BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUV422P9LE) = copy(kPixelFormatYUV422P9BE)
      .reverseEndian()
      .name("YUV422P9LE");

    //! planar YUV 4:4:4, 9-bits per channel:
    set(kPixelFormatYUV444P9BE) = copy(kPixelFormatYUV422P9BE)
      .name( "YUV444P9BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUV444P9LE) = copy(kPixelFormatYUV444P9BE)
      .reverseEndian()
      .name("YUV444P9LE");

    //! planar YUV 4:2:0, 10-bits per channel:
    set(kPixelFormatYUV420P10BE) = copy(kPixelFormatYUV420P9BE)
      .name( "YUV420P10BE" )
      .depth( 10, 10, 10 )
      .lshift( 6, 6, 6 );

    set(kPixelFormatYUV420P10LE) = copy(kPixelFormatYUV420P10BE)
      .reverseEndian()
      .name("YUV420P10LE");

    //! planar YUV 4:2:2, 10-bits per channel:
    set(kPixelFormatYUV422P10BE) = copy(kPixelFormatYUV420P10BE)
      .name( "YUV422P10BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUV422P10LE) = copy(kPixelFormatYUV422P10BE)
      .reverseEndian()
      .name("YUV422P10LE");

    //! planar YUV 4:4:4, 10-bits per channel:
    set(kPixelFormatYUV444P10BE) = copy(kPixelFormatYUV422P10BE)
      .name( "YUV444P10BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUV444P10LE) = copy(kPixelFormatYUV444P10BE)
      .reverseEndian()
      .name("YUV444P10LE");


    set(kPixelFormatRGBA64BE) = copy(kPixelFormatARGB)
      .name( "RGBA64BE" )
      .addFlags( kBE )
      .depth( 16, 16, 16, 16 )
      .lshift( 0, 16, 32, 48 )
      .stride( 64, 64, 64, 64 );

    set(kPixelFormatRGBA64LE) = copy(kPixelFormatRGBA64BE)
      .reverseEndian()
      .name("RGBA64LE");


    set(kPixelFormatBGRA64BE) = copy(kPixelFormatRGBA64BE)
      .name( "BGRA64BE" )
      .lshift( 32, 16, 0, 48 );

    set(kPixelFormatBGRA64LE) = copy(kPixelFormatBGRA64LE)
      .reverseEndian()
      .name("BGRA64LE");


    set(kPixelFormatGBRP) = TraitsInit()
      .name( "GBRP" )
      .flags( kRGB | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 2, 0, 1 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );


    set(kPixelFormatGBRP9BE) = copy(kPixelFormatGBRP)
      .name( "GBRP9BE" )
      .addFlags( kBE )
      .depth( 9, 9, 9 )
      .lshift( 7, 7, 7 )
      .stride( 16, 16, 16 );

    set(kPixelFormatGBRP9LE) = copy(kPixelFormatGBRP9BE)
      .reverseEndian()
      .name("GBRP9LE");


    set(kPixelFormatGBRP10BE) = copy(kPixelFormatGBRP9BE)
      .name( "GBRP10BE" )
      .depth( 10, 10, 10 )
      .lshift( 6, 6, 6 );

    set(kPixelFormatGBRP10LE) = copy(kPixelFormatGBRP10BE)
      .reverseEndian()
      .name("GBRP10LE");


    set(kPixelFormatGBRP16BE) = copy(kPixelFormatGBRP9BE)
      .name( "GBRP16BE" )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 );

    set(kPixelFormatGBRP16LE) = copy(kPixelFormatGBRP16BE)
      .reverseEndian()
      .name("GBRP16LE");


    set(kPixelFormatYUVA420P9BE) = copy(kPixelFormatYUVA420P)
      .name( "YUVA420P9BE" )
      .addFlags( kBE )
      .depth( 9, 9, 9, 9 )
      .lshift( 7, 7, 7, 7 )
      .stride( 16, 16, 16, 16 );

    set(kPixelFormatYUVA420P9LE) = copy(kPixelFormatYUVA420P9BE)
      .reverseEndian()
      .name("YUVA420P9LE");


    set(kPixelFormatYUVA422P9BE) = copy(kPixelFormatYUVA420P9BE)
      .name( "YUVA422P9BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUVA422P9LE) = copy(kPixelFormatYUVA422P9BE)
      .reverseEndian()
      .name("YUVA422P9LE");


    set(kPixelFormatYUVA444P9BE) = copy(kPixelFormatYUVA422P9BE)
      .name( "YUVA444P9BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUVA444P9LE) = copy(kPixelFormatYUVA444P9BE)
      .reverseEndian()
      .name("YUVA444P9LE");


    set(kPixelFormatYUVA420P10BE) = copy(kPixelFormatYUVA420P9BE)
      .name( "YUVA420P10BE" )
      .depth( 10, 10, 10, 10 )
      .lshift( 6, 6, 6, 6 );

    set(kPixelFormatYUVA420P10LE) = copy(kPixelFormatYUVA420P10BE)
      .reverseEndian()
      .name("YUVA420P10LE");


    set(kPixelFormatYUVA422P10BE) = copy(kPixelFormatYUVA420P10BE)
      .name( "YUVA422P10BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUVA422P10LE) = copy(kPixelFormatYUVA422P10BE)
      .reverseEndian()
      .name("YUVA422P10LE");


    set(kPixelFormatYUVA444P10BE) = copy(kPixelFormatYUVA422P10BE)
      .name( "YUVA444P10BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUVA444P10LE) = copy(kPixelFormatYUVA444P10BE)
      .reverseEndian()
      .name("YUVA444P10LE");


    set(kPixelFormatYUVA420P16BE) = copy(kPixelFormatYUVA420P9BE)
      .name( "YUVA420P16BE" )
      .depth( 16, 16, 16, 16 )
      .lshift( 0, 0, 0, 0 );

    set(kPixelFormatYUVA420P16LE) = copy(kPixelFormatYUVA420P16BE)
      .reverseEndian()
      .name("YUVA420P16LE");


    set(kPixelFormatYUVA422P16BE) = copy(kPixelFormatYUVA420P16BE)
      .name( "YUVA422P16BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUVA422P16LE) = copy(kPixelFormatYUVA422P16BE)
      .reverseEndian()
      .name("YUVA422P16LE");


    set(kPixelFormatYUVA444P16BE) = copy(kPixelFormatYUVA422P16BE)
      .name( "YUVA444P16BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUVA444P16LE) = copy(kPixelFormatYUVA444P16BE)
      .reverseEndian()
      .name("YUVA444P16LE");


    set(kPixelFormatXYZ12BE) = TraitsInit()
      .name( "XYZ12BE" )
      .flags( kBE | kXYZ | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 12, 12, 12 )
      .lshift( 4, 20, 36 )
      .stride( 48, 48, 48 )
      .samples( 1, 1, 1 );

    set(kPixelFormatXYZ12LE) = copy(kPixelFormatXYZ12BE)
      .reverseEndian()
      .name("XYZ12LE");

    //! planar YUV 4:2:2, 16bpp, 1 plane for Y and 1 plane for the UV
    //! channels, which are interleaved (first byte U and the
    //! following byte V), 8 bits per channel
    set(kPixelFormatNV16) = copy(kPixelFormatNV12)
      .name( "NV16" )
      .chromaBoxH( 1 );

    //! planar YUV 4:2:2, 20bpp, 1 plane for Y and 1 plane for the UV
    //! channels, which are interleaved (first byte U and the
    //! following byte V), 10 bits per channel
    set(kPixelFormatNV20BE) = copy(kPixelFormatNV16)
      .name( "NV20BE" )
      .addFlags( kBE )
      .depth( 10, 10, 10 )
      .lshift( 6, 6, 22 )
      .stride( 16, 32, 32 );

    set(kPixelFormatNV20LE) = copy(kPixelFormatNV20BE)
      .reverseEndian()
      .name("NV20LE");


    set(kPixelFormatYVYU422) = copy(kPixelFormatUYVY422)
      .name( "YVYU422" )
      .lshift( 0, 24, 8 );


    set(kPixelFormatYA16BE) = copy(kPixelFormatY400A)
      .name( "YA16BE" )
      .addFlags( kBE )
      .depth( 16, 16 )
      .lshift( 0, 16 )
      .stride( 16, 16 );

    set(kPixelFormatYA16LE) = copy(kPixelFormatYA16BE)
      .reverseEndian()
      .name("YA16LE");


    set(kPixelFormat0RGB) = copy(kPixelFormatRGB24)
      .name( "0RGB" )
      .lshift( 8, 16, 24 )
      .stride( 32, 32, 32 );

    set(kPixelFormatRGB0) = copy(kPixelFormat0RGB)
      .name( "RGB0" )
      .lshift( 0, 8, 16 );

    set(kPixelFormat0BGR) = copy(kPixelFormat0RGB)
      .name( "0BGR" )
      .lshift( 24, 16, 8 );

    set(kPixelFormatBGR0) = copy(kPixelFormat0RGB)
      .name( "BGR0" )
      .lshift( 16, 8, 0 );


    set(kPixelFormatYUVA422P) = copy(kPixelFormatYUVA420P)
      .name( "YUVA422P" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUVA444P) = copy(kPixelFormatYUVA422P)
      .name( "YUVA444P" )
      .chromaBoxW( 1 );


    set(kPixelFormatYUV420P12LE) = copy(kPixelFormatYUV420P16LE)
      .name( "YUV420P12LE" )
      .depth( 12, 12, 12 )
      .lshift( 4, 4, 4 );

    set(kPixelFormatYUV420P12BE) = copy(kPixelFormatYUV420P12LE)
      .reverseEndian()
      .name("YUV420P12BE");


    set(kPixelFormatYUV420P14LE) = copy(kPixelFormatYUV420P16LE)
      .name( "YUV420P14LE" )
      .depth( 14, 14, 14 )
      .lshift( 2, 2, 2 );

    set(kPixelFormatYUV420P14BE) = copy(kPixelFormatYUV420P14LE)
      .reverseEndian()
      .name("YUV420P14BE");


    set(kPixelFormatYUV422P12BE) = copy(kPixelFormatYUV420P12BE)
      .name( "YUV422P12BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUV422P12LE) = copy(kPixelFormatYUV422P12BE)
      .reverseEndian()
      .name("YUV422P12LE");


    set(kPixelFormatYUV422P14BE) = copy(kPixelFormatYUV420P14BE)
      .name( "YUV422P14BE" )
      .chromaBoxH( 1 );

    set(kPixelFormatYUV422P14LE) = copy(kPixelFormatYUV422P14BE)
      .reverseEndian()
      .name("YUV422P14LE");


    set(kPixelFormatYUV444P12BE) = copy(kPixelFormatYUV422P12BE)
      .name( "YUV444P12BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUV444P12LE) = copy(kPixelFormatYUV444P12BE)
      .reverseEndian()
      .name("YUV444P12LE");


    set(kPixelFormatYUV444P14BE) = copy(kPixelFormatYUV422P14BE)
      .name( "YUV444P14BE" )
      .chromaBoxW( 1 );

    set(kPixelFormatYUV444P14LE) = copy(kPixelFormatYUV444P14BE)
      .reverseEndian()
      .name("YUV444P14LE");


    set(kPixelFormatGBRP12BE) = copy(kPixelFormatGBRP16BE)
      .name( "GBRP12BE" )
      .depth( 12, 12, 12 )
      .lshift( 4, 4, 4 );

    set(kPixelFormatGBRP12LE) = copy(kPixelFormatGBRP12BE)
      .reverseEndian()
      .name("GBRP12LE");


    set(kPixelFormatGBRP14BE) = copy(kPixelFormatGBRP16BE)
      .name( "GBRP14BE" )
      .depth( 14, 14, 14 )
      .lshift( 2, 2, 2 );

    set(kPixelFormatGBRP14LE) = copy(kPixelFormatGBRP14BE)
      .reverseEndian()
      .name("GBRP14LE");


    set(kPixelFormatGBRAP) = copy(kPixelFormatGBRP)
      .name( "GBRAP" )
      .addFlags( kAlpha )
      .channels( 4 )
      .plane( 2, 0, 1, 3 )
      .depth( 8, 8, 8, 8 )
      .lshift( 0, 0, 0, 0 )
      .stride( 8, 8, 8, 8 )
      .samples( 1, 1, 1, 1 );


    set(kPixelFormatGBRAP16BE) = copy(kPixelFormatGBRAP)
      .name( "GBRAP16BE" )
      .addFlags( kBE )
      .depth( 16, 16, 16, 16 )
      .lshift( 0, 0, 0, 0 )
      .stride( 16, 16, 16, 16 );

    set(kPixelFormatGBRAP16LE) = copy(kPixelFormatGBRAP16BE)
      .reverseEndian()
      .name("GBRAP16LE");

    // even lines:  G R  G R
    //  odd lines:  B G  B G
    set(kPixelFormatBayerBGGR8) = TraitsInit()
      .name( "BayerBGGR8" )
      .flags( kBayer | kPacked )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .bayer( 1, 0, 2, 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 0, 8, 0, 8 )
      .stride( 16, 16, 16, 16 )
      .samples( 1, 2, 1 );

    // even lines:  G B  G B
    //  odd lines:  R G  R G
    set(kPixelFormatBayerRGGB8) = copy(kPixelFormatBayerBGGR8)
      .name( "BayerRGGB8" )
      .bayer( 1, 2, 0, 1 );

    // even lines:  R G  R G
    //  odd lines:  G B  G B
    set(kPixelFormatBayerGBRG8) = copy(kPixelFormatBayerBGGR8)
      .name( "BayerGBRG8" )
      .bayer( 0, 1, 1, 2 );

    // even lines:  B G  B G
    //  odd lines:  G R  G R
    set(kPixelFormatBayerGRBG8) = copy(kPixelFormatBayerBGGR8)
      .name( "BayerGRBG8" )
      .bayer( 2, 1, 1, 0 );


    // even lines:  G R  G R
    //  odd lines:  B G  B G
    set(kPixelFormatBayerBGGR16LE) = copy(kPixelFormatBayerBGGR8)
      .name( "BayerBGGR16LE" )
      .addFlags( kLE )
      .depth( 16, 16, 16, 16 )
      .lshift( 0, 16, 0, 16 )
      .stride( 32, 32, 32, 32 );

    set(kPixelFormatBayerBGGR16BE) = copy(kPixelFormatBayerBGGR16LE)
      .reverseEndian()
      .name("BayerBGGR16BE");


    // even lines:  G B  G B
    //  odd lines:  R G  R G
    set(kPixelFormatBayerRGGB16LE) = copy(kPixelFormatBayerBGGR16LE)
      .name( "BayerRGGB16LE" )
      .bayer( 1, 2, 0, 1 );

    set(kPixelFormatBayerRGGB16BE) = copy(kPixelFormatBayerRGGB16LE)
      .reverseEndian()
      .name("BayerRGGB16BE");


    // even lines:  R G  R G
    //  odd lines:  G B  G B
    set(kPixelFormatBayerGBRG16LE) = copy(kPixelFormatBayerBGGR16LE)
      .name( "BayerGBRG16LE" )
      .bayer( 0, 1, 1, 2 );

    set(kPixelFormatBayerGBRG16BE) = copy(kPixelFormatBayerGBRG16LE)
      .reverseEndian()
      .name("BayerGBRG16BE");


    // even lines:  B G  B G
    //  odd lines:  G R  G R
    set(kPixelFormatBayerGRBG16LE) = copy(kPixelFormatBayerBGGR16LE)
      .name( "BayerGRBG16LE" )
      .bayer( 2, 1, 1, 0 );

    set(kPixelFormatBayerGRBG16BE) = copy(kPixelFormatBayerGRBG16LE)
      .reverseEndian()
      .name("BayerGRBG16BE");


    set(kPixelFormatYUV440P10LE) = copy(kPixelFormatYUV440P)
      .name( "YUV440P10LE" )
      .depth( 10, 10, 10 )
      .lshift( 6, 6, 6 )
      .addFlags( kLE );

    set(kPixelFormatYUV440P10BE) = copy(kPixelFormatYUV440P10LE)
      .name( "YUV440P10BE" )
      .reverseEndian();

    set(kPixelFormatYUV440P12LE) = copy(kPixelFormatYUV440P)
      .name( "YUV440P12LE" )
      .depth( 12, 12, 12 )
      .lshift( 4, 4, 4 )
      .addFlags( kLE );

    set(kPixelFormatYUV440P12BE) = copy(kPixelFormatYUV440P12LE)
      .name( "YUV440P12BE" )
      .reverseEndian();

    //! packed AYUV 4:4:4, 64bpp (1 Cr & Cb sample per 1x1 Y & A samples)
    set(kPixelFormatAYUV64LE) = TraitsInit()
      .name( "AYUV64LE" )
      .flags( kAlpha | kYUV | kPacked | kLE)
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 16, 16, 16, 16 )
      .lshift( 16, 32, 48, 0 )
      .stride( 64, 64, 64, 64 )
      .samples( 1, 1, 1, 1 );

    set(kPixelFormatAYUV64BE) = copy(kPixelFormatAYUV64LE)
      .name("AYUV64BE")
      .reverseEndian();

    //! semi-planar YUV 4:2:0, 10bpp per component,
    //! data in the high bits, zero padding in the bottom 6 bits
    set(kPixelFormatP010LE) = copy(kPixelFormatNV12)
      .name( "P010LE" )
      .depth( 10, 10, 10 )
      .lshift( 0, 0, 16 )
      .stride( 16, 32, 32 )
      .addFlags( kLE );

    set(kPixelFormatP010BE) = copy(kPixelFormatP010LE)
      .name( "P010BE" )
      .reverseEndian();

    //! planar GBR 4:4:4:4 48bpp
    set(kPixelFormatGBRAP12BE) = copy(kPixelFormatGBRAP)
      .name( "GBRAP12BE" )
      .depth( 12, 12, 12, 12 )
      .lshift( 4, 4, 4, 4 )
      .stride( 16, 16, 16, 16 )
      .addFlags( kBE );

    set(kPixelFormatGBRAP12LE) = copy(kPixelFormatGBRAP12BE)
      .name( "GBRAP12LE" )
      .reverseEndian();

    //! planar GBR 4:4:4:4 40bpp
    set(kPixelFormatGBRAP10BE) = copy(kPixelFormatGBRAP)
      .name( "GBRAP10BE" )
      .depth( 10, 10, 10, 10 )
      .lshift( 6, 6, 6, 6 )
      .stride( 16, 16, 16, 16 )
      .addFlags( kBE );

    set(kPixelFormatGBRAP10LE) = copy(kPixelFormatGBRAP10BE)
      .name( "GBRAP10LE" )
      .reverseEndian();

    //! Y, 12bpp
    set(kPixelFormatGRAY12BE) = copy(kPixelFormatGRAY16BE)
      .name( "GRAY12BE" )
      .depth( 12 )
      .lshift( 4 )
      .stride( 16 );

    set(kPixelFormatGRAY12LE) = copy(kPixelFormatGRAY12BE)
      .name( " GRAY12LE" )
      .reverseEndian();

    //! Y, 10bpp
    set(kPixelFormatGRAY10BE) = copy(kPixelFormatGRAY16BE)
      .name( "GRAY10BE" )
      .depth( 10 )
      .lshift( 6 )
      .stride( 16 );

    set(kPixelFormatGRAY10LE) = copy(kPixelFormatGRAY10BE)
      .name( "GRAY10LE" )
      .reverseEndian();

    //! semi-planar YUV 4:2:0, 16bpp per component
    set(kPixelFormatP016LE) = copy(kPixelFormatNV12)
      .name( "P016LE" )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 16 )
      .stride( 16, 32, 32 )
      .addFlags( kLE );

    set(kPixelFormatP016BE) = copy(kPixelFormatP016LE)
      .name( "P016BE" )
      .reverseEndian();

    //! Y, 9bpp
    set(kPixelFormatGRAY9BE) = copy(kPixelFormatGRAY16BE)
      .name( "GRAY9BE" )
      .depth( 9 )
      .lshift( 7 )
      .stride( 16 );

    set(kPixelFormatGRAY9LE) = copy(kPixelFormatGRAY9BE)
      .name( "GRAY9LE" )
      .reverseEndian();

    //! IEEE-754 single precision planar GBR 4:4:4, 96bpp
    set(kPixelFormatGBRPF32BE) = TraitsInit()
      .name( "GBRPF32BE" )
      .flags( kRGB | kPlanar | kBE | kFloat )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 2, 0, 1 )
      .depth( 32, 32, 32 )
      .lshift( 0, 0, 0 )
      .stride( 32, 32, 32 )
      .samples( 1, 1, 1 );

    set(kPixelFormatGBRPF32LE) = copy(kPixelFormatGBRPF32BE)
      .name( "GBRPF32LE" )
      .reverseEndian();

    //! IEEE-754 single precision planar GBRA 4:4:4:4, 128bpp
    set(kPixelFormatGBRAPF32BE) = TraitsInit()
      .name( "GBRAPF32BE" )
      .flags( kRGB | kAlpha | kPlanar | kBE | kFloat )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 2, 0, 1, 3 )
      .depth( 32, 32, 32, 32 )
      .lshift( 0, 0, 0, 0 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    set(kPixelFormatGBRAPF32LE) = copy(kPixelFormatGBRAPF32BE)
      .name( "GBRAPF32LE" )
      .reverseEndian();

    //! Y, 14bpp
    set(kPixelFormatGRAY14BE) = copy(kPixelFormatGRAY16BE)
      .name( "GRAY14BE" )
      .depth( 14 )
      .lshift( 2 )
      .stride( 16 );

    set(kPixelFormatGRAY14LE) = copy(kPixelFormatGRAY14BE)
      .name( "GRAY14LE" )
      .reverseEndian();

    //! IEEE-754 single precision Y, 32bpp
    set(kPixelFormatGRAYF32BE) = TraitsInit()
      .name( "GRAYF32BE" )
      .flags( kPlanar | kFloat )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 32 )
      .lshift( 0 )
      .stride( 32 )
      .samples( 1 );

    set(kPixelFormatGRAYF32LE) = copy(kPixelFormatGRAYF32BE)
      .name( "GRAYF32LE" )
      .reverseEndian();

    //! planar YUV 4:2:2, 24bpp, 12b alpha
    set(kPixelFormatYUVA422P12BE) = copy(kPixelFormatYUVA422P16BE)
      .name( "YUVA422P12BE" )
      .depth( 12, 12, 12, 12 )
      .lshift( 4, 4, 4, 4 );

    set(kPixelFormatYUVA422P12LE) = copy(kPixelFormatYUVA422P12BE)
      .name( "YUVA422P12LE" )
      .reverseEndian();

    //! planar YUV 4:4:4, 36bpp, 12b alpha
    set(kPixelFormatYUVA444P12BE) = copy(kPixelFormatYUVA444P16BE)
      .name( "YUVA444P12BE" )
      .depth( 12, 12, 12, 12 )
      .lshift( 4, 4, 4, 4 );

    set(kPixelFormatYUVA444P12LE) = copy(kPixelFormatYUVA444P12BE)
      .name( "YUVA444P12LE" )
      .reverseEndian();

    //! semi-planar YUV 4:4:4, 24bpp
    set(kPixelFormatNV24) = copy(kPixelFormatNV12)
      .name( "NV24" )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 );

    //! semi-planar YVU 4:4:4, 24bpp
    set(kPixelFormatNV42) = copy(kPixelFormatNV21)
      .name( "NV42" )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 );

    for (TraitsInitVector::iterator i = TraitsInitVector::begin();
         i != TraitsInitVector::end(); ++i)
    {
      TraitsInit & traits = *i;
      traits.calc_padding();
    }
  }

  //----------------------------------------------------------------
  // kTraits
  //
  static const TraitsInitVector kTraits;


  //----------------------------------------------------------------
  // Traits::rshift
  //
  unsigned char
  Traits::rshift(unsigned char channel) const
  {
    return stride_[channel] - depth_[channel] - lshift_[channel];
  }

  //----------------------------------------------------------------
  // Traits::getPlanes
  //
  unsigned char
  Traits::getPlanes(unsigned char stride[4]) const
  {
    unsigned char known[4];
    known[0] = plane_[0];
    stride[0] = stride_[0];

    unsigned char numPlanes = 1;
    for (unsigned char i = 1; i < channels_; i++)
    {
      bool same = false;
      for (unsigned char j = 0; j < numPlanes; j++)
      {
        if (plane_[i] == known[j])
        {
          same = true;
          break;
        }
      }

      if (!same)
      {
        known[numPlanes] = plane_[i];
        stride[numPlanes] = stride_[i];
        numPlanes++;
      }
    }

    return numPlanes;
  }

  namespace pixelFormat
  {
    //----------------------------------------------------------------
    // getTraits
    //
    const Traits *
    getTraits(TPixelFormatId id)
    {
      static const std::size_t numFormats = kTraits.size();

      if (id >= 0 && std::size_t(id) < numFormats)
      {
        return &kTraits[id];
      }

      return NULL;
    }
  }


  //----------------------------------------------------------------
  // toUInt16
  //
  inline static unsigned short int
  toUInt16(const unsigned char * src, const bool littleEndian)
  {
    return
      littleEndian ?
      ((unsigned short int)(src[1]) << 8) | src[0] :
      ((unsigned short int)(src[0]) << 8) | src[1];
  }

  //----------------------------------------------------------------
  // pixelIntensity
  //
  double
  pixelIntensity(const int x,
                 const int y,
                 const unsigned char * data,
                 const std::size_t rowBytes,
                 const pixelFormat::Traits & ptts)
  {
    static const unsigned short int bitmask[] = {
      0x0,
      0x1, 0x3, 0x7, 0xF,
      0x1F, 0x3F, 0x7F, 0xFF,
      0x1FF, 0x3FF, 0x7FF, 0xFFF,
      0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
    };

    const bool littleEndian = ptts.flags_ & pixelFormat::kLE;

    int localIndex = x % ptts.samples_[0];
    unsigned int skipBits = (ptts.stride_[0]) * (x / ptts.samples_[0]);
    unsigned int skipBytes = skipBits >> 3;

    const unsigned char * src = data + y * rowBytes + skipBytes;

    uint64 pixel = 0;

    if (ptts.stride_[0] == 8)
    {
      pixel = src[0];
    }
    else if (ptts.stride_[0] == 16)
    {
      pixel = toUInt16(src, littleEndian);
    }
    else if (ptts.stride_[0] == 24)
    {
      YAE_ASSERT(!littleEndian);
      pixel = ((unsigned int)(src[0] << 16) |
               (unsigned int)(src[1] << 8) |
               (unsigned int)(src[2]));
    }
    else if (ptts.stride_[0] == 32)
    {
      YAE_ASSERT(!littleEndian);
      pixel = ((unsigned int)(src[0] << 24) |
               (unsigned int)(src[1] << 16) |
               (unsigned int)(src[2] << 8) |
               (unsigned int)(src[3]));
    }
    else if (ptts.stride_[0] == 48)
    {
      uint64 a = toUInt16(src,     littleEndian);
      uint64 b = toUInt16(src + 2, littleEndian);
      uint64 c = toUInt16(src + 4, littleEndian);

      pixel = (a << 32) | (b << 16) | c;
    }
    else if (ptts.stride_[0] < 8)
    {
      std::size_t skip = skipBits - (skipBytes << 3);
      pixel =
        bitmask[ptts.stride_[0]] &
        (src[0] >> (8 - ptts.stride_[0] - skip));
    }
    else
    {
      // FIXME: write me!
      YAE_ASSERT(false);
      return 0.0;
    }

    if (ptts.flags_ & pixelFormat::kRGB)
    {
      unsigned short int r =
        bitmask[ptts.depth_[0]] &
        (pixel >> (ptts.stride_[0] - ptts.lshift_[0] - ptts.depth_[0]));

      unsigned short int g =
        bitmask[ptts.depth_[1]] &
        (pixel >> (ptts.stride_[1] - ptts.lshift_[1] - ptts.depth_[1]));

      unsigned short int b =
        bitmask[ptts.depth_[2]] &
        (pixel >> (ptts.stride_[2] - ptts.lshift_[2] - ptts.depth_[2]));

      double t = (double(r) / double(bitmask[ptts.depth_[0]]) +
                  double(g) / double(bitmask[ptts.depth_[1]]) +
                  double(b) / double(bitmask[ptts.depth_[2]])) / 3.0;
      return t;
    }
    else if (!(ptts.flags_ & pixelFormat::kPaletted))
    {
      unsigned short int lum =
        bitmask[ptts.depth_[0]] &
        (pixel >> (ptts.stride_[0] -
                   ptts.lshift_[0] -
                   ptts.depth_[0] * (1 + localIndex)));
      double t = double(lum) / double(bitmask[ptts.depth_[0]]);
      return t;
    }

    YAE_ASSERT(false);
    return 0.0;
  }

}
