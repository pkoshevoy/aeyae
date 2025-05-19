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
#include "yae/ffmpeg/yae_live_reader.h"
#include "yae/ffmpeg/yae_movie.h"
#include "yae/thread/yae_queue.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_recording.h"
#include "yae/video/yae_video.h"


//----------------------------------------------------------------
// YAE_LIVE_READER_GUID
//
#define YAE_LIVE_READER_GUID "4A16794E-FEF7-483D-BFC9-F03A1FBCB35C"


namespace yae
{

  //----------------------------------------------------------------
  // Live
  //
  class Live
  {
  private:
    // intentionally disabled:
    Live(const Live &);
    Live & operator = (const Live &);

  public:
    Live();
    ~Live();

    inline const char * getResourcePath() const
    { return filepath_.empty() ? NULL : filepath_.c_str(); }

    bool open(const std::string & filepath, bool hwdec);
    bool updateTimelinePositions();
    void calcTimeline(TTime & start, TTime & duration);
    TSeekPosPtr getPosition(double t);

    bool seek(double seekTime);
    void getPlaybackInterval(double & timeIn, double & timeOut) const;
    void setPlaybackIntervalStart(double timeIn);
    void setPlaybackIntervalEnd(double timeOut);

    bool findBoundingSamples(std::size_t & i0,
                             std::size_t & i1,
                             uint64_t pos) const;

    void adjustTimestamps(AVFormatContext * ctx, AVPacket * pkt) const;

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
    std::vector<uint64_t> walltime_; // expressed in kTimebase
    std::vector<uint64_t> filesize_; // bytes

    enum { kTimebase = 1000 };

    //----------------------------------------------------------------
    // Seg
    //
    struct Seg
    {
      Seg(std::size_t i = 0, std::size_t n = 0):
        i_(i),
        n_(n)
      {}

      inline uint64_t t0(const Live & ctx) const
      { return n_ ? ctx.walltime_.at(i_) : 0; }

      inline uint64_t t1(const Live & ctx) const
      { return n_ ? ctx.walltime_.at(i_ + n_ - 1) : 0; }

      inline uint64_t p0(const Live & ctx) const
      { return n_ ? ctx.filesize_.at(i_) : 0; }

      inline uint64_t p1(const Live & ctx) const
      { return n_ ? ctx.filesize_.at(i_ + n_ - 1) : 0; }

      inline uint64_t dt(const Live & ctx) const
      { return t1(ctx) - t0(ctx); }

      inline uint64_t bytes(const Live & ctx) const
      { return p1(ctx) - p0(ctx); }

      double bytes_per_sec(const Live & ctx) const;
      double estimate_pos(const Live & ctx, double walltime) const;

      // index of first sample:
      std::size_t i_;

      // number of samples:
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

      // segment index:
      std::size_t ix_;

      // walltime, start time:
      uint64_t t0_;

      // file offsets, start and end:
      uint64_t p0_;
      uint64_t p1_;
    };

    const ByteRange * findBoundingRange(double t) const;

    // timebase of the .dat file:
    uint64_t walltimeTimebase_;

    std::vector<ByteRange> ranges_;
    mutable std::size_t currRange_;
    mutable yae::TTime walltimeOffset_;
  };


  //----------------------------------------------------------------
  // BytePos
  //
  struct BytePos : ISeekPos
  {
    BytePos(uint64_t pos,
            double sec,
            const Live & reader,
            double bytes_per_sec);

    // virtual:
    double get() const;
    void set(double pos);

    // virtual:
    std::string to_str() const;

    // virtual:
    bool lt(const TFrameBase & f, double dur = 0.0) const;
    bool gt(const TFrameBase & f, double dur = 0.0) const;

    // virtual:
    int seek(AVFormatContext * ctx, const AVStream * s) const;

    // bytes:
    uint64_t pos_;

    // seconds:
    double sec_;

    // for iterative refinement:
    const Live & reader_;
    double bytes_per_sec_;
  };

  //----------------------------------------------------------------
  // TBytePosPtr
  //
  typedef yae::shared_ptr<BytePos, ISeekPos> TBytePosPtr;


  //----------------------------------------------------------------
  // BytePos::BytePos
  //
  BytePos::BytePos(uint64_t pos,
                   double sec,
                   const Live & reader,
                   double bytes_per_sec):
    pos_(pos),
    sec_(sec),
    reader_(reader),
    bytes_per_sec_(bytes_per_sec)
  {}

