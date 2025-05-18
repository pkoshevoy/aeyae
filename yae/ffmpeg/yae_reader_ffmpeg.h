// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:02:05 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_READER_FFMPEG_H_
#define YAE_READER_FFMPEG_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/video/yae_reader.h"

// standard:
#include <list>


namespace yae
{

  //----------------------------------------------------------------
  // ReaderFFMPEG
  //
  struct YAE_API ReaderFFMPEG : public IReader
  {
  private:
    //! intentionally disabled:
    ReaderFFMPEG(const ReaderFFMPEG & f);
    ReaderFFMPEG & operator = (const ReaderFFMPEG & f);

    //! private implementation details:
    class Private;
    Private * const private_;

  protected:
    ReaderFFMPEG();
    virtual ~ReaderFFMPEG();

  public:
    static ReaderFFMPEG * create();
    virtual void destroy();

    //! prototype factory method that returns a new instance of ReaderFFMPEG,
    //! not a deep copy:
    virtual ReaderFFMPEG * clone() const;

    //! return a human readable name for this reader (preferably unique):
    virtual const char * name() const;

    //! a unique identifier for this plugin (use uuidgen to make one):
    static const char * kGUID;
    virtual const char * guid() const;

    //! plugin parameters for this reader:
    virtual ISettingGroup * settings();

    //! assemble a list of supported URL protocols:
    virtual bool getUrlProtocols(std::list<std::string> & protocols) const;

    //! open a resource specified by the resourcePath such as filepath or URL:
    virtual bool open(const char * resourcePathUTF8, bool hwdec);

    //! close currently open resource:
    virtual void close();

    virtual const char * getResourcePath() const;

    // in case reader emits an event indicating MPEG-TS program changes, or
    // ES codec changes - use this to refresh cached program and track info:
    virtual void refreshInfo();

    virtual std::size_t getNumberOfPrograms() const;
    virtual bool getProgramInfo(std::size_t i, TProgramInfo & info) const;

    virtual std::size_t getNumberOfVideoTracks() const;
    virtual std::size_t getNumberOfAudioTracks() const;

    virtual std::size_t getSelectedVideoTrackIndex() const;
    virtual std::size_t getSelectedAudioTrackIndex() const;

    virtual bool selectVideoTrack(std::size_t i);
    virtual bool selectAudioTrack(std::size_t i);

    virtual bool getSelectedVideoTrackInfo(TTrackInfo & info) const;
    virtual bool getSelectedAudioTrackInfo(TTrackInfo & info) const;

    // NOTE: for MPEG-TS files that may contain PTS timeline anomalies
    // it may be preferable to reference file position (or packet index)
    // as a timeline source instead.
    //
    // Here, for MPEG-TS typically start time is 0, base 188 (TS packet size)
    // and duration time is file size in bytes, base 188 (TS packet size)
    //
    virtual RefTimeline getRefTimeline() const;
    virtual bool setRefTimeline(RefTimeline timeline);
    virtual bool getPacketsExtent(TTime & start, TTime & duration) const;

    virtual bool getVideoDuration(TTime & start, TTime & duration) const;
    virtual bool getAudioDuration(TTime & start, TTime & duration) const;

    virtual bool getVideoTraits(VideoTraits & traits) const;
    virtual bool getAudioTraits(AudioTraits & traits) const;

    virtual bool setAudioTraitsOverride(const AudioTraits & override);
    virtual bool setVideoTraitsOverride(const VideoTraits & override);

    virtual bool getAudioTraitsOverride(AudioTraits & override) const;
    virtual bool getVideoTraitsOverride(VideoTraits & override) const;

    virtual bool isSeekable() const;
    virtual bool seek(double t);

    virtual bool readVideo(TVideoFramePtr & frame, QueueWaitMgr * mgr = 0);
    virtual bool readAudio(TAudioFramePtr & frame, QueueWaitMgr * mgr = 0);

    virtual bool blockedOnVideo() const;
    virtual bool blockedOnAudio() const;

    virtual bool threadStart();
    virtual bool threadStop();

    virtual void getPlaybackInterval(double & timeIn, double & timeOut) const;
    virtual void setPlaybackIntervalStart(double timeIn);
    virtual void setPlaybackIntervalEnd(double timeOut);
    virtual void setPlaybackEnabled(bool enabled);
    virtual void setPlaybackLooping(bool enabled);

    // these are used to speed up video decoding:
    virtual void skipLoopFilter(bool skip);
    virtual void skipNonReferenceFrames(bool skip);

    // this can be used to slow down audio if video decoding is too slow,
    // or it can be used to speed up audio to watch the movie faster:
    virtual bool setTempo(double tempo);

    // enable/disable video deinterlacing:
    virtual bool setDeinterlacing(bool enabled);

    // whether or not this will actually output any closed captions
    // depends on the input stream (whether it carries closed captions)
    // and the video decoder used (hardware decoders may not pass through
    // the closed captions data for decoding):
    //
    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    virtual void setRenderCaptions(unsigned int cc);
    virtual unsigned int getRenderCaptions() const;

    // query subtitles, enable/disable rendering...
    virtual std::size_t subsCount() const;
    virtual TSubsFormat subsInfo(std::size_t i, TTrackInfo & info) const;
    virtual void setSubsRender(std::size_t i, bool render);
    virtual bool getSubsRender(std::size_t i) const;

    // chapter navigation:
    virtual std::size_t countChapters() const;
    virtual bool getChapterInfo(std::size_t i, TChapter & c) const;

    // attachments (fonts, thumbnails, etc...)
    virtual std::size_t getNumberOfAttachments() const;
    virtual const TAttachment * getAttachmentInfo(std::size_t i) const;

    // frames produced by this reader will be tagged with a reader ID,
    // so that renderers can disambiguate between frames produced
    // by different readers:
    virtual void setReaderId(unsigned int readerId);

    // a reference to the clock that audio/video renderers use
    // to synchronize their output:
    virtual void setSharedClock(const SharedClock & clock);

    // optional, if set then reader may call eo->note(event)
    // to notify observer of significant events, such as MPEG-TS
    // program structure changes, ES codec changes, etc...
    virtual void setEventObserver(const TEventObserverPtr & eo);
  };

}


#endif // YAE_READER_FFMPEG_H_
