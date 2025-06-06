// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Wed May 26 22:17:43 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_READER_H_
#define YAE_READER_H_

// aeyae:
#include "yae/api/yae_plugin_interface.h"
#include "yae/api/yae_shared_ptr.h"
#include "yae/thread/yae_queue.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"

// standard:
#include <string>
#include <list>


namespace yae
{

  //----------------------------------------------------------------
  // RefTimeline
  //
  enum RefTimeline
  {
    kRefTimelinePos = 0,
    kRefTimelinePts = 1,
  };


  //----------------------------------------------------------------
  // IReader
  //
  struct YAE_API IReader : public IPlugin
  {
  public:

    //! a prototype factory method for constructing reader instances:
    virtual IReader * clone() const = 0;

    //! assemble a list of supported URL protocols:
    virtual bool getUrlProtocols(std::list<std::string> & protocols) const = 0;

    //! open a resource specified by the resourcePath such as filepath or URL:
    virtual bool open(const char * resourcePathUTF8, bool hwdec) = 0;

    //! close currently open resource:
    virtual void close() = 0;

    virtual const char * getResourcePath() const = 0;

    // in case reader emits an event indicating MPEG-TS program changes, or
    // ES codec changes - use this to refresh cached program and track info:
    virtual void refreshInfo() {}

    virtual std::size_t getNumberOfPrograms() const = 0;
    virtual bool getProgramInfo(std::size_t i, TProgramInfo & info) const = 0;

    virtual std::size_t getNumberOfVideoTracks() const = 0;
    virtual std::size_t getNumberOfAudioTracks() const = 0;

    virtual std::size_t getSelectedVideoTrackIndex() const = 0;
    virtual std::size_t getSelectedAudioTrackIndex() const = 0;

    virtual bool selectVideoTrack(std::size_t i) = 0;
    virtual bool selectAudioTrack(std::size_t i) = 0;

    // helpers:
    inline bool hasSelectedVideoTrack() const
    {
      return (this->getSelectedVideoTrackIndex() <
              this->getNumberOfVideoTracks());
    }

    inline bool hasSelectedAudioTrack() const
    {
      return (this->getSelectedAudioTrackIndex() <
              this->getNumberOfAudioTracks());
    }

    inline Timespan getVideoTimespan() const
    {
      TTime t0;
      TTime dt;
      return (this->getVideoDuration(t0, dt) ?
              Timespan(t0, t0 + dt) :
              Timespan());
    }

    inline Timespan getAudioTimespan() const
    {
      TTime t0;
      TTime dt;
      return (this->getAudioDuration(t0, dt) ?
              Timespan(t0, t0 + dt) :
              Timespan());
    }

    // NOTE: returns false if there is no such track:
    virtual bool getVideoTrackInfo(std::size_t video_track_index,
                                   TTrackInfo & info,
                                   VideoTraits & traits) const = 0;

    virtual bool getAudioTrackInfo(std::size_t audio_track_index,
                                   TTrackInfo & info,
                                   AudioTraits & traits) const = 0;

    // NOTE: for MPEG-TS files that may contain PTS timeline anomalies
    // it may be preferable to reference file position (or packet index)
    // as a timeline source instead.
    //
    // Here, for MPEG-TS typically start time is 0, base 188 (TS packet size)
    // and duration time is file size in bytes, base 188 (TS packet size)
    //
    virtual RefTimeline getRefTimeline() const
    { return kRefTimelinePts; }

    virtual bool setRefTimeline(RefTimeline ref_timeline)
    {
      (void)ref_timeline;
      return false;
    }

    virtual bool getPacketsExtent(TTime & start, TTime & duration) const
    {
      (void)start;
      (void)duration;
      return false;
    }

    virtual bool getVideoDuration(TTime & start, TTime & duration) const = 0;
    virtual bool getAudioDuration(TTime & start, TTime & duration) const = 0;

    //! access currently selected audio/video track traits:
    virtual bool getAudioTraits(AudioTraits & traits) const = 0;
    virtual bool getVideoTraits(VideoTraits & traits) const = 0;

    //! force decoded audio/video frames to be in a particular format:
    virtual bool setAudioTraitsOverride(const AudioTraits & override) = 0;
    virtual bool setVideoTraitsOverride(const VideoTraits & override) = 0;

