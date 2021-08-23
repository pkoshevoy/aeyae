// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system:
#ifdef _WIN32
#include <windows.h>
#endif

// standard:
#include <stdarg.h>
#include <string>
#include <cstring>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#endif

// ffmpeg includes:
extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/log.h>
}

// aeyae:
#include "yae_ffmpeg_utils.h"
#include "../utils/yae_time.h"

// namespace shortcut:
namespace fs = boost::filesystem;


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
      av_log_set_level(AV_LOG_INFO);
      av_log_set_flags(AV_LOG_SKIP_REPEATED);
#if 0
      avcodec_register_all();
      avfilter_register_all();
      av_register_all();
#endif
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
  // LogToFFmpeg::setPriorityThreshold
  //
  void
  LogToFFmpeg::setPriorityThreshold(int priority)
  {
    threshold_ = priority;

    int level =
      priority < TLog::kInfo ? AV_LOG_DEBUG :
      priority < TLog::kWarning ? AV_LOG_INFO :
      priority < TLog::kError ? AV_LOG_WARNING :
      AV_LOG_ERROR;

    av_log_set_level(level);
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

    // add timestamp to the message:
    std::ostringstream oss;
    TTime now = TTime::now();
    int64_t now_usec = now.get(1000000);
    oss << yae::unix_epoch_time_to_localtime_str(now.get(1))
        << '.'
        << std::setw(6) << std::setfill('0') << (now_usec % 1000000);

    av_log(NULL, log_level, "%s %s: %s\n",
           oss.str().c_str(),
           source,
           message);
  }

#ifdef _WIN32
  //----------------------------------------------------------------
  // av_log_callback
  //
  static void
  av_log_callback(void * ctx, int level, const char * format, va_list args)
  {
    static boost::mutex * mutex = new boost::mutex();
    boost::lock_guard<boost::mutex> lock(*mutex);
    std::string message = yae::vstrfmt(format, args);

#ifdef _WIN32
    OutputDebugStringA( message.c_str());
#else
    fprintf(stderr, "%s", message.c_str());
    fflush(stderr);
#endif
    // YAE_BREAKPOINT_IF(level < AV_LOG_WARNING);
  }
