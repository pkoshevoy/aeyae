// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <deque>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QTime>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#elif !defined(_WIN32)
#include <QtDBus/QtDBus>
#endif

// libass includes:
// #undef YAE_USE_LIBASS
#ifdef YAE_USE_LIBASS
extern "C"
{
#include <ass/ass.h>
}
#endif

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/thread/yae_threading.h"

// local includes:
#include <yaeCanvas.h>
#include <yaeUtilsQt.h>


//----------------------------------------------------------------
// yae_show_program_listing
//
static void
yae_show_program_listing(std::ostream & ostr,
                         const char * program,
                         std::size_t len = 0,
                         const char * errorMessage = NULL)
{
  if (!len && program)
  {
    len = strlen(program);
  }

  unsigned int lineNo = 0;
  char prev = '\n';
  const char * i = program;
  const char * end = program + len;
  while (i < end)
  {
    if (prev == '\n')
    {
      lineNo++;

      if (lineNo == 1 || (errorMessage && (lineNo % 20) == 1))
      {
        ostr << "\n        ";
        for (int i = 1; i < 80; i += 8)
        {
          ostr << std::left << std::setw(8) << std::setfill(' ') << i;
        }
        ostr << '\n';
      }

      ostr << std::left << std::setw(8) << std::setfill(' ') << lineNo;
    }

    ostr << *i;
    prev = *i;
    i++;
  }

  if (errorMessage)
  {
    ostr << '\n' << errorMessage << std::endl;
  }
}

//----------------------------------------------------------------
// yae_gl_arb_passthrough_2d
//
static const char * yae_gl_arb_passthrough_2d =
  "!!ARBfp1.0\n"
  "TEX result.color, fragment.texcoord[0], texture[0], 2D;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuv_to_rgb_2d
//
static const char * yae_gl_arb_yuv_to_rgb_2d =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"
  "TEMP yuv;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX yuv.y, fragment.texcoord[0], texture[1], 2D;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[2], 2D;\n"
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"
  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuva_to_rgba_2d
//
static const char * yae_gl_arb_yuva_to_rgba_2d =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"
  "TEMP yuv;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX yuv.y, fragment.texcoord[0], texture[1], 2D;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[2], 2D;\n"
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[3], 2D;\n"
  "MOV result.color.a, yuv.x;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_passthrough
//
static const char * yae_gl_arb_passthrough =
  "!!ARBfp1.0\n"
  "TEX result.color, fragment.texcoord[0], texture[0], RECT;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuv_to_rgb
//
static const char * yae_gl_arb_yuv_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"
  "PARAM subsample_uv = program.local[3];\n"
  "TEMP yuv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX yuv.y, coord_uv, texture[1], RECT;\n"
  "TEX yuv.z, coord_uv, texture[2], RECT;\n"
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"
  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuva_to_rgba
//
static const char * yae_gl_arb_yuva_to_rgba =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"
  "PARAM subsample_uv = program.local[3];\n"
  "TEMP yuv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX yuv.y, coord_uv, texture[1], RECT;\n"
  "TEX yuv.z, coord_uv, texture[2], RECT;\n"
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[3], RECT;\n"
  "MOV result.color.a, yuv.x;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuv_p10_to_rgb
//
static const char * yae_gl_arb_yuv_p10_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"
  "PARAM subsample_uv = program.local[3];\n"
  "TEMP yuv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX yuv.y, coord_uv, texture[1], RECT;\n"
  "TEX yuv.z, coord_uv, texture[2], RECT;\n"
  "MUL yuv, yuv, 64.0;\n"
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"
  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuyv_to_rgb_antialias
//
static const char * yae_gl_arb_yuyv_to_rgb_antialias =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"

  "TEMP t0;\n"
  "TEMP t1;\n"
  "TEMP tmp;\n"
  "TEMP x00;\n"
  "TEMP q00;\n"
  "TEMP q01;\n"
  "TEMP q02;\n"
  "TEMP q10;\n"
  "TEMP q11;\n"
  "TEMP q12;\n"
  "TEMP www;\n"
  "TEMP yuv;\n"

  "FLR x00,   fragment.texcoord[0];\n"
  "MAD t0,    x00.x, 0.5, 0.1;\n"
  "FRC t0,    t0;\n"
  "MUL t0,    t0, 2.0;\n"
  "FLR t0,    t0;\n"
  "SUB t1,    1.1, t0;\n"
  "FLR t1,    t1;\n"

  // sample texture data:
  "TEX q00,   x00, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 0, 0, 0 };\n"
  "TEX q01,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 2, 0, 0, 0 };\n"
  "TEX q02,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 0, 1, 0, 0 };\n"
  "TEX q10,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 1, 0, 0 };\n"
  "TEX q11,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 2, 1, 0, 0 };\n"
  "TEX q12,   tmp, texture[0], RECT;\n"

  // calculate interpolation weights:
  "TEMP w0;\n"
  "TEMP w1;\n"
  "FRC w0, fragment.texcoord[0];\n"
  "SUB w1, 1.0, w0;\n"

  "MOV tmp.x, q00.x;\n"
  "MUL tmp.y, q00.a, t1.x;\n"
  "MUL tmp.z, q00.a, t0.x;\n"
  "MAD tmp.y, q01.a, t0.x, tmp.y;\n"
  "MAD tmp.z, q01.a, t1.x, tmp.z;\n"
  "MUL www, w1.x, w1.y;\n"
  "MUL yuv, tmp, www;\n"

  "MOV tmp.x, q01.x;\n"
  "MUL tmp.y, q01.a, t0.x;\n"
  "MUL tmp.z, q01.a, t1.x;\n"
  "MAD tmp.y, q02.a, t1.x, tmp.y;\n"
  "MAD tmp.z, q02.a, t0.x, tmp.z;\n"
  "MUL www, w0.x, w1.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  "MOV tmp.x, q10.x;\n"
  "MUL tmp.y, q10.a, t1.x;\n"
  "MUL tmp.z, q10.a, t0.x;\n"
  "MAD tmp.y, q11.a, t0.x, tmp.y;\n"
  "MAD tmp.z, q11.a, t1.x, tmp.z;\n"
  "MUL www, w1.x, w0.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  "MOV tmp.x, q11.x;\n"
  "MUL tmp.y, q11.a, t0.x;\n"
  "MUL tmp.z, q11.a, t1.x;\n"
  "MAD tmp.y, q12.a, t1.x, tmp.y;\n"
  "MAD tmp.z, q12.a, t0.x, tmp.z;\n"
  "MUL www, w0.x, w0.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  // convert to RGB:
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"

  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_uyvy_to_rgb_antialias
//
static const char * yae_gl_arb_uyvy_to_rgb_antialias =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"

  "TEMP t0;\n"
  "TEMP t1;\n"
  "TEMP tmp;\n"
  "TEMP x00;\n"
  "TEMP q00;\n"
  "TEMP q01;\n"
  "TEMP q02;\n"
  "TEMP q10;\n"
  "TEMP q11;\n"
  "TEMP q12;\n"
  "TEMP www;\n"
  "TEMP yuv;\n"

  "FLR x00,   fragment.texcoord[0];\n"
  "MAD t0,    x00.x, 0.5, 0.1;\n"
  "FRC t0,    t0;\n"
  "MUL t0,    t0, 2.0;\n"
  "FLR t0,    t0;\n"
  "SUB t1,    1.1, t0;\n"
  "FLR t1,    t1;\n"

  // sample texture data:
  "TEX q00,   x00, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 0, 0, 0 };\n"
  "TEX q01,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 2, 0, 0, 0 };\n"
  "TEX q02,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 0, 1, 0, 0 };\n"
  "TEX q10,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 1, 0, 0 };\n"
  "TEX q11,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 2, 1, 0, 0 };\n"
  "TEX q12,   tmp, texture[0], RECT;\n"

  // calculate interpolation weights:
  "TEMP w0;\n"
  "TEMP w1;\n"
  "FRC w0, fragment.texcoord[0];\n"
  "SUB w1, 1.0, w0;\n"

  "MOV tmp.x, q00.a;\n"
  "MUL tmp.y, q00.x, t1.x;\n"
  "MUL tmp.z, q00.x, t0.x;\n"
  "MAD tmp.y, q01.x, t0.x, tmp.y;\n"
  "MAD tmp.z, q01.x, t1.x, tmp.z;\n"
  "MUL www, w1.x, w1.y;\n"
  "MUL yuv, tmp, www;\n"

  "MOV tmp.x, q01.a;\n"
  "MUL tmp.y, q01.x, t0.x;\n"
  "MUL tmp.z, q01.x, t1.x;\n"
  "MAD tmp.y, q02.x, t1.x, tmp.y;\n"
  "MAD tmp.z, q02.x, t0.x, tmp.z;\n"
  "MUL www, w0.x, w1.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  "MOV tmp.x, q10.a;\n"
  "MUL tmp.y, q10.x, t1.x;\n"
  "MUL tmp.z, q10.x, t0.x;\n"
  "MAD tmp.y, q11.x, t0.x, tmp.y;\n"
  "MAD tmp.z, q11.x, t1.x, tmp.z;\n"
  "MUL www, w1.x, w0.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  "MOV tmp.x, q11.a;\n"
  "MUL tmp.y, q11.x, t0.x;\n"
  "MUL tmp.z, q11.x, t1.x;\n"
  "MAD tmp.y, q12.x, t1.x, tmp.y;\n"
  "MAD tmp.z, q12.x, t0.x, tmp.z;\n"
  "MUL www, w0.x, w0.y;\n"
  "MAD yuv, tmp, www, yuv;\n"

  // convert to RGB:
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"

  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuyv_to_rgb
//
static const char * yae_gl_arb_yuyv_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"

  "TEMP t0;\n"
  "TEMP t1;\n"
  "TEMP tmp;\n"
  "TEMP x00;\n"
  "TEMP q00;\n"
  "TEMP q01;\n"
  "TEMP q10;\n"
  "TEMP q11;\n"
  "TEMP yuv;\n"

  "FLR x00,   fragment.texcoord[0];\n"
  "MAD t0,    x00.x, 0.5, 0.1;\n"
  "FRC t0,    t0;\n"
  "MUL t0,    t0, 2.0;\n"
  "FLR t0,    t0;\n"
  "SUB t1,    1.1, t0;\n"
  "FLR t1,    t1;\n"

  // sample texture data:
  "TEX q00,   x00, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 0, 0, 0 };\n"
  "TEX q01,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 0, 1, 0, 0 };\n"
  "TEX q10,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 1, 0, 0 };\n"
  "TEX q11,   tmp, texture[0], RECT;\n"

  "MOV yuv.x, q00.x;\n"
  "MUL yuv.y, q00.a, t1.x;\n"
  "MUL yuv.z, q00.a, t0.x;\n"
  "MAD yuv.y, q01.a, t0.x, yuv.y;\n"
  "MAD yuv.z, q01.a, t1.x, yuv.z;\n"

  // convert to RGB:
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"

  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_uyvy_to_rgb
//
static const char * yae_gl_arb_uyvy_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM vr = program.local[0];\n"
  "PARAM vg = program.local[1];\n"
  "PARAM vb = program.local[2];\n"

  "TEMP t0;\n"
  "TEMP t1;\n"
  "TEMP tmp;\n"
  "TEMP x00;\n"
  "TEMP q00;\n"
  "TEMP q01;\n"
  "TEMP q10;\n"
  "TEMP q11;\n"
  "TEMP yuv;\n"

  "FLR x00,   fragment.texcoord[0];\n"
  "MAD t0,    x00.x, 0.5, 0.1;\n"
  "FRC t0,    t0;\n"
  "MUL t0,    t0, 2.0;\n"
  "FLR t0,    t0;\n"
  "SUB t1,    1.1, t0;\n"
  "FLR t1,    t1;\n"

  // sample texture data:
  "TEX q00,   x00, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 0, 0, 0 };\n"
  "TEX q01,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 0, 1, 0, 0 };\n"
  "TEX q10,   tmp, texture[0], RECT;\n"
  "ADD tmp,   x00, { 1, 1, 0, 0 };\n"
  "TEX q11,   tmp, texture[0], RECT;\n"

  "MOV yuv.x, q00.a;\n"
  "MUL yuv.y, q00.x, t1.x;\n"
  "MUL yuv.z, q00.x, t0.x;\n"
  "MAD yuv.y, q01.x, t0.x, yuv.y;\n"
  "MAD yuv.z, q01.x, t1.x, yuv.z;\n"

  // convert to RGB:
  "DPH result.color.r, yuv, vr;\n"
  "DPH result.color.g, yuv, vg;\n"
  "DPH result.color.b, yuv, vb;\n"

  "MOV result.color.a, 1.0;\n"
  "END\n";

//----------------------------------------------------------------
// yae_is_opengl_extension_supported
//
bool
yae_is_opengl_extension_supported(const char * extension)
{
  static bool ready = false;
  static std::set<std::string> ext;

  // Extension names should not have spaces:
  GLubyte * found = (GLubyte *) ::strchr(extension, ' ');
  if (found || *extension == '\0')
  {
    return false;
  }

  if (!ready)
  {
    const GLubyte * extensions = glGetString(GL_EXTENSIONS);
    std::istringstream ss((const char *)extensions);
    std::copy(std::istream_iterator<std::string>(ss),
              std::istream_iterator<std::string>(),
              std::inserter(ext, ext.begin()));
    ready = true;
  }

  std::set<std::string>::const_iterator i = ext.find(std::string(extension));
  bool supported = (i != ext.end());
  return supported;
}

