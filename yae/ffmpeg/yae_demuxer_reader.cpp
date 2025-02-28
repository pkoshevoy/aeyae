// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jun  3 11:12:37 MDT 2018
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <stdexcept>

// local includes:
#include "yae_demuxer_reader.h"
#include "yae/ffmpeg/yae_audio_track.h"
#include "yae/ffmpeg/yae_remux.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/ffmpeg/yae_video_track.h"
#include "yae/thread/yae_queue.h"
#include "yae/thread/yae_threading.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video.h"


//----------------------------------------------------------------
// YAE_DEMUXER_READER_GUID
//
#define YAE_DEMUXER_READER_GUID "343CCC39-0EDC-481A-B8C8-B4BEBFB747B5"


namespace yae
{

  //----------------------------------------------------------------
  // c608
  //
  const unsigned int c608 = MKTAG('c', '6', '0', '8');

  //----------------------------------------------------------------
  // c708
  //
  const unsigned int c708 = MKTAG('c', '7', '0', '8');


  //----------------------------------------------------------------
  // DemuxerReader::DemuxerReader
  //
  DemuxerReader::DemuxerReader(const TDemuxerInterfacePtr & demuxer,
                               bool hwdec):
    readerId_(std::numeric_limits<unsigned int>::max()),
    thread_(this)
  {
    ensure_ffmpeg_initialized();

    init(demuxer, hwdec);
  }

  //----------------------------------------------------------------
  // DemuxerReader::init
  //
  void
  DemuxerReader::init(const TDemuxerInterfacePtr & demuxer_ptr, bool hwdec)
  {
    hwdec_ = hwdec;
    demuxer_.reset(demuxer_ptr ? demuxer_ptr->clone() : NULL);
    selectedVideoTrack_ = std::numeric_limits<std::size_t>::max();
    selectedAudioTrack_ = std::numeric_limits<std::size_t>::max();
    skipLoopFilter_ = false;
    skipNonReferenceFrames_ = false;
    enableClosedCaptions_ = 0;
    timeIn_ = -std::numeric_limits<double>::max();
    timeOut_ = std::numeric_limits<double>::max();
    playbackEnabled_ = false;
    looping_ = false;
    mustStop_ = true;
    mustSeek_ = false;
    seekTime_ = 0.0;

    if (!demuxer_)
    {
      return;
    }

    // shortcut:
    const DemuxerSummary & summary = demuxer_->summary();

    // collect programs:
    std::map<int, std::size_t> index_lut;
    for (std::map<int, TProgramInfo>::const_iterator
           i = summary.programs_.begin(); i != summary.programs_.end(); ++i)
    {
      index_lut[i->first] = programs_.size();
      programs_.push_back(i->second);
    }

    // collect tracks:
    for (std::map<std::string, TrackPtr>::const_iterator
           i = summary.decoders_.begin(); i != summary.decoders_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TrackPtr & decoder = i->second;

      if (!decoder)
      {
        continue;
      }

      decoder->maybe_reopen(hwdec);

      int prog_id = yae::at(summary.trk_prog_, track_id);
      prog_lut_[track_id] = yae::at(index_lut, prog_id);

      VideoTrackPtr v_trk =
        boost::dynamic_pointer_cast<VideoTrack, Track>(decoder);

      AudioTrackPtr a_trk =
        boost::dynamic_pointer_cast<AudioTrack, Track>(decoder);

      SubttTrackPtr s_trk =
        boost::dynamic_pointer_cast<SubtitlesTrack, Track>(decoder);

      if (v_trk)
      {
        video_.push_back(v_trk);
      }
      else if (a_trk)
      {
        audio_.push_back(a_trk);
      }
      else if (s_trk)
      {
        subtt_.push_back(s_trk);
      }
    }

    // collect chapters:
    for (std::map<TTime, TChapter>::const_iterator
           i = summary.chapters_.begin(); i != summary.chapters_.end(); ++i)
    {
      const TChapter & chapter = i->second;
      chapters_.push_back(chapter);
    }
  }

  //----------------------------------------------------------------
  // DemuxerReader::~DemuxerReader
  //
  DemuxerReader::~DemuxerReader()
  {
    close();

    for (std::size_t i = 0, n = video_.size(); i < n; i++)
    {
      const VideoTrackPtr & track = video_[i];
      track->setSubs(NULL);
    }
  }

