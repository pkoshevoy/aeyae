// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

//----------------------------------------------------------------
// __STDC_CONSTANT_MACROS
//
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// aeyae:
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_movie.h"
#include "yae/ffmpeg/yae_reader_ffmpeg.h"
#include "yae/thread/yae_queue.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// standard:
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <set>


//----------------------------------------------------------------
// YAE_READER_FFMPEG_GUID
//
#define YAE_READER_FFMPEG_GUID "5299E7BE-11B9-49D3-967D-5A15184A0AE9"

namespace yae
{

  //----------------------------------------------------------------
  // ReaderFFMPEG::Private
  //
  class ReaderFFMPEG::Private
  {
  private:
    // intentionally disabled:
    Private(const Private &);
    Private & operator = (const Private &);

  public:
    Private():
      readerId_((unsigned int)~0),
      ref_timeline_(kRefTimelinePts)
    {
      this->init_in_out();
    }

    void init_in_out()
    {
      if (ref_timeline_ == kRefTimelinePos)
      {
        timeIn_.reset(new PacketPos(0, 188));
        timeOut_.reset(new PacketPos(kMaxInt64, 188));
      }
      else
      {
        timeIn_.reset(new TimePos(TTime::min_flicks_as_sec()));
        timeOut_.reset(new TimePos(TTime::max_flicks_as_sec()));
      }
    }

    void init_ref_timeline()
    {
      std::string src = movie_.getFormatName();
      // check the file for timeline anomalies and select kRefTimelinePos
      // only if timeline anomalies are detected:
      ref_timeline_ =
        movie_.find_anomalies() ? kRefTimelinePos : kRefTimelinePts;
      this->init_in_out();
    }

    Movie movie_;
    unsigned int readerId_;
    RefTimeline ref_timeline_;
    TSeekPosPtr timeIn_;
    TSeekPosPtr timeOut_;
  };

  //----------------------------------------------------------------
  // ReaderFFMPEG::ReaderFFMPEG
  //
  ReaderFFMPEG::ReaderFFMPEG():
    IReader(),
    private_(new ReaderFFMPEG::Private())
  {}