//----------------------------------------------------------------
// yae_to_opengl
//
unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes)
{
  shouldSwapBytes = GL_FALSE;

  switch (yaePixelFormat)
  {
    case yae::kPixelFormatYUYV422:
      //! packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    case yae::kPixelFormatUYVY422:
      //! packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1

      if (yae_is_opengl_extension_supported("GL_APPLE_ycbcr_422"))
      {
        internalFormat = 3;
        format = GL_YCBCR_422_APPLE;
#ifdef __BIG_ENDIAN__
        dataType =
          yaePixelFormat == yae::kPixelFormatYUYV422 ?
          GL_UNSIGNED_SHORT_8_8_APPLE :
          GL_UNSIGNED_SHORT_8_8_REV_APPLE;
#else
        dataType =
          yaePixelFormat == yae::kPixelFormatYUYV422 ?
          GL_UNSIGNED_SHORT_8_8_REV_APPLE :
          GL_UNSIGNED_SHORT_8_8_APPLE;
#endif
        return 3;
      }
      break;

    case yae::kPixelFormatYUV420P:
      //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    case yae::kPixelFormatYUV422P:
      //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    case yae::kPixelFormatYUV444P:
      //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    case yae::kPixelFormatYUV410P:
      //! planar YUV 4:1:0, 9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    case yae::kPixelFormatYUV411P:
      //! planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    case yae::kPixelFormatGRAY8:
      //! Y, 8bpp
    case yae::kPixelFormatPAL8:
      //! 8 bit with kPixelFormatRGB32 palette
    case yae::kPixelFormatNV12:
      //! planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV
      //! components, which are interleaved (first byte U and the
      //! following byte V)
    case yae::kPixelFormatNV21:
      //! as above, but U and V bytes are swapped
    case yae::kPixelFormatYUV440P:
      //! planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    case yae::kPixelFormatYUVA420P:
      //! planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A
      //! samples)
    case yae::kPixelFormatYUVJ420P:
      //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples), JPEG
    case yae::kPixelFormatYUVJ422P:
      //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ444P:
      //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ440P:
      //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples), JPEG

      internalFormat = GL_LUMINANCE;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_BYTE;
      return 1;

    case yae::kPixelFormatRGB24:
      //! packed RGB 8:8:8, 24bpp, RGBRGB...
      internalFormat = GL_RGB;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE;
      return 3;

    case yae::kPixelFormatBGR24:
      //! packed RGB 8:8:8, 24bpp, BGRBGR...
      internalFormat = 3;
      format = GL_BGR;
      dataType = GL_UNSIGNED_BYTE;
      return 3;

    case yae::kPixelFormatRGB8:
      //! packed RGB 3:3:2, 8bpp, (msb)3R 3G 2B(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_3_3_2;
      return 3;

    case yae::kPixelFormatBGR8:
      //! packed RGB 3:3:2, 8bpp, (msb)2B 3G 3R(lsb)
      internalFormat = GL_R3_G3_B2;
      format = GL_RGB;
      dataType = GL_UNSIGNED_BYTE_2_3_3_REV;
      return 3;

    case yae::kPixelFormatARGB:
      //! packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
      internalFormat = 4;
      format = GL_BGRA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#endif
      return 4;

    case yae::kPixelFormatRGBA:
      //! packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
      internalFormat = 4;
      format = GL_RGBA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      return 4;

    case yae::kPixelFormatABGR:
      //! packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
      internalFormat = 4;
      format = GL_RGBA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#endif
      return 4;

    case yae::kPixelFormatBGRA:
      //! packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
      internalFormat = 4;
      format = GL_BGRA;
#ifdef __BIG_ENDIAN__
      dataType = GL_UNSIGNED_INT_8_8_8_8;
#else
      dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
#endif
      return 4;

    case yae::kPixelFormatGRAY16BE:
      //! Y, 16bpp, big-endian
    case yae::kPixelFormatYUV420P16BE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV422P16BE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! big-endian
    case yae::kPixelFormatYUV444P16BE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;

    case yae::kPixelFormatGRAY16LE:
      //! Y, 16bpp, little-endian
    case yae::kPixelFormatYUV420P16LE:
      //! planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV422P16LE:
      //! planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples),
      //! little-endian
    case yae::kPixelFormatYUV444P16LE:
      //! planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples),
      //! little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;

    case yae::kPixelFormatRGB48BE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB16;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT;
      return 3;

    case yae::kPixelFormatRGB48LE:
      //! packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for
      //! each R/G/B component is stored as little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB16;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT;
      return 3;

    case yae::kPixelFormatRGB565BE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5;
      return 3;

    case yae::kPixelFormatRGB565LE:
      //! packed RGB 5:6:5, 16bpp, (msb) 5R 6G 5B(lsb), little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5;
      return 3;

    case yae::kPixelFormatBGR565BE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), big-endian
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5_REV;
      return 3;

    case yae::kPixelFormatBGR565LE:
      //! packed BGR 5:6:5, 16bpp, (msb) 5B 6G 5R(lsb), little-endian
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGB;
      dataType = GL_UNSIGNED_SHORT_5_6_5_REV;
      return 3;

    case yae::kPixelFormatRGB555BE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), big-endian,
      //! most significant bit to 0
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;

    case yae::kPixelFormatRGB555LE:
      //! packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), little-endian,
      //! most significant bit to 0
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;

    case yae::kPixelFormatBGR555BE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), big-endian,
      //! most significant bit to 1
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;

    case yae::kPixelFormatBGR555LE:
      //! packed BGR 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), little-endian,
      //! most significant bit to 1
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = 3;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      return 3;

    case yae::kPixelFormatRGB444BE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), big-endian,
      //! most significant bits to 0
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;

    case yae::kPixelFormatRGB444LE:
      //! packed RGB 4:4:4, 16bpp, (msb)4A 4R 4G 4B(lsb), little-endian,
      //! most significant bits to 0
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_BGRA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;

    case yae::kPixelFormatBGR444BE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), big-endian,
      //! most significant bits to 1
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;

    case yae::kPixelFormatBGR444LE:
      //! packed BGR 4:4:4, 16bpp, (msb)4A 4B 4G 4R(lsb), little-endian,
      //! most significant bits to 1
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_RGB4;
      format = GL_RGBA;
      dataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      return 3;

    case yae::kPixelFormatY400A:
      //! 8bit gray, 8bit alpha
      internalFormat = GL_LUMINANCE8_ALPHA8;
      format = GL_LUMINANCE_ALPHA;
      dataType = GL_UNSIGNED_BYTE;
      return 2;

    default:
      break;
  }

  return 0;
}

//----------------------------------------------------------------
// yae_assert_gl_no_error
//
static bool
yae_assert_gl_no_error()
{
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
  {
    return true;
  }

  std::cerr << "glGetError: " << err << std::endl;
  YAE_ASSERT(false);
  return false;
}

//----------------------------------------------------------------
// load_arb_program_natively
//
static bool
load_arb_program_natively(GLenum target, const char * prog)
{
  std::size_t len = strlen(prog);
  glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)len, prog);
  GLenum err = glGetError();
  (void)err;

  GLint errorPos = -1;
  glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);

#if !defined(NDEBUG)
  if (errorPos < (GLint)len && errorPos >= 0)
  {
    const GLubyte * err = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
    yae_show_program_listing(std::cerr, prog, len, (const char *)err);
  }
#endif

  GLint isNative = 0;
  glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

  if (errorPos == -1 &&
      isNative == 1)
  {
    return true;
  }

  return false;
}

namespace yae
{

  //----------------------------------------------------------------
  // TGLSaveState
  //
  struct TGLSaveState
  {
    TGLSaveState(GLbitfield mask):
      applied_(false)
    {
      glPushAttrib(mask);
      applied_ = yae_assert_gl_no_error();
    }

    ~TGLSaveState()
    {
      if (applied_)
      {
        glPopAttrib();
      }
    }

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveClientState
  //
  struct TGLSaveClientState
  {
    TGLSaveClientState(GLbitfield mask):
      applied_(false)
    {
      glPushClientAttrib(mask);
      applied_ = yae_assert_gl_no_error();
    }

    ~TGLSaveClientState()
    {
      if (applied_)
      {
        glPopClientAttrib();
      }
    }

  protected:
    bool applied_;
  };

  //----------------------------------------------------------------
  // TGLSaveMatrixState
  //
  struct TGLSaveMatrixState
  {
    TGLSaveMatrixState(GLenum mode):
      matrixMode_(mode)
    {
      glMatrixMode(matrixMode_);
      glPushMatrix();
    }

    ~TGLSaveMatrixState()
    {
      glMatrixMode(matrixMode_);
      glPopMatrix();
    }

  protected:
    GLenum matrixMode_;
  };

  //----------------------------------------------------------------
  // powerOfTwoLEQ
  //
  // calculate largest power-of-two less then or equal to the given
  //
  template <typename TScalar>
  inline static TScalar
  powerOfTwoLEQ(const TScalar & given)
  {
    const std::size_t n = sizeof(given) * 8;
    TScalar smaller = TScalar(0);
    TScalar closest = TScalar(1);
    for (std::size_t i = 0; (i < n) && (closest <= given); i++)
    {
      smaller = closest;
      closest *= TScalar(2);
    }

    return smaller;
  }

  //----------------------------------------------------------------
  // TMakeCurrentContext
  //
  struct TMakeCurrentContext
  {
    TMakeCurrentContext(QGLWidget * canvas):
      canvas_(canvas)
    {
      canvas_->makeCurrent();
    }

    ~TMakeCurrentContext()
    {
      canvas_->doneCurrent();
    }

    QGLWidget * canvas_;
  };


  //----------------------------------------------------------------
  // TFragmentShaderProgram::TFragmentShaderProgram
  //
  TFragmentShaderProgram::TFragmentShaderProgram(const char * code):
    code_(code),
    handle_(0)
  {}

  //----------------------------------------------------------------
  // TFragmentShaderProgram::destroy
  //
  void
  TFragmentShaderProgram::destroy()
  {
    if (handle_)
    {
      glDeleteProgramsARB(1, &handle_);
      handle_ = 0;
    }
  }


  //----------------------------------------------------------------
  // TFragmentShader::TFragmentShader
  //
  TFragmentShader::TFragmentShader(const TFragmentShaderProgram * program,
                                   TPixelFormatId format):
    program_(program),
    numPlanes_(0)
  {
    memset(stride_, 0, sizeof(stride_));
    memset(subsample_x_, 1, sizeof(subsample_x_));
    memset(subsample_y_, 1, sizeof(subsample_y_));
    memset(internalFormatGL_, 0, sizeof(internalFormatGL_));
    memset(pixelFormatGL_, 0, sizeof(pixelFormatGL_));
    memset(dataTypeGL_, 0, sizeof(dataTypeGL_));
    memset(magFilterGL_, 0, sizeof(magFilterGL_));
    memset(minFilterGL_, 0, sizeof(minFilterGL_));
    memset(shouldSwapBytes_, 0, sizeof(shouldSwapBytes_));

    const pixelFormat::Traits * ptts = pixelFormat::getTraits(format);
    if (ptts)
    {
      // figure out how many texture objects this format requires
      // per sample planes:

      // build a histogram:
      unsigned char nchannels[4] = { 0 };
      for (unsigned char i = 0; i < ptts->channels_; i++)
      {
        nchannels[ptts->plane_[i]]++;
      }

      // count contributing histogram bins, calculate sample stride:
      numPlanes_ = 0;
      for (unsigned char channel = 0, i = 0; i < ptts->channels_; i++)
      {
        if (!nchannels[i])
        {
          continue;
        }

        stride_[numPlanes_] = ptts->stride_[channel];
        unsigned char stride_bytes = stride_[numPlanes_] / 8;

        const bool nativeEndian =
          (ptts->flags_ & pixelFormat::kNativeEndian);

        if (stride_bytes == 1 && ptts->depth_[0] == 8)
        {
          internalFormatGL_[numPlanes_] = (GLint)GL_LUMINANCE;
          pixelFormatGL_   [numPlanes_] = (GLenum)GL_LUMINANCE;
          dataTypeGL_      [numPlanes_] = (GLenum)GL_UNSIGNED_BYTE;
          shouldSwapBytes_ [numPlanes_] = (GLint)0;
        }
        else if (stride_bytes == 2 && ptts->depth_[0] == 8)
        {
          internalFormatGL_[numPlanes_] = (GLint)GL_LUMINANCE8_ALPHA8;
          pixelFormatGL_   [numPlanes_] = (GLenum)GL_LUMINANCE_ALPHA;
          dataTypeGL_      [numPlanes_] = (GLenum)GL_UNSIGNED_BYTE;
          shouldSwapBytes_ [numPlanes_] = (GLint)0;
        }
        else if (stride_bytes == 2 && ptts->depth_[0] > 8)
        {
          internalFormatGL_[numPlanes_] = (GLint)GL_LUMINANCE16;
          pixelFormatGL_   [numPlanes_] = (GLenum)GL_LUMINANCE;
          dataTypeGL_      [numPlanes_] = (GLenum)GL_UNSIGNED_SHORT;
          shouldSwapBytes_ [numPlanes_] = (GLint)(nativeEndian ? 0 : 1);
        }
        else if (program)
        {
          unsigned int supportedChannels =
            yae_to_opengl(format,
                          internalFormatGL_[numPlanes_],
                          pixelFormatGL_   [numPlanes_],
                          dataTypeGL_      [numPlanes_],
                          shouldSwapBytes_ [numPlanes_]);
          YAE_ASSERT(supportedChannels == nchannels[i]);
        }

        if (ptts->flags_ & pixelFormat::kYUV && nchannels[i] > 1)
        {
          // YUYV, UYVY, NV12, NV21 should avoid linear filtering,
          // it blends U and V channels inappropriately;
          // the fragment shader should perform the antialising
          // instead, after YUV -> RGB conversion:
          magFilterGL_[numPlanes_] = GL_NEAREST;
          minFilterGL_[numPlanes_] = GL_NEAREST;
        }
        else
        {
          magFilterGL_[numPlanes_] = GL_LINEAR;
          minFilterGL_[numPlanes_] = GL_LINEAR;
        }

        channel += nchannels[i];
        numPlanes_++;
      }

      // consider chroma (UV) plane(s) sub-sampling:
      if ((ptts->flags_ & pixelFormat::kYUV) &&
          (ptts->flags_ & pixelFormat::kPlanar) &&
          (ptts->chromaBoxW_ > 1 ||
           ptts->chromaBoxH_ > 1))
      {
        for (unsigned char i = nchannels[0]; i < 3 && i < ptts->channels_; )
        {
          subsample_x_[i] = ptts->chromaBoxW_;
          subsample_y_[i] = ptts->chromaBoxH_;
          i += nchannels[i];
        }
      }
    }
  }

