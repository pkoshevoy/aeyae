// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  3 10:55:14 MDT 2018
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_DEMUXER_READER_H_
#define YAE_DEMUXER_READER_H_

// system includes:
#include <list>
#include <string>

// yae includes:
#include "yae/api/yae_api.h"
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"


namespace yae
{

  //----------------------------------------------------------------
  // DemuxerReader
  //
  struct YAE_API DemuxerReader : public IReader
  {
  protected:
    DemuxerReader(const TDemuxerInterfacePtr & src = TDemuxerInterfacePtr());
    virtual ~DemuxerReader();

    void init(const TDemuxerInterfacePtr & src);

  public:
    static DemuxerReader * create(const TDemuxerInterfacePtr & demuxer =
                                  TDemuxerInterfacePtr());
    virtual void destroy();

    //! prototype factory method that returns a new instance of DemuxerReader,
    //! not a deep copy:
    virtual DemuxerReader * clone() const;

    //! return a human readable name for this reader (preferably unique):
    virtual const char * name() const;

    //! a unique identifier for this implementation (use uuidgen to make one):
    static const char * kGUID;
    virtual const char * guid() const;

    //! plugin parameters for this reader:
    virtual ISettingGroup * settings();

    //! assemble a list of supported URL protocols:
    virtual bool getUrlProtocols(std::list<std::string> & protocols) const;

    //! open a resource specified by the resourcePath such as filepath or URL:
    virtual bool open(const char * resourcePathUTF8);

    //! close currently open resource:
    virtual void close();

    virtual std::size_t getNumberOfPrograms() const;
    virtual bool getProgramInfo(std::size_t i, TProgramInfo & info) const;

    virtual std::size_t getNumberOfVideoTracks() const;
    virtual std::size_t getNumberOfAudioTracks() const;

    virtual std::size_t getSelectedVideoTrackIndex() const;
    virtual std::size_t getSelectedAudioTrackIndex() const;

    virtual bool selectVideoTrack(std::size_t i);
    virtual bool selectAudioTrack(std::size_t i);

    virtual void getSelectedVideoTrackInfo(TTrackInfo & info) const;
    virtual void getSelectedAudioTrackInfo(TTrackInfo & info) const;

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

    // helpers:
    void requestMutex(boost::unique_lock<boost::timed_mutex> & lk);

    int seekTo(double seekTime, bool dropPendingFrames);

    int rewind(const AudioTrackPtr & audioTrack,
               const VideoTrackPtr & videoTrack,
               bool seekToTimeIn = true);

    // this will demux and push packets to decoding queues:
    void thread_loop();

    // accessors:
    inline VideoTrackPtr selectedVideoTrack() const
    {
      return ((selectedVideoTrack_ < video_.size()) ?
              video_[selectedVideoTrack_] :
              VideoTrackPtr());
    }

    inline AudioTrackPtr selectedAudioTrack() const
    {
      return ((selectedAudioTrack_ < audio_.size()) ?
              audio_[selectedAudioTrack_] :
              AudioTrackPtr());
    }

    inline SubttTrackPtr subttTrack(std::size_t i) const
    {
      return (i < subtt_.size()) ? subtt_[i] : SubttTrackPtr();
    }

  protected:
    //! intentionally disabled:
    DemuxerReader(const DemuxerReader & f);
    DemuxerReader & operator = (const DemuxerReader & f);

    // demuxer:
    TDemuxerInterfacePtr demuxer_;

    // reader id:
    unsigned int readerId_;

    // these are derived from demuxer summary:
    std::vector<TProgramInfo> programs_;
    std::map<std::string, std::size_t> prog_lut_;
    std::vector<VideoTrackPtr> video_;
    std::vector<AudioTrackPtr> audio_;
    std::vector<SubttTrackPtr> subtt_;
    std::vector<TChapter> chapters_;

    // threading:
    mutable boost::timed_mutex mutex_;
    Thread<DemuxerReader> thread_;

    // output queue(s) deadlock avoidance mechanism:
    QueueWaitMgr outputTerminator_;

    // this one is only used to avoid a deadlock waiting
    // for audio renderer to empty out the frame queue
    // during frame stepping (when playback is disabled)
    QueueWaitMgr framestepTerminator_;

    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    // 0 - disabled
    // 1 - CC1
    // 2 - CC2
    // 3 - CC3
    // 4 - CC4
    unsigned int enableClosedCaptions_;

    // looping parameters:
    double timeIn_;
    double timeOut_;
    bool playbackEnabled_;
    bool looping_;

    bool mustStop_;
    bool mustSeek_;
    double seekTime_;

    // shared clock used to synchronize the renderers:
    SharedClock clock_;

    // top-level settings group:
    yae::TSettingGroup settings_;
    yae::TSettingUInt32 videoQueueSize_;
    yae::TSettingUInt32 audioQueueSize_;
  };

  //----------------------------------------------------------------
  // DemuxerReaderPtr
  //
  typedef yae::shared_ptr<DemuxerReader, yae::IPlugin, yae::call_destroy>
  DemuxerReaderPtr;

  //----------------------------------------------------------------
  // make_yaerx_reader
  //
  YAE_API DemuxerReaderPtr
  make_yaerx_reader(const std::string & yaerx_path);

}


#endif // YAE_DEMUXER_READER_H_
