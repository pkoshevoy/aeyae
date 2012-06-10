// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Mon Feb 21 15:36:48 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:

// yae includes:
#include <yaePixelFormats.h>
#include <yaePixelFormatTraits.h>


namespace yae
{
  using namespace pixelFormat;

  //----------------------------------------------------------------
  // TraitsInit
  //
  struct TraitsInit : public Traits
  {
    friend class TraitsInitVector;

  protected:
    inline TraitsInit & name(const char * name)
    {
      name_ = name;
      return *this;
    }

    inline TraitsInit & flags(unsigned char flags)
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
  };

  //----------------------------------------------------------------
  // TraitsInitVector::TraitsInitVector
  //
  TraitsInitVector::TraitsInitVector()
  {
    //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    set(kPixelFormatYUV420P) = TraitsInit()
      .name( "YUV420P" )
      .flags( kColor | kPlanar )
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
      .flags( kColor | kPacked )
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
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 0, 8, 16 )
      .stride( 24, 24, 24 )
      .samples( 1, 1, 1 );

    //! packed RGB 8:8:8, 24bpp, BGRBGR...
    set(kPixelFormatBGR24) = TraitsInit()
      .name( "BGR24" )
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 8, 8, 8 )
      .lshift( 16, 8, 0 )
      .stride( 24, 24, 24 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    set(kPixelFormatYUV422P) = TraitsInit()
      .name( "YUV422P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    set(kPixelFormatYUV444P) = TraitsInit()
      .name( "YUV444P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:1:0, 9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    set(kPixelFormatYUV410P) = TraitsInit()
      .name( "YUV410P" )
      .flags( kColor | kPlanar )
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
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 4 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

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
    set(kPixelFormatMONOBLACK) = TraitsInit()
      .name( "MONOBLACK" )
      .flags( kPlanar )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 1 )
      .lshift( 0 )
      .stride( 1 )
      .samples( 1 );

    //! 8 bit with kPixelFormatRGB32 palette
    set(kPixelFormatPAL8) = TraitsInit()
      .name( "PAL8" )
      .flags( kPlanar | kPaletted )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 8 )
      .lshift( 0 )
      .stride( 8 )
      .samples( 1 );

    //! packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    set(kPixelFormatUYVY422) = TraitsInit()
      .name( "UYVY422" )
      .flags( kColor | kPacked )
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
      .flags( kColor | kPacked )
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
      .flags( kColor | kPacked )
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
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 1, 2, 1 )
      .lshift( 3, 1, 0 )
      .stride( 4, 4, 4 )
      .samples( 1, 1, 1 );

    //! packed RGB 1:2:1, 8bpp, (msb)1B 2G 1R(lsb)
    set(kPixelFormatBGR4_BYTE) = TraitsInit()
      .name( "BGR4_BYTE" )
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 1, 2, 1 )
      .lshift( 7, 5, 4 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
    set(kPixelFormatRGB8) = TraitsInit()
      .name( "RGB8" )
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 3, 3, 2 )
      .lshift( 0, 3, 5 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! packed RGB 1:2:1 bitstream, 4bpp, (msb)1R 2G 1B(lsb), a byte
    //! contains two pixels, the first pixel in the byte is the one
    //! composed by the 4 msb bits
    set(kPixelFormatRGB4) = TraitsInit()
      .name( "RGB4" )
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 1, 2, 1 )
      .lshift( 0, 1, 3 )
      .stride( 4, 4, 4 )
      .samples( 1, 1, 1 );

    //! packed RGB 1:2:1, 8bpp, (msb)1R 2G 1B(lsb)
    set(kPixelFormatRGB4_BYTE) = TraitsInit()
      .name( "RGB4_BYTE" )
      .flags( kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 1, 2, 1 )
      .lshift( 4, 5, 7 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV
    //! channels, which are interleaved (first byte U and the
    //! following byte V)
    set(kPixelFormatNV12) = TraitsInit()
      .name( "NV12" )
      .flags( kColor | kPacked | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 1 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 8 )
      .stride( 8, 16, 16 )
      .samples( 1, 1, 1 );

    //! as above, but U and V bytes are swapped
    set(kPixelFormatNV21) = TraitsInit()
      .name( "NV21" )
      .flags( kColor | kPacked | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 1 )
      .depth( 8, 8, 8 )
      .lshift( 0, 8, 0 )
      .stride( 8, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    set(kPixelFormatARGB) = TraitsInit()
      .name( "ARGB" )
      .flags( kAlpha | kColor | kPacked )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 8, 16, 24, 0 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    //! packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    set(kPixelFormatRGBA) = TraitsInit()
      .name( "RGBA" )
      .flags( kAlpha | kColor | kPacked )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 0, 8, 16, 24 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    //! packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    set(kPixelFormatABGR) = TraitsInit()
      .name( "ABGR" )
      .flags( kAlpha | kColor | kPacked )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 24, 16, 8, 0 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    set(kPixelFormatBGRA) = TraitsInit()
      .name( "BGRA" )
      .flags( kAlpha | kColor | kPacked )
      .channels( 4 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0, 0 )
      .depth( 8, 8, 8, 8 )
      .lshift( 16, 8, 0, 24 )
      .stride( 32, 32, 32, 32 )
      .samples( 1, 1, 1, 1 );

    //! Y, 16bpp, big-endian
    set(kPixelFormatGRAY16BE) = TraitsInit()
      .name( "GRAY16BE" )
      .flags( kBE | kPlanar )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 16 )
      .lshift( 0 )
      .stride( 16 )
      .samples( 1 );

    //! Y, 16bpp, little-endian
    set(kPixelFormatGRAY16LE) = TraitsInit()
      .name( "GRAY16LE" )
      .flags( kLE | kPlanar )
      .channels( 1 )
      .chromaBoxW( 0 )
      .chromaBoxH( 0 )
      .plane( 0 )
      .depth( 16 )
      .lshift( 0 )
      .stride( 16 )
      .samples( 1 );

    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples)
    set(kPixelFormatYUV440P) = TraitsInit()
      .name( "YUV440P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
    //! samples)
    set(kPixelFormatYUVA420P) = TraitsInit()
      .name( "YUVA420P" )
      .flags( kAlpha | kColor | kPlanar )
      .channels( 4 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2, 3 )
      .depth( 8, 8, 8, 8 )
      .lshift( 0, 0, 0, 0 )
      .stride( 8, 8, 8, 8 )
      .samples( 1, 1, 1, 1 );

    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as big-endian
    set(kPixelFormatRGB48BE) = TraitsInit()
      .name( "RGB48BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 16, 16, 16 )
      .lshift( 0, 16, 32 )
      .stride( 48, 48, 48 )
      .samples( 1, 1, 1 );

    //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
    //! each R/G/B component is stored as little-endian
    set(kPixelFormatRGB48LE) = TraitsInit()
      .name( "RGB48LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 16, 16, 16 )
      .lshift( 0, 16, 32 )
      .stride( 48, 48, 48 )
      .samples( 1, 1, 1 );

    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
    set(kPixelFormatRGB565BE) = TraitsInit()
      .name( "RGB565BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 6, 5 )
      .lshift( 0, 5, 11 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
    set(kPixelFormatRGB565LE) = TraitsInit()
      .name( "RGB565LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 6, 5 )
      .lshift( 0, 5, 11 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
    //! most significant bit to 0
    set(kPixelFormatRGB555BE) = TraitsInit()
      .name( "RGB555BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 5, 5 )
      .lshift( 1, 6, 11 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
    //! most significant bit to 0
    set(kPixelFormatRGB555LE) = TraitsInit()
      .name( "RGB555LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 5, 5 )
      .lshift( 1, 6, 11 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
    set(kPixelFormatBGR565BE) = TraitsInit()
      .name( "BGR565BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 6, 5 )
      .lshift( 11, 5, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
    set(kPixelFormatBGR565LE) = TraitsInit()
      .name( "BGR565LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 6, 5 )
      .lshift( 11, 5, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
    //! most significant bit to 1
    set(kPixelFormatBGR555BE) = TraitsInit()
      .name( "BGR555BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 5, 5 )
      .lshift( 11, 6, 1 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
    //! most significant bit to 1
    set(kPixelFormatBGR555LE) = TraitsInit()
      .name( "BGR555LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 5, 5, 5 )
      .lshift( 11, 6, 1 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! little-endian
    set(kPixelFormatYUV420P16LE) = TraitsInit()
      .name( "YUV420P16LE" )
      .flags( kLE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
    //! big-endian
    set(kPixelFormatYUV420P16BE) = TraitsInit()
      .name( "YUV420P16BE" )
      .flags( kBE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! little-endian
    set(kPixelFormatYUV422P16LE) = TraitsInit()
      .name( "YUV422P16LE" )
      .flags( kLE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
    //! big-endian
    set(kPixelFormatYUV422P16BE) = TraitsInit()
      .name( "YUV422P16BE" )
      .flags( kBE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! little-endian
    set(kPixelFormatYUV444P16LE) = TraitsInit()
      .name( "YUV444P16LE" )
      .flags( kLE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
    //! big-endian
    set(kPixelFormatYUV444P16BE) = TraitsInit()
      .name( "YUV444P16BE" )
      .flags( kBE | kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 16, 16, 16 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
    //! most significant bits to 0
    set(kPixelFormatRGB444BE) = TraitsInit()
      .name( "RGB444BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 4, 4, 4 )
      .lshift( 4, 8, 12 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
    //! most significant bits to 0
    set(kPixelFormatRGB444LE) = TraitsInit()
      .name( "RGB444LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 4, 4, 4 )
      .lshift( 4, 8, 12 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
    //! most significant bits to 1
    set(kPixelFormatBGR444BE) = TraitsInit()
      .name( "BGR444BE" )
      .flags( kBE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 4, 4, 4 )
      .lshift( 12, 8, 4 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
    //! most significant bits to 1
    set(kPixelFormatBGR444LE) = TraitsInit()
      .name( "BGR444LE" )
      .flags( kLE | kColor | kPacked )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 0, 0 )
      .depth( 4, 4, 4 )
      .lshift( 12, 8, 4 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

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
    set(kPixelFormatYUVJ420P) = TraitsInit()
      .name( "YUVJ420P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    set(kPixelFormatYUVJ422P) = TraitsInit()
      .name( "YUVJ422P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    set(kPixelFormatYUVJ444P) = TraitsInit()
      .name( "YUVJ444P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples)
    set(kPixelFormatYUVJ440P) = TraitsInit()
      .name( "YUVJ440P" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 8, 8, 8 )
      .lshift( 0, 0, 0 )
      .stride( 8, 8, 8 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 9-bits per channel:
    set(kPixelFormatYUV420P9) = TraitsInit()
      .name( "YUV420P9" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 9, 9, 9 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 9-bits per channel:
    set(kPixelFormatYUV422P9) = TraitsInit()
      .name( "YUV422P9" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 9, 9, 9 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 9-bits per channel:
    set(kPixelFormatYUV444P9) = TraitsInit()
      .name( "YUV444P9" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 9, 9, 9 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:0, 10-bits per channel:
    set(kPixelFormatYUV420P10) = TraitsInit()
      .name( "YUV420P10" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 2 )
      .plane( 0, 1, 2 )
      .depth( 10, 10, 10 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:2:2, 10-bits per channel:
    set(kPixelFormatYUV422P10) = TraitsInit()
      .name( "YUV422P10" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 2 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 10, 10, 10 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

    //! planar YUV 4:4:4, 10-bits per channel:
    set(kPixelFormatYUV444P10) = TraitsInit()
      .name( "YUV444P10" )
      .flags( kColor | kPlanar )
      .channels( 3 )
      .chromaBoxW( 1 )
      .chromaBoxH( 1 )
      .plane( 0, 1, 2 )
      .depth( 10, 10, 10 )
      .lshift( 0, 0, 0 )
      .stride( 16, 16, 16 )
      .samples( 1, 1, 1 );

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
}