  //----------------------------------------------------------------
  // BytePos::get
  //
  double
  BytePos::get() const
  {
    YAE_ASSERT(false);
    return sec_;
  }

  //----------------------------------------------------------------
  // BytePos::set
  //
  void
  BytePos::set(double sec)
  {
    YAE_ASSERT(false);
    double dt = sec - sec_;
    int64_t dp = int64_t(bytes_per_sec_ * dt) / 188;
    int64_t pos = int64_t(pos_) + dp * 188;
    pos_ = (pos < 0) ? 0 : pos;
    sec_ = sec;
  }

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
  // BytePos::seek
  //
  int
  BytePos::seek(AVFormatContext * context, const AVStream * stream) const
  {
    int err = 0;
    int streamIndex = stream ? stream->index : -1;
    int seekFlags = AVSEEK_FLAG_BYTE;
    int64_t pos = pos_;

    for (int attempt = 0; attempt < 10; attempt++)
    {
      err = avformat_seek_file(context,
                               streamIndex,
                               kMinInt64,
                               pos,
                               pos, // kMaxInt64,
                               seekFlags);

      TPacketPtr packetPtr(new AvPkt());
      AVPacket & packet = packetPtr->get();
      int skip = 0;
      for (skip = 0; skip < 1000; skip++)
      {
        err = av_read_frame(context, &packet);
        if (packet.dts == AV_NOPTS_VALUE ||
            packet.pos == -1)
        {
          continue;
        }

        const AVStream * s =
          ((unsigned int)(packet.stream_index) < context->nb_streams) ?
          context->streams[packet.stream_index] : NULL;

        if (s && stream && s->index != stream->index)
        {
          continue;
        }

        break;
      }

      const AVStream & s = *(context->streams[packet.stream_index]);
      reader_.adjustTimestamps(context, &packet);

      TTime pkt_time(s.time_base.num * packet.dts, s.time_base.den);
      double t = pkt_time.sec();
      double dt = t - sec_;
      if (!pos || fabs(dt) <= 0.034)
      {
        err = avformat_seek_file(context,
                                 streamIndex,
                                 kMinInt64,
                                 pos,
                                 pos, // kMaxInt64,
                                 seekFlags);
        break;
      }

      int64_t prev_pos = pos;
      double padding = 0.0;
      while (pos == prev_pos)
      {
        static const double dampening_factor = 0.95;
        int64_t pos_err = dampening_factor * bytes_per_sec_ * (dt + padding);
        pos_err += (pos_err % 188);
        pos = std::max<int64_t>(0, pos - pos_err);

#ifndef NDEBUG
        yae_wlog("LiveReader: stream %i, seek %s, attempt %i, skip %i, "
                 "t = %s, dt = %.3f, pos_err = %12" PRIi64,
                 packet.stream_index,
                 TTime(sec_).to_hhmmss_ms().c_str(),
                 attempt,
                 skip,
                 pkt_time.to_hhmmss_ms().c_str(),
                 dt,
                 pos_err);
#endif
        padding += 0.5;
      }
    }

    return err;
  }


  //----------------------------------------------------------------
  // Live::Live
  //
  Live::Live():
      readerId_((unsigned int)~0),
      timeIn_(TTime::min_flicks_as_sec()),
      timeOut_(TTime::max_flicks_as_sec()),
      sumTime_(0),
      sumSize_(0),
      walltimeTimebase_(1),
      currRange_(std::numeric_limits<std::size_t>::max()),
      walltimeOffset_(0, 1)
  {
    movie_.setAdjustTimestamps(&adjustTimestampsCallback, this);
  }

  //----------------------------------------------------------------
  // Live::~Live
  //
  Live::~Live()
  {
    movie_.close();
  }

  //----------------------------------------------------------------
  // Live::Seg::bytes_per_sec
  //
  double
  Live::Seg::bytes_per_sec(const Live & ctx) const
  {
    if (n_ < 2)
    {
      return 0;
    }

    std::size_t i1 = i_ + n_ - 1;
    uint64_t dv = ctx.filesize_[i1] - ctx.filesize_[i_];
    uint64_t dt = ctx.walltime_[i1] - ctx.walltime_[i_];

    double bytes_per_sec =
      double(dv * Live::kTimebase) / double(dt);

    return bytes_per_sec;
  }

  //----------------------------------------------------------------
  // Live::Seg::estimate_pos
  //
  double
  Live::Seg::estimate_pos(const Live & ctx, double walltime) const
  {
    double bytes_per_sec = this->bytes_per_sec(ctx);
    uint64_t p0 = this->p0(ctx);

    double t0_sec =
      double(this->t0(ctx)) /
      double(Live::kTimebase);

    double dt = std::max(0.0, walltime - t0_sec);
    double p = p0 + uint64_t(dt * bytes_per_sec);
    return p;
  }

