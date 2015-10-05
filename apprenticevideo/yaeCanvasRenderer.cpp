// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <algorithm>
#include <deque>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>

// boost includes:
#include <boost/thread.hpp>

// local includes:
#include "yaeCanvasRenderer.h"


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
bool
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
// YAE_HAS_GL_ACTIVE_TEXTURE
//
#define YAE_HAS_GL_ACTIVE_TEXTURE 1

namespace yae
{

  //----------------------------------------------------------------
  // YAE_GL_FRAGMENT_PROGRAM_ARB
  //
  struct YAE_API OpenGLFunctionPointers
#if YAE_HAS_GL_ACTIVE_TEXTURE
    : public QOpenGLFunctions
#endif
  {
    typedef void (*TProgramStringARB)(GLenum target,
                                      GLenum format,
                                      GLsizei len,
                                      const void * string);
    TProgramStringARB glProgramStringARB;

    typedef void (*TGetProgramivARB)(GLenum target,
                                     GLenum pname,
                                     GLint * params);
    TGetProgramivARB glGetProgramivARB;

    typedef void (*TDeleteProgramsARB)(GLsizei n,
                                       const GLuint * programs);
    TDeleteProgramsARB glDeleteProgramsARB;

    typedef void (*TBindProgramARB)(GLenum target, GLuint program);
    TBindProgramARB glBindProgramARB;

    typedef void (*TGenProgramsARB)(GLsizei n, GLuint * programs);
    TGenProgramsARB glGenProgramsARB;

    typedef void (* TProgramLocalParameter4dvARB)(GLenum target,
                                                  GLuint index,
                                                  const GLdouble * params);
    TProgramLocalParameter4dvARB glProgramLocalParameter4dvARB;

    OpenGLFunctionPointers()
    {
      QOpenGLContext * opengl = QOpenGLContext::currentContext();

      glProgramStringARB = (TProgramStringARB)
        opengl->getProcAddress("glProgramStringARB");

      glGetProgramivARB = (TGetProgramivARB)
        opengl->getProcAddress("glGetProgramivARB");

      glDeleteProgramsARB = (TDeleteProgramsARB)
        opengl->getProcAddress("glDeleteProgramsARB");

      glBindProgramARB = (TBindProgramARB)
        opengl->getProcAddress("glBindProgramARB");

      glGenProgramsARB = (TGenProgramsARB)
        opengl->getProcAddress("glGenProgramsARB");

      glProgramLocalParameter4dvARB = (TProgramLocalParameter4dvARB)
        opengl->getProcAddress("glProgramLocalParameter4dvARB");
    }

    static OpenGLFunctionPointers & get();
  };

  //----------------------------------------------------------------
  // OpenGLFunctionPointers::get
  //
  OpenGLFunctionPointers &
  OpenGLFunctionPointers::get()
  {
    static OpenGLFunctionPointers singleton;
    return singleton;
  }
}

//----------------------------------------------------------------
// yae_reset_opengl_to_initial_state
//
void
yae_reset_opengl_to_initial_state()
{
  yae::OpenGLFunctionPointers & opengl = yae::OpenGLFunctionPointers::get();

  opengl.glBindBuffer(GL_ARRAY_BUFFER, 0);
  yae_assert_gl_no_error();

  opengl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  yae_assert_gl_no_error();

  int maxAttribs = 0;
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
  yae_assert_gl_no_error();

  for (int i = 0; i < maxAttribs; ++i)
  {
    opengl.glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
    yae_assert_gl_no_error();

    opengl.glDisableVertexAttribArray(i);
    yae_assert_gl_no_error();
  }

  opengl.glActiveTexture(GL_TEXTURE0);
  yae_assert_gl_no_error();

  glBindTexture(GL_TEXTURE_2D, 0);
  yae_assert_gl_no_error();

  glDisable(GL_LIGHTING);
  yae_assert_gl_no_error();

  glDisable(GL_FOG);
  yae_assert_gl_no_error();

  glDisable(GL_DEPTH_TEST);
  yae_assert_gl_no_error();

  glDisable(GL_STENCIL_TEST);
  yae_assert_gl_no_error();

  glDisable(GL_SCISSOR_TEST);
  yae_assert_gl_no_error();

  glColorMask(true, true, true, true);
  yae_assert_gl_no_error();

  glClearColor(0, 0, 0, 0);
  yae_assert_gl_no_error();

  glDepthMask(true);
  yae_assert_gl_no_error();

  glDepthFunc(GL_LESS);
  yae_assert_gl_no_error();

  glClearDepth(1);
  yae_assert_gl_no_error();

  glStencilMask(0xff);
  yae_assert_gl_no_error();

  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  yae_assert_gl_no_error();

  glStencilFunc(GL_ALWAYS, 0, 0xff);
  yae_assert_gl_no_error();

  glDisable(GL_POLYGON_OFFSET_FILL);
  yae_assert_gl_no_error();

  glDisable(GL_LINE_SMOOTH);
  yae_assert_gl_no_error();

  glDisable(GL_ALPHA_TEST);
  yae_assert_gl_no_error();

  glDisable(GL_TEXTURE_2D);
  yae_assert_gl_no_error();

  glDisable(GL_BLEND);
  yae_assert_gl_no_error();

  glBlendFunc(GL_ONE, GL_ZERO);
  yae_assert_gl_no_error();

  opengl.glUseProgram(0);
  yae_assert_gl_no_error();

  glShadeModel(GL_FLAT);
  glClearDepth(0);
  glClearStencil(0);
  glClearAccum(0, 0, 0, 0);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glAlphaFunc(GL_ALWAYS, 0.0f);
}