  //----------------------------------------------------------------
  // DemuxerReader::create
  //
  DemuxerReader *
  DemuxerReader::create(const TDemuxerInterfacePtr & demuxer, bool hwdec)
  {
    return new DemuxerReader(demuxer, hwdec);
  }

  //----------------------------------------------------------------
  // DemuxerReader::destroy
  //
  void
  DemuxerReader::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // DemuxerReader::clone
  //
  DemuxerReader *
  DemuxerReader::clone() const
  {
    return DemuxerReader::create(demuxer_, hwdec_);
  }

  //----------------------------------------------------------------
  // DemuxerReader::name
  //
  const char *
  DemuxerReader::name() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // DemuxerReader::kGUID
  //
  const char *
  DemuxerReader::kGUID = "e78f73cd-d092-40ee-861d-4d94554e99d7";

  //----------------------------------------------------------------
  // DemuxerReader::guid
  //
  const char *
  DemuxerReader::guid() const
  {
    return DemuxerReader::kGUID;
  }

  //----------------------------------------------------------------
  // DemuxerReader::settings
  //
  ISettingGroup *
  DemuxerReader::settings()
  {
    return &settings_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getUrlProtocols
  //
  bool
  DemuxerReader::getUrlProtocols(std::list<std::string> & protocols) const
  {
    YAE_ASSERT(false);
    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::open
  //
  bool
  DemuxerReader::open(const char * resourcePathUTF8, bool hwdec)
  {
    if (!al::ends_with(resourcePathUTF8, ".yaerx"))
    {
      return false;
    }

    TSerialDemuxerPtr serial_demuxer =
      yae::load_yaerx(std::string(resourcePathUTF8));
    if (!serial_demuxer)
    {
      return false;
    }

    resourcePath_ = resourcePathUTF8;
    init(serial_demuxer, hwdec);
    return !!demuxer_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::close
  //
  void
  DemuxerReader::close()
  {
    threadStop();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getResourcePath
  //
  const char *
  DemuxerReader::getResourcePath() const
  {
    return resourcePath_.empty() ? NULL : resourcePath_.c_str();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getNumberOfPrograms
  //
  std::size_t
  DemuxerReader::getNumberOfPrograms() const
  {
    return programs_.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getProgramInfo
  //
  bool
  DemuxerReader::getProgramInfo(std::size_t i, TProgramInfo & info) const
  {
    if (i < programs_.size())
    {
      info = programs_[i];
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getNumberOfVideoTracks
  //
  std::size_t
  DemuxerReader::getNumberOfVideoTracks() const
  {
    return video_.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getNumberOfAudioTracks
  //
  std::size_t
  DemuxerReader::getNumberOfAudioTracks() const
  {
    return audio_.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getSelectedVideoTrackIndex
  //
  std::size_t
  DemuxerReader::getSelectedVideoTrackIndex() const
  {
    return selectedVideoTrack_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getSelectedAudioTrackIndex
  //
  std::size_t
  DemuxerReader::getSelectedAudioTrackIndex() const
  {
    return selectedAudioTrack_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::selectVideoTrack
  //
  bool
  DemuxerReader::selectVideoTrack(std::size_t i)
  {
    if (selectedVideoTrack_ == i)
    {
      // same track:
      return true;
    }

    if (selectedVideoTrack_ < video_.size())
    {
      const VideoTrackPtr & track = video_[selectedVideoTrack_];
      track->close();
    }

    selectedVideoTrack_ = i;

    if (selectedVideoTrack_ >= video_.size())
    {
      return false;
    }

    const VideoTrackPtr & track = video_[selectedVideoTrack_];
    track->setPlaybackInterval(TSeekPosPtr(new TimePos(timeIn_)),
                               TSeekPosPtr(new TimePos(timeOut_)),
                               playbackEnabled_);
    track->skipLoopFilter(skipLoopFilter_);
    track->skipNonReferenceFrames(skipNonReferenceFrames_);
    track->enableClosedCaptions(enableClosedCaptions_);
    track->setSubs(&subtt_);

    return track->initTraits();
  }

  //----------------------------------------------------------------
  // DemuxerReader::selectAudioTrack
  //
  bool
  DemuxerReader::selectAudioTrack(std::size_t i)
  {
   if (selectedAudioTrack_ == i)
    {
      // same track:
      return true;
    }

    if (selectedAudioTrack_ < audio_.size())
    {
      const AudioTrackPtr & track = audio_[selectedAudioTrack_];
      track->close();
    }

    selectedAudioTrack_ = i;

    if (selectedAudioTrack_ >= audio_.size())
    {
      return false;
    }

    const AudioTrackPtr & track = audio_[selectedAudioTrack_];
    track->setPlaybackInterval(TSeekPosPtr(new TimePos(timeIn_)),
                               TSeekPosPtr(new TimePos(timeOut_)),
                               playbackEnabled_);

    return track->initTraits();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getSelectedVideoTrackInfo
  //
  bool
  DemuxerReader::getSelectedVideoTrackInfo(TTrackInfo & info) const
  {
    info.nprograms_ = programs_.size();
    info.program_ = info.nprograms_;
    info.ntracks_ = video_.size();
    info.index_ = selectedVideoTrack_;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      const VideoTrackPtr & track = video_[info.index_];
      info.setLang(track->getLang());
      info.setName(track->getName());

      const std::string & track_id = track->id();
      info.program_ = yae::get(prog_lut_, track_id);

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getSelectedAudioTrackInfo
  //
  bool
  DemuxerReader::getSelectedAudioTrackInfo(TTrackInfo & info) const
  {
    info.nprograms_ = programs_.size();
    info.program_ = info.nprograms_;
    info.ntracks_ = audio_.size();
    info.index_ = selectedAudioTrack_;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      const AudioTrackPtr & track = audio_[info.index_];
      info.setLang(track->getLang());
      info.setName(track->getName());

      const std::string & track_id = track->id();
      info.program_ = yae::get(prog_lut_, track_id);

      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getVideoDuration
  //
  bool
  DemuxerReader::getVideoDuration(TTime & start, TTime & duration) const
  {
    VideoTrackPtr track = selectedVideoTrack();
    if (!track)
    {
      return false;
    }

    const std::string & track_id = track->id();
    const DemuxerSummary & summary = demuxer_->summary();
    const Timeline::Track & tt = summary.get_track_timeline(track_id);

    start = tt.pts_span_.front().t0_;
    duration = tt.pts_span_.back().t1_ - start;
    return true;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getAudioDuration
  //
  bool
  DemuxerReader::getAudioDuration(TTime & start, TTime & duration) const
  {
    AudioTrackPtr track = selectedAudioTrack();
    if (!track)
    {
      return false;
    }

    const std::string & track_id = track->id();
    const DemuxerSummary & summary = demuxer_->summary();
    const Timeline::Track & tt = summary.get_track_timeline(track_id);

    start = tt.pts_span_.front().t0_;
    duration = tt.pts_span_.back().t1_ - start;
    return true;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getVideoTraits
  //
  bool
  DemuxerReader::getVideoTraits(VideoTraits & traits) const
  {
    VideoTrackPtr track = selectedVideoTrack();
    if (!track)
    {
      return false;
    }

    return track->getTraits(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::getAudioTraits
  //
  bool
  DemuxerReader::getAudioTraits(AudioTraits & traits) const
  {
    AudioTrackPtr track = selectedAudioTrack();
    if (!track)
    {
      return false;
    }

    return track->getTraits(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::setAudioTraitsOverride
  //
  bool
  DemuxerReader::setAudioTraitsOverride(const AudioTraits & traits)
  {
    AudioTrackPtr track = selectedAudioTrack();
    if (!track)
    {
      return false;
    }

    return track->setTraitsOverride(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::setVideoTraitsOverride
  //
  bool
  DemuxerReader::setVideoTraitsOverride(const VideoTraits & traits)
  {
    VideoTrackPtr track = selectedVideoTrack();
    if (!track)
    {
      return false;
    }

    return track->setTraitsOverride(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::getAudioTraitsOverride
  //
  bool
  DemuxerReader::getAudioTraitsOverride(AudioTraits & traits) const
  {
    AudioTrackPtr track = selectedAudioTrack();
    if (!track)
    {
      return false;
    }

    return track->getTraitsOverride(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::getVideoTraitsOverride
  //
  bool
  DemuxerReader::getVideoTraitsOverride(VideoTraits & traits) const
  {
    VideoTrackPtr track = selectedVideoTrack();
    if (!track)
    {
      return false;
    }

    return track->getTraitsOverride(traits);
  }

  //----------------------------------------------------------------
  // DemuxerReader::isSeekable
  //
  bool
  DemuxerReader::isSeekable() const
  {
    return true;
  }

  //----------------------------------------------------------------
  // DemuxerReader::seek
  //
  bool
  DemuxerReader::seek(double seekTime)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      mustSeek_ = true;
      seekTime_ = seekTime;

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->packetQueueClear();
        do { videoTrack->frameQueueClear(); }
        while (!videoTrack->packetQueueWaitForConsumerToBlock(1e-2));
        videoTrack->frameQueueClear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::string ts = TTime(seekTime).to_hhmmss_ms();
        av_log(NULL, AV_LOG_DEBUG,
               "CLEAR VIDEO FRAME QUEUE for seek: %s",
               ts.c_str());
#endif
      }

      AudioTrackPtr audioTrack = selectedAudioTrack();
      if (audioTrack)
      {
        audioTrack->packetQueueClear();
        do { audioTrack->frameQueueClear(); }
        while (!audioTrack->packetQueueWaitForConsumerToBlock(1e-2));
        audioTrack->frameQueueClear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::string ts = TTime(seekTime).to_hhmmss_ms();
        av_log(NULL, AV_LOG_DEBUG,
               "CLEAR AUDIO FRAME QUEUE for seek: %s",
               ts.c_str());
#endif
      }

      return true;
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::readVideo
  //
  bool
  DemuxerReader::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    VideoTrackPtr track = selectedVideoTrack();
    if (!track)
    {
      return false;
    }

    bool ok = track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // DemuxerReader::readAudio
  //
  bool
  DemuxerReader::readAudio(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    AudioTrackPtr track = selectedAudioTrack();
    if (!track)
    {
      return false;
    }

    bool ok = track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // DemuxerReader::blockedOnVideo
  //
  bool
  DemuxerReader::blockedOnVideo() const
  {
    VideoTrackPtr videoTrack = selectedVideoTrack();
    AudioTrackPtr audioTrack = selectedAudioTrack();
    bool blocked = blockedOn(videoTrack.get(), audioTrack.get());

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      av_log(NULL, AV_LOG_DEBUG, "BLOCKED ON VIDEO");
    }
#endif

    return blocked;
  }

  //----------------------------------------------------------------
  // DemuxerReader::blockedOnAudio
  //
  bool
  DemuxerReader::blockedOnAudio() const
  {
    VideoTrackPtr videoTrack = selectedVideoTrack();
    AudioTrackPtr audioTrack = selectedAudioTrack();
    bool blocked = blockedOn(audioTrack.get(), videoTrack.get());

 #if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      av_log(NULL, AV_LOG_DEBUG, "BLOCKED ON AUDIO");
    }
#endif

    return blocked;
  }

  //----------------------------------------------------------------
  // DemuxerReader::threadStart
  //
  bool
  DemuxerReader::threadStart()
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);
      mustStop_ = false;
    }
    catch (...)
    {}

    VideoTrackPtr videoTrack = selectedVideoTrack();
    if (videoTrack)
    {
      videoTrack->threadStart();
      videoTrack->packetQueueWaitForConsumerToBlock();
    }

    AudioTrackPtr audioTrack = selectedAudioTrack();
    if (audioTrack)
    {
      audioTrack->threadStart();
      audioTrack->packetQueueWaitForConsumerToBlock();
    }

    outputTerminator_.stopWaiting(false);
    framestepTerminator_.stopWaiting(!playbackEnabled_);
    return thread_.run();
  }

  //----------------------------------------------------------------
  // DemuxerReader::threadStop
  //
  bool
  DemuxerReader::threadStop()
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);
      mustStop_ = true;
    }
    catch (...)
    {}

    VideoTrackPtr videoTrack = selectedVideoTrack();
    if (videoTrack)
    {
      videoTrack->threadStop();
    }

    AudioTrackPtr audioTrack = selectedAudioTrack();
    if (audioTrack)
    {
      audioTrack->threadStop();
    }

    outputTerminator_.stopWaiting(true);
    framestepTerminator_.stopWaiting(true);

    thread_.interrupt();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getPlaybackInterval
  //
  void
  DemuxerReader::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    timeIn = timeIn_;
    timeOut = timeOut_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setPlaybackIntervalStart
  //
  void
  DemuxerReader::setPlaybackIntervalStart(double timeIn)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      timeIn_ = timeIn;

      TSeekPosPtr posIn(new TimePos(timeIn_));
      TSeekPosPtr posOut(new TimePos(timeOut_));

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
      }

      AudioTrackPtr audioTrack = selectedAudioTrack();
      if (audioTrack)
      {
        audioTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::setPlaybackIntervalEnd
  //
  void
  DemuxerReader::setPlaybackIntervalEnd(double timeOut)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      timeOut_ = timeOut;

      TSeekPosPtr posIn(new TimePos(timeIn_));
      TSeekPosPtr posOut(new TimePos(timeOut_));

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
      }

      AudioTrackPtr audioTrack = selectedAudioTrack();
      if (audioTrack)
      {
        audioTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::setPlaybackEnabled
  //
  void
  DemuxerReader::setPlaybackEnabled(bool enabled)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      playbackEnabled_ = enabled;
      clock_.setRealtime(playbackEnabled_);
      framestepTerminator_.stopWaiting(!playbackEnabled_);

      if (playbackEnabled_ && looping_)
      {
        TSeekPosPtr posIn(new TimePos(timeIn_));
        TSeekPosPtr posOut(new TimePos(timeOut_));

        VideoTrackPtr videoTrack = selectedVideoTrack();
        if (videoTrack)
        {
          videoTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
        }

        AudioTrackPtr audioTrack = selectedAudioTrack();
        if (audioTrack)
        {
          audioTrack->setPlaybackInterval(posIn, posOut, playbackEnabled_);
        }
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::setPlaybackLooping
  //
  void
  DemuxerReader::setPlaybackLooping(bool enabled)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      looping_ = enabled;
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::skipLoopFilter
  //
  void
  DemuxerReader::skipLoopFilter(bool skip)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      skipLoopFilter_ = skip;

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->skipLoopFilter(skipLoopFilter_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::skipNonReferenceFrames
  //
  void
  DemuxerReader::skipNonReferenceFrames(bool skip)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      skipNonReferenceFrames_ = skip;

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->skipNonReferenceFrames(skipNonReferenceFrames_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::setTempo
  //
  bool
  DemuxerReader::setTempo(double tempo)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      // first set audio tempo -- this may fail:
      AudioTrackPtr audioTrack = selectedAudioTrack();
      if (audioTrack && !audioTrack->setTempo(tempo))
      {
        return false;
      }

      // then set video tempo -- this can't fail:
      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        return videoTrack->setTempo(tempo);
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setDeinterlacing
  //
  bool
  DemuxerReader::setDeinterlacing(bool enabled)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        return videoTrack->setDeinterlacing(enabled);
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setRenderCaptions
  //
  void
  DemuxerReader::setRenderCaptions(unsigned int cc)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      enableClosedCaptions_ = cc;

      VideoTrackPtr videoTrack = selectedVideoTrack();
      if (videoTrack)
      {
        videoTrack->enableClosedCaptions(cc);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // DemuxerReader::getRenderCaptions
  //
  unsigned int
  DemuxerReader::getRenderCaptions() const
  {
    return enableClosedCaptions_;
  }

  //----------------------------------------------------------------
  // DemuxerReader::subsCount
  //
  std::size_t
  DemuxerReader::subsCount() const
  {
    return subtt_.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::subsInfo
  //
  TSubsFormat
  DemuxerReader::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    info.nprograms_ = programs_.size();
    info.program_ = info.nprograms_;
    info.ntracks_ = subtt_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      const SubttTrackPtr & track = subtt_[i];
      info.setLang(track->getLang());
      info.setName(track->getName());

      const std::string & track_id = track->id();
      info.program_ = yae::get(prog_lut_, track_id);

      return track->format_;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setSubsRender
  //
  void
  DemuxerReader::setSubsRender(std::size_t i, bool render)
  {
    SubttTrackPtr subtt = subttTrack(i);
    if (subtt)
    {
      SubtitlesTrack & subs = *subtt;
      subs.render_ = render;
    }
  }

  //----------------------------------------------------------------
  // DemuxerReader::getSubsRender
  //
  bool
  DemuxerReader::getSubsRender(std::size_t i) const
  {
    SubttTrackPtr subtt = subttTrack(i);
    if (subtt)
    {
      const SubtitlesTrack & subs = *subtt;
      return subs.render_;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::countChapters
  //
  std::size_t
  DemuxerReader::countChapters() const
  {
    return chapters_.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getChapterInfo
  //
  bool
  DemuxerReader::getChapterInfo(std::size_t i, TChapter & c) const
  {
    if (i < chapters_.size())
    {
      c = chapters_[i];
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // DemuxerReader::getNumberOfAttachments
  //
  std::size_t
  DemuxerReader::getNumberOfAttachments() const
  {
    const DemuxerSummary & summary = demuxer_->summary();
    const std::vector<TAttachment> & attachments = summary.attachments_;
    return attachments.size();
  }

  //----------------------------------------------------------------
  // DemuxerReader::getAttachmentInfo
  //
  const TAttachment *
  DemuxerReader::getAttachmentInfo(std::size_t i) const
  {
    const DemuxerSummary & summary = demuxer_->summary();
    const std::vector<TAttachment> & attachments = summary.attachments_;
    return (i < attachments.size()) ? &attachments[i] : NULL;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setReaderId
  //
  void
  DemuxerReader::setReaderId(unsigned int readerId)
  {
    readerId_ = readerId;
  }

  //----------------------------------------------------------------
  // DemuxerReader::setSharedClock
  //
  void
  DemuxerReader::setSharedClock(const SharedClock & clock)
  {
    boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
    requestMutex(lock);

    clock_.cancelWaitForOthers();
    clock_ = clock;
    clock_.setMasterClock(clock_);
    clock_.setRealtime(playbackEnabled_);
  }

  //----------------------------------------------------------------
  // DemuxerReader::requestMutex
  //
  void
  DemuxerReader::requestMutex(boost::unique_lock<boost::timed_mutex> & lk)
  {
    while (!lk.timed_lock(boost::posix_time::milliseconds(1000)))
    {
      // demuxer_->requestDemuxerInterrupt();
      YAE_ASSERT(false);
      boost::this_thread::yield();
    }
  }

  //----------------------------------------------------------------
  // DemuxerReader::seekTo
  //
  int
  DemuxerReader::seekTo(double seekTime, bool dropPendingFrames)
  {
    TTime t(seekTime);

    AudioTrackPtr audioTrack = selectedAudioTrack();
    VideoTrackPtr videoTrack = selectedVideoTrack();

    std::string track_id =
      videoTrack ? videoTrack->id() :
      audioTrack ? audioTrack->id() :
      std::string();

    const DemuxerSummary & summary = demuxer_->summary();

    int seekFlags = 0;
    int err = demuxer_->seek(seekFlags, t, track_id);

    if (err < 0)
    {
      if (seekTime <= 0.0)
      {
        // must be trying to rewind a stream of undefined duration:
        seekFlags |= AVSEEK_FLAG_BYTE;
      }

      err = demuxer_->seek(seekFlags | AVSEEK_FLAG_ANY, t, track_id);
    }

    if (err < 0)
    {
#ifndef NDEBUG
      av_log(NULL, AV_LOG_DEBUG,
             "avformat_seek_file (%s) returned %s (%i)",
             TTime(seekTime).to_hhmmss_ms().c_str(),
             av_errstr(err).c_str(),
             err);
#endif

      return err;
    }

    TSeekPosPtr seekPos(new TimePos(seekTime));

    if (videoTrack)
    {
      err = videoTrack->resetTimeCounters(seekPos, dropPendingFrames);
    }

    if (!err && audioTrack)
    {
      err = audioTrack->resetTimeCounters(seekPos, dropPendingFrames);
    }

    clock_.cancelWaitForOthers();
    clock_.resetCurrentTime();

    const std::size_t nsubs = subtt_.size();
    for (std::size_t i = 0; i < nsubs; i++)
    {
      SubtitlesTrack & subs = *(subtt_[i]);
      subs.clear();
    }

    return err;
  }

  //----------------------------------------------------------------
  // DemuxerReader::rewind
  //
  int
  DemuxerReader::rewind(const AudioTrackPtr & audioTrack,
                        const VideoTrackPtr & videoTrack,
                        bool seekToTimeIn)
  {
    // wait for the the frame queues to empty out:
    if (audioTrack)
    {
      audioTrack->packetQueueWaitForConsumerToBlock();
      audioTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
    }

    if (videoTrack)
    {
      videoTrack->packetQueueWaitForConsumerToBlock();
      videoTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
    }

    boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
    requestMutex(lock);

    if (mustStop_)
    {
      // user must have switched to another item in the playlist:
      return AVERROR(EAGAIN);
    }

    double seekTime = seekToTimeIn ? timeIn_ : 0.0;
    bool dropPendingFrames = false;
    return seekTo(seekTime, dropPendingFrames);
  }

  //----------------------------------------------------------------
  // DemuxerReader::thread_loop
  //
  void
  DemuxerReader::thread_loop()
  {
    VideoTrackPtr videoTrack = selectedVideoTrack();
    AudioTrackPtr audioTrack = selectedAudioTrack();

    PacketQueueCloseOnExit videoCloseOnExit(videoTrack);
    PacketQueueCloseOnExit audioCloseOnExit(audioTrack);

    try
    {
      int err = 0;
      while (!err)
      {
        boost::this_thread::interruption_point();

        // check whether it's time to rewind to the in-point:
        bool mustRewind = true;

        if (audioTrack && audioTrack->discarded_ < 1)
        {
          mustRewind = false;
        }
        else if (videoTrack && videoTrack->discarded_ < 3)
        {
          mustRewind = false;
        }

        if (mustRewind)
        {
          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
          }
          else
          {
            break;
          }
        }

        // service seek request, read a packet:
        AVStream * stream = NULL;
        TPacketPtr packetPtr;
        {
          boost::lock_guard<boost::timed_mutex> lock(mutex_);

          if (mustStop_)
          {
            break;
          }

          if (mustSeek_)
          {
            bool dropPendingFrames = true;
            err = seekTo(seekTime_, dropPendingFrames);
            mustSeek_ = false;
          }

          if (!err)
          {
            packetPtr = demuxer_->get(stream);
            if (!packetPtr)
            {
              err = AVERROR_EOF;
            }
          }
        }

        if (err)
        {
          if (!playbackEnabled_ && err == AVERROR_EOF)
          {
            // avoid constantly rewinding when playback is paused,
            // slow down a little:
            boost::this_thread::interruption_point();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(333));
          }
#ifndef NDEBUG
          else
          {
            av_log(NULL, AV_LOG_DEBUG,
                   "DemuxerReader::thread_loop, %s (%i)",
                   av_errstr(err).c_str(), err);
          }
#endif

          if (err != AVERROR_EOF)
          {
            // keep trying, it may be able recover:
            err = 0;
            continue;
          }

          if (audioTrack)
          {
            // flush out buffered frames with an empty packet:
            audioTrack->packetQueuePush(TPacketPtr(), &outputTerminator_);
          }

          if (videoTrack)
          {
            // flush out buffered frames with an empty packet:
            videoTrack->packetQueuePush(TPacketPtr(), &outputTerminator_);
          }

          if (!playbackEnabled_)
          {
            // during framestep do not stop when end of file is reached,
            // simply rewind to the beginning:
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          // it appears playback has finished, unless user starts
          // framestep (playback disabled) while this thread waits
          // for all queues to empty:
          if (audioTrack)
          {
            audioTrack->packetQueueWaitForConsumerToBlock();
            audioTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
          }

          if (videoTrack)
          {
            videoTrack->packetQueueWaitForConsumerToBlock();
            videoTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
          }

          // check whether user disabled playback while
          // we were waiting for the queues:
          if (!playbackEnabled_)
          {
            // during framestep do not stop when end of file is reached,
            // simply rewind to the beginning:
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          // check whether user enabled playback looping while
          // we were waiting for the queues:
          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          break;
        }

        // shortcut:
        AVPacket & packet = packetPtr->get();

        if (videoTrack &&
            videoTrack->streamIndex() == packet.stream_index)
        {
          if (!videoTrack->packetQueuePush(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else if (audioTrack &&
                 audioTrack->streamIndex() == packet.stream_index)
        {
          if (!audioTrack->packetQueuePush(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else
        {
          bool closedCaptions = false;
          if (stream)
          {
            if (stream->codecpar->codec_tag == c608)
            {
              // convert to CEA708 packets wrapping CEA608 data, it's
              // the only format ffmpeg captions decoder understands:
              closedCaptions = convert_quicktime_c608(packet);
            }
            else if (stream->codecpar->codec_tag == c708)
            {
              // convert to CEA708 packets wrapping CEA608 data, it's
              // the only format ffmpeg captions decoder understands:
              closedCaptions = convert_quicktime_c708(packet);
            }
          }

          TrackPtr decoder =
            packetPtr->demuxer_->getTrack(packetPtr->trackId_);

          SubttTrackPtr subttTrack =
            boost::dynamic_pointer_cast<SubtitlesTrack, Track>(decoder);

          SubtitlesTrack * subs = subttTrack.get();

          if (stream && videoTrack && (closedCaptions || subs))
          {
            static const Rational tb(1, AV_TIME_BASE);

            // shortcut:
            AVCodecContext * subsDec = subs ? subs->codecContext() : NULL;

            TSubsFrame sf;
            sf.time_.time_ = av_rescale_q(packet.pts,
                                          stream->time_base,
                                          tb);
            sf.time_.base_ = AV_TIME_BASE;
            sf.tEnd_ = TTime(std::numeric_limits<int64>::max(), AV_TIME_BASE);

            if (subs)
            {
              sf.render_ = subs->render_;
              sf.traits_ = subs->format_;
              sf.extraData_ = subs->extraData_;
            }
            else
            {
              sf.traits_ = kSubsCEA608;
            }

            // copy the reference frame size:
            if (subsDec)
            {
              sf.rw_ = subsDec->width;
              sf.rh_ = subsDec->height;
            }

            if (subs && subs->format_ == kSubsDVD && !(sf.rw_ && sf.rh_))
            {
              sf.rw_ = subs->vobsub_.w_;
              sf.rh_ = subs->vobsub_.h_;
            }

            if (packet.data && packet.size)
            {
              TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                      &IPlanarBuffer::deallocator);
              buffer->resize(0, packet.size, 1);
              unsigned char * dst = buffer->data(0);
              memcpy(dst, packet.data, packet.size);

              sf.data_ = buffer;
            }

            for (int i = 0; i < packet.side_data_elems; i++)
            {
              const AVPacketSideData & side_data = packet.side_data[i];
              if (side_data.type == AV_PKT_DATA_MPEGTS_STREAM_ID)
              {
                continue;
              }

              TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                      &IPlanarBuffer::deallocator);
              buffer->resize(0, side_data.size, 1, 1);
              unsigned char * dst = buffer->data(0);
              memcpy(dst, side_data.data, side_data.size);

              sf.sideData_[side_data.type].push_back(buffer);
            }

            if (subsDec)
            {
              // decode the subtitle:
              int gotSub = 0;
              AVSubtitle sub;
              err = avcodec_decode_subtitle2(subsDec,
                                             &sub,
                                             &gotSub,
                                             &packet);

              if (err >= 0 && gotSub)
              {
                const uint8_t * hdr = subsDec->subtitle_header;
                const std::size_t sz = subsDec->subtitle_header_size;
                sf.private_ = TSubsPrivatePtr(new TSubsPrivate(sub, hdr, sz),
                                              &TSubsPrivate::deallocator);

                static const Rational tb_msec(1, 1000);

                if (packet.pts != AV_NOPTS_VALUE)
                {
                  sf.time_.time_ = av_rescale_q(packet.pts,
                                                stream->time_base,
                                                tb);

                  sf.time_.time_ += av_rescale_q(sub.start_display_time,
                                                 tb_msec,
                                                 tb);
                }

                if (packet.pts != AV_NOPTS_VALUE &&
                    sub.end_display_time > sub.start_display_time)
                {
                  double dt =
                    double(sub.end_display_time - sub.start_display_time) *
                    double(tb_msec.num) /
                    double(tb_msec.den);

                  // avoid subs that are visible for more than 5 seconds:
                  if (dt > 0.5 && dt < 5.0)
                  {
                    sf.tEnd_ = sf.time_;
                    sf.tEnd_ += dt;
                  }
                }

                subs->addTimingEtc(sf);
              }

              err = 0;
            }
            else if (closedCaptions)
            {
              // let the captions decoder handle it:
              videoTrack->cc_.decode(stream->time_base,
                                     packet,
                                     &outputTerminator_);
            }

            if (subs)
            {
              sf.trackId_ = subs->Track::id();
              subs->push(sf, &outputTerminator_);
            }
          }
        }
      }
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      av_log(NULL, AV_LOG_DEBUG,
             "DemuxerReader::thread_loop caught exception: %s",
             e.what());
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      av_log(NULL, AV_LOG_DEBUG,
             "DemuxerReader::thread_loop caught unexpected exception");
#endif
    }

#if 0 // ndef NDEBUG
    av_log(NULL, AV_LOG_DEBUG, "DemuxerReader::thread_loop terminated");
#endif
  }

  //----------------------------------------------------------------
  // make_yaerx_reader
  //
  DemuxerReaderPtr
  make_yaerx_reader(const std::string & yaerx_path, bool hwdec)
  {
    TSerialDemuxerPtr serial_demuxer = load_yaerx(yaerx_path);
    DemuxerReaderPtr reader_ptr(DemuxerReader::create(serial_demuxer, hwdec));
    return reader_ptr;
  }

}