  //----------------------------------------------------------------
  // Live::open
  //
  bool
  Live::open(const std::string & filepath, bool hwdec)
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

    return movie_.open(filepath.c_str(), hwdec);
  }

  //----------------------------------------------------------------
  // Live::updateTimelinePositions
  //
  bool
  Live::updateTimelinePositions()
  {
    if (!dat_)
    {
      return false;
    }

    YAE_BENCHMARK(probe, "Live::updateTimelinePositions");

    // shortcut:
    TOpenFile & dat = *dat_;

    // load walltime:filesize pairs:
    Data buffer(16);
    Bitstream bs(buffer);
    bool updated = false;
    bool start_new_range = false;

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

      if (buffer.starts_with("timebase", 8))
      {
        // discont, because "timebase" is written right after fopen:
        bs.seek(64);
        walltimeTimebase_ = bs.read_bits(64);
        yae_wlog("LiveReader: timebase: %" PRIu64 "", walltimeTimebase_);
        start_new_range = true;
        continue;
      }

      bs.seek(0);
      uint64_t walltime = bs.read_bits(64);
      uint64_t filesize = bs.read_bits(64);

      if (Live::kTimebase != walltimeTimebase_)
      {
        // convert to Live::kTimebase:
        walltime = (walltime * Live::kTimebase) / walltimeTimebase_;
      }

      // make sure walltime and filesize are monotonically increasing:
      if (!walltime_.empty() &&
          (walltime <= walltime_.back() ||
           filesize < filesize_.back()))
      {
        YAE_EXPECT(walltime_.back() < walltime);
        YAE_EXPECT(filesize_.back() < filesize);
        continue;
      }

      if (segments_.empty())
      {
        ranges_.push_back(ByteRange(0, walltime, filesize));
        segments_.push_back(Seg());
        start_new_range = false;
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
          double((sumSize_ + seg_sz) * Live::kTimebase) /
          double(sumTime_ + seg_dt);

        if (avg_bytes_per_sec > 0.0)
        {
          double bytes_per_sec = double(dv * Live::kTimebase) / double(dt);
          double r = bytes_per_sec / avg_bytes_per_sec;

          if (((r < 0.01 && dt > 3.0 * Live::kTimebase) ||

               // keep ranges short to minimize interpolation error
               // when adjusting timestamps:
               // seg_dt >= 60.0 * Live::kTimebase ||

               // discont indicated by "timebase" in .dat:
               start_new_range) &&
              segment->bytes_per_sec(*this) > 1000)
          {
            // discont, instantaneous bitrate is less than half of avg bitrate,
            // or max segment duration reached
            ranges_.back().p1_ = filesize;
            ranges_.push_back(ByteRange(ranges_.size(), walltime, filesize));
            segments_.push_back(Seg(walltime_.size()));
            sumTime_ += seg_dt;
            sumSize_ += seg_sz;
            start_new_range = false;

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
  // Live::calcTimeline
  //
  void
  Live::calcTimeline(TTime & start, TTime & duration)
  {
    YAE_BENCHMARK(probe, "Live::calcTimeline");

    boost::unique_lock<boost::mutex> lock(mutex_);
    updateTimelinePositions();

    if (walltime_.empty())
    {
      start.reset(0, 0); // invalid time
      duration.reset(0, Live::kTimebase); // zero duration
    }
    else
    {
      start = TTime(walltime_.front(), Live::kTimebase);
      duration = TTime(walltime_.back() - walltime_.front(), Live::kTimebase);
    }

    // FIXME: consider timespan of the Recording (as scheduled),
    // and possibly adjust the start time to the scheduled start time
    // (unless Recording started late ... usually they start 30-60s early):
  }

  //----------------------------------------------------------------
  // Live::getPosition
  //
  TSeekPosPtr
  Live::getPosition(double t)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    updateTimelinePositions();

    const ByteRange * range = findBoundingRange(t);
    if (!range)
    {
      return TSeekPosPtr(new TimePos(t));
    }

    const Seg & segment = segments_[range->ix_];
    double p = segment.estimate_pos(*this, t);

    uint64_t tt = uint64_t(std::max<double>(0.0, t) * Live::kTimebase);
    uint64_t t0 = segment.t0(*this);
    uint64_t t1 = segment.t1(*this);
    uint64_t dt = t1 - t0;
    if (dt > 0 && tt <= t1)
    {
      // binary search through walltime:
      std::size_t j0 = segment.i_;
      std::size_t j1 = segment.n_ + j0 - 1;

      while (j0 + 1 < j1)
      {
        std::size_t j = (j0 + j1) >> 1;
        if (tt <= walltime_[j])
        {
          j1 = j;
        }
        else
        {
          j0 = j;
        }
      }

      p = (tt == walltime_[j1]) ? filesize_[j1] : filesize_[j0];
    }

    uint64_t pos = uint64_t(p);
    pos -= pos % 188;

    double bytes_per_sec = segment.bytes_per_sec(*this);
    return TSeekPosPtr(new BytePos(pos, t, *this, bytes_per_sec));
  }

  //----------------------------------------------------------------
  // Live::seek
  //
  bool
  Live::seek(double seekTime)
  {
    TSeekPosPtr pos = getPosition(seekTime);
    return movie_.requestSeek(pos);
  }

  //----------------------------------------------------------------
  // Live::getPlaybackInterval
  //
  void
  Live::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    timeIn = timeIn_;
    timeOut = timeOut_;
  }

  //----------------------------------------------------------------
  // Live::setPlaybackIntervalEnd
  //
  void
  Live::setPlaybackIntervalStart(double timeIn)
  {
    timeIn_ = timeIn;

    TSeekPosPtr pos = getPosition(timeIn_);
    movie_.setPlaybackIntervalStart(pos);
  }

  //----------------------------------------------------------------
  // Live::setPlaybackIntervalEnd
  //
  void
  Live::setPlaybackIntervalEnd(double timeOut)
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
  // Live::findBoundingSamples
  //
  bool
  Live::findBoundingSamples(std::size_t & i0,
                            std::size_t & i1,
                            uint64_t pos) const
  {
    while ((i1 - i0) > 1)
    {
      std::size_t j = i0 + ((i1 - i0) >> 1);

      if (filesize_[j] <= pos)
      {
        i0 = j;
      }
      else
      {
        i1 = j;
      }
    }

    i1 = i0 + 1;
    bool ok = i1 < segments_.size();
    return ok;
  }

  //----------------------------------------------------------------
  // Live::findBoundingRange
  //
  const Live::ByteRange *
  Live::findBoundingRange(double t) const
  {
    if (ranges_.empty())
    {
      return NULL;
    }

    uint64_t tt = uint64_t(std::max(0.0, t) * Live::kTimebase);
    std::size_t i0 = 0;
    std::size_t i1 = ranges_.size() - 1;

    while ((i1 - i0) > 1)
    {
      std::size_t j = i0 + ((i1 - i0) >> 1);

      if (ranges_[j].t0_ <= tt)
      {
        i0 = j;
      }
      else
      {
        i1 = j;
      }
    }

    return (ranges_[i1].t0_ <= tt ? &(ranges_[i1]) : &(ranges_[i0]));
  }

  //----------------------------------------------------------------
  // Live::adjustTimestamps
  //
  void
  Live::adjustTimestamps(AVFormatContext * ctx,
                         AVPacket * pkt) const
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

    TTime dts(stream->time_base.num * pkt->dts,
              stream->time_base.den);

    const ByteRange * range =
      (currRange_ < ranges_.size()) ? &(ranges_.at(currRange_)) : NULL;

    if (range && pkt->pos != -1)
    {
      // check that the range is still valid:
      const Seg & segment = segments_[range->ix_];
      double byterate = segment.bytes_per_sec(*this);
      double posErr0 = double(pkt->pos - int64_t(range->p0_));
      if (posErr0 < -byterate)
      {
        range = NULL;
      }
      else if (range->p1_ != std::numeric_limits<uint64_t>::max())
      {
        double posErr1 = double(pkt->pos - int64_t(range->p1_));
        if (posErr1 > byterate)
        {
          range = NULL;
        }
      }
    }

    if (!range && pkt->pos != -1)
    {
      const ByteRange * r0 = &(ranges_.front());
      const ByteRange * r1 = &(ranges_.back());
      range = find_range(r0, r1, pkt->pos);

      const Seg & segment = segments_[range->ix_];
      double byterate = segment.bytes_per_sec(*this);

      if (byterate > 0.0)
      {
        currRange_ = range - r0;
        double posErr = double(pkt->pos - range->p0_);
        double secErr = posErr / byterate;
#if 0
        std::size_t i0 = segment.i_;
        std::size_t i1 = segment.n_ + i0;
        if (findBoundingSamples(i0, i1, pkt->pos))
        {
          uint64_t dz = filesize_[i1] - filesize_[i0];
          uint64_t dt = walltime_[i1] - walltime_[i0];
          byterate = double(dz * Live::kTimebase) / double(dt);
          posErr = double(pkt->pos - filesize_[i0]);
          secErr = posErr / byterate;

          TTime walltime(walltime_[i0], Live::kTimebase);
          walltimeOffset_ = (dts - walltime) - TTime(secErr);
        }
        else
#endif
        {
          TTime walltime(range->t0_, Live::kTimebase);
          walltimeOffset_ = (dts - walltime) - TTime(secErr);
        }

#ifndef NDEBUG
        yae_wlog
          ("LiveReader"
           ": pkt = (%i, %s, %12" PRIi64 ")"
           ", range: r[%i] = (%s, %12" PRIu64 ")"
           ", byterate %.3f"
           ", posErr %13.1f"
           ", secErr %.3f"
           ", walltimeOffset %s",
           pkt->stream_index,
           dts.to_hhmmss_ms().c_str(),
           pkt->pos,
           range - r0,
           TTime(range->t0_ - walltime_[0],
                 Live::kTimebase).to_hhmmss_ms().c_str(),
           range->p0_,
           byterate,
           posErr,
           secErr,
           (TTime(walltime_[0], Live::kTimebase) +
            walltimeOffset_).to_hhmmss_ms().c_str());
#endif
      }
    }

    dts -= walltimeOffset_;
    pkt->dts = av_rescale_q(dts.time_,
                            Rational(1, dts.base_),
                            stream->time_base);
#if 0 // ndef NDEBUG
    yae_wlog("LiveReader"
             ": pkt = (stream %i, %s, %12" PRIi64 ")",
             pkt->stream_index,
             (dts - TTime(walltime_[0], Live::kTimebase)).
             to_hhmmss_ms().c_str(),
             pkt->pos);
#endif

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
  // Live::adjustTimestampsCallback
  //
  void
  Live::adjustTimestampsCallback(void * ctx,
                                 AVFormatContext * fmt,
                                 AVPacket * pkt)
  {
    Live * reader = (Live *)ctx;
    reader->adjustTimestamps(fmt, pkt);
  }


  //----------------------------------------------------------------
  // LiveReader::Private
  //
  struct LiveReader::Private : Live
  {};


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
  LiveReader::open(const char * resourcePathUTF8, bool hwdec)
  {
    return private_->open(std::string(resourcePathUTF8), hwdec);
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
  // LiveReader::getResourcePath
  //
  const char *
  LiveReader::getResourcePath() const
  {
    return private_->getResourcePath();
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
  // LiveReader::getVideoTrackInfo
  //
  bool
  LiveReader::getVideoTrackInfo(std::size_t i,
                                TTrackInfo & info,
                                VideoTraits & traits) const
  {
    return private_->movie_.getVideoTrackInfo(i, info, traits);
  }

  //----------------------------------------------------------------
  // LiveReader::getAudioTrackInfo
  //
  bool
  LiveReader::getAudioTrackInfo(std::size_t i,
                                TTrackInfo & info,
                                AudioTraits & traits) const
  {
    return private_->movie_.getAudioTrackInfo(i, info, traits);
  }

  //----------------------------------------------------------------
  // LiveReader::getVideoDuration
  //
  bool
  LiveReader::getVideoDuration(TTime & start, TTime & duration) const
  {
    YAE_BENCHMARK(probe, "LiveReader::getVideoDuration");

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
    YAE_BENCHMARK(probe, "LiveReader::getAudioDuration");

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
  LiveReader::setAudioTraitsOverride(const AudioTraits & traits)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->setTraitsOverride(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::setVideoTraitsOverride
  //
  bool
  LiveReader::setVideoTraitsOverride(const VideoTraits & traits)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->setTraitsOverride(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::getAudioTraitsOverride
  //
  bool
  LiveReader::getAudioTraitsOverride(AudioTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->getTraitsOverride(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // LiveReader::getVideoTraitsOverride
  //
  bool
  LiveReader::getVideoTraitsOverride(VideoTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->getTraitsOverride(traits);
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

  //----------------------------------------------------------------
  // LiveReader::setEventObserver
  //
  void
  LiveReader::setEventObserver(const TEventObserverPtr & eo)
  {
    private_->movie_.setEventObserver(eo);
  }
}
