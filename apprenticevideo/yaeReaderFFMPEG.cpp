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
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <typeinfo>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeReaderFFMPEG.h>

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
// openUtf8
// 
static int
openUtf8(const char * filenameUTF8, int accessMode, int permissions)
{
#ifdef _WIN32
  accessMode |= O_BINARY;
  
  size_t wcsSize =
    MultiByteToWideChar(CP_UTF8,      // source string encoding
                        0,            // flags (precomposed, composite, etc...)
                        filenameUTF8, // source string
                        -1,           // source string size
                        NULL,         // output string destination buffer
                        0);           // output string destination buffer size
  
  wchar_t * filenameUTF16 = (wchar_t *)malloc(sizeof(wchar_t) * (wcsSize + 1));
  MultiByteToWideChar(CP_UTF8,
                      0,
                      filenameUTF8,
                      -1,
                      filenameUTF16,
                      wcsSize);
  
  int fd = _wopen(filenameUTF16, accessMode, permissions);
  free(filenameUTF16);
  
#else
  
  int fd = open(filenameUTF8, accessMode, permissions);
#endif
  
  return fd;
}


//----------------------------------------------------------------
// fileSeek64
// 
static int64_t
fileSeek64(int fd, int64_t offset, int whence)
{
#ifdef _WIN32
  __int64 pos = _lseeki64(fd, offset, whence);
#else
  off64_t pos = lseek64(fd, offset, whence);
#endif
  
  return pos;
}

//----------------------------------------------------------------
// fileSize64
// 
static int64_t
fileSize64(int fd)
{
#ifdef _WIN32
  struct _stati64 st;
  __int64 ret = _fstati64(fd, &st);
#else
  struct stat st;
  int ret = fstat(fd, &st);
#endif
  
  if (ret < 0)
  {
    return ret;
  }
  
  return st.st_size;
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
    
    int permissions = 0666;
    int fd = openUtf8(filename, accessMode, permissions);
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
    return read(fd, buf, size);
  }
  
  //----------------------------------------------------------------
  // urlWrite
  // 
  static int
  urlWrite(URLContext * h, const unsigned char * buf, int size)
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
      int64_t size = fileSize64(fd);
      if (size < 0)
      {
        return AVERROR(errno);
      }
      
      return size;
    }

    return fileSeek64(fd, pos, whence);
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
    &urlGetFileHandle
  };
  
}


namespace yae
{

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
    void close();
    
    // get track name:
    const char * getName() const;
    
    // accessor to the codec context:
    inline AVCodecContext * codecContext() const
    { return stream_->codec; }
    
    // accessor to the codec:
    inline AVCodec * codec() const
    { return codec_; }
    
    // get track duration:
    void getDuration(TTime & t) const;
    
    // get current position:
    bool getPosition(TTime & t) const;
    
    // seek to a position:
    bool setPosition(const TTime & t);
    
  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);
    
  protected:
    AVFormatContext * context_;
    AVStream * stream_;
    AVCodec * codec_;
  };
  
  //----------------------------------------------------------------
  // TrackPtr
  // 
  typedef boost::shared_ptr<Track> TrackPtr;
  
  //----------------------------------------------------------------
  // Track::Track
  // 
  Track::Track(AVFormatContext * context, AVStream * stream):
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
  // Track::getPosition
  // 
  bool
  Track::getPosition(TTime & t) const
  {
    if (!stream_)
    {
      assert(false);
      return false;
    }
    
    // FIXME: not sure that I am looking at the correct timestamp:
    if (stream_->cur_pkt.pts == int64_t(AV_NOPTS_VALUE))
    {
      return false;
    }
    
    t.time_ = stream_->time_base.num * stream_->cur_pkt.pts;
    t.base_ = stream_->time_base.den;
    return true;
  }
  
  //----------------------------------------------------------------
  // Track::setPosition
  // 
  bool
  Track::setPosition(const TTime & t)
  {
    if (!stream_)
    {
      assert(false);
      return false;
    }
    
    return true;
  }
  
  
  //----------------------------------------------------------------
  // VideoTrack
  // 
  struct VideoTrack : public Track
  {
    VideoTrack(AVFormatContext * context, AVStream * stream);
    
    bool getTraits(VideoTraits & traits) const;
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
    
    switch (context->pix_fmt)
    {
      case PIX_FMT_YUVA420P:
        // same as I420, plus alpha plane (same size as Y plane)
        
      case PIX_FMT_YUV420P:
        t.colorFormat_ = kColorFormatI420;
        break;
        
      case PIX_FMT_YUYV422:
 	t.colorFormat_ = kColorFormatYUYV;
        break;
        
      case PIX_FMT_RGB24:
        t.colorFormat_ = kColorFormatRGB;
        break;
        
      case PIX_FMT_BGR24:
 	t.colorFormat_ = kColorFormatBGR;
        break;
        
      case PIX_FMT_ARGB:
        t.colorFormat_ = kColorFormatARGB;
        break;
        
      case PIX_FMT_YUVJ420P:
 	t.colorFormat_ = kColorFormatYUVJ420P;
        break;
        
      case PIX_FMT_UYVY422:
        t.colorFormat_ = kColorFormatUYVY;
        break;
        
      case PIX_FMT_BGRA:
        t.colorFormat_ = kColorFormatBGRA;
        break;
        
      default:
        t.colorFormat_ = kInvalidColorFormat;
    }
    
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
  // AudioTrack
  // 
  struct AudioTrack : public Track
  {
    AudioTrack(AVFormatContext * context, AVStream * stream);
    
    bool getTraits(AudioTraits & traits) const;
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
        t.sampleFormat_ = kAudio16BitLittleEndian;
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
    
  private:
    // intentionally disabled:
    Movie(const Movie &);
    Movie & operator = (const Movie &);
    
  protected:
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
    }
    
    if (i >= numVideoTracks)
    {
      return false;
    }
    
    selectedVideoTrack_ = i;
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
    }
    
    if (i >= numAudioTracks)
    {
      return false;
    }
    
    selectedAudioTrack_ = i;
    return audioTracks_[selectedAudioTrack_]->open();
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
        av_register_protocol(&fileUtf8::urlProtocol);
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
    return private_->movie_.getSelectedVideoTrack();
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
  ReaderFFMPEG::readVideo(TVideoFrame & frame)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::readAudio
  // 
  bool
  ReaderFFMPEG::readAudio(TAudioFrame & frame)
  {
    return false;
  }
  
}
