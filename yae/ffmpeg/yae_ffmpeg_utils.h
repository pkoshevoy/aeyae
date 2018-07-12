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
#include "../utils/yae_log.h"
#include "../video/yae_video.h"

// standard C++ library:
#include <string>
#include <iostream>

// ffmpeg includes:
extern "C"
{
#include <libavfilter/avfilter.h>
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
  // ensure_ffmpeg_initialized
  //
  YAE_API void
  ensure_ffmpeg_initialized();

  //----------------------------------------------------------------
  // av_strerr
  //
  YAE_API std::string av_strerr(int errnum);

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

  //----------------------------------------------------------------
  // getDictionary
  //
  YAE_API void
  getDictionary(TDictionary & dict, const AVDictionary * avdict);

  //----------------------------------------------------------------
  // setDictionary
  //
  YAE_API void
  setDictionary(AVDictionary *& avdict, const TDictionary & dict);


  //----------------------------------------------------------------
  // LogToFFmpeg
  //
  struct YAE_API LogToFFmpeg : public IMessageCarrier
  {
    // virtual:
    void destroy();

    //! a prototype factory method for constructing objects of the same kind,
    //! but not necessarily deep copies of the original prototype object:
    // virtual:
    LogToFFmpeg * clone() const
    { return new LogToFFmpeg(); }

    // virtual:
    const char * name() const
    { return "LogToFFmpeg"; }

    // virtual:
    const char * guid() const
    { return "b392b7fc-f08b-438a-95a7-7cc210d634bf"; }

    // virtual:
    ISettingGroup * settings()
    { return NULL; }

    // virtual:
    IMessageCarrier::TPriority priorityThreshold() const
    { return threshold_; }

    // virtual:
    void setPriorityThreshold(IMessageCarrier::TPriority priority)
    { threshold_ = priority; }

    // virtual:
    void deliver(TPriority priority, const char * src, const char * msg);

  protected:
    IMessageCarrier::TPriority threshold_;
  };

}


#endif // YAE_FFMPEG_UTILS_H_
