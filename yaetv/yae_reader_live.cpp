// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jan 26 17:09:43 MST 2020
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


// standard:
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdio.h>
#include <typeinfo>

// aeyae:
#include "yae/api/yae_settings.h"
#include "yae/ffmpeg/yae_movie.h"
#include "yae/thread/yae_queue.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// local:
#include "yae_dvr.h"
#include "yae_reader_live.h"


//----------------------------------------------------------------
// YAE_READER_LIVE_GUID
//
#define YAE_READER_LIVE_GUID "4A16794E-FEF7-483D-BFC9-F03A1FBCB35C"


namespace yae
{

  //----------------------------------------------------------------
  // ReaderLive::Private
  //
  class ReaderLive::Private
  {
  private:
    // intentionally disabled:
    Private(const Private &);
    Private & operator = (const Private &);

  public:
    Private();

    bool open(const std::string & filepath);
    bool updateTimelinePositions();
    void calcTimeline(TTime & start, TTime & duration);
    uint64_t calcPosition(double t) const;

    void getPlaybackInterval(double & timeIn, double & timeOut) const;
    void setPlaybackIntervalStart(double timeIn);
    void setPlaybackIntervalEnd(double timeOut);

    Movie movie_;
    unsigned int readerId_;
    double timeIn_;
    double timeOut_;

    std::string filepath_;
    TRecordingPtr rec_;
    TOpenFilePtr dat_;

    // protect against concurrent access:
    mutable boost::mutex mutex_;
    std::vector<uint64_t> walltime_;
    std::vector<uint64_t> filesize_;

    //----------------------------------------------------------------
    // Seg
    //
    struct Seg
    {
      Seg(std::size_t i = 0, std::size_t n = 0):
        i_(0),
        n_(0)
      {}

      inline uint64_t t0(const Private & ctx) const
      { return n_ ? ctx.walltime_.at(i_) : 0; }

      inline uint64_t t1(const Private & ctx) const
      { return n_ ? ctx.walltime_.at(i_ + n_ - 1) : 0; }

      inline uint64_t p0(const Private & ctx) const
      { return n_ ? ctx.filesize_.at(i_) : 0; }

      inline uint64_t p1(const Private & ctx) const
      { return n_ ? ctx.filesize_.at(i_ + n_ - 1) : 0; }

      inline uint64_t seconds(const Private & ctx) const
      { return t1(ctx) - t0(ctx); }

      inline uint64_t bytes(const Private & ctx) const
      { return p1(ctx) - p0(ctx); }

      double bytes_per_sec(const Private & ctx) const;

      std::size_t i_;
      std::size_t n_;
    };

    // disconts cause timeline gaps, so we'll be
    // keeping track of the contiguous segments:
    std::vector<Seg> segments_;
  };

  //----------------------------------------------------------------
  // ReaderLive::Private::Private
  //
  ReaderLive::Private::Private():
      readerId_((unsigned int)~0),
      timeIn_(0.0),
      timeOut_(kMaxDouble)
  {}

