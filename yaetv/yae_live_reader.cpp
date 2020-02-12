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
#include "yae_live_reader.h"


//----------------------------------------------------------------
// YAE_LIVE_READER_GUID
//
#define YAE_LIVE_READER_GUID "4A16794E-FEF7-483D-BFC9-F03A1FBCB35C"


namespace yae
{

  //----------------------------------------------------------------
  // BytePos
  //
  struct BytePos : ISeekPos
  {
    BytePos(uint64_t pos, double sec);

    // virtual:
    std::string to_str() const;

    // virtual:
    bool lt(const TFrameBase & f, double dur = 0.0) const;
    bool gt(const TFrameBase & f, double dur = 0.0) const;

    // virtual:
    std::string to_str(const TFrameBase & f, double dur) const;

    // virtual:
    int seek(AVFormatContext * ctx, const AVStream * s) const;

    // bytes:
    uint64_t pos_;

    // seconds:
    double sec_;
  };

  //----------------------------------------------------------------
  // TBytePosPtr
  //
  typedef yae::shared_ptr<BytePos, ISeekPos> TBytePosPtr;


  //----------------------------------------------------------------
  // BytePos::BytePos
  //
  BytePos::BytePos(uint64_t pos, double sec):
    pos_(pos),
    sec_(sec)
  {}

  //----------------------------------------------------------------
  // BytePos::to_str
  //
  std::string
  BytePos::to_str() const
  {
    return yae::strfmt("offset %" PRIu64 " (%s)",
                       pos_,
                       TTime(sec_).to_hhmmss_ms().c_str());
  }

  //----------------------------------------------------------------
  // BytePos::lt
  //
  bool
  BytePos::lt(const TFrameBase & f, double dur) const
  {
    (void)dur;
    return sec_ < f.time_.sec();
  }

  //----------------------------------------------------------------
  // BytePos::gt
  //
  bool
  BytePos::gt(const TFrameBase & f, double dur) const
  {
    (void)dur;
    return f.time_.sec() < sec_;
  }

  //----------------------------------------------------------------
  // BytePos::to_str
  //
  std::string
  BytePos::to_str(const TFrameBase & f, double dur) const
  {
    return yae::strfmt("offset %" PRIu64 " (%s)",
                       f.pos_,
                       (f.time_ + dur).to_hhmmss_ms().c_str());
  }

  //----------------------------------------------------------------
  // BytePos::seek
  //
  int
  BytePos::seek(AVFormatContext * context, const AVStream * stream) const
  {
    int streamIndex = stream ? stream->index : -1;
    int seekFlags = AVSEEK_FLAG_BYTE;
    // seekFlags |= AVSEEK_FLAG_ANY;
    int err = avformat_seek_file(context,
                                 streamIndex,
                                 kMinInt64,
                                 pos_,
                                 pos_, // kMaxInt64,
                                 seekFlags);
    return err;
  }


  //----------------------------------------------------------------
  // LiveReader::Private
  //
  class LiveReader::Private
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
    TSeekPosPtr getPosition(double t);

    bool seek(double seekTime);
    void getPlaybackInterval(double & timeIn, double & timeOut) const;
    void setPlaybackIntervalStart(double timeIn);
    void setPlaybackIntervalEnd(double timeOut);

    void adjustTimestamps(AVFormatContext * ctx, AVPacket * pkt);

    static void
    adjustTimestampsCallback(void * readerPrivate,
                             AVFormatContext * ctx,
                             AVPacket * pkt);

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
        i_(i),
        n_(n)
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

    uint64_t sumTime_;
    uint64_t sumSize_;

    //----------------------------------------------------------------
    // ByteRange
    //
    struct ByteRange
    {
      ByteRange(std::size_t index = 0,
                uint64_t start_time = 0,
                uint64_t start_pos = 0,
                uint64_t end_pos = std::numeric_limits<uint64_t>::max()):
        ix_(index),
        t0_(start_time),
        p0_(start_pos),
        p1_(end_pos)
      {}

      std::size_t ix_;
      uint64_t t0_;
      uint64_t p0_;
      uint64_t p1_;
    };

