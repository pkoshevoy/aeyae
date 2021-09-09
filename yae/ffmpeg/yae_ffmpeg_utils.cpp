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
#include "yae_demuxer.h"
#include "yae_ffmpeg_utils.h"
#include "yae_track.h"
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
  // av_errstr
  //
  std::string
  av_errstr(int errnum)
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
#ifdef NDEBUG
      priority < TLog::kInfo ? AV_LOG_DEBUG :
#else
      priority < TLog::kInfo ? AV_LOG_INFO :
#endif
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
#ifdef NDEBUG
      priority < TLog::kInfo ? AV_LOG_DEBUG :
#else
      priority < TLog::kInfo ? AV_LOG_INFO :
#endif
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
  // AvFrm::hwframe_transfer_data
  //
  int
  AvFrm::hwframe_transfer_data()
  {
    if (!frame_->hw_frames_ctx)
    {
      return 0;
    }

    AvFrm tmp;
    int err = av_hwframe_transfer_data(tmp.frame_, frame_, 0);

    if (err < 0)
    {
      yae_elog("av_hwframe_transfer_data failed, %s",
               av_errstr(err).c_str());
    }
    else
    {
      av_frame_copy_props(tmp.frame_, frame_);
      assign_frame(*frame_, *(tmp.frame_));
    }

    return err;
  }

  //----------------------------------------------------------------
  // AvFrm::hwupload
  //
  int
  AvFrm::hwupload(AVBufferRef * hw_frames_ctx)
  {
    if (hw_frames_ctx == frame_->hw_frames_ctx)
    {
      return 0;
    }

    YAE_ASSERT(hw_frames_ctx);
    if (!hw_frames_ctx)
    {
      return AVERROR(ENOSYS);
    }

    const AVHWFramesContext * ctx =
      (const AVHWFramesContext *)(hw_frames_ctx->data);

    if (ctx->sw_format != get_pix_fmt() ||
        ctx->width != frame_->width ||
        ctx->height != frame_->height)
    {
      YAE_ASSERT(false);
      return AVERROR(ENOSYS);
    }

    AvFrm tmp;
    int err = av_hwframe_get_buffer(hw_frames_ctx, tmp.frame_, 0);
    YAE_ASSERT(!err);

    err = av_hwframe_transfer_data(tmp.frame_, frame_, 0);
    YAE_ASSERT(!err);

    if (!err)
    {
      av_frame_copy_props(tmp.frame_, frame_);
      assign_frame(*frame_, *(tmp.frame_));
    }

    return err;
  }

  //----------------------------------------------------------------
  // AvFrm::alloc_video_buffers
  //
  int
  AvFrm::alloc_video_buffers(int format,
                             int width,
                             int height,
                             int align)
  {
    int bufferSize = av_image_get_buffer_size((AVPixelFormat)format,
                                              width,
                                              height,
                                              align);

    YAE_ASSERT(!frame_->buf[0]);
    frame_->buf[0] = av_buffer_alloc(bufferSize + AV_INPUT_BUFFER_PADDING_SIZE);
    frame_->format = format;
    frame_->width = width;
    frame_->height = height;

    return av_image_fill_arrays(frame_->data,
                                frame_->linesize,
                                frame_->buf[0]->data,
                                (AVPixelFormat)format,
                                width,
                                height,
                                align);
  }

  //----------------------------------------------------------------
  // AvFrm::alloc_samples_buffer
  //
  int AvFrm::alloc_samples_buffer(int nb_channels,
                                  int nb_samples,
                                  AVSampleFormat sample_fmt,
                                  int align)
  {
    int buf_size = av_samples_get_buffer_size(frame_->linesize,
                                              nb_channels,
                                              nb_samples,
                                              sample_fmt,
                                              align);
    if (buf_size < 0)
    {
      return buf_size;
    }

    YAE_ASSERT(!frame_->buf[0]);
    frame_->buf[0] = av_buffer_alloc(buf_size + AV_INPUT_BUFFER_PADDING_SIZE);
    int err = av_samples_fill_arrays(frame_->data,
                                     frame_->linesize,
                                     frame_->buf[0]->data,
                                     nb_channels,
                                     nb_samples,
                                     sample_fmt,
                                     align);

    frame_->format = sample_fmt;
    frame_->channels = nb_channels;
    frame_->nb_samples = nb_samples;

    return err;
  }

  //----------------------------------------------------------------
  // AvFrm::make_writable
  //
  int AvFrm::make_writable()
  {
    int err = 0;
    if (!frame_->buf[0])
    {
      // first make it refcounted:
      AvFrm tmp(*this);
      err = av_frame_ref(frame_, tmp.frame_);
      YAE_ASSERT(!err);
    }

    err = av_frame_make_writable(frame_);
    YAE_ASSERT(!err);
    return err;
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

      YAE_ASSERT(specs.colorspace != AVCOL_SPC_RGB || is_rgb);
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


  //----------------------------------------------------------------
  // make_avfrm
  //
  AvFrm
  make_avfrm(AVPixelFormat pix_fmt,
             int luma_w,
             int luma_h,
             AVColorSpace csp,
             AVColorPrimaries pri,
             AVColorTransferCharacteristic trc,
             AVColorRange rng,
             int par_num ,
             int par_den,
             unsigned char fill_luma,
             unsigned char fill_chroma)
  {
    yae::AvFrm frm;
    frm.alloc_video_buffers(pix_fmt, luma_w, luma_h);

    AVFrame & frame = frm.get();
    frame.colorspace = csp;
    frame.color_primaries = pri;
    frame.color_trc = trc;
    frame.color_range = rng;
    frame.sample_aspect_ratio.num = par_num;
    frame.sample_aspect_ratio.den = par_den;
    yae::add_missing_specs(frame, yae::guess_specs(frame));

    const AVPixFmtDescriptor & desc = *(av_pix_fmt_desc_get(pix_fmt));
    int chroma_h = AV_CEIL_RSHIFT(luma_h, desc.log2_chroma_h);

    for (int i = 0; i < desc.nb_components; i++)
    {
      const AVComponentDescriptor & comp = desc.comp[i];
      int h = i ? chroma_h : luma_h;
      unsigned char fill = i ? fill_chroma : fill_luma;
      memset(frame.data[comp.plane], fill, frame.linesize[comp.plane] * h);
    }

    return frm;
  }

  //----------------------------------------------------------------
  // make_444
  //
  template <typename TData>
  static AvFrm
  make_rgb(const TextureGenerator & tex_gen,
           AVPixelFormat pix_fmt,
           int luma_w,
           int luma_h)
  {
    // shortcuts:
    const Colorspace & colorspace = *tex_gen.colorspace();
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    YAE_ASSERT(desc);

    const bool is_rgb = desc ? yae::is_rgb(*desc) : false;
    YAE_ASSERT(is_rgb);

    const AVComponentDescriptor & luma = desc->comp[0];
    const double c1 = ~((~0) << luma.depth);

    yae::AvFrm frm =
      make_avfrm(pix_fmt,
                 luma_w,
                 luma_h,
                 colorspace.av_csp_,
                 colorspace.av_pri_,
                 colorspace.av_trc_,
                 AVCOL_RANGE_JPEG);

    // shortcut:
    AVFrame & frame = frm.get();
    const int r_plane = desc->comp[0].plane;
    const int g_plane = desc->comp[1].plane;
    const int b_plane = desc->comp[2].plane;

    for (int row = 0; row < luma_h; row++)
    {
      uint8_t * rr = (frame.data[r_plane] + row * frame.linesize[r_plane] +
                      desc->comp[0].offset);

      uint8_t * gr = (frame.data[g_plane] + row * frame.linesize[g_plane] +
                      desc->comp[1].offset);

      uint8_t * br = (frame.data[b_plane] + row * frame.linesize[b_plane] +
                      desc->comp[2].offset);

      for (int col = 0; col < luma_w; col++)
      {
        v3x1_t rgb = tex_gen.get_rgb(row, col);
        uint32_t r = uint32_t(c1 * rgb[0]);
        uint32_t g = uint32_t(c1 * rgb[1]);
        uint32_t b = uint32_t(c1 * rgb[2]);

        *((TData *)(rr + col * desc->comp[0].step)) = r << luma.shift;
        *((TData *)(gr + col * desc->comp[1].step)) = g << luma.shift;
        *((TData *)(br + col * desc->comp[2].step)) = b << luma.shift;
      }
    }

    return frm;
  }

  //----------------------------------------------------------------
  // make_444
  //
  template <typename TData>
  static AvFrm
  make_444(const TextureGenerator & tex_gen,
           AVPixelFormat pix_fmt,
           int luma_w,
           int luma_h,
           AVColorRange av_rng)
  {
    // shortcuts:
    const Colorspace & colorspace = *tex_gen.colorspace();
    const bool full_rng = av_rng == AVCOL_RANGE_JPEG;
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    YAE_ASSERT(desc);

    const bool is_ycbcr = desc ? yae::is_ycbcr(*desc) : false;
    YAE_ASSERT(is_ycbcr);

    const AVComponentDescriptor & luma = desc->comp[0];
    const unsigned int full_luma = ~((~0) << luma.depth);
    const unsigned int depth_8 = luma.depth - 8;

    // min luma:
    const double y0 = full_rng ? 0 : (16 << depth_8);

    // max luma:
    const double y1 = full_rng ? full_luma : (235 << depth_8);

    // max chroma:
    const double c1 = full_rng ? full_luma : (240 << depth_8);

    yae::AvFrm frm =
      make_avfrm(pix_fmt,
                 luma_w,
                 luma_h,
                 colorspace.av_csp_,
                 colorspace.av_pri_,
                 colorspace.av_trc_,
                 full_rng ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG);

    // shortcut:
    AVFrame & frame = frm.get();
    const int y_plane = desc->comp[0].plane;
    const int u_plane = desc->comp[1].plane;
    const int v_plane = desc->comp[2].plane;

    for (int row = 0; row < luma_h; row++)
    {
      uint8_t * yr = (frame.data[y_plane] + row * frame.linesize[y_plane] +
                      desc->comp[0].offset);

      uint8_t * ur = (frame.data[u_plane] + row * frame.linesize[u_plane] +
                      desc->comp[1].offset);

      uint8_t * vr = (frame.data[v_plane] + row * frame.linesize[v_plane] +
                      desc->comp[2].offset);

      for (int col = 0; col < luma_w; col++)
      {
        v3x1_t ypbpr = tex_gen.get_ypbpr(row, col);
        uint32_t y = uint32_t(y0 + (y1 - y0) * (ypbpr[0]));
        uint32_t u = uint32_t(y0 + (c1 - y0) * (ypbpr[1] + 0.5));
        uint32_t v = uint32_t(y0 + (c1 - y0) * (ypbpr[2] + 0.5));

        *((TData *)(yr + col * desc->comp[0].step)) = y << luma.shift;
        *((TData *)(ur + col * desc->comp[1].step)) = u << luma.shift;
        *((TData *)(vr + col * desc->comp[2].step)) = v << luma.shift;
      }
    }

    return frm;
  }

  //----------------------------------------------------------------
  // make_422
  //
  template <typename TData>
  static AvFrm
  make_422(const TextureGenerator & tex_gen,
           AVPixelFormat pix_fmt,
           int luma_w,
           int luma_h,
           AVColorRange av_rng)
  {
    // shortcuts:
    const Colorspace & colorspace = *tex_gen.colorspace();
    const bool full_rng = av_rng == AVCOL_RANGE_JPEG;
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    YAE_ASSERT(desc);

    const bool is_ycbcr = desc ? yae::is_ycbcr(*desc) : false;
    YAE_ASSERT(is_ycbcr);

    const AVComponentDescriptor & luma = desc->comp[0];
    const unsigned int full_luma = ~((~0) << luma.depth);
    const unsigned int depth_8 = luma.depth - 8;

    // min luma:
    const double y0 = full_rng ? 0 : (16 << depth_8);

    // max luma:
    const double y1 = full_rng ? full_luma : (235 << depth_8);

    // max chroma:
    const double c1 = full_rng ? full_luma : (240 << depth_8);

    yae::AvFrm frm =
      make_avfrm(pix_fmt,
                 luma_w,
                 luma_h,
                 colorspace.av_csp_,
                 colorspace.av_pri_,
                 colorspace.av_trc_,
                 full_rng ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG);

    // shortcut:
    AVFrame & frame = frm.get();
    const int chroma_w = luma_w >> 1;
    const int y_plane = desc->comp[0].plane;
    const int u_plane = desc->comp[1].plane;
    const int v_plane = desc->comp[2].plane;

    for (int row = 0; row < luma_h; row++)
    {
      uint8_t * yr = (frame.data[y_plane] + row * frame.linesize[y_plane] +
                      desc->comp[0].offset);

      uint8_t * ur = (frame.data[u_plane] + row * frame.linesize[u_plane] +
                      desc->comp[1].offset);

      uint8_t * vr = (frame.data[v_plane] + row * frame.linesize[v_plane] +
                      desc->comp[2].offset);

      for (int col = 0; col < chroma_w; col++)
      {
        // luma colums:
        int col_0 = col << 1;
        int col_1 = col_0 + 1;

        v3x1_t ypbpr_00 = tex_gen.get_ypbpr(row, col_0);
        v3x1_t ypbpr_01 = tex_gen.get_ypbpr(row, col_1);

        uint32_t y_00 = uint32_t(y0 + (y1 - y0) * (ypbpr_00[0]));
        uint32_t y_01 = uint32_t(y0 + (y1 - y0) * (ypbpr_01[0]));

        uint32_t u = uint32_t(y0 + (c1 - y0) * ((ypbpr_00[1] +
                                                 ypbpr_01[1]) * 0.5 +
                                                0.5));

        uint32_t v = uint32_t(y0 + (c1 - y0) * ((ypbpr_00[2] +
                                                 ypbpr_01[2]) * 0.5 +
                                                0.5));

        *((TData *)(yr + col_0 * desc->comp[0].step)) = y_00 << luma.shift;
        *((TData *)(yr + col_1 * desc->comp[0].step)) = y_01 << luma.shift;

        *((TData *)(ur + col * desc->comp[1].step)) = u << luma.shift;
        *((TData *)(vr + col * desc->comp[2].step)) = v << luma.shift;
      }
    }

    return frm;
  }

  //----------------------------------------------------------------
  // make_420
  //
  template <typename TData>
  static AvFrm
  make_420(const TextureGenerator & tex_gen,
           AVPixelFormat pix_fmt,
           int luma_w,
           int luma_h,
           AVColorRange av_rng)
  {
    // shortcuts:
    const Colorspace & colorspace = *tex_gen.colorspace();
    const bool full_rng = av_rng == AVCOL_RANGE_JPEG;
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    YAE_ASSERT(desc);

    const bool is_ycbcr = desc ? yae::is_ycbcr(*desc) : false;
    YAE_ASSERT(is_ycbcr);

    const AVComponentDescriptor & luma = desc->comp[0];
    const unsigned int full_luma = ~((~0) << luma.depth);
    const unsigned int depth_8 = luma.depth - 8;

    // min luma:
    const double y0 = full_rng ? 0 : (16 << depth_8);

    // max luma:
    const double y1 = full_rng ? full_luma : (235 << depth_8);

    // max chroma:
    const double c1 = full_rng ? full_luma : (240 << depth_8);

    yae::AvFrm frm =
      make_avfrm(pix_fmt,
                 luma_w,
                 luma_h,
                 colorspace.av_csp_,
                 colorspace.av_pri_,
                 colorspace.av_trc_,
                 full_rng ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG);

    // shortcut:
    AVFrame & frame = frm.get();

    const int chroma_w = luma_w >> 1;
    const int chroma_h = luma_h >> 1;
    const int y_plane = desc->comp[0].plane;
    const int u_plane = desc->comp[1].plane;
    const int v_plane = desc->comp[2].plane;

    for (int row = 0; row < chroma_h; row++)
    {
      // luma rows:
      int row_0 = row << 1;
      int row_1 = row_0 + 1;

      uint8_t * yr_0 = (frame.data[y_plane] + row_0 * frame.linesize[y_plane] +
                        desc->comp[0].offset);

      uint8_t * yr_1 = (frame.data[y_plane] + row_1 * frame.linesize[y_plane] +
                        desc->comp[0].offset);

      uint8_t * ur = (frame.data[u_plane] + row * frame.linesize[u_plane] +
                      desc->comp[1].offset);

      uint8_t * vr = (frame.data[v_plane] + row * frame.linesize[v_plane] +
                      desc->comp[2].offset);

      for (int col = 0; col < chroma_w; col++)
      {
        // luma colums:
        int col_0 = col << 1;
        int col_1 = col_0 + 1;

        v3x1_t ypbpr_00 = tex_gen.get_ypbpr(row_0, col_0);
        v3x1_t ypbpr_01 = tex_gen.get_ypbpr(row_0, col_1);
        v3x1_t ypbpr_10 = tex_gen.get_ypbpr(row_1, col_0);
        v3x1_t ypbpr_11 = tex_gen.get_ypbpr(row_1, col_1);

        uint32_t y_00 = uint32_t(y0 + (y1 - y0) * (ypbpr_00[0]));
        uint32_t y_01 = uint32_t(y0 + (y1 - y0) * (ypbpr_01[0]));
        uint32_t y_10 = uint32_t(y0 + (y1 - y0) * (ypbpr_10[0]));
        uint32_t y_11 = uint32_t(y0 + (y1 - y0) * (ypbpr_11[0]));

        uint32_t u = uint32_t(y0 + (c1 - y0) * ((ypbpr_00[1] +
                                                 ypbpr_01[1] +
                                                 ypbpr_10[1] +
                                                 ypbpr_11[1]) * 0.25 +
                                                0.5));
        uint32_t v = uint32_t(y0 + (c1 - y0) * ((ypbpr_00[2] +
                                                 ypbpr_01[2] +
                                                 ypbpr_10[2] +
                                                 ypbpr_11[2]) * 0.25 +
                                                0.5));

        *((TData *)(yr_0 + col_0 * desc->comp[0].step)) = y_00 << luma.shift;
        *((TData *)(yr_0 + col_1 * desc->comp[0].step)) = y_01 << luma.shift;
        *((TData *)(yr_1 + col_0 * desc->comp[0].step)) = y_10 << luma.shift;
        *((TData *)(yr_1 + col_1 * desc->comp[0].step)) = y_11 << luma.shift;

        *((TData *)(ur + col * desc->comp[1].step)) = u << luma.shift;
        *((TData *)(vr + col * desc->comp[2].step)) = v << luma.shift;
      }
    }

    return frm;
  }

  //----------------------------------------------------------------
  // make_textured_frame
  //
  AvFrm
  make_textured_frame(const TextureGenerator & tex_gen,
                      AVPixelFormat pix_fmt,
                      int luma_w,
                      int luma_h,
                      AVColorRange av_rng)
  {
    const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(pix_fmt);
    YAE_ASSERT(desc);

    const bool is_ycbcr = desc ? yae::is_ycbcr(*desc) : false;
    const AVComponentDescriptor & luma = desc->comp[0];
    const int depth = luma.depth + luma.shift;

    YAE_ASSERT(desc->log2_chroma_h < 2);
    const bool subsample_rows = desc->log2_chroma_h == 1;

    YAE_ASSERT(desc->log2_chroma_h < 2);
    const bool subsample_cols = desc->log2_chroma_h == 1;

    return
      (depth == 8) ?

      (subsample_cols ?
       (subsample_rows ?
        make_420<uint8_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng) :
        make_422<uint8_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng)) :

       (is_ycbcr ?
        make_444<uint8_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng) :
        make_rgb<uint8_t>(tex_gen, pix_fmt, luma_w, luma_h))) :

      (subsample_cols ?
       (subsample_rows ?
        make_420<uint16_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng) :
        make_422<uint16_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng)) :

       (is_ycbcr ?
        make_444<uint16_t>(tex_gen, pix_fmt, luma_w, luma_h, av_rng) :
        make_rgb<uint16_t>(tex_gen, pix_fmt, luma_w, luma_h)));
  }

  //----------------------------------------------------------------
  // save_as
  //
  bool
  save_as(const std::string & path,
          const yae::AvFrm & src,
          const yae::TTime & frame_dur)
  {
    std::string url = al::starts_with(path, "file:") ? path : ("file:" + path);
    yae::Rational framerate(frame_dur.base_, frame_dur.time_);
    yae::Rational timebase(1, frame_dur.base_);

    // setup output format:
    yae::AvOutputContextPtr muxer_ptr(avformat_alloc_context());
    AVFormatContext * muxer = muxer_ptr.get();
    muxer->url = av_strndup(url.c_str(), url.size());
    muxer->oformat = av_guess_format(NULL, path.c_str(), NULL);

    AVCodecID codec_id =
      al::ends_with(path, ".png") ? AV_CODEC_ID_PNG :
      muxer->oformat->video_codec;

    const AVCodec * codec = avcodec_find_encoder(codec_id);
    if (!codec)
    {
      yae_elog("avcodec_find_encoder(%i) failed, %s\n",
               muxer->oformat->video_codec, path.c_str());
      return false;
    }

    yae::AvFrm frm = src;
    AVFrame & frame = frm.get();
    if (codec->pix_fmts && !yae::has(codec->pix_fmts, src.get_pix_fmt()))
    {
      // convert to pixel format supported by the encoder:
      yae::AvFrmSpecs specs = src;
      specs.format = codec->pix_fmts[0];
      specs.colorspace = AVCOL_SPC_BT709;
      specs.color_range = AVCOL_RANGE_JPEG;
      specs.color_primaries = AVCOL_PRI_BT709;
      specs.color_trc = AVCOL_TRC_BT709;

      yae::VideoFilterGraph vf;
      vf.setup(src, framerate, timebase, specs);

      yae::AvFrm tmp = src;
      vf.push(&tmp.get());
      vf.push(NULL);

      YAE_ASSERT(vf.pull(&frame, timebase));
    }

    // setup the encoder:
    yae::AvCodecContextPtr encoder_ptr(avcodec_alloc_context3(codec));
    if (!encoder_ptr)
    {
      yae_elog("avcodec_alloc_context3(%i) failed, %s\n",
               muxer->oformat->video_codec, path.c_str());
      YAE_ASSERT(false);
      return false;
    }

    AVCodecContext & encoder = *encoder_ptr;
    encoder.bit_rate = frame.width * frame.height * 8;
    // encoder.rc_max_rate = encoder.bit_rate * 2;
    encoder.width = frame.width;
    encoder.height = frame.height;
    encoder.time_base.num = 1;
    encoder.time_base.den = frame_dur.base_;
    encoder.ticks_per_frame = frame_dur.time_;
    encoder.framerate.num = frame_dur.base_;
    encoder.framerate.den = frame_dur.time_;
    encoder.gop_size = 1; // expressed as number of frames
    encoder.max_b_frames = 0;
    encoder.pix_fmt = (AVPixelFormat)(frame.format);
    encoder.sample_aspect_ratio = frame.sample_aspect_ratio;

    AVDictionary * encoder_opts = NULL;
    int err = avcodec_open2(&encoder, codec, &encoder_opts);
    av_dict_free(&encoder_opts);

    if (err < 0)
    {
      yae_elog("avcodec_open2 error %i: \"%s\"\n",
               err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // setup output streams:
    AVStream * dst = avformat_new_stream(muxer, NULL);
    dst->time_base.num = 1;
    dst->time_base.den = frame_dur.base_;
    dst->avg_frame_rate.num = frame_dur.base_;
    dst->avg_frame_rate.den = frame_dur.time_;
    dst->duration = 1;
    dst->sample_aspect_ratio = frame.sample_aspect_ratio;

    err = avcodec_parameters_from_context(dst->codecpar, &encoder);
    if (err < 0)
    {
      yae_elog("avcodec_parameters_from_context error %i: \"%s\"\n",
               err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // open the muxer:
    AVDictionary * io_opts = NULL;
    err = avio_open2(&(muxer->pb),
                     path.c_str(),
                     AVIO_FLAG_WRITE,
                     NULL,
                     &io_opts);
    av_dict_free(&io_opts);

    if (err < 0)
    {
      yae_elog("avio_open2(%s) error %i: \"%s\"\n",
               path.c_str(), err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // write the header:
    AVDictionary * muxer_opts = NULL;
    av_dict_set_int(&muxer_opts, "update", 1, 0);
    err = avformat_write_header(muxer, &muxer_opts);
    av_dict_free(&muxer_opts);

    if (err < 0)
    {
      yae_elog("avformat_write_header(%s) error %i: \"%s\"\n",
               path.c_str(), err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // send the frame to the encoder:
    frame.key_frame = 1;
    frame.pict_type = AV_PICTURE_TYPE_I;
    frame.pts = av_rescale_q(frame.pts, timebase, dst->time_base);
    err = avcodec_send_frame(&encoder, &frame);
    if (err < 0)
    {
      yae_elog("avcodec_send_frame error %i: \"%s\"\n",
               err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    while (!err)
    {
      yae::AvPkt pkt;
      AVPacket & out = pkt.get();
      err = avcodec_receive_packet(&encoder, &out);
      if (err)
      {
        if (err < 0 && err != AVERROR(EAGAIN) && err != AVERROR_EOF)
        {
          yae_elog("avcodec_receive_packet error %i: \"%s\"\n",
                   err, yae::av_errstr(err).c_str());
          YAE_ASSERT(false);
        }

        break;
      }

      out.stream_index = dst->index;
      out.dts = av_rescale_q(out.dts, encoder.time_base, dst->time_base);
      out.pts = av_rescale_q(out.pts, encoder.time_base, dst->time_base);
      out.duration = av_rescale_q(out.duration,
                                  encoder.time_base,
                                  dst->time_base);

      err = av_interleaved_write_frame(muxer, &out);
      if (err < 0)
      {
        yae_elog("av_interleaved_write_frame(%s) error %i: \"%s\"\n",
                 path.c_str(), err, yae::av_errstr(err).c_str());
        YAE_ASSERT(false);
        return false;
      }

      // flush-out the encoder:
      err = avcodec_send_frame(&encoder, NULL);
    }

    err = av_write_trailer(muxer);
    if (err < 0)
    {
      yae_elog("avformat_write_trailer(%s) error %i: \"%s\"\n",
               path.c_str(), err, yae::av_errstr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    muxer_ptr.reset();
    return true;
  }

  //----------------------------------------------------------------
  // save_as_png
  //
  bool
  save_as_png(const yae::AvFrm & frm,
              const std::string & prefix,
              const yae::TTime & frame_dur)
  {
    const AVPixelFormat pix_fmt = frm.get_pix_fmt();
    const char * pix_fmt_txt = av_get_pix_fmt_name(pix_fmt);
    const AVFrame & frame = frm.get();

    std::string path;
    {
      std::ostringstream oss;
      oss << prefix
          << pix_fmt_txt << '.'
          << frame.width << '.'
          << frame.height << ".png";
      path = oss.str();
    }

    return save_as(path, frm, frame_dur);
  }

  //----------------------------------------------------------------
  // make_hwframes_ctx
  //
  yae::AvBufferRef
  make_hwframes_ctx(AVBufferRef * device_ctx_ref,
                    int width,
                    int height,
                    AVPixelFormat sw_format)
  {
    if (!device_ctx_ref)
    {
      return yae::AvBufferRef();
    }

    yae::AvBufferRef hw(av_hwframe_ctx_alloc(device_ctx_ref));
    AVHWFramesContext * hw_ctx = hw.get<AVHWFramesContext>();
    if (!hw_ctx)
    {
      return yae::AvBufferRef();
    }

    int err = 0;
    AVHWFramesConstraints * hw_caps =
      av_hwdevice_get_hwframe_constraints(device_ctx_ref, NULL);

    if (hw_caps &&
        hw_caps->max_width >= width &&
        hw_caps->max_height >= height)
    {
      hw_ctx->width = width;
      hw_ctx->height = height;
      hw_ctx->format = hw_caps->valid_hw_formats[0];

      hw_ctx->sw_format = AV_PIX_FMT_NONE;
      if (sw_format == AV_PIX_FMT_NONE)
      {
        hw_ctx->sw_format =
          hw_caps->valid_sw_formats ?
          hw_caps->valid_sw_formats[0] :
          AV_PIX_FMT_NONE;
      }
      else
      {
        for (int i = 0; hw_caps->valid_sw_formats[i] != AV_PIX_FMT_NONE; i++)
        {
          if (hw_caps->valid_sw_formats[i] == sw_format)
          {
            hw_ctx->sw_format = sw_format;
            break;
          }
        }
      }

      if (hw_ctx->sw_format != AV_PIX_FMT_NONE)
      {
        err = av_hwframe_ctx_init(hw.ref_);
        YAE_ASSERT(!err);

        if (err)
        {
          hw.reset();
        }
      }
      else
      {
        hw.reset();
      }
    }

    av_hwframe_constraints_free(&hw_caps);

    return err ? yae::AvBufferRef() : hw;
  }

}
