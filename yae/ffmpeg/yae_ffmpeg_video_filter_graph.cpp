// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <sstream>
#include <cstring>

// aeyae:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_ffmpeg_video_filter_graph.h"
#include "yae/utils/yae_benchmark.h"


namespace yae
{

  //----------------------------------------------------------------
  // color_scale_filter
  //
  static std::string
  color_scale_filter(const yae::AvFrmSpecs & src_specs,
                     const yae::AvFrmSpecs & dst_specs)
  {
    AVPixelFormat src_pix_fmt = src_specs.get_pix_fmt();
    AVPixelFormat dst_pix_fmt = dst_specs.get_pix_fmt();
    AVPixelFormat out_pix_fmt =
      (dst_pix_fmt == AV_PIX_FMT_NONE) ? src_pix_fmt : dst_pix_fmt;

    AVColorRange out_color_range =
      dst_specs.color_range == AVCOL_RANGE_UNSPECIFIED ?
      src_specs.color_range : dst_specs.color_range;

    AVColorSpace out_colorspace =
      dst_specs.colorspace == AVCOL_SPC_UNSPECIFIED ?
      src_specs.colorspace : dst_specs.colorspace;

    if (src_specs.format == out_pix_fmt &&
        src_specs.color_range == out_color_range &&
        src_specs.colorspace == out_colorspace)
    {
      // nothing to do:
      return std::string();
    }

    std::ostringstream oss;
    oss << "scale=";

    // colorspace:
    oss << "in_color_matrix=" << av_color_space_name(src_specs.colorspace)
        << ":out_color_matrix=" << av_color_space_name(out_colorspace);

    // color range:
    oss << ":in_range=" << av_color_range_name(src_specs.color_range)
        << ":out_range=" << av_color_range_name(out_color_range);

    return oss.str();
  }

  //----------------------------------------------------------------
  // get_zscale_range
  //
  static const char *
  to_zscale_range(AVColorRange color_range)
  {
    return (color_range == AVCOL_RANGE_JPEG ? "full" : "limited");
  }

  //----------------------------------------------------------------
  // to_zscale_primaries
  //
  static const char *
  to_zscale_primaries(AVColorPrimaries color_primaries)
  {
    return (color_primaries == AVCOL_PRI_BT709 ? "709" :
            color_primaries == AVCOL_PRI_SMPTE170M ? "170m" :
            color_primaries == AVCOL_PRI_SMPTE240M ? "240m" :
            color_primaries == AVCOL_PRI_BT2020 ? "2020" :
            "input");
  }

  //----------------------------------------------------------------
  // to_zscale_matrix
  //
  static const char *
  to_zscale_matrix(AVColorSpace colorspace)
  {
    return (colorspace == AVCOL_SPC_BT709 ? "709" :
            colorspace == AVCOL_SPC_BT470BG ? "470bg" :
            colorspace == AVCOL_SPC_SMPTE170M ? "170m" :
            colorspace == AVCOL_SPC_BT2020_NCL ? "2020_ncl" :
            colorspace == AVCOL_SPC_BT2020_CL ? "2020_cl" :
            "input");
  }

  //----------------------------------------------------------------
  // to_zscale_transfer
  //
  static const char *
  to_zscale_transfer(AVColorTransferCharacteristic color_trc)
  {
    return (color_trc == AVCOL_TRC_BT709 ? "709" :
            color_trc == AVCOL_TRC_SMPTE170M ? "601" :
            color_trc == AVCOL_TRC_LINEAR ? "linear" :
            color_trc == AVCOL_TRC_BT2020_10 ? "2020_10" :
            color_trc == AVCOL_TRC_BT2020_12 ? "2020_12" :
            color_trc == AVCOL_TRC_SMPTE2084 ? "smpte2084" :
            color_trc == AVCOL_TRC_IEC61966_2_1 ? "iec61966-2-1" :
            color_trc == AVCOL_TRC_ARIB_STD_B67 ? "arib-std-b67" :
            "input");
  }

  //----------------------------------------------------------------
  // to_zscale_location
  //
  static const char *
  to_zscale_location(AVChromaLocation chroma_loc)
  {
    return (chroma_loc == AVCHROMA_LOC_LEFT ? "left" :
            chroma_loc == AVCHROMA_LOC_CENTER ? "center" :
            chroma_loc == AVCHROMA_LOC_TOPLEFT ? "topleft" :
            chroma_loc == AVCHROMA_LOC_TOP ? "top" :
            chroma_loc == AVCHROMA_LOC_BOTTOMLEFT ? "bottomleft" :
            chroma_loc == AVCHROMA_LOC_BOTTOM ? "bottom" :
            "input");
  }