  //----------------------------------------------------------------
  // configure_builtin_shader
  //
  static unsigned int
  configure_builtin_shader(TFragmentShader & builtinShader,
                           TPixelFormatId yaePixelFormat)
  {
    const TFragmentShaderProgram * shaderProgram = builtinShader.program_;
    builtinShader = TFragmentShader(shaderProgram, yaePixelFormat);

    unsigned int supportedChannels =
      yae_to_opengl(yaePixelFormat,
                    builtinShader.internalFormatGL_[0],
                    builtinShader.pixelFormatGL_[0],
                    builtinShader.dataTypeGL_[0],
                    builtinShader.shouldSwapBytes_[0]);

    // restrict to a single texture object:
    builtinShader.numPlanes_ = 1;
    builtinShader.magFilterGL_[0] = GL_LINEAR;
    builtinShader.minFilterGL_[0] = GL_LINEAR;

    return supportedChannels;
  }

  //----------------------------------------------------------------
  // TBaseCanvas
  //
  class TBaseCanvas
  {
  public:
    TBaseCanvas():
      dar_(0.0),
      darCropped_(0.0),
      skipColorConverter_(false),
      verticalScalingEnabled_(false),
      shader_(NULL)
    {
      double identity[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
      };
      memcpy(m34_to_rgb_, identity, sizeof(m34_to_rgb_));
    }

    virtual ~TBaseCanvas()
    {
      destroyFragmentShaders();
      builtinShaderProgram_.destroy();
    }

    virtual void createFragmentShaders() = 0;

    virtual void clear(QGLWidget * canvas) = 0;

    virtual bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & f) = 0;

    virtual void draw() = 0;

    // helper:
    inline const pixelFormat::Traits * pixelTraits() const
    {
      return (frame_ ?
              pixelFormat::getTraits(frame_->traits_.pixelFormat_) :
              NULL);
    }

    void skipColorConverter(QGLWidget * canvas, bool enable)
    {
      if (skipColorConverter_ == enable)
      {
        return;
      }

      TVideoFramePtr frame;
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        frame = frame_;
        frame_.reset();
      }

      skipColorConverter_ = enable;

      if (frame)
      {
        loadFrame(canvas, frame);
      }
    }

    void enableVerticalScaling(bool enable)
    {
      verticalScalingEnabled_ = enable;
    }

    bool getCroppedFrame(TCropFrame & crop) const
    {
      if (!frame_)
      {
        return false;
      }

      if (crop_.isEmpty())
      {
        const VideoTraits & vtts = frame_->traits_;

        crop.x_ = vtts.offsetLeft_;
        crop.y_ = vtts.offsetTop_;
        crop.w_ = vtts.visibleWidth_;
        crop.h_ = vtts.visibleHeight_;

        if (darCropped_)
        {
          double par = (vtts.pixelAspectRatio_ != 0.0 &&
                        vtts.pixelAspectRatio_ != 1.0 ?
                        vtts.pixelAspectRatio_ : 1.0);

          double dar = double(par * crop.w_) / double(crop.h_);

          if (dar < darCropped_)
          {
            // adjust height:
            int h = int(0.5 + double(par * crop.w_) / darCropped_);
            crop.y_ += (crop.h_ - h) / 2;
            crop.h_ = h;
          }
          else
          {
            // adjust width:
            int w = int(0.5 + double(crop.h_ / par) * darCropped_);
            crop.x_ += (crop.w_ - w) / 2;
            crop.w_ = w;
          }
        }
      }
      else
      {
        crop = crop_;
      }

      return true;
    }

    bool imageWidthHeight(double & w, double & h) const
    {
      TCropFrame crop;
      if (getCroppedFrame(crop))
      {
        // video traits shortcut:
        const VideoTraits & vtts = frame_->traits_;

        w = crop.w_;
        h = crop.h_;

        if (!verticalScalingEnabled_)
        {
          if (dar_ != 0.0)
          {
            w = floor(0.5 + dar_ * h);
          }
          else if (vtts.pixelAspectRatio_ != 0.0)
          {
            w = floor(0.5 + w * vtts.pixelAspectRatio_);
          }
        }
        else
        {
          if (dar_ != 0.0)
          {
            double wh = w / h;

            if (dar_ > wh)
            {
              w = floor(0.5 + dar_ * h);
            }
            else if (dar_ < wh)
            {
              h = floor(0.5 + w / dar_);
            }
          }
          else if (vtts.pixelAspectRatio_ > 1.0)
          {
            w = floor(0.5 + w * vtts.pixelAspectRatio_);
          }
          else if (vtts.pixelAspectRatio_ < 1.0)
          {
            h = floor(0.5 + h / vtts.pixelAspectRatio_);
          }
        }

        return true;
      }

      return false;
    }

    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const
    {
      if (imageWidthHeight(w, h))
      {
        // video traits shortcut:
        const VideoTraits & vtts = frame_->traits_;

        if (vtts.cameraRotation_ % 90 == 0)
        {
          // must be a camera phone video that needs to be
          // rotated for viewing:
          if (vtts.cameraRotation_ % 180 != 0)
          {
            std::swap(w, h);
          }

          rotate = vtts.cameraRotation_;
        }
        else
        {
          rotate = 0;
        }

        return true;
      }

      return false;
    }

    inline void overrideDisplayAspectRatio(double dar)
    {
      dar_ = dar;
    }

    inline void cropFrame(double darCropped)
    {
      crop_.clear();
      darCropped_ = darCropped;
    }

    inline void cropFrame(const TCropFrame & crop)
    {
      darCropped_ = 0.0;
      crop_ = crop;
    }

    inline void getFrame(TVideoFramePtr & frame) const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      frame = frame_;
    }

    // helper:
    const TFragmentShader *
    fragmentShaderFor(TPixelFormatId format) const
    {
      if (skipColorConverter_)
      {
        return NULL;
      }

      std::map<TPixelFormatId, TFragmentShader>::const_iterator
        found = shaders_.find(format);

#if !defined(NDEBUG)
      // for debugging only:
      {
        const pixelFormat::Traits * ptts = pixelFormat::getTraits(format);
        std::cerr << "\n" << ptts->name_ << " FRAGMENT SHADER:";
        if (found != shaders_.end())
        {
          std::cerr << '\n';
          yae_show_program_listing(std::cerr, found->second.program_->code_);
          std::cerr << std::endl;
        }
        else
        {
          std::cerr << " NOT FOUND" << std::endl;
        }
      }
#endif

      if (found != shaders_.end())
      {
        return &(found->second);
      }

      return NULL;
    }

  protected:
    // helper:
    const TFragmentShader *
    findSomeShaderFor(TPixelFormatId format) const
    {
      const TFragmentShader * shader = fragmentShaderFor(format);
      if (!shader && builtinShader_.program_)
      {
        shader = &builtinShader_;
#if !defined(NDEBUG)
        std::cerr << "WILL USE PASS-THROUGH SHADER" << std::endl;
#endif
      }

      return shader;
    }

    // helper:
    void destroyFragmentShaders()
    {
      shader_ = NULL;
      shaders_.clear();

      while (!shaderPrograms_.empty())
      {
        TFragmentShaderProgram & program = shaderPrograms_.front();
        program.destroy();
        shaderPrograms_.pop_front();
      }
    }

    // helper:
    bool createBuiltinFragmentShader(const char * code)
    {
      bool ok = false;
      builtinShaderProgram_.destroy();
      builtinShaderProgram_.code_ = code;

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glGenProgramsARB(1, &builtinShaderProgram_.handle_);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                       builtinShaderProgram_.handle_);

      if (load_arb_program_natively(GL_FRAGMENT_PROGRAM_ARB,
                                    builtinShaderProgram_.code_))
      {
        builtinShader_.program_ = &builtinShaderProgram_;
        ok = true;
      }
      else
      {
        glDeleteProgramsARB(1, &builtinShaderProgram_.handle_);
        builtinShaderProgram_.handle_ = 0;
        builtinShader_.program_ = NULL;
      }
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
      return ok;
    }

    // helper:
    bool createFragmentShadersFor(const TPixelFormatId * formats,
                                  const std::size_t numFormats,
                                  const char * code)
    {
      bool ok = false;
      TFragmentShaderProgram program(code);

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glGenProgramsARB(1, &program.handle_);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program.handle_);

      if (load_arb_program_natively(GL_FRAGMENT_PROGRAM_ARB, program.code_))
      {
        shaderPrograms_.push_back(program);

        const TFragmentShaderProgram *
          shaderProgram = &(shaderPrograms_.back());

        for (std::size_t i = 0; i < numFormats; i++)
        {
          TPixelFormatId format = formats[i];
          shaders_[format] = TFragmentShader(shaderProgram, format);
        }

        ok = true;
      }
      else
      {
        glDeleteProgramsARB(1, &program.handle_);
        program.handle_ = 0;
      }

      glDisable(GL_FRAGMENT_PROGRAM_ARB);
      return ok;
    }

    // helper:
    inline bool setFrame(const TVideoFramePtr & frame,
                         bool & colorSpaceOrRangeChanged)
    {
      // NOTE: this assumes that the mutex is already locked:
      bool frameSizeOrFormatChanged = false;

      colorSpaceOrRangeChanged =
        (!frame_ || !frame ||
         !frame_->traits_.sameColorSpaceAndRange(frame->traits_));

      if (!frame_ || !frame ||
          !frame_->traits_.sameFrameSizeAndFormat(frame->traits_))
      {
        crop_.clear();
        frameSizeOrFormatChanged = true;
        colorSpaceOrRangeChanged = true;
      }

      frame_ = frame;
      return frameSizeOrFormatChanged;
    }

    mutable boost::mutex mutex_;
    TVideoFramePtr frame_;
    TCropFrame crop_;
    double dar_;
    double darCropped_;
    bool skipColorConverter_;
    bool verticalScalingEnabled_;

    TFragmentShaderProgram builtinShaderProgram_;
    TFragmentShader builtinShader_;

    std::list<TFragmentShaderProgram> shaderPrograms_;
    std::map<TPixelFormatId, TFragmentShader> shaders_;

    // shader selected for current frame:
    const TFragmentShader * shader_;

    // 3x4 matrix for color conversion to full-range RGB,
    // including luma scale and shift:
    double m34_to_rgb_[12];
  };

  //----------------------------------------------------------------
  // TModernCanvas
  //
  struct TModernCanvas : public TBaseCanvas
  {
    // virtual:
    void createFragmentShaders();

    // virtual:
    void clear(QGLWidget * canvas);

    // virtual:
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);

    // virtual:
    void draw();

  protected:
    std::vector<GLuint> texId_;
  };

  //----------------------------------------------------------------
  // TModernCanvas::createFragmentShaders
  //
  void
  TModernCanvas::createFragmentShaders()
  {
    if (!shaderPrograms_.empty())
    {
      // avoid re-creating duplicate shaders:
      YAE_ASSERT(false);
      return;
    }

    // for YUV formats:
    static const TPixelFormatId yuv[] = {
      kPixelFormatYUV420P,
      kPixelFormatYUV422P,
      kPixelFormatYUV444P,
      kPixelFormatYUV410P,
      kPixelFormatYUV411P,
      kPixelFormatYUV440P,
      kPixelFormatYUVJ420P,
      kPixelFormatYUVJ422P,
      kPixelFormatYUVJ444P,
      kPixelFormatYUVJ440P
    };

    createFragmentShadersFor(yuv, sizeof(yuv) / sizeof(yuv[0]),
                             yae_gl_arb_yuv_to_rgb);

    // for YUVA formats:
    static const TPixelFormatId yuva[] = {
      kPixelFormatYUVA420P
    };

    createFragmentShadersFor(yuva, sizeof(yuva) / sizeof(yuva[0]),
                             yae_gl_arb_yuva_to_rgba);

    // for YUVP10 formats:
    static const TPixelFormatId yuv_p10[] = {
      kPixelFormatYUV420P10,
      kPixelFormatYUV422P10,
      kPixelFormatYUV444P10
    };

    createFragmentShadersFor(yuv_p10, sizeof(yuv_p10) / sizeof(yuv_p10[0]),
                             yae_gl_arb_yuv_p10_to_rgb);

    // for YUYV formats:
    static const TPixelFormatId yuyv[] = {
      kPixelFormatYUYV422
    };

    if (!createFragmentShadersFor(yuyv, sizeof(yuyv) / sizeof(yuyv[0]),
                                  yae_gl_arb_yuyv_to_rgb_antialias))
    {
      // perhaps the anti-aliased program was too much for this GPU,
      // try one witnout anti-aliasing:
      createFragmentShadersFor(yuyv, sizeof(yuyv) / sizeof(yuyv[0]),
                               yae_gl_arb_yuyv_to_rgb);
    }

    // for UYVY formats:
    static const TPixelFormatId uyvy[] = {
      kPixelFormatUYVY422
    };

    if (!createFragmentShadersFor(uyvy, sizeof(uyvy) / sizeof(uyvy[0]),
                                  yae_gl_arb_uyvy_to_rgb_antialias))
    {
      // perhaps the anti-aliased program was too much for this GPU,
      // try one witnout anti-aliasing:
      createFragmentShadersFor(uyvy, sizeof(uyvy) / sizeof(uyvy[0]),
                               yae_gl_arb_uyvy_to_rgb);
    }

    // for natively supported formats:
    createBuiltinFragmentShader(yae_gl_arb_passthrough);
  }

  //----------------------------------------------------------------
  // alignmentFor
  //
  inline static int
  alignmentFor(std::size_t rowBytes)
  {
    int a =
      !(rowBytes % 8) ? 8 :
      !(rowBytes % 4) ? 4 :
      !(rowBytes % 2) ? 2 :
      1;
    return a;
  }

  //----------------------------------------------------------------
  // alignmentFor
  //
  inline static int
  alignmentFor(const unsigned char * data, std::size_t rowBytes)
  {
    int a = alignmentFor((std::size_t)data);
    int b = alignmentFor(rowBytes);
    return a < b ? a : b;
  }

  //----------------------------------------------------------------
  // TModernCanvas::clear
  //
  void
  TModernCanvas::clear(QGLWidget * canvas)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    if (!texId_.empty())
    {
      glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
      texId_.clear();
    }

    dar_ = 0.0;
    darCropped_ = 0.0;
    crop_.clear();
    frame_ = TVideoFramePtr();
  }

  //----------------------------------------------------------------
  // TModernCanvas::loadFrame
  //
  bool
  TModernCanvas::loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame)
  {
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    if (!ptts)
    {
      // don't know how to handle this pixel format:
      return false;
    }

    unsigned int supportedChannels =
      configure_builtin_shader(builtinShader_, vtts.pixelFormat_);

    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    // take the new frame:
    bool colorSpaceOrRangeChanged = false;
    bool frameSizeOrFormatChanged = setFrame(frame, colorSpaceOrRangeChanged);

    // setup new texture objects:
    if (frameSizeOrFormatChanged)
    {
      if (!texId_.empty())
      {
        glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
        texId_.clear();
      }

      shader_ = findSomeShaderFor(vtts.pixelFormat_);

      if (!supportedChannels && !shader_)
      {
        return false;
      }

      const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;

      texId_.resize(shader.numPlanes_);
      glGenTextures((GLsizei)(texId_.size()), &(texId_.front()));

      glEnable(GL_TEXTURE_RECTANGLE_ARB);
      for (std::size_t i = 0; i < shader.numPlanes_; i++)
      {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]);

#ifdef __APPLE__
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                        GL_TEXTURE_STORAGE_HINT_APPLE,
                        GL_STORAGE_CACHED_APPLE);