//----------------------------------------------------------------
// load_arb_program_natively
//
static bool
load_arb_program_natively(yae::OpenGLFunctionPointers & opengl,
                          GLenum target,
                          const char * prog)
{
  std::size_t len = strlen(prog);
  opengl.glProgramStringARB(target,
                            GL_PROGRAM_FORMAT_ASCII_ARB,
                            (GLsizei)len,
                            prog);
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
  opengl.glGetProgramivARB(target,
                           GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,
                           &isNative);

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
  // TGLSaveState::TGLSaveState
  //
  TGLSaveState::TGLSaveState(GLbitfield mask):
    applied_(false)
  {
    glPushAttrib(mask);
    applied_ = yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // TGLSaveState::~TGLSaveState
  //
  TGLSaveState::~TGLSaveState()
  {
    if (applied_)
    {
      glPopAttrib();
    }
  }


  //----------------------------------------------------------------
  // TGLSaveClientState::TGLSaveClientState
  //
  TGLSaveClientState::TGLSaveClientState(GLbitfield mask):
    applied_(false)
  {
    glPushClientAttrib(mask);
    applied_ = yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // TGLSaveClientState::~TGLSaveClientState
  //
  TGLSaveClientState::~TGLSaveClientState()
  {
    if (applied_)
    {
      glPopClientAttrib();
    }
  }


  //----------------------------------------------------------------
  // TGLSaveMatrixState::TGLSaveMatrixState
  //
  TGLSaveMatrixState::TGLSaveMatrixState(GLenum mode):
    matrixMode_(mode)
  {
    glMatrixMode(matrixMode_);
    glPushMatrix();
  }

  //----------------------------------------------------------------
  // TGLSaveMatrixState::~TGLSaveMatrixState
  //
  TGLSaveMatrixState::~TGLSaveMatrixState()
  {
    glMatrixMode(matrixMode_);
    glPopMatrix();
  }


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
  // TFragmentShaderProgram
  //
  struct YAE_API TFragmentShaderProgram
  {
    TFragmentShaderProgram(const char * code = NULL);

    // delete the program:
    void destroy();

    // helper:
    inline bool loaded() const
    { return code_ && handle_; }

    // GL_ARB_fragment_program source code:
    const char * code_;

    // GL_ARB_fragment_program handle:
    GLuint handle_;
  };

  //----------------------------------------------------------------
  // TFragmentShader
  //
  struct YAE_API TFragmentShader
  {
    TFragmentShader(const TFragmentShaderProgram * program = NULL,
                    TPixelFormatId format = kInvalidPixelFormat);

    // pointer to the shader program:
    const TFragmentShaderProgram * program_;

    // number of texture objects required for this pixel format:
    unsigned char numPlanes_;

    // sample stride per texture object:
    unsigned char stride_[4];

    // sample plane (sub)sampling per texture object:
    unsigned char subsample_x_[4];
    unsigned char subsample_y_[4];

    GLint internalFormatGL_[4];
    GLenum pixelFormatGL_[4];
    GLenum dataTypeGL_[4];
    GLenum magFilterGL_[4];
    GLenum minFilterGL_[4];
    GLint shouldSwapBytes_[4];
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
      OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();
      opengl.glDeleteProgramsARB(1, &handle_);
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
  struct TBaseCanvas
  {
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

    virtual void clear(IOpenGLContext & context) = 0;

    virtual bool loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame) = 0;

    virtual void draw() = 0;

    // helper:
    inline const pixelFormat::Traits * pixelTraits() const
    {
      return (frame_ ?
              pixelFormat::getTraits(frame_->traits_.pixelFormat_) :
              NULL);
    }

    void skipColorConverter(IOpenGLContext & context, bool enable)
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
        loadFrame(context, frame);
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

#if 0 // !defined(NDEBUG)
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
#if 0 // !defined(NDEBUG)
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
      OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();

      bool ok = false;
      builtinShaderProgram_.destroy();
      builtinShaderProgram_.code_ = code;

      glEnable(GL_FRAGMENT_PROGRAM_ARB);

      opengl.glGenProgramsARB(1, &builtinShaderProgram_.handle_);
      opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                              builtinShaderProgram_.handle_);

      if (load_arb_program_natively(opengl,
                                    GL_FRAGMENT_PROGRAM_ARB,
                                    builtinShaderProgram_.code_))
      {
        builtinShader_.program_ = &builtinShaderProgram_;
        ok = true;
      }
      else
      {
        opengl.glDeleteProgramsARB(1, &builtinShaderProgram_.handle_);
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
      OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();

      bool ok = false;
      TFragmentShaderProgram program(code);

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      opengl.glGenProgramsARB(1, &program.handle_);
      opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program.handle_);

      if (load_arb_program_natively(opengl,
                                    GL_FRAGMENT_PROGRAM_ARB,
                                    program.code_))
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
        opengl.glDeleteProgramsARB(1, &program.handle_);
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
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

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
  TModernCanvas::clear(IOpenGLContext & context)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(context);

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
  TModernCanvas::loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame)
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
    TMakeCurrentContext currentContext(context);

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
      OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();

      if (colorSpaceOrRangeChanged && frame->traits_.initAbcToRgbMatrix_)
      {
        frame->traits_.initAbcToRgbMatrix_(&m34_to_rgb_[0], vtts);
      }

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                              shader_->program_->handle_);
      {
        // pass the color transform matrix to the shader:
        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                             0, &m34_to_rgb_[0]);
        yae_assert_gl_no_error();

        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                             1, &m34_to_rgb_[4]);
        yae_assert_gl_no_error();

        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                             2, &m34_to_rgb_[8]);
        yae_assert_gl_no_error();

        // pass the subsampling factors to the shader:
        GLdouble subsample_uv[4] = { 1.0 };
        subsample_uv[0] = 1.0 / double(ptts->chromaBoxW_);
        subsample_uv[1] = 1.0 / double(ptts->chromaBoxH_);

        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
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

    OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();
