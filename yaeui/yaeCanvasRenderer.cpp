// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/utils/yae_benchmark.h"

// standard:
#include <algorithm>
#include <deque>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <set>
#include <sstream>
#include <string.h>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/thread.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// yaeui:
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
    ostr << '\n' << errorMessage << '\n';
  }
}

//----------------------------------------------------------------
// yae_gl_arb_passthrough_2d
//
static const char * yae_gl_arb_passthrough_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale = program.local[1];\n"
  "TEMP rgba;\n"
  "TEX rgba, fragment.texcoord[0], texture[0], 2D;\n"
  "MUL rgba, rgba, rescale;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_passthrough_clut_2d
//
static const char * yae_gl_arb_passthrough_clut_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP rgba_in;\n"
  "TEX rgba_in, fragment.texcoord[0], texture[0], 2D;\n"
  "TEMP rgba;\n"
  "MOV rgba, rgba_in.zyxw;\n"
  "MUL rgba, rgba, rescale_to_full;\n"
  "MUL rgba, rgba, rescale_to_clut;\n"
  "TEX rgba, rgba, texture[1], 3D;\n"
  "MOV rgba.a, rgba_in.a;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuv_to_rgb_2d
//
static const char * yae_gl_arb_yuv_to_rgb_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP yuv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX yuv.y, fragment.texcoord[0], texture[1], 2D;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[2], 2D;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[3], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_nv12_to_rgb_2d
//
static const char * yae_gl_arb_nv12_to_rgb_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP yuv;\n"
  "TEMP uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX uv, fragment.texcoord[0], texture[1], 2D;\n"
  "MOV yuv.y, uv.r;\n"
  "MOV yuv.x, uv.a;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[2], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_nv21_to_rgb_2d
//
static const char * yae_gl_arb_nv21_to_rgb_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP yuv;\n"
  "TEMP uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX uv, fragment.texcoord[0], texture[1], 2D;\n"
  "MOV yuv.y, uv.a;\n"
  "MOV yuv.x, uv.r;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[2], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuva_to_rgba_2d
//
static const char * yae_gl_arb_yuva_to_rgba_2d =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP yuv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], 2D;\n"
  "TEX yuv.y, fragment.texcoord[0], texture[1], 2D;\n"
  "TEX yuv.x, fragment.texcoord[0], texture[2], 2D;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[4], 3D;\n"
  "TEX rgba.a, fragment.texcoord[0], texture[3], 2D;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_passthrough
//
static const char * yae_gl_arb_passthrough =
  "!!ARBfp1.0\n"
  "PARAM rescale = program.local[1];\n"
  "TEMP rgba;\n"
  "TEX rgba, fragment.texcoord[0], texture[0], RECT;\n"
  "MUL rgba, rgba, rescale;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_passthrough_clut
//
static const char * yae_gl_arb_passthrough_clut =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "TEMP rgba_in;\n"
  "TEX rgba_in, fragment.texcoord[0], texture[0], RECT;\n"
  "TEMP rgba;\n"
  "MOV rgba, rgba_in.zyxw;\n"
  "MUL rgba, rgba, rescale_to_full;\n"
  "MUL rgba, rgba, rescale_to_clut;\n"
  "TEX rgba, rgba, texture[1], 3D;\n"
  "MOV rgba.a, rgba_in.a;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuv_to_rgb
//
static const char * yae_gl_arb_yuv_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "PARAM subsample_uv = program.local[2];\n"
  "TEMP yuv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX yuv.y, coord_uv, texture[1], RECT;\n"
  "TEX yuv.x, coord_uv, texture[2], RECT;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[3], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuva_to_rgba
//
static const char * yae_gl_arb_yuva_to_rgba =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "PARAM subsample_uv = program.local[2];\n"
  "TEMP yuv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX yuv.y, coord_uv, texture[1], RECT;\n"
  "TEX yuv.x, coord_uv, texture[2], RECT;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[4], 3D;\n"
  "TEX rgba.a, fragment.texcoord[0], texture[3], RECT;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";


// FIXME: pkoshevoy:
//
// YUYV and UYVY could be handled far simpler using one RGBA texture,
// and it wouldn't require NEAREST sampling and manual anti-aliasing
// either...

//----------------------------------------------------------------
// yae_gl_arb_yuyv_to_rgb_antialias
//
static const char * yae_gl_arb_yuyv_to_rgb_antialias =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"

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
  "MOV yuv, yuv.zyxw;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[1], 3D;\n"

  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_uyvy_to_rgb_antialias
//
static const char * yae_gl_arb_uyvy_to_rgb_antialias =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"

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
  "MOV yuv, yuv.zyxw;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[1], 3D;\n"

  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_yuyv_to_rgb
//
static const char * yae_gl_arb_yuyv_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"

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
  "MOV yuv, yuv.zyxw;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[1], 3D;\n"

  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_uyvy_to_rgb
//
static const char * yae_gl_arb_uyvy_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"

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
  "MOV yuv, yuv.zyxw;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[1], 3D;\n"

  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_nv12_to_rgb
//
static const char * yae_gl_arb_nv12_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "PARAM subsample_uv = program.local[2];\n"
  "TEMP yuv;\n"
  "TEMP uv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX uv, coord_uv, texture[1], RECT;\n"
  "MOV yuv.y, uv.r;\n"
  "MOV yuv.x, uv.a;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[2], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
  "END\n";

//----------------------------------------------------------------
// yae_gl_arb_nv21_to_rgb
//
static const char * yae_gl_arb_nv21_to_rgb =
  "!!ARBfp1.0\n"
  "PARAM rescale_to_clut = program.local[0];\n"
  "PARAM rescale_to_full = program.local[1];\n"
  "PARAM subsample_uv = program.local[2];\n"
  "TEMP yuv;\n"
  "TEMP uv;\n"
  "TEMP coord_uv;\n"
  "MUL coord_uv, fragment.texcoord[0], subsample_uv;\n"
  "TEX yuv.z, fragment.texcoord[0], texture[0], RECT;\n"
  "TEX uv, coord_uv, texture[1], RECT;\n"
  "MOV yuv.y, uv.a;\n"
  "MOV yuv.x, uv.r;\n"
  "MUL yuv, yuv, rescale_to_full;\n"
  "MUL yuv, yuv, rescale_to_clut;\n"
  "TEMP rgba;\n"
  "TEX rgba, yuv, texture[2], 3D;\n"
  "MOV rgba.a, 1.0;\n"
  "MUL result.color, fragment.color, rgba;\n"
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
    YAE_OPENGL_HERE();
    const GLubyte * extensions = YAE_OPENGL(glGetString(GL_EXTENSIONS));
    if (!extensions)
    {
      return false;
    }

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
    case yae::kPixelFormatYUVA422P:
    case yae::kPixelFormatYUVA444P:
    case yae::kPixelFormatYUVJ420P:
      //! planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples), JPEG
    case yae::kPixelFormatYUVJ422P:
      //! planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ444P:
      //! planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples), JPEG
    case yae::kPixelFormatYUVJ440P:
      //! planar YUV 4:4:0, 16bpp, (1 Cr & Cb sample per 1x2 Y samples), JPEG
    case yae::kPixelFormatNV24:
      //! semi-planar YUV 4:4:4, 24bpp
    case yae::kPixelFormatNV42:
      //! semi-planar YVU 4:4:4, 24bpp

      internalFormat = GL_LUMINANCE;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_BYTE;
      return 1;

    case yae::kPixelFormatYUV420P9BE:
    case yae::kPixelFormatYUV422P9BE:
    case yae::kPixelFormatYUV444P9BE:
      //! planar YUV, 9 bits per channel:
    case yae::kPixelFormatYUV420P10BE:
    case yae::kPixelFormatYUV422P10BE:
    case yae::kPixelFormatYUV444P10BE:
      //! planar YUV, 10 bits per channel:
    case yae::kPixelFormatP010BE:
      //! semi-planar YUV 4:2:0, 10bpp per component,
      //! data in the high bits, zero padding in the bottom 6 bits
    case yae::kPixelFormatP016BE:
      //! semi-planar YUV 4:2:0, 16bpp per component
#ifndef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
      return 1;

    case yae::kPixelFormatYUV420P9LE:
    case yae::kPixelFormatYUV422P9LE:
    case yae::kPixelFormatYUV444P9LE:
      //! planar YUV, 9 bits per channel:
    case yae::kPixelFormatYUV420P10LE:
    case yae::kPixelFormatYUV422P10LE:
    case yae::kPixelFormatYUV444P10LE:
      //! planar YUV, 10 bits per channel:
    case yae::kPixelFormatP010LE:
      //! semi-planar YUV 4:2:0, 10bpp per component,
      //! data in the high bits, zero padding in the bottom 6 bits
    case yae::kPixelFormatP016LE:
      //! semi-planar YUV 4:2:0, 16bpp per component
#ifdef __BIG_ENDIAN__
      shouldSwapBytes = GL_TRUE;
#endif
      internalFormat = GL_LUMINANCE16;
      format = GL_LUMINANCE;
      dataType = GL_UNSIGNED_SHORT;
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
// yae_opengl_debug_message_cb
//
extern "C" void
yae_opengl_debug_message_cb(GLenum src,
                            GLenum t,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar * message,
                            const void * userParam)
{
  if (severity != 0x826B) // GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    yae_error
      << "OpenGL debug message, source " << int(src)
      << ", type " << int(t)
      << ", id " << int(id)
      << ", severity " << int(severity)
      << ": " << std::string(message, message + length).c_str();
  }

  return;
}

namespace yaegl
{

  static int depth = 0;
  static std::list<GLenum> mode;

#ifndef YAE_USE_QGL_WIDGET
  static void begin(GLenum mode)
  {
    // yae_dlog("OpenGL (%i) begin: %i", depth, mode);
    yaegl::depth++;
    YAE_ASSERT(yaegl::depth == 1);
    OpenGLFunctionPointers::get()._glBegin(mode);
    yaegl::mode.push_back(mode);
  }

  static void end()
  {
    yaegl::depth--;
    // yae_dlog("OpenGL (%i) end", depth);
    YAE_ASSERT(depth == 0);
    OpenGLFunctionPointers::get()._glEnd();
    yaegl::mode.pop_back();
  }
#endif

