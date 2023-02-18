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
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // scale_filter_chain
  //
  YAE_API std::string
  scale_filter_chain(int src_width,
                     int src_height,
                     int dst_width,
                     int dst_height,
                     const char * sws_flags = NULL);


  //----------------------------------------------------------------
  // DeinterlaceOpts
  //
  enum DeinterlaceOpts
  {
    DEINT_DISABLED = -1,
    DEINT_SEND_FRAME = 0,
    DEINT_SEND_FIELD = 1,
    DEINT_SEND_FRAME_NOSPATIAL = 2,
    DEINT_SEND_FIELD_NOSPATIAL = 3,
  };


  //----------------------------------------------------------------
  // DeinterlaeFieldOrder
  //
  enum DeinterlaeFieldOrder
  {
    DEINT_FIELD_ORDER_AUTO = -1,
    DEINT_FIELD_ORDER_TFF = 0,
    DEINT_FIELD_ORDER_BFF = 1,
  };


  //----------------------------------------------------------------
  // deint_filter_chain
  //
  YAE_API std::string
  deint_filter_chain(int src_width,
                     int src_height,
                     int dst_width,
                     int dst_height,
                     yae::DeinterlaceOpts deinterlace = DEINT_SEND_FRAME,
                     yae::DeinterlaeFieldOrder parity = DEINT_FIELD_ORDER_AUTO,
                     const char * sws_flags = NULL);


  //----------------------------------------------------------------
  // VideoFilterGraph
  //
  struct YAE_API VideoFilterGraph
  {
    VideoFilterGraph();
    ~VideoFilterGraph();

    // reset to initial state, release allocated resources:
    void reset();

    inline bool initialized() const
    { return src_ && sink_; }

    bool same_traits(const AVFrame & src,
                     const yae::AvFrmSpecs & dstSpecs,
                     const char * filterChain = NULL) const;

  protected:
    // set up buffer filter and add hwdownload if necessary,
    // return output pixel format for the input portion of avfilter graph:
    AVPixelFormat setup_input_filters(std::ostringstream & oss,
                                      const AVFrame & src) const;

  public:
    //----------------------------------------------------------------
    // SetupFlags
    //
    enum SetupFlags
    {
      SCALE_TO_SPECS = 1,
    };

    // (re)configure avfilter graph on-demand for a given source
    // frame configuration and output specifications and any
    // other in-between filters specified in filterChain.
    //
    // throws an Error if filter graph configuration fails
    //
    // returns true if the filter was reconfigured,
    // returns false if no changes were required.
    //
    bool setup(// input specification:
               const AVFrame & src,
               const AVRational & src_framerate,
               const AVRational & src_timebase,

               // output specifications... any unspecified properties
               // should preserve input properties.
               //
               // width <= 0, heigth <= 0 imply that output width, height
               // is the same as input width, height:
               const AvFrmSpecs & dst_specs,

               // additional filters:
               const char * filter_chain = NULL,

               // since filter_chain may already be resizing
               // video to match dst_specs we need to specify
               // explicitly whether to add a separate scale filter:
               unsigned int setup_hints = 0);

    void push(AVFrame * in);

    // NOTE: a filter (yadif) may change the timebase of the output frame:
    bool pull(AVFrame * out, AVRational & outTimeBase);

    // accessors:
    inline const yae::AvBufferRef & hw_frames() const
    { return hw_frames_; }

    inline const yae::AvFrmSpecs & src_specs() const
    { return src_specs_; }

    inline const yae::AvFrmSpecs & dst_specs() const
    { return dst_specs_; }

    inline const AVRational & src_framerate() const
    { return src_framerate_; }

    inline const AVRational & src_timebase() const
    { return src_timebase_; }

    inline const std::string & get_filters() const
    { return filters_; }

    // set to 0 to let AVFilterGraph choose,
    // set to 1 to disable multi-threading
    // set to n > 1 for multi-threading
    inline void set_num_threads(int nb_threads)
    { nb_threads_ = nb_threads; }

  protected:
    // default is 0, let libavfilter decide:
    int nb_threads_;

    yae::AvBufferRef hw_frames_;
    yae::AvFrmSpecs src_specs_;
    yae::AvFrmSpecs dst_specs_;
    AVRational src_framerate_;
    AVRational src_timebase_;

    std::string filter_chain_;
    std::string filters_;
    bool flushing_;

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };
}


#endif // YAE_FFMPEG_VIDEO_FILTER_GRAPH_H_