#if YAE_HAS_GL_ACTIVE_TEXTURE
    opengl.glActiveTexture(GL_TEXTURE0);
    yae_assert_gl_no_error();
#endif

    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

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
        opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                shader_->program_->handle_);
      }

#if YAE_HAS_GL_ACTIVE_TEXTURE
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        opengl.glActiveTexture((GLenum)(GL_TEXTURE0 + k));
        yae_assert_gl_no_error();

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[k + i]);
      }
#else
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]);
#endif

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
#if YAE_HAS_GL_ACTIVE_TEXTURE
    for (std::size_t k = 0; k < shader.numPlanes_; k++)
    {
      opengl.glActiveTexture((GLenum)(GL_TEXTURE0 + k));
      yae_assert_gl_no_error();

      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }
#else
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
#endif

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
    void clear(IOpenGLContext & context);

    // virtual:
    bool loadFrame(IOpenGLContext & context, const TVideoFramePtr & frame);

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
  TLegacyCanvas::clear(IOpenGLContext & context)
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(context);

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
  TLegacyCanvas::loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame)
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
    TMakeCurrentContext currentContext(context);

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
      OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();

      if (colorSpaceOrRangeChanged && frame->traits_.initAbcToRgbMatrix_)
      {
        frame->traits_.initAbcToRgbMatrix_(&m34_to_rgb_[0], vtts);
      }

      glEnable(GL_FRAGMENT_PROGRAM_ARB);
      opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                              shader_->program_->handle_);
      {
        // pass the color transform matrix to the shader:
        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                             0, &m34_to_rgb_[0]);
        yae_assert_gl_no_error();

        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                             1, &m34_to_rgb_[4]);
        yae_assert_gl_no_error();

        opengl.glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
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

    double iw = 0.0;
    double ih = 0.0;
    imageWidthHeight(iw, ih);

    TCropFrame crop;
    getCroppedFrame(crop);

    OpenGLFunctionPointers & opengl = OpenGLFunctionPointers::get();
#if YAE_HAS_GL_ACTIVE_TEXTURE
    opengl.glActiveTexture(GL_TEXTURE0);
    yae_assert_gl_no_error();
#endif

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
        opengl.glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                shader_->program_->handle_);
      }

#if YAE_HAS_GL_ACTIVE_TEXTURE
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        opengl.glActiveTexture((GLenum)(GL_TEXTURE0 + k));
        yae_assert_gl_no_error();

        glBindTexture(GL_TEXTURE_2D, texId_[k + i * shader.numPlanes_]);
      }
