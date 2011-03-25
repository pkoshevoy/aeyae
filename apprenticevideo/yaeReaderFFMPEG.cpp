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


//----------------------------------------------------------------
// fileUtf8
// 
namespace fileUtf8
{
  
  //----------------------------------------------------------------
  // kProtocolName
  // 
  const char * kProtocolName = "fileUtfEight";
  
  //----------------------------------------------------------------
  // urlGetFileHandle
  // 
  static void
  urlSetFileHandle(URLContext * h, int fd)
  {
    h->priv_data = (void *)(size_t)fd;
  }
  
  //----------------------------------------------------------------
  // urlGetFileHandle
  // 
  static int
  urlGetFileHandle(URLContext * h)
  {
    int fd = (size_t)h->priv_data;
    return fd;
  }
  
  //----------------------------------------------------------------
  // urlOpen
  // 
  static int
  urlOpen(URLContext * h, const char * url, int flags)
  {
    const char * filename = url;
    av_strstart(url, kProtocolName, &filename);
    av_strstart(filename, "://", &filename);
    
    int accessMode = 0;
    if (flags & URL_RDWR)
    {
        accessMode = O_CREAT | O_TRUNC | O_RDWR;
    }
    else if (flags & URL_WRONLY)
    {
        accessMode = O_CREAT | O_TRUNC | O_WRONLY;
    }
    else
    {
        accessMode = O_RDONLY;
    }
    
#ifdef _WIN32
    accessMode |= (_O_BINARY | _O_SEQUENTIAL);
#endif
    
    int permissions = 0666;
    int fd = yae::fileOpenUtf8(filename, accessMode, permissions);
    if (fd < 0)
    {
      return AVERROR(errno);
    }
    
    urlSetFileHandle(h, fd);
    return 0;
  }
  
  //----------------------------------------------------------------
  // urlRead
  // 
  static int
  urlRead(URLContext * h, unsigned char * buf, int size)
  {
    int fd = urlGetFileHandle(h);

#ifdef _WIN32
    int nb = _read(fd, buf, size);
#else
    int nb = read(fd, buf, size);
#endif

    if (nb < 0)
    {
      std::cerr
        << "read(" << fd << ", " << buf << ", " << size << ") "
        << "failed, error: " << errno << " - " << strerror(errno)
        << std::endl;
    }
    
    return nb;
  }
  
  //----------------------------------------------------------------
  // urlWrite
  // 
  static int
  urlWrite(URLContext * h,
#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52 && \
                                       LIBAVFORMAT_VERSION_MINOR >= 68)
           const unsigned char * buf,
#else
           unsigned char * buf,
#endif
           int size)
  {
    int fd = urlGetFileHandle(h);
    return write(fd, buf, size);
  }
  
  //----------------------------------------------------------------
  // urlSeek
  // 
  static int64_t
  urlSeek(URLContext * h, int64_t pos, int whence)
  {
    int fd = urlGetFileHandle(h);
    if (whence == AVSEEK_SIZE)
    {
      int64_t size = yae::fileSize64(fd);
      if (size < 0)
      {
        return AVERROR(errno);
      }
      
      return size;
    }

    return yae::fileSeek64(fd, pos, whence);
  }
  
  //----------------------------------------------------------------
  // urlClose
  // 
  static int
  urlClose(URLContext * h)
  {
    int fd = urlGetFileHandle(h);
    return close(fd);
  }
  
  //----------------------------------------------------------------
  // urlProtocol
  // 
  static URLProtocol urlProtocol =
  {
    kProtocolName,
    &urlOpen,
    &urlRead,
    &urlWrite,
    &urlSeek,
    &urlClose,
    0, // next
    0, // url_read_pause
    0, // url_read_seek
    &urlGetFileHandle,
#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52 && \
                                       LIBAVFORMAT_VERSION_MINOR >= 69)
    0, // priv_data_size
    0, // priv_data_class
#endif
  };
  
}


namespace yae
{
  
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
    void getDuration(TTime & t) const;
    
    // get current position:
    virtual bool getPosition(TTime &) const
    { return false; }
    
    // seek to a position:
    virtual bool setPosition(const TTime &)
    { return false; }
    
    // accessor to the packet queue:
    inline TPacketQueue & packetQueue()
    { return packetQueue_; }
    
