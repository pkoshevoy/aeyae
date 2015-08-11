// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FFMPEG_VIDEO_FILTER_GRAPH_H_
#define YAE_FFMPEG_VIDEO_FILTER_GRAPH_H_

// aeyae:
#include "../api/yae_api.h"
#include "../video/yae_video.h"

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
  // VideoFilterGraph
  //
  struct YAE_API VideoFilterGraph
  {
    VideoFilterGraph();
    ~VideoFilterGraph();

    void reset();

    bool setup(int srcWidth,
               int srcHeight,
               const AVRational & srcTimeBase,
               const AVRational & srcPAR,
               AVPixelFormat srcPixFmt,
               AVPixelFormat dstPixFmt,
               const char * filterChain = NULL,
               bool * frameTraitsChanged = NULL);

    bool push(AVFrame * in);
    bool pull(AVFrame * out);

  protected:
    std::string filterChain_;
    int srcWidth_;
    int srcHeight_;
    AVRational srcTimeBase_;
    AVRational srcPAR_;
    AVPixelFormat srcPixFmt_;
    AVPixelFormat dstPixFmt_[2];

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };
}


#endif // YAE_FFMPEG_VIDEO_FILTER_GRAPH_H_
