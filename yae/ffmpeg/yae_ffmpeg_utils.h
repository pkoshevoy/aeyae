// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_UTILS_H_
#define YAE_FFMPEG_UTILS_H_

// standard C++ library:
#include <string>
#include <iostream>

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/api/yae_log.h"
#include "yae/api/yae_message_carrier_interface.h"
#include "yae/video/yae_video.h"


//----------------------------------------------------------------
// YAE_ASSERT_NO_AVERROR_OR_RETURN
//
#define YAE_ASSERT_NO_AVERROR_OR_RETURN(err, ret)       \
  do {                                                  \
    if (err < 0)                                        \
    {                                                   \
      yae_error << "AVERROR: " << yae::av_errstr(err);  \
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

  // forward declarations:
  struct TextureGenerator;


  //----------------------------------------------------------------
  // ensure_ffmpeg_initialized
  //
  YAE_API void
  ensure_ffmpeg_initialized();

  //----------------------------------------------------------------
  // av_errstr
  //
  YAE_API std::string av_errstr(int errnum);

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
    int priorityThreshold() const
    { return threshold_; }

    // virtual:
    void setPriorityThreshold(int priority);

    // virtual:
    void deliver(int priority, const char * src, const char * msg);

  protected:
    int threshold_;
  };


  //----------------------------------------------------------------
  // Rational
  //
  struct YAE_API Rational : public AVRational
  {
    Rational(int n = 0, int d = 1)
    {
      AVRational::num = n;
      AVRational::den = d;
    }
  };


  //----------------------------------------------------------------
  // same_avbuffer
  //
  inline bool
  same_avbuffer(const AVBufferRef * a, const AVBufferRef * b)
  {
    return (a && b) ? (a->data == b->data) : (!a && !b);
  }

  //----------------------------------------------------------------
  // AvBufferRef
  //
  // ::AVBufferRef is basically an ffmpeg version of std::shared_ptr
  //
  struct YAE_API AvBufferRef
  {
    // does not increment refcount:
    AvBufferRef():
      ref_(NULL)
    {}

    // does not increment refcount:
    explicit AvBufferRef(::AVBufferRef * ref):
      ref_(ref)
    {}

    // increments refcount:
    AvBufferRef(const AvBufferRef & other):
      ref_(other.ref_ ? ::av_buffer_ref(other.ref_) : NULL)
    {}

    // decrements refcount:
    ~AvBufferRef()
    {
      ::av_buffer_unref(&ref_);
    }

    // increments refcount:
    inline AvBufferRef & operator = (const AvBufferRef & other)
    {
      if (&other != this)
      {
        ::av_buffer_unref(&ref_);
        ref_ = other.ref_ ? ::av_buffer_ref(other.ref_) : NULL;
      }

      return *this;
    }

    // does not increment refcount:
    AvBufferRef & operator = (::AVBufferRef * ref)
    {
      reset(ref);
      return *this;
    }

    void reset(::AVBufferRef * ref = NULL)
    {
      if (ref != ref_)
      {
        ::av_buffer_unref(&ref_);
        ref_ = ref;
      }
    }

    template <typename TData>
    TData * get() const
    {
      TData * buffer = ref_ ? (TData *)(ref_->data) : NULL;
      return buffer;
    }

    inline bool same_as(const ::AVBufferRef * ref) const
    {
      return yae::same_avbuffer(ref_, ref);
    }

    inline int refcount() const
    {
      return ref_ ? ::av_buffer_get_ref_count(ref_) : 0;
    }

    ::AVBufferRef * ref_;
  };

  //----------------------------------------------------------------
  // AvFrm
  //
  struct YAE_API AvFrm
  {
    AvFrm(const AVFrame * frame = NULL);
    AvFrm(const AvFrm & frame);
    ~AvFrm();

    AvFrm & operator = (const AvFrm & frame);

    inline const AVFrame & get() const
    { return *frame_; }

    inline AVFrame & get()
    { return *frame_; }

    inline operator const AVFrame & () const
    { return *frame_; }

    inline operator AVFrame & ()
    { return *frame_; }

    inline AVPixelFormat get_pix_fmt() const
    { return (AVPixelFormat)(get().format); }

    // stop referencing any AVBuffers, reset to initial state:
    inline void clear()
    { *this = AvFrm(); }

    // see if the frame has at least one plane of data:
    inline bool has_data() const
    { return ((frame_->buf[0] && frame_->buf[0]->data) || frame_->data[0]); }

    inline const AVHWFramesContext * get_hw_frames_ctx() const
    {
      return (frame_->hw_frames_ctx ?
              (const AVHWFramesContext *)(frame_->hw_frames_ctx->data) :
              NULL);
    }

    inline int hwdownload()
    { return this->hwframe_transfer_data(); }

    AVPixelFormat sw_pix_fmt() const;

    // if a frame exists in hw context memory (on the GPU, etc...) and
    // we need to manipulate it on the CPU, then call
    // av_hwframe_transfer_data to download it to system memory:
    int hwframe_transfer_data();

    int hwupload(AVBufferRef * hw_frames_ctx);

    int alloc_video_buffers(int format,
                            int width,
                            int height,
                            int align = AV_INPUT_BUFFER_PADDING_SIZE);

    int alloc_samples_buffer(int nb_channels,
                             int nb_samples,
                             AVSampleFormat sample_fmt = AV_SAMPLE_FMT_S16,
                             // 0 == default alignment
                             // 1 == no alignment
                             int align = 0);

    inline int get_buffer(int align = AV_INPUT_BUFFER_PADDING_SIZE)
    { return av_frame_get_buffer(frame_, align); }

    inline int is_writable() const
    { return av_frame_is_writable(const_cast<AVFrame *>(frame_)); }

    // ffmpegs copy-on-write mechanism:
    int make_writable();

  protected:
    AVFrame * frame_;
  };


  //----------------------------------------------------------------
  // AvFrmSpecs
  //
  struct YAE_API AvFrmSpecs
  {
    AvFrmSpecs();
    AvFrmSpecs(const AVFrame & src);
    AvFrmSpecs(const AvFrm & src);
    AvFrmSpecs(const VideoTraits & vtts);

    void clear();
    void assign(const AVFrame & src);

    AvFrmSpecs & override_with(const AvFrmSpecs & specs);
    AvFrmSpecs & add_missing_specs(const AvFrmSpecs & specs);
    AvFrmSpecs & guess_missing_specs();

    inline AVPixelFormat get_pix_fmt() const
    { return format; }

    inline bool is_hdr() const
    {
      return (colorspace == AVCOL_SPC_BT2020_NCL ||
              colorspace == AVCOL_SPC_BT2020_CL);
    }

    inline bool is_sdr() const
    { return !is_hdr(); }

    int width;
    int height;
    AVPixelFormat format;
    AVColorSpace colorspace;
    AVColorRange color_range;
    AVColorPrimaries color_primaries;
    AVColorTransferCharacteristic color_trc;
    AVChromaLocation chroma_location;
    AVRational sample_aspect_ratio;
  };


  //----------------------------------------------------------------
  // pix_fmt
  //
  inline AVPixelFormat
  pix_fmt(const ::AVFrame & frame)
  {
    return (AVPixelFormat)(frame.format);
  }

  //----------------------------------------------------------------
  // pix_fmt
  //
  inline AVPixelFormat
  pix_fmt(const AvFrmSpecs & specs)
  {
    return specs.get_pix_fmt();
  }

  //----------------------------------------------------------------
  // sw_pix_fmt
  //
  YAE_API AVPixelFormat
  sw_pix_fmt(const ::AVFrame & frame);


  //----------------------------------------------------------------
  // is_less_than
  //
  template <typename TFrame>
  static inline bool is_less_than(const TFrame & f, int w, int h)
  {
    return (f.width * f.height) < (w * h);
  }

  //----------------------------------------------------------------
  // is_leq
  //
  template <typename TFrame>
  static inline bool is_leq(const TFrame & f, int w, int h)
  {
    return (f.width * f.height) <= (w * h);
  }

  //----------------------------------------------------------------
  // has_color_specs
  //
  template <typename TFrame>
  inline bool
  has_color_specs(const TFrame & src)
  {
    return (src.colorspace != AVCOL_SPC_UNSPECIFIED &&
            src.colorspace != AVCOL_SPC_RESERVED &&
            src.color_primaries != AVCOL_PRI_UNSPECIFIED &&
            src.color_primaries != AVCOL_PRI_RESERVED0 &&
            src.color_primaries != AVCOL_PRI_RESERVED &&
            src.color_trc != AVCOL_TRC_UNSPECIFIED &&
            src.color_trc != AVCOL_TRC_RESERVED0 &&
            src.color_trc != AVCOL_TRC_RESERVED);
  }

  //----------------------------------------------------------------
  // clear_specs
  //
  template <typename Specs>
  void
  clear_specs(Specs & specs)
  {
    specs.width = 0;
    specs.height = 0;
    specs.format = AV_PIX_FMT_NONE;
    specs.colorspace = AVCOL_SPC_UNSPECIFIED;
    specs.color_range = AVCOL_RANGE_UNSPECIFIED;
    specs.color_primaries = AVCOL_PRI_UNSPECIFIED;
    specs.color_trc = AVCOL_TRC_UNSPECIFIED;
    specs.chroma_location = AVCHROMA_LOC_UNSPECIFIED;
    specs.sample_aspect_ratio.num = 0;
    specs.sample_aspect_ratio.den = 1;
  }

  //----------------------------------------------------------------
  // override_specs
  //
  template <typename ASpecs, typename BSpecs>
  void
  override_specs(ASpecs & a, const BSpecs & b)
  {
    if (b.width > 0)
    {
      a.width = b.width;
    }

    if (b.height > 0)
    {
      a.height = b.height;
    }

    if (b.format != AV_PIX_FMT_NONE)
    {
      a.format = b.format;
    }

    if (b.colorspace != AVCOL_SPC_UNSPECIFIED &&
        b.colorspace != AVCOL_SPC_RESERVED)
    {
      a.colorspace = b.colorspace;
    }

    if (b.color_range != AVCOL_RANGE_UNSPECIFIED)
    {
      a.color_range = b.color_range;
    }

    if (b.color_primaries != AVCOL_PRI_UNSPECIFIED &&
        b.color_primaries != AVCOL_PRI_RESERVED0 &&
        b.color_primaries != AVCOL_PRI_RESERVED)
    {
      a.color_primaries = b.color_primaries;
    }

    if (b.color_trc != AVCOL_TRC_UNSPECIFIED &&
        b.color_trc != AVCOL_TRC_RESERVED0 &&
        b.color_trc != AVCOL_TRC_RESERVED)
    {
      a.color_trc = b.color_trc;
    }

    if (b.chroma_location != AVCHROMA_LOC_UNSPECIFIED)
    {
      a.chroma_location = b.chroma_location;
    }

    if (b.sample_aspect_ratio.num != 0)
    {
      a.sample_aspect_ratio = b.sample_aspect_ratio;
    }
  }

  //----------------------------------------------------------------
  // add_missing_specs
  //
  template <typename ASpecs, typename BSpecs>
  void
  add_missing_specs(ASpecs & a, const BSpecs & b)
  {
    if (a.width <= 0)
    {
      a.width = b.width;
    }

    if (a.height <= 0)
    {
      a.height = b.height;
    }

    if (a.format == AV_PIX_FMT_NONE)
    {
      a.format = b.format;
    }

    if (a.colorspace == AVCOL_SPC_UNSPECIFIED ||
        a.colorspace == AVCOL_SPC_RESERVED)
    {
      a.colorspace = b.colorspace;
    }

    if (a.color_range == AVCOL_RANGE_UNSPECIFIED)
    {
      a.color_range = b.color_range;
    }

    if (a.color_primaries == AVCOL_PRI_UNSPECIFIED ||
        a.color_primaries == AVCOL_PRI_RESERVED0 ||
        a.color_primaries == AVCOL_PRI_RESERVED)
    {
      a.color_primaries = b.color_primaries;
    }

    if (a.color_trc == AVCOL_TRC_UNSPECIFIED ||
        a.color_trc == AVCOL_TRC_RESERVED0 ||
        a.color_trc == AVCOL_TRC_RESERVED)
    {
      a.color_trc = b.color_trc;
    }

    if (a.chroma_location == AVCHROMA_LOC_UNSPECIFIED)
    {
      a.chroma_location = b.chroma_location;
    }

    if (a.sample_aspect_ratio.num == 0)
    {
      a.sample_aspect_ratio = b.sample_aspect_ratio;
    }
  }

  //----------------------------------------------------------------
  // same_color_space
  //
  template <typename ASpecs, typename BSpecs>
  bool
  same_color_space(const ASpecs & a, const BSpecs & b)
  {
    return (a.colorspace == b.colorspace &&
            a.color_primaries == b.color_primaries &&
            a.color_trc == b.color_trc);
  }

  //----------------------------------------------------------------
  // same_color_specs
  //
  template <typename ASpecs, typename BSpecs>
  bool
  same_color_specs(const ASpecs & a, const BSpecs & b)
  {
    return (a.color_range == b.color_range &&
            same_color_space(a, b));
  }

  //----------------------------------------------------------------
  // copy_color_specs
  //
  template <typename ASpecs, typename BSpecs>
  void
  copy_color_specs(ASpecs & a, const BSpecs & b)
  {
    a.colorspace = b.colorspace;
    a.color_range = b.color_range;
    a.color_primaries = b.color_primaries;
    a.color_trc = b.color_trc;
  }

  //----------------------------------------------------------------
  // same_specs
  //
  template <typename ASpecs, typename BSpecs>
  bool
  same_specs(const ASpecs & a, const BSpecs & b)
  {
    return (a.width == b.width &&
            a.height == b.height &&
            a.format == b.format &&
            same_color_specs(a, b));
  }

  //----------------------------------------------------------------
  // copy_specs
  //
  template <typename SrcSpecs>
  AvFrmSpecs
  copy_specs(const SrcSpecs & src)
  {
    return AvFrmSpecs(src);
  }

  //----------------------------------------------------------------
  // guess_specs
  //
  YAE_API AvFrmSpecs
  guess_specs(const AvFrmSpecs & src);

  //----------------------------------------------------------------
  // guess_specs
  //
  inline AvFrmSpecs
  guess_specs(const AvFrm & src)
  { return guess_specs(AvFrmSpecs(src)); }

  //----------------------------------------------------------------
  // change_specs
  //
  YAE_API AvFrmSpecs
  change_specs(const AvFrmSpecs & src_specs,
               AVPixelFormat dst_pix_fmt,
               int dst_width = -1,
               int dst_height = -1);

  //----------------------------------------------------------------
  // change_specs
  //
  inline AvFrmSpecs
  change_specs(const AvFrm & src_specs,
               AVPixelFormat dst_pix_fmt,
               int dst_width = -1,
               int dst_height = -1)
  {
    return change_specs(AvFrmSpecs(src_specs),
                        dst_pix_fmt,
                        dst_width,
                        dst_height);
  }


  //----------------------------------------------------------------
  // has
  //
  template <typename TData>
  bool
  has(const TData * values, TData v, int end = -1)
  {
    for (const TData * i = values; i && (int(*i) != end); ++i)
    {
      if (v == *i)
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // make_avfrm
  //
  YAE_API AvFrm
  make_avfrm(AVPixelFormat pix_fmt,
             int luma_w,
             int luma_h,
             AVColorSpace csp = AVCOL_SPC_BT709,
             AVColorPrimaries pri = AVCOL_PRI_BT709,
             AVColorTransferCharacteristic trc = AVCOL_TRC_BT709,
             AVColorRange rng = AVCOL_RANGE_MPEG,
             int par_num = 1,
             int par_den = 1,
             unsigned char fill_luma = 0x7f,
             unsigned char fill_chroma = 0x7f);

  //----------------------------------------------------------------
  // make_textured_frame
  //
  YAE_API AvFrm
  make_textured_frame(const TextureGenerator & tex_gen,
                      AVPixelFormat pix_fmt,
                      int luma_w,
                      int luma_h,
                      AVColorRange av_rng = AVCOL_RANGE_MPEG);

  //----------------------------------------------------------------
  // save_as
  //
  YAE_API bool
  save_as(const std::string & path,
          const yae::AvFrm & src,
          const yae::TTime & frame_dur = yae::TTime(1, 25));

  //----------------------------------------------------------------
  // save_as_png
  //
  YAE_API bool
  save_as_png(const yae::AvFrm & frm,
              const std::string & prefix,
              const yae::TTime & frame_dur = yae::TTime(1, 25));

  //----------------------------------------------------------------
  // save_as_jpg
  //
  YAE_API bool
  save_as_jpg(const yae::AvFrm & frm,
              const std::string & prefix,
              const yae::TTime & frame_dur = yae::TTime(1, 25));

  //----------------------------------------------------------------
  // save_as_tiff
  //
  YAE_API bool
  save_as_tiff(const yae::AvFrm & frm,
               const std::string & prefix,
               const yae::TTime & frame_dur = yae::TTime(1, 25));

  //----------------------------------------------------------------
  // make_hwframes_ctx
  //
  YAE_API yae::AvBufferRef
  make_hwframes_ctx(AVBufferRef * device_ctx_ref,
                    int width,
                    int height,
                    AVPixelFormat sw_format);

}


#endif // YAE_FFMPEG_UTILS_H_