  bool assert_no_error()
  {
    // NOTE: not allowed to call glGetError between glBegin/glEnd
    YAE_ASSERT(depth == 0);
    if (depth != 0)
    {
      return false;
    }

    YAE_OPENGL_HERE();
    GLenum err = YAE_OPENGL(glGetError());
    if (err == GL_NO_ERROR)
    {
      return true;
    }

    for (int i = 0; i < 10 && err != GL_NO_ERROR; i++)
    {
      yae_elog("glGetError: %i", err);
      err = YAE_OPENGL(glGetError());
    }

    //  YAE_ASSERT(false);
    // char *crash = NULL;
    // *crash = *crash;
    return false;
  }

#ifndef YAE_USE_QGL_WIDGET
  //----------------------------------------------------------------
  // get_addr
  //
  QFunctionPointer
  get_addr(QOpenGLContext * opengl, const char * name)
  {
    QFunctionPointer func = opengl->getProcAddress(name);
    if (!func)
    {
      yae_elog("OpenGL function not found: %s", name);
      YAE_ASSERT(false);
    }

    return func;
  }

  //----------------------------------------------------------------
  // OpenGLFunctionPointers::OpenGLFunctionPointers
  //
  OpenGLFunctionPointers::OpenGLFunctionPointers()
  {
    memset(this, 0, sizeof(*this));

    // QOpenGLFunctions::initializeOpenGLFunctions();
    QOpenGLContext * ctx = QOpenGLContext::currentContext();

    this->glDebugMessageCallback = (TDebugMessageCallback)
      get_addr(ctx, "glDebugMessageCallback");

    this->_glBegin = (TBegin)
      get_addr(ctx, "glBegin");

    this->_glEnd = (TEnd)
      get_addr(ctx, "glEnd");

    this->glBegin = &yaegl::begin;
    this->glEnd = &yaegl::end;

    this->glClear = (TClear)
      get_addr(ctx, "glClear");

    this->glClearAccum = (TClearAccum)
      get_addr(ctx, "glClearAccum");

    this->glClearColor = (TClearColor)
      get_addr(ctx, "glClearColor");

    this->glClearDepth = (TClearDepth)
      get_addr(ctx, "glClearDepth");

    this->glClearStencil = (TClearStencil)
      get_addr(ctx, "glClearStencil");

    this->glDepthFunc = (TDepthFunc)
      get_addr(ctx, "glDepthFunc");

    this->glDepthMask = (TDepthMask)
      get_addr(ctx, "glDepthMask");

    this->glColorMask = (TColorMask)
      get_addr(ctx, "glColorMask");

    this->glStencilFunc = (TStencilFunc)
      get_addr(ctx, "glStencilFunc");

    this->glStencilMask = (TStencilMask)
      get_addr(ctx, "glStencilMask");

    this->glStencilOp = (TStencilOp)
      get_addr(ctx, "glStencilOp");

    this->glColor3d = (TColor3d)
      get_addr(ctx, "glColor3d");

    this->glColor3f = (TColor3f)
      get_addr(ctx, "glColor3f");

    this->glColor3fv = (TColor3fv)
      get_addr(ctx, "glColor3fv");

    this->glColor4d = (TColor4d)
      get_addr(ctx, "glColor4d");

    this->glColor4ub = (TColor4ub)
      get_addr(ctx, "glColor4ub");

    this->glVertex2d = (TVertex2d)
      get_addr(ctx, "glVertex2d");

    this->glVertex2dv = (TVertex2dv)
      get_addr(ctx, "glVertex2dv");

    this->glVertex2i = (TVertex2i)
      get_addr(ctx, "glVertex2i");

    this->glRectd = (TRectd)
      get_addr(ctx, "glRectd");

    this->glRecti = (TRecti)
      get_addr(ctx, "glRecti");

    this->glMatrixMode = (TMatrixMode)
      get_addr(ctx, "glMatrixMode");

    this->glPushMatrix = (TPushMatrix)
      get_addr(ctx, "glPushMatrix");

    this->glPopMatrix = (TPopMatrix)
      get_addr(ctx, "glPopMatrix");

    this->glViewport = (TViewport)
      get_addr(ctx, "glViewport");

    this->glOrtho = (TOrtho)
      get_addr(ctx, "glOrtho");

    this->glLoadIdentity = (TLoadIdentity)
      get_addr(ctx, "glLoadIdentity");

    this->glRotated = (TRotated)
      get_addr(ctx, "glRotated");

    this->glScaled = (TScaled)
      get_addr(ctx, "glScaled");

    this->glTranslated = (TTranslated)
      get_addr(ctx, "glTranslated");

    this->glPolygonMode = (TPolygonMode)
      get_addr(ctx, "glPolygonMode");

    this->glShadeModel = (TShadeModel)
      get_addr(ctx, "glShadeModel");

    this->glAlphaFunc = (TAlphaFunc)
      get_addr(ctx, "glAlphaFunc");

    this->glPushAttrib = (TPushAttrib)
      get_addr(ctx, "glPushAttrib");

    this->glPopAttrib = (TPopAttrib)
      get_addr(ctx, "glPopAttrib");

    this->glPushClientAttrib = (TPushClientAttrib)
      get_addr(ctx, "glPushClientAttrib");

    this->glPopClientAttrib = (TPopClientAttrib)
      get_addr(ctx, "glPopClientAttrib");

    this->glEnable = (TEnable)
      get_addr(ctx, "glEnable");

    this->glDisable = (TDisable)
      get_addr(ctx, "glDisable");

    this->glHint = (THint)
      get_addr(ctx, "glHint");

    this->glBlendFunc = (TBlendFunc)
      get_addr(ctx, "glBlendFunc");

    this->glBlendFuncSeparate = (TBlendFuncSeparate)
      get_addr(ctx, "glBlendFuncSeparate");

    this->glLineStipple = (TLineStipple)
      get_addr(ctx, "glLineStipple");

    this->glLineWidth = (TLineWidth)
      get_addr(ctx, "glLineWidth");

    this->glScissor = (TScissor)
      get_addr(ctx, "glScissor");

    this->glBindBuffer = (TBindBuffer)
      get_addr(ctx, "glBindBuffer");

    this->glCheckFramebufferStatus = (TCheckFramebufferStatus)
      get_addr(ctx, "glCheckFramebufferStatus");

    this->glDeleteTextures = (TDeleteTextures)
      get_addr(ctx, "glDeleteTextures");

    this->glGenTextures = (TGenTextures)
      get_addr(ctx, "glGenTextures");

    this->glGetError = (TGetError)
      get_addr(ctx, "glGetError");

    this->glGetString = (TGetString)
      get_addr(ctx, "glGetString");

    this->glGetIntegerv = (TGetIntegerv)
      get_addr(ctx, "glGetIntegerv");

    this->glGetTexLevelParameteriv = (TGetTexLevelParameteriv)
      get_addr(ctx, "glGetTexLevelParameteriv");

    this->glIsTexture = (TIsTexture)
      get_addr(ctx, "glIsTexture");

    this->glTexEnvi = (TTexEnvi)
      get_addr(ctx, "glTexEnvi");

    this->glTexCoord2d = (TTexCoord2d)
      get_addr(ctx, "glTexCoord2d");

    this->glTexCoord2i = (TTexCoord2i)
      get_addr(ctx, "glTexCoord2i");

    this->glTexImage2D = (TTexImage2D)
      get_addr(ctx, "glTexImage2D");

    this->glTexImage3D = (TTexImage3D)
      get_addr(ctx, "glTexImage3D");

    this->glTexSubImage2D = (TTexSubImage2D)
      get_addr(ctx, "glTexSubImage2D");

    this->glTexParameteri = (TTexParameteri)
      get_addr(ctx, "glTexParameteri");

    this->glPixelStorei = (TPixelStorei)
      get_addr(ctx, "glPixelStorei");

    this->glActiveTexture = (TActiveTexture)
      get_addr(ctx, "glActiveTexture");

    this->glBindTexture = (TBindTexture)
      get_addr(ctx, "glBindTexture");

    this->glDisableVertexAttribArray = (TDisableVertexAttribArray)
      get_addr(ctx, "glDisableVertexAttribArray");

    this->glVertexAttribPointer = (TVertexAttribPointer)
      get_addr(ctx, "glVertexAttribPointer");

    this->glUseProgram = (TUseProgram)
      get_addr(ctx, "glUseProgram");

    this->glProgramStringARB = (TProgramStringARB)
      get_addr(ctx, "glProgramStringARB");

    this->glGetProgramivARB = (TGetProgramivARB)
      get_addr(ctx, "glGetProgramivARB");

    this->glDeleteProgramsARB = (TDeleteProgramsARB)
      get_addr(ctx, "glDeleteProgramsARB");

    this->glBindProgramARB = (TBindProgramARB)
      get_addr(ctx, "glBindProgramARB");

    this->glGenProgramsARB = (TGenProgramsARB)
      get_addr(ctx, "glGenProgramsARB");

    this->glProgramLocalParameter4dvARB = (TProgramLocalParameter4dvARB)
      get_addr(ctx, "glProgramLocalParameter4dvARB");

    this->glProgramLocalParameter4dARB = (TProgramLocalParameter4dARB)
      get_addr(ctx, "glProgramLocalParameter4dARB");
  }

  //----------------------------------------------------------------
  // OpenGLFunctionPointers::get
  //
  OpenGLFunctionPointers &
  OpenGLFunctionPointers::get()
  {
    static OpenGLFunctionPointers singleton;
    return singleton;
  }
#endif
}

//----------------------------------------------------------------
// yae_reset_opengl_to_initial_state
//
void
yae_reset_opengl_to_initial_state()
{
  YAE_OPENGL_HERE();
  yae_assert_gl_no_error();

#ifndef YAE_USE_QGL_WIDGET
  YAE_OPENGL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  yae_assert_gl_no_error();

  YAE_OPENGL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  yae_assert_gl_no_error();
#endif

  int maxAttribs = 0;
  YAE_OPENGL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs));
  yae_assert_gl_no_error();

#ifndef YAE_USE_QGL_WIDGET
  for (int i = 0; i < maxAttribs; ++i)
  {
    YAE_OPENGL(glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0));
    yae_assert_gl_no_error();

    YAE_OPENGL(glDisableVertexAttribArray(i));
    yae_assert_gl_no_error();
  }