#endif

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                        GL_TEXTURE_MAG_FILTER,
                        shader.magFilterGL_[i]);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                        GL_TEXTURE_MIN_FILTER,
                        shader.minFilterGL_[i]);
        yae_assert_gl_no_error();

        TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
        {
          glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
                       0, // always level-0 for GL_TEXTURE_RECTANGLE_ARB
                       shader.internalFormatGL_[i],
                       vtts.encodedWidth_ / shader.subsample_x_[i],
                       vtts.encodedHeight_ / shader.subsample_y_[i],
                       0, // border width
                       shader.pixelFormatGL_[i],
                       shader.dataTypeGL_[i],
                       NULL);
          yae_assert_gl_no_error();
        }
      }
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    if (!supportedChannels && !shader_)
    {
      return false;
    }

    // upload texture data:
    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    {
      glEnable(GL_TEXTURE_RECTANGLE_ARB);

      for (std::size_t i = 0; i < shader.numPlanes_; i++)
      {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]);

        glPixelStorei(GL_UNPACK_SWAP_BYTES, shader.shouldSwapBytes_[i]);

        const unsigned char * data = frame->data_->data(i);
        std::size_t rowSize =
          frame->data_->rowBytes(i) / (shader.stride_[i] / 8);
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignmentFor(data, rowSize));
        glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(rowSize));
        yae_assert_gl_no_error();

        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
                     0, // always level-0 for GL_TEXTURE_RECTANGLE_ARB
                     shader.internalFormatGL_[i],
                     vtts.encodedWidth_ / shader.subsample_x_[i],
                     vtts.encodedHeight_ / shader.subsample_y_[i],
                     0, // border width
                     shader.pixelFormatGL_[i],
                     shader.dataTypeGL_[i],
                     data);
        yae_assert_gl_no_error();
      }
      glDisable(GL_TEXTURE_RECTANGLE_ARB);
    }

    if (shader_)
    {
      if (colorSpaceOrRangeChanged)
      {
        init_abc_to_rgb_matrix(&m34_to_rgb_[0], vtts);
      }

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_->program_->handle_);
      {
        // pass the color transform matrix to the shader:
        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      0, &m34_to_rgb_[0]);
        yae_assert_gl_no_error();

        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      1, &m34_to_rgb_[4]);
        yae_assert_gl_no_error();

        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      2, &m34_to_rgb_[8]);
        yae_assert_gl_no_error();

        // pass the subsampling factors to the shader:
        GLdouble subsample_uv[4] = { 1.0 };
        subsample_uv[0] = 1.0 / double(ptts->chromaBoxW_);
        subsample_uv[1] = 1.0 / double(ptts->chromaBoxH_);

        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      3, subsample_uv);
        yae_assert_gl_no_error();
      }
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

    return true;
  }

  //----------------------------------------------------------------
  // TModernCanvas::draw
  //
  void
  TModernCanvas::draw()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (texId_.empty())
    {
      return;
    }

    double w = 0.0;
    double h = 0.0;
    imageWidthHeight(w, h);

    TCropFrame crop;
    getCroppedFrame(crop);

    if (glActiveTexture)
    {
      glActiveTexture(GL_TEXTURE0);
      yae_assert_gl_no_error();
    }

    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.f, 1.f, 1.f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (shader_)
    {
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
    }

    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    const std::size_t numTextures = texId_.size();
    for (std::size_t i = 0; i < numTextures; i += shader.numPlanes_)
    {
      if (shader_)
      {
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_->program_->handle_);
      }

      if (glActiveTexture)
      {
        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          glActiveTexture((GLenum)(GL_TEXTURE0 + k));
          yae_assert_gl_no_error();

          glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[k + i]);
        }
      }
      else
      {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]);
      }

      glBegin(GL_QUADS);
      {
        glTexCoord2i(crop.x_, crop.y_);
        glVertex2i(0, 0);

        glTexCoord2i(crop.x_ + crop.w_, crop.y_);
        glVertex2i(int(w), 0);

        glTexCoord2i(crop.x_ + crop.w_, crop.y_ + crop.h_);
        glVertex2i(int(w), int(h));

        glTexCoord2i(crop.x_, crop.y_ + crop.h_);
        glVertex2i(0, int(h));
      }
      glEnd();
    }

    // un-bind the textures:
    if (glActiveTexture)
    {
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        glActiveTexture((GLenum)(GL_TEXTURE0 + k));
        yae_assert_gl_no_error();

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
      }
    }
    else
    {
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }

    if (shader_)
    {
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

    glDisable(GL_TEXTURE_RECTANGLE_ARB);
  }

  //----------------------------------------------------------------
  // TEdge
  //
  struct TEdge
  {
    // texture:
    GLsizei offset_;
    GLsizei extent_;
    GLsizei length_;

    // padding:
    GLsizei v0_;
    GLsizei v1_;

    // texture coordinates:
    GLdouble t0_;
    GLdouble t1_;
  };

  //----------------------------------------------------------------
  // TFrameTile
  //
  struct TFrameTile
  {
    TEdge x_;
    TEdge y_;
  };

  //----------------------------------------------------------------
  // TLegacyCanvas
  //
  // This is a subclass implementing frame rendering on OpenGL
  // hardware that doesn't support GL_EXT_texture_rectangle
  //
  struct TLegacyCanvas : public TBaseCanvas
  {
    TLegacyCanvas();

    // virtual:
    void createFragmentShaders();

    // virtual:
    void clear(QGLWidget * canvas);

    // virtual:
    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame);

    // virtual:
    void draw();

  protected:
    // unpadded image dimensions:
    GLsizei w_;
    GLsizei h_;

    std::vector<TFrameTile> tiles_;
    std::vector<GLuint> texId_;
  };

  //----------------------------------------------------------------
  // TLegacyCanvas::TLegacyCanvas
  //
  TLegacyCanvas::TLegacyCanvas():
    TBaseCanvas(),
    w_(0),
    h_(0)
  {}

  //----------------------------------------------------------------
  // TLegacyCanvas::createFragmentShaders
  //
  void
  TLegacyCanvas::createFragmentShaders()
  {
    if (!shaderPrograms_.empty())
    {
      // avoid re-creating duplicate shaders:
      YAE_ASSERT(false);
      return;
    }

    // for YUV formats:
    static const TPixelFormatId yuv[] = {
      kPixelFormatYUV420P,
      kPixelFormatYUV422P,
      kPixelFormatYUV444P,
      kPixelFormatYUV410P,
      kPixelFormatYUV411P,
      kPixelFormatYUV440P,
      kPixelFormatYUVJ420P,
      kPixelFormatYUVJ422P,
      kPixelFormatYUVJ444P,
      kPixelFormatYUVJ440P
    };

    createFragmentShadersFor(yuv, sizeof(yuv) / sizeof(yuv[0]),
                             yae_gl_arb_yuv_to_rgb_2d);

    // for YUVA formats:
    static const TPixelFormatId yuva[] = {
      kPixelFormatYUVA420P
    };

    createFragmentShadersFor(yuva, sizeof(yuva) / sizeof(yuva[0]),
                             yae_gl_arb_yuva_to_rgba_2d);

    // for natively supported formats:
    createBuiltinFragmentShader(yae_gl_arb_passthrough_2d);
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::clear
  //
  void
  TLegacyCanvas::clear(QGLWidget * canvas)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    if (!texId_.empty())
    {
      glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
      texId_.clear();
    }

    w_ = 0;
    h_ = 0;
    dar_ = 0.0;
    darCropped_ = 0.0;
    crop_.clear();
    tiles_.clear();
    frame_ = TVideoFramePtr();
  }

  //----------------------------------------------------------------
  // calculateEdges
  //
  static void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgeSize,
                 GLsizei textureEdgeMax)
  {
    if (!edgeSize)
    {
      return;
    }

    GLsizei offset = 0;
    GLsizei extent = edgeSize;
    GLsizei segmentStart = 0;

    while (true)
    {
      edges.push_back(TEdge());
      TEdge & edge = edges.back();

      edge.offset_ = offset;
      edge.extent_ = std::min<GLsizei>(textureEdgeMax, powerOfTwoLEQ(extent));

      if (edge.extent_ < extent &&
          edge.extent_ < textureEdgeMax)
      {
        edge.extent_ *= 2;
      }

      // padding:
      GLsizei p0 = (edge.offset_ > 0) ? 1 : 0;
      GLsizei p1 = (edge.extent_ < extent) ? 1 : 0;

      edge.length_ = std::min<GLsizei>(edge.extent_, extent);
      edge.v0_ = segmentStart;
      edge.v1_ = edge.v0_ + edge.length_ - (p0 + p1);
      segmentStart = edge.v1_;

      edge.t0_ = double(p0) / double(edge.extent_);
      edge.t1_ = double(edge.length_ - p1) / double(edge.extent_);

      if (edge.extent_ < extent)
      {
        offset += edge.extent_ - 2;
        extent -= edge.extent_ - 2;
        continue;
      }

      break;
    }
  }

  //----------------------------------------------------------------
  // getTextureEdgeMax
  //
  static GLsizei
  getTextureEdgeMax()
  {
    static GLsizei edgeMax = 64;

    if (edgeMax > 64)
    {
      return edgeMax;
    }

    for (unsigned int i = 0; i < 8; i++, edgeMax *= 2)
    {
      glTexImage2D(GL_PROXY_TEXTURE_2D,
                   0, // level
                   GL_RGBA,
                   edgeMax * 2, // width
                   edgeMax * 2, // height
                   0,
                   GL_RGBA,
                   GL_UNSIGNED_BYTE,
                   NULL);
      GLenum err = glGetError();
      if (err != GL_NO_ERROR)
      {
        break;
      }

      GLint width = 0;
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
                               0, // level
                               GL_TEXTURE_WIDTH,
                               &width);
      if (width != GLint(edgeMax * 2))
      {
        break;
      }
    }

    return edgeMax;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::loadFrame
  //
  bool
  TLegacyCanvas::loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame)
  {
    // video traits shortcut:
    const VideoTraits & vtts = frame->traits_;

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(vtts.pixelFormat_);

    if (!ptts)
    {
      // don't know how to handle this pixel format:
      return false;
    }

    unsigned int supportedChannels =
      configure_builtin_shader(builtinShader_, vtts.pixelFormat_);

    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(canvas);

    // avoid creating excessively oversized tiles:
    static const GLsizei textureEdgeMax =
      std::min<GLsizei>(4096, std::max<GLsizei>(64, getTextureEdgeMax() / 2));

    // take the new frame:
    bool colorSpaceOrRangeChanged = false;
    bool frameSizeOrFormatChanged = setFrame(frame, colorSpaceOrRangeChanged);

    TCropFrame crop;
    getCroppedFrame(crop);

    if (frameSizeOrFormatChanged || w_ != crop.w_ || h_ != crop.h_)
    {
      if (!texId_.empty())
      {
        glDeleteTextures((GLsizei)(texId_.size()), &(texId_.front()));
        texId_.clear();
      }

      w_ = crop.w_;
      h_ = crop.h_;
      shader_ = findSomeShaderFor(vtts.pixelFormat_);

      if (!supportedChannels && !shader_)
      {
        return false;
      }

      const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;

      // calculate x-min, x-max coordinates for each tile:
      std::deque<TEdge> x;
      calculateEdges(x, w_, textureEdgeMax);

      // calculate y-min, y-max coordinates for each tile:
      std::deque<TEdge> y;
      calculateEdges(y, h_, textureEdgeMax);

      // setup the tiles:
      const std::size_t rows = y.size();
      const std::size_t cols = x.size();
      tiles_.resize(rows * cols);

      texId_.resize(rows * cols * shader.numPlanes_);
      glGenTextures((GLsizei)(texId_.size()), &(texId_.front()));

      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        TFrameTile & tile = tiles_[i];
        tile.x_ = x[i % cols];
        tile.y_ = y[i / cols];

        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          GLuint texId = texId_[k + i * shader.numPlanes_];
          glBindTexture(GL_TEXTURE_2D, texId);

          if (!glIsTexture(texId))
          {
            YAE_ASSERT(false);
            return false;
          }

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_BASE_LEVEL, 0);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MAX_LEVEL, 0);

          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MAG_FILTER,
                          shader.magFilterGL_[k]);
          glTexParameteri(GL_TEXTURE_2D,
                          GL_TEXTURE_MIN_FILTER,
                          shader.minFilterGL_[k]);
          yae_assert_gl_no_error();

          glTexImage2D(GL_TEXTURE_2D,
                       0, // mipmap level
                       shader.internalFormatGL_[k],
                       tile.x_.extent_ / shader.subsample_x_[k],
                       tile.y_.extent_ / shader.subsample_y_[k],
                       0, // border width
                       shader.pixelFormatGL_[k],
                       shader.dataTypeGL_[k],
                       NULL);

          if (!yae_assert_gl_no_error())
          {
            return false;
          }
        }
      }
    }

    if (!supportedChannels && !shader_)
    {
      return false;
    }

    // get the source data pointers:
    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    const unsigned char * src[4] = { NULL };

    for (std::size_t k = 0; k < shader.numPlanes_; k++)
    {
      const unsigned int subsample_x = shader.subsample_x_[k];
      const unsigned int subsample_y = shader.subsample_y_[k];
      const std::size_t bytesPerRow = frame_->data_->rowBytes(k);
      const std::size_t bytesPerPixel = ptts->stride_[k] / 8;
      src[k] =
        frame_->data_->data(k) +
        (crop.y_ / subsample_y) * bytesPerRow +
        (crop.x_ / subsample_x) * bytesPerPixel;
    }

    // upload the texture data:
    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    for (std::size_t k = 0; k < shader.numPlanes_; k++)
    {
      unsigned int subsample_x = shader.subsample_x_[k];
      unsigned int subsample_y = shader.subsample_y_[k];

      glPixelStorei(GL_UNPACK_SWAP_BYTES, shader.shouldSwapBytes_[k]);

      const unsigned char * data = frame->data_->data(k);
      std::size_t rowSize =
        frame->data_->rowBytes(k) / (ptts->stride_[k] / 8);
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignmentFor(data, rowSize));
      glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(rowSize));
      yae_assert_gl_no_error();

      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        const TFrameTile & tile = tiles_[i];

        GLuint texId = texId_[k + i * shader.numPlanes_];
        glBindTexture(GL_TEXTURE_2D, texId);

        if (!glIsTexture(texId))
        {
          YAE_ASSERT(false);
          continue;
        }

        glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                      tile.x_.offset_ / subsample_x);
        yae_assert_gl_no_error();

        glPixelStorei(GL_UNPACK_SKIP_ROWS,
                      tile.y_.offset_ / subsample_y);
        yae_assert_gl_no_error();

        glTexSubImage2D(GL_TEXTURE_2D,
                        0, // mipmap level
                        0, // x-offset
                        0, // y-offset
                        tile.x_.length_ / subsample_x,
                        tile.y_.length_ / subsample_y,
                        shader.pixelFormatGL_[k],
                        shader.dataTypeGL_[k],
                        src[k]);
        yae_assert_gl_no_error();

        if (tile.x_.length_ < tile.x_.extent_)
        {
          // extend on the right to avoid texture filtering artifacts:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                        (tile.x_.offset_ + tile.x_.length_) /
                        subsample_x - 1);
          yae_assert_gl_no_error();

          glPixelStorei(GL_UNPACK_SKIP_ROWS,
                        tile.y_.offset_ / subsample_y);
          yae_assert_gl_no_error();

          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          tile.x_.length_ / subsample_x,
                          0,

                          // width, height
                          1,
                          tile.y_.length_ / subsample_y,

                          shader.pixelFormatGL_[k],
                          shader.dataTypeGL_[k],
                          src[k]);
          yae_assert_gl_no_error();
        }

        if (tile.y_.length_ < tile.y_.extent_)
        {
          // extend on the bottom to avoid texture filtering artifacts:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                        tile.x_.offset_ / subsample_x);

          glPixelStorei(GL_UNPACK_SKIP_ROWS,
                        (tile.y_.offset_ + tile.y_.length_) /
                        subsample_y - 1);

          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          0,
                          tile.y_.length_ / subsample_y,

                          // width, height
                          tile.x_.length_ / subsample_x,
                          1,

                          shader.pixelFormatGL_[k],
                          shader.dataTypeGL_[k],
                          src[k]);
          yae_assert_gl_no_error();
        }

        if (tile.x_.length_ < tile.x_.extent_ &&
            tile.y_.length_ < tile.y_.extent_)
        {
          // extend the bottom-right corner:
          glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                        (tile.x_.offset_ + tile.x_.length_) /
                        subsample_x - 1);
          glPixelStorei(GL_UNPACK_SKIP_ROWS,
                        (tile.y_.offset_ + tile.y_.length_) /
                        subsample_y - 1);

          glTexSubImage2D(GL_TEXTURE_2D,
                          0, // mipmap level

                          // x,y offset
                          tile.x_.length_ / subsample_x,
                          tile.y_.length_ / subsample_y,

                          // width, height
                          1,
                          1,

                          shader.pixelFormatGL_[k],
                          shader.dataTypeGL_[k],
                          src[k]);
          yae_assert_gl_no_error();
        }
      }
    }

    if (shader_)
    {
      if (colorSpaceOrRangeChanged)
      {
        init_abc_to_rgb_matrix(&m34_to_rgb_[0], vtts);
      }

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_->program_->handle_);
      {
        // pass the color transform matrix to the shader:
        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      0, &m34_to_rgb_[0]);
        yae_assert_gl_no_error();

        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      1, &m34_to_rgb_[4]);
        yae_assert_gl_no_error();

        glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                      2, &m34_to_rgb_[8]);
        yae_assert_gl_no_error();
      }
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

    return true;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::draw
  //
  void
  TLegacyCanvas::draw()
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    if (texId_.empty() || !frame_)
    {
      return;
    }

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);

    double iw = 0.0;
    double ih = 0.0;
    imageWidthHeight(iw, ih);

    TCropFrame crop;
    getCroppedFrame(crop);

    if (glActiveTexture)
    {
      glActiveTexture(GL_TEXTURE0);
      yae_assert_gl_no_error();
    }

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(1.f, 1.f, 1.f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (shader_)
    {
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
    }

    double sx = iw / double(crop.w_);
    double sy = ih / double(crop.h_);
    glScaled(sx, sy, 1.0);

    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    const std::size_t numTiles = tiles_.size();
    for (std::size_t i = 0; i < numTiles; i++)
    {
      const TFrameTile & tile = tiles_[i];

      if (shader_)
      {
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_->program_->handle_);
      }

      if (glActiveTexture)
      {
        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          glActiveTexture((GLenum)(GL_TEXTURE0 + k));
          yae_assert_gl_no_error();

          glBindTexture(GL_TEXTURE_2D, texId_[k + i * shader.numPlanes_]);
        }
      }
      else
      {
        glBindTexture(GL_TEXTURE_2D, texId_[i * shader.numPlanes_]);
      }

      glBegin(GL_QUADS);
      {
        glTexCoord2d(tile.x_.t0_, tile.y_.t0_);
        glVertex2i(tile.x_.v0_, tile.y_.v0_);

        glTexCoord2d(tile.x_.t1_, tile.y_.t0_);
        glVertex2i(tile.x_.v1_, tile.y_.v0_);

        glTexCoord2d(tile.x_.t1_, tile.y_.t1_);
        glVertex2i(tile.x_.v1_, tile.y_.v1_);

        glTexCoord2d(tile.x_.t0_, tile.y_.t1_);
        glVertex2i(tile.x_.v0_, tile.y_.v1_);
      }
      glEnd();
    }

    // un-bind the textures:
    if (glActiveTexture)
    {
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        glActiveTexture((GLenum)(GL_TEXTURE0 + k));
        yae_assert_gl_no_error();

        glBindTexture(GL_TEXTURE_2D, 0);
      }
    }
    else
    {
      glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (shader_)
    {
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

    glDisable(GL_TEXTURE_2D);
  }


  //----------------------------------------------------------------
  // Canvas::TPrivate
  //
  class Canvas::TPrivate
  {
    std::string openglVendorInfo_;
    std::string openglRendererInfo_;
    std::string openglVersionInfo_;

    TLegacyCanvas * legacy_;
    TModernCanvas * modern_;
    TBaseCanvas * renderer_;

    // maximum texture size supported by the GL_EXT_texture_rectangle;
    // frames with width/height in excess of this value will be processed
    // using the legacy canvas renderer, which cuts frames into tiles
    // of supported size and renders them seamlessly:
    unsigned int maxTexSize_;

  public:
    TPrivate():
      openglVendorInfo_((const char *)glGetString(GL_VENDOR)),
      openglRendererInfo_((const char *)glGetString(GL_RENDERER)),
      openglVersionInfo_((const char *)glGetString(GL_VERSION)),
      legacy_(new TLegacyCanvas()),
      modern_(NULL),
      maxTexSize_(getTextureEdgeMax())
    {
      // rectangular textures do not work correctly on VirtualBox VMs,
      // so try to detect this and fall back to power-of-2 textures:
      bool virtualBoxVM =
        (openglVendorInfo_ == "Humper" &&
         openglRendererInfo_ == "Chromium" &&
         openglVersionInfo_ == "2.1 Chromium 1.9");

      if (yae_is_opengl_extension_supported("GL_ARB_texture_rectangle") &&
          !virtualBoxVM)
      {
        modern_ = new TModernCanvas();
      }

      if (yae_is_opengl_extension_supported("GL_ARB_fragment_program"))
      {
        GLint numTextureUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &numTextureUnits);

        if (numTextureUnits > 2)
        {
          legacy_->createFragmentShaders();

          if (modern_)
          {
            modern_->createFragmentShaders();
          }
        }
      }

      renderer_ = legacy_;
    }

    ~TPrivate()
    {
      delete legacy_;
      delete modern_;
    }

    void clear(QGLWidget * canvas)
    {
      renderer_->clear(canvas);
    }

    TBaseCanvas * rendererFor(const VideoTraits & vtts) const
    {
      if (modern_)
      {
        if (renderer_ == modern_)
        {
          if (maxTexSize_ < vtts.encodedWidth_ ||
              maxTexSize_ < vtts.encodedHeight_)
          {
            // use tiled legacy OpenGL renderer:
            return legacy_;
          }
        }
        else if (maxTexSize_ >= vtts.encodedWidth_ &&
                 maxTexSize_ >= vtts.encodedHeight_)
        {
          // use to modern OpenGL renderer:
          return modern_;
        }
      }

      // keep using the current renderer:
      return renderer_;
    }

    bool loadFrame(QGLWidget * canvas, const TVideoFramePtr & frame)
    {
      if (modern_)
      {
        TBaseCanvas * renderer = rendererFor(frame->traits_);
        if (renderer != renderer_)
        {
          // switch to a different renderer:
          renderer_->clear(canvas);
          renderer_ = renderer;
        }
      }

      return renderer_->loadFrame(canvas, frame);
    }

    void draw()
    {
      renderer_->draw();
    }

    inline const pixelFormat::Traits * pixelTraits() const
    {
      return renderer_->pixelTraits();
    }

    void skipColorConverter(QGLWidget * canvas, bool enable)
    {
      legacy_->skipColorConverter(canvas, enable);

      if (modern_)
      {
        modern_->skipColorConverter(canvas, enable);
      }
    }

    void enableVerticalScaling(bool enable)
    {
      legacy_->enableVerticalScaling(enable);

      if (modern_)
      {
        modern_->enableVerticalScaling(enable);
      }
    }

    bool getCroppedFrame(TCropFrame & crop) const
    {
      return renderer_->getCroppedFrame(crop);
    }

    bool imageWidthHeight(double & w, double & h) const
    {
      return renderer_->imageWidthHeight(w, h);
    }

    bool imageWidthHeightRotated(double & w, double & h, int & rotate) const
    {
      return renderer_->imageWidthHeightRotated(w, h, rotate);
    }

    inline void overrideDisplayAspectRatio(double dar)
    {
      legacy_->overrideDisplayAspectRatio(dar);

      if (modern_)
      {
        modern_->overrideDisplayAspectRatio(dar);
      }
    }

    inline void cropFrame(double darCropped)
    {
      legacy_->cropFrame(darCropped);

      if (modern_)
      {
        modern_->cropFrame(darCropped);
      }
    }

    inline void cropFrame(const TCropFrame & crop)
    {
      legacy_->cropFrame(crop);

      if (modern_)
      {
        modern_->cropFrame(crop);
      }
    }

    inline void getFrame(TVideoFramePtr & frame) const
    {
      renderer_->getFrame(frame);
    }

    const TFragmentShader *
    fragmentShaderFor(const VideoTraits & vtts) const
    {
      TBaseCanvas * renderer = rendererFor(vtts);
      return renderer->fragmentShaderFor(vtts.pixelFormat_);
    }
  };



