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
    YAE_BENCHMARK(benchmark, "VideoFilterGraph::setup");

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

      if (src_pix_fmt != dst_pix_fmt)
      {
        const char * dst_pix_fmt_txt = av_get_pix_fmt_name(dst_pix_fmt);
        oss << ",format=pix_fmts=" << dst_pix_fmt_txt;
      }

      oss << ",buffersink";

      filter_chain_ = filter_chain;
      filters_ = oss.str().c_str();
    }

    return setup_filter_links();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::setup_filter_links
  //
  bool
  VideoFilterGraph::setup_filter_links()
  {
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

      for (int i = 0; i < graph_->nb_filters; i++)
      {
        AVFilterContext * filter_ctx = graph_->filters[i];
        YAE_ASSERT(!filter_ctx->hw_device_ctx);
        filter_ctx->hw_device_ctx = av_buffer_ref(device_ref);

        bool hwdownload = strcmp(filter_ctx->filter->name, "hwdownload") == 0;
        if (hwdownload)
        {
          break;
        }

        for (int j = 0; j < filter_ctx->nb_outputs; j++)
        {
          AVFilterLink * link = filter_ctx->outputs[j];
          YAE_ASSERT(!link->hw_frames_ctx);
          link->hw_frames_ctx = av_buffer_ref(hw_frames_.ref_);
        }
      }
    }

    err = avfilter_graph_config(graph_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    src_ = lookup_src(graph_->nb_filters ? graph_->filters[0] : NULL,
                      "buffer");
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
    YAE_BENCHMARK(benchmark, "VideoFilterGraph::push");

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
    YAE_BENCHMARK(benchmark, "VideoFilterGraph::pull");

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