    // packet decoding thread:
    virtual void threadLoop() {}
    virtual bool threadStart();
    virtual bool threadStop();
    
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
    
    AVFormatContext * context_;
    AVStream * stream_;
    AVCodec * codec_;
    TPacketQueue packetQueue_;
    PacketTime packetTime_;
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
    packetQueue_(120)
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
      close();
      return false;
    }
    
    if (stream_->duration == int64_t(AV_NOPTS_VALUE) &&
        context_->duration == int64_t(AV_NOPTS_VALUE))
    {
      // unknown duration:
      close();
      return false;
    }

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
  // Track::getName
  // 
  const char *
  Track::getName() const
  {
    if (!stream_)
    {
      return NULL;
    }
    
    AVMetadataTag * name = av_metadata_get(stream_->metadata,
                                           "name",
                                           NULL,
                                           0);
    if (name)
    {
      return name->value;
    }
    
    AVMetadataTag * title = av_metadata_get(stream_->metadata,
                                            "title",
                                            NULL,
                                            0);
    if (title)
    {
      return title->value;
    }
    
    AVMetadataTag * lang = av_metadata_get(stream_->metadata,
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
  // Track::getDuration
  // 
  void
  Track::getDuration(TTime & t) const
  {
    if (!stream_)
    {
      YAE_ASSERT(false);
      return;
    }
    
    if (context_ && stream_->duration == int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      t.time_ = context_->duration;
      t.base_ = AV_TIME_BASE;
      return;
    }
    
    // return track duration:
    t.time_ = stream_->time_base.num * stream_->duration;
    t.base_ = stream_->time_base.den;
  }
  
  //----------------------------------------------------------------
  // Track::threadStart
  // 
  bool
  Track::threadStart()
  {
    packetQueue_.open();
    return thread_.run();
  }
  
  //----------------------------------------------------------------
  // Track::threadStop
  // 
  bool
  Track::threadStop()
  {
    packetQueue_.close();
    thread_.stop();
    return thread_.wait();
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
  // VideoTrack
  // 
  struct VideoTrack : public Track
  {
    VideoTrack(AVFormatContext * context, AVStream * stream);
    
    // virtual:
    bool open();
    
    // virtual: get current position:
    bool getPosition(TTime & t) const;
    
    // virtual: seek to a position:
    bool setPosition(const TTime & t);
    
    // virtual:
    void threadLoop();
    bool threadStop();
    
    // video traits, not overridden:
    bool getTraits(VideoTraits & traits) const;
    
    // use this for video frame conversion (pixel format and size)
    bool setTraitsOverride(const VideoTraits & override);
    bool getTraitsOverride(VideoTraits & override) const;
    
    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TVideoFramePtr & frame);
    bool getNextFrameDontWait(TVideoFramePtr & frame);
    
  protected:
    TVideoFrameQueue frameQueue_;
    VideoTraits override_;
    uint64 framesDecoded_;
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
    frameQueue_(30)
  {
    YAE_ASSERT(stream->codec->codec_type == CODEC_TYPE_VIDEO);

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
      getTraits(override_);
      framesDecoded_ = 0;
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // VideoTrack::getPosition
  // 
  bool
  VideoTrack::getPosition(TTime & t) const
  {
    TVideoFramePtr curr;
    if (frameQueue_.peekHead(curr))
    {
      t = curr->time_;
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // VideoTrack::setPosition
  // 
  bool
  VideoTrack::setPosition(const TTime & t)
  {
    // FIXME:
    return false;
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
    
  private:
    // intentionally disabled:
    FrameWithAutoCleanup(const FrameWithAutoCleanup &);
    FrameWithAutoCleanup & operator = (const FrameWithAutoCleanup &);
    
    AVFrame * frame_;
  };

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
  verifyPTS(bool gotPrevPTS, const TTime & prevPTS, const TTime & nextPTS)
  {
    return (!gotPrevPTS ||
            (prevPTS.base_ == nextPTS.base_ ?
             prevPTS.time_ < nextPTS.time_ :
             prevPTS.toSeconds() < nextPTS.toSeconds()));
  }

  //----------------------------------------------------------------
  // VideoTrack::threadLoop
  // 
  void
  VideoTrack::threadLoop()
  {
    TOpenHere<TVideoFrameQueue> closeOnExit(frameQueue_);
    
    FrameWithAutoCleanup frameAutoCleanup;
    struct SwsContext * imgConvertCtx = NULL;
    
    // shortcut to native frame format traits:
    VideoTraits nativeTraits;
    getTraits(nativeTraits);
    
    // pixel format shortcut:
    TPixelFormatId yaeNativeFormat = nativeTraits.pixelFormat_;
    TPixelFormatId yaeOutputFormat = override_.pixelFormat_;
    if (yaeOutputFormat == kPixelFormatY400A &&
        yaeNativeFormat != kPixelFormatY400A)
    {
      // sws_getContext doesn't support Y400A, so drop the alpha channel:
      yaeOutputFormat = kPixelFormatGRAY8;
    }
    
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(yaeOutputFormat);
    if (!ptts)
    {
      YAE_ASSERT(false);
      return;
    }
    
    // get number of contiguous sample planes,
    // sample set stride (in bits) for each plane:
    unsigned char samplePlaneStride[4] = { 0 };
    unsigned char numSamplePlanes = ptts->getPlanes(samplePlaneStride);
    
    unsigned int encodedWidth = override_.encodedWidth_;
    if (ptts->chromaBoxW_ > 1)
    {
      unsigned int remainder = encodedWidth % ptts->chromaBoxW_;
      if (remainder)
      {
        encodedWidth += ptts->chromaBoxW_ - remainder;
      }
    }
    
    unsigned int encodedHeight = override_.encodedHeight_;
    if (ptts->chromaBoxH_ > 1)
    {
      unsigned int remainder = encodedHeight % ptts->chromaBoxH_;
      if (remainder)
      {
        encodedHeight += ptts->chromaBoxH_ - remainder;
      }
    }
    
    // calculate number of bytes for each sample plane:
    std::size_t totalPixels = encodedWidth * encodedHeight;
    
    std::size_t samplePlaneSize[4] = { 0 };
    samplePlaneSize[0] = totalPixels * samplePlaneStride[0] / 8;
    
    std::size_t sampleLineSize[4] = { 0 };
    sampleLineSize[0] = encodedWidth * samplePlaneStride[0] / 8;

    for (unsigned char i = 1; i < numSamplePlanes; i++)
    {
      samplePlaneSize[i] = totalPixels * samplePlaneStride[i] / 8;
      sampleLineSize[i] = encodedWidth * samplePlaneStride[i] / 8;
    }
    
    // account for sub-sampling of UV plane(s):
    std::size_t chromaBoxArea = ptts->chromaBoxW_ * ptts->chromaBoxH_;
    if (chromaBoxArea > 1)
    {
      unsigned char uvSamplePlanes =
        (ptts->flags_ & pixelFormat::kAlpha) ?
        numSamplePlanes - 2 :
        numSamplePlanes - 1;
      
      for (unsigned char i = 1; i < 1 + uvSamplePlanes; i++)
      {
        samplePlaneSize[i] /= chromaBoxArea;
        sampleLineSize[i] /= ptts->chromaBoxW_;
      }
    }

    int64_t startTime = stream_->start_time;
    if (startTime == AV_NOPTS_VALUE)
    {
      startTime = 0;
    }
    else if (startTime > INT64_C(0xFFFFFFFF))
    {
      // probably an integer overflow error, check it:
      startTime = (int32_t)startTime;

      double frameDuration =
        double(stream_->time_base.den) /
        (double(stream_->time_base.num) * nativeTraits.frameRate_);
      
      if (-double(startTime) > 3.0 * frameDuration)
      {
        // the start time is not within 3 frames near zero,
        // assume it's just broken and reset to zero:
        startTime = 0;
      }
    }
    
    // shortcut to the frame rate:
    AVRational frameRate =
      (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den) ?
      stream_->avg_frame_rate :
      stream_->r_frame_rate;
    
    // shortcut for ffmpeg pixel format:
    enum PixelFormat ffmpegPixelFormat = yae_to_ffmpeg(yaeOutputFormat);

    // it appears ffmpeg outputs decoded frames in correct presentation order
    // but without the presentation time stamps (PTS), therefore I will
    // keep a sorted list of packet time stamps for all packets going
    // into avcodec_decode_video2 and use the earliest time stamp
    // from that list to assign PTS to the decoded frames.
    std::list<PacketTime> packetTimes;
    
    TTime prevPTS;
    bool gotPrevPTS = false;
    
    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();
        
        TPacketPtr packetPtr;
        bool ok = packetQueue_.pop(packetPtr);
        if (!ok)
        {
          break;
        }

        // make a local shallow copy of the packet:
        AVPacket packet = packetPtr->ffmpeg_;
#if 0
        std::cerr << "\n0. in  pts: " << packetTime_.pts_
                  << ", dts: " << packetTime_.dts_
                  << ", len: " << packetTime_.duration_ << std::endl;
#endif
        
        // save packet timestamps and duration so we can capture
        // and re-use this to timestamp the decoded frame:
        savePacketTime(packet);
        if (packetTime_.pts_ != AV_NOPTS_VALUE)
        {
          insertPacketTime(packetTimes, packetTime_);
        }
        
        // Decode video frame
        int gotPicture = 0;
        AVFrame * avFrame = frameAutoCleanup;
        AVCodecContext * codecContext = this->codecContext();
        avcodec_decode_video2(codecContext,
                              avFrame,
                              &gotPicture,
                              &packet);
        if (!gotPicture)
        {
          continue;
        }
        
        framesDecoded_++;
        TVideoFramePtr vfPtr(new TVideoFrame());
        TVideoFrame & vf = *vfPtr;
        vf.time_.base_ = stream_->time_base.den;

        // shortcut to the saved packet time associated with this frame:
        const PacketTime * frameTime = (PacketTime *)(avFrame->opaque);
        
        PacketTime t;
        if (!packetTimes.empty())
        {
          t = packetTimes.back();
          packetTimes.pop_back();
#if 0
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
        
        if (!gotPTS &&
            framesDecoded_ == 1)
        {
          vf.time_.time_ = startTime;
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
        }
        
        if (!gotPTS &&
            frameTime &&
            frameTime->pts_ != AV_NOPTS_VALUE)
        {
          vf.time_.time_ = stream_->time_base.num * frameTime->pts_;
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
        }
        
        if (!gotPTS &&
            avFrame->pts != AV_NOPTS_VALUE &&
            codecContext->time_base.num != AV_NOPTS_VALUE &&
            codecContext->time_base.den != AV_NOPTS_VALUE &&
            codecContext->time_base.num &&
            codecContext->time_base.den)
        {
          vf.time_.time_ = avFrame->pts * codecContext->time_base.num;
          vf.time_.base_ = codecContext->time_base.den;
          
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
        }
        
        if (!gotPTS &&
            frameRate.num &&
            frameRate.den)
        {
          vf.time_.time_ =
            startTime +
            (framesDecoded_ - 1) *
            (stream_->time_base.den * frameRate.den) /
            (stream_->time_base.num * frameRate.num);
          
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
          
          if (!gotPTS && gotPrevPTS)
          {
            // increment by average frame duration:
            vf.time_ = prevPTS;
            vf.time_ += TTime(frameRate.den,
                              frameRate.num);
            
            gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
          }
        }

        if (!gotPTS &&
            t.pts_ != AV_NOPTS_VALUE)
        {
          vf.time_.time_ = stream_->time_base.num * t.pts_;
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
        }
        
        YAE_ASSERT(gotPTS);
        if (!gotPTS && gotPrevPTS)
        {
          vf.time_ = prevPTS;
          vf.time_.time_++;
          
          gotPTS = verifyPTS(gotPrevPTS, prevPTS, vf.time_);
        }
        
        YAE_ASSERT(gotPTS);
        if (gotPTS)
        {
          if (gotPrevPTS)
          {
            double ta = prevPTS.toSeconds();
            double tb = vf.time_.toSeconds();
            double dt = tb - ta;
            double fd = 1.0 / nativeTraits.frameRate_;
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
          
          gotPrevPTS = true;
          prevPTS = vf.time_;
        }
        
        vf.traits_ = override_;
        vf.traits_.encodedWidth_ = encodedWidth;
        vf.traits_.encodedHeight_ = encodedHeight;
        vf.traits_.pixelFormat_ = yaeOutputFormat;
        
        TSampleBufferPtr sampleBuffer(new TSampleBuffer(numSamplePlanes),
                                      &ISampleBuffer::deallocator);
        for (unsigned char i = 0; i < numSamplePlanes; i++)
        {
          std::size_t rowBytes = sampleLineSize[i];
          std::size_t rows = samplePlaneSize[i] / rowBytes;
          sampleBuffer->resize(i, rowBytes, rows, 16);
        }
        vf.sampleBuffer_ = sampleBuffer;

        if (nativeTraits.pixelFormat_ == yaeOutputFormat &&
            nativeTraits.encodedWidth_ == encodedWidth &&
            nativeTraits.encodedHeight_ == encodedHeight)
        {
          // copy the sample planes:
          
          for (unsigned char i = 0; i < numSamplePlanes; i++)
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
          for (unsigned char i = 0; i < numSamplePlanes; i++)
          {
            pict.data[i] = sampleBuffer->samples(i);
            pict.linesize[i] = sampleBuffer->rowBytes(i);
          }
          
          if (imgConvertCtx == NULL)
          {
            imgConvertCtx = sws_getContext(// from:
                                           nativeTraits.encodedWidth_,
                                           nativeTraits.encodedHeight_, 
                                           codecContext->pix_fmt,
                                           
                                           // to:
                                           vf.traits_.encodedWidth_,
                                           vf.traits_.encodedHeight_,
                                           ffmpegPixelFormat,
                                           
                                           SWS_BICUBIC,
                                           NULL,
                                           NULL,
                                           NULL);
            if (imgConvertCtx == NULL)
            {
              YAE_ASSERT(false);
              break;
            }
          }
          
          sws_scale(imgConvertCtx,
                    avFrame->data,
                    avFrame->linesize,
                    0,
                    codecContext->height,
                    pict.data,
                    pict.linesize);
        }
        
        // put the output frame into frame queue:
        frameQueue_.push(vfPtr);

        // put repeated output frames into frame queue:
        for (int i = 0; i < avFrame->repeat_pict; i++)
        {
          TVideoFramePtr rvfPtr(new TVideoFrame(vf));
          TVideoFrame & rvf = *rvfPtr;
          
          if (frameRate.num && frameRate.den)
          {
            rvf.time_ += TTime((i + 1) * frameRate.den, frameRate.num);
          }
          else
          {
            rvf.time_.time_++;
          }
          
          std::cerr << "frame repeated at " << rvf.time_.toSeconds() << " sec"
                    << std::endl;
          frameQueue_.push(rvfPtr);
        }
      }
      catch (...)
      {
        break;
      }
    }
    
    if (imgConvertCtx)
    {
      sws_freeContext(imgConvertCtx);
      imgConvertCtx = NULL;
    }
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
    
    return true;
  }
  
  //----------------------------------------------------------------
  // VideoTrack::setTraitsOverride
  // 
  bool
  VideoTrack::setTraitsOverride(const VideoTraits & override)
  {
    override_ = override;
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
  VideoTrack::getNextFrame(TVideoFramePtr & frame)
  {
    return frameQueue_.pop(frame);
  }

  //----------------------------------------------------------------
  // VideoTrack::getNextFrameDontWait
  // 
  bool
  VideoTrack::getNextFrameDontWait(TVideoFramePtr & frame)
  {
    return frameQueue_.tryPop(frame);
  }
  
  //----------------------------------------------------------------
  // AudioTrack
  // 
  struct AudioTrack : public Track
  {
    AudioTrack(AVFormatContext * context, AVStream * stream);
    
    // virtual:
    bool open();
    
    // virtual: get current position:
    bool getPosition(TTime & t) const;
    
    // virtual: seek to a position:
    bool setPosition(const TTime & t);
    
    // virtual:
    void threadLoop();
    bool threadStop();
    
    // audio traits, not overridden:
    bool getTraits(AudioTraits & traits) const;
    
    // use this for audio format conversion (sample rate, channels, etc...)
    bool setTraitsOverride(const AudioTraits & override);
    bool getTraitsOverride(AudioTraits & override) const;
    
    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TAudioFramePtr & frame);
    bool getNextFrameDontWait(TAudioFramePtr & frame);
    
  protected:
    TAudioFrameQueue frameQueue_;
    AudioTraits override_;
    uint64 samplesDecoded_;
  };
  
  //----------------------------------------------------------------
  // AudioTrackPtr
  // 
  typedef boost::shared_ptr<AudioTrack> AudioTrackPtr;
  
  //----------------------------------------------------------------
  // AudioTrack::AudioTrack
  // 
  AudioTrack::AudioTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream)
  {
    YAE_ASSERT(stream->codec->codec_type == CODEC_TYPE_AUDIO);
    
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
      getTraits(override_);
      samplesDecoded_ = 0;
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // AudioTrack::getPosition
  // 
  bool
  AudioTrack::getPosition(TTime & t) const
  {
    TAudioFramePtr curr;
    if (frameQueue_.peekHead(curr))
    {
      t = curr->time_;
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // AudioTrack::setPosition
  // 
  bool
  AudioTrack::setPosition(const TTime & t)
  {
    // FIXME:
    return false;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  // 
  static enum SampleFormat
  yae_to_ffmpeg(TAudioSampleFormat yaeSampleFormat)
  {
    switch (yaeSampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return SAMPLE_FMT_U8;

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        return SAMPLE_FMT_S16;

      case kAudio32BitFloat:
        return SAMPLE_FMT_FLT;

      default:
        break;
    }
    
    return SAMPLE_FMT_NONE;
  }
  
  //----------------------------------------------------------------
  // AudioTrack::threadLoop
  // 
  void
  AudioTrack::threadLoop()
  {
    TOpenHere<TAudioFrameQueue> closeOnExit(frameQueue_);
    
    TAudioFrame::TTraits nativeTraits;
    getTraits(nativeTraits);
    
    // declare a 16-byte aligned buffer for decoded audio samples:
    DECLARE_ALIGNED(16, uint8_t, buffer)
      [(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

    int nativeChannels = getNumberOfChannels(nativeTraits.channelLayout_);
    int nativeSampleRate = nativeTraits.sampleRate_;
    enum SampleFormat nativeFormat = yae_to_ffmpeg(nativeTraits.sampleFormat_);
    
    int outputChannels = getNumberOfChannels(override_.channelLayout_);
    int outputSampleRate = override_.sampleRate_;
    enum SampleFormat outputFormat = yae_to_ffmpeg(override_.sampleFormat_);
    
    unsigned int bytesPerSample =
      nativeChannels *
      getBitsPerSample(nativeTraits.sampleFormat_) / 8;
    
    ReSampleContext * resampleCtx = NULL;
    
    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();
        
        TPacketPtr packetPtr;
        bool ok = packetQueue_.pop(packetPtr);
        if (!ok)
        {
          break;
        }
        
        // make a local shallow copy of the packet:
        AVPacket packet = packetPtr->ffmpeg_;
        
        // save packet timestamps and duration so we can capture
        // and re-use this to timestamp the decoded frame:
        // savePacketTime(packet);
        
        // Decode audio frame, piecewise:
        std::list<std::vector<unsigned char> > chunks;
        std::size_t totalBytes = 0;
        
        while (packet.size)
        {
          int bufferSize = sizeof(buffer);
          int bytesUsed = avcodec_decode_audio3(codecContext(),
                                                (int16_t *)(&buffer[0]),
                                                &bufferSize,
                                                &packet);
          
          if (bytesUsed < 0)
          {
            break;
          }
          
          // adjust the packet (the copy, not the original):
          packet.size -= bytesUsed;
          packet.data += bytesUsed;
          
          if (bufferSize)
          {
            chunks.push_back(std::vector<unsigned char>
                             (buffer, buffer + bufferSize));
          
            totalBytes += bufferSize;
          }
        }
        
        if (!totalBytes)
        {
          continue;
        }

        std::size_t numSamples = totalBytes / bytesPerSample;
        samplesDecoded_ += numSamples;

        // FIXME: add support for resampling to support override_:
        if (false && !resampleCtx)
        {
          resampleCtx = av_audio_resample_init(outputChannels,
                                               nativeChannels,
                                               outputSampleRate,
                                               nativeSampleRate,
                                               outputFormat,
                                               nativeFormat,
                                               16, // taps
                                               10, // log2 phase count
                                               0, // linear
                                               0.8); // cutoff frequency
        }
        
        TAudioFramePtr afPtr(new TAudioFrame());
        TAudioFrame & af = *afPtr;
        
        af.traits_ = override_;
        af.time_.base_ = stream_->time_base.den;
        
        if (packet.pts != AV_NOPTS_VALUE)
        {
          af.time_.time_ = stream_->time_base.num * packet.pts;
        }
        else if (packet.dts != AV_NOPTS_VALUE)
        {
          af.time_.time_ = stream_->time_base.num * packet.dts;
        }
        else
        {
          af.time_.time_ = samplesDecoded_ - numSamples;
          af.time_.base_ = nativeTraits.sampleRate_;
        }
        
        TSampleBufferPtr sampleBuffer(new TSampleBuffer(1),
                                      &ISampleBuffer::deallocator);
        af.sampleBuffer_ = sampleBuffer;
        sampleBuffer->resize(0, totalBytes, 1, 16);
        unsigned char * afSampleBuffer = sampleBuffer->samples(0);
        
        // concatenate chunks into a contiguous frame buffer:
        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();
          memcpy(afSampleBuffer, chunk, chunkSize);
          
          afSampleBuffer += chunkSize;
          chunks.pop_front();
        }
        
        // put the decoded frame into frame queue:
        frameQueue_.push(afPtr);
      }
      catch (...)
      {
        break;
      }
    }
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
      case SAMPLE_FMT_U8:
        t.sampleFormat_ = kAudio8BitOffsetBinary;
        break;
        
      case SAMPLE_FMT_S16:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio16BitBigEndian;
#else
        t.sampleFormat_ = kAudio16BitLittleEndian;
#endif
        break;
        
      case SAMPLE_FMT_FLT:
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
    
    return true;
  }
  
  //----------------------------------------------------------------
  // AudioTrack::setTraitsOverride
  // 
  bool
  AudioTrack::setTraitsOverride(const AudioTraits & override)
  {
    override_ = override;
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
  AudioTrack::getNextFrame(TAudioFramePtr & frame)
  {
    return frameQueue_.pop(frame);
  }
  
  //----------------------------------------------------------------
  // AudioTrack::getNextFrameDontWait
  // 
  bool
  AudioTrack::getNextFrameDontWait(TAudioFramePtr & frame)
  {
    return frameQueue_.tryPop(frame);
  }
  
  
  //----------------------------------------------------------------
  // Movie
  // 
  struct Movie
  {
    Movie();
    
    // NOTE: destructor will close the movie:
    ~Movie();
    
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
    
  private:
    // intentionally disabled:
    Movie(const Movie &);
    Movie & operator = (const Movie &);
    
  protected:
    // worker thread:
    Thread<Movie> thread_;
    
    AVFormatContext * context_;
    
    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    
    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;
  };
  
  
  //----------------------------------------------------------------
  // Movie::Movie
  // 
  Movie::Movie():
    thread_(this),
    context_(NULL),
    selectedVideoTrack_(0),
    selectedAudioTrack_(0)
  {}
  
  //----------------------------------------------------------------
  // Movie::~Movie
  // 
  Movie::~Movie()
  {
    close();
  }
  
  //----------------------------------------------------------------
  // Movie::open
  // 
  bool
  Movie::open(const char * resourcePath)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();
    
    int err = av_open_input_file(&context_,
                                 resourcePath,
                                 NULL, // AVInputFormat to force
                                 0,    // buffer size, 0 if default
                                 NULL);// additional parameters
    if (err != 0)
    {
      close();
      return false;
    }
    
    // spend at most 2 seconds trying to analyze the file:
    context_->max_analyze_duration = 30 * AV_TIME_BASE;
    
    err = av_find_stream_info(context_);
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
      if (codecType == CODEC_TYPE_VIDEO)
      {
        videoTracks_.push_back(VideoTrackPtr(new VideoTrack(context_,
                                                            stream)));
      }
      else if (codecType == CODEC_TYPE_AUDIO)
      {
        audioTracks_.push_back(AudioTrackPtr(new AudioTrack(context_,
                                                            stream)));
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
    
    av_close_input_file(context_);
    context_ = NULL;
  }
  
  //----------------------------------------------------------------
  // Movie::selectVideoTrack
  // 
  bool
  Movie::selectVideoTrack(std::size_t i)
  {
    if (selectedVideoTrack_ == i)
    {
      // already selected:
      return true;
    }
    
    const std::size_t numVideoTracks = videoTracks_.size();
    if (selectedVideoTrack_ < numVideoTracks)
    {
      // close currently selected track:
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      t->close();
    }
    
    selectedVideoTrack_ = i;
    if (selectedVideoTrack_ >= numVideoTracks)
    {
      return false;
    }
    
    return videoTracks_[selectedVideoTrack_]->open();
  }
  
  //----------------------------------------------------------------
  // Movie::selectAudioTrack
  // 
  bool
  Movie::selectAudioTrack(std::size_t i)
  {
    if (selectedAudioTrack_ == i)
    {
      // already selected:
      return true;
    }
    
    const std::size_t numAudioTracks = audioTracks_.size();
    if (selectedAudioTrack_ < numAudioTracks)
    {
      // close currently selected track:
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->close();
    }
    
    selectedAudioTrack_ = i;
    if (selectedAudioTrack_ >= numAudioTracks)
    {
      return false;
    }
    
    return audioTracks_[selectedAudioTrack_]->open();
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
    
    // Read frames and save first five frames to disk
    AVPacket ffmpeg;
    while (true)
    {
      boost::this_thread::interruption_point();
      
      int err = av_read_frame(context_, &ffmpeg);
      if (err)
      {
        break;
      }
      
      TPacketPtr packet = copyPacket(ffmpeg);
      if (packet)
      {
        if (videoTrack &&
            videoTrack->streamIndex() == ffmpeg.stream_index)
        {
          if (!videoTrack->packetQueue().push(packet))
          {
            break;
          }
        }
        else if (audioTrack &&
                 audioTrack->streamIndex() == ffmpeg.stream_index)
        {
          if (!audioTrack->packetQueue().push(packet))
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
      t->threadStart();
    }
    
    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->threadStart();
    }
    
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
    
    thread_.stop();
    return thread_.wait();
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
        av_register_all();
#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52 && \
                                       LIBAVFORMAT_VERSION_MINOR >= 69)
        av_register_protocol2(&fileUtf8::urlProtocol,
                              sizeof(fileUtf8::urlProtocol));
#else
        av_register_protocol(&fileUtf8::urlProtocol);
#endif
        ffmpegInitialized_ = true;
      }
      
#ifdef DEBUG
      // display availabled IO protocols:
      URLProtocol * up = first_protocol;
      while (up != NULL)
      {
        std::cout << "protocol: " << up->name << std::endl;
        up = up->next;
      }
#endif
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
  ReaderFFMPEG::getVideoDuration(TTime & t) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      private_->movie_.getVideoTracks()[i]->getDuration(t);
      return true;
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioDuration
  // 
  bool
  ReaderFFMPEG::getAudioDuration(TTime & t) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      private_->movie_.getAudioTracks()[i]->getDuration(t);
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
  // ReaderFFMPEG::getVideoPosition
  // 
  bool
  ReaderFFMPEG::getVideoPosition(TTime & t) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getPosition(t);
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioPosition
  // 
  bool
  ReaderFFMPEG::getAudioPosition(TTime & t) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      // FIXME: if getPosition fails for a track,
      // perhaps it is possible to look at the format context
      // or some other track?
      return private_->movie_.getAudioTracks()[i]->getPosition(t);
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::setVideoPosition
  // 
  bool
  ReaderFFMPEG::setVideoPosition(const TTime & t)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->setPosition(t);
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::setAudioPosition
  // 
  bool
  ReaderFFMPEG::setAudioPosition(const TTime & t)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->setPosition(t);
    }
    
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::readVideo
  // 
  bool
  ReaderFFMPEG::readVideo(TVideoFramePtr & frame)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (private_->movie_.getVideoTracks().size() <= i)
    {
      return false;
    }
    
    VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
    return t->getNextFrame(frame);
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::readAudio
  // 
  bool
  ReaderFFMPEG::readAudio(TAudioFramePtr & frame)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (private_->movie_.getAudioTracks().size() <= i)
    {
      return false;
    }
    
    AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
    return t->getNextFrame(frame);
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::readVideoDontWait
  // 
  bool
  ReaderFFMPEG::readVideoDontWait(TVideoFramePtr & frame)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (private_->movie_.getVideoTracks().size() <= i)
    {
      return false;
    }
    
    VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
    return t->getNextFrameDontWait(frame);
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::readAudioDontWait
  // 
  bool
  ReaderFFMPEG::readAudioDontWait(TAudioFramePtr & frame)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (private_->movie_.getAudioTracks().size() <= i)
    {
      return false;
    }
    
    AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
    return t->getNextFrameDontWait(frame);
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
  
}