#ifdef YAE_USE_LIBASS
  //----------------------------------------------------------------
  // getFontsConf
  //
  static bool
  getFontsConf(std::string & fontsConf, bool & removeAfterUse)
  {
#if !defined(_WIN32)
    fontsConf = "/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the system fontconfig file:
      removeAfterUse = false;
      return true;
    }
#endif

#if defined(__APPLE__)
    fontsConf = "/opt/local/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the macports fontconfig file:
      removeAfterUse = false;
      return true;
    }
#endif

    removeAfterUse = true;
    int64 appPid = QCoreApplication::applicationPid();

    QString tempDir = YAE_STANDARD_LOCATION(TempLocation);
    QString fontsDir = YAE_STANDARD_LOCATION(FontsLocation);
    QString cacheDir = YAE_STANDARD_LOCATION(CacheLocation);

    QString fontconfigCache =
      cacheDir + QString::fromUtf8("/apprenticevideo-fontconfig-cache");

    std::ostringstream os;
    os << "<?xml version=\"1.0\"?>" << std::endl
       << "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">" << std::endl
       << "<fontconfig>" << std::endl
       << "\t<dir>"
       << QDir::toNativeSeparators(fontsDir).toUtf8().constData()
       << "</dir>" << std::endl;

#ifdef __APPLE__
    os << "\t<dir>/Library/Fonts</dir>" << std::endl
       << "\t<dir>~/Library/Fonts</dir>" << std::endl;
