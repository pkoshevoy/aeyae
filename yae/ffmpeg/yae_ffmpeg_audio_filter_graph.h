// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_
#define YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/video/yae_video.h"

// standard:
#include <string>

// ffmpeg:
extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
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

    // return true if the graph setup was updated,
    // return false if the graph has not changed or failed:
    //
    bool setup(// input format:
               const AVRational & srcTimeBase,
               enum AVSampleFormat srcSampleFmt,
               int srcSampleRate,
               const AVChannelLayout & srcChannelLayout,

               // output format:
               enum AVSampleFormat dstSampleFmt,
               int dstSampleRate,
               const AVChannelLayout & dstChannelLayout,

               const char * filterChain = NULL,
               bool * frameTraitsChanged = NULL);

    bool push(AVFrame * in);
    bool pull(AVFrame * out);

    inline bool is_valid() const
    { return graph_ && src_ && sink_; }

    inline const std::string & get_filters() const
    { return filters_; }

  protected:
    std::string filterChain_;
    std::string filters_;

    AVRational srcTimeBase_;

    enum AVSampleFormat srcSampleFmt_;
    enum AVSampleFormat dstSampleFmt_;

    int srcSampleRate_;
    int dstSampleRate_;

    yae::ChannelLayout srcChannelLayout_;
    yae::ChannelLayout dstChannelLayout_;

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };
}


#endif // YAE_FFMPEG_AUDIO_FILTER_GRAPH_H_
