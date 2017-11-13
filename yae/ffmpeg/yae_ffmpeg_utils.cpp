// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_ffmpeg_utils.h"

// standard C++ library:
#include <string>
#include <cstring>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavformat/avformat.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // lockManager
  //
  static int
  lockManager(void ** context, enum AVLockOp op)
  {
    try
    {
      switch (op)
      {
        case AV_LOCK_CREATE:
        {
          *context = new boost::mutex();
        }
        break;

        case AV_LOCK_OBTAIN:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->lock();
        }
        break;

        case AV_LOCK_RELEASE:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->unlock();
        }
        break;

        case AV_LOCK_DESTROY:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          delete mtx;
        }
        break;

        default:
          YAE_ASSERT(false);
          return -1;
      }

      return 0;
    }
    catch (...)
    {}

    return -1;
  }

  //----------------------------------------------------------------
  // ensure_ffmpeg_initialized
  //
  void
  ensure_ffmpeg_initialized()
  {
    // flag indicating whether av_register_all has been called already:
    static bool ffmpeg_initialized = false;

    if (!ffmpeg_initialized)
    {
      av_log_set_flags(AV_LOG_SKIP_REPEATED);
      avfilter_register_all();
      av_register_all();

      avformat_network_init();

      av_lockmgr_register(&lockManager);
      ffmpeg_initialized = true;
    }
  }

  //----------------------------------------------------------------
  // dump_averror
  //
  std::ostream &
  dump_averror(std::ostream & os, int err)
  {
#ifdef AV_ERROR_MAX_STRING_SIZE
    static const std::size_t buffer_size = AV_ERROR_MAX_STRING_SIZE;
#else
    static const std::size_t buffer_size = 256;
#endif

    char errbuf[buffer_size] = { 0 };
    av_strerror(err, errbuf, sizeof(errbuf));
    os << "AVERROR: " << errbuf << std::endl;
    return os;
  }

  //----------------------------------------------------------------
  // lookup_src
  //
  AVFilterContext *
  lookup_src(AVFilterContext * filter, const char * name)
  {
    if (!filter)
    {
      return NULL;
    }

    if (filter->nb_inputs == 0 &&
        filter->nb_outputs == 1 &&
        std::strcmp(filter->filter->name, name) == 0)
    {
      return filter;
    }

    for (unsigned int i = 0; i < filter->nb_inputs; i++)
    {
      AVFilterContext * found = lookup_src(filter->inputs[i]->src, name);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // lookup_sink
  //
  AVFilterContext *
  lookup_sink(AVFilterContext * filter, const char * name)
  {
    if (!filter)
    {
      return NULL;
    }

    if (filter->nb_inputs == 1 &&
        filter->nb_outputs == 0 &&
        std::strcmp(filter->filter->name, name) == 0)
    {
      return filter;
    }

    for (unsigned int i = 0; i < filter->nb_outputs; i++)
    {
      AVFilterContext * found = lookup_sink(filter->outputs[i]->dst, name);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // ffmpeg_to_yae
  //
  bool
  ffmpeg_to_yae(enum AVSampleFormat givenFormat,
                TAudioSampleFormat & sampleFormat,
                TAudioChannelFormat & channelFormat)
  {
    channelFormat =
      (givenFormat == AV_SAMPLE_FMT_U8  ||
       givenFormat == AV_SAMPLE_FMT_S16 ||
       givenFormat == AV_SAMPLE_FMT_S32 ||
       givenFormat == AV_SAMPLE_FMT_FLT ||
       givenFormat == AV_SAMPLE_FMT_DBL) ?
      kAudioChannelsPacked : kAudioChannelsPlanar;

    switch (givenFormat)
    {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
        sampleFormat = kAudio8BitOffsetBinary;
        break;

      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
#ifdef __BIG_ENDIAN__
        sampleFormat = kAudio16BitBigEndian;
#else
        sampleFormat = kAudio16BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
#ifdef __BIG_ENDIAN__
        sampleFormat = kAudio32BitBigEndian;
#else
        sampleFormat = kAudio32BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        sampleFormat = kAudio32BitFloat;
        break;

      case AV_SAMPLE_FMT_DBL:
      case AV_SAMPLE_FMT_DBLP:
        sampleFormat = kAudio64BitDouble;
        break;

      default:
        channelFormat = kAudioChannelFormatInvalid;
        sampleFormat = kAudioInvalidFormat;
        return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  enum AVSampleFormat
  yae_to_ffmpeg(TAudioSampleFormat sampleFormat,
                TAudioChannelFormat channelFormat)
  {
    bool planar = channelFormat == kAudioChannelsPlanar;

    switch (sampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return (planar ? AV_SAMPLE_FMT_U8P : AV_SAMPLE_FMT_U8);

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        YAE_ASSERT(sampleFormat == kAudio16BitNative);
        return (planar ? AV_SAMPLE_FMT_S16P : AV_SAMPLE_FMT_S16);

      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        YAE_ASSERT(sampleFormat == kAudio32BitNative);
        return (planar ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S32);

      case kAudio32BitFloat:
        return (planar ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT);

      case kAudio64BitDouble:
        return (planar ? AV_SAMPLE_FMT_DBLP : AV_SAMPLE_FMT_DBL);

      default:
        break;
    }

    YAE_ASSERT(false);
    return AV_SAMPLE_FMT_NONE;
  }


  //----------------------------------------------------------------
  // getTrackLang
  //
  const char *
  getTrackLang(const AVDictionary * metadata)
  {
    const AVDictionaryEntry * lang = av_dict_get(metadata,
                                                 "language",
                                                 NULL,
                                                 0);

    if (lang)
    {
      return lang->value;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // getTrackName
  //
  const char *
  getTrackName(const AVDictionary * metadata)
  {
    const AVDictionaryEntry * name = av_dict_get(metadata,
                                                 "name",
                                                 NULL,
                                                 0);
    if (name)
    {
      return name->value;
    }

    const AVDictionaryEntry * title = av_dict_get(metadata,
                                                  "title",
                                                  NULL,
                                                  0);
    if (title)
    {
      return title->value;
    }

    return NULL;
  }

}