  //----------------------------------------------------------------
  // ReaderFFMPEG::~ReaderFFMPEG
  //
  ReaderFFMPEG::~ReaderFFMPEG()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::create
  //
  ReaderFFMPEG *
  ReaderFFMPEG::create()
  {
    return new ReaderFFMPEG();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::destroy
  //
  void
  ReaderFFMPEG::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::clone
  //
  ReaderFFMPEG *
  ReaderFFMPEG::clone() const
  {
    return ReaderFFMPEG::create();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::name
  //
  const char *
  ReaderFFMPEG::name() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::kGUID
  //
  const char *
  ReaderFFMPEG::kGUID = YAE_READER_FFMPEG_GUID;

  //----------------------------------------------------------------
  // ReaderFFMPEG::guid
  //
  const char *
  ReaderFFMPEG::guid() const
  {
    return ReaderFFMPEG::kGUID;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::settings
  //
  ISettingGroup *
  ReaderFFMPEG::settings()
  {
    return private_->movie_.settings();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getUrlProtocols
  //
  bool
  ReaderFFMPEG::getUrlProtocols(std::list<std::string> & protocols) const
  {
    return private_->movie_.getUrlProtocols(protocols);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::open
  //
  bool
  ReaderFFMPEG::open(const char * resourcePathUTF8, bool hwdec)
  {
    bool ok = private_->movie_.open(resourcePathUTF8, hwdec);
    private_->init_ref_timeline();
    return ok;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::close
  //
  void
  ReaderFFMPEG::close()
  {
    return private_->movie_.close();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getResourcePath
  //
  const char *
  ReaderFFMPEG::getResourcePath() const
  {
    return private_->movie_.getResourcePath();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::refreshInfo
  //
  void
  ReaderFFMPEG::refreshInfo()
  {
    private_->movie_.requestInfoRefresh();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfPrograms
  //
  std::size_t
  ReaderFFMPEG::getNumberOfPrograms() const
  {
    return private_->movie_.get_num_programs();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getProgramInfo
  //
  bool
  ReaderFFMPEG::getProgramInfo(std::size_t i, TProgramInfo & info) const
  {
    return private_->movie_.getProgramInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfVideoTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfVideoTracks() const
  {
    return private_->movie_.get_num_video_tracks();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfAudioTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfAudioTracks() const
  {
    return private_->movie_.get_num_audio_tracks();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedVideoTrackIndex
  //
  std::size_t
  ReaderFFMPEG::getSelectedVideoTrackIndex() const
  {
    return private_->movie_.getSelectedVideoTrack();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackIndex
  //
  std::size_t
  ReaderFFMPEG::getSelectedAudioTrackIndex() const
  {
    return private_->movie_.getSelectedAudioTrack();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::selectVideoTrack
  //
  bool
  ReaderFFMPEG::selectVideoTrack(std::size_t i)
  {
    return private_->movie_.selectVideoTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::selectAudioTrack
  //
  bool
  ReaderFFMPEG::selectAudioTrack(std::size_t i)
  {
    return private_->movie_.selectAudioTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTrackInfo
  //
  bool
  ReaderFFMPEG::getVideoTrackInfo(std::size_t track_index,
                                  TTrackInfo & info,
                                  VideoTraits & traits) const
  {
    return private_->movie_.getVideoTrackInfo(track_index, info, traits);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTrackInfo
  //
  bool
  ReaderFFMPEG::getAudioTrackInfo(std::size_t track_index,
                                  TTrackInfo & info,
                                  AudioTraits & traits) const
  {
    return private_->movie_.getAudioTrackInfo(track_index, info, traits);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getRefTimeline
  //
  RefTimeline
  ReaderFFMPEG::getRefTimeline() const
  {
    return private_->ref_timeline_;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setRefTimeline
  //
  bool
  ReaderFFMPEG::setRefTimeline(RefTimeline ref_timeline)
  {
    private_->ref_timeline_ = ref_timeline;
    return true;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getPacketsExtent
  //
  bool
  ReaderFFMPEG::getPacketsExtent(TTime & start, TTime & duration) const
  {
    uint64_t file_size = private_->movie_.get_file_size();
    start.reset(0, 188);
    duration.reset(file_size, 188);
    return true;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoDuration
  //
  bool
  ReaderFFMPEG::getVideoDuration(TTime & start, TTime & duration) const
  {
    VideoTrackPtr track = private_->movie_.curr_video_track();
    if (track)
    {
      track->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioDuration
  //
  bool
  ReaderFFMPEG::getAudioDuration(TTime & start, TTime & duration) const
  {
    AudioTrackPtr track = private_->movie_.curr_audio_track();
    if (track)
    {
      track->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraits
  //
  bool
  ReaderFFMPEG::getVideoTraits(VideoTraits & traits) const
  {
    VideoTrackPtr track = private_->movie_.curr_video_track();
    return track ? track->getTraits(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraits
  //
  bool
  ReaderFFMPEG::getAudioTraits(AudioTraits & traits) const
  {
    AudioTrackPtr track = private_->movie_.curr_audio_track();
    return track ? track->getTraits(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::setAudioTraitsOverride(const AudioTraits & traits)
  {
    AudioTrackPtr track = private_->movie_.curr_audio_track();
    return track ? track->setTraitsOverride(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::setVideoTraitsOverride(const VideoTraits & traits)
  {
    VideoTrackPtr track = private_->movie_.curr_video_track();
    return track ? track->setTraitsOverride(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::getAudioTraitsOverride(AudioTraits & traits) const
  {
    AudioTrackPtr track = private_->movie_.curr_audio_track();
    return track ? track->getTraitsOverride(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::getVideoTraitsOverride(VideoTraits & traits) const
  {
    VideoTrackPtr track = private_->movie_.curr_video_track();
    return track ? track->getTraitsOverride(traits) : false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::isSeekable
  //
  bool
  ReaderFFMPEG::isSeekable() const
  {
    return private_->movie_.isSeekable() && private_->movie_.hasDuration();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::seek
  //
  bool
  ReaderFFMPEG::seek(double seekTime)
  {
    if (this->refers_to_packet_pos_timeline())
    {
      int64_t packet_pos = seekTime * 188;
      packet_pos -= packet_pos % 188;
      TPacketPosPtr pos(new PacketPos(packet_pos, 188));
      return private_->movie_.requestSeek(pos);
    }

    TTimePosPtr pos(new TimePos(seekTime));
    return private_->movie_.requestSeek(pos);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::readVideo
  //
  bool
  ReaderFFMPEG::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    VideoTrackPtr track = private_->movie_.curr_video_track();
    bool ok = track && track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = private_->readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::readAudio
  //
  bool
  ReaderFFMPEG::readAudio(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    AudioTrackPtr track = private_->movie_.curr_audio_track();
    bool ok = track && track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = private_->readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::blockedOnVideo
  //
  bool
  ReaderFFMPEG::blockedOnVideo() const
  {
    return private_->movie_.blockedOnVideo();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::blockedOnAudio
  //
  bool
  ReaderFFMPEG::blockedOnAudio() const
  {
    return private_->movie_.blockedOnAudio();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::threadStart
  //
  bool
  ReaderFFMPEG::threadStart()
  {
    return private_->movie_.threadStart();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::threadStop
  //
  bool
  ReaderFFMPEG::threadStop()
  {
    return private_->movie_.threadStop();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getPlaybackInterval
  //
  void
  ReaderFFMPEG::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    timeIn = private_->timeIn_->get();
    timeOut = private_->timeOut_->get();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalStart
  //
  void
  ReaderFFMPEG::setPlaybackIntervalStart(double pos)
  {
    if (this->refers_to_packet_pos_timeline())
    {
      private_->timeIn_.reset(new PacketPos(pos * 188, 188));
    }
    else
    {
      private_->timeIn_.reset(new TimePos(pos));
    }

    private_->movie_.setPlaybackIntervalStart(private_->timeIn_);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalEnd
  //
  void
  ReaderFFMPEG::setPlaybackIntervalEnd(double pos)
  {
    if (this->refers_to_packet_pos_timeline())
    {
      private_->timeOut_.reset(new PacketPos(pos * 188, 188));
    }
    else
    {
      private_->timeOut_.reset(new TimePos(pos));
    }

    private_->movie_.setPlaybackIntervalEnd(private_->timeOut_);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackEnabled
  //
  void
  ReaderFFMPEG::setPlaybackEnabled(bool enabled)
  {
#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (enabled)
    {
      yae_debug << "\nPLAYBACK ENABLED, framestep not possible\n";
    }
    else
    {
      yae_debug << "\nPLAYBACK DISABLED (paused), framestep OK\n";
    }
#endif

    private_->movie_.setPlaybackEnabled(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackLooping
  //
  void
  ReaderFFMPEG::setPlaybackLooping(bool enabled)
  {
    private_->movie_.setPlaybackLooping(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::skipLoopFilter
  //
  void
  ReaderFFMPEG::skipLoopFilter(bool skip)
  {
    private_->movie_.skipLoopFilter(skip);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::skipNonReferenceFrames
  //
  void
  ReaderFFMPEG::skipNonReferenceFrames(bool skip)
  {
    private_->movie_.skipNonReferenceFrames(skip);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setTempo
  //
  bool
  ReaderFFMPEG::setTempo(double tempo)
  {
    return private_->movie_.setTempo(tempo);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setDeinterlacing
  //
  bool
  ReaderFFMPEG::setDeinterlacing(bool enabled)
  {
    return private_->movie_.setDeinterlacing(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setRenderCaptions
  //
  void
  ReaderFFMPEG::setRenderCaptions(unsigned int cc)
  {
    private_->movie_.setRenderCaptions(cc);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getRenderCaptions
  //
  unsigned int
  ReaderFFMPEG::getRenderCaptions() const
  {
    return private_->movie_.getRenderCaptions();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsCount
  //
  std::size_t
  ReaderFFMPEG::subsCount() const
  {
    return private_->movie_.subsCount();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsInfo
  //
  TSubsFormat
  ReaderFFMPEG::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    return private_->movie_.subsInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setSubsRender
  //
  void
  ReaderFFMPEG::setSubsRender(std::size_t i, bool render)
  {
    private_->movie_.setSubsRender(i, render);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSubsRender
  //
  bool
  ReaderFFMPEG::getSubsRender(std::size_t i) const
  {
    return private_->movie_.getSubsRender(i);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::countChapters
  //
  std::size_t
  ReaderFFMPEG::countChapters() const
  {
    return private_->movie_.countChapters();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getChapterInfo
  //
  bool
  ReaderFFMPEG::getChapterInfo(std::size_t i, TChapter & c) const
  {
    return private_->movie_.getChapterInfo(i, c);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfAttachments
  //
  std::size_t
  ReaderFFMPEG::getNumberOfAttachments() const
  {
    return private_->movie_.attachments().size();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAttachmentInfo
  //
  const TAttachment *
  ReaderFFMPEG::getAttachmentInfo(std::size_t i) const
  {
    if (i < private_->movie_.attachments().size())
    {
      return &(private_->movie_.attachments()[i]);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setReaderId
  //
  void
  ReaderFFMPEG::setReaderId(unsigned int readerId)
  {
    private_->readerId_ = readerId;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setSharedClock
  //
  void
  ReaderFFMPEG::setSharedClock(const SharedClock & clock)
  {
    private_->movie_.setSharedClock(clock);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setEventObserver
  //
  void
  ReaderFFMPEG::setEventObserver(const TEventObserverPtr & eo)
  {
    private_->movie_.setEventObserver(eo);
  }
}

#if 0
extern "C"
{
  //----------------------------------------------------------------
  // yae_create_plugin
  //
  YAE_API_EXPORT yae::IPlugin *
  yae_create_plugin(std::size_t i)
  {
    if (i == 0)
    {
      return yae::ReaderFFMPEG::create();
    }

    return NULL;
  }
}
#endif
