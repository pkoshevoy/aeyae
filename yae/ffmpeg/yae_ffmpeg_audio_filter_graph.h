// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_
#define YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_

// aeyae:
#include "../api/yae_api.h"

// standard C++ library:
#include <string>

// ffmpeg includes:
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // AudioFilterGraph
  //
  struct YAE_API AudioFilterGraph
  {
    AudioFilterGraph();
    ~AudioFilterGraph();

    void reset();

    bool setup(// input format:
               const AVRational & srcTimeBase,
               enum AVSampleFormat srcSampleFmt,
               int srcSampleRate,
               int64 srcChannelLayout,

               // output format:
               enum AVSampleFormat dstSampleFmt,
               int dstSampleRate,
               int64 dstChannelLayout,

               const char * filterChain = NULL,
               bool * frameTraitsChanged = NULL);

    bool push(AVFrame * in);
    bool pull(AVFrame * out);

  protected:
    std::string filterChain_;

    AVRational srcTimeBase_;

    enum AVSampleFormat srcSampleFmt_;
    enum AVSampleFormat dstSampleFmt_[2];

    int srcSampleRate_;
    int dstSampleRate_[2];

    int64 srcChannelLayout_;
    int64 dstChannelLayout_[2];

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };
}


#endif // YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_