#endif

  if (YAE_OGL_FN(glActiveTexture))
  {
    YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
    yae_assert_gl_no_error();
  }

  YAE_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_LIGHTING));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_FOG));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_DEPTH_TEST));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_STENCIL_TEST));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_SCISSOR_TEST));
  yae_assert_gl_no_error();

  YAE_OPENGL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
  yae_assert_gl_no_error();

  YAE_OPENGL(glClearColor(0, 0, 0, 1));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDepthMask(true));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDepthFunc(GL_LESS));
  yae_assert_gl_no_error();

  YAE_OPENGL(glClearDepth(1));
  yae_assert_gl_no_error();

  YAE_OPENGL(glStencilMask(0xff));
  yae_assert_gl_no_error();

  YAE_OPENGL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
  yae_assert_gl_no_error();

  YAE_OPENGL(glStencilFunc(GL_ALWAYS, 0, 0xff));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_POLYGON_OFFSET_FILL));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_LINE_SMOOTH));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_ALPHA_TEST));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_TEXTURE_2D));
  yae_assert_gl_no_error();

  YAE_OPENGL(glDisable(GL_BLEND));
  yae_assert_gl_no_error();

  YAE_OPENGL(glBlendFunc(GL_ONE, GL_ZERO));
  yae_assert_gl_no_error();

  if (YAE_OGL_FN(glBlendFuncSeparate))
  {
    // for Wayland compatibility:
    YAE_OPENGL(glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO));
    yae_assert_gl_no_error();
  }

#ifndef YAE_USE_QGL_WIDGET
  YAE_OPENGL(glUseProgram(0));
  yae_assert_gl_no_error();
#endif

  YAE_OPENGL(glShadeModel(GL_FLAT));
  YAE_OPENGL(glClearDepth(1));
  YAE_OPENGL(glClearStencil(0));
  YAE_OPENGL(glClearAccum(0, 0, 0, 0));
  YAE_OPENGL(glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST));
  YAE_OPENGL(glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST));
  YAE_OPENGL(glAlphaFunc(GL_ALWAYS, 0.0f));
}

//----------------------------------------------------------------
// load_arb_program_natively
//
static bool
load_arb_program_natively(GLenum target, const char * prog)
{
  YAE_OPENGL_HERE();

  std::size_t len = strlen(prog);
  YAE_OPENGL(glProgramStringARB(target,
                                GL_PROGRAM_FORMAT_ASCII_ARB,
                                (GLsizei)len,
                                prog));
  GLenum err = YAE_OPENGL(glGetError());
  (void)err;

  GLint errorPos = -1;
  YAE_OPENGL(glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos));

#if !defined(NDEBUG)
  if (errorPos < (GLint)len && errorPos >= 0)
  {
    const GLubyte * err = YAE_OPENGL(glGetString(GL_PROGRAM_ERROR_STRING_ARB));
    std::ostringstream oss;
    yae_show_program_listing(oss, prog, len, (const char *)err);
    yae_debug << oss.str();
  }
