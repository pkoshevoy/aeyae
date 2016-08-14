// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_UTILS_H_
#define YAE_FFMPEG_UTILS_H_

// aeyae:
#include "../api/yae_api.h"
#include "../video/yae_video.h"

// standard C++ library:
#include <string>
#include <iostream>

// ffmpeg includes:
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}


//----------------------------------------------------------------
// YAE_ASSERT_NO_AVERROR_OR_RETURN
//
#define YAE_ASSERT_NO_AVERROR_OR_RETURN(err, ret)       \
  do {                                                  \
    if (err < 0)                                        \
    {                                                   \
      dump_averror(std::cerr, err);                     \
      YAE_ASSERT(false);                                \
      return ret;                                       \
    }                                                   \
  } while (0)

//----------------------------------------------------------------
// YAE_ASSERT_OR_RETURN
//
#define YAE_ASSERT_OR_RETURN(predicate, ret)            \
  do {                                                  \
    if (!(predicate))                                   \
    {                                                   \
      YAE_ASSERT(false);                                \
      return ret;                                       \
    }                                                   \
  } while (0)


namespace yae
{

  //----------------------------------------------------------------
  // dump_averror
  //
  YAE_API std::ostream &
  dump_averror(std::ostream & os, int err);

  //----------------------------------------------------------------
  // lookup_src
  //
  YAE_API AVFilterContext *
  lookup_src(AVFilterContext * filter, const char * name);

  //----------------------------------------------------------------
  // lookup_sink
  //
  YAE_API AVFilterContext *
  lookup_sink(AVFilterContext * filter, const char * name);

  //----------------------------------------------------------------
  // ffmpeg_to_yae
  //
  YAE_API bool
  ffmpeg_to_yae(enum AVSampleFormat givenFormat,
                TAudioSampleFormat & sampleFormat,
                TAudioChannelFormat & channelFormat);

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  YAE_API enum AVSampleFormat
  yae_to_ffmpeg(TAudioSampleFormat sampleFormat,
                TAudioChannelFormat channelFormat);

  //----------------------------------------------------------------
  // getTrackLang
  //
  YAE_API const char *
  getTrackLang(const AVDictionary * metadata);

  //----------------------------------------------------------------
  // getTrackName
  //
  YAE_API const char *
  getTrackName(const AVDictionary * metadata);

}


#endif // YAE_FFMPEG_UTILS_H_
