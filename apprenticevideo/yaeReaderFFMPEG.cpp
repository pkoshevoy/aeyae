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
  // toPixelFormat
  // 
  static enum PixelFormat
  toPixelFormat(TVideoColorFormat colorFormat)
  {
    switch (colorFormat)
    {
      case kColorFormatI420Alpha:
        return PIX_FMT_YUVA420P;

      case kColorFormatI420:
        return PIX_FMT_YUV420P;

      case kColorFormatYUYV:
        return PIX_FMT_YUYV422;

      case kColorFormatRGB:
        return PIX_FMT_RGB24;

      case kColorFormatBGR:
        return PIX_FMT_BGR24;

      case kColorFormatARGB:
        return PIX_FMT_ARGB;

      case kColorFormatYUVJ420P:
        return PIX_FMT_YUVJ420P;

      case kColorFormatUYVY:
        return PIX_FMT_UYVY422;

      case kColorFormatBGRA:
        return PIX_FMT_BGRA;

      case kColorFormatYUVJ422P:
        return PIX_FMT_YUVJ422P;

      default:
        break;
    }

    assert(false);
    return PIX_FMT_NONE;
  }

  //----------------------------------------------------------------
  // toVideoColorFormat
  // 
  static TVideoColorFormat
  toVideoColorFormat(enum PixelFormat pixelFormat)
  {
    switch (pixelFormat)
    {
      case PIX_FMT_YUVA420P:
        return kColorFormatI420Alpha;
        
      case PIX_FMT_YUV420P:
        return kColorFormatI420;
        
      case PIX_FMT_YUYV422:
        return kColorFormatYUYV;
        
      case PIX_FMT_RGB24:
        return kColorFormatRGB;
        
      case PIX_FMT_BGR24:
        return kColorFormatBGR;
        
      case PIX_FMT_ARGB:
        return kColorFormatARGB;
        
      case PIX_FMT_YUVJ420P:
        return kColorFormatYUVJ420P;
        
      case PIX_FMT_UYVY422:
        return kColorFormatUYVY;
        
      case PIX_FMT_BGRA:
        return kColorFormatBGRA;

      case PIX_FMT_YUVJ422P:
        return kColorFormatYUVJ422P;
        
      default:
        break;
    }

    assert(false);
    return kInvalidColorFormat;
  }
  
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
    // worker thread:
    Thread<Track> thread_;
    
    AVFormatContext * context_;
    AVStream * stream_;
    AVCodec * codec_;
    TPacketQueue packetQueue_;
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
    codec_(NULL)
  {
    if (context_ && stream_)
    {
      assert(context_->streams[stream_->index] == stream_);
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
      assert(false);
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
    
    // use this for video frame conversion (color format and size)
    bool setTraitsOverride(const VideoTraits & override);
    bool getTraitsOverride(VideoTraits & override) const;
    
    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TVideoFramePtr & frame);
    bool getNextFrameDontWait(TVideoFramePtr & frame);
    
  protected:
    TVideoFrameQueue frameQueue_;
    VideoTraits override_;
  };
  
  //----------------------------------------------------------------
  // VideoTrackPtr
  // 
  typedef boost::shared_ptr<VideoTrack> VideoTrackPtr;
  
  //----------------------------------------------------------------
  // VideoTrack::VideoTrack
  // 
  VideoTrack::VideoTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream)
  {
    assert(stream->codec->codec_type == CODEC_TYPE_VIDEO);
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
      
      // FIXME: force YUV420P for debugging purposes:
      override_.colorFormat_ = kColorFormatI420;
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
    {}
    
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
  // VideoTrack::threadLoop
  // 
  void
  VideoTrack::threadLoop()
  {
    TOpenHere<TVideoFrameQueue> closeOnExit(frameQueue_);
    
    FrameWithAutoCleanup frameAutoCleanup;
    struct SwsContext * imgConvertCtx = NULL;


    bool hasAlpha = hasAlphaChannel(override_.colorFormat_);
    bool isPlanar = isFormatPlanar(override_.colorFormat_);
    unsigned int bitsPerPixel = getBitsPerPixel(override_.colorFormat_);
    enum PixelFormat pixelFormat = toPixelFormat(override_.colorFormat_);
    
    std::size_t pixelsPerFrame = (override_.encodedWidth_ *
                                  override_.encodedHeight_);
    
    std::size_t bytesPerFrame = (pixelsPerFrame * bitsPerPixel) / 8;
    
    // NOTE: this assumes there is at most 1 byte per Y (and A) channel:
    std::size_t bytesPerChromaPlane =
      hasAlpha ?
      (bytesPerFrame - 2 * pixelsPerFrame) / 2 :
      (bytesPerFrame - pixelsPerFrame) / 2;

    // FIXME: this is definitely broken for NxM sub-sampling
    // of UV planes when N != M
    std::size_t bytesPerChromaRow =
      hasAlpha ?
      (override_.encodedWidth_ * (bitsPerPixel - 16)) / 8 :
      (override_.encodedWidth_ * (bitsPerPixel - 8)) / 8;
    
    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();
        
        TPacketPtr packet;
        bool ok = packetQueue_.pop(packet);
        if (!ok)
        {
          break;
        }
        
        // Decode video frame
        int gotPicture = 0;
        
        AVFrame * avFrame = frameAutoCleanup;
        /* int bytesUsed = */
        avcodec_decode_video2(codecContext(),
                              avFrame,
                              &gotPicture,
                              &packet->ffmpeg_);
        if (!gotPicture)
        {
          continue;
        }
        
        TVideoFramePtr vfPtr(new TVideoFrame());
        TVideoFrame & vf = *vfPtr;
        
        vf.traits_ = override_;
        vf.time_.time_ = stream_->time_base.num * packet->ffmpeg_.pts;
        vf.time_.base_ = stream_->time_base.den;
        
        vf.setBufferSize<unsigned char>(bytesPerFrame);
        unsigned char * vfData = vf.getBuffer<unsigned char>();
        
        AVPicture pict;
        pict.data[0] = vfData;

        if (isPlanar)
        {
          pict.linesize[0] = override_.encodedWidth_;
          pict.linesize[1] = bytesPerChromaRow;
          pict.linesize[2] = pict.linesize[1];
          
          pict.data[1] = vfData + pixelsPerFrame;
          pict.data[2] = pict.data[1] + bytesPerChromaPlane;
          
          if (hasAlpha)
          {
            pict.data[3] = pict.data[2] + bytesPerChromaPlane;
            pict.linesize[3] = pict.linesize[0];
          }
        }
        else
        {
          pict.linesize[0] = (override_.encodedWidth_ * bitsPerPixel) / 8;
        }
        
        // Convert the image into the desired color format:
        if (imgConvertCtx == NULL)
        {
          imgConvertCtx = sws_getContext(// from:
                                         vf.traits_.visibleWidth_,
                                         vf.traits_.visibleHeight_, 
                                         codecContext()->pix_fmt,
                                         
                                         // to:
                                         vf.traits_.visibleWidth_,
                                         vf.traits_.visibleHeight_,
                                         pixelFormat,
                                         
                                         SWS_BICUBIC,
                                         NULL,
                                         NULL,
                                         NULL);
          if (imgConvertCtx == NULL)
          {
            assert(false);
            break;
          }
        }
        
        sws_scale(imgConvertCtx,
                  avFrame->data,
                  avFrame->linesize,
                  0,
                  codecContext()->height,
                  pict.data,
                  pict.linesize);
        
        // put the decoded/scaled frame into frame queue:
        frameQueue_.push(vfPtr);
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
    
    //! color format:
    t.colorFormat_ = toVideoColorFormat(context->pix_fmt);
    
    //! frame rate:
    t.frameRate_ =
      stream_->avg_frame_rate.den ?
      double(stream_->avg_frame_rate.num) /
      double(stream_->avg_frame_rate.den) :
      0.0;
    
    //! encoded frame size (including any padding):
    t.encodedWidth_ = context->width;
    t.encodedHeight_ = context->height;
    
    //! top/left corner offset to the visible portion of the encoded frame:
    t.offsetTop_ = 0;
    t.offsetLeft_ = 0;
    
    //! dimensions of the visible portion of the encoded frame:
    t.visibleWidth_ = context->width;
    t.visibleHeight_ = context->height;
    
    //! pixel aspect ration, used to calculate visible frame dimensions:
    t.pixelAspectRatio_ =
      stream_->sample_aspect_ratio.den ?
      double(stream_->sample_aspect_ratio.num) /
      double(stream_->sample_aspect_ratio.den) :
      1.0;
    
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
    assert(stream->codec->codec_type == CODEC_TYPE_AUDIO);
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
  // AudioTrack::threadLoop
  // 
  void
  AudioTrack::threadLoop()
  {
    TOpenHere<TAudioFrameQueue> closeOnExit(frameQueue_);
    
    TAudioFrame::TTraits trackTraits;
    getTraits(trackTraits);
    
    // declare a 16-byte aligned buffer for decoded audio samples:
    DECLARE_ALIGNED(16, uint8_t, buffer)
      [(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    
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
          
          chunks.push_back(std::vector<unsigned char>
                           (buffer, buffer + bufferSize));
          
          totalBytes += bufferSize;
        }
        
        if (!totalBytes)
        {
          continue;
        }

        TAudioFramePtr afPtr(new TAudioFrame());
        TAudioFrame & af = *afPtr;
        
        af.traits_ = trackTraits;
        af.time_.time_ = stream_->time_base.num * packet.pts;
        af.time_.base_ = stream_->time_base.den;
        
        af.setBufferSize<unsigned char>(totalBytes);
        unsigned char * afBuffer = af.getBuffer<unsigned char>();
        
        // concatenate chunks into a contiguous frame buffer:
        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();
          memcpy(afBuffer, chunk, chunkSize);
          
          afBuffer += chunkSize;
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
    override = override;
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