    yae::TTime walltimeOffset_;
    std::vector<ByteRange> ranges_;
    std::size_t currRange_;
  };


  //----------------------------------------------------------------
  // LiveReader::Private::Private
  //
  LiveReader::Private::Private():
      readerId_((unsigned int)~0),
      timeIn_(TTime::min_flicks_as_sec()),
      timeOut_(TTime::max_flicks_as_sec()),
      sumTime_(0),
      sumSize_(0),
      walltimeOffset_(0, 1),
      currRange_(std::numeric_limits<std::size_t>::max())
  {
    movie_.setAdjustTimestamps(&adjustTimestampsCallback, this);
  }

  //----------------------------------------------------------------
  // LiveReader::Private::Seg::bytes_per_sec
  //
  double
  LiveReader::Private::Seg::bytes_per_sec(const Private & ctx) const
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
  // LiveReader::Private::open
  //
  bool
  LiveReader::Private::open(const std::string & filepath)
  {
    filepath_ = filepath;
    segments_.clear();
    ranges_.clear();
    sumTime_ = 0;
    sumSize_ = 0;

    std::string folder;
    std::string fn_ext;
    if (parse_file_path(filepath, folder, fn_ext))
    {
      std::string basename;
      std::string suffix;
      if (parse_file_name(fn_ext, basename, suffix))
      {
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
          dat_.reset();
        }
        else
        {
          updateTimelinePositions();
        }
      }
    }

    return movie_.open(filepath.c_str());
  }

  //----------------------------------------------------------------
  // LiveReader::Private::updateTimelinePositions
  //
  bool
  LiveReader::Private::updateTimelinePositions()
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
        ranges_.push_back(ByteRange(0, walltime, filesize));
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
        uint64_t seg_sz = filesize - segment->p0(*this);
        uint64_t seg_dt = walltime - segment->t0(*this);

        double avg_bytes_per_sec =
          double(sumSize_ + seg_sz) /
          double(sumTime_ + seg_dt);

        if (avg_bytes_per_sec > 0.0)
        {
          double bytes_per_sec = double(dv) / double(dt);
          double r = bytes_per_sec / avg_bytes_per_sec;

          if (r < 0.01 && dt > 3)
          {
            // discont, instantaneous bitrate is less than half of avg bitrate:
            ranges_.back().p1_ = filesize;
            ranges_.push_back(ByteRange(ranges_.size(), walltime, filesize));
            segments_.push_back(Seg(walltime_.size()));
            sumTime_ += seg_dt;
            sumSize_ += seg_sz;

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
  // LiveReader::Private::calcTimeline
  //
  void
  LiveReader::Private::calcTimeline(TTime & start, TTime & duration)
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
  // LiveReader::Private::getPosition
  //
  TSeekPosPtr
  LiveReader::Private::getPosition(double t)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    updateTimelinePositions();

    if (segments_.empty())
    {
      return TSeekPosPtr(new TimePos(t));
    }

    // find the segment that corresponds to the given position:
    for (std::size_t i = 0, n = segments_.size(); i < n; i++)
    {
      const Seg & segment = segments_[i];
      uint64_t t0 = segment.t0(*this);
      uint64_t t1 = segment.t1(*this);
      uint64_t dt = t1 - t0;

      if (dt > 0 && t <= double(t1))
      {
#if 0
        // linear approximation:
        double s = std::max(0.0, t - double(t0)) / double(dt);
        double offset = double(segment.bytes(*this) * s);

        // make sure packet position is a multiple of 188
        // to maintain alignment with TS packet boundaries:
        uint64_t pos = segment.p0(*this) + uint64_t(offset);
        pos -= pos % 188;
        return TSeekPosPtr(new BytePos(pos, t));
#else
        // binary search through walltime:
        std::size_t j0 = segment.i_;
        std::size_t j1 = segment.n_ + j0 - 1;
        uint64_t z = uint64_t(t);
        while (j0 + 1 < j1)
        {
          std::size_t j = (j0 + j1) >> 1;
          if (z <= walltime_[j])
          {
            j1 = j;
          }
          else
          {
            j0 = j;
          }
        }

        return (z == walltime_[j1] ?
                TSeekPosPtr(new BytePos(filesize_[j1], t)) :
                TSeekPosPtr(new BytePos(filesize_[j0], t)));
#endif
      }
    }

    // extrapolate beyond last segment:
    const Seg & segment = segments_.back();
    double byterate = segment.bytes_per_sec(*this);
    double t1 = double(segment.t1(*this));
    double p = double(segment.p1(*this)) + (t - t1) * byterate;
    p = std::min<double>(p, std::numeric_limits<uint64_t>::max());

    uint64_t pos = uint64_t(p);
    pos -= pos % 188;
    return TSeekPosPtr(new BytePos(pos, t));
  }

  //----------------------------------------------------------------
  // LiveReader::Private::seek
  //
  bool
  LiveReader::Private::seek(double seekTime)
  {
    TSeekPosPtr pos = getPosition(seekTime);
    return movie_.requestSeek(pos);
  }

  //----------------------------------------------------------------
  // LiveReader::Private::getPlaybackInterval
  //
  void
  LiveReader::Private::getPlaybackInterval(double & timeIn,
                                           double & timeOut) const
  {
    timeIn = timeIn_;
    timeOut = timeOut_;
  }

  //----------------------------------------------------------------
  // LiveReader::Private::setPlaybackIntervalEnd
  //
  void
  LiveReader::Private::setPlaybackIntervalStart(double timeIn)
  {
    timeIn_ = timeIn;

    TSeekPosPtr pos = getPosition(timeIn_);
    movie_.setPlaybackIntervalStart(pos);
  }

  //----------------------------------------------------------------
  // LiveReader::Private::setPlaybackIntervalEnd
  //
  void
  LiveReader::Private::setPlaybackIntervalEnd(double timeOut)
  {
    timeOut_ = timeOut;

    TSeekPosPtr pos = getPosition(timeOut_);
    movie_.setPlaybackIntervalEnd(pos);
  }

  //----------------------------------------------------------------
  // find_range
  //
  template <typename TByteRange>
  static const TByteRange *
  find_range(const TByteRange * r0, const TByteRange * r1, uint64_t pos)
  {
    if (r0 == r1)
    {
      YAE_ASSERT(pos < r1->p1_);
      return r0;
    }

    std::size_t n = r1 - r0 + 1;
    std::size_t i = n / 2;
    const TByteRange * ri = r0 + i;

    if (pos < ri->p0_)
    {
      return find_range<TByteRange>(r0, ri - 1, pos);
    }

    return find_range<TByteRange>(ri, r1, pos);
  }

  //----------------------------------------------------------------
  // LiveReader::Private::adjustTimestamps
  //
  void
  LiveReader::Private::adjustTimestamps(AVFormatContext * ctx, AVPacket * pkt)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (ranges_.empty() || pkt->dts == AV_NOPTS_VALUE)
    {
      return;
    }

    const AVStream * stream =
      pkt->stream_index < int(ctx->nb_streams) ?
      ctx->streams[pkt->stream_index] :
      NULL;

    if (!stream)
    {
      return;
    }

    const ByteRange * range =
      (currRange_ < ranges_.size()) ? &(ranges_.at(currRange_)) : NULL;

    if (!range || (pkt->pos != -1 &&
                   (pkt->pos < range->p0_ ||
                    range->p1_ <= pkt->pos)))
    {
      const ByteRange * r0 = &(ranges_.front());
      const ByteRange * r1 = &(ranges_.back());
      range = find_range(r0, r1, pkt->pos);
      currRange_ = range - r0;

      const Seg & segment = segments_[range->ix_];
      double byterate = segment.bytes_per_sec(*this);

      TTime dts(stream->time_base.num * pkt->dts,
                stream->time_base.den);

      double posErr = double(pkt->pos - range->p0_);
      double secErr = posErr / byterate;

      TTime walltime(range->t0_, 1);
      walltimeOffset_ = (dts - walltime) - TTime(secErr);

#if 0
      yae_wlog
        ("LiveReader"
         ": pkt = (%s, %12" PRIu64 ")"
         ", r0: r[%i] = (%s, %12" PRIu64 ")"
         ", r1: r[%i] = (%s, %12" PRIu64 ")"
         ", range: r[%i] = (%s, %12" PRIu64 ")"
         ", byterate %.3f"
         ", posErr %13.1f"
         ", secErr %.3f"
         ", walltimeOffset %s",
         dts.to_hhmmss_ms().c_str(),
         pkt->pos,
         r0 - r0, TTime(r0->t0_, 1).to_hhmmss_ms().c_str(), r0->p0_,
         r1 - r0, TTime(r1->t0_, 1).to_hhmmss_ms().c_str(), r1->p0_,
         range - r0, TTime(range->t0_, 1).to_hhmmss_ms().c_str(), range->p0_,
         byterate,
         posErr,
         secErr,
         walltimeOffset_.to_hhmmss_ms().c_str());
#endif
    }

    TTime dts(stream->time_base.num * pkt->dts,
              stream->time_base.den);
    dts -= walltimeOffset_;
    pkt->dts = av_rescale_q(dts.time_,
                            Rational(1, dts.base_),
                            stream->time_base);

    if (pkt->pts == AV_NOPTS_VALUE)
    {
      return;
    }

    TTime pts(stream->time_base.num * pkt->pts,
              stream->time_base.den);
    pts -= walltimeOffset_;
    pkt->pts = av_rescale_q(pts.time_,
                            Rational(1, pts.base_),
                            stream->time_base);
  }

  //----------------------------------------------------------------
  // LiveReader::Private::adjustTimestampsCallback
  //
  void
  LiveReader::Private::adjustTimestampsCallback(void * ctx,
                                                AVFormatContext * fmt,
                                                AVPacket * pkt)
  {
    LiveReader::Private * reader = (LiveReader::Private *)ctx;
    reader->adjustTimestamps(fmt, pkt);
  }


  //----------------------------------------------------------------
  // LiveReader::LiveReader
  //
  LiveReader::LiveReader():
    IReader(),
    private_(new LiveReader::Private())
  {}

  //----------------------------------------------------------------
  // LiveReader::~LiveReader
  //
  LiveReader::~LiveReader()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // LiveReader::create
  //
  LiveReader *
  LiveReader::create()
  {
    return new LiveReader();
  }

  //----------------------------------------------------------------
  // LiveReader::destroy
  //
  void
  LiveReader::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // LiveReader::clone
  //
  LiveReader *
  LiveReader::clone() const
  {
    return LiveReader::create();
  }

  //----------------------------------------------------------------
  // LiveReader::name
  //
  const char *
  LiveReader::name() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // LiveReader::kGUID
  //
  const char *
  LiveReader::kGUID = YAE_LIVE_READER_GUID;

  //----------------------------------------------------------------
  // LiveReader::guid
  //
  const char *
  LiveReader::guid() const
  {
    return LiveReader::kGUID;
  }

  //----------------------------------------------------------------
  // LiveReader::settings
  //
  ISettingGroup *
  LiveReader::settings()
  {
    return private_->movie_.settings();
  }

  //----------------------------------------------------------------
  // LiveReader::getUrlProtocols
  //
  bool
  LiveReader::getUrlProtocols(std::list<std::string> & protocols) const
  {
    return private_->movie_.getUrlProtocols(protocols);
  }

  //----------------------------------------------------------------
  // LiveReader::open
  //
  bool
  LiveReader::open(const char * resourcePathUTF8)
  {
    return private_->open(std::string(resourcePathUTF8));
  }

  //----------------------------------------------------------------
  // LiveReader::close
  //
  void
  LiveReader::close()
  {
    return private_->movie_.close();
  }

  //----------------------------------------------------------------
  // LiveReader::getNumberOfPrograms
  //
  std::size_t
  LiveReader::getNumberOfPrograms() const
  {
    // FIXME: pkoshevoy: use recording info .json

    const std::vector<TProgramInfo> & progs = private_->movie_.getPrograms();
    const std::size_t nprogs = progs.size();
    return nprogs;
  }

  //----------------------------------------------------------------
  // LiveReader::getProgramInfo
  //
  bool
  LiveReader::getProgramInfo(std::size_t i, TProgramInfo & info) const
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
  // LiveReader::getNumberOfVideoTracks
  //
  std::size_t
  LiveReader::getNumberOfVideoTracks() const
  {
    // FIXME: pkoshevoy: use recording info .json

    return private_->movie_.getVideoTracks().size();
  }

  //----------------------------------------------------------------
  // LiveReader::getNumberOfAudioTracks
  //
  std::size_t
  LiveReader::getNumberOfAudioTracks() const
  {
    // FIXME: pkoshevoy: use recording info .json

    return private_->movie_.getAudioTracks().size();
  }

  //----------------------------------------------------------------
  // LiveReader::getSelectedVideoTrackIndex
  //
  std::size_t
  LiveReader::getSelectedVideoTrackIndex() const
  {
    return private_->movie_.getSelectedVideoTrack();
  }

  //----------------------------------------------------------------
  // LiveReader::getSelectedAudioTrackIndex
  //
  std::size_t
  LiveReader::getSelectedAudioTrackIndex() const
  {
    return private_->movie_.getSelectedAudioTrack();
  }

  //----------------------------------------------------------------
  // LiveReader::selectVideoTrack
  //
  bool
  LiveReader::selectVideoTrack(std::size_t i)
  {
    return private_->movie_.selectVideoTrack(i);
  }

  //----------------------------------------------------------------
  // LiveReader::selectAudioTrack
  //
  bool
  LiveReader::selectAudioTrack(std::size_t i)
  {
    return private_->movie_.selectAudioTrack(i);
  }

  //----------------------------------------------------------------
  // LiveReader::getSelectedVideoTrackName
  //
  void
  LiveReader::getSelectedVideoTrackInfo(TTrackInfo & info) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    private_->movie_.getVideoTrackInfo(i, info);
  }

  //----------------------------------------------------------------
  // LiveReader::getSelectedAudioTrackInfo
  //
  void
  LiveReader::getSelectedAudioTrackInfo(TTrackInfo & info) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    private_->movie_.getAudioTrackInfo(i, info);
  }

  //----------------------------------------------------------------
  // LiveReader::getVideoDuration
  //
  bool
  LiveReader::getVideoDuration(TTime & start, TTime & duration) const
  {
    if (!private_->walltime_.empty())
    {
      private_->calcTimeline(start, duration);
      return true;
    }

    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      private_->movie_.getVideoTracks()[i]->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::getAudioDuration
  //
  bool
  LiveReader::getAudioDuration(TTime & start, TTime & duration) const
  {
    if (!private_->walltime_.empty())
    {
      private_->calcTimeline(start, duration);
      return true;
    }

    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      private_->movie_.getAudioTracks()[i]->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::getVideoTraits
  //
  bool
  LiveReader::getVideoTraits(VideoTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::getAudioTraits
  //
  bool
  LiveReader::getAudioTraits(AudioTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::setAudioTraitsOverride
  //
  bool
  LiveReader::setAudioTraitsOverride(const AudioTraits & override)
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
  // LiveReader::setVideoTraitsOverride
  //
  bool
  LiveReader::setVideoTraitsOverride(const VideoTraits & override)
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
  // LiveReader::getAudioTraitsOverride
  //
  bool
  LiveReader::getAudioTraitsOverride(AudioTraits & override) const
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
  // LiveReader::getVideoTraitsOverride
  //
  bool
  LiveReader::getVideoTraitsOverride(VideoTraits & override) const
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
  // LiveReader::isSeekable
  //
  bool
  LiveReader::isSeekable() const
  {
    return private_->movie_.isSeekable() && private_->movie_.hasDuration();
  }

  //----------------------------------------------------------------
  // LiveReader::seek
  //
  bool
  LiveReader::seek(double seekTime)
  {
    return private_->seek(seekTime);
  }

  //----------------------------------------------------------------
  // LiveReader::readVideo
  //
  bool
  LiveReader::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
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
  // LiveReader::readAudio
  //
  bool
  LiveReader::readAudio(TAudioFramePtr & frame, QueueWaitMgr * terminator)
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
  // LiveReader::blockedOnVideo
  //
  bool
  LiveReader::blockedOnVideo() const
  {
    return private_->movie_.blockedOnVideo();
  }

  //----------------------------------------------------------------
  // LiveReader::blockedOnAudio
  //
  bool
  LiveReader::blockedOnAudio() const
  {
    return private_->movie_.blockedOnAudio();
  }

  //----------------------------------------------------------------
  // LiveReader::threadStart
  //
  bool
  LiveReader::threadStart()
  {
    return private_->movie_.threadStart();
  }

  //----------------------------------------------------------------
  // LiveReader::threadStop
  //
  bool
  LiveReader::threadStop()
  {
    return private_->movie_.threadStop();
  }

  //----------------------------------------------------------------
  // LiveReader::getPlaybackInterval
  //
  void
  LiveReader::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    private_->getPlaybackInterval(timeIn, timeOut);
  }

  //----------------------------------------------------------------
  // LiveReader::setPlaybackIntervalStart
  //
  void
  LiveReader::setPlaybackIntervalStart(double timeIn)
  {
    private_->setPlaybackIntervalStart(timeIn);
  }

  //----------------------------------------------------------------
  // LiveReader::setPlaybackIntervalEnd
  //
  void
  LiveReader::setPlaybackIntervalEnd(double timeOut)
  {
    private_->setPlaybackIntervalEnd(timeOut);
  }

  //----------------------------------------------------------------
  // LiveReader::setPlaybackEnabled
  //
  void
  LiveReader::setPlaybackEnabled(bool enabled)
  {
    private_->movie_.setPlaybackEnabled(enabled);
  }

  //----------------------------------------------------------------
  // LiveReader::setPlaybackLooping
  //
  void
  LiveReader::setPlaybackLooping(bool enabled)
  {
    private_->movie_.setPlaybackLooping(enabled);
  }

  //----------------------------------------------------------------
  // LiveReader::skipLoopFilter
  //
  void
  LiveReader::skipLoopFilter(bool skip)
  {
    private_->movie_.skipLoopFilter(skip);
  }

  //----------------------------------------------------------------
  // LiveReader::skipNonReferenceFrames
  //
  void
  LiveReader::skipNonReferenceFrames(bool skip)
  {
    private_->movie_.skipNonReferenceFrames(skip);
  }

  //----------------------------------------------------------------
  // LiveReader::setTempo
  //
  bool
  LiveReader::setTempo(double tempo)
  {
    return private_->movie_.setTempo(tempo);
  }

  //----------------------------------------------------------------
  // LiveReader::setDeinterlacing
  //
  bool
  LiveReader::setDeinterlacing(bool enabled)
  {
    return private_->movie_.setDeinterlacing(enabled);
  }

  //----------------------------------------------------------------
  // LiveReader::setRenderCaptions
  //
  void
  LiveReader::setRenderCaptions(unsigned int cc)
  {
    private_->movie_.setRenderCaptions(cc);
  }

  //----------------------------------------------------------------
  // LiveReader::getRenderCaptions
  //
  unsigned int
  LiveReader::getRenderCaptions() const
  {
    return private_->movie_.getRenderCaptions();
  }

  //----------------------------------------------------------------
  // LiveReader::subsCount
  //
  std::size_t
  LiveReader::subsCount() const
  {
    return private_->movie_.subsCount();
  }

  //----------------------------------------------------------------
  // LiveReader::subsInfo
  //
  TSubsFormat
  LiveReader::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    return private_->movie_.subsInfo(i, info);
  }

  //----------------------------------------------------------------
  // LiveReader::setSubsRender
  //
  void
  LiveReader::setSubsRender(std::size_t i, bool render)
  {
    private_->movie_.setSubsRender(i, render);
  }

  //----------------------------------------------------------------
  // LiveReader::getSubsRender
  //
  bool
  LiveReader::getSubsRender(std::size_t i) const
  {
    return private_->movie_.getSubsRender(i);
  }

  //----------------------------------------------------------------
  // LiveReader::countChapters
  //
  std::size_t
  LiveReader::countChapters() const
  {
    return private_->movie_.countChapters();
  }

  //----------------------------------------------------------------
  // LiveReader::getChapterInfo
  //
  bool
  LiveReader::getChapterInfo(std::size_t i, TChapter & c) const
  {
    return private_->movie_.getChapterInfo(i, c);
  }

  //----------------------------------------------------------------
  // LiveReader::getNumberOfAttachments
  //
  std::size_t
  LiveReader::getNumberOfAttachments() const
  {
    return private_->movie_.attachments().size();
  }

  //----------------------------------------------------------------
  // LiveReader::getAttachmentInfo
  //
  const TAttachment *
  LiveReader::getAttachmentInfo(std::size_t i) const
  {
    if (i < private_->movie_.attachments().size())
    {
      return &(private_->movie_.attachments()[i]);
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // LiveReader::setReaderId
  //
  void
  LiveReader::setReaderId(unsigned int readerId)
  {
    private_->readerId_ = readerId;
  }

  //----------------------------------------------------------------
  // LiveReader::setSharedClock
  //
  void
  LiveReader::setSharedClock(const SharedClock & clock)
  {
    private_->movie_.setSharedClock(clock);
  }
}