#endif

  GLint isNative = 0;
  YAE_OPENGL(glGetProgramivARB(target,
                               GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB,
                               &isNative));

  if (errorPos == -1 &&
      isNative == 1)
  {
    return true;
  }

  YAE_ASSERT(false);
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
    YAE_OPENGL_HERE();
    YAE_OPENGL(glPushAttrib(mask));
    applied_ = yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // TGLSaveState::~TGLSaveState
  //
  TGLSaveState::~TGLSaveState()
  {
    if (applied_)
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glPopAttrib());
    }
  }


  //----------------------------------------------------------------
  // TGLSaveClientState::TGLSaveClientState
  //
  TGLSaveClientState::TGLSaveClientState(GLbitfield mask):
    applied_(false)
  {
    YAE_OPENGL_HERE();
    YAE_OPENGL(glPushClientAttrib(mask));
    applied_ = yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // TGLSaveClientState::~TGLSaveClientState
  //
  TGLSaveClientState::~TGLSaveClientState()
  {
    if (applied_)
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glPopClientAttrib());
    }
  }


  //----------------------------------------------------------------
  // TGLSaveMatrixState::TGLSaveMatrixState
  //
  TGLSaveMatrixState::TGLSaveMatrixState(GLenum mode):
    matrixMode_(mode)
  {
    YAE_OPENGL_HERE();
    YAE_OPENGL(glMatrixMode(matrixMode_));
    YAE_OPENGL(glPushMatrix());
  }

  //----------------------------------------------------------------
  // TGLSaveMatrixState::~TGLSaveMatrixState
  //
  TGLSaveMatrixState::~TGLSaveMatrixState()
  {
    YAE_OPENGL_HERE();
    YAE_OPENGL(glMatrixMode(matrixMode_));
    YAE_OPENGL(glPopMatrix());
  }


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
      YAE_OPENGL_HERE();
      YAE_OPENGL(glDeleteProgramsARB(1, &handle_));
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
        else if (stride_bytes == 4 && ptts->depth_[0] > 8)
        {
          internalFormatGL_[numPlanes_] = (GLint)GL_LUMINANCE16_ALPHA16;
          pixelFormatGL_   [numPlanes_] = (GLenum)GL_LUMINANCE_ALPHA;
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

        if (ptts->flags_ & pixelFormat::kYUV && nchannels[i] > 2)
        {
          // YUYV, UYVY, should avoid linear filtering,
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
  // TBaseCanvas::TBaseCanvas
  //
  TBaseCanvas::TBaseCanvas(const ShaderPrograms & shaders):
    dar_(0.0),
    darCropped_(0.0),
    skipColorConverter_(false),
    verticalScalingEnabled_(false),
    shader_(NULL),
    clut_(5),
    clut_tex_id_(0)
  {
    clut_input_.av_fmt_ = AV_PIX_FMT_NONE;

    typedef std::map<TPixelFormatId, const TFragmentShaderProgram *> TProgs;
    for (TProgs::const_iterator i = shaders.lut_.begin();
         i != shaders.lut_.end(); ++i)
    {
      const TPixelFormatId & format = i->first;
      const TFragmentShaderProgram * program = i->second;
      shaders_[format] = TFragmentShader(program, format);
    }

    if (shaders.builtin_.handle_)
    {
      builtinShader_.program_ = &(shaders.builtin_);
    }
  }

  //----------------------------------------------------------------
  // TBaseCanvas::~TBaseCanvas
  //
  TBaseCanvas::~TBaseCanvas()
  {
    shader_ = NULL;
    shaders_.clear();
  }

  //----------------------------------------------------------------
  // TBaseCanvas::pixelTraits
  //
  const pixelFormat::Traits *
  TBaseCanvas::pixelTraits() const
  {
    return (frame_ ?
            pixelFormat::getTraits(frame_->traits_.pixelFormat_) :
            NULL);
  }

  //----------------------------------------------------------------
  // TBaseCanvas::skipColorConverter
  //
  void
  TBaseCanvas::skipColorConverter(IOpenGLContext & context, bool enable)
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

  //----------------------------------------------------------------
  // TBaseCanvas::enableVerticalScaling
  //
  void
  TBaseCanvas::enableVerticalScaling(bool enable)
  {
    verticalScalingEnabled_ = enable;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::getCroppedFrame
  //
  bool
  TBaseCanvas::getCroppedFrame(TCropFrame & crop) const
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

  //----------------------------------------------------------------
  // TBaseCanvas::imageWidthHeight
  //
  bool
  TBaseCanvas::imageWidthHeight(double & w, double & h) const
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

  //----------------------------------------------------------------
  // TBaseCanvas::imageWidthHeightRotated
  //
  bool
  TBaseCanvas::imageWidthHeightRotated(double & w,
                                       double & h,
                                       int & rotate) const
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

  //----------------------------------------------------------------
  // TBaseCanvas::nativeAspectRatioUncropped
  //
  double
  TBaseCanvas::nativeAspectRatioUncropped() const
  {
    if (frame_)
    {
      // video traits shortcut:
      const VideoTraits & vtts = frame_->traits_;

      double w = vtts.visibleWidth_;
      double h = vtts.visibleHeight_;

      if (!verticalScalingEnabled_)
      {
        if (vtts.pixelAspectRatio_ != 0.0)
        {
          w = floor(0.5 + w * vtts.pixelAspectRatio_);
        }
      }
      else
      {
        if (vtts.pixelAspectRatio_ > 1.0)
        {
          w = floor(0.5 + w * vtts.pixelAspectRatio_);
        }
        else if (vtts.pixelAspectRatio_ < 1.0)
        {
          h = floor(0.5 + h / vtts.pixelAspectRatio_);
        }
      }

      double dar = h ? (w / h) : 0.0;
      return dar;
    }

    return 0.0;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::nativeAspectRatioUncroppedRotated
  //
  double
  TBaseCanvas::nativeAspectRatioUncroppedRotated(int & rotate) const
  {
    double dar = nativeAspectRatioUncropped();

    if (dar)
    {
      // video traits shortcut:
      const VideoTraits & vtts = frame_->traits_;

      if (vtts.cameraRotation_ % 90 == 0)
      {
        // must be a camera phone video that needs to be
        // rotated for viewing:
        if (vtts.cameraRotation_ % 180 != 0)
        {
          dar = 1.0 / dar;
        }

        rotate = vtts.cameraRotation_;
      }
      else
      {
        rotate = 0;
      }
    }

    return dar;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::nativeAspectRatio
  //
  double
  TBaseCanvas::nativeAspectRatio() const
  {
    TCropFrame crop;
    if (getCroppedFrame(crop))
    {
      // video traits shortcut:
      const VideoTraits & vtts = frame_->traits_;

      double w = crop.w_;
      double h = crop.h_;

      if (!verticalScalingEnabled_)
      {
        if (vtts.pixelAspectRatio_ != 0.0)
        {
          w = floor(0.5 + w * vtts.pixelAspectRatio_);
        }
      }
      else
      {
        if (vtts.pixelAspectRatio_ > 1.0)
        {
          w = floor(0.5 + w * vtts.pixelAspectRatio_);
        }
        else if (vtts.pixelAspectRatio_ < 1.0)
        {
          h = floor(0.5 + h / vtts.pixelAspectRatio_);
        }
      }

      double dar = h ? (w / h) : 0.0;
      return dar;
    }

    return 0.0;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::nativeAspectRatio
  //
  double
  TBaseCanvas::nativeAspectRatioRotated(int & rotate) const
  {
    double dar = nativeAspectRatio();

    if (dar)
    {
      // video traits shortcut:
      const VideoTraits & vtts = frame_->traits_;

      if (vtts.cameraRotation_ % 90 == 0)
      {
        // must be a camera phone video that needs to be
        // rotated for viewing:
        if (vtts.cameraRotation_ % 180 != 0)
        {
          dar = 1.0 / dar;
        }

        rotate = vtts.cameraRotation_;
      }
      else
      {
        rotate = 0;
      }
    }

    return dar;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::overrideDisplayAspectRatio
  //
  void
  TBaseCanvas::overrideDisplayAspectRatio(double dar)
  {
    dar_ = dar;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::displayAspectRatioFor
  //
  double
  TBaseCanvas::displayAspectRatioFor(int cameraRotation) const
  {
    double dar = dar_;

    if (dar && cameraRotation % 180 != 0)
    {
      dar = 1.0 / dar;
    }

    return dar;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::cropFrame
  //
  void
  TBaseCanvas::cropFrame(double darCropped)
  {
    crop_.clear();
    darCropped_ = darCropped;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::cropFrame
  //
  void
  TBaseCanvas::cropFrame(const TCropFrame & crop)
  {
    darCropped_ = 0.0;
    crop_ = crop;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::getFrame
  //
  void
  TBaseCanvas::getFrame(TVideoFramePtr & frame) const
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    frame = frame_;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::fragmentShaderFor
  //
  const TFragmentShader *
  TBaseCanvas::fragmentShaderFor(TPixelFormatId format) const
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
      std::ostringstream oss;
      oss << "\n" << ptts->name_ << " FRAGMENT SHADER:";

      if (found != shaders_.end())
      {
        oss << '\n';
        yae_show_program_listing(oss, found->second.program_->code_);
        oss << '\n';
      }
      else
      {
        oss << " NOT FOUND\n";
      }

      yae_debug << oss.str();
    }
#endif

    if (found != shaders_.end())
    {
      return &(found->second);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::findSomeShaderFor
  //
  const TFragmentShader *
  TBaseCanvas::findSomeShaderFor(TPixelFormatId format) const
  {
    const TFragmentShader * shader = fragmentShaderFor(format);
    if (!shader && builtinShader_.program_)
    {
      shader = &builtinShader_;
#if 0 // !defined(NDEBUG)
      const pixelFormat::Traits * ptts = pixelFormat::getTraits(format);
      std::ostringstream oss;
      oss << "WILL USE PASS-THROUGH SHADER for " << ptts->name_ << "\n";
      yae_show_program_listing(oss, shader->program_->code_);
      yae_debug << oss.str();
#endif
    }

    return shader;
  }

  //----------------------------------------------------------------
  // ShaderPrograms::~ShaderPrograms
  //
  ShaderPrograms::~ShaderPrograms()
  {
    while (!programs_.empty())
    {
      programs_.front().destroy();
      programs_.pop_front();
    }

    builtin_.destroy();
  }

  //----------------------------------------------------------------
  // ShaderPrograms::createBuiltinShaderProgram
  //
  bool
  ShaderPrograms::createBuiltinShaderProgram(const char * code)
  {
    YAE_OPENGL_HERE();

    bool ok = false;
    builtin_.destroy();
    builtin_.code_ = code;

    YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));

    YAE_OPENGL(glGenProgramsARB(1, &builtin_.handle_));
    YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                builtin_.handle_));

    if (load_arb_program_natively(GL_FRAGMENT_PROGRAM_ARB,
                                  builtin_.code_))
    {
      ok = true;
    }
    else
    {
      YAE_OPENGL(glDeleteProgramsARB(1, &builtin_.handle_));
      builtin_.handle_ = 0;
    }
    YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
    return ok;
  }

  //----------------------------------------------------------------
  // ShaderPrograms::createShaderProgramsFor
  //
  bool
  ShaderPrograms::createShaderProgramsFor(const TPixelFormatId * formats,
                                          const std::size_t numFormats,
                                          const char * code)
  {
    YAE_OPENGL_HERE();

    bool ok = false;
    TFragmentShaderProgram program(code);

    YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));
    YAE_OPENGL(glGenProgramsARB(1, &program.handle_));
    YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program.handle_));

    if (load_arb_program_natively(GL_FRAGMENT_PROGRAM_ARB,
                                  program.code_))
    {
      programs_.push_back(program);
      const TFragmentShaderProgram * p = &(programs_.back());

      for (std::size_t i = 0; i < numFormats; i++)
      {
        TPixelFormatId format = formats[i];
        lut_[format] = p;
      }

      ok = true;
    }
    else
    {
      YAE_OPENGL(glDeleteProgramsARB(1, &program.handle_));
      program.handle_ = 0;
    }

    YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
    return ok;
  }

  //----------------------------------------------------------------
  // get_opengl_version
  //
  static bool
  get_opengl_version(uint16_t & major, uint16_t & minor)
  {
    major = 0;
    minor = 0;

    YAE_OPENGL_HERE();

    const char * version =
      ((const char *)YAE_OPENGL(glGetString(GL_VERSION)));

    if (version && *version)
    {
      const std::size_t version_len = strlen(version);
      const char * found = strchr(version, '.');
      if ((found != NULL) &&
          (found - 1) >= version &&
          (found + 1) < (version + version_len))
      {
        major = (*(found - 1) - '0');
        minor = (*(found + 1) - '0');
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // check_supports_texture_3d
  //
  static bool
  check_supports_texture_3d()
  {
    uint16_t major = 0;
    uint16_t minor = 0;
    YAE_ASSERT(get_opengl_version(major, minor));
    yae_ilog("get_opengl_version: %i.%i", major, minor);

    if (major < 1 || (major == 1 && minor < 2))
    {
      return false;
    }

#ifdef YAE_USE_QGL_WIDGET
    return glTexImage3D != NULL;
#else
    return true;
#endif
  }

  //----------------------------------------------------------------
  // TBaseCanvas::setFrame
  //
  bool
  TBaseCanvas::setFrame(const TVideoFramePtr & frame,
                        bool & colorSpaceOrRangeChanged)
  {
    static const bool supports_tex_3d = check_supports_texture_3d();

    // NOTE: this assumes that the mutex is already locked:
    bool frameSizeOrFormatChanged = false;

    if (!frame_ || !frame ||
        !frame_->traits_.sameFrameSizeAndFormat(frame->traits_))
    {
      crop_.clear();
      frameSizeOrFormatChanged = true;
    }

    colorSpaceOrRangeChanged =
      frame && !(clut_input_.sameColorSpaceAndRange(frame->traits_) &&
                 clut_input_.av_fmt_ == frame->traits_.av_fmt_);

    bool upload_clut_texture =
      supports_tex_3d && colorSpaceOrRangeChanged && !skipColorConverter_;

    if (upload_clut_texture)
    {
      const VideoTraits & vtts = frame->traits_;
      clut_input_ = vtts;

      // update the color transform LUT:
      const Colorspace * dst_colorspace =
        Colorspace::get(AVCOL_SPC_BT709,
                        AVCOL_PRI_BT709,
                        AVCOL_TRC_BT709);

      Colorspace::Format src_format(vtts.av_fmt_, vtts.av_rng_);
      static const Colorspace::Format dst_format(AV_PIX_FMT_RGB24,
                                                 AVCOL_RANGE_JPEG);

      static const Colorspace::DynamicRange dst_dynamic_range(100.0); // SDR
      const double peak_ratio = vtts.dynamic_range_.Lw_ / dst_dynamic_range.Lw_;

      // ToneMapPiecewise tone_map;
      // ToneMapLog tone_map;
      ToneMapGamma tone_map(vtts.dynamic_range_, dst_dynamic_range);

      if (vtts.colorspace_->av_trc_ == AVCOL_TRC_ARIB_STD_B67)
      {
        tone_map.tune_for_hlg(vtts.dynamic_range_, dst_dynamic_range);
      }

      clut_.fill(*vtts.colorspace_,
                 *dst_colorspace,
                 src_format,
                 dst_format,
                 vtts.dynamic_range_,
                 dst_dynamic_range,
                 (peak_ratio < 1.5) ? NULL : &tone_map);

#if 0 // for debugging only:
      {
        AvFrm frm = lut_3d_to_2d_rgb(clut_, *dst_colorspace);
        std::string fn_prefix = "/tmp/clut-canvas-renderer-";
        YAE_ASSERT(save_as_png(frm, fn_prefix, TTime(1, 30)));
      }
#endif
    }

    if (!clut_tex_id_ && !skipColorConverter_ && supports_tex_3d)
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glGenTextures(1, &clut_tex_id_));
      upload_clut_texture = true;
    }

    if (clut_tex_id_ && upload_clut_texture)
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glEnable(GL_TEXTURE_3D));
      YAE_OPENGL(glBindTexture(GL_TEXTURE_3D, clut_tex_id_));

      YAE_OPENGL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,
                                 GL_CLAMP_TO_EDGE));
      YAE_OPENGL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,
                                 GL_CLAMP_TO_EDGE));
      YAE_OPENGL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,
                                 GL_CLAMP_TO_EDGE));
      YAE_OPENGL(glTexParameteri(GL_TEXTURE_3D,
                                 GL_TEXTURE_MAG_FILTER, GL_LINEAR));
      YAE_OPENGL(glTexParameteri(GL_TEXTURE_3D,
                                 GL_TEXTURE_MIN_FILTER, GL_LINEAR));

      YAE_OPENGL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE));
      yae_assert_gl_no_error();

      YAE_OPENGL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
      yae_assert_gl_no_error();

      YAE_OPENGL(glPixelStorei(GL_UNPACK_ROW_LENGTH,
                               (GLint)(clut_.size_1d_)));
      yae_assert_gl_no_error();

      YAE_OPENGL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,
                               (GLint)(clut_.size_1d_)));
      yae_assert_gl_no_error();

      YAE_OPENGL(glTexImage3D(GL_TEXTURE_3D, // target
                              0, // level
                              GL_RGB, // internalFormat
                              clut_.size_1d_, // width
                              clut_.size_1d_, // height
                              clut_.size_1d_, // depth
                              0, // border
                              GL_RGB, // format
                              GL_UNSIGNED_BYTE, // type
                              clut_.get_data())); // pixels
      yae_assert_gl_no_error();
      YAE_OPENGL(glDisable(GL_TEXTURE_3D));
    }

    frame_ = frame;
    return frameSizeOrFormatChanged;
  }

  //----------------------------------------------------------------
  // TBaseCanvas::clearFrame
  //
  void
  TBaseCanvas::clearFrame()
  {
    if (clut_tex_id_)
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glDeleteTextures(1, &clut_tex_id_));
      clut_tex_id_ = 0;
    }

    dar_ = 0.0;
    darCropped_ = 0.0;
    crop_.clear();
    frame_ = TVideoFramePtr();
  }


  //----------------------------------------------------------------
  // fragment_shaders_supported
  //
  static bool
  fragment_shaders_supported()
  {
    if (yae_is_opengl_extension_supported("GL_ARB_fragment_program"))
    {
      GLint numTextureUnits = 0;
      YAE_OPENGL_HERE();
      YAE_OPENGL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB,
                               &numTextureUnits));
      if (numTextureUnits > 2)
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // TModernCanvas::TModernCanvas
  //
  TModernCanvas::TModernCanvas():
    TBaseCanvas(TModernCanvas::shaders())
  {}

  //----------------------------------------------------------------
  // TModernCanvas::shaders
  //
  const ShaderPrograms &
  TModernCanvas::shaders()
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    static ShaderPrograms s;
    static bool initialized = false;

    if (initialized)
    {
      return s;
    }

    if (!fragment_shaders_supported())
    {
      initialized = true;
      return s;
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
      kPixelFormatYUVJ440P,
      kPixelFormatYUV420P9,
      kPixelFormatYUV422P9,
      kPixelFormatYUV444P9,
      kPixelFormatYUV420P10,
      kPixelFormatYUV422P10,
      kPixelFormatYUV444P10,
      kPixelFormatYUV420P12,
      kPixelFormatYUV422P12,
      kPixelFormatYUV444P12,
      kPixelFormatYUV420P14,
      kPixelFormatYUV422P14,
      kPixelFormatYUV444P14,
      kPixelFormatYUV420P16,
      kPixelFormatYUV422P16,
      kPixelFormatYUV444P16
    };

    s.createShaderProgramsFor(yuv, sizeof(yuv) / sizeof(yuv[0]),
                              yae_gl_arb_yuv_to_rgb);

    // for YUVA formats:
    static const TPixelFormatId yuva[] = {
      kPixelFormatYUVA420P
    };

    s.createShaderProgramsFor(yuva, sizeof(yuva) / sizeof(yuva[0]),
                              yae_gl_arb_yuva_to_rgba);

    // for YUYV formats:
    static const TPixelFormatId yuyv[] = {
      kPixelFormatYUYV422
    };

    if (!s.createShaderProgramsFor(yuyv, sizeof(yuyv) / sizeof(yuyv[0]),
                                   yae_gl_arb_yuyv_to_rgb_antialias))
    {
      // perhaps the anti-aliased program was too much for this GPU,
      // try one witnout anti-aliasing:
      s.createShaderProgramsFor(yuyv, sizeof(yuyv) / sizeof(yuyv[0]),
                                yae_gl_arb_yuyv_to_rgb);
    }

    // for UYVY formats:
    static const TPixelFormatId uyvy[] = {
      kPixelFormatUYVY422
    };

    if (!s.createShaderProgramsFor(uyvy, sizeof(uyvy) / sizeof(uyvy[0]),
                                   yae_gl_arb_uyvy_to_rgb_antialias))
    {
      // perhaps the anti-aliased program was too much for this GPU,
      // try one witnout anti-aliasing:
      s.createShaderProgramsFor(uyvy, sizeof(uyvy) / sizeof(uyvy[0]),
                                yae_gl_arb_uyvy_to_rgb);
    }

    // for NV12 formats:
    static const TPixelFormatId nv12[] = {
      kPixelFormatNV12,
      kPixelFormatP010,
      kPixelFormatP016
    };

    s.createShaderProgramsFor(nv12, sizeof(nv12) / sizeof(nv12[0]),
                              yae_gl_arb_nv12_to_rgb);

    // for NV21 formats:
    static const TPixelFormatId nv21[] = {
      kPixelFormatNV21,
      kPixelFormatNV42
    };

    s.createShaderProgramsFor(nv21, sizeof(nv21) / sizeof(nv21[0]),
                              yae_gl_arb_nv21_to_rgb);

    // for natively supported formats, with CLUT:
    static const TPixelFormatId native[] = {
      kPixelFormatRGB24,
      kPixelFormatBGR24,
      kPixelFormatRGB8,
      kPixelFormatBGR8,
      kPixelFormatARGB,
      kPixelFormatRGBA,
      kPixelFormatABGR,
      kPixelFormatBGRA,
      kPixelFormatY400A,
      kPixelFormatGRAY16BE,
      kPixelFormatGRAY16LE,
      kPixelFormatRGB48BE,
      kPixelFormatRGB48LE,
      kPixelFormatRGB565BE,
      kPixelFormatRGB565LE,
      kPixelFormatBGR565BE,
      kPixelFormatBGR565LE,
      kPixelFormatRGB555BE,
      kPixelFormatRGB555LE,
      kPixelFormatBGR555BE,
      kPixelFormatBGR555LE,
      kPixelFormatRGB444BE,
      kPixelFormatRGB444LE,
      kPixelFormatBGR444BE,
      kPixelFormatBGR444LE,
    };

    s.createShaderProgramsFor(native, sizeof(native) / sizeof(native[0]),
                              yae_gl_arb_passthrough_clut);

    // for natively supported formats, without CLUT:
    s.createBuiltinShaderProgram(yae_gl_arb_passthrough);

    initialized = true;
    return s;
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
  int
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
    YAE_BENCHMARK(benchmark, "TModernCanvas::clear");
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(context);

    if (!texId_.empty())
    {
      YAE_OPENGL_HERE();
      YAE_OPENGL(glDeleteTextures((GLsizei)(texId_.size()),
                                  &(texId_.front())));
      texId_.clear();
    }

    TBaseCanvas::clearFrame();
  }

  //----------------------------------------------------------------
  // TModernCanvas::loadFrame
  //
  bool
  TModernCanvas::loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame)
  {
    YAE_BENCHMARK(benchmark, "TModernCanvas::loadFrame");

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
    YAE_OPENGL_HERE();

    // take the new frame:
    bool colorSpaceOrRangeChanged = false;
    bool frameSizeOrFormatChanged = setFrame(frame, colorSpaceOrRangeChanged);

    // setup new texture objects:
    if (frameSizeOrFormatChanged)
    {
      if (!texId_.empty())
      {
        YAE_OPENGL(glDeleteTextures((GLsizei)(texId_.size()),
                                    &(texId_.front())));
        texId_.clear();
      }

      shader_ = findSomeShaderFor(vtts.pixelFormat_);

      if (!supportedChannels && !shader_)
      {
        return false;
      }

      const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;

      texId_.resize(shader.numPlanes_);
      YAE_OPENGL(glGenTextures((GLsizei)(texId_.size()), &(texId_.front())));

      YAE_OPENGL(glEnable(GL_TEXTURE_RECTANGLE_ARB));
      for (std::size_t i = 0; i < shader.numPlanes_; i++)
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]));

