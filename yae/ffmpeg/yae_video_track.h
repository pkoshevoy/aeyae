// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_TRACK_H_
#define YAE_VIDEO_TRACK_H_

// boost library:
#ifndef Q_MOC_RUN
#include <boost/chrono/chrono.hpp>
#include <boost/shared_ptr.hpp>
#endif

// yae includes:
#include "yae/ffmpeg/yae_closed_captions.h"
#include "yae/ffmpeg/yae_ffmpeg_video_filter_graph.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/ffmpeg/yae_track.h"
#include "yae/thread/yae_queue.h"


namespace yae
{

  //----------------------------------------------------------------
  // TVideoFrameQueue
  //
  typedef Queue<TVideoFramePtr> TVideoFrameQueue;


  //----------------------------------------------------------------
  // TAVFrameBuffer
  //
  struct YAE_API TAVFrameBuffer : public IPlanarBuffer
  {
    TAVFrameBuffer(AVFrame * src);

    // virtual:
    void destroy();

    // virtual:
    std::size_t planes() const;

    // virtual:
    unsigned char * data(std::size_t plane) const;

    // virtual:
    std::size_t rowBytes(std::size_t plane) const;

    AvFrm frame_;
  };


  //----------------------------------------------------------------
  // VideoTrack
  //
  struct YAE_API VideoTrack : public Track
  {
    VideoTrack(Track * track);
    ~VideoTrack();

    // virtual:
    bool initTraits();

    // virtual:
    AVCodecContext * open();

    // virtual:
    bool frameQueueWaitForConsumerToBlock(QueueWaitMgr * mgr = NULL)
    { return frameQueue_.waitIndefinitelyForConsumerToBlock(mgr); }

    // virtual:
    void frameQueueClear()
    { frameQueue_.clear(); }

    // these are used to speed up video decoding:
    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    // helpers: these are used to re-configure output buffers
    // when frame traits change:
    void refreshTraits(const AVFrame * decoded = NULL);

    // virtual:
    bool decoderStartup();
    bool decoderShutdown();

    // virtual:
    void handle(const AvFrm & decodedFrame);

    // virtual:
    bool threadStop();

    // video traits, not overridden:
    bool getTraits(VideoTraits & traits, const AVFrame * decoded = NULL) const;

    // use this for video frame conversion (pixel format and size)
    bool setTraitsOverride(const VideoTraits & traits,
                           bool deint,
                           double overrideSourcePAR = 0.0);

    bool getTraitsOverride(VideoTraits & traits) const;

    inline bool setTraitsOverride(const VideoTraits & traits)
    { return setTraitsOverride(traits, deinterlace_); }

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(const TSeekPosPtr & posIn,
                             const TSeekPosPtr & posOut,
                             bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(const TSeekPosPtr & seekPos,
                          bool dropPendingFrames);

    // adjust frame duration:
    bool setDeinterlacing(bool enabled);

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    void enableClosedCaptions(unsigned int cc);

    void setSubs(std::vector<SubttTrackPtr> * subs)
    { subs_ = subs; }

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    bool deinterlace_;

    // override input (native) pixel aspect ratio (in case it's wrong):
    double overrideSourcePAR_;

    TVideoFrameQueue frameQueue_;
    VideoTraits override_;
    VideoTraits native_;
    VideoTraits output_;

    AVRational frameRate_;

    TTime prevPTS_;
    bool hasPrevPTS_;

    uint64 framesDecoded_;
    uint64 framesProduced_;

    // CEA-608 closed captions decoder:
    CaptionsDecoder cc_;

    // subtitles:
    std::vector<SubttTrackPtr> * subs_;

    VideoFilterGraph filterGraph_;

    std::vector<unsigned char> temp_;

#ifndef NDEBUG
    // for estimating decoder fps and output fps:
    boost::chrono::steady_clock::time_point t0_;
#endif
  };

  //----------------------------------------------------------------
  // VideoTrackPtr
  //
  typedef boost::shared_ptr<VideoTrack> VideoTrackPtr;
}


#endif // YAE_VIDEO_TRACK_H_
