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
#include <yaeAPI.h>
#include <yaePixelFormats.h>

// ffmpeg includes:
extern "C"
{
#include <libavutil/avutil.h>
}


namespace yae
{
  //----------------------------------------------------------------
  // ffmpeg_to_yae
  // 
  YAE_API TPixelFormatId
  ffmpeg_to_yae(enum PixelFormat ffmpegPixelFormat);

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  // 
  YAE_API enum PixelFormat
  yae_to_ffmpeg(TPixelFormatId yaePixelFormat);
}


#endif // YAE_PIXEL_FORMAT_FFMPEG_H_