  //----------------------------------------------------------------
  // get_nominal_peak_luminance_cd_m2
  //
  static double
  get_nominal_peak_luminance_cd_m2(const AVColorTransferCharacteristic trc)
  {
    uint32_t cd_m2 =
      (trc == AVCOL_TRC_SMPTE2084) ? 10000.0 :
      (trc == AVCOL_TRC_BT2020_10 ||
       trc == AVCOL_TRC_BT2020_12 ||
       trc == AVCOL_TRC_ARIB_STD_B67) ? 1000.0 :
      100.0;
    return cd_m2;
  }

  //----------------------------------------------------------------
  // zscale_filter_chain
  //
  static std::string
  zscale_filter_chain(const yae::AvFrmSpecs & src_specs,
                      const yae::AvFrmSpecs & dst_specs)
  {
    AVPixelFormat src_pix_fmt = src_specs.get_pix_fmt();
    AVPixelFormat dst_pix_fmt = dst_specs.get_pix_fmt();

    yae::AvFrmSpecs out_specs = dst_specs;
    out_specs.format =
      (dst_pix_fmt == AV_PIX_FMT_NONE) ? src_pix_fmt : dst_pix_fmt;

    out_specs.color_range =
      dst_specs.color_range == AVCOL_RANGE_UNSPECIFIED ?
      src_specs.color_range : dst_specs.color_range;

    out_specs.colorspace =
      dst_specs.colorspace == AVCOL_SPC_UNSPECIFIED ?
      src_specs.colorspace : dst_specs.colorspace;

    out_specs.color_primaries =
      dst_specs.color_primaries == AVCOL_PRI_UNSPECIFIED ?
      src_specs.color_primaries : dst_specs.color_primaries;

    out_specs.color_trc =
      dst_specs.color_trc == AVCOL_TRC_UNSPECIFIED ?
      src_specs.color_trc : dst_specs.color_trc;

    if (src_specs.format == out_specs.format &&
        src_specs.color_range == out_specs.color_range &&
        src_specs.colorspace == out_specs.colorspace &&
        src_specs.color_primaries == out_specs.color_primaries &&
        src_specs.color_trc == out_specs.color_trc)
    {
      // nothing to do:
      return std::string();
    }

    double dst_cd_m2 = get_nominal_peak_luminance_cd_m2(out_specs.color_trc);

    // https://movielabs.com/ngvideo/MovieLabs_Mapping_BT.709_to_HDR10_v1.0.pdf
    if (src_specs.is_sdr() && out_specs.is_hdr())
    {
      dst_cd_m2 = 203.0;
    }

    std::ostringstream oss;

    // yuv -> linear rgb
    oss << "zscale=";

    // input colorspace:
    oss << "min=" << to_zscale_matrix(src_specs.colorspace);

    // input color range:
    if (src_specs.color_range != AVCOL_RANGE_UNSPECIFIED)
    {
      oss << ":rin=" << to_zscale_range(src_specs.color_range);
    }

    // input color primaries:
    if (src_specs.color_primaries != AVCOL_PRI_UNSPECIFIED)
    {
      oss << ":pin=" << to_zscale_primaries(src_specs.color_primaries);
    }

    // input transfer characteristic:
    if (src_specs.color_trc != AVCOL_TRC_UNSPECIFIED)
    {
      oss << ":tin=" << to_zscale_transfer(src_specs.color_trc);
    }

    // input chroma location:
    if (src_specs.chroma_location != AVCHROMA_LOC_UNSPECIFIED)
    {
      oss << ":cin=" << to_zscale_location(src_specs.chroma_location);
    }

    // output float rgb with linear color transfer characteristics
    oss << ":t=linear,format=gbrpf32le";

    if (src_specs.is_hdr() && out_specs.is_sdr())
    {
      oss << ",tonemap=gamma";
    }

    // linear rgb -> yuv
    oss << ",zscale=tin=linear";

    if (src_specs.is_hdr() != out_specs.is_hdr())
    {
      oss << ":npl=" << dst_cd_m2;
    }

    // output color primaries:
    oss << ":p=" << to_zscale_primaries(out_specs.color_primaries);

    // output colorspace:
    oss << ":m=" << to_zscale_matrix(out_specs.colorspace);

    // output color range:
    oss << ":r=" << to_zscale_range(out_specs.color_range);

    // output transfer characteristic:
    oss << ":t=" << to_zscale_transfer(out_specs.color_trc);

    // dithering:
    if (src_pix_fmt != out_specs.format)
    {
      const AVPixFmtDescriptor * src_desc =
        av_pix_fmt_desc_get(src_pix_fmt);
      const AVPixFmtDescriptor * out_desc =
        av_pix_fmt_desc_get(out_specs.format);

      if (out_desc->comp[0].depth < src_desc->comp[0].depth)
      {
        // avoid introducing banding artifacts when reducing bitdepth:
        oss << ":dither=ordered";
      }
    }

    return oss.str();
  }


