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
#include "yae_ffmpeg_utils.h"
#include "yae_ffmpeg_video_filter_graph.h"


namespace yae
{

  //----------------------------------------------------------------
  // VideoFilterGraph::VideoFilterGraph
  //
  VideoFilterGraph::VideoFilterGraph():
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

    srcTimeBase_.num = 0;
    srcTimeBase_.den = 1;

    srcPAR_.num = 0;
    srcPAR_.den = 1;

    srcWidth_  = 0;
    srcHeight_ = 0;

    srcPixFmt_    = AV_PIX_FMT_NONE;
    dstPixFmt_[0] = AV_PIX_FMT_NONE;
    dstPixFmt_[1] = AV_PIX_FMT_NONE;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::setup
  //
  bool
  VideoFilterGraph::setup(int srcWidth,
                          int srcHeight,
                          const AVRational & srcFrameRate,
                          const AVRational & srcTimeBase,
                          const AVRational & srcPAR,
                          AVPixelFormat srcPixFmt,
                          AVPixelFormat dstPixFmt,
                          const char * filterChain,
                          bool * frameTraitsChanged)
  {
    filterChain = filterChain ? filterChain : "";
    bool sameTraits = (srcWidth_ == srcWidth &&
                       srcHeight_ == srcHeight &&
                       srcPixFmt_ == srcPixFmt &&
                       dstPixFmt_[0] == dstPixFmt &&
                       filterChain_ == filterChain);

    if (frameTraitsChanged)
    {
      *frameTraitsChanged = !sameTraits;
    }

    if (sameTraits)
    {
      return true;
    }

    reset();

    srcWidth_ = srcWidth;
    srcHeight_ = srcHeight;
    srcPixFmt_ = srcPixFmt;
    dstPixFmt_[0] = dstPixFmt;

    srcFrameRate_ = srcFrameRate;
    srcTimeBase_ = srcTimeBase;
    srcPAR_ = srcPAR;

    std::string filters;
    {
      std::ostringstream os;

      const char * srcPixFmtTxt = av_get_pix_fmt_name(srcPixFmt_);
      os << "buffer"
         << "=width=" << srcWidth_
         << ":height=" << srcHeight_
         << ":pix_fmt=" << srcPixFmtTxt
         << ":frame_rate=" << srcFrameRate_.num << '/' << srcFrameRate_.den
         << ":time_base=" << srcTimeBase_.num << '/' << srcTimeBase_.den
         << ":sar=" << srcPAR_.num << '/' << srcPAR_.den;

      if (filterChain && *filterChain && std::strcmp(filterChain, "null") != 0)
      {
        os << ',' << filterChain;
      }

      const char * dstPixFmtTxt = av_get_pix_fmt_name(dstPixFmt_[0]);
      os << ",format"
         << "=pix_fmts=" << dstPixFmtTxt
         << ",buffersink";

      filters = os.str().c_str();
      filterChain_ = filterChain;
    }

    graph_ = avfilter_graph_alloc();
    int err = avfilter_graph_parse2(graph_, filters.c_str(), &in_, &out_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

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
  bool
  VideoFilterGraph::push(AVFrame * frame)
  {
    int err = av_buffersrc_add_frame(src_, frame);

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::pull
  //
  bool
  VideoFilterGraph::pull(AVFrame * frame, AVRational & outTimeBase)
  {
    int err = av_buffersink_get_frame(sink_, frame);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
    {
      return false;
    }

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    // some filters (yadif) may change the timebase:
    outTimeBase = sink_->inputs[0]->time_base;

    return true;
  }

}