#ifdef __APPLE__
        YAE_OPENGL(glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                                   GL_TEXTURE_STORAGE_HINT_APPLE,
                                   GL_STORAGE_CACHED_APPLE));
#endif

        YAE_OPENGL(glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                                   GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        YAE_OPENGL(glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                                   GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        YAE_OPENGL(glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                                   GL_TEXTURE_MAG_FILTER,
                                   shader.magFilterGL_[i]));
        YAE_OPENGL(glTexParameteri(GL_TEXTURE_RECTANGLE_ARB,
                                   GL_TEXTURE_MIN_FILTER,
                                   shader.minFilterGL_[i]));
        yae_assert_gl_no_error();

        TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
        {
          YAE_OPENGL(glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
                                  0, // always 0 for GL_TEXTURE_RECTANGLE_ARB
                                  shader.internalFormatGL_[i],
                                  vtts.encodedWidth_ / shader.subsample_x_[i],
                                  vtts.encodedHeight_ / shader.subsample_y_[i],
                                  0, // border width
                                  shader.pixelFormatGL_[i],
                                  shader.dataTypeGL_[i],
                                  NULL));
          yae_assert_gl_no_error();
        }
      }
      YAE_OPENGL(glDisable(GL_TEXTURE_RECTANGLE_ARB));
    }

    if (!supportedChannels && !shader_)
    {
      return false;
    }

    // upload texture data:
    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
    {
      YAE_OPENGL(glEnable(GL_TEXTURE_RECTANGLE_ARB));

      for (std::size_t i = 0; i < shader.numPlanes_; i++)
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]));

        YAE_OPENGL(glPixelStorei(GL_UNPACK_SWAP_BYTES,
                                 shader.shouldSwapBytes_[i]));

        const unsigned char * data = frame->data_->data(i);
        std::size_t rowSize =
          frame->data_->rowBytes(i) / (shader.stride_[i] / 8);
        YAE_OPENGL(glPixelStorei(GL_UNPACK_ALIGNMENT,
                                 alignmentFor(data, rowSize)));
        YAE_OPENGL(glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(rowSize)));
        yae_assert_gl_no_error();

        YAE_OPENGL(glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,
                                0, // always 0 for GL_TEXTURE_RECTANGLE_ARB
                                shader.internalFormatGL_[i],
                                vtts.encodedWidth_ / shader.subsample_x_[i],
                                vtts.encodedHeight_ / shader.subsample_y_[i],
                                0, // border width
                                shader.pixelFormatGL_[i],
                                shader.dataTypeGL_[i],
                                data));
        yae_assert_gl_no_error();
      }
      YAE_OPENGL(glDisable(GL_TEXTURE_RECTANGLE_ARB));
    }

    if (shader_)
    {
      YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));
      YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                  shader_->program_->handle_));
      {
        // rescale 3D texture coordinates so they would map
        // to the [0, 1] x [0, 1] x [0, 1] region of the CLUT:
        GLdouble lut_rescale_yuv[4] = { 1.0 };
        lut_rescale_yuv[0] = clut_.zs_;
        lut_rescale_yuv[1] = clut_.zs_;
        lut_rescale_yuv[2] = clut_.zs_;
        YAE_OPENGL(glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                                 0, lut_rescale_yuv));
        yae_assert_gl_no_error();

        // scale yuv 9,10,12,14-bit samples to 16-bit, etc...
        YAE_OPENGL(glProgramLocalParameter4dARB
                   (GL_FRAGMENT_PROGRAM_ARB,
                    1,
                    double(1u << ptts->datatype_lpad_[0]),
                    double(1u << ptts->datatype_lpad_[1]),
                    double(1u << ptts->datatype_lpad_[2]),
                    double(1u << ptts->datatype_lpad_[3])));
        yae_assert_gl_no_error();

        // pass the chroma subsampling factors to the shader:
        GLdouble subsample_uv[4] = { 1.0 };
        subsample_uv[0] = 1.0 / double(ptts->chromaBoxW_);
        subsample_uv[1] = 1.0 / double(ptts->chromaBoxH_);

        YAE_OPENGL(glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                                 2, subsample_uv));
        yae_assert_gl_no_error();
      }
      YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
    }

    return true;
  }

  //----------------------------------------------------------------
  // TModernCanvas::draw
  //
  void
  TModernCanvas::draw(double opacity) const
  {
    YAE_BENCHMARK(benchmark, "TModernCanvas::draw");
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

    YAE_OPENGL_HERE();
    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OPENGL(glEnable(GL_TEXTURE_RECTANGLE_ARB));
    YAE_OPENGL(glDisable(GL_LIGHTING));
    YAE_OPENGL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    YAE_OPENGL(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    YAE_OPENGL(glColor4d(1.0, 1.0, 1.0, opacity));

    if (shader_)
    {
      YAE_OPENGL(glEnable(GL_TEXTURE_3D));
      YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));
    }

    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    const std::size_t numTextures = texId_.size();

    for (std::size_t i = 0; i < numTextures; i += shader.numPlanes_)
    {
      if (shader_)
      {
        YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                    shader_->program_->handle_));
      }

      if (YAE_OGL_FN(glActiveTexture))
      {
        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + k)));
          yae_assert_gl_no_error();

          YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[k + i]));
        }

        YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + shader.numPlanes_)));
      }
      else
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texId_[i]));
      }

      if (clut_tex_id_)
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_3D, clut_tex_id_));
      }

      {
        yaegl::BeginEnd mode(GL_TRIANGLE_FAN);
        const VideoTraits & vtts = frame_->traits_;
        const int hflip = vtts.hflip_ ? 1 : 0;
        const int vflip = vtts.vflip_ ? 1 : 0;

        YAE_OPENGL(glTexCoord2i(crop.x_ + crop.w_ * hflip,
                                crop.y_ + crop.h_ * vflip));
        YAE_OPENGL(glVertex2i(0, 0));

        YAE_OPENGL(glTexCoord2i(crop.x_ + crop.w_ * (1 - hflip),
                                crop.y_ + crop.h_ * vflip));
        YAE_OPENGL(glVertex2i(int(w), 0));

        YAE_OPENGL(glTexCoord2i(crop.x_ + crop.w_ * (1 - hflip),
                                crop.y_ + crop.h_ * (1 - vflip)));
        YAE_OPENGL(glVertex2i(int(w), int(h)));

        YAE_OPENGL(glTexCoord2i(crop.x_ + crop.w_ * hflip,
                                crop.y_ + crop.h_ * (1 - vflip)));
        YAE_OPENGL(glVertex2i(0, int(h)));
      }

      yae_assert_gl_no_error();
    }

    // un-bind the textures:
    if (YAE_OGL_FN(glActiveTexture))
    {
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + k)));
        yae_assert_gl_no_error();

        YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0));
      }

      YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + shader.numPlanes_)));
    }
    else
    {
      YAE_OPENGL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0));
    }

    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    if (shader_)
    {
      YAE_OPENGL(glBindTexture(GL_TEXTURE_3D, 0));
      YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0));
      YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
      YAE_OPENGL(glDisable(GL_TEXTURE_3D));
    }

    YAE_OPENGL(glDisable(GL_TEXTURE_RECTANGLE_ARB));
  }


  //----------------------------------------------------------------
  // TLegacyCanvas::TLegacyCanvas
  //
  TLegacyCanvas::TLegacyCanvas():
    TBaseCanvas(TLegacyCanvas::shaders()),
    w_(0),
    h_(0)
  {}

  //----------------------------------------------------------------
  // TLegacyCanvas::shaders
  //
  const ShaderPrograms &
  TLegacyCanvas::shaders()
  {
    static boost::mutex mutex;
    boost::lock_guard<boost::mutex> lock(mutex);

    static ShaderPrograms s;
    static bool initialized = false;

    if (initialized)
    {
      return s;
    }

    if (!fragment_shaders_supported())
    {
      initialized = true;
      return s;
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
      kPixelFormatYUVJ440P,
      kPixelFormatYUV420P9,
      kPixelFormatYUV422P9,
      kPixelFormatYUV444P9,
      kPixelFormatYUV420P10,
      kPixelFormatYUV422P10,
      kPixelFormatYUV444P10,
      kPixelFormatYUV420P12,
      kPixelFormatYUV422P12,
      kPixelFormatYUV444P12,
      kPixelFormatYUV420P14,
      kPixelFormatYUV422P14,
      kPixelFormatYUV444P14,
      kPixelFormatYUV420P16,
      kPixelFormatYUV422P16,
      kPixelFormatYUV444P16
    };

    s.createShaderProgramsFor(yuv, sizeof(yuv) / sizeof(yuv[0]),
                              yae_gl_arb_yuv_to_rgb_2d);

    // for YUVA formats:
    static const TPixelFormatId yuva[] = {
      kPixelFormatYUVA420P
    };

    s.createShaderProgramsFor(yuva, sizeof(yuva) / sizeof(yuva[0]),
                              yae_gl_arb_yuva_to_rgba_2d);

    // for NV12 formats:
    static const TPixelFormatId nv12[] = {
      kPixelFormatNV12,
      kPixelFormatP010,
      kPixelFormatP016
    };

    s.createShaderProgramsFor(nv12, sizeof(nv12) / sizeof(nv12[0]),
                              yae_gl_arb_nv12_to_rgb_2d);

    // for NV21 formats:
    static const TPixelFormatId nv21[] = {
      kPixelFormatNV21,
      kPixelFormatNV24
    };

    s.createShaderProgramsFor(nv21, sizeof(nv21) / sizeof(nv21[0]),
                              yae_gl_arb_nv21_to_rgb_2d);

    // for natively supported formats, with CLUT:
    static const TPixelFormatId native[] = {
      kPixelFormatRGB24,
      kPixelFormatBGR24,
      kPixelFormatRGB8,
      kPixelFormatBGR8,
      kPixelFormatARGB,
      kPixelFormatRGBA,
      kPixelFormatABGR,
      kPixelFormatBGRA,
      kPixelFormatY400A,
      kPixelFormatGRAY16BE,
      kPixelFormatGRAY16LE,
      kPixelFormatRGB48BE,
      kPixelFormatRGB48LE,
      kPixelFormatRGB565BE,
      kPixelFormatRGB565LE,
      kPixelFormatBGR565BE,
      kPixelFormatBGR565LE,
      kPixelFormatRGB555BE,
      kPixelFormatRGB555LE,
      kPixelFormatBGR555BE,
      kPixelFormatBGR555LE,
      kPixelFormatRGB444BE,
      kPixelFormatRGB444LE,
      kPixelFormatBGR444BE,
      kPixelFormatBGR444LE,
    };

    s.createShaderProgramsFor(native, sizeof(native) / sizeof(native[0]),
                              yae_gl_arb_passthrough_clut_2d);

    // for natively supported formats, without CLUT:
    s.createBuiltinShaderProgram(yae_gl_arb_passthrough_2d);

    initialized = true;
    return s;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::clear
  //
  void
  TLegacyCanvas::clear(IOpenGLContext & context)
  {
    YAE_BENCHMARK(benchmark, "TLegacyCanvas::clear");
    boost::lock_guard<boost::mutex> lock(mutex_);
    TMakeCurrentContext currentContext(context);
    YAE_OPENGL_HERE();

    TBaseCanvas::clearFrame();

    if (!texId_.empty())
    {
      YAE_OPENGL(glDeleteTextures((GLsizei)(texId_.size()),
                                  &(texId_.front())));
      texId_.clear();
    }

    w_ = 0;
    h_ = 0;
    tiles_.clear();

    TBaseCanvas::clearFrame();
  }

  //----------------------------------------------------------------
  // calculateEdges
  //
  void
  calculateEdges(std::deque<TEdge> & edges,
                 GLsizei edgeSize,
                 GLsizei textureEdgeMax,
                 bool flip)
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

    if (!flip)
    {
      return;
    }

    for (std::deque<TEdge>::iterator i = edges.begin(); i != edges.end(); ++i)
    {
      TEdge & edge = *i;
      edge.v0_ = edgeSize - edge.v0_;
      edge.v1_ = edgeSize - edge.v1_;
    }
  }

  //----------------------------------------------------------------
  // get_max_texture_2d
  //
  GLsizei
  get_max_texture_2d()
  {
    static GLsizei edgeMax = 64;

    if (edgeMax > 64)
    {
      return edgeMax;
    }

    YAE_OPENGL_HERE();
    for (unsigned int i = 0; i < 8; i++, edgeMax *= 2)
    {
      YAE_OPENGL(glTexImage2D(GL_PROXY_TEXTURE_2D,
                              0, // level
                              GL_RGBA,
                              edgeMax * 2, // width
                              edgeMax * 2, // height
                              0,
                              GL_RGBA,
                              GL_UNSIGNED_BYTE,
                              NULL));
      GLenum err = YAE_OPENGL(glGetError());
      if (err != GL_NO_ERROR)
      {
        break;
      }

      GLint width = 0;
      YAE_OPENGL(glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D,
                                          0, // level
                                          GL_TEXTURE_WIDTH,
                                          &width));
      if (width != GLint(edgeMax * 2))
      {
        break;
      }
    }

    yae_ilog("GL_RGBA 2D texture size max: %i x %i", edgeMax, edgeMax);
    return edgeMax;
  }

  //----------------------------------------------------------------
  // get_supports_texture_rectangle
  //
  bool
  get_supports_texture_rectangle()
  {
    static bool texture_rectangle =
      yae_is_opengl_extension_supported("GL_ARB_texture_rectangle");
    return texture_rectangle;
  }

  //----------------------------------------------------------------
  // get_supports_luminance16
  //
  bool
  get_supports_luminance16()
  {
    // don't really know how to check for GL_LUMINANCE16 support reliably
    // ... glTexImage2D followed by glGetTexImage didn't work
    return (get_supports_texture_rectangle() &&
            get_max_texture_2d() > 2048);
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::loadFrame
  //
  bool
  TLegacyCanvas::loadFrame(IOpenGLContext & context,
                           const TVideoFramePtr & frame)
  {
    YAE_BENCHMARK(benchmark, "TLegacyCanvas::loadFrame");

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
    YAE_OPENGL_HERE();

    // avoid creating excessively oversized tiles:
    static const GLsizei textureEdgeMax =
      std::min<GLsizei>(4096, std::max<GLsizei>(64, get_max_texture_2d() / 2));

    // take the new frame:
    bool colorSpaceOrRangeChanged = false;
    bool frameSizeOrFormatChanged = setFrame(frame, colorSpaceOrRangeChanged);

    TCropFrame crop;
    getCroppedFrame(crop);

    if (frameSizeOrFormatChanged || w_ != crop.w_ || h_ != crop.h_)
    {
      if (!texId_.empty())
      {
        YAE_OPENGL(glDeleteTextures((GLsizei)(texId_.size()),
                                    &(texId_.front())));
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
      calculateEdges(x, w_, textureEdgeMax, vtts.hflip_);

      // calculate y-min, y-max coordinates for each tile:
      std::deque<TEdge> y;
      calculateEdges(y, h_, textureEdgeMax, vtts.vflip_);

      // setup the tiles:
      const std::size_t rows = y.size();
      const std::size_t cols = x.size();
      tiles_.resize(rows * cols);

      texId_.resize(rows * cols * shader.numPlanes_);
      YAE_OPENGL(glGenTextures((GLsizei)(texId_.size()), &(texId_.front())));

      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        TFrameTile & tile = tiles_[i];
        tile.x_ = x[i % cols];
        tile.y_ = y[i / cols];

        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          GLuint texId = texId_[k + i * shader.numPlanes_];
          YAE_OPENGL(glBindTexture(GL_TEXTURE_2D, texId));

          if (!YAE_OPENGL(glIsTexture(texId)))
          {
            YAE_ASSERT(false);
            return false;
          }

          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_BASE_LEVEL, 0));
          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_MAX_LEVEL, 0));

          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_MAG_FILTER,
                                     shader.magFilterGL_[k]));
          YAE_OPENGL(glTexParameteri(GL_TEXTURE_2D,
                                     GL_TEXTURE_MIN_FILTER,
                                     shader.minFilterGL_[k]));
          yae_assert_gl_no_error();

          YAE_OPENGL(glTexImage2D(GL_TEXTURE_2D,
                                  0, // mipmap level
                                  shader.internalFormatGL_[k],
                                  tile.x_.extent_ / shader.subsample_x_[k],
                                  tile.y_.extent_ / shader.subsample_y_[k],
                                  0, // border width
                                  shader.pixelFormatGL_[k],
                                  shader.dataTypeGL_[k],
                                  NULL));

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

      YAE_OPENGL(glPixelStorei(GL_UNPACK_SWAP_BYTES,
                               shader.shouldSwapBytes_[k]));

      const unsigned char * data = frame->data_->data(k);
      std::size_t rowSize =
        frame->data_->rowBytes(k) / (ptts->stride_[k] / 8);
      YAE_OPENGL(glPixelStorei(GL_UNPACK_ALIGNMENT,
                               alignmentFor(data, rowSize)));
      YAE_OPENGL(glPixelStorei(GL_UNPACK_ROW_LENGTH,
                               (GLint)(rowSize)));
      yae_assert_gl_no_error();

      for (std::size_t i = 0; i < tiles_.size(); ++i)
      {
        const TFrameTile & tile = tiles_[i];

        GLuint texId = texId_[k + i * shader.numPlanes_];
        YAE_OPENGL(glBindTexture(GL_TEXTURE_2D, texId));

        if (!YAE_OPENGL(glIsTexture(texId)))
        {
          YAE_ASSERT(false);
          continue;
        }

        YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                                 tile.x_.offset_ / subsample_x));
        yae_assert_gl_no_error();

        YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_ROWS,
                                 tile.y_.offset_ / subsample_y));
        yae_assert_gl_no_error();

        YAE_OPENGL(glTexSubImage2D(GL_TEXTURE_2D,
                                   0, // mipmap level
                                   0, // x-offset
                                   0, // y-offset
                                   tile.x_.length_ / subsample_x,
                                   tile.y_.length_ / subsample_y,
                                   shader.pixelFormatGL_[k],
                                   shader.dataTypeGL_[k],
                                   src[k]));
        yae_assert_gl_no_error();

        if (tile.x_.length_ < tile.x_.extent_)
        {
          // extend on the right to avoid texture filtering artifacts:
          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                                   (tile.x_.offset_ + tile.x_.length_) /
                                   subsample_x - 1));
          yae_assert_gl_no_error();

          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_ROWS,
                                   tile.y_.offset_ / subsample_y));
          yae_assert_gl_no_error();

          YAE_OPENGL(glTexSubImage2D(GL_TEXTURE_2D,
                                     0, // mipmap level

                                     // x,y offset
                                     tile.x_.length_ / subsample_x,
                                     0,

                                     // width, height
                                     1,
                                     tile.y_.length_ / subsample_y,

                                     shader.pixelFormatGL_[k],
                                     shader.dataTypeGL_[k],
                                     src[k]));
          yae_assert_gl_no_error();
        }

        if (tile.y_.length_ < tile.y_.extent_)
        {
          // extend on the bottom to avoid texture filtering artifacts:
          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                                   tile.x_.offset_ / subsample_x));

          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_ROWS,
                                   (tile.y_.offset_ + tile.y_.length_) /
                                   subsample_y - 1));

          YAE_OPENGL(glTexSubImage2D(GL_TEXTURE_2D,
                                     0, // mipmap level

                                     // x,y offset
                                     0,
                                     tile.y_.length_ / subsample_y,

                                     // width, height
                                     tile.x_.length_ / subsample_x,
                                     1,

                                     shader.pixelFormatGL_[k],
                                     shader.dataTypeGL_[k],
                                     src[k]));
          yae_assert_gl_no_error();
        }

        if (tile.x_.length_ < tile.x_.extent_ &&
            tile.y_.length_ < tile.y_.extent_)
        {
          // extend the bottom-right corner:
          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                                   (tile.x_.offset_ + tile.x_.length_) /
                                   subsample_x - 1));
          YAE_OPENGL(glPixelStorei(GL_UNPACK_SKIP_ROWS,
                                   (tile.y_.offset_ + tile.y_.length_) /
                                   subsample_y - 1));

          YAE_OPENGL(glTexSubImage2D(GL_TEXTURE_2D,
                                     0, // mipmap level

                                     // x,y offset
                                     tile.x_.length_ / subsample_x,
                                     tile.y_.length_ / subsample_y,

                                     // width, height
                                     1,
                                     1,

                                     shader.pixelFormatGL_[k],
                                     shader.dataTypeGL_[k],
                                     src[k]));
          yae_assert_gl_no_error();
        }
      }
    }

    if (shader_)
    {
      YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));
      YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                  shader_->program_->handle_));
      {
        // rescale 3D texture coordinates so they would map
        // to the [0, 1] x [0, 1] x [0, 1] region of the CLUT:
        GLdouble lut_rescale_yuv[4] = { 1.0 };
        lut_rescale_yuv[0] = clut_.zs_;
        lut_rescale_yuv[1] = clut_.zs_;
        lut_rescale_yuv[2] = clut_.zs_;
        YAE_OPENGL(glProgramLocalParameter4dvARB(GL_FRAGMENT_PROGRAM_ARB,
                                                 0, lut_rescale_yuv));
        yae_assert_gl_no_error();

        // scale yuv 9,10,12,14-bit samples to 16-bit, etc...
        YAE_OPENGL(glProgramLocalParameter4dARB
                   (GL_FRAGMENT_PROGRAM_ARB,
                    1,
                    double(1u << ptts->datatype_lpad_[0]),
                    double(1u << ptts->datatype_lpad_[1]),
                    double(1u << ptts->datatype_lpad_[2]),
                    double(1u << ptts->datatype_lpad_[3])));
        yae_assert_gl_no_error();
      }
      YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
    }

    return true;
  }

  //----------------------------------------------------------------
  // TLegacyCanvas::draw
  //
  void
  TLegacyCanvas::draw(double opacity) const
  {
    YAE_BENCHMARK(benchmark, "TLegacyCanvas::draw");
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

    YAE_OPENGL_HERE();

    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OPENGL(glEnable(GL_TEXTURE_2D));
    YAE_OPENGL(glDisable(GL_LIGHTING));
    YAE_OPENGL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    YAE_OPENGL(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
    YAE_OPENGL(glColor4d(1.0, 1.0, 1.0, opacity));

    if (shader_)
    {
      YAE_OPENGL(glEnable(GL_TEXTURE_3D));
      YAE_OPENGL(glEnable(GL_FRAGMENT_PROGRAM_ARB));
    }

    double sx = iw / double(crop.w_);
    double sy = ih / double(crop.h_);
    YAE_OPENGL(glScaled(sx, sy, 1.0));

    const TFragmentShader & shader = shader_ ? *shader_ : builtinShader_;
    const std::size_t numTiles = tiles_.size();

    for (std::size_t i = 0; i < numTiles; i++)
    {
      const TFrameTile & tile = tiles_[i];

      if (shader_)
      {
        YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
                                    shader_->program_->handle_));
      }

      if (YAE_OGL_FN(glActiveTexture))
      {
        for (std::size_t k = 0; k < shader.numPlanes_; k++)
        {
          YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + k)));
          yae_assert_gl_no_error();

          YAE_OPENGL(glBindTexture(GL_TEXTURE_2D,
                                   texId_[k + i * shader.numPlanes_]));
        }

        YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + shader.numPlanes_)));
      }
      else
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_2D,
                                 texId_[i * shader.numPlanes_]));
      }

      if (clut_tex_id_)
      {
        YAE_OPENGL(glBindTexture(GL_TEXTURE_3D, clut_tex_id_));
      }

      {
        yaegl::BeginEnd mode(GL_TRIANGLE_FAN);

        YAE_OPENGL(glTexCoord2d(tile.x_.t0_, tile.y_.t0_));
        YAE_OPENGL(glVertex2i(tile.x_.v0_, tile.y_.v0_));

        YAE_OPENGL(glTexCoord2d(tile.x_.t1_, tile.y_.t0_));
        YAE_OPENGL(glVertex2i(tile.x_.v1_, tile.y_.v0_));

        YAE_OPENGL(glTexCoord2d(tile.x_.t1_, tile.y_.t1_));
        YAE_OPENGL(glVertex2i(tile.x_.v1_, tile.y_.v1_));

        YAE_OPENGL(glTexCoord2d(tile.x_.t0_, tile.y_.t1_));
        YAE_OPENGL(glVertex2i(tile.x_.v0_, tile.y_.v1_));
      }

      yae_assert_gl_no_error();
    }

    // un-bind the textures:
    if (YAE_OGL_FN(glActiveTexture))
    {
      for (std::size_t k = 0; k < shader.numPlanes_; k++)
      {
        YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + k)));
        yae_assert_gl_no_error();

        YAE_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
      }

      YAE_OPENGL(glActiveTexture((GLenum)(GL_TEXTURE0 + shader.numPlanes_)));
    }
    else
    {
      YAE_OPENGL(glBindTexture(GL_TEXTURE_2D, 0));
    }

    if (YAE_OGL_FN(glActiveTexture))
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    if (shader_)
    {
      YAE_OPENGL(glBindTexture(GL_TEXTURE_3D, 0));
      YAE_OPENGL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, 0));
      YAE_OPENGL(glDisable(GL_FRAGMENT_PROGRAM_ARB));
      YAE_OPENGL(glDisable(GL_TEXTURE_3D));
    }

    YAE_OPENGL(glDisable(GL_TEXTURE_2D));
  }


  //----------------------------------------------------------------
  // CanvasRenderer::CanvasRenderer
  //
  CanvasRenderer::CanvasRenderer():
    legacy_(new TLegacyCanvas()),
    modern_(NULL)
  {
    YAE_OPENGL_HERE();

    const char * vendor =
      ((const char *)YAE_OPENGL(glGetString(GL_VENDOR)));
    std::string openglVendorInfo = vendor ? vendor : "";

    const char * renderer =
      ((const char *)YAE_OPENGL(glGetString(GL_RENDERER)));
    std::string openglRendererInfo = renderer ? renderer : "";

    const char * version =
      ((const char *)YAE_OPENGL(glGetString(GL_VERSION)));
    std::string openglVersionInfo = version ? version : "";

    // rectangular textures do not work correctly on VirtualBox VMs,
    // so try to detect this and fall back to power-of-2 textures:
    bool virtualBoxVM =
      (openglVendorInfo == "Humper" ||
       openglRendererInfo == "Chromium" ||
       openglVersionInfo == "2.1 Chromium 1.9");

    if (yae_is_opengl_extension_supported("GL_ARB_texture_rectangle") &&
        !virtualBoxVM)
    {
      modern_ = new TModernCanvas();
    }

    renderer_ = legacy_;

    // cache the GL_LUMINANCE16 status:
    get_supports_luminance16();
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
    YAE_BENCHMARK(benchmark, "CanvasRenderer::clear");
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
      // maximum texture size supported by the GL_EXT_texture_rectangle;
      // frames with width/height in excess of this value will be processed
      // using the legacy canvas renderer, which cuts frames into tiles
      // of supported size and renders them seamlessly:
      unsigned int maxTexSize = get_max_texture_2d();

      if (renderer_ == modern_)
      {
        if (maxTexSize < vtts.encodedWidth_ ||
            maxTexSize < vtts.encodedHeight_)
        {
          // use tiled legacy OpenGL renderer:
          return legacy_;
        }
      }
      else if (maxTexSize >= vtts.encodedWidth_ &&
               maxTexSize >= vtts.encodedHeight_)
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
    YAE_BENCHMARK(benchmark, "CanvasRenderer::loadFrame");

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
  CanvasRenderer::draw(double opacity) const
  {
    YAE_BENCHMARK(benchmark, "CanvasRenderer::draw");
    renderer_->draw(opacity);
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
  // CanvasRenderer::nativeAspectRatio
  //
  double
  CanvasRenderer::nativeAspectRatioUncropped() const
  {
    return renderer_->nativeAspectRatioUncropped();
  }

  //----------------------------------------------------------------
  // CanvasRenderer::nativeAspectRatioUncroppedRotated
  //
  double
  CanvasRenderer::nativeAspectRatioUncroppedRotated(int & rotate) const
  {
    return renderer_->nativeAspectRatioUncroppedRotated(rotate);
  }

  //----------------------------------------------------------------
  // CanvasRenderer::nativeAspectRatio
  //
  double
  CanvasRenderer::nativeAspectRatio() const
  {
    return renderer_->nativeAspectRatio();
  }

  //----------------------------------------------------------------
  // CanvasRenderer::nativeAspectRatioRotated
  //
  double
  CanvasRenderer::nativeAspectRatioRotated(int & rotate) const
  {
    return renderer_->nativeAspectRatioRotated(rotate);
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

  //----------------------------------------------------------------
  // TBaseRenderer::paintImage
  //
  void
  TBaseCanvas::paintImage(double x,
                          double y,
                          double w_max,
                          double h_max,
                          double opacity) const
  {
    YAE_BENCHMARK(benchmark, "TBaseCanvas::paintImage");
    double croppedWidth = 0.0;
    double croppedHeight = 0.0;
    int cameraRotation = 0;
    this->imageWidthHeightRotated(croppedWidth,
                                  croppedHeight,
                                  cameraRotation);
    if (!croppedWidth || !croppedHeight)
    {
      return;
    }

    double w = w_max;
    double h = h_max;
    double car = w_max / h_max;
    double dar = croppedWidth / croppedHeight;

    if (dar < car)
    {
      w = h_max * dar;
      x += 0.5 * (w_max - w);
    }
    else
    {
      h = w_max / dar;
      y += 0.5 * (h_max - h);
    }

    TGLSaveMatrixState pushViewMatrix(GL_MODELVIEW);

    YAE_OPENGL_HERE();
    YAE_OPENGL(glTranslated(x, y, 0.0));
    YAE_OPENGL(glScaled(w / croppedWidth, h / croppedHeight, 1.0));

    if (cameraRotation && cameraRotation % 90 == 0)
    {
      YAE_OPENGL(glTranslated(0.5 * croppedWidth, 0.5 * croppedHeight, 0));
      YAE_OPENGL(glRotated(double(cameraRotation), 0, 0, 1));

      if (cameraRotation % 180 != 0)
      {
        YAE_OPENGL(glTranslated(-0.5 * croppedHeight,
                                -0.5 * croppedWidth, 0));
      }
      else
      {
        YAE_OPENGL(glTranslated(-0.5 * croppedWidth,
                                -0.5 * croppedHeight, 0));
      }
    }

    this->draw(opacity);
    yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // CanvasRenderer::adjustPixelFormatForOpenGL
  //
  bool
  CanvasRenderer::adjustPixelFormatForOpenGL(bool skipColorConverter,
                                             const VideoTraits & vtts,
                                             TPixelFormatId & output) const
  {
    TBaseCanvas * renderer = rendererFor(vtts);
    return adjust_pixel_format_for_opengl(renderer,
                                          skipColorConverter,
                                          vtts.pixelFormat_,
                                          output);
  }

  //----------------------------------------------------------------
  // adjust_pixel_format_for_opengl
  //
  bool
  adjust_pixel_format_for_opengl(const TBaseCanvas * renderer,
                                 bool skipColorConverter,
                                 TPixelFormatId nativeFormat,
                                 TPixelFormatId & adjustedFormat)
  {
    // pixel format traits shortcut:
    const pixelFormat::Traits * ptts = pixelFormat::getTraits(nativeFormat);

    TPixelFormatId outputFormat = nativeFormat;
    bool unsupported = (ptts == NULL);

    if (!unsupported)
    {
      unsupported = (ptts->flags_ & pixelFormat::kPaletted) != 0;
    }
    else if (nativeFormat == kInvalidPixelFormat)
    {
      return false;
    }

    if (!unsupported)
    {
      GLint internalFormatGL;
      GLenum pixelFormatGL;
      GLenum dataTypeGL;
      GLint shouldSwapBytes;
      unsigned int supportedChannels = yae_to_opengl(nativeFormat,
                                                     internalFormatGL,
                                                     pixelFormatGL,
                                                     dataTypeGL,
                                                     shouldSwapBytes);

      const TFragmentShader * fragmentShader =
        skipColorConverter ? NULL :
        supportedChannels == ptts->channels_ ? NULL :
        renderer->fragmentShaderFor(nativeFormat);

      if (!supportedChannels && !fragmentShader)
      {
        unsupported = true;
      }
      else if (supportedChannels != ptts->channels_ &&
               !skipColorConverter &&
               !fragmentShader)
      {
        unsupported = true;
      }
    }

    if (unsupported)
    {
      outputFormat = kPixelFormatGRAY8;

      if (ptts)
      {
        if ((ptts->flags_ & pixelFormat::kAlpha) &&
            (ptts->flags_ & pixelFormat::kColor))
        {
          outputFormat = kPixelFormatBGRA;
        }
        else if ((ptts->flags_ & pixelFormat::kColor) ||
                 (ptts->flags_ & pixelFormat::kPaletted))
        {
          if (yae_is_opengl_extension_supported("GL_APPLE_ycbcr_422"))
          {
            outputFormat = kPixelFormatYUYV422;
          }
          else
          {
            outputFormat = kPixelFormatBGR24;
          }
        }
      }
    }

    if (!get_supports_luminance16())
    {
      // if 16-bit textures are not supported then
      // don't accept higher than 8-bit pixel formats:
      if (ptts->depth_[0] > 8)
      {
        unsupported = true;

        bool has_alpha = ptts->has_alpha();
        bool is_rgb = ptts->is_rgb();
        bool is_yuv = ptts->is_yuv();
        bool is_420 = ptts->is_420();
        bool is_422 = ptts->is_422();
        bool is_444 = ptts->is_444();

        yae_debug
          << "native format: " << ptts->name_
          << ", alpha: " << has_alpha
          << ", rgb: " << is_rgb
          << ", yuv: " << is_yuv
          << ", 420: " << is_420
          << ", 422: " << is_422
          << ", 444: " << is_444;

        if (is_rgb)
        {
          outputFormat = has_alpha ? kPixelFormatBGRA : kPixelFormatBGR24;
        }
        else if (is_yuv && has_alpha)
        {
          outputFormat =
            is_444 ? kPixelFormatYUVA444P :
            is_422 ? kPixelFormatYUVA422P :
            kPixelFormatYUVA420P;
        }
        else if (is_yuv)
        {
          outputFormat =
            is_444 ? kPixelFormatYUV444P :
            is_422 ? kPixelFormatYUV422P :
            kPixelFormatYUV420P;
        }
        else if (has_alpha)
        {
          outputFormat = kPixelFormatY400A;
        }
        else
        {
          outputFormat = kPixelFormatGRAY8;
        }
      }
    }

    adjustedFormat = outputFormat;
    return unsupported;
  }

}

//----------------------------------------------------------------
// yae_assert_gl_no_error
//
bool
yae_assert_gl_no_error()
{
  return yaegl::assert_no_error();
}
