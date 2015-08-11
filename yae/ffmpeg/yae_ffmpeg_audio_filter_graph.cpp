// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <sstream>

// aeyae:
#include "yae_ffmpeg_audio_filter_graph.h"
#include "yae_ffmpeg_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // AudioFilterGraph::AudioFilterGraph
  //
  AudioFilterGraph::AudioFilterGraph():
    src_(NULL),
    sink_(NULL),
    in_(NULL),
    out_(NULL),
    graph_(NULL)
  {
    reset();
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::~AudioFilterGraph
  //
  AudioFilterGraph::~AudioFilterGraph()
  {
    reset();
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::reset
  //
  void
  AudioFilterGraph::reset()
  {
    avfilter_graph_free(&graph_);
    avfilter_inout_free(&in_);
    avfilter_inout_free(&out_);

    srcTimeBase_.num = 0;
    srcTimeBase_.den = 1;

    srcSampleFmt_    = AV_SAMPLE_FMT_NONE;
    dstSampleFmt_[0] = AV_SAMPLE_FMT_NONE;
    dstSampleFmt_[1] = AV_SAMPLE_FMT_NONE;

    srcSampleRate_    = -1;
    dstSampleRate_[0] = -1;
    dstSampleRate_[1] = -1;

    srcChannelLayout_    = -1;
    dstChannelLayout_[0] = -1;
    dstChannelLayout_[1] = -1;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::setup
  //
  bool
  AudioFilterGraph::setup(// input format:
                          const AVRational & srcTimeBase,
                          enum AVSampleFormat srcSampleFmt,
                          int srcSampleRate,
                          int64 srcChannelLayout,

                          // output format:
                          enum AVSampleFormat dstSampleFmt,
                          int dstSampleRate,
                          int64 dstChannelLayout,

                          const char * filterChain,
                          bool * frameTraitsChanged)
  {
    filterChain = filterChain ? filterChain : "";
    bool sameTraits = (srcSampleRate_ == srcSampleRate &&
                       srcChannelLayout_ == srcChannelLayout &&
                       srcSampleFmt_ == srcSampleFmt &&
                       dstSampleRate_[0] == dstSampleRate &&
                       dstSampleFmt_[0] == dstSampleFmt &&
                       dstChannelLayout_[0] == dstChannelLayout &&
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

    srcTimeBase_ = srcTimeBase;

    srcSampleFmt_    = srcSampleFmt;
    dstSampleFmt_[0] = dstSampleFmt;

    srcSampleRate_    = srcSampleRate;
    dstSampleRate_[0] = dstSampleRate;

    srcChannelLayout_    = srcChannelLayout;
    dstChannelLayout_[0] = dstChannelLayout;

    std::string filters;
    {
      std::ostringstream os;

      const char * srcSampleFmtTxt = av_get_sample_fmt_name(srcSampleFmt_);
      os << "abuffer"
         << "=time_base=" << srcTimeBase_.num << '/' << srcTimeBase_.den
         << ":sample_rate=" << srcSampleRate_
         << ":sample_fmt=" << srcSampleFmtTxt
         << ":channel_layout=0x" << std::hex << srcChannelLayout << std::dec;

      if (filterChain && *filterChain && strcmp(filterChain, "anull") != 0)
      {
        os << ',' << filterChain;
      }

      const char * dstSampleFmtTxt = av_get_sample_fmt_name(dstSampleFmt);
      os << ",aformat"
         << "=sample_fmts=" << dstSampleFmtTxt
         << ":sample_rates=" << dstSampleRate
         << ":channel_layouts=0x" << std::hex << dstChannelLayout << std::dec
         << ",abuffersink";

      filters = os.str().c_str();
      filterChain_ = filterChain;
    }

    graph_ = avfilter_graph_alloc();
    int err = avfilter_graph_parse2(graph_, filters.c_str(), &in_, &out_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = avfilter_graph_config(graph_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    src_ = lookup_src(graph_->nb_filters ? graph_->filters[0] : NULL,
                      "abuffer");
    sink_ = lookup_sink(src_, "abuffersink");
    YAE_ASSERT_OR_RETURN(src_ && sink_, false);

    return true;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::push
  //
  bool
  AudioFilterGraph::push(AVFrame * frame)
  {
    int err = av_buffersrc_add_frame(src_, frame);

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::pull
  //
  bool
  AudioFilterGraph::pull(AVFrame * frame)
  {
    int err = av_buffersink_get_frame(sink_, frame);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
    {
      return false;
    }

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }

}