#endif

#ifndef _WIN32
    const char * fontdir[] = {
      "/usr/share/fonts",
      "/usr/X11R6/lib/X11/fonts",
      "/opt/kde3/share/fonts",
      "/usr/local/share/fonts"
    };

    std::size_t nfontdir = sizeof(fontdir) / sizeof(fontdir[0]);
    for (std::size_t i = 0; i < nfontdir; i++)
    {
      QString path = QString::fromUtf8(fontdir[i]);
      if (QFileInfo(path).exists())
      {
        os << "\t<dir>" << fontdir[i] << "</dir>" << std::endl;
      }
    }
#endif

    os << "\t<cachedir>"
       << QDir::toNativeSeparators(fontconfigCache).toUtf8().constData()
       << "</cachedir>" << std::endl
       << "</fontconfig>" << std::endl;

    QString fn =
      tempDir +
      QString::fromUtf8("/apprenticevideo.fonts.conf.") +
      QString::number(appPid);

    fontsConf = QDir::toNativeSeparators(fn).toUtf8().constData();

#if !defined(NDEBUG)
    std::cerr << "fonts.conf: " << fontsConf << std::endl;
#endif

    std::FILE * fout = fopenUtf8(fontsConf.c_str(), "w");
    if (!fout)
    {
      return false;
    }

    std::string xml = os.str().c_str();

#if !defined(NDEBUG)
    std::cerr << "fonts.conf content:\n" << xml << std::endl;
#endif

    std::size_t nout = fwrite(xml.c_str(), 1, xml.size(), fout);
    fclose(fout);

    return nout == xml.size();
  }

  //----------------------------------------------------------------
  // TLibassInitDoneCallback
  //
  typedef void(*TLibassInitDoneCallback)(void *, TLibass *);
#endif

  //----------------------------------------------------------------
  // TLibassInit
  //
  class TLibass
  {
  public:

#ifdef YAE_USE_LIBASS
    //----------------------------------------------------------------
    // TLine
    //
    struct TLine
    {
      TLine(int64 pts = 0,
            const unsigned char * data = NULL,
            std::size_t size = 0):
        pts_(pts),
        data_((const char *)data, (const char *)data + size)
      {}

      bool operator == (const TLine & sub) const
      {
        return pts_ == sub.pts_ && data_ == sub.data_;
      }

      // presentation timestamp expressed in milliseconds:
      int64 pts_;

      // subtitle dialog line:
      std::string data_;
    };

    TLibass():
      callbackContext_(NULL),
      callback_(NULL),
      initialized_(false),
      assLibrary_(NULL),
      assRenderer_(NULL),
      assTrack_(NULL),
      bufferSize_(0)
    {}

    ~TLibass()
    {
      uninit();
    }

    void setCallback(void * context, TLibassInitDoneCallback callback)
    {
      YAE_ASSERT(!callback_ || !callback);
      callbackContext_ = context;
      callback_ = callback;
    }

    void setHeader(const unsigned char * codecPrivate = NULL,
                   std::size_t codecPrivateSize = 0)
    {
      header_.clear();
      if (codecPrivate && codecPrivateSize)
      {
        const char * header = (const char *)codecPrivate;
        header_.assign(header, header + codecPrivateSize);
      }
    }

    void setCustomFonts(const std::list<TFontAttachment> & customFonts)
    {
      customFonts_ = customFonts;
    }

    inline bool isReady() const
    {
      return initialized_;
    }

    void setFrameSize(int w, int h)
    {
      ass_set_frame_size(assRenderer_, w, h);

      double ar = double(w) / double(h);
      ass_set_aspect_ratio(assRenderer_, ar, ar);
    }

    void processData(const unsigned char * data, std::size_t size, int64 pts)
    {
      TLine line(pts, data, size);
      if (has(buffer_, line))
      {
        return;
      }

      // std::cerr << "ass_process_data: " << line.data_ << std::endl;

      if (bufferSize_)
      {
        const TLine & first = buffer_.front();
        if (pts < first.pts_)
        {
          // user skipped back in time, purge cached subs:
          ass_flush_events(assTrack_);
          buffer_.clear();
          bufferSize_ = 0;
        }
      }

      if (bufferSize_ < 10)
      {
        bufferSize_++;
      }
      else
      {
        buffer_.pop_front();
      }

      buffer_.push_back(line);
      ass_process_data(assTrack_, (char *)data, (int)size);
    }

    ASS_Image * renderFrame(int64 now, int * detectChange)
    {
      return ass_render_frame(assRenderer_,
                              assTrack_,
                              (long long)now,
                              detectChange);
    }

    void init()
    {
      uninit();

      assLibrary_ = ass_library_init();
      assRenderer_ = ass_renderer_init(assLibrary_);
      assTrack_ = ass_new_track(assLibrary_);

      for (std::list<TFontAttachment>::const_iterator
             i = customFonts_.begin(); i != customFonts_.end(); ++i)
      {
        const TFontAttachment & font = *i;
        ass_add_font(assLibrary_,
                     (char *)font.filename_,
                     (char *)font.data_,
                     (int)font.size_);
      }

      // lookup Fontconfig configuration file path:
      std::string fontsConf;
      bool removeAfterUse = false;
      getFontsConf(fontsConf, removeAfterUse);

      const char * defaultFont = NULL;
      const char * defaultFamily = NULL;
      int useFontconfig = 1;
      int updateFontCache = 1;

      ass_set_fonts(assRenderer_,
                    defaultFont,
                    defaultFamily,
                    useFontconfig,
                    fontsConf.size() ? fontsConf.c_str() : NULL,
                    updateFontCache);

      if (removeAfterUse)
      {
        // remove the temporary fontconfig file:
        QFile::remove(QString::fromUtf8(fontsConf.c_str()));
      }

      if (assTrack_ && header_.size())
      {
        ass_process_codec_private(assTrack_,
                                  &header_[0],
                                  (int)(header_.size()));
      }
    }

    void uninit()
    {
      if (assTrack_)
      {
        ass_free_track(assTrack_);
        assTrack_ = NULL;

        ass_renderer_done(assRenderer_);
        assRenderer_ = NULL;

        ass_library_done(assLibrary_);
        assLibrary_ = NULL;
      }
    }

    void threadLoop()
    {
      // begin:
      initialized_ = false;

      // this can take a while to rebuild the font cache:
      init();

      // done:
      initialized_ = true;

      if (callback_)
      {
        callback_(callbackContext_, this);
      }
    }

    void * callbackContext_;
    TLibassInitDoneCallback callback_;
    bool initialized_;

    ASS_Library * assLibrary_;
    ASS_Renderer * assRenderer_;
    ASS_Track * assTrack_;
    std::vector<char> header_;
    std::list<TFontAttachment> customFonts_;
    std::list<TLine> buffer_;
    std::size_t bufferSize_;
#endif
  };

  //----------------------------------------------------------------
  // libassInitThread
  //
#ifdef YAE_USE_LIBASS
  static Thread<TLibass> libassInitThread;
