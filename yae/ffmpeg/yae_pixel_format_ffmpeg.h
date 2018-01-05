// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 20 16:44:22 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PIXEL_FORMAT_FFMPEG_H_
#define YAE_PIXEL_FORMAT_FFMPEG_H_

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// yae includes:
#include "../video/yae_pixel_formats.h"
#include "../video/yae_video.h"

// ffmpeg includes:
extern "C"
{
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
}


namespace yae
{
  //----------------------------------------------------------------
  // ffmpeg_to_yae
  //
  YAE_API TPixelFormatId
  ffmpeg_to_yae(enum AVPixelFormat ffmpegPixelFormat);

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  YAE_API enum AVPixelFormat
  yae_to_ffmpeg(TPixelFormatId yaePixelFormat);

  //----------------------------------------------------------------
  // to_yae_color_space
  //
  YAE_API TColorSpaceId
  to_yae_color_space(AVColorSpace c);

  //----------------------------------------------------------------
  // to_ffmpeg_color_space
  //
  YAE_API AVColorSpace
  to_ffmpeg_color_space(TColorSpaceId c);

  //----------------------------------------------------------------
  // to_yae_color_range
  //
  YAE_API TColorRangeId
  to_yae_color_range(AVColorRange r);

  //----------------------------------------------------------------
  // to_ffmpeg_color_range
  //
  YAE_API AVColorRange
  to_ffmpeg_color_range(TColorRangeId r);

  //----------------------------------------------------------------
  // init_abc_to_rgb_matrix
  //
  // Fill in the m3x4 matrix for color conversion from
  // input color format ABC to full-range RGB:
  //
  // [R, G, B]T = m3x4 * [A, B, C, 1]T
  //
  // NOTE: ABC and RGB are expressed in the [0, 1] range,
  //       not [0, 255].
  //
  // NOTE: Here ABC typically refers to YUV input color format,
  //       however it doesn't have to be YUV.
  //
  YAE_API bool
  init_abc_to_rgb_matrix(double * m3x4, const VideoTraits & vtts);

}


#endif // YAE_PIXEL_FORMAT_FFMPEG_H_
