// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae_ffmpeg_utils.h"

// standard:
#include <stdarg.h>
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
#include <libavutil/log.h>
}


namespace yae
{

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
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
#endif

  YAE_DISABLE_DEPRECATION_WARNINGS

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
      avcodec_register_all();
      avfilter_register_all();
      av_register_all();

      avformat_network_init();

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
      av_lockmgr_register(&lockManager);
#endif

      ffmpeg_initialized = true;
    }
  }

  YAE_ENABLE_DEPRECATION_WARNINGS

  //----------------------------------------------------------------
  // av_strerr
  //
  std::string
  av_strerr(int errnum)
  {
#ifdef AV_ERROR_MAX_STRING_SIZE
    static const std::size_t buffer_size = AV_ERROR_MAX_STRING_SIZE;
#else
    static const std::size_t buffer_size = 256;
#endif

    char errbuf[buffer_size] = { 0 };
    ::av_strerror(errnum, errbuf, sizeof(errbuf));

    return std::string(errbuf);
  }

  //----------------------------------------------------------------
  // dump_averror
  //
  std::ostream &
  dump_averror(std::ostream & os, int err)
  {
    os << "AVERROR: " << yae::av_strerr(err) << std::endl;
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

  //----------------------------------------------------------------
  // getDictionary
  //
  void
  getDictionary(TDictionary & dict, const AVDictionary * av_dict)
  {
    const AVDictionaryEntry * iter = NULL;
    while ((iter = av_dict_get(av_dict, "", iter, AV_DICT_IGNORE_SUFFIX)))
    {
      std::string key(iter->key);
      std::string value(iter->value);
      dict[key] = value;
    }
  }

  //----------------------------------------------------------------
  // setDictionary
  //
  void
  setDictionary(AVDictionary *& av_dict, const TDictionary & dict)
  {
    for (TDictionary::const_iterator
           i = dict.begin(); i != dict.end(); ++i)
    {
      const std::string & k = i->first;
      const std::string & v = i->second;
      av_dict_set(&av_dict, k.c_str(), v.c_str(), 0);
    }
  }


  //----------------------------------------------------------------
  // LogToFFmpeg::destroy
  //
  void
  LogToFFmpeg::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // LogToFFmpeg::deliver
  //
  void
  LogToFFmpeg::deliver(int priority,
                       const char * source,
                       const char * message)
  {
    if (priority < threshold_)
    {
      return;
    }

    int log_level =
      priority < TLog::kInfo ? AV_LOG_DEBUG :
      priority < TLog::kWarning ? AV_LOG_INFO :
      priority < TLog::kError ? AV_LOG_WARNING :
      AV_LOG_ERROR;

    av_log(NULL, log_level, "%s: %s\n", source, message);
  }

  //----------------------------------------------------------------
  // av_log_callback
  //
  static void
  av_log_callback(void * ctx, int level, const char * format, va_list args)
  {
    static boost::mutex * mutex = new boost::mutex();
    boost::lock_guard<boost::mutex> lock(*mutex);
    std::string message = yae::vstrfmt(format, args);
    fprintf(stderr, "%s", message.c_str());
    fflush(stderr);
    // YAE_BREAKPOINT_IF(level < AV_LOG_WARNING);
  }

  //----------------------------------------------------------------
  // AvLog
  //
  struct AvLog : public TLog
  {
    AvLog()
    {
      assign(std::string("av_log"), new LogToFFmpeg());
      // av_log_set_callback(&av_log_callback);
    }
  };


  //----------------------------------------------------------------
  // logger
  //
  TLog & logger()
  {
    static AvLog * singleton = new AvLog();
    return *singleton;
  }

}