#endif

  //----------------------------------------------------------------
  // asyncInitLibass
  //
  TLibass *
  Canvas::asyncInitLibass(const unsigned char * header,
                          const std::size_t headerSize)
  {
    TLibass * libass = NULL;

#ifdef YAE_USE_LIBASS
    libass = new TLibass();
    libass->setCallback(this, &Canvas::libassInitDoneCallback);
    libass->setHeader(header, headerSize);
    libass->setCustomFonts(customFonts_);

    if (!libassInitThread.isRunning())
    {
      libassInitThread.setContext(libass);
      libassInitThread.run();
    }
    else
    {
      YAE_ASSERT(false);
      libassInitThread.stop();
    }
#endif

    return libass;
  }

  //----------------------------------------------------------------
  // stopAsyncInitLibassThread
  //
  static void
  stopAsyncInitLibassThread()
  {
#ifdef YAE_USE_LIBASS
    if (libassInitThread.isRunning())
    {
      libassInitThread.stop();
      libassInitThread.wait();
      libassInitThread.setContext(NULL);
    }
#endif
  }

  //----------------------------------------------------------------
  // TFontAttachment::TFontAttachment
  //
  TFontAttachment::TFontAttachment(const char * filename,
                                   const unsigned char * data,
                                   std::size_t size):
    filename_(filename),
    data_(data),
    size_(size)
  {}

  //----------------------------------------------------------------
  // Canvas::Canvas
  //
  Canvas::Canvas(const QGLFormat & format,
                 QWidget * parent,
                 const QGLWidget * shareWidget,
                 Qt::WindowFlags f):
    QGLWidget(format, parent, shareWidget, f),
    private_(NULL),
    overlay_(NULL),
    libass_(NULL),
    showTheGreeting_(true),
    subsInOverlay_(false),
    renderMode_(Canvas::kScaleToFit),
    timerHideCursor_(this),
    timerScreenSaver_(this)
  {
    setObjectName("yae::Canvas");
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoBufferSwap(true);
    setAutoFillBackground(false);
    setMouseTracking(true);

    timerHideCursor_.setSingleShot(true);
    timerHideCursor_.setInterval(3000);

    timerScreenSaver_.setSingleShot(true);
    timerScreenSaver_.setInterval(29000);

    timerScreenSaverUnInhibit_.setSingleShot(true);
    timerScreenSaverUnInhibit_.setInterval(59000);

    bool ok = true;
    ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                 this, SLOT(hideCursor()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaver_, SIGNAL(timeout()),
                 this, SLOT(screenSaverInhibit()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaverUnInhibit_, SIGNAL(timeout()),
                 this, SLOT(screenSaverUnInhibit()));
    YAE_ASSERT(ok);

    greeting_ = tr("drop videos/music here\n\n"
                   "press spacebar to pause/resume\n\n"
                   "alt-left/alt-right to navigate playlist\n\n"
#ifdef __APPLE__
                   "use apple remote for volume and seeking\n\n"
#endif
                   "explore the menus for more options");
  }

  //----------------------------------------------------------------
  // Canvas::~Canvas
  //
  Canvas::~Canvas()
  {
    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    delete private_;
    delete overlay_;
  }

  //----------------------------------------------------------------
  // Canvas::initializePrivateBackend
  //
  void
  Canvas::initializePrivateBackend()
  {
    TMakeCurrentContext currentContext(this);

    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    delete private_;
    private_ = NULL;

    delete overlay_;
    overlay_ = NULL;

    private_ = new TPrivate();
    overlay_ = new TPrivate();

    libass_ = asyncInitLibass();
  }

  //----------------------------------------------------------------
  // Canvas::fragmentShaderFor
  //
  const TFragmentShader *
  Canvas::fragmentShaderFor(const VideoTraits & vtts) const
  {
    return private_ ? private_->fragmentShaderFor(vtts) : NULL;
  }

  //----------------------------------------------------------------
  // Canvas::acceptFramesWithReaderId
  //
  void
  Canvas::acceptFramesWithReaderId(unsigned int readerId)
  {
    payload_.setExpectedReaderId(readerId);
  }

  //----------------------------------------------------------------
  // Canvas::libassAddFont
  //
  void
  Canvas::libassAddFont(const char * filename,
                        const unsigned char * data,
                        const std::size_t size)
  {
    customFonts_.push_back(TFontAttachment(filename, data, size));
  }

  //----------------------------------------------------------------
  // Canvas::clear
  //
  void
  Canvas::clear()
  {
    private_->clear(this);
    clearOverlay();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::clearOverlay
  //
  void
  Canvas::clearOverlay()
  {
    overlay_->clear(this);

    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    showTheGreeting_ = false;
    subsInOverlay_ = false;
    subs_.clear();
    customFonts_.clear();
  }

  //----------------------------------------------------------------
  // Canvas::refresh
  //
  void
  Canvas::refresh()
  {
    if (!isVisible())
    {
      return;
    }

    QGLWidget::updateGL();
    QGLWidget::doneCurrent();
  }

  //----------------------------------------------------------------
  // Canvas::render
  //
  bool
  Canvas::render(const TVideoFramePtr & frame)
  {
    bool postThePayload = payload_.set(frame);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new RenderFrameEvent(payload_));
    }

    if (autoCropThread_.isRunning())
    {
      autoCrop_.setFrame(frame);
    }

    return true;
  }

  //----------------------------------------------------------------
  // UpdateOverlayEvent
  //
  struct UpdateOverlayEvent : public QEvent
  {
    UpdateOverlayEvent(): QEvent(QEvent::User) {}
  };

  //----------------------------------------------------------------
  // LibassInitDoneEvent
  //
  struct LibassInitDoneEvent : public QEvent
  {
    LibassInitDoneEvent(TLibass * libass):
      QEvent(QEvent::User),
      libass_(libass)
    {}

    TLibass * libass_;
  };

  //----------------------------------------------------------------
  // Canvas::event
  //
  bool
  Canvas::event(QEvent * event)
  {
    if (event->type() == QEvent::User)
    {
      RenderFrameEvent * renderEvent = dynamic_cast<RenderFrameEvent *>(event);
      if (renderEvent)
      {
        event->accept();

        TVideoFramePtr frame;
        renderEvent->payload_.get(frame);
        loadFrame(frame);

        return true;
      }

      UpdateOverlayEvent * overlayEvent =
        dynamic_cast<UpdateOverlayEvent *>(event);
      if (overlayEvent)
      {
        event->accept();

        updateOverlay(true);
        refresh();
        return true;
      }

      LibassInitDoneEvent * libassInitDoneEvent =
        dynamic_cast<LibassInitDoneEvent *>(event);
      if (libassInitDoneEvent)
      {
        event->accept();

#ifdef YAE_USE_LIBASS
        stopAsyncInitLibassThread();
        updateOverlay(true);
        refresh();
#endif
        return true;
      }
    }

    return QGLWidget::event(event);
  }

  //----------------------------------------------------------------
  // Canvas::mouseMoveEvent
  //
  void
  Canvas::mouseMoveEvent(QMouseEvent * event)
  {
    setCursor(QCursor(Qt::ArrowCursor));
    timerHideCursor_.start();
  }

  //----------------------------------------------------------------
  // Canvas::mouseDoubleClickEvent
  //
  void
  Canvas::mouseDoubleClickEvent(QMouseEvent * event)
  {
    emit toggleFullScreen();
  }

  //----------------------------------------------------------------
  // Canvas::resizeEvent
  //
  void
  Canvas::resizeEvent(QResizeEvent * event)
  {
    QGLWidget::resizeEvent(event);

    if (overlay_ && (subsInOverlay_ || showTheGreeting_))
    {
      updateOverlay(true);
    }
  }

  //----------------------------------------------------------------
  // Canvas::initializeGL
  //
  void
  Canvas::initializeGL()
  {
    QGLWidget::initializeGL();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    // glShadeModel(GL_SMOOTH);
    glShadeModel(GL_FLAT);
    glClearDepth(0);
    glClearStencil(0);
    glClearAccum(0, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
  }

  //----------------------------------------------------------------
  // calcImageWidth
  //
  static double
  calcImageWidth(const Canvas::TPrivate * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(w, h, cameraRotation);
    return w;
  }

  //----------------------------------------------------------------
  // calcImageHeight
  //
  static double
  calcImageHeight(const Canvas::TPrivate * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(w, h, cameraRotation);
    return h;
  }

  //----------------------------------------------------------------
  // paintImage
  //
  static void
  paintImage(Canvas::TPrivate * canvas,
             int canvasWidth,
             int canvasHeight,
             Canvas::TRenderMode renderMode)
  {
    double croppedWidth = 0.0;
    double croppedHeight = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(croppedWidth,
                                    croppedHeight,
                                    cameraRotation);

    double dar = croppedWidth / croppedHeight;
    double car = double(canvasWidth) / double(canvasHeight);

    double x = 0.0;
    double y = 0.0;
    double w = double(canvasWidth);
    double h = double(canvasHeight);

    if (renderMode == Canvas::kScaleToFit)
    {
      if (dar < car)
      {
        w = double(canvasHeight) * dar;
        x = 0.5 * (double(canvasWidth) - w);
      }
      else
      {
        h = double(canvasWidth) / dar;
        y = 0.5 * (double(canvasHeight) - h);
      }
    }
    else
    {
      if (dar < car)
      {
        h = double(canvasWidth) / dar;
        y = 0.5 * (double(canvasHeight) - h);
      }
      else
      {
        w = double(canvasHeight) * dar;
        x = 0.5 * (double(canvasWidth) - w);
      }
    }

    glViewport(GLint(x + 0.5), GLint(y + 0.5),
               GLsizei(w + 0.5), GLsizei(h + 0.5));

    double left = 0.0;
    double right = croppedWidth;
    double top = 0.0;
    double bottom = croppedHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1.0, 1.0);

    if (cameraRotation && cameraRotation % 90 == 0)
    {
      glTranslated(0.5 * croppedWidth, 0.5 * croppedHeight, 0);
      glRotated(double(cameraRotation), 0, 0, 1);

      if (cameraRotation % 180 != 0)
      {
        glTranslated(-0.5 * croppedHeight, -0.5 * croppedWidth, 0);
      }
      else
      {
        glTranslated(-0.5 * croppedWidth, -0.5 * croppedHeight, 0);
      }
    }

    canvas->draw();
    yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // Canvas::paintGL
  //
  void
  Canvas::paintGL()
  {
    if (width() == 0 || height() == 0)
    {
      return;
    }

    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    if (!ptts)
    {
      // unsupported pixel format:
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      if (!showTheGreeting_)
      {
        return;
      }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int canvasWidth = width();
    int canvasHeight = height();

    // draw a checkerboard to help visualize the alpha channel:
    if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                 pixelFormat::kPaletted)))
    {
      glViewport(0, 0, canvasWidth, canvasHeight);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, canvasWidth, canvasHeight, 0, -1.0, 1.0);

      float zebra[2][3] = {
        { 1.0f, 1.0f, 1.0f },
        { 0.7f, 0.7f, 0.7f }
      };

      int edgeSize = 24;
      bool evenRow = false;
      for (int y = 0; y < canvasHeight; y += edgeSize, evenRow = !evenRow)
      {
        int y1 = std::min(y + edgeSize, canvasHeight);

        bool evenCol = false;
        for (int x = 0; x < canvasWidth; x += edgeSize, evenCol = !evenCol)
        {
          int x1 = std::min(x + edgeSize, canvasWidth);

          float * color = (evenRow ^ evenCol) ? zebra[0] : zebra[1];
          glColor3fv(color);

          glRecti(x, y, x1, y1);
        }
      }

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    if (ptts)
    {
      // draw the frame:
      paintImage(private_, canvasWidth, canvasHeight, renderMode_);
    }

    // draw the overlay:
    if (subsInOverlay_ || showTheGreeting_)
    {
      if (overlay_ && overlay_->pixelTraits())
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        paintImage(overlay_, canvasWidth, canvasHeight, kScaleToFit);
        glDisable(GL_BLEND);
      }
      else
      {
        qApp->postEvent(this, new UpdateOverlayEvent());
      }
    }
  }

  //----------------------------------------------------------------
  // Canvas::loadFrame
  //
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
    if (!frame)
    {
      return false;
    }

    bool ok = private_->loadFrame(this, frame);
    showTheGreeting_ = false;
    setSubs(frame->subs_);

    refresh();

    if (ok && !timerScreenSaver_.isActive())
    {
      timerScreenSaver_.start();
    }

    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::currentFrame
  //
  TVideoFramePtr
  Canvas::currentFrame() const
  {
    TVideoFramePtr frame;

    if (private_)
    {
      private_->getFrame(frame);
    }

    return frame;
  }

  //----------------------------------------------------------------
  // Canvas::setSubs
  //
  void
  Canvas::setSubs(const std::list<TSubsFrame> & subs)
  {
    std::list<TSubsFrame> renderSubs;

    for (std::list<TSubsFrame>::const_iterator i = subs.begin();
         i != subs.end(); ++i)
    {
      const TSubsFrame & subs = *i;
      if (subs.render_)
      {
        renderSubs.push_back(subs);
      }
    }

    bool reparse = (renderSubs != subs_);
    if (reparse)
    {
      subs_ = renderSubs;
    }

    updateOverlay(reparse);
  }

  //----------------------------------------------------------------
  // TQImageBuffer
  //
  struct TQImageBuffer : public IPlanarBuffer
  {
    QImage qimg_;

    TQImageBuffer(int w, int h, QImage::Format fmt):
      qimg_(w, h, fmt)
    {
      unsigned char * dst = qimg_.bits();
      int rowBytes = qimg_.bytesPerLine();
      memset(dst, 0, rowBytes * h);
    }

    // virtual:
    void destroy()
    { delete this; }

    // virtual:
    std::size_t planes() const
    { return 1; }

    // virtual:
    unsigned char * data(std::size_t plane) const
    {
      const uchar * bits = qimg_.bits();
      return const_cast<unsigned char *>(bits);
    }

    // virtual:
    std::size_t rowBytes(std::size_t planeIndex) const
    {
      int n = qimg_.bytesPerLine();
      return (std::size_t)n;
    }
  };

  //----------------------------------------------------------------
  // drawPlainText
  //
  static bool
  drawPlainText(const std::string & text,
                QPainter & painter,
                QRect & bbox,
                int textAlignment)
  {
    QString qstr = QString::fromUtf8(text.c_str()).trimmed();
    if (!qstr.isEmpty())
    {
      QRect used;
      drawTextWithShadowToFit(painter,
                              bbox,
                              textAlignment,
                              qstr,
                              QPen(Qt::black),
                              true, // outline shadow
                              1, // shadow offset
                              &used);
      if (!used.isNull())
      {
        // avoid overwriting subs on top of each other:
        bbox.setBottom(used.top());
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // TPainterWrapper
  //
  struct TPainterWrapper
  {
    TPainterWrapper(int w, int h):
      painter_(NULL),
      w_(w),
      h_(h)
    {}

    ~TPainterWrapper()
    {
      delete painter_;
    }

    inline TVideoFramePtr & getFrame()
    {
      if (!frame_)
      {
        frame_.reset(new TVideoFrame());

        TQImageBuffer * imageBuffer =
          new TQImageBuffer(w_, h_, QImage::Format_ARGB32);
        // imageBuffer->qimg_.fill(0);

        frame_->data_.reset(imageBuffer);
      }

      return frame_;
    }

    inline QImage & getImage()
    {
      TVideoFramePtr & vf = getFrame();
      TQImageBuffer * imageBuffer = (TQImageBuffer *)(vf->data_.get());
      return imageBuffer->qimg_;
    }

    inline QPainter & getPainter()
    {
      if (!painter_)
      {
        QImage & image = getImage();
        painter_ = new QPainter(&image);

        painter_->setPen(Qt::white);
        painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);

        QFont ft;
        int px = std::max<int>(20, 56.0 * (h_ / 1024.0));
        ft.setPixelSize(px);
        painter_->setFont(ft);
      }

      return *painter_;
    }

    inline void painterEnd()
    {
      if (painter_)
      {
        painter_->end();
      }
    }

  private:
    TVideoFramePtr frame_;
    QPainter * painter_;
    int w_;
    int h_;
  };

  //----------------------------------------------------------------
  // calcFrameTransform
  //
  static void
  calcFrameTransform(double bbox_w,
                     double bbox_h,
                     double frame_w,
                     double frame_h,
                     double & offset_x,
                     double & offset_y,
                     double & scaled_w,
                     double & scaled_h,
                     bool fit_to_height = false)
  {
    double bbox_ar = double(bbox_w) / double(bbox_h);
    double frame_ar = double(frame_w) / double(frame_h);

    offset_x = 0;
    offset_y = 0;
    scaled_w = bbox_w;
    scaled_h = bbox_h;

    if (!fit_to_height)
    {
      fit_to_height = frame_ar < bbox_ar;
    }

    if (fit_to_height)
    {
      scaled_w = bbox_h * frame_ar;
      offset_x = 0.5 * (bbox_w - scaled_w);
    }
    else
    {
      scaled_h = bbox_w / frame_ar;
      offset_y = 0.5 * (bbox_h - scaled_h);
    }
  }

  //----------------------------------------------------------------
  // TScaledFrame
  //
  struct TScaledFrame
  {
    double x_;
    double y_;
    double w_;
    double h_;
  };

  //----------------------------------------------------------------
  // calcFrameTransform
  //
  inline static void
  calcFrameTransform(double bbox_w,
                     double bbox_h,
                     double frame_w,
                     double frame_h,
                     TScaledFrame & f,
                     bool fit_to_height = false)
  {
    calcFrameTransform(bbox_w,
                       bbox_h,
                       frame_w,
                       frame_h,
                       f.x_,
                       f.y_,
                       f.w_,
                       f.h_,
                       fit_to_height);
  }

  //----------------------------------------------------------------
  // Canvas::loadSubs
  //
  bool
  Canvas::updateOverlay(bool reparse)
  {
    if (showTheGreeting_)
    {
      return updateGreeting();
    }

    double imageWidth = 0.0;
    double imageHeight = 0.0;
    int cameraRotation = 0;
    private_->imageWidthHeightRotated(imageWidth,
                                      imageHeight,
                                      cameraRotation);

    double w = this->width();
    double h = this->height();

    double max_w = 1920.0;
    double max_h = 1080.0;

    if (h > max_h)
    {
      w *= max_h / h;
      h = max_h;
    }

    if (w > max_w)
    {
      h *= max_w / w;
      w = max_w;
    }

    double fw = w;
    double fh = h;
    double fx = 0;
    double fy = 0;

    calcFrameTransform(w, h, imageWidth, imageHeight, fx, fy, fw, fh);

    int ix = int(fx);
    int iy = int(fy);
    int iw = int(fw);
    int ih = int(fh);

    TPainterWrapper wrapper((int)w, (int)h);

    int textAlignment = Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignBottom;
    bool paintedSomeSubs = false;
    bool libassSameSubs = false;

    QRect canvasBBox(16, 16, (int)w - 32, (int)h - 32);
    TVideoFramePtr frame = currentFrame();

    for (std::list<TSubsFrame>::const_iterator i = subs_.begin();
         i != subs_.end() && reparse; ++i)
    {
      const TSubsFrame & subs = *i;
      const TSubsFrame::IPrivate * subExt = subs.private_.get();
      const unsigned int nrects = subExt ? subExt->numRects() : 0;
      unsigned int nrectsPainted = 0;

      for (unsigned int j = 0; j < nrects; j++)
      {
        TSubsFrame::TRect r;
        subExt->getRect(j, r);

        if (r.type_ == kSubtitleBitmap)
        {
          const unsigned char * pal = r.data_[1];

          QImage img(r.w_, r.h_, QImage::Format_ARGB32);
          unsigned char * dst = img.bits();
          int dstRowBytes = img.bytesPerLine();

          for (int y = 0; y < r.h_; y++)
          {
            const unsigned char * srcLine = r.data_[0] + y * r.rowBytes_[0];
            unsigned char * dstLine = dst + y * dstRowBytes;

            for (int x = 0; x < r.w_; x++, dstLine += 4, srcLine++)
            {
              int colorIndex = *srcLine;
              memcpy(dstLine, pal + colorIndex * 4, 4);
            }
          }

          // always fit to box height regardless of frame aspect ratio;
          // this may crop off part of the frame on the left and right,
          // but in practice it makes subtitles more visible when watching
          // a 4x3 video cropped from 16x9 blu-ray (FLCL, Star Trek TNG, etc...)
          bool fit_to_height = true;

          double rw = double(subs.rw_ ? subs.rw_ : imageWidth);
          double rh = double(subs.rh_ ? subs.rh_ : imageHeight);

          TScaledFrame sf;
          calcFrameTransform(w, h, rw, rh, sf, fit_to_height);

          double sx = sf.w_ / rw;
          double sy = sf.h_ / rh;

          QPoint dstPos((int)(sx * double(r.x_) + sf.x_),
                        (int)(sy * double(r.y_) + sf.y_));
          QSize dstSize((int)(sx * double(r.w_)),
                        (int)(sy * double(r.h_)));

          wrapper.getPainter().drawImage(QRect(dstPos, dstSize),
                                         img, img.rect());
          paintedSomeSubs = true;
          nrectsPainted++;
        }
        else if (r.type_ == kSubtitleASS)
        {
          std::string assa(r.assa_);
          bool done = false;

#ifdef YAE_USE_LIBASS
          if (!libass_)
          {
            if (subs.traits_ == kSubsSSA && subs.extraData_)
            {
              libass_ = asyncInitLibass(subs.extraData_->data(0),
                                        subs.extraData_->rowBytes(0));
            }
            else if (subExt->headerSize())
            {
              libass_ = asyncInitLibass(subExt->header(),
                                        subExt->headerSize());
            }
          }

          if (libass_ && libass_->isReady())
          {
            int64 pts = (int64)(subs.time_.toSeconds() * 1000.0 + 0.5);
            libass_->processData((unsigned char *)&assa[0], assa.size(), pts);
            nrectsPainted++;
            done = true;
          }
#endif
          if (!done)
          {
            std::string text = assaToPlainText(assa);
            text = convertEscapeCodes(text);

            if (drawPlainText(text,
                              wrapper.getPainter(),
                              canvasBBox,
                              textAlignment))
            {
              paintedSomeSubs = true;
              nrectsPainted++;
            }
          }
        }
      }

#ifdef YAE_USE_LIBASS
      if (!nrectsPainted && subs.data_ &&
          (subs.traits_ == kSubsSSA))
      {
        if (!libass_)
        {
          if (subs.traits_ == kSubsSSA && subs.extraData_)
          {
            libass_ = asyncInitLibass(subs.extraData_->data(0),
                                      subs.extraData_->rowBytes(0));
          }
          else if (subExt && subExt->headerSize())
          {
            libass_ = asyncInitLibass(subExt->header(),
                                      subExt->headerSize());
          }
        }

        if (libass_ && libass_->isReady())
        {
          const unsigned char * ssa = subs.data_->data(0);
          const std::size_t ssaSize = subs.data_->rowBytes(0);

          int64 pts = (int64)(subs.time_.toSeconds() * 1000.0 + 0.5);
          libass_->processData(ssa, ssaSize, pts);
          nrectsPainted++;
        }
      }
#endif

      if (!nrectsPainted && subs.data_ &&
          (subs.traits_ == kSubsSSA ||
           subs.traits_ == kSubsText ||
           subs.traits_ == kSubsSUBRIP))
      {
        const unsigned char * str = subs.data_->data(0);
        const unsigned char * end = str + subs.data_->rowBytes(0);

        std::string text(str, end);
        if (subs.traits_ == kSubsSSA)
        {
          text = assaToPlainText(text);
        }
        else
        {
          text = stripHtmlTags(text);
        }

        text = convertEscapeCodes(text);
        if (drawPlainText(text,
                          wrapper.getPainter(),
                          canvasBBox,
                          textAlignment))
        {
          paintedSomeSubs = true;
        }
      }
    }

#ifdef YAE_USE_LIBASS
    if (libass_ && libass_->isReady() && frame)
    {
      libass_->setFrameSize(iw, ih);

      // the list of images is owned by libass,
      // libass is responsible for their deallocation:
      int64 now = (int64)(frame->time_.toSeconds() * 1000.0 + 0.5);

      int changeDetected = 0;
      ASS_Image * pic = libass_->renderFrame(now, &changeDetected);
      libassSameSubs = !changeDetected;
      paintedSomeSubs = changeDetected;

      unsigned char bgra[4];
      while (pic && changeDetected)
      {
#ifdef __BIG_ENDIAN__
        bgra[3] = 0xFF & (pic->color >> 8);
        bgra[2] = 0xFF & (pic->color >> 16);
        bgra[1] = 0xFF & (pic->color >> 24);
        bgra[0] = 0xFF & (pic->color);
#else
        bgra[0] = 0xFF & (pic->color >> 8);
        bgra[1] = 0xFF & (pic->color >> 16);
        bgra[2] = 0xFF & (pic->color >> 24);
        bgra[3] = 0xFF & (pic->color);
#endif
        QImage tmp(pic->w, pic->h, QImage::Format_ARGB32);
        int dstRowBytes = tmp.bytesPerLine();
        unsigned char * dst = tmp.bits();

        for (int y = 0; y < pic->h; y++)
        {
          const unsigned char * srcLine = pic->bitmap + pic->stride * y;
          unsigned char * dstLine = dst + dstRowBytes * y;

          for (int x = 0; x < pic->w; x++, dstLine += 4, srcLine++)
          {
            unsigned char alpha = *srcLine;
#ifdef __BIG_ENDIAN__
            dstLine[0] = alpha;
            memcpy(dstLine + 1, bgra + 1, 3);
#else
            memcpy(dstLine, bgra, 3);
            dstLine[3] = alpha;
#endif
          }
        }

        wrapper.getPainter().drawImage(QRect(pic->dst_x + ix,
                                             pic->dst_y + iy,
                                             pic->w,
                                             pic->h),
                                       tmp, tmp.rect());

        pic = pic->next;
      }
    }
#endif

    wrapper.painterEnd();

    if (reparse && !libassSameSubs)
    {
      subsInOverlay_ = paintedSomeSubs;
    }

    if (!paintedSomeSubs)
    {
      return true;
    }

    TVideoFramePtr & vf = wrapper.getFrame();
    VideoTraits & vtts = vf->traits_;
    QImage & image = wrapper.getImage();

#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    subsInOverlay_ = overlay_->loadFrame(this, vf);
    YAE_ASSERT(subsInOverlay_);
    return subsInOverlay_;
  }

  //----------------------------------------------------------------
  // Canvas::setGreeting
  //
  void
  Canvas::setGreeting(const QString & greeting)
  {
    showTheGreeting_ = true;
    greeting_ = greeting;
    updateGreeting();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::updateGreeting
  //
  bool
  Canvas::updateGreeting()
  {
    if (!overlay_)
    {
      return false;
    }

    double w = this->width();
    double h = this->height();

    double max_w = 1920.0;
    double max_h = 1080.0;

    if (h > max_h)
    {
      w *= max_h / h;
      h = max_h;
    }

    if (w > max_w)
    {
      h *= max_w / w;
      w = max_w;
    }

    TVideoFramePtr vf(new TVideoFrame());
    TQImageBuffer * imageBuffer =
      new TQImageBuffer((int)w, (int)h, QImage::Format_ARGB32);
    vf->data_.reset(imageBuffer);

    // shortcut:
    QImage & subsFrm = imageBuffer->qimg_;
    subsFrm.fill(0);

    QPainter painter(&subsFrm);
    painter.setPen(QColor(0x7f, 0x7f, 0x7f, 0x7f));

    QFont ft;
    int px = std::max<int>(12, 56.0 * std::min<double>(w / max_w, h / max_h));
    ft.setPixelSize(px);
    painter.setFont(ft);

    int textAlignment = Qt::TextWordWrap | Qt::AlignCenter;
    QRect canvasBBox = subsFrm.rect();

    std::string text(greeting_.toUtf8().constData());
    if (!drawPlainText(text, painter, canvasBBox, textAlignment))
    {
      return false;
    }

    painter.end();

    VideoTraits & vtts = vf->traits_;
#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = subsFrm.bytesPerLine() / 4;
    vtts.encodedHeight_ = subsFrm.byteCount() / subsFrm.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    bool ok = overlay_->loadFrame(this, vf);
    YAE_ASSERT(ok);
    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::skipColorConverter
  //
  void
  Canvas::skipColorConverter(bool enable)
  {
    private_->skipColorConverter(this, enable);
  }

  //----------------------------------------------------------------
  // Canvas::enableVerticalScaling
  //
  void
  Canvas::enableVerticalScaling(bool enable)
  {
    private_->enableVerticalScaling(enable);
  }

  //----------------------------------------------------------------
  // Canvas::overrideDisplayAspectRatio
  //
  void
  Canvas::overrideDisplayAspectRatio(double dar)
  {
    private_->overrideDisplayAspectRatio(dar);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(double darCropped)
  {
    private_->cropFrame(darCropped);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(const TCropFrame & crop)
  {
    cropAutoDetectStop();

#if 0
    std::cerr << "\nCROP FRAME AUTO DETECTED: "
              << "x = " << crop.x_ << ", "
              << "y = " << crop.y_ << ", "
              << "w = " << crop.w_ << ", "
              << "h = " << crop.h_
              << std::endl;
#endif

    private_->cropFrame(crop);
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetect
  //
  void
  Canvas::cropAutoDetect(void * callbackContext, TAutoCropCallback callback)
  {
    if (!autoCropThread_.isRunning())
    {
      autoCrop_.reset(callbackContext, callback);
      autoCropThread_.setContext(&autoCrop_);
      autoCropThread_.run();
    }
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetectStop
  //
  void
  Canvas::cropAutoDetectStop()
  {
    autoCrop_.stop();
    autoCropThread_.stop();
    autoCropThread_.wait();
    autoCropThread_.setContext(NULL);
  }

  //----------------------------------------------------------------
  // Canvas::imageWidth
  //
  double
  Canvas::imageWidth() const
  {
    return private_ ? calcImageWidth(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageHeight
  //
  double
  Canvas::imageHeight() const
  {
    return private_ ? calcImageHeight(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageAspectRatio
  //
  double
  Canvas::imageAspectRatio(double & w, double & h) const
  {
    double dar = 0.0;
    w = 0.0;
    h = 0.0;

    if (private_)
    {
      dar = private_->imageWidthHeight(w, h) ? w / h : 0.0;
    }

    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::setRenderMode
  //
  void
  Canvas::setRenderMode(Canvas::TRenderMode renderMode)
  {
    if (renderMode_ != renderMode)
    {
      renderMode_ = renderMode;
      refresh();
    }
  }

  //----------------------------------------------------------------
  // Canvas::libassInitDoneCallback
  //
  void
  Canvas::libassInitDoneCallback(void * context, TLibass * libass)
  {
    Canvas * canvas = (Canvas *)context;
    qApp->postEvent(canvas, new LibassInitDoneEvent(libass));
  }

  //----------------------------------------------------------------
  // Canvas::hideCursor
  //
  void
  Canvas::hideCursor()
  {
    setCursor(QCursor(Qt::BlankCursor));
  }

  //----------------------------------------------------------------
  // screenSaverUnInhibitCookie
  //
  static unsigned int screenSaverUnInhibitCookie = 0;

  //----------------------------------------------------------------
  // Canvas::screenSaverInhibit
  //
  void
  Canvas::screenSaverInhibit()
  {
#ifdef __APPLE__
    UpdateSystemActivity(UsrActivity);
#elif defined(_WIN32)
    // http://www.codeproject.com/KB/system/disablescreensave.aspx
    //
    // Call the SystemParametersInfo function to query and reset the
    // screensaver time-out value.  Use the user's default settings
    // in case your application terminates abnormally.
    //

    static UINT spiGetter[] = { SPI_GETLOWPOWERTIMEOUT,
                                SPI_GETPOWEROFFTIMEOUT,
                                SPI_GETSCREENSAVETIMEOUT };

    static UINT spiSetter[] = { SPI_SETLOWPOWERTIMEOUT,
                                SPI_SETPOWEROFFTIMEOUT,
                                SPI_SETSCREENSAVETIMEOUT };

    std::size_t numParams = sizeof(spiGetter) / sizeof(spiGetter[0]);
    for (std::size_t i = 0; i < numParams; i++)
    {
      UINT val = 0;
      BOOL ok = SystemParametersInfo(spiGetter[i], 0, &val, 0);
      YAE_ASSERT(ok);
      if (ok)
      {
        ok = SystemParametersInfo(spiSetter[i], val, NULL, 0);
        YAE_ASSERT(ok);
      }
    }

#else
    // try using DBUS to talk to the screensaver...
    bool done = false;

    if (QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        // apparently SimulateUserActivity is not enough to keep Ubuntu
        // from starting the screensaver
        screensaver.call(QDBus::NoBlock, "SimulateUserActivity");

        // try to inhibit the screensaver as well:
        if (!screenSaverUnInhibitCookie)
        {
          QDBusMessage out =
            screensaver.call(QDBus::Block,
                             "Inhibit",
                             QVariant(QApplication::applicationName()),
                             QVariant("video playback"));

          if (out.type() == QDBusMessage::ReplyMessage &&
              !out.arguments().empty())
          {
            screenSaverUnInhibitCookie = out.arguments().front().toUInt();
          }
        }

        if (screenSaverUnInhibitCookie)
        {
          timerScreenSaverUnInhibit_.start();
        }

        done = true;
      }
    }

    if (!done)
    {
      // FIXME: not sure how to do this yet
      std::cerr << "screenSaverInhibit" << std::endl;
    }
#endif
  }

  //----------------------------------------------------------------
  // Canvas::screenSaverUnInhibit
  //
  void
  Canvas::screenSaverUnInhibit()
  {
#if !defined(__APPLE__) && !defined(_WIN32)
    if (screenSaverUnInhibitCookie &&
        QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        screensaver.call(QDBus::NoBlock,
                         "UnInhibit",
                         QVariant(screenSaverUnInhibitCookie));
        screenSaverUnInhibitCookie = 0;
      }
    }
#endif
  }
}
