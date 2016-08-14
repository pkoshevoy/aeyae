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

// system includes:
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <set>

// yae includes:
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_movie.h"
#include "yae/thread/yae_queue.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// local includes:
#include "yae_reader_ffmpeg.h"


//----------------------------------------------------------------
// YAE_READER_FFMPEG_GUID
//
#define YAE_READER_FFMPEG_GUID "5299E7BE-11B9-49D3-967D-5A15184A0AE9"

//----------------------------------------------------------------
// YAE_DEBUG_SEEKING_AND_FRAMESTEP
//
#define YAE_DEBUG_SEEKING_AND_FRAMESTEP 0

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
      readerId_((unsigned int)~0)
    {}

    Movie movie_;
    unsigned int readerId_;
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
  ReaderFFMPEG::open(const char * resourcePathUTF8)
  {
    return private_->movie_.open(resourcePathUTF8);
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
  // ReaderFFMPEG::getNumberOfVideoTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfVideoTracks() const
  {
    return private_->movie_.getVideoTracks().size();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfAudioTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfAudioTracks() const
  {
    return private_->movie_.getAudioTracks().size();
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
  // ReaderFFMPEG::getSelectedVideoTrackName
  //
  void
  ReaderFFMPEG::getSelectedVideoTrackInfo(TTrackInfo & info) const
  {
    info.ntracks_ = private_->movie_.getVideoTracks().size();
    info.index_ = private_->movie_.getSelectedVideoTrack();
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
    }
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackInfo
  //
  void
  ReaderFFMPEG::getSelectedAudioTrackInfo(TTrackInfo & info) const
  {
    info.ntracks_ = private_->movie_.getAudioTracks().size();
    info.index_ = private_->movie_.getSelectedAudioTrack();
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
    }
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoDuration
  //
  bool
  ReaderFFMPEG::getVideoDuration(TTime & start, TTime & duration) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      private_->movie_.getVideoTracks()[i]->getDuration(start, duration);
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
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      private_->movie_.getAudioTracks()[i]->getDuration(start, duration);
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
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraits
  //
  bool
  ReaderFFMPEG::getAudioTraits(AudioTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::setAudioTraitsOverride(const AudioTraits & override)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->setTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::setVideoTraitsOverride(const VideoTraits & override)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->setTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::getAudioTraitsOverride(AudioTraits & override) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->getTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::getVideoTraitsOverride(VideoTraits & override) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->getTraitsOverride(override);
    }

    return false;
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
    return private_->movie_.requestSeekTime(seekTime);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::readVideo
  //
  bool
  ReaderFFMPEG::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (private_->movie_.getVideoTracks().size() <= i)
    {
      return false;
    }

    VideoTrackPtr track = private_->movie_.getVideoTracks()[i];
    bool ok = track->getNextFrame(frame, terminator);
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
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (private_->movie_.getAudioTracks().size() <= i)
    {
      return false;
    }

    AudioTrackPtr track = private_->movie_.getAudioTracks()[i];
    bool ok = track->getNextFrame(frame, terminator);
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
    private_->movie_.getPlaybackInterval(timeIn, timeOut);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalStart
  //
  void
  ReaderFFMPEG::setPlaybackIntervalStart(double timeIn)
  {
    private_->movie_.setPlaybackIntervalStart(timeIn);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalEnd
  //
  void
  ReaderFFMPEG::setPlaybackIntervalEnd(double timeOut)
  {
    private_->movie_.setPlaybackIntervalEnd(timeOut);
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
      std::cerr << "\nPLAYBACK ENABLED, framestep not possible" << std::endl;
    }
    else
    {
      std::cerr << "\nPLAYBACK DISABLED (paused), framestep OK" << std::endl;
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
}

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
#ifndef NDEBUG
      av_log_set_level(AV_LOG_DEBUG);
#endif
      return yae::ReaderFFMPEG::create();
    }

    return NULL;
  }
}
