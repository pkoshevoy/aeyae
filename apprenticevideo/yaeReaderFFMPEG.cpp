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
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <set>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>
#include <yaeQueue.h>
#include <yaeReader.h>
#include <yaeReaderFFMPEG.h>
#include <yaeThreading.h>
#include <yaeUtils.h>
#include <yaePixelFormatFFMPEG.h>
#include <yaePixelFormatTraits.h>
#include <yaeAudioFragment.h>
#include <yaeAudioTempoFilter.h>

// ffmpeg includes:
extern "C"
{
#include <libavutil/avstring.h>
#include <libavutil/error.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}


namespace yae
{

  //----------------------------------------------------------------
  // kMaxDouble
  //
  static const double kMaxDouble = std::numeric_limits<double>::max();

  //----------------------------------------------------------------
  // kMaxInt64
  //
  static const int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();

  //----------------------------------------------------------------
  // kMinInt64
  //
  static const int64_t kMinInt64 = std::numeric_limits<int64_t>::min();

  //----------------------------------------------------------------
  // kQueueSizeSmall
  //
  enum
  {
#if 1
    kQueueSizeSmall = 30,
    kQueueSizeMedium = 50,
    kQueueSizeLarge = 120
#else
    kQueueSizeSmall = 1,
    kQueueSizeMedium = 1,
    kQueueSizeLarge = 1
#endif
  };

  //----------------------------------------------------------------
  // Packet
  //
  struct Packet
  {
    Packet()
    {
      memset(&ffmpeg_, 0, sizeof(AVPacket));
    }

    ~Packet()
    {
      av_free_packet(&ffmpeg_);
    }

    bool set(const AVPacket & packet)
    {
      ffmpeg_ = packet;

      int err = av_dup_packet(&ffmpeg_);
      if (err)
      {
        memset(&ffmpeg_, 0, sizeof(AVPacket));
      }

      return !err;
    }

    // raw ffmpeg packet:
    AVPacket ffmpeg_;

  private:
    // intentionally disabled:
    Packet(const Packet &);
    Packet & operator = (const Packet &);
  };

  //----------------------------------------------------------------
  // TPacketPtr
  //
  typedef boost::shared_ptr<Packet> TPacketPtr;

  //----------------------------------------------------------------
  // TPacketQueue
  //
  typedef Queue<TPacketPtr> TPacketQueue;

  //----------------------------------------------------------------
  // TVideoFrameQueue
  //
  typedef Queue<TVideoFramePtr> TVideoFrameQueue;

  //----------------------------------------------------------------
  // TAudioFrameQueue
  //
  typedef Queue<TAudioFramePtr> TAudioFrameQueue;

  //----------------------------------------------------------------
  // PacketTime
  //
  struct PacketTime
  {
    PacketTime(int64_t pts = AV_NOPTS_VALUE,
               int64_t dts = AV_NOPTS_VALUE,
               int64_t duration = AV_NOPTS_VALUE):
      pts_(pts),
      dts_(dts),
      duration_(duration)
    {}

    int64_t pts_;
    int64_t dts_;
    int64_t duration_;
  };

  //----------------------------------------------------------------
  // lockManager
  //
  static int
  lockManager(void ** context, enum AVLockOp op)
  {
    try
    {
      switch (op)
      {
        case AV_LOCK_CREATE:
        {
          *context = new boost::mutex();
        }
        break;

        case AV_LOCK_OBTAIN:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->lock();
        }
        break;

        case AV_LOCK_RELEASE:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->unlock();
        }
        break;

        case AV_LOCK_DESTROY:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          delete mtx;
        }
        break;

        default:
          YAE_ASSERT(false);
          return -1;
      }

      return 0;
    }
    catch (...)
    {}

    return -1;
  }

  //----------------------------------------------------------------
  // getSubsFormat
  //
  static TSubsFormat
  getSubsFormat(enum CodecID id)
  {
    switch (id)
    {
      case CODEC_ID_DVD_SUBTITLE:
        return kSubsDVD;

      case CODEC_ID_DVB_SUBTITLE:
        return kSubsDVB;

      case CODEC_ID_TEXT:
        return kSubsText;

      case CODEC_ID_XSUB:
        return kSubsXSUB;

      case CODEC_ID_SSA:
        return kSubsSSA;

      case CODEC_ID_MOV_TEXT:
        return kSubsMovText;

      case CODEC_ID_HDMV_PGS_SUBTITLE:
        return kSubsHDMVPGS;

      case CODEC_ID_DVB_TELETEXT:
        return kSubsDVBTeletext;

      case CODEC_ID_SRT:
        return kSubsSRT;

      case CODEC_ID_MICRODVD:
        return kSubsMICRODVD;

      case CODEC_ID_EIA_608:
        return kSubsCEA608;

      case CODEC_ID_JACOSUB:
        return kSubsJACOSUB;

      default:
        break;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // SubtitlesInfo
  //
  struct SubtitlesInfo
  {
    SubtitlesInfo():
      streamIndex_(~0),
      format_(kSubsNone),
      render_(false)
    {}

    std::size_t streamIndex_;
    std::string title_;
    TSubsFormat format_;
    bool render_;
  };

  //----------------------------------------------------------------
  // Track
  //
  struct Track
  {
    // NOTE: constructor does not open the stream:
    Track(AVFormatContext * context, AVStream * stream);

    // NOTE: destructor will close the stream:
    virtual ~Track();

    // open the stream for decoding:
    bool open();

    // close the stream:
    virtual void close();

    // get track name:
    const char * getName() const;

    // accessor to stream index of this track within AVFormatContext:
    inline int streamIndex() const
    { return stream_->index; }

    // accessor to the codec context:
    inline AVCodecContext * codecContext() const
    { return stream_->codec; }

    // accessor to the codec:
    inline AVCodec * codec() const
    { return codec_; }

    // get track duration:
    void getDuration(TTime & start, TTime & duration) const;

    // accessor to the packet queue:
    inline TPacketQueue & packetQueue()
    { return packetQueue_; }

    // decode a given packet:
    virtual bool decoderStartup()
    { return false; }

    virtual bool decoderShutdown()
    { return false; }

    virtual bool decode(const TPacketPtr & packetPtr)
    { return false; }

    // packet decoding thread:
    virtual void threadLoop() {}
    virtual bool threadStart();
    virtual bool threadStop();

    // adjust frame duration:
    virtual bool setTempo(double tempo);

  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);

  protected:
    static int callbackGetBuffer(AVCodecContext * c, AVFrame * frame);
    static int callbackRegetBuffer(AVCodecContext * c, AVFrame * frame);
    static void callbackReleaseBuffer(AVCodecContext * c, AVFrame * frame);

    virtual int getBuffer(AVCodecContext * c, AVFrame * frame);
    virtual int regetBuffer(AVCodecContext * c, AVFrame * frame);
    virtual void releaseBuffer(AVCodecContext * c, AVFrame * frame);

    void savePacketTime(const AVPacket & packet);

    // worker thread:
    Thread<Track> thread_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;
    AVStream * stream_;
    AVCodec * codec_;
    TPacketQueue packetQueue_;
    PacketTime packetTime_;

    double timeIn_;
    double timeOut_;
    bool playbackInterval_;
    int64_t startTime_;

    // for adjusting frame duration (playback tempo scaling):
    mutable boost::mutex tempoMutex_;
    double tempo_;

  public:
    uint64_t discarded_;
  };

  //----------------------------------------------------------------
  // TrackPtr
  //
  typedef boost::shared_ptr<Track> TrackPtr;

  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(AVFormatContext * context, AVStream * stream):
    thread_(this),
    context_(context),
    stream_(stream),
    codec_(NULL),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackInterval_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    if (context_ && stream_)
    {
      YAE_ASSERT(context_->streams[stream_->index] == stream_);
    }
  }

  //----------------------------------------------------------------
  // Track::~Track
  //
  Track::~Track()
  {
    threadStop();
    close();
  }

  //----------------------------------------------------------------
  // Track::open
  //
  bool
  Track::open()
  {
    if (!stream_)
    {
      return false;
    }

    threadStop();
    close();

    codec_ = avcodec_find_decoder(stream_->codec->codec_id);
    if (!codec_)
    {
      // unsupported codec:
      return false;
    }

    int err = avcodec_open(stream_->codec, codec_);
    if (err < 0)
    {
      // unsupported codec:
      codec_ = NULL;
      return false;
    }
#if 0
    if (stream_->duration == int64_t(AV_NOPTS_VALUE) &&
        !stream_->codec->bit_rate &&
        context_->duration == int64_t(AV_NOPTS_VALUE) &&
        !context_->bit_rate)
    {
      // unknown duration:
      close();
      return false;
    }
#endif
    stream_->codec->opaque = this;
    stream_->codec->get_buffer = &callbackGetBuffer;
    stream_->codec->reget_buffer = &callbackRegetBuffer;
    stream_->codec->release_buffer = &callbackReleaseBuffer;

    return true;
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    if (stream_ && codec_)
    {
      avcodec_close(stream_->codec);
      codec_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // getTrackName
  //
  static const char *
  getTrackName(AVDictionary * metadata)
  {
    AVDictionaryEntry * name = av_dict_get(metadata,
                                           "name",
                                           NULL,
                                           0);
    if (name)
    {
      return name->value;
    }

    AVDictionaryEntry * title = av_dict_get(metadata,
                                            "title",
                                            NULL,
                                            0);
    if (title)
    {
      return title->value;
    }

    AVDictionaryEntry * lang = av_dict_get(metadata,
                                           "language",
                                           NULL,
                                           0);
    if (lang)
    {
      return lang->value;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Track::getName
  //
  const char *
  Track::getName() const
  {
    return stream_ ? getTrackName(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getDuration
  //
  void
  Track::getDuration(TTime & start, TTime & duration) const
  {
    if (!stream_)
    {
      YAE_ASSERT(false);
      return;
    }

    if (stream_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // return track duration:
      start.base_ = stream_->time_base.den;
      start.time_ =
        stream_->start_time != int64_t(AV_NOPTS_VALUE) ?
        stream_->time_base.num * stream_->start_time : 0;

      duration.time_ = stream_->time_base.num * stream_->duration;
      duration.base_ = stream_->time_base.den;
      return;
    }

    if (!context_)
    {
      YAE_ASSERT(false);
      return;
    }

    if (context_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      start.base_ = AV_TIME_BASE;
      start.time_ =
        context_->start_time != int64_t(AV_NOPTS_VALUE) ?
        context_->start_time : 0;

      duration.time_ = context_->duration;
      duration.base_ = AV_TIME_BASE;
      return;
    }

    int64 fileSize = avio_size(context_->pb);
    int64 fileBits = fileSize * 8;

    start.base_ = AV_TIME_BASE;
    start.time_ = 0;

    if (context_->bit_rate)
    {
      double t =
        double(fileBits / context_->bit_rate) +
        double(fileBits % context_->bit_rate) /
        double(context_->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return;
    }

    if (context_->nb_streams == 1 && stream_->codec->bit_rate)
    {
      double t =
        double(fileBits / stream_->codec->bit_rate) +
        double(fileBits % stream_->codec->bit_rate) /
        double(stream_->codec->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return;
    }

#if 0
    if (context_->nb_streams == 1 &&
        codec_->id == CODEC_ID_RAWVIDEO &&
        stream_->cur_pkt.size)
    {
      if (stream_->avg_frame_rate.num &&
          stream_->avg_frame_rate.den)
      {
        uint64 nframes = fileSize / stream_->cur_pkt.size;
        duration.time_ = stream_->avg_frame_rate.den * nframes;
        duration.base_ = stream_->avg_frame_rate.num;
        return;
      }
      else if (stream_->r_frame_rate.num &&
               stream_->r_frame_rate.den)
      {
        uint64 nframes = fileSize / stream_->cur_pkt.size;
        duration.time_ = stream_->r_frame_rate.den * nframes;
        duration.base_ = stream_->r_frame_rate.num;
        return;
      }
    }
#endif

    // unknown duration:
    duration.time_ = std::numeric_limits<int64>::max();
    duration.base_ = AV_TIME_BASE;
  }

  //----------------------------------------------------------------
  // Track::threadStart
  //
  bool
  Track::threadStart()
  {
    terminator_.stopWaiting(false);
    packetQueue_.open();
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Track::threadStop
  //
  bool
  Track::threadStop()
  {
    terminator_.stopWaiting(true);
    packetQueue_.close();
    thread_.stop();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Track::setTempo
  //
  bool
  Track::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);
    tempo_ = tempo;
    return true;
  }

  //----------------------------------------------------------------
  // Track::callbackGetBuffer
  //
  int
  Track::callbackGetBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    Track * t = (Track *)(codecContext->opaque);
    return t->getBuffer(codecContext, frame);
  }

  //----------------------------------------------------------------
  // Track::callbackRegetBuffer
  //
  int
  Track::callbackRegetBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    Track * t = (Track *)(codecContext->opaque);
    return t->regetBuffer(codecContext, frame);
  }

  //----------------------------------------------------------------
  // Track::callbackReleaseBuffer
  //
  void
  Track::callbackReleaseBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    Track * t = (Track *)(codecContext->opaque);
    t->releaseBuffer(codecContext, frame);
  }

  //----------------------------------------------------------------
  // Track::getBuffer
  //
  int
  Track::getBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    int err = avcodec_default_get_buffer(codecContext, frame);
    if (!err)
    {
      PacketTime * ts = new PacketTime(packetTime_);
      frame->opaque = ts;
    }

    return err;
  }

  //----------------------------------------------------------------
  // Track::regetBuffer
  //
  int
  Track::regetBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    int err = avcodec_default_reget_buffer(codecContext, frame);
    if (!err)
    {
      PacketTime * ts = (PacketTime *)(frame->opaque);
      delete ts;

      ts = new PacketTime(packetTime_);
      frame->opaque = ts;
    }

    return err;
  }

  //----------------------------------------------------------------
  // Track::releaseBuffer
  //
  void
  Track::releaseBuffer(AVCodecContext * codecContext, AVFrame * frame)
  {
    avcodec_default_release_buffer(codecContext, frame);

    PacketTime * ts = (PacketTime *)(frame->opaque);
    delete ts;

    frame->opaque = NULL;
  }

  //----------------------------------------------------------------
  // Track::savePacketTime
  //
  void
  Track::savePacketTime(const AVPacket & packet)
  {
#if 1
    if (packet.pts == stream_->start_time)
    {
      packetTime_.pts_ = AV_NOPTS_VALUE;
    }
    else
#endif
    {
      packetTime_.pts_ = packet.pts;
    }

#if 1
    if (packet.dts == stream_->first_dts)
    {
      packetTime_.dts_ = AV_NOPTS_VALUE;
    }
    else
#endif
    {
      packetTime_.dts_ = packet.dts;
    }

    packetTime_.duration_ = packet.duration;
  }

  //----------------------------------------------------------------
  // FrameWithAutoCleanup
  //
  struct FrameWithAutoCleanup
  {
    FrameWithAutoCleanup():
      frame_(avcodec_alloc_frame())
    {
      frame_->opaque = NULL;
    }

    ~FrameWithAutoCleanup()
    {
      av_free(frame_);
    }

    operator AVFrame * ()
    {
      return frame_;
    }

    operator AVPicture * ()
    {
      return (AVPicture *)frame_;
    }

    void reset()
    {
      av_free(frame_);
      frame_ = avcodec_alloc_frame();
      frame_->opaque = NULL;
    }

  private:
    // intentionally disabled:
    FrameWithAutoCleanup(const FrameWithAutoCleanup &);
    FrameWithAutoCleanup & operator = (const FrameWithAutoCleanup &);

    AVFrame * frame_;
  };

  //----------------------------------------------------------------
  // VideoTrack
  //
  struct VideoTrack : public Track
  {
    VideoTrack(AVFormatContext * context, AVStream * stream);

    // row byte alignment constant:
    enum { kRowAlignment = 16 };

    // virtual:
    bool open();

    // these are used to speed up video decoding:
    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    // virtual:
    bool decoderStartup();
    bool decoderShutdown();
    bool decode(const TPacketPtr & packetPtr);

    // virtual:
    void threadLoop();
    bool threadStop();

    // video traits, not overridden:
    bool getTraits(VideoTraits & traits) const;

    // use this for video frame conversion (pixel format and size)
    bool setTraitsOverride(const VideoTraits & override);
    bool getTraitsOverride(VideoTraits & override) const;

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(double timeIn, double timeOut, bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(double seekTime);

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    TVideoFrameQueue frameQueue_;
    VideoTraits override_;
    VideoTraits native_;
    VideoTraits output_;

    // output sample buffer properties;
    unsigned char numSamplePlanes_;
    std::size_t samplePlaneSize_[4];
    std::size_t sampleLineSize_[4];
    AVRational frameRate_;

    // it appears ffmpeg outputs decoded frames in correct presentation order
    // but without the presentation time stamps (PTS), therefore I will
    // keep a sorted list of packet time stamps for all packets going
    // into avcodec_decode_video2 and use the earliest time stamp
    // from that list to assign PTS to the decoded frames.
    std::list<PacketTime> packetTimes_;

    int64 ptsBestEffort_;
    TTime prevPTS_;
    bool hasPrevPTS_;

    uint64 framesDecoded_;
    struct SwsContext * imgConvertCtx_;

    FrameWithAutoCleanup frameAutoCleanup_;
  };

  //----------------------------------------------------------------
  // VideoTrackPtr
  //
  typedef boost::shared_ptr<VideoTrack> VideoTrackPtr;

  //----------------------------------------------------------------
  // descendingSortOrder
  //
  static bool
  aFollowsB(const TVideoFramePtr & a,
            const TVideoFramePtr & b)
  {
    TTime framePosition;
    if (a->time_.base_ == b->time_.base_)
    {
      return a->time_.time_ > b->time_.time_;
    }

    double ta = double(a->time_.time_) / double(a->time_.base_);
    double tb = double(b->time_.time_) / double(b->time_.base_);
    return ta > tb;
  }

  //----------------------------------------------------------------
  // VideoTrack::VideoTrack
  //
  VideoTrack::VideoTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    frameQueue_(kQueueSizeSmall),
    numSamplePlanes_(0),
    hasPrevPTS_(false),
    framesDecoded_(0),
    imgConvertCtx_(NULL)
  {
    YAE_ASSERT(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO);

    // make sure the frames are sorted from oldest to newest:
    frameQueue_.setSortFunc(&aFollowsB);
  }

  //----------------------------------------------------------------
  // VideoTrack::open
  //
  bool
  VideoTrack::open()
  {
    if (Track::open())
    {
      skipLoopFilter(skipLoopFilter_);
      skipNonReferenceFrames(skipNonReferenceFrames_);

      bool ok = getTraits(override_);
      framesDecoded_ = 0;

      return ok;
    }

    return false;
  }

  //----------------------------------------------------------------
  // VideoTrack::skipLoopFilter
  //
  void
  VideoTrack::skipLoopFilter(bool skip)
  {
    skipLoopFilter_ = skip;

    if (stream_->codec)
    {
      if (skipLoopFilter_)
      {
        stream_->codec->skip_loop_filter = AVDISCARD_ALL;
      }
      else
      {
        stream_->codec->skip_loop_filter = AVDISCARD_DEFAULT;
      }
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::skipNonReferenceFrames
  //
  void
  VideoTrack::skipNonReferenceFrames(bool skip)
  {
    skipNonReferenceFrames_ = skip;

    if (stream_->codec)
    {
      if (skipNonReferenceFrames_)
      {
        stream_->codec->skip_frame = AVDISCARD_NONREF;
      }
      else
      {
        stream_->codec->skip_frame = AVDISCARD_DEFAULT;
      }
    }
  }

  //----------------------------------------------------------------
  // insertPacketTime
  //
  static void
  insertPacketTime(std::list<PacketTime> & packetTimes,
                   const PacketTime & t)
  {
    if (packetTimes.empty())
    {
      packetTimes.push_front(t);
      return;
    }

    for (std::list<PacketTime>::iterator i = packetTimes.begin();
         i != packetTimes.end(); ++i)
    {
      const PacketTime & ti = *i;
      if (t.pts_ > ti.pts_)
      {
        packetTimes.insert(i, t);
        return;
      }
    }

    packetTimes.push_back(t);
  }

  //----------------------------------------------------------------
  // verifyPTS
  //
  // verify that presentation timestamps are monotonically increasing
  //
  static inline bool
  verifyPTS(bool hasPrevPTS, const TTime & prevPTS, const TTime & nextPTS,
            const char * debugMessage = NULL)
  {
    bool ok = (!hasPrevPTS ||
               (prevPTS.base_ == nextPTS.base_ ?
                prevPTS.time_ < nextPTS.time_ :
                prevPTS.toSeconds() < nextPTS.toSeconds()));
#if 0
    if (ok && debugMessage)
    {
      std::cerr << "PTS OK: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << ", " << debugMessage << std::endl;
    }
#else
    (void)debugMessage;
#endif
    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderStartup
  //
  bool
  VideoTrack::decoderStartup()
  {
    // shortcut to native frame format traits:
    getTraits(native_);

    // pixel format shortcut:
    output_ = override_;
    if (output_.pixelFormat_ == kPixelFormatY400A &&
        native_.pixelFormat_ != kPixelFormatY400A)
    {
      // sws_getContext doesn't support Y400A, so drop the alpha channel:
      output_.pixelFormat_ = kPixelFormatGRAY8;
    }

    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(output_.pixelFormat_);
    if (!ptts)
    {
      YAE_ASSERT(false);
      return false;
    }

    // get number of contiguous sample planes,
    // sample set stride (in bits) for each plane:
    unsigned char samplePlaneStride[4] = { 0 };
    numSamplePlanes_ = ptts->getPlanes(samplePlaneStride);

    if (output_.pixelFormat_ != native_.pixelFormat_)
    {
      output_.encodedWidth_ =
        (override_.encodedWidth_ + (kRowAlignment - 1)) &
        ~(kRowAlignment - 1);
    }

    if (ptts->chromaBoxW_ > 1)
    {
      unsigned int remainder = output_.encodedWidth_ % ptts->chromaBoxW_;
      if (remainder)
      {
        output_.encodedWidth_ += ptts->chromaBoxW_ - remainder;
      }
    }

    if (ptts->chromaBoxH_ > 1)
    {
      unsigned int remainder = output_.encodedHeight_ % ptts->chromaBoxH_;
      if (remainder)
      {
        output_.encodedHeight_ += ptts->chromaBoxH_ - remainder;
      }
    }

    // calculate number of bytes for each sample plane:
    std::size_t totalPixels = output_.encodedWidth_ * output_.encodedHeight_;

    memset(samplePlaneSize_, 0, sizeof(samplePlaneSize_));
    samplePlaneSize_[0] = totalPixels * samplePlaneStride[0] / 8;

    memset(sampleLineSize_, 0, sizeof(sampleLineSize_));
    sampleLineSize_[0] = output_.encodedWidth_ * samplePlaneStride[0] / 8;

    for (unsigned char i = 1; i < numSamplePlanes_; i++)
    {
      samplePlaneSize_[i] = totalPixels * samplePlaneStride[i] / 8;
      sampleLineSize_[i] = output_.encodedWidth_ * samplePlaneStride[i] / 8;
    }

    // account for sub-sampling of UV plane(s):
    std::size_t chromaBoxArea = ptts->chromaBoxW_ * ptts->chromaBoxH_;
    if (chromaBoxArea > 1)
    {
      unsigned char uvSamplePlanes =
        (ptts->flags_ & pixelFormat::kAlpha) ?
        numSamplePlanes_ - 2 :
        numSamplePlanes_ - 1;

      for (unsigned char i = 1; i < 1 + uvSamplePlanes; i++)
      {
        samplePlaneSize_[i] /= chromaBoxArea;
        sampleLineSize_[i] /= ptts->chromaBoxW_;
      }
    }

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    // shortcut to the frame rate:
    frameRate_ =
      (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den) ?
      stream_->avg_frame_rate :
      stream_->r_frame_rate;

    if (native_.pixelFormat_ != output_.pixelFormat_ ||
        native_.encodedWidth_ != output_.encodedWidth_ ||
        native_.encodedHeight_ != output_.encodedHeight_)
    {
      // shortcut for ffmpeg pixel format:
      enum PixelFormat ffmpegPixelFormat = yae_to_ffmpeg(output_.pixelFormat_);
      AVCodecContext * codecContext = this->codecContext();

      imgConvertCtx_ = sws_getContext(// from:
                                      native_.encodedWidth_,
                                      native_.encodedHeight_,
                                      codecContext->pix_fmt,

                                      // to:
                                      output_.encodedWidth_,
                                      output_.encodedHeight_,
                                      ffmpegPixelFormat,

                                      SWS_FAST_BILINEAR,
                                      NULL,
                                      NULL,
                                      NULL);
    }

    frameAutoCleanup_.reset();
    packetTimes_.clear();
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    // framesDecoded_ = 0;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderShutdown
  //
  bool
  VideoTrack::decoderShutdown()
  {
    if (imgConvertCtx_)
    {
      sws_freeContext(imgConvertCtx_);
      imgConvertCtx_ = NULL;
    }

    frameAutoCleanup_.reset();
    packetTimes_.clear();
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    frameQueue_.close();
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decode
  //
  bool
  VideoTrack::decode(const TPacketPtr & packetPtr)
  {
    try
    {
      // make a local shallow copy of the packet:
      AVPacket packet = packetPtr->ffmpeg_;

      // save packet timestamps and duration so we can capture
      // and re-use this to timestamp the decoded frame:
      savePacketTime(packet);
      if (packetTime_.pts_ != AV_NOPTS_VALUE ||
          packetTime_.dts_ != AV_NOPTS_VALUE)
      {
        insertPacketTime(packetTimes_, packetTime_);
      }

      // Decode video frame
      int gotPicture = 0;
      AVFrame * avFrame = frameAutoCleanup_;
      AVCodecContext * codecContext = this->codecContext();
      avcodec_decode_video2(codecContext,
                            avFrame,
                            &gotPicture,
                            &packet);
      if (!gotPicture)
      {
        return true;
      }

      framesDecoded_++;
      TVideoFramePtr vfPtr(new TVideoFrame());
      TVideoFrame & vf = *vfPtr;

      // shortcut to the saved packet time associated with this frame:
      const PacketTime * frameTime = (PacketTime *)(avFrame->opaque);

      PacketTime t;
      if (!packetTimes_.empty())
      {
        t = packetTimes_.back();
        packetTimes_.pop_back();
#if 0 // ndef NDEBUG
        if (frameTime && frameTime->pts_ != t.pts_)
        {
          std::cerr << "\ntimestamp mismatch: " << std::endl
                    << "1. out pts: " << frameTime->pts_
                    << ", dts: " << frameTime->dts_
                    << ", len: " << frameTime->duration_ << std::endl
                    << "2. out pts: " << t.pts_
                    << ", dts: " << t.dts_
                    << ", len: " << t.duration_ << std::endl;
        }
#endif
      }

      bool gotPTS = false;
      vf.time_.base_ = stream_->time_base.den;

      // std::cerr << "T: " << avFrame->best_effort_timestamp << std::endl;

      if (!gotPTS && framesDecoded_ == 1)
      {
        ptsBestEffort_ = 0;
        vf.time_.time_ = startTime_;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t0");
      }

      if (!gotPTS)
      {
        if (ptsBestEffort_ < avFrame->best_effort_timestamp)
        {
          ptsBestEffort_ = avFrame->best_effort_timestamp;
        }
        else
        {
          ptsBestEffort_++;
        }

        vf.time_.time_ = stream_->time_base.num * ptsBestEffort_;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_,
                           "avFrame->best_effort_timestamp");
      }

      if (!gotPTS &&
          avFrame->pts != AV_NOPTS_VALUE &&
          codecContext->time_base.num &&
          codecContext->time_base.den)
      {
        vf.time_.time_ = avFrame->pts * codecContext->time_base.num;
        vf.time_.base_ = codecContext->time_base.den;

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "avFrame->pts");
      }
#if 0
      if (!gotPTS && frameTime && frameTime->pts_ != AV_NOPTS_VALUE)
      {
        vf.time_.time_ = stream_->time_base.num * frameTime->pts_;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "frameTime");
      }

      if (!gotPTS && t.pts_ != AV_NOPTS_VALUE)
      {
        vf.time_.time_ = stream_->time_base.num * t.pts_;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t.pts");
      }

      if (!gotPTS && t.dts_ != AV_NOPTS_VALUE && packetTimes_.empty())
      {
        vf.time_.time_ = stream_->time_base.num * t.dts_;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t.dts");
      }

      if (!gotPTS && frameRate_.num && frameRate_.den)
      {
        vf.time_.base_ = stream_->time_base.den;
        vf.time_.time_ =
          startTime_ +
          (framesDecoded_ - 1) *
          (int64_t(frameRate_.den) * int64_t(stream_->time_base.num)) /
          (int64_t(frameRate_.num));

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t0 + n/fps");

        if (!gotPTS && hasPrevPTS_)
        {
          // increment by average frame duration:
          vf.time_ = prevPTS_;
          vf.time_ += TTime(frameRate_.den,
                            frameRate_.num);

          gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t += 1/fps");
        }
      }
#endif

      if (!gotPTS && hasPrevPTS_ && frameRate_.num && frameRate_.den)
      {
        // increment by average frame duration:
        vf.time_ = prevPTS_;
        vf.time_ += TTime(frameRate_.den, frameRate_.num);
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t += 1/fps");
      }

      YAE_ASSERT(gotPTS);
      if (!gotPTS && hasPrevPTS_)
      {
        vf.time_ = prevPTS_;
        vf.time_.time_++;

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t++");
      }

      YAE_ASSERT(gotPTS);
      if (gotPTS)
      {
#if 0 // ndef NDEBUG
        if (hasPrevPTS_)
        {
          double ta = prevPTS_.toSeconds();
          double tb = vf.time_.toSeconds();
          double dt = tb - ta;
          double fd = 1.0 / native_.frameRate_;
          // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
          if (dt > 2.0 * fd)
          {
            std::cerr
              << "\nNOTE: detected large PTS jump: " << std::endl
              << "frame\t:" << framesDecoded_ - 2 << " - " << ta << std::endl
              << "frame\t:" << framesDecoded_ - 1 << " - " << tb << std::endl
              << "difference " << dt << " seconds, equivalent to "
              << dt / fd << " frames" << std::endl
              << std::endl;
          }
        }
#endif

        hasPrevPTS_ = true;
        prevPTS_ = vf.time_;
      }

      // make sure the frame is in the in/out interval:
      if (playbackInterval_)
      {
        double t = vf.time_.toSeconds();
        double dt = 1.0 / double(output_.frameRate_);
        if (t > timeOut_ || (t + dt) < timeIn_)
        {
          if (t > timeOut_)
          {
            discarded_++;
          }

#if 0
          std::cerr << "discarding video frame: " << t
                    << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                    << std::endl;
#endif
          return true;
        }

        discarded_ = 0;
      }

      vf.traits_ = output_;

      TPlanarBufferPtr sampleBuffer(new TPlanarBuffer(numSamplePlanes_),
                                    &IPlanarBuffer::deallocator);
      for (unsigned char i = 0; i < numSamplePlanes_; i++)
      {
        std::size_t rowBytes = sampleLineSize_[i];
        std::size_t rows = samplePlaneSize_[i] / rowBytes;
        sampleBuffer->resize(i, rowBytes, rows, kRowAlignment);
      }
      vf.data_ = sampleBuffer;

      // don't forget about tempo scaling:
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);
        vf.tempo_ = tempo_;
      }

      if (!imgConvertCtx_)
      {
        // copy the sample planes:

        for (unsigned char i = 0; i < numSamplePlanes_; i++)
        {
          std::size_t dstRowBytes = sampleBuffer->rowBytes(i);
          std::size_t dstRows = sampleBuffer->rows(i);
          unsigned char * dst = sampleBuffer->samples(i);

          std::size_t srcRowBytes = avFrame->linesize[i];
          const unsigned char * src = avFrame->data[i];

          std::size_t copyRowBytes = std::min(srcRowBytes, dstRowBytes);
          for (std::size_t i = 0; i < dstRows; i++)
          {
            memcpy(dst, src, copyRowBytes);
            src += srcRowBytes;
            dst += dstRowBytes;
          }
        }
      }
      else
      {
        // convert the image to the desired pixel format:

        AVPicture pict;
        for (unsigned char i = 0; i < numSamplePlanes_; i++)
        {
          pict.data[i] = sampleBuffer->samples(i);
          pict.linesize[i] = (int)sampleBuffer->rowBytes(i);
        }

        sws_scale(imgConvertCtx_,
                  avFrame->data,
                  avFrame->linesize,
                  0,
                  codecContext->height,
                  pict.data,
                  pict.linesize);
      }

      // put the output frame into frame queue:
      if (!frameQueue_.push(vfPtr, &terminator_))
      {
        return false;
      }

      // std::cerr << "V: " << vf.time_.toSeconds() << std::endl;

      // put repeated output frames into frame queue:
      for (int i = 0; i < avFrame->repeat_pict; i++)
      {
        TVideoFramePtr rvfPtr(new TVideoFrame(vf));
        TVideoFrame & rvf = *rvfPtr;

        if (frameRate_.num && frameRate_.den)
        {
          rvf.time_ += TTime((i + 1) * frameRate_.den, frameRate_.num);
        }
        else
        {
          rvf.time_.time_++;
        }

#if 0 // ndef NDEBUG
        std::cerr << "frame repeated at " << rvf.time_.toSeconds() << " sec"
                  << std::endl;
#endif

        if (!frameQueue_.push(rvfPtr, &terminator_))
        {
          return false;
        }
      }
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::threadLoop
  //
  void
  VideoTrack::threadLoop()
  {
    decoderStartup();

    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();

        TPacketPtr packetPtr;
        if (!packetQueue_.pop(packetPtr, &terminator_))
        {
          break;
        }

        if (!decode(packetPtr))
        {
          break;
        }
      }
      catch (...)
      {
        break;
      }
    }

    decoderShutdown();
  }

  //----------------------------------------------------------------
  // VideoTrack::threadStop
  //
  bool
  VideoTrack::threadStop()
  {
    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraits
  //
  bool
  VideoTrack::getTraits(VideoTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    // shortcut:
    const AVCodecContext * context = stream_->codec;

    //! pixel format:
    t.pixelFormat_ = ffmpeg_to_yae(context->pix_fmt);

    //! frame rate:
    if (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den)
    {
      t.frameRate_ =
        double(stream_->avg_frame_rate.num) /
        double(stream_->avg_frame_rate.den);
    }
    else if (stream_->r_frame_rate.num && stream_->r_frame_rate.den)
    {
      t.frameRate_ =
        double(stream_->r_frame_rate.num) /
        double(stream_->r_frame_rate.den);
    }
    else
    {
      t.frameRate_ = 0.0;
      YAE_ASSERT(false);
    }

    //! encoded frame size (including any padding):
    t.encodedWidth_ = context->coded_width;
    t.encodedHeight_ = context->coded_height;

    //! top/left corner offset to the visible portion of the encoded frame:
    t.offsetTop_ = 0;
    t.offsetLeft_ = 0;

    //! dimensions of the visible portion of the encoded frame:
    t.visibleWidth_ = context->width;
    t.visibleHeight_ = context->height;

    //! pixel aspect ration, used to calculate visible frame dimensions:
    t.pixelAspectRatio_ = 1.0;

    if (context->sample_aspect_ratio.num &&
        context->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(context->sample_aspect_ratio.num) /
                             double(context->sample_aspect_ratio.den));
    }
    else if (stream_->sample_aspect_ratio.num &&
             stream_->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(stream_->sample_aspect_ratio.num) /
                             double(stream_->sample_aspect_ratio.den));
    }

    //! a flag indicating whether video is upside-down:
    t.isUpsideDown_ = false;

    return
      t.frameRate_ > 0.0 &&
      t.encodedWidth_ > 0 &&
      t.encodedHeight_ > 0 &&
      t.pixelFormat_ != kInvalidPixelFormat;
  }

  //----------------------------------------------------------------
  // VideoTrack::setTraitsOverride
  //
  bool
  VideoTrack::setTraitsOverride(const VideoTraits & override)
  {
    if (compare<VideoTraits>(override_, override) == 0)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(!alreadyDecoding);

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = override;

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraitsOverride
  //
  bool
  VideoTrack::getTraitsOverride(VideoTraits & override) const
  {
    override = override_;
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getNextFrame
  //
  bool
  VideoTrack::getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || !playbackInterval_)
      {
        break;
      }

      // discard outlier frames:
      double t = frame->time_.toSeconds();
      double dt = 1.0 / double(frame->traits_.frameRate_);
      if (t < timeOut_ && (t + dt) > timeIn_)
      {
        break;
      }

#if 0
      std::cerr << "ignoring video frame: " << t
                << ", expecting [" << timeIn << ", " << timeOut << ")"
                << std::endl;
#endif
    }

    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::setPlaybackInterval
  //
  void
  VideoTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackInterval_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // VideoTrack::resetTimeCounters
  //
  int
  VideoTrack::resetTimeCounters(double seekTime)
  {
    packetQueue().clear();
    frameQueue_.clear();
    packetQueue().waitForConsumerToBlock();
    frameQueue_.clear();

    // push a NULL frame into frame queue to resetTimeCounter
    // down the line:
    frameQueue_.startNewSequence(TVideoFramePtr());

    int err = 0;
    if (stream_ && stream_->codec)
    {
      avcodec_flush_buffers(stream_->codec);
#if 1
      avcodec_close(stream_->codec);
      codec_ = avcodec_find_decoder(stream_->codec->codec_id);
      err = avcodec_open(stream_->codec, codec_);
      YAE_ASSERT(err >= 0);
#endif
    }

    setPlaybackInterval(seekTime, timeOut_, playbackInterval_);
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
    packetTimes_.clear();
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    framesDecoded_ = 0;

    return err;
  }

  //----------------------------------------------------------------
  // AudioTrack
  //
  struct AudioTrack : public Track
  {
    AudioTrack(AVFormatContext * context, AVStream * stream);

    // virtual:
    bool open();

    // virtual: FIXME: write me!
    bool decoderStartup();
    bool decoderShutdown();
    bool decode(const TPacketPtr & packetPtr);

    // virtual:
    void threadLoop();
    bool threadStop();

    // reset remixer/resampler if native traits change during decoding:
    void noteNativeTraitsChanged();

    // audio traits, not overridden:
    bool getTraits(AudioTraits & traits) const;

    // use this for audio format conversion (sample rate, channels, etc...)
    bool setTraitsOverride(const AudioTraits & override);
    bool getTraitsOverride(AudioTraits & override) const;

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(double timeIn, double timeOut, bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(double seekTime);

    // adjust frame duration:
    bool setTempo(double tempo);

    TAudioFrameQueue frameQueue_;
    AudioTraits override_;
    AudioTraits native_;
    AudioTraits output_;

    // output sample buffer properties:
    int nativeChannels_;
    int outputChannels_;
    unsigned int nativeBytesPerSample_;
    unsigned int outputBytesPerSample_;
    TDataBuffer nativeBuffer_;
    TDataBuffer remixBuffer_;
    TDataBuffer resampleBuffer_;
    std::vector<double> remixChannelMatrix_;

    TTime prevPTS_;
    bool hasPrevPTS_;
    uint64 prevNumSamples_;
    uint64 samplesDecoded_;

    ReSampleContext * resampleCtx_;

    // for adjusting audio frame duration:
    std::vector<unsigned char> tempoBuffer_;
    IAudioTempoFilter * tempoFilter_;
  };

  //----------------------------------------------------------------
  // AudioTrackPtr
  //
  typedef boost::shared_ptr<AudioTrack> AudioTrackPtr;

  //----------------------------------------------------------------
  // AudioTrack::AudioTrack
  //
  AudioTrack::AudioTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream),
    frameQueue_(kQueueSizeMedium),
    nativeBytesPerSample_(0),
    outputBytesPerSample_(0),
    hasPrevPTS_(false),
    prevNumSamples_(0),
    samplesDecoded_(0),
    resampleCtx_(NULL),
    tempoFilter_(NULL)
  {
    YAE_ASSERT(stream->codec->codec_type == AVMEDIA_TYPE_AUDIO);

    // match output queue size to input queue size:
    frameQueue_.setMaxSize(packetQueue_.getMaxSize());
  }

  //----------------------------------------------------------------
  // AudioTrack::open
  //
  bool
  AudioTrack::open()
  {
    if (Track::open())
    {
      bool ok = getTraits(override_);
      samplesDecoded_ = 0;

      return ok;
    }

    return false;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  static enum AVSampleFormat
  yae_to_ffmpeg(TAudioSampleFormat yaeSampleFormat)
  {
    switch (yaeSampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return AV_SAMPLE_FMT_U8;

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        return AV_SAMPLE_FMT_S16;

      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        return AV_SAMPLE_FMT_S32;

      case kAudio32BitFloat:
        return AV_SAMPLE_FMT_FLT;

      default:
        break;
    }

    return AV_SAMPLE_FMT_NONE;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderStartup
  //
  bool
  AudioTrack::decoderStartup()
  {
    output_ = override_;

    outputChannels_ = getNumberOfChannels(output_.channelLayout_);

    outputBytesPerSample_ =
      outputChannels_ * getBitsPerSample(output_.sampleFormat_) / 8;

    // declare a 16-byte aligned buffer for decoded audio samples:
    nativeBuffer_.resize((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2, 1, 16);

    getTraits(native_);
    noteNativeTraitsChanged();

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    samplesDecoded_ = 0;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderShutdown
  //
  bool
  AudioTrack::decoderShutdown()
  {
    if (resampleCtx_)
    {
      // FIXME: flush the resampler:
      audio_resample_close(resampleCtx_);
      resampleCtx_ = NULL;
    }

    remixBuffer_ = TDataBuffer();
    resampleBuffer_ = TDataBuffer();

    frameQueue_.close();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decode
  //
  bool
  AudioTrack::decode(const TPacketPtr & packetPtr)
  {
    try
    {
      // make a local shallow copy of the packet:
      AVPacket packet = packetPtr->ffmpeg_;
      AVCodecContext * codecContext = this->codecContext();

      // save packet timestamps and duration so we can capture
      // and re-use this to timestamp the decoded frame:
      // savePacketTime(packet);

      // Decode audio frame, piecewise:
      std::list<std::vector<unsigned char> > chunks;
      std::size_t nativeBytes = 0;
      std::size_t outputBytes = 0;

      while (packet.size)
      {
        int bufferSize = (int)(nativeBuffer_.rowBytes());
        int bytesUsed = avcodec_decode_audio3(codecContext,
                                              nativeBuffer_.data<int16_t>(),
                                              &bufferSize,
                                              &packet);

        if (bytesUsed < 0)
        {
          break;
        }

        // adjust the packet (the copy, not the original):
        packet.size -= bytesUsed;
        packet.data += bytesUsed;

        if (!bufferSize)
        {
          continue;
        }

        if (nativeChannels_ != codecContext->channels ||
            native_.sampleRate_ != codecContext->sample_rate)
        {
          // detected a change in the number of audio channels,
          // or detected a change in audio sample rate,
          // prepare to remix or resample accordingly:
          getTraits(native_);
          noteNativeTraitsChanged();
        }

        const int srcSamples = bufferSize / nativeBytesPerSample_;

        if (outputChannels_ != nativeChannels_)
        {
          yae::remix(srcSamples,
                     native_.sampleFormat_,
                     native_.channelFormat_,
                     native_.channelLayout_,
                     nativeBuffer_.data(),
                     output_.channelLayout_,
                     remixBuffer_.data(),
                     &remixChannelMatrix_[0]);
        }

        if (resampleCtx_)
        {
          int16_t * dst = resampleBuffer_.data<int16_t>();

          int16_t * src =
            (outputChannels_ == nativeChannels_ ?
             nativeBuffer_.data<int16_t>() :
             remixBuffer_.data<int16_t>());

          int dstSamples = audio_resample(resampleCtx_, dst, src, srcSamples);

          if (dstSamples)
          {
            std::size_t resampledBytes = dstSamples * outputBytesPerSample_;
            chunks.push_back(std::vector<unsigned char>
                             (resampleBuffer_.data(),
                              resampleBuffer_.data() + resampledBytes));
            outputBytes += resampledBytes;
          }
        }
        else if (outputChannels_ != nativeChannels_)
        {
          std::size_t remixedBytes = srcSamples * outputBytesPerSample_;
            chunks.push_back(std::vector<unsigned char>
                             (remixBuffer_.data(),
                              remixBuffer_.data() + remixedBytes));
          outputBytes += remixedBytes;
        }
        else
        {
          chunks.push_back(std::vector<unsigned char>
                           (nativeBuffer_.data(),
                            nativeBuffer_.data() + bufferSize));
          outputBytes += bufferSize;
        }

        nativeBytes += bufferSize;
      }

      if (!outputBytes)
      {
        return true;
      }

      std::size_t numNativeSamples = nativeBytes / nativeBytesPerSample_;
      samplesDecoded_ += numNativeSamples;

      TAudioFramePtr afPtr(new TAudioFrame());
      TAudioFrame & af = *afPtr;

      af.traits_ = output_;
      af.time_.base_ = stream_->time_base.den;

      bool gotPTS = false;

      if (!gotPTS &&
          packet.pts != AV_NOPTS_VALUE)
      {
        af.time_.time_ = stream_->time_base.num * packet.pts;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_, "packet.pts");
      }

      if (!gotPTS &&
          packet.dts != AV_NOPTS_VALUE)
      {
        af.time_.time_ = stream_->time_base.num * packet.dts;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_, "packet.dts");
      }

      if (!gotPTS)
      {
        af.time_.base_ = native_.sampleRate_;
        af.time_.time_ = samplesDecoded_ - numNativeSamples;
        af.time_ += TTime(startTime_, stream_->time_base.den);

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_ += TTime(prevNumSamples_, native_.sampleRate_);

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      YAE_ASSERT(gotPTS);
      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_.time_++;

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      YAE_ASSERT(gotPTS);
      if (gotPTS)
      {
#if 0 // ndef NDEBUG
        if (hasPrevPTS_)
        {
          double ta = prevPTS_.toSeconds();
          double tb = af.time_.toSeconds();
          double dt = tb - ta;
          // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
          if (dt > 0.67)
          {
            std::cerr
              << "\nNOTE: detected large audio PTS jump -- " << std::endl
              << dt << " seconds" << std::endl
              << std::endl;
          }
        }
#endif

        hasPrevPTS_ = true;
        prevPTS_ = af.time_;
        prevNumSamples_ = numNativeSamples;
      }

      // make sure the frame is in the in/out interval:
      if (playbackInterval_)
      {
        double t = af.time_.toSeconds();
        double dt = double(numNativeSamples) / double(native_.sampleRate_);
        if (t > timeOut_ || (t + dt) < timeIn_)
        {
          if (t > timeOut_)
          {
            discarded_++;
          }

#if 0
          std::cerr << "discarding audio frame: " << t
                    << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                    << std::endl;
#endif
          return true;
        }

        discarded_ = 0;
      }

      TPlanarBufferPtr sampleBuffer(new TPlanarBuffer(1),
                                    &IPlanarBuffer::deallocator);
      af.data_ = sampleBuffer;

      bool shouldAdjustTempo = true;
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);
        shouldAdjustTempo = tempoFilter_ && tempo_ != 1.0;
        af.tempo_ = shouldAdjustTempo ? tempo_ : 1.0;
      }

      if (!shouldAdjustTempo)
      {
        // concatenate chunks into a contiguous frame buffer:
        sampleBuffer->resize(0, outputBytes, 1, 16);
        unsigned char * afSampleBuffer = sampleBuffer->samples(0);

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();
          memcpy(afSampleBuffer, chunk, chunkSize);

          afSampleBuffer += chunkSize;
          chunks.pop_front();
        }
      }
      else
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);

        // pass source samples through the tempo filter:
        std::size_t frameSize = 0;

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();

          const unsigned char * srcStart = chunk;
          const unsigned char * srcEnd = srcStart + chunkSize;
          const unsigned char * src = srcStart;

          while (src < srcEnd)
          {
            unsigned char * dstStart = &tempoBuffer_[0];
            unsigned char * dstEnd = dstStart + tempoBuffer_.size();
            unsigned char * dst = dstStart;
            tempoFilter_->apply(&src, srcEnd, &dst, dstEnd);

            std::size_t tmpSize = dst - dstStart;
            sampleBuffer->resize(frameSize + tmpSize);

            unsigned char * afSampleBuffer = sampleBuffer->samples(0);
            memcpy(afSampleBuffer + frameSize, dstStart, tmpSize);
            frameSize += tmpSize;
          }

          YAE_ASSERT(src == srcEnd);
          chunks.pop_front();
        }
      }

      // put the decoded frame into frame queue:
      if (!frameQueue_.push(afPtr, &terminator_))
      {
        return false;
      }

      // std::cerr << "A: " << af.time_.toSeconds() << std::endl;
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::threadLoop
  //
  void
  AudioTrack::threadLoop()
  {
    decoderStartup();

    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();

        TPacketPtr packetPtr;
        if (!packetQueue_.pop(packetPtr, &terminator_))
        {
          break;
        }

        if (!decode(packetPtr))
        {
          break;
        }
      }
      catch (...)
      {
        break;
      }
    }

    decoderShutdown();
  }

  //----------------------------------------------------------------
  // AudioTrack::threadStop
  //
  bool
  AudioTrack::threadStop()
  {
    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // AudioTrack::noteNativeTraitsChanged
  //
  void
  AudioTrack::noteNativeTraitsChanged()
  {
    if (resampleCtx_)
    {
      // FIXME: flush the resampler:
      audio_resample_close(resampleCtx_);
      resampleCtx_ = NULL;
    }

    // reset the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      delete tempoFilter_;
      tempoFilter_ = NULL;
    }

    remixBuffer_ = TDataBuffer();
    resampleBuffer_ = TDataBuffer();

    unsigned int bitsPerSample = getBitsPerSample(native_.sampleFormat_);
    nativeChannels_ = getNumberOfChannels(native_.channelLayout_);
    nativeBytesPerSample_ = (nativeChannels_ * bitsPerSample / 8);

    if (outputChannels_ != nativeChannels_)
    {
      // lookup a remix matrix:
      getRemixMatrix(nativeChannels_, outputChannels_, remixChannelMatrix_);
      remixBuffer_.resize(std::size_t(double(nativeBuffer_.rowBytes()) *
                                      double(outputChannels_) /
                                      double(nativeChannels_) + 0.5),
                          1, 16);
    }

    // we implement our own remixing because ffmpeg supports
    // a very limited subset of possible channel configurations:
    if (output_.sampleRate_ != native_.sampleRate_)
    {
      resampleBuffer_.resize(std::size_t(double(nativeBuffer_.rowBytes()) *
                                         double(outputBytesPerSample_) /
                                         double(nativeBytesPerSample_) + 0.5),
                             1, 16);
    }

    if (native_.sampleFormat_ != output_.sampleFormat_ ||
        native_.sampleRate_ != output_.sampleRate_)
    {
      enum AVSampleFormat nativeFormat = yae_to_ffmpeg(native_.sampleFormat_);
      enum AVSampleFormat outputFormat = yae_to_ffmpeg(output_.sampleFormat_);
      resampleCtx_ = av_audio_resample_init(outputChannels_,
                                            outputChannels_,
                                            output_.sampleRate_,
                                            native_.sampleRate_,
                                            outputFormat,
                                            nativeFormat,
                                            16, // taps
                                            10, // log2 phase count
                                            0, // linear
                                            0.8); // cutoff frequency
    }

    // initialize the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      YAE_ASSERT(!tempoFilter_);

      if ((output_.channelFormat_ != kAudioChannelsPlanar ||
           output_.channelLayout_ == kAudioMono))
      {
        if (output_.sampleFormat_ == kAudio8BitOffsetBinary)
        {
          tempoFilter_ = new TAudioTempoFilterU8();
        }
        else if (output_.sampleFormat_ == kAudio16BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI16();
        }
        else if (output_.sampleFormat_ == kAudio32BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI32();
        }
        else if (output_.sampleFormat_ == kAudio32BitFloat)
        {
          tempoFilter_ = new TAudioTempoFilterF32();
        }

        if (tempoFilter_)
        {
          tempoFilter_->reset(output_.sampleRate_, outputChannels_);
          tempoFilter_->setTempo(tempo_);

          std::size_t fragmentSize = tempoFilter_->fragmentSize();
          tempoBuffer_.resize(fragmentSize * 3);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraits
  //
  bool
  AudioTrack::getTraits(AudioTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    // shortcut:
    const AVCodecContext * context = stream_->codec;

    switch (context->sample_fmt)
    {
      case AV_SAMPLE_FMT_U8:
        t.sampleFormat_ = kAudio8BitOffsetBinary;
        break;

      case AV_SAMPLE_FMT_S16:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio16BitBigEndian;
#else
        t.sampleFormat_ = kAudio16BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_S32:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio32BitBigEndian;
#else
        t.sampleFormat_ = kAudio32BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_FLT:
        t.sampleFormat_ = kAudio32BitFloat;
        break;

      default:
        t.sampleFormat_ = kAudioInvalidFormat;
        break;
    }

    switch (context->channels)
    {
      case 1:
        t.channelLayout_ = kAudioMono;
        break;

      case 2:
        t.channelLayout_ = kAudioStereo;
        break;

      case 3:
        t.channelLayout_ = kAudio2Pt1;
        break;

      case 4:
        t.channelLayout_ = kAudioQuad;
        break;

      case 5:
        t.channelLayout_ = kAudio4Pt1;
        break;

      case 6:
        t.channelLayout_ = kAudio5Pt1;
        break;

      case 7:
        t.channelLayout_ = kAudio6Pt1;
        break;

      case 8:
        t.channelLayout_ = kAudio7Pt1;
        break;

      default:
        t.channelLayout_ = kAudioChannelLayoutInvalid;
        break;
    }

    //! audio sample rate, Hz:
    t.sampleRate_ = context->sample_rate;

    //! packed, planar:
    t.channelFormat_ = kAudioChannelsPacked;

    return
      t.sampleRate_ > 0 &&
      t.sampleFormat_ != kAudioInvalidFormat &&
      t.channelLayout_ != kAudioChannelLayoutInvalid;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTraitsOverride
  //
  bool
  AudioTrack::setTraitsOverride(const AudioTraits & override)
  {
    if (compare<AudioTraits>(override_, override) == 0)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(!alreadyDecoding);

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = override;

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraitsOverride
  //
  bool
  AudioTrack::getTraitsOverride(AudioTraits & override) const
  {
    override = override_;
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getNextFrame
  //
  bool
  AudioTrack::getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || !playbackInterval_)
      {
        break;
      }

      // discard outlier frames:
      const AudioTraits & atraits = frame->traits_;
      unsigned int sampleSize = getBitsPerSample(atraits.sampleFormat_) / 8;
      int channels = getNumberOfChannels(atraits.channelLayout_);
      std::size_t frameSize = frame->data_->rowBytes(0);
      std::size_t numSamples = frameSize / (channels * sampleSize);

      double t = frame->time_.toSeconds();
      double dt = double(numSamples) / double(atraits.sampleRate_);

      if (t < timeOut_ && (t + dt) > timeIn_)
      {
        break;
      }

#if 0
      std::cerr << "ignoring audio frame: " << t
                << ", expecting [" << timeIn << ", " << timeOut << ")"
                << std::endl;
#endif
    }

    return ok;
  }

  //----------------------------------------------------------------
  // AudioTrack::setPlaybackInterval
  //
  void
  AudioTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackInterval_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // AudioTrack::resetTimeCounters
  //
  int
  AudioTrack::resetTimeCounters(double seekTime)
  {
    packetQueue().clear();
    frameQueue_.clear();
    packetQueue().waitForConsumerToBlock();
    frameQueue_.clear();

    // push a NULL frame into frame queue to resetTimeCounter
    // down the line:
    frameQueue_.startNewSequence(TAudioFramePtr());

    int err = 0;
    if (stream_ && stream_->codec)
    {
      avcodec_flush_buffers(stream_->codec);
#if 1
      avcodec_close(stream_->codec);
      codec_ = avcodec_find_decoder(stream_->codec->codec_id);
      err = avcodec_open(stream_->codec, codec_);
      YAE_ASSERT(err >= 0);
#endif
    }

    setPlaybackInterval(seekTime, timeOut_, playbackInterval_);
    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
    samplesDecoded_ = 0;

    return err;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTempo
  //
  bool
  AudioTrack::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);

    tempo_ = tempo;

    if (tempoFilter_ && !tempoFilter_->setTempo(tempo_))
    {
      return false;
    }

    if (tempo_ == 1.0 && tempoFilter_)
    {
      tempoFilter_->clear();
    }

    return true;
  }

  //----------------------------------------------------------------
  // Movie
  //
  struct Movie
  {
    Movie();

    // NOTE: destructor will close the movie:
    ~Movie();

    bool getUrlProtocols(std::list<std::string> & protocols) const;

    bool open(const char * resourcePath);
    void close();

    inline const std::vector<VideoTrackPtr> & getVideoTracks() const
    { return videoTracks_; }

    inline const std::vector<AudioTrackPtr> & getAudioTracks() const
    { return audioTracks_; }

    inline std::size_t getSelectedVideoTrack() const
    { return selectedVideoTrack_; }

    inline std::size_t getSelectedAudioTrack() const
    { return selectedAudioTrack_; }

    bool selectVideoTrack(std::size_t i);
    bool selectAudioTrack(std::size_t i);

    // this will read the file and push audio/video packets to decoding queues:
    void threadLoop();
    bool threadStart();
    bool threadStop();

    bool requestSeekTime(double seekTime);

  protected:
    int seekTo(double seekTime);

  public:
    int rewind(const AudioTrackPtr & audioTrack,
               const VideoTrackPtr & videoTrack,
               bool seekToTimeIn = true);

    void getPlaybackInterval(double & timeIn, double & timeOut) const;
    void setPlaybackIntervalStart(double timeIn);
    void setPlaybackIntervalEnd(double timeOut);
    void setPlaybackInterval(bool enabled);
    void setPlaybackLooping(bool enabled);

    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    bool setTempo(double tempo);

    std::size_t subsCount() const;
    const char * subsInfo(std::size_t i, TSubsFormat * t) const;
    void subsRender(std::size_t i, bool render);

  private:
    // intentionally disabled:
    Movie(const Movie &);
    Movie & operator = (const Movie &);

  protected:
    // worker thread:
    Thread<Movie> thread_;
    mutable boost::mutex mutex_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;

    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    std::vector<SubtitlesInfo> subs_;

    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    // demuxer current position (DTS):
    TTime dts_;
    int dtsStreamIndex_;

    double timeIn_;
    double timeOut_;
    bool playbackInterval_;
    bool looping_;

    bool mustSeek_;
    double seekTime_;
  };


  //----------------------------------------------------------------
  // Movie::Movie
  //
  Movie::Movie():
    thread_(this),
    context_(NULL),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    selectedVideoTrack_(0),
    selectedAudioTrack_(0),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackInterval_(false),
    looping_(false),
    mustSeek_(false),
    seekTime_(0.0)
  {}

  //----------------------------------------------------------------
  // Movie::~Movie
  //
  Movie::~Movie()
  {
    close();
  }

  //----------------------------------------------------------------
  // Movie::getUrlProtocols
  //
  bool
  Movie::getUrlProtocols(std::list<std::string> & protocols) const
  {
    protocols.clear();

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 13, 0)
    void * opaque = NULL;
    const char * name = NULL;
    while ((name = avio_enum_protocols(&opaque, 0)))
    {
      protocols.push_back(std::string(name));
    }
#else
    URLProtocol * protocol = NULL;
    while ((protocol = av_protocol_next(protocol)))
    {
      if (protocol->url_read)
      {
        protocols.push_back(std::string(protocol->name));
      }
    }
#endif

    return true;
  }

  //----------------------------------------------------------------
  // Movie::open
  //
  bool
  Movie::open(const char * resourcePath)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();

    int err = avformat_open_input(&context_,
                                  resourcePath,
                                  NULL, // AVInputFormat to force
                                  NULL);// AVDictionary of options
    if (err != 0)
    {
      close();
      return false;
    }

    // spend at most 2 seconds trying to analyze the file:
    context_->max_analyze_duration = 30 * AV_TIME_BASE;

    err = avformat_find_stream_info(context_, NULL);
    if (err < 0)
    {
      close();
      return false;
    }

    for (unsigned int i = 0; i < context_->nb_streams; i++)
    {
      AVStream * stream = context_->streams[i];
      TrackPtr track(new Track(context_, stream));

      if (!track->open())
      {
        // unsupported codec, ignore it:
        continue;
      }

      const AVMediaType codecType = stream->codec->codec_type;
      if (codecType == AVMEDIA_TYPE_VIDEO)
      {
        VideoTrackPtr track(new VideoTrack(context_, stream));
        VideoTraits traits;
        if (track->getTraits(traits))
        {
          videoTracks_.push_back(track);
        }
      }
      else if (codecType == AVMEDIA_TYPE_AUDIO)
      {
        AudioTrackPtr track(new AudioTrack(context_, stream));
        AudioTraits traits;
        if (track->getTraits(traits))
        {
          audioTracks_.push_back(track);
        }
      }
      else if (codecType == AVMEDIA_TYPE_SUBTITLE)
      {
        subs_.push_back(SubtitlesInfo());
        SubtitlesInfo & subs = subs_.back();

        subs.streamIndex_ = i;
        subs.title_ = std::string("Track ") + toText(subs_.size());

        const char * name = getTrackName(stream->metadata);
        if (name)
        {
          subs.title_ += ", ";
          subs.title_ += name;
        }

        subs.format_ = getSubsFormat(stream->codec->codec_id);
      }
    }

    if (videoTracks_.empty() &&
        audioTracks_.empty())
    {
      // no decodable video/audio tracks present:
      close();
      return false;
    }

    // by default do not select any tracks:
    selectedVideoTrack_ = videoTracks_.size();
    selectedAudioTrack_ = audioTracks_.size();

    return true;
  }

  //----------------------------------------------------------------
  // Movie::close
  //
  void
  Movie::close()
  {
    if (context_ == NULL)
    {
      return;
    }

    threadStop();

    const std::size_t numVideoTracks = videoTracks_.size();
    selectVideoTrack(numVideoTracks);

    const std::size_t numAudioTracks = audioTracks_.size();
    selectAudioTrack(numAudioTracks);

    videoTracks_.clear();
    audioTracks_.clear();
    subs_.clear();

    av_close_input_file(context_);
    context_ = NULL;
  }

  //----------------------------------------------------------------
  // Movie::selectVideoTrack
  //
  bool
  Movie::selectVideoTrack(std::size_t i)
  {
    const std::size_t numVideoTracks = videoTracks_.size();
    if (selectedVideoTrack_ < numVideoTracks)
    {
      // close currently selected track:
      VideoTrackPtr track = videoTracks_[selectedVideoTrack_];
      track->close();
    }

    selectedVideoTrack_ = i;
    if (selectedVideoTrack_ >= numVideoTracks)
    {
      return false;
    }

    VideoTrackPtr track = videoTracks_[selectedVideoTrack_];
    track->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
    track->skipLoopFilter(skipLoopFilter_);
    track->skipNonReferenceFrames(skipNonReferenceFrames_);
    return track->open();
  }

  //----------------------------------------------------------------
  // Movie::selectAudioTrack
  //
  bool
  Movie::selectAudioTrack(std::size_t i)
  {
    const std::size_t numAudioTracks = audioTracks_.size();
    if (selectedAudioTrack_ < numAudioTracks)
    {
      // close currently selected track:
      AudioTrackPtr track = audioTracks_[selectedAudioTrack_];
      track->close();
    }

    selectedAudioTrack_ = i;
    if (selectedAudioTrack_ >= numAudioTracks)
    {
      return false;
    }

    AudioTrackPtr track = audioTracks_[selectedAudioTrack_];
    track->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
    return track->open();
  }

  //----------------------------------------------------------------
  // copyPacket
  //
  static TPacketPtr
  copyPacket(const AVPacket & ffmpeg)
  {
    try
    {
      TPacketPtr p(new Packet());
      if (p->set(ffmpeg))
      {
        return p;
      }
    }
    catch (...)
    {}

    return TPacketPtr();
  }

  //----------------------------------------------------------------
  // PacketQueueCloseOnExit
  //
  struct PacketQueueCloseOnExit
  {
    TrackPtr track_;

    PacketQueueCloseOnExit(TrackPtr track):
      track_(track)
    {
      if (track_ && track_->packetQueue().isClosed())
      {
        track_->packetQueue().open();
      }
    }

    ~PacketQueueCloseOnExit()
    {
      if (track_)
      {
        track_->packetQueue().close();
      }
    }
  };

  //----------------------------------------------------------------
  // Movie::threadLoop
  //
  void
  Movie::threadLoop()
  {
    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      videoTrack = videoTracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audioTracks_.size())
    {
      audioTrack = audioTracks_[selectedAudioTrack_];
    }

    PacketQueueCloseOnExit videoCloseOnExit(videoTrack);
    PacketQueueCloseOnExit audioCloseOnExit(audioTrack);

    AVPacket ffmpeg;
    try
    {
      int err = 0;
      while (true)
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
        {
          boost::lock_guard<boost::mutex> lock(mutex_);

          if (mustSeek_)
          {
            err = seekTo(seekTime_);
            mustSeek_ = false;
          }

          if (!err)
          {
            err = av_read_frame(context_, &ffmpeg);
          }
        }

        if (err)
        {
          av_free_packet(&ffmpeg);

          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          if (!playbackInterval_)
          {
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          // done:
          if (audioTrack)
          {
            audioTrack->packetQueue().waitForConsumerToBlock();
            audioTrack->frameQueue_.waitForConsumerToBlock();
          }

          if (videoTrack)
          {
            videoTrack->packetQueue().waitForConsumerToBlock();
            videoTrack->frameQueue_.waitForConsumerToBlock();
          }

          break;
        }

        if (ffmpeg.dts != AV_NOPTS_VALUE)
        {
          // keep track of current DTS, so that we would know which way to seek
          // relative to the current position (back/forth)
          const AVStream * stream = context_->streams[ffmpeg.stream_index];
          const AVRational & timebase = stream->time_base;
          TTime dts(ffmpeg.dts * timebase.num, timebase.den);

          dts_ = dts;
          dtsStreamIndex_ = ffmpeg.stream_index;
        }

        TPacketPtr packet = copyPacket(ffmpeg);
        if (packet)
        {
          if (videoTrack &&
              videoTrack->streamIndex() == ffmpeg.stream_index)
          {
            // if (!videoTrack->decode(packet))
            if (!videoTrack->packetQueue().push(packet, &terminator_))
            {
              break;
            }
          }
          else if (audioTrack &&
                   audioTrack->streamIndex() == ffmpeg.stream_index)
          {
            // if (!audioTrack->decode(packet))
            if (!audioTrack->packetQueue().push(packet, &terminator_))
            {
              break;
            }
          }
        }
        else
        {
          av_free_packet(&ffmpeg);
        }
      }
    }
    catch (...)
    {
#if 0
      std::cerr << "\nMovie::threadLoop caught exception" << std::endl;
#endif
    }

    if (videoTrack)
    {
      // videoTrack->decoderShutdown();
    }

    if (audioTrack)
    {
      // audioTrack->decoderShutdown();
    }
  }

  //----------------------------------------------------------------
  // Movie::threadStart
  //
  bool
  Movie::threadStart()
  {
    if (!context_)
    {
      return false;
    }

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      // t->decoderStartup();
      t->threadStart();
      t->packetQueue().waitForConsumerToBlock();
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      // t->decoderStartup();
      t->threadStart();
      t->packetQueue().waitForConsumerToBlock();
    }

    terminator_.stopWaiting(false);
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Movie::threadStop
  //
  bool
  Movie::threadStop()
  {
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      t->threadStop();
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->threadStop();
    }

    terminator_.stopWaiting(true);
    thread_.stop();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Movie::requestSeekTime
  //
  bool
  Movie::requestSeekTime(double seekTime)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      mustSeek_ = true;
      seekTime_ = seekTime;

      VideoTrackPtr videoTrack;
      AudioTrackPtr audioTrack;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->packetQueue().clear();
        videoTrack->frameQueue_.clear();
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->packetQueue().clear();
        audioTrack->frameQueue_.clear();
      }

      return true;
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::seekTo
  //
  int
  Movie::seekTo(double seekTime)
  {
    if (!context_)
    {
      return -1;
    }

    double tCurr = dts_.toSeconds();
    int seekFlags = seekTime < tCurr ? AVSEEK_FLAG_BACKWARD : 0;

    int64_t ts = int64_t(seekTime * double(AV_TIME_BASE));
    int err = avformat_seek_file(context_,
                                 -1,
                                 kMinInt64,
                                 ts,
                                 kMaxInt64,
                                 seekFlags);
    if (err < 0)
    {
      seekFlags |= AVSEEK_FLAG_ANY;
      err = avformat_seek_file(context_,
                               -1,
                               kMinInt64,
                               ts,
                               kMaxInt64,
                               seekFlags);
    }

    if (err < 0)
    {
#if 1
      std::cerr << "Movie::seek(" << seekTime << ") returned " << err
                << std::endl;
#endif
      return err;
    }

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
      err = videoTrack->resetTimeCounters(seekTime);
    }

    if (!err && selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
      err = audioTrack->resetTimeCounters(seekTime);
    }

    return err;
  }

  //----------------------------------------------------------------
  // Movie::rewind
  //
  int
  Movie::rewind(const AudioTrackPtr & audioTrack,
                const VideoTrackPtr & videoTrack,
                bool seekToTimeIn)
  {
    // wait for the the frame queues to empty out:
    if (audioTrack)
    {
      audioTrack->packetQueue().waitForConsumerToBlock();
      audioTrack->frameQueue_.waitForConsumerToBlock();
    }
    else if (videoTrack)
    {
      videoTrack->packetQueue().waitForConsumerToBlock();
      videoTrack->frameQueue_.waitForConsumerToBlock();
    }

    boost::lock_guard<boost::mutex> lock(mutex_);
    double seekTime = seekToTimeIn ? timeIn_ : 0.0;
    return seekTo(seekTime);
  }

  //----------------------------------------------------------------
  // Movie::getPlaybackInterval
  //
  void
  Movie::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeIn = timeIn_;
      timeOut = timeOut_;
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalStart
  //
  void
  Movie::setPlaybackIntervalStart(double timeIn)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeIn_ = timeIn;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalEnd
  //
  void
  Movie::setPlaybackIntervalEnd(double timeOut)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeOut_ = timeOut;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackInterval
  //
  void
  Movie::setPlaybackInterval(bool enabled)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      playbackInterval_ = enabled;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackLooping
  //
  void
  Movie::setPlaybackLooping(bool enabled)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      looping_ = enabled;
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::skipLoopFilter
  //
  void
  Movie::skipLoopFilter(bool skip)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      skipLoopFilter_ = skip;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->skipLoopFilter(skipLoopFilter_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::skipNonReferenceFrames
  //
  void
  Movie::skipNonReferenceFrames(bool skip)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      skipNonReferenceFrames_ = skip;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->skipNonReferenceFrames(skipNonReferenceFrames_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setTempo
  //
  bool
  Movie::setTempo(double tempo)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);

      // first set audio tempo -- this may fail:
      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        if (!audioTrack->setTempo(tempo))
        {
          return false;
        }
      }

      // then set video tempo -- this can't fail:
      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        return videoTrack->setTempo(tempo);
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::subsCount
  //
  std::size_t
  Movie::subsCount() const
  {
    return subs_.size();
  }

  //----------------------------------------------------------------
  // Movie::subsInfo
  //
  const char *
  Movie::subsInfo(std::size_t i, TSubsFormat * t) const
  {
    std::size_t nsubs = subs_.size();
    if (i >= nsubs)
    {
      if (t)
      {
        *t = kSubsNone;
      }

      return NULL;
    }

    const SubtitlesInfo & subs = subs_[i];

    if (t)
    {
      *t = subs.format_;
    }

    return &subs.title_[0];
  }

  //----------------------------------------------------------------
  // Movie::subsRender
  //
  void
  Movie::subsRender(std::size_t i, bool render)
  {
    std::size_t nsubs = subs_.size();
    if (i < nsubs)
    {
      SubtitlesInfo & subs = subs_[i];
      subs.render_ = render;
    }
  }


  //----------------------------------------------------------------
  // ReaderFFMPEG::Private
  //
  class ReaderFFMPEG::Private
  {
  private:
    // intentionally disabled:
    Private(const Private &);
    Private & operator = (const Private &);

    // flag indicating whether av_register_all has been called already:
    static bool ffmpegInitialized_;

  public:
    Private():
      frame_(NULL)
    {
      if (!ffmpegInitialized_)
      {
        av_log_set_flags(AV_LOG_SKIP_REPEATED);
#if CONFIG_AVDEVICE
        avdevice_register_all();
#endif
#if CONFIG_AVFILTER
        avfilter_register_all();
#endif
        av_register_all();

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 13, 0)
        avformat_network_init();
#endif

        av_lockmgr_register(&lockManager);
        ffmpegInitialized_ = true;
      }
    }

    Movie movie_;
    AVFrame * frame_;
    AVPacket packet_;
  };

  //----------------------------------------------------------------
  // ReaderFFMPEG::Private::ffmpegInitialized_
  //
  bool
  ReaderFFMPEG::Private::ffmpegInitialized_ = false;


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
  // ReaderFFMPEG::getName
  //
  const char *
  ReaderFFMPEG::getName() const
  {
    return typeid(*this).name();
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
  const char *
  ReaderFFMPEG::getSelectedVideoTrackName() const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getName();
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackName
  //
  const char *
  ReaderFFMPEG::getSelectedAudioTrackName() const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->getName();
    }

    return NULL;
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
    return ok;
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
  // ReaderFFMPEG::setPlaybackInterval
  //
  void
  ReaderFFMPEG::setPlaybackInterval(bool enabled)
  {
    private_->movie_.setPlaybackInterval(enabled);
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
  const char *
  ReaderFFMPEG::subsInfo(std::size_t i, TSubsFormat * t) const
  {
    return private_->movie_.subsInfo(i, t);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsRender
  //
  void
  ReaderFFMPEG::subsRender(std::size_t i, bool render)
  {
    private_->movie_.subsRender(i, render);
  }
}