#else
      glBindTexture(GL_TEXTURE_2D, texId_[i * shader.numPlanes_]);
#endif

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
#if YAE_HAS_GL_ACTIVE_TEXTURE
    for (std::size_t k = 0; k < shader.numPlanes_; k++)
    {
      opengl.glActiveTexture((GLenum)(GL_TEXTURE0 + k));
      yae_assert_gl_no_error();

      glBindTexture(GL_TEXTURE_2D, 0);
    }
#else
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    if (shader_)
    {
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
    }

    glDisable(GL_TEXTURE_2D);
  }


  //----------------------------------------------------------------
  // CanvasRenderer::CanvasRenderer
  //
  CanvasRenderer::CanvasRenderer():
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

  //----------------------------------------------------------------
  // CanvasRenderer::~CanvasRenderer
  //
  CanvasRenderer::~CanvasRenderer()
  {
    delete legacy_;
    delete modern_;
  }

  //----------------------------------------------------------------
  // CanvasRenderer::clear
  //
  void
  CanvasRenderer::clear(IOpenGLContext & context)
  {
    renderer_->clear(context);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::rendererFor
  //
  TBaseCanvas *
  CanvasRenderer::rendererFor(const VideoTraits & vtts) const
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

  //----------------------------------------------------------------
  // CanvasRenderer::loadFrame
  //
  bool
  CanvasRenderer::loadFrame(IOpenGLContext & context,
                            const TVideoFramePtr & frame)
  {
    if (modern_)
    {
      TBaseCanvas * renderer = rendererFor(frame->traits_);
      if (renderer != renderer_)
      {
        // switch to a different renderer:
        renderer_->clear(context);
        renderer_ = renderer;
      }
    }

    return renderer_->loadFrame(context, frame);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::draw
  //
  void
  CanvasRenderer::draw()
  {
    renderer_->draw();
  }

  //----------------------------------------------------------------
  // CanvasRenderer::pixelTraits
  //
  const pixelFormat::Traits *
  CanvasRenderer::pixelTraits() const
  {
    return renderer_->pixelTraits();
  }

  //----------------------------------------------------------------
  // CanvasRenderer::skipColorConverter
  //
  void
  CanvasRenderer::skipColorConverter(IOpenGLContext & context, bool enable)
  {
    legacy_->skipColorConverter(context, enable);

    if (modern_)
    {
      modern_->skipColorConverter(context, enable);
    }
  }

  //----------------------------------------------------------------
  // CanvasRenderer::enableVerticalScaling
  //
  void
  CanvasRenderer::enableVerticalScaling(bool enable)
  {
    legacy_->enableVerticalScaling(enable);

    if (modern_)
    {
      modern_->enableVerticalScaling(enable);
    }
  }

  //----------------------------------------------------------------
  // CanvasRenderer::getCroppedFrame
  //
  bool
  CanvasRenderer::getCroppedFrame(TCropFrame & crop) const
  {
    return renderer_->getCroppedFrame(crop);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::imageWidthHeight
  //
  bool
  CanvasRenderer::imageWidthHeight(double & w, double & h) const
  {
    return renderer_->imageWidthHeight(w, h);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::imageWidthHeightRotated
  //
  bool
  CanvasRenderer::imageWidthHeightRotated(double & w,
                                          double & h,
                                          int & rotate) const
  {
    return renderer_->imageWidthHeightRotated(w, h, rotate);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::overrideDisplayAspectRatio
  //
  void
  CanvasRenderer::overrideDisplayAspectRatio(double dar)
  {
    legacy_->overrideDisplayAspectRatio(dar);

    if (modern_)
    {
      modern_->overrideDisplayAspectRatio(dar);
    }
  }

  //----------------------------------------------------------------
  // CanvasRenderer::cropFrame
  //
  void
  CanvasRenderer::cropFrame(double darCropped)
  {
    legacy_->cropFrame(darCropped);

    if (modern_)
    {
      modern_->cropFrame(darCropped);
    }
  }

  //----------------------------------------------------------------
  // CanvasRenderer::cropFrame
  //
  void
  CanvasRenderer::cropFrame(const TCropFrame & crop)
  {
    legacy_->cropFrame(crop);

    if (modern_)
    {
      modern_->cropFrame(crop);
    }
  }

  //----------------------------------------------------------------
  // CanvasRenderer::getFrame
  //
  void
  CanvasRenderer::getFrame(TVideoFramePtr & frame) const
  {
    renderer_->getFrame(frame);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::fragmentShaderFor
  //
  const TFragmentShader *
  CanvasRenderer::fragmentShaderFor(const VideoTraits & vtts) const
  {
    TBaseCanvas * renderer = rendererFor(vtts);
    return renderer->fragmentShaderFor(vtts.pixelFormat_);
  }

}