  //----------------------------------------------------------------
  // scale_filter_chain
  //
  std::string
  scale_filter_chain(int src_width,
                     int src_height,
                     int dst_width,
                     int dst_height,
                     const char * sws_flags)
  {
    std::ostringstream oss;

    if (dst_width < 0)
    {
      dst_width = src_width;
    }

    if (dst_height < 0)
    {
      dst_height = src_height;
    }

    if (src_width != dst_width ||
        src_height != dst_height)
    {
      int dw = src_width - dst_width;
      int dh = src_height - dst_height;

      if (dh == 1 && !dw)
      {
        oss << "crop=h=" << dst_height;
      }
      else if (dw == 1 && !dh)
      {
        oss << "crop=w=" << dst_width;
      }
      else
      {
        oss << "scale=" << dst_width << ':' << dst_height;

        if (sws_flags)
        {
          oss << ":flags=" << sws_flags;
        }
      }

      oss << ",setsar=1";
    }

    return oss.str();
  }

  //----------------------------------------------------------------
  // deint_filter_chain
  //
  std::string
  deint_filter_chain(int src_width,
                     int src_height,
                     int dst_width,
                     int dst_height,
                     yae::DeinterlaceOpts deinterlace,
                     yae::DeinterlaeFieldOrder field_order,
                     const char * sws_flags)
  {
    std::ostringstream oss;

    if (dst_width < 0)
    {
      dst_width = src_width;
    }

    if (dst_height < 0)
    {
      dst_height = src_height;
    }

    const char * mode =
      deinterlace == DEINT_SEND_FRAME ? "send_frame" :
      deinterlace == DEINT_SEND_FIELD ? "send_field" :
      deinterlace == DEINT_SEND_FRAME_NOSPATIAL ? "send_frame_nospatial" :
      deinterlace == DEINT_SEND_FIELD_NOSPATIAL ? "send_field_nospatial" :
      NULL;

    if (!mode)
    {
      YAE_ASSERT(deinterlace == DEINT_DISABLED);
      return scale_filter_chain(src_width,
                                src_height,
                                dst_width,
                                dst_height,
                                sws_flags);
    }

    int dw = src_width - dst_width;
    int dh = src_height - dst_height;

    if (dst_width < src_width)
    {
      // scale horizontally first, to reduce work for yadif:
      if (dw == 1 && !dh)
      {
        oss << "crop=w=" << dst_width;
      }
      else
      {
        oss << "scale=" << dst_width << ':' << src_height;

        if (sws_flags)
        {
          oss << ":flags=" << sws_flags;
        }
      }

      oss << ",";
    }

    const char * parity =
      field_order == DEINT_FIELD_ORDER_TFF ? "tff" :
      field_order == DEINT_FIELD_ORDER_BFF ? "bff" :
      "auto";

    oss << "yadif=mode=" << mode
        << ":parity=" << parity
        << ":deint=interlaced";

    // do the rest of scaling, if any:
    if (src_height != dst_height || src_width < dst_width)
    {
      if (dh == 1 && !dw)
      {
        oss << "crop=h=" << dst_height;
      }
      else if (dw == 1 && !dh)
      {
        oss << "crop=w=" << dst_width;
      }
      else
      {
        oss << ",scale=" << dst_width << ':' << dst_height;

        if (sws_flags)
        {
          oss << ":flags=" << sws_flags;
        }

        oss << ",setsar=1";
      }
    }

    return oss.str();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::VideoFilterGraph
  //
  VideoFilterGraph::VideoFilterGraph():
    nb_threads_(0),
    flushing_(false),
    src_(NULL),
    sink_(NULL),
    in_(NULL),
    out_(NULL),
    graph_(NULL)
  {
    reset();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::~VideoFilterGraph
  //
  VideoFilterGraph::~VideoFilterGraph()
  {
    reset();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::reset
  //
  void
  VideoFilterGraph::reset()
  {
    avfilter_graph_free(&graph_);
    avfilter_inout_free(&in_);
    avfilter_inout_free(&out_);

    src_framerate_.num = 0;
    src_framerate_.den = 1;

    src_timebase_.num = 0;
    src_timebase_.den = 1;

    src_specs_.clear();
    dst_specs_.clear();
    hw_frames_.reset();

    flushing_ = false;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::same_traits
  //
  bool
  VideoFilterGraph::same_traits(const AVFrame & src,
                                const AvFrmSpecs & dst_specs,
                                const char * filter_chain) const
  {
    filter_chain = filter_chain ? filter_chain : "";

    bool same = (hw_frames_.same_as(src.hw_frames_ctx) &&
                 yae::same_specs(src_specs_, AvFrmSpecs(src)) &&
                 yae::same_specs(dst_specs_, dst_specs) &&
                 filter_chain_ == filter_chain);

    return same;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::setup_input_filters
  //
  AVPixelFormat
  VideoFilterGraph::setup_input_filters(std::ostringstream & oss,
                                        const AVFrame & src) const
  {
    // shortcut:
    AVPixelFormat src_pix_fmt = yae::pix_fmt(src);
    AVPixelFormat dst_pix_fmt = yae::pix_fmt(dst_specs_);

    const char * src_pix_fmt_txt = av_get_pix_fmt_name(src_pix_fmt);

    oss << "buffer"
        << "=width=" << src.width
        << ":height=" << src.height
        << ":pix_fmt=" << src_pix_fmt_txt
        << ":frame_rate=" << src_framerate_.num << '/' << src_framerate_.den
        << ":time_base=" << src_timebase_.num << '/' << src_timebase_.den;

    if (src.sample_aspect_ratio.num)
    {
      oss << ":sar="
          << src.sample_aspect_ratio.num << '/'
          << src.sample_aspect_ratio.den;
    }

    const AVPixFmtDescriptor * dst_desc = av_pix_fmt_desc_get(dst_pix_fmt);
    if (src.hw_frames_ctx && (dst_desc->flags & AV_PIX_FMT_FLAG_HWACCEL) == 0)
    {
      oss << ",hwdownload";

      AVPixelFormat * formats = NULL;
      av_hwframe_transfer_get_formats(src.hw_frames_ctx,
                                      AV_HWFRAME_TRANSFER_DIRECTION_FROM,
                                      &formats,
                                      0);
      YAE_ASSERT(formats);

      if (yae::has(formats, dst_pix_fmt, AV_PIX_FMT_NONE))
      {
        src_pix_fmt = dst_pix_fmt;
      }
      else if (formats)
      {
        src_pix_fmt = formats[0];
      }
      av_freep(&formats);

      const char * src_pix_fmt_txt = av_get_pix_fmt_name(src_pix_fmt);
      oss << ",format" << "=pix_fmts=" << src_pix_fmt_txt;
    }

    return src_pix_fmt;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::setup
  //
  bool
  VideoFilterGraph::setup(const AVFrame & src,
                          const AVRational & src_framerate,
                          const AVRational & src_timebase,
                          const AvFrmSpecs & dst_specs,
                          const char * filter_chain,
                          unsigned int setup_hints)
  {
    // YAE_BENCHMARK(benchmark, "VideoFilterGraph::setup");

    filter_chain = filter_chain ? filter_chain : "";

    bool same = same_traits(src, dst_specs, filter_chain);
    if (same && !flushing_)
    {
      return false;
    }

    reset();

    src_timebase_ = src_timebase;
    src_framerate_ = src_framerate;
    src_specs_ = yae::copy_specs(src);
    dst_specs_ = yae::copy_specs(dst_specs);

    if (src.hw_frames_ctx)
    {
      hw_frames_.reset(av_buffer_ref(src.hw_frames_ctx));
    }

    // shortcut:
    AVPixelFormat src_pix_fmt = yae::pix_fmt(src);
    AVPixelFormat dst_pix_fmt = yae::pix_fmt(dst_specs_);

    // insert any additional filters:
    {
      std::ostringstream oss;

      src_pix_fmt = setup_input_filters(oss, src);
      if (filter_chain && *filter_chain && strcmp(filter_chain, "null") != 0)
      {
        oss << ',' << filter_chain;
      }

      if ((setup_hints & SCALE_TO_SPECS) == SCALE_TO_SPECS)
      {
        std::string scale = scale_filter_chain(src.width,
                                               src.height,
                                               dst_specs_.width,
                                               dst_specs_.height);
        if (!scale.empty())
        {
          oss << "," << scale;
        }
      }

#if 0
      // if the source (A)RGB pixel format bitdepth is less than 8
      // then convert it (A)RGB:
      const AVPixFmtDescriptor * desc = av_pix_fmt_desc_get(dst_pix_fmt);
      if (desc &&
          desc->nb_components > 0 &&
          desc->comp[0].depth < 8 &&
          (desc->flags & AV_PIX_FMT_FLAG_RGB) == AV_PIX_FMT_FLAG_RGB)
      {
#if AV_HAVE_BIGENDIAN
        static const AVPixelFormat native_rgb = AV_PIX_FMT_RGB24;
        static const AVPixelFormat native_argb = AV_PIX_FMT_ARGB;
#else
        static const AVPixelFormat native_rgb = AV_PIX_FMT_BGR24;
        static const AVPixelFormat native_argb = AV_PIX_FMT_BGRA;
#endif
        const bool flag_alpha =
          (desc->flags & AV_PIX_FMT_FLAG_ALPHA) == AV_PIX_FMT_FLAG_ALPHA;

        dst_pix_fmt = flag_alpha ? native_argb : native_rgb;
      }
#endif

      const bool same_color_space = yae::same_color_space(src, dst_specs_);
      const bool same_color_specs = yae::same_color_specs(src, dst_specs_);

      if (!same_color_specs)
      {
        static const AVFilter * has_zscale = avfilter_get_by_name("zscale");

        std::string color_xform =
          (has_zscale && !same_color_space) ?
          zscale_filter_chain(src, dst_specs_) :
          color_scale_filter(src, dst_specs_);

        if (!color_xform.empty())
        {
          oss << "," << color_xform;
        }
      }

      // force the output pixel format, even if it's the same as source,
      // because it's possible the format was changed by an upstream filter:
      const char * dst_pix_fmt_txt = av_get_pix_fmt_name(dst_pix_fmt);
      oss << ",format=pix_fmts=" << dst_pix_fmt_txt
          << ",buffersink";

      filter_chain_ = filter_chain;
      filters_ = oss.str().c_str();
    }

    graph_ = avfilter_graph_alloc();

    if (nb_threads_ > 0)
    {
      graph_->nb_threads = nb_threads_;
    }

    int err = avfilter_graph_parse2(graph_, filters_.c_str(), &in_, &out_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    if (hw_frames_.ref_)
    {
      AVHWFramesContext * hw_frames_ctx = hw_frames_.get<AVHWFramesContext>();
      AVBufferRef * device_ref = hw_frames_ctx->device_ref;

      for (int i = 0; i < int(graph_->nb_filters); i++)
      {
        AVFilterContext * filter_ctx = graph_->filters[i];
        YAE_ASSERT(!filter_ctx->hw_device_ctx);
        filter_ctx->hw_device_ctx = av_buffer_ref(device_ref);
      }
    }

    src_ = lookup_src(graph_->nb_filters ? graph_->filters[0] : NULL,
                      "buffer");
    YAE_ASSERT(src_);

    AVBufferSrcParameters * par = av_buffersrc_parameters_alloc();
    par->width = src.width;
    par->height = src.height;
    par->format = src.format;
    par->frame_rate = src_framerate_;
    par->time_base = src_timebase_;

    if (src.sample_aspect_ratio.num > 0 &&
        src.sample_aspect_ratio.den > 0)
    {
      par->sample_aspect_ratio = src.sample_aspect_ratio;
    }

    par->hw_frames_ctx = hw_frames_.ref_;

    err = av_buffersrc_parameters_set(src_, par);
    av_freep(&par);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = avfilter_graph_config(graph_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    sink_ = lookup_sink(src_, "buffersink");
    YAE_ASSERT_OR_RETURN(src_ && sink_, false);

    return true;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::push
  //
  void
  VideoFilterGraph::push(AVFrame * frame)
  {
    // YAE_BENCHMARK(benchmark, "VideoFilterGraph::push");

    if (!frame)
    {
      if (flushing_)
      {
        // already flushing, no need to do it twice:
        return;
      }

      // flushing, clear some cached properties
      // so the chain gets rebuilt next time:
      flushing_ = true;
    }
    else
    {
      YAE_ASSERT(!flushing_);
    }

    int err = av_buffersrc_add_frame(src_, frame);
    YAE_ASSERT(err >= 0);
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::pull
  //
  bool
  VideoFilterGraph::pull(AVFrame * frame, AVRational & out_timebase)
  {
    // YAE_BENCHMARK(benchmark, "VideoFilterGraph::pull");

    // avoid leaking unintentionally:
    av_frame_unref(frame);

    int err = av_buffersink_get_frame(sink_, frame);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
    {
      return false;
    }

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    // some filters (yadif) may change the timebase:
    out_timebase = sink_->inputs[0]->time_base;

    return true;
  }

}