  //----------------------------------------------------------------
  // ReaderLive::Private::Seg::bytes_per_sec
  //
  double
  ReaderLive::Private::Seg::bytes_per_sec(const Private & ctx) const
  {
    if (n_ < 2)
    {
      return 0;
    }

    std::size_t i1 = i_ + n_ - 1;
    uint64_t dv = ctx.filesize_[i1] - ctx.filesize_[i_];
    uint64_t dt = ctx.walltime_[i1] - ctx.walltime_[i_];
    double bytes_per_sec = double(dv) / double(dt);
    return bytes_per_sec;
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::open
  //
  bool
  ReaderLive::Private::open(const std::string & filepath)
  {
    std::string folder;
    std::string fn_ext;
    if (!parse_file_path(filepath, folder, fn_ext))
    {
      return false;
    }

    std::string basename;
    std::string suffix;
    if (!parse_file_name(fn_ext, basename, suffix))
    {
      return false;
    }

    fs::path path(folder);
    std::string fn_rec = (path / (basename + ".json")).string();
    std::string fn_dat = (path / (basename + ".dat")).string();

    try
    {
      Json::Value json;
      yae::TOpenFile(fn_rec, "rb").load(json);
      rec_.reset(new Recording());
      yae::load(json, *rec_);
    }
    catch (...)
    {
      // return false;
    }

    dat_.reset(new TOpenFile(fn_dat, "rb"));
    if (!dat_->is_open())
    {
      return false;
    }

    filepath_ = filepath;
    segments_.clear();

    return movie_.open(filepath.c_str());
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::updateTimelinePositions
  //
  bool
  ReaderLive::Private::updateTimelinePositions()
  {
    if (!dat_)
    {
      return false;
    }

    // shortcut:
    TOpenFile & dat = *dat_;

    // load walltime:filesize pairs:
    Data buffer(16);
    Bitstream bs(buffer);
    bool updated = false;

    // clear the EOF indicator:
    clearerr(dat.file_);

    // read until EOF:
    int err = 0;

    while (dat.is_open() && !dat.is_eof())
    {
      uint64_t pos = ftell64(dat.file_);

      YAE_ASSERT(pos % 16 == 0);
      if (pos % 16 != 0)
      {
        pos -= pos % 16;
        err = fseek64(dat.file_, pos, SEEK_SET);
        YAE_ASSERT(!err);
      }

      std::size_t nbytes = dat.read(buffer.get(), buffer.size());
      if (nbytes != 16)
      {
        err = fseek64(dat.file_, pos, SEEK_SET);
        YAE_ASSERT(!err);
        break;
      }

      bs.seek(0);
      uint64_t walltime = bs.read_bits(64);
      uint64_t filesize = bs.read_bits(64);

      YAE_ASSERT(walltime_.empty() ||
                 (walltime_.back() < walltime &&
                  filesize_.back() <= filesize));

      if (segments_.empty())
      {
        segments_.push_back(Seg());
      }

      // shortcut:
      Seg * segment = &(segments_.back());

      if (segment->n_)
      {
        uint64_t dt = walltime - walltime_.back();
        uint64_t dv = filesize - filesize_.back();

        YAE_ASSERT(dt);
        if (!dt)
        {
          continue;
        }

        // check for disconts:
        double avg_bytes_per_sec = segment->bytes_per_sec(*this);

        if (avg_bytes_per_sec > 0.0)
        {
          double bytes_per_sec = double(dv) / double(dt);

          double r =
            (1.0 + avg_bytes_per_sec - bytes_per_sec) /
            (1.0 + avg_bytes_per_sec);

          if (r > 0.5)
          {
            // discont, instantaneous bitrate is less than half of avg bitrate:
            segments_.push_back(Seg(walltime_.size()));

            // update the shortcut:
            segment = &(segments_.back());
          }
        }
      }

      walltime_.push_back(walltime);
      filesize_.push_back(filesize);
      segment->n_++;

      updated = true;
    }

    return updated;
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::calcTimeline
  //
  void
  ReaderLive::Private::calcTimeline(TTime & start, TTime & duration)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    updateTimelinePositions();

    if (walltime_.empty())
    {
      start.reset(0, 0); // invalid time
      duration.reset(0, 1); // zero duration
    }
    else
    {
      start = walltime_.front();
      duration = walltime_.back() - walltime_.front();
    }

    // FIXME: consider timespan of the Recording (as scheduled),
    // and possibly adjust the start time to the scheduled start time
    // (unless Recording started late ... usually they start 30-60s early):
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::calcPosition
  //
  uint64_t
  ReaderLive::Private::calcPosition(double t) const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);

    // find the segment that corresponds to the given position:
    for (std::size_t i = 0, n = segments_.size(); i < n; i++)
    {
      const Seg & segment = segments_[i];
      uint64_t t0 = segment.t0(*this);
      uint64_t t1 = segment.t1(*this);
      uint64_t dt = t1 - t0;

      if (dt > 0 && t <= double(t1))
      {
        double s = std::max(0.0, t - double(t0)) / double(dt);
        double offset = double(segment.bytes(*this) * s);

        // make sure packet position is a multiple of 188
        // to maintain alignment with TS packet boundaries:
        uint64_t pos = segment.p0(*this) + uint64_t(offset);
        pos -= pos % 188;
        return pos;
      }
    }

    if (segments_.empty())
    {
      return 0;
    }

    // extrapolate beyond last segment:
    const Seg & segment = segments_.back();
    double byterate = segment.bytes_per_sec(*this);
    double t1 = double(segment.t1(*this));
    double p = double(segment.p1(*this)) + (t - t1) * byterate;
    p = std::min<double>(p, std::numeric_limits<uint64_t>::max());

    uint64_t pos = uint64_t(p);
    pos -= pos % 188;
    return pos;
  }


  //----------------------------------------------------------------
  // ReaderLive::Private::getPlaybackInterval
  //
  void
  ReaderLive::Private::getPlaybackInterval(double & timeIn,
                                           double & timeOut) const
  {
    timeIn = timeIn_;
    timeOut = timeOut_;
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::setPlaybackIntervalEnd
  //
  void
  ReaderLive::Private::setPlaybackIntervalStart(double timeIn)
  {
    timeIn_ = timeIn;

    TBytePosPtr pos(new BytePos(calcPosition(timeIn_)));
    movie_.setPlaybackIntervalStart(pos);
  }

  //----------------------------------------------------------------
  // ReaderLive::Private::setPlaybackIntervalEnd
  //
  void
  ReaderLive::Private::setPlaybackIntervalEnd(double timeOut)
  {
    timeOut_ = timeOut;

    TBytePosPtr pos(new BytePos(calcPosition(timeOut_)));
    movie_.setPlaybackIntervalEnd(pos);
  }


  //----------------------------------------------------------------
  // ReaderLive::ReaderLive
  //
  ReaderLive::ReaderLive():
    IReader(),
    private_(new ReaderLive::Private())
  {}

  //----------------------------------------------------------------
  // ReaderLive::~ReaderLive
  //
  ReaderLive::~ReaderLive()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ReaderLive::create
  //
  ReaderLive *
  ReaderLive::create()
  {
    return new ReaderLive();
  }

  //----------------------------------------------------------------
  // ReaderLive::destroy
  //
  void
  ReaderLive::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // ReaderLive::clone
  //
  ReaderLive *
  ReaderLive::clone() const
  {
    return ReaderLive::create();
  }

  //----------------------------------------------------------------
  // ReaderLive::name
  //
  const char *
  ReaderLive::name() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // ReaderLive::kGUID
  //
  const char *
  ReaderLive::kGUID = YAE_READER_LIVE_GUID;

  //----------------------------------------------------------------
  // ReaderLive::guid
  //
  const char *
  ReaderLive::guid() const
  {
    return ReaderLive::kGUID;
  }

  //----------------------------------------------------------------
  // ReaderLive::settings
  //
  ISettingGroup *
  ReaderLive::settings()
  {
    return private_->movie_.settings();
  }

  //----------------------------------------------------------------
  // ReaderLive::getUrlProtocols
  //
  bool
  ReaderLive::getUrlProtocols(std::list<std::string> & protocols) const
  {
    return private_->movie_.getUrlProtocols(protocols);
  }

  //----------------------------------------------------------------
  // ReaderLive::open
  //
  bool
  ReaderLive::open(const char * resourcePathUTF8)
  {
    return private_->open(std::string(resourcePathUTF8));
  }

  //----------------------------------------------------------------
  // ReaderLive::close
  //
  void
  ReaderLive::close()
  {
    return private_->movie_.close();
  }

  //----------------------------------------------------------------
  // ReaderLive::getNumberOfPrograms
  //
  std::size_t
  ReaderLive::getNumberOfPrograms() const
  {
    // FIXME: pkoshevoy: use recording info .json

    const std::vector<TProgramInfo> & progs = private_->movie_.getPrograms();
    const std::size_t nprogs = progs.size();
    return nprogs;
  }

  //----------------------------------------------------------------
  // ReaderLive::getProgramInfo
  //
  bool
  ReaderLive::getProgramInfo(std::size_t i, TProgramInfo & info) const
  {
    // FIXME: pkoshevoy: use recording info .json

    const std::vector<TProgramInfo> & progs = private_->movie_.getPrograms();
    const std::size_t nprogs = progs.size();

    if (nprogs <= i)
    {
      return false;
    }

    info = progs[i];
    return true;
  }

  //----------------------------------------------------------------
  // ReaderLive::getNumberOfVideoTracks
  //
  std::size_t
  ReaderLive::getNumberOfVideoTracks() const
  {
    // FIXME: pkoshevoy: use recording info .json

    return private_->movie_.getVideoTracks().size();
  }

  //----------------------------------------------------------------
  // ReaderLive::getNumberOfAudioTracks
  //
  std::size_t
  ReaderLive::getNumberOfAudioTracks() const
  {
    // FIXME: pkoshevoy: use recording info .json

    return private_->movie_.getAudioTracks().size();
  }

  //----------------------------------------------------------------
  // ReaderLive::getSelectedVideoTrackIndex
  //
  std::size_t
  ReaderLive::getSelectedVideoTrackIndex() const
  {
    return private_->movie_.getSelectedVideoTrack();
  }

  //----------------------------------------------------------------
  // ReaderLive::getSelectedAudioTrackIndex
  //
  std::size_t
  ReaderLive::getSelectedAudioTrackIndex() const
  {
    return private_->movie_.getSelectedAudioTrack();
  }

  //----------------------------------------------------------------
  // ReaderLive::selectVideoTrack
  //
  bool
  ReaderLive::selectVideoTrack(std::size_t i)
  {
    return private_->movie_.selectVideoTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderLive::selectAudioTrack
  //
  bool
  ReaderLive::selectAudioTrack(std::size_t i)
  {
    return private_->movie_.selectAudioTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderLive::getSelectedVideoTrackName
  //
  void
  ReaderLive::getSelectedVideoTrackInfo(TTrackInfo & info) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    private_->movie_.getVideoTrackInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderLive::getSelectedAudioTrackInfo
  //
  void
  ReaderLive::getSelectedAudioTrackInfo(TTrackInfo & info) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    private_->movie_.getAudioTrackInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderLive::getVideoDuration
  //
  bool
  ReaderLive::getVideoDuration(TTime & start, TTime & duration) const
  {
    private_->calcTimeline(start, duration);
    return true;
  }

  //----------------------------------------------------------------
  // ReaderLive::getAudioDuration
  //
  bool
  ReaderLive::getAudioDuration(TTime & start, TTime & duration) const
  {
    private_->calcTimeline(start, duration);
    return true;
  }

  //----------------------------------------------------------------
  // ReaderLive::getVideoTraits
  //
  bool
  ReaderLive::getVideoTraits(VideoTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderLive::getAudioTraits
  //
  bool
  ReaderLive::getAudioTraits(AudioTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderLive::setAudioTraitsOverride
  //
  bool
  ReaderLive::setAudioTraitsOverride(const AudioTraits & override)
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
  // ReaderLive::setVideoTraitsOverride
  //
  bool
  ReaderLive::setVideoTraitsOverride(const VideoTraits & override)
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
  // ReaderLive::getAudioTraitsOverride
  //
  bool
  ReaderLive::getAudioTraitsOverride(AudioTraits & override) const
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
  // ReaderLive::getVideoTraitsOverride
  //
  bool
  ReaderLive::getVideoTraitsOverride(VideoTraits & override) const
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
  // ReaderLive::isSeekable
  //
  bool
  ReaderLive::isSeekable() const
  {
    return private_->movie_.isSeekable() && private_->movie_.hasDuration();
  }

  //----------------------------------------------------------------
  // ReaderLive::seek
  //
  bool
  ReaderLive::seek(double seekTime)
  {
    TTimePosPtr pos(new TimePos(seekTime));
    return private_->movie_.requestSeek(pos);
  }

  //----------------------------------------------------------------
  // ReaderLive::readVideo
  //
  bool
  ReaderLive::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    // FIXME: pkoshevoy: remap frame time

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
  // ReaderLive::readAudio
  //
  bool
  ReaderLive::readAudio(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    // FIXME: pkoshevoy: remap frame time

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
  // ReaderLive::blockedOnVideo
  //
  bool
  ReaderLive::blockedOnVideo() const
  {
    return private_->movie_.blockedOnVideo();
  }

  //----------------------------------------------------------------
  // ReaderLive::blockedOnAudio
  //
  bool
  ReaderLive::blockedOnAudio() const
  {
    return private_->movie_.blockedOnAudio();
  }

  //----------------------------------------------------------------
  // ReaderLive::threadStart
  //
  bool
  ReaderLive::threadStart()
  {
    return private_->movie_.threadStart();
  }

  //----------------------------------------------------------------
  // ReaderLive::threadStop
  //
  bool
  ReaderLive::threadStop()
  {
    return private_->movie_.threadStop();
  }

  //----------------------------------------------------------------
  // ReaderLive::getPlaybackInterval
  //
  void
  ReaderLive::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    private_->getPlaybackInterval(timeIn, timeOut);
  }

  //----------------------------------------------------------------
  // ReaderLive::setPlaybackIntervalStart
  //
  void
  ReaderLive::setPlaybackIntervalStart(double timeIn)
  {
    private_->setPlaybackIntervalStart(private_->timeIn_);
  }

  //----------------------------------------------------------------
  // ReaderLive::setPlaybackIntervalEnd
  //
  void
  ReaderLive::setPlaybackIntervalEnd(double timeOut)
  {
    private_->setPlaybackIntervalEnd(private_->timeOut_);
  }

  //----------------------------------------------------------------
  // ReaderLive::setPlaybackEnabled
  //
  void
  ReaderLive::setPlaybackEnabled(bool enabled)
  {
    private_->movie_.setPlaybackEnabled(enabled);
  }

  //----------------------------------------------------------------
  // ReaderLive::setPlaybackLooping
  //
  void
  ReaderLive::setPlaybackLooping(bool enabled)
  {
    private_->movie_.setPlaybackLooping(enabled);
  }

  //----------------------------------------------------------------
  // ReaderLive::skipLoopFilter
  //
  void
  ReaderLive::skipLoopFilter(bool skip)
  {
    private_->movie_.skipLoopFilter(skip);
  }

  //----------------------------------------------------------------
  // ReaderLive::skipNonReferenceFrames
  //
  void
  ReaderLive::skipNonReferenceFrames(bool skip)
  {
    private_->movie_.skipNonReferenceFrames(skip);
  }

  //----------------------------------------------------------------
  // ReaderLive::setTempo
  //
  bool
  ReaderLive::setTempo(double tempo)
  {
    return private_->movie_.setTempo(tempo);
  }

  //----------------------------------------------------------------
  // ReaderLive::setDeinterlacing
  //
  bool
  ReaderLive::setDeinterlacing(bool enabled)
  {
    return private_->movie_.setDeinterlacing(enabled);
  }

  //----------------------------------------------------------------
  // ReaderLive::setRenderCaptions
  //
  void
  ReaderLive::setRenderCaptions(unsigned int cc)
  {
    private_->movie_.setRenderCaptions(cc);
  }

  //----------------------------------------------------------------
  // ReaderLive::getRenderCaptions
  //
  unsigned int
  ReaderLive::getRenderCaptions() const
  {
    return private_->movie_.getRenderCaptions();
  }

  //----------------------------------------------------------------
  // ReaderLive::subsCount
  //
  std::size_t
  ReaderLive::subsCount() const
  {
    return private_->movie_.subsCount();
  }

  //----------------------------------------------------------------
  // ReaderLive::subsInfo
  //
  TSubsFormat
  ReaderLive::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    return private_->movie_.subsInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderLive::setSubsRender
  //
  void
  ReaderLive::setSubsRender(std::size_t i, bool render)
  {
    private_->movie_.setSubsRender(i, render);
  }

  //----------------------------------------------------------------
  // ReaderLive::getSubsRender
  //
  bool
  ReaderLive::getSubsRender(std::size_t i) const
  {
    return private_->movie_.getSubsRender(i);
  }

  //----------------------------------------------------------------
  // ReaderLive::countChapters
  //
  std::size_t
  ReaderLive::countChapters() const
  {
    return private_->movie_.countChapters();
  }

  //----------------------------------------------------------------
  // ReaderLive::getChapterInfo
  //
  bool
  ReaderLive::getChapterInfo(std::size_t i, TChapter & c) const
  {
    return private_->movie_.getChapterInfo(i, c);
  }

  //----------------------------------------------------------------
  // ReaderLive::getNumberOfAttachments
  //
  std::size_t
  ReaderLive::getNumberOfAttachments() const
  {
    return private_->movie_.attachments().size();
  }

  //----------------------------------------------------------------
  // ReaderLive::getAttachmentInfo
  //
  const TAttachment *
  ReaderLive::getAttachmentInfo(std::size_t i) const
  {
    if (i < private_->movie_.attachments().size())
    {
      return &(private_->movie_.attachments()[i]);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // ReaderLive::setReaderId
  //
  void
  ReaderLive::setReaderId(unsigned int readerId)
  {
    private_->readerId_ = readerId;
  }

  //----------------------------------------------------------------
  // ReaderLive::setSharedClock
  //
  void
  ReaderLive::setSharedClock(const SharedClock & clock)
  {
    private_->movie_.setSharedClock(clock);
  }
}