    virtual bool getAudioTraitsOverride(AudioTraits & override) const = 0;
    virtual bool getVideoTraitsOverride(VideoTraits & override) const = 0;

    //! check whether it is possible to set the current position:
    virtual bool isSeekable() const = 0;

    //! set current position to a given value (or an earlier value nearby):
    virtual bool seek(double t) = 0;

    //! By default readAudio and readVideo will block indefinitely
    //! until a frame arrives or the frame queue is closed.
    //! Use QueueWaitMgr if you need to break out of the waiting
    //! loop for some reason (such as in order to avoid a deadlock).
    virtual bool readVideo(TVideoFramePtr & frame, QueueWaitMgr * mgr) = 0;
    virtual bool readAudio(TAudioFramePtr & frame, QueueWaitMgr * mgr) = 0;

    //! when blocked on video -- read video to unblock and break the deadlock:
    virtual bool blockedOnVideo() const = 0;

    //! when blocked on audio -- read audio to unblock and break the deadlock:
    virtual bool blockedOnAudio() const = 0;

    virtual bool threadStart() = 0;
    virtual bool threadStop() = 0;

    virtual void getPlaybackInterval(double & tIn, double & tOut) const = 0;
    virtual void setPlaybackIntervalStart(double timeIn) = 0;
    virtual void setPlaybackIntervalEnd(double timeOut) = 0;
    virtual void setPlaybackEnabled(bool enabled) = 0;
    virtual void setPlaybackLooping(bool enabled) = 0;

    // these are used to speed up video decoding:
    virtual void skipLoopFilter(bool skip) = 0;
    virtual void skipNonReferenceFrames(bool skip) = 0;

    // this can be used to slow down audio if video decoding is too slow,
    // or it can be used to speed up audio to watch the movie faster:
    virtual bool setTempo(double tempo) = 0;

    // enable/disable video deinterlacing:
    virtual bool setDeinterlacing(bool enabled) = 0;

    // whether or not this will actually output any closed captions
    // depends on the input stream (whether it carries closed captions)
    // and the video decoder used (hardware decoders may not pass through
    // the closed captions data for decoding)
    //
    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    virtual void setRenderCaptions(unsigned int cc) = 0;
    virtual unsigned int getRenderCaptions() const = 0;

    // query subtitles, enable/disable rendering...
    virtual std::size_t subsCount() const = 0;
    virtual TSubsFormat subsInfo(std::size_t i, TTrackInfo & info) const = 0;
    virtual void setSubsRender(std::size_t i, bool render) = 0;
    virtual bool getSubsRender(std::size_t i) const = 0;

    // helper:
    inline bool hasSelectedSubsTrack() const
    {
      for (std::size_t i = 0, n = this->subsCount(); i < n; i++)
      {
        if (this->getSubsRender(i))
        {
          return true;
        }
      }

      return false;
    }

    // chapter navigation:
    virtual std::size_t countChapters() const = 0;
    virtual bool getChapterInfo(std::size_t i, TChapter & c) const = 0;

    // attachments (fonts, thumbnails, etc...)
    virtual std::size_t getNumberOfAttachments() const = 0;
    virtual const TAttachment * getAttachmentInfo(std::size_t i) const = 0;

    // frames produced by this reader will be tagged with a reader ID,
    // so that renderers can disambiguate between frames produced
    // by different readers:
    virtual void setReaderId(unsigned int readerId) = 0;

    // a reference to the clock that audio/video renderers use
    // to synchronize their output:
    virtual void setSharedClock(const SharedClock & clock) = 0;

    // optional, if set then reader may call eo->note(event)
    // to notify observer of significant events, such as MPEG-TS
    // program structure changes, ES codec changes, etc...
    virtual void setEventObserver(const TEventObserverPtr & eo) = 0;

    // helpers:
    inline bool refers_to_packet_pos_timeline() const
    { return this->getRefTimeline() == kRefTimelinePos; }

    inline bool refers_to_frame_pts_timeline() const
    { return this->getRefTimeline() == kRefTimelinePts; }
  };

  //----------------------------------------------------------------
  // IReaderPtr
  //
  typedef yae::shared_ptr<IReader, IPlugin, call_destroy> IReaderPtr;

  //----------------------------------------------------------------
  // IReaderWPtr
  //
  typedef yae::weak_ptr<IReader, IPlugin, call_destroy> IReaderWPtr;

}


#endif // YAE_READER_H_