#endif

  //----------------------------------------------------------------
  // AvLog
  //
  struct AvLog : public TLog
  {
    AvLog()
    {
      ensure_ffmpeg_initialized();

      LogToFFmpeg * log_to_ffmpeg = new LogToFFmpeg();
#ifdef NDEBUG
      log_to_ffmpeg->setPriorityThreshold(yae::TLog::kInfo);
#else
      log_to_ffmpeg->setPriorityThreshold(yae::TLog::kDebug);
#endif
      assign(std::string("av_log"), log_to_ffmpeg);
#ifdef _WIN32
      av_log_set_callback(&av_log_callback);
#endif
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


  //----------------------------------------------------------------
  // assign_frame
  //
  static void
  assign_frame(::AVFrame & dst, const ::AVFrame & src)
  {
    if (&dst != &src)
    {
      av_frame_unref(&dst);

      if (src.data[0] || src.hw_frames_ctx)
      {
        av_frame_ref(&dst, &src);
      }
      else
      {
        dst.format = src.format;
        dst.width = src.width;
        dst.height = src.height;
        dst.channels = src.channels;
        dst.channel_layout = src.channel_layout;
        dst.nb_samples = src.nb_samples;
        av_frame_copy_props(&dst, &src);
      }
    }
  }


  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm(const AVFrame * frame):
    frame_(av_frame_alloc())
  {
    if (frame)
    {
      assign_frame(*frame_, *frame);
    }
  }

  //----------------------------------------------------------------
  // AvFrm::AvFrm
  //
  AvFrm::AvFrm(const AvFrm & frame):
    frame_(av_frame_alloc())
  {
    assign_frame(*frame_, *(frame.frame_));
  }

  //----------------------------------------------------------------
  // AvFrm::~AvFrm
  //
  AvFrm::~AvFrm()
  {
    av_frame_free(&frame_);
  }

  //----------------------------------------------------------------
  // AvFrm::operator =
  //
  AvFrm &
  AvFrm::operator = (const AvFrm & frame)
  {
    if (this != &frame)
    {
      assign_frame(*frame_, *(frame.frame_));
    }

    return *this;
  }

  //----------------------------------------------------------------
  // sw_pix_fmt
  //
  AVPixelFormat
  sw_pix_fmt(const ::AVFrame & frame)
  {
    if (frame.hw_frames_ctx)
    {
      const AVHWFramesContext * hw_frames_ctx =
        (const AVHWFramesContext *)(frame.hw_frames_ctx->data);
      YAE_ASSERT(hw_frames_ctx);

      if (hw_frames_ctx)
      {
        return hw_frames_ctx->sw_format;
      }
    }

    return (AVPixelFormat)(frame.format);
  }

  //----------------------------------------------------------------
  // AvFrm::sw_pix_fmt
  //
  AVPixelFormat
  AvFrm::sw_pix_fmt() const
  {
    return yae::sw_pix_fmt(*frame_);
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::AvFrmSpecs
  //
  AvFrmSpecs::AvFrmSpecs()
  {
    clear();
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::AvFrmSpecs
  //
  AvFrmSpecs::AvFrmSpecs(const AVFrame & src)
  {
    assign(src);
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::AvFrmSpecs
  //
  AvFrmSpecs::AvFrmSpecs(const AvFrm & src)
  {
    assign(src.get());
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::clear
  //
  void
  AvFrmSpecs::clear()
  {
    yae::clear_specs(*this);
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::assign
  //
  void
  AvFrmSpecs::assign(const AVFrame & src)
  {
    width = src.width;
    height = src.height;
    format = yae::sw_pix_fmt(src);
    colorspace = src.colorspace;
    color_range = src.color_range;
    color_primaries = src.color_primaries;
    color_trc = src.color_trc;
    chroma_location = src.chroma_location;
    sample_aspect_ratio = src.sample_aspect_ratio;
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::override_with
  //
  AvFrmSpecs &
  AvFrmSpecs::override_with(const AvFrmSpecs & specs)
  {
    yae::override_specs(*this, specs);
    return *this;
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::add_missing_specs
  //
  AvFrmSpecs &
  AvFrmSpecs::add_missing_specs(const AvFrmSpecs & specs)
  {
    yae::add_missing_specs(*this, specs);
    return *this;
  }

  //----------------------------------------------------------------
  // AvFrmSpecs::guess_missing_specs
  //
  AvFrmSpecs &
  AvFrmSpecs::guess_missing_specs()
  {
    *this = yae::guess_specs(*this);
    return *this;
  }


  //----------------------------------------------------------------
  // guess_specs
  //
  AvFrmSpecs
  guess_specs(const AvFrmSpecs & src)
  {
    const AVPixelFormat pix_fmt = src.get_pix_fmt();
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    const bool is_rgb = desc ? yae::is_rgb(*desc) : false;

    AvFrmSpecs specs = copy_specs(src);

    specs.color_range =
      (src.color_range != AVCOL_RANGE_UNSPECIFIED) ? src.color_range :
      is_rgb ? AVCOL_RANGE_JPEG :
      AVCOL_RANGE_MPEG;

    specs.chroma_location =
      (src.chroma_location != AVCHROMA_LOC_UNSPECIFIED) ? src.chroma_location :
      AVCHROMA_LOC_UNSPECIFIED;

    bool has_colorspace = (src.colorspace != AVCOL_SPC_UNSPECIFIED &&
                           src.colorspace != AVCOL_SPC_RESERVED);

    bool has_primaries = (src.color_primaries != AVCOL_PRI_UNSPECIFIED &&
                          src.color_primaries != AVCOL_PRI_RESERVED0 &&
                          src.color_primaries != AVCOL_PRI_RESERVED);

    bool has_trc = (src.color_trc != AVCOL_TRC_UNSPECIFIED &&
                    src.color_trc != AVCOL_TRC_RESERVED0 &&
                    src.color_trc != AVCOL_TRC_RESERVED);

    if (has_colorspace && !has_primaries)
    {
      specs.color_primaries =
        (src.colorspace == AVCOL_SPC_RGB ||
         src.colorspace == AVCOL_SPC_BT709) ? AVCOL_PRI_BT709 :
        (src.colorspace == AVCOL_SPC_FCC) ? AVCOL_PRI_BT470M :
        (src.colorspace == AVCOL_SPC_BT470BG) ? AVCOL_PRI_BT470BG :
        (src.colorspace == AVCOL_SPC_SMPTE170M) ? AVCOL_PRI_SMPTE170M :
        (src.colorspace == AVCOL_SPC_SMPTE240M) ? AVCOL_PRI_SMPTE240M :
        (src.colorspace == AVCOL_SPC_BT2020_NCL ||
         src.colorspace == AVCOL_SPC_BT2020_CL ||
         src.colorspace == AVCOL_SPC_ICTCP) ? AVCOL_PRI_BT2020 :
        src.color_primaries;
    }

    if (has_colorspace && !has_trc)
    {
      specs.color_trc =
        (src.colorspace == AVCOL_SPC_RGB) ? AVCOL_TRC_IEC61966_2_1 :
        (src.colorspace == AVCOL_SPC_BT709) ? AVCOL_TRC_BT709 :
        (src.colorspace == AVCOL_SPC_FCC) ? AVCOL_TRC_GAMMA22 :
        (src.colorspace == AVCOL_SPC_BT470BG) ? AVCOL_TRC_GAMMA28 :
        (src.colorspace == AVCOL_SPC_SMPTE170M) ? AVCOL_TRC_SMPTE170M :
        (src.colorspace == AVCOL_SPC_SMPTE240M) ? AVCOL_TRC_SMPTE240M :
        // just guessing here ... could be bt709 or ARIB STD-B67
        (src.colorspace == AVCOL_SPC_BT2020_NCL ||
         src.colorspace == AVCOL_SPC_BT2020_CL ||
         src.colorspace == AVCOL_SPC_ICTCP) ? AVCOL_TRC_SMPTE2084 :
        src.color_trc;
    }

    if (!has_colorspace && has_primaries && has_trc)
    {
      // derive based on primaries and trc:
      specs.colorspace =
        (src.color_primaries == AVCOL_PRI_BT709 &&
         src.color_trc == AVCOL_TRC_BT709) ? AVCOL_SPC_BT709 :

        (src.color_primaries == AVCOL_PRI_BT470M &&
         src.color_trc == AVCOL_TRC_GAMMA22) ? AVCOL_SPC_FCC :

        (src.color_primaries == AVCOL_PRI_BT470BG &&
         src.color_trc == AVCOL_TRC_GAMMA28) ? AVCOL_SPC_BT470BG :

        (src.color_primaries == AVCOL_PRI_SMPTE170M &&
         src.color_trc == AVCOL_TRC_SMPTE170M) ? AVCOL_SPC_SMPTE170M :

        (src.color_primaries == AVCOL_PRI_SMPTE240M &&
         src.color_trc == AVCOL_TRC_SMPTE240M) ? AVCOL_SPC_SMPTE240M :

        (src.color_primaries == AVCOL_PRI_BT2020 &&
         (src.color_trc == AVCOL_TRC_BT2020_10 ||
          src.color_trc == AVCOL_TRC_BT2020_12 ||
          src.color_trc == AVCOL_TRC_SMPTE2084 ||
          src.color_trc == AVCOL_TRC_ARIB_STD_B67)) ? AVCOL_SPC_BT2020_NCL :

        (src.color_primaries == AVCOL_PRI_SMPTE428 &&
         src.color_trc == AVCOL_TRC_SMPTE428) ? AVCOL_SPC_RGB :

        src.colorspace;
    }

    if (yae::has_color_specs(specs))
    {
      return specs;
    }

    bool got_colorspace = (specs.colorspace != AVCOL_SPC_UNSPECIFIED &&
                           specs.colorspace != AVCOL_SPC_RESERVED);

    bool got_primaries = (specs.color_primaries != AVCOL_PRI_UNSPECIFIED &&
                          specs.color_primaries != AVCOL_PRI_RESERVED0 &&
                          specs.color_primaries != AVCOL_PRI_RESERVED);

    bool got_trc = (specs.color_trc != AVCOL_TRC_UNSPECIFIED &&
                    specs.color_trc != AVCOL_TRC_RESERVED0 &&
                    specs.color_trc != AVCOL_TRC_RESERVED);

    const bool is_sd =
      (src.width && src.width < 1280) &&
      (src.height && src.height < 720) &&
      is_less_than(src, 1280, 720);

    if (is_rgb)
    {
      specs.colorspace =
        got_colorspace ? specs.colorspace : AVCOL_SPC_RGB;

      specs.color_primaries =
        got_primaries ? specs.color_primaries : AVCOL_PRI_BT709;

      specs.color_trc =
        got_trc ? specs.color_trc : AVCOL_TRC_IEC61966_2_1;
    }
    else if (is_sd)
    {
      // assume BT.601:
      specs.colorspace =
        got_colorspace ? specs.colorspace : AVCOL_SPC_SMPTE170M;

      specs.color_primaries =
        got_primaries ? specs.color_primaries : AVCOL_PRI_SMPTE170M;

      specs.color_trc =
        got_trc ? specs.color_trc : AVCOL_TRC_SMPTE170M;
    }
    else
    {
      // assume BT.709:
      specs.colorspace =
        got_colorspace ? specs.colorspace : AVCOL_SPC_BT709;

      specs.color_primaries =
        got_primaries ? specs.color_primaries : AVCOL_PRI_BT709;

      specs.color_trc =
        got_trc ? specs.color_trc : AVCOL_TRC_BT709;
    }

    return specs;
  }

  //----------------------------------------------------------------
  // change_specs
  //
  AvFrmSpecs
  change_specs(const AvFrmSpecs & src_specs,
               AVPixelFormat dst_pix_fmt,
               int dst_width,
               int dst_height)
  {
    if ((src_specs.get_pix_fmt() == dst_pix_fmt) &&
        (src_specs.width == dst_width || dst_width <= 0) &&
        (src_specs.height == dst_height || dst_height <= 0))
    {
      return src_specs;
    }

    AvFrmSpecs dst_specs = src_specs;
    dst_specs.format = dst_pix_fmt;
    dst_specs.width = (dst_width <= 0) ? src_specs.width : dst_width;
    dst_specs.height = (dst_height <= 0) ? src_specs.height : dst_height;

    const AVPixFmtDescriptor * dst_desc =
      av_pix_fmt_desc_get(dst_specs.get_pix_fmt());
    const AVComponentDescriptor & luma = dst_desc->comp[0];

    if (luma.depth > 8)
    {
      // pass through HDR as-is:
      dst_specs.color_range = src_specs.color_range;
      dst_specs.colorspace = src_specs.colorspace;
      dst_specs.color_primaries = src_specs.color_primaries;
      dst_specs.color_trc = src_specs.color_trc;
    }
    else
    {
      // we want SDR:
      dst_specs.color_range = AVCOL_RANGE_MPEG;
      dst_specs.colorspace = AVCOL_SPC_BT709;
      dst_specs.color_primaries = AVCOL_PRI_BT709;
      dst_specs.color_trc = AVCOL_TRC_BT709;
    }

    return dst_specs;
  }

}
