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
#include <stdint.h>
#include <stdlib.h>
#include <typeinfo>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <sstream>
#include <iostream>

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
  struct_stat st;
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
    Track(AVStream * stream);
    
    // NOTE: destructor will close the stream:
    ~Track();
    
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
    
  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);
    
  protected:
    AVStream * stream_;
    AVCodec * codec_;
  };
  
  
  //----------------------------------------------------------------
  // Track::Track
  // 
  Track::Track(AVStream * stream):
    stream_(stream),
    codec_(NULL)
  {}
  
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
  // TrackPtr
  // 
  typedef boost::shared_ptr<Track> TrackPtr;
  
  
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
    
    inline const std::vector<TrackPtr> & getVideoTracks() const
    { return videoTracks_; }
    
    inline const std::vector<TrackPtr> & getAudioTracks() const
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
    AVFormatContext * formatContext_;
    
    std::vector<TrackPtr> videoTracks_;
    std::vector<TrackPtr> audioTracks_;
    
    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;
  };
  
  
  //----------------------------------------------------------------
  // Movie::Movie
  // 
  Movie::Movie():
    formatContext_(NULL),
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
    
    int err = av_open_input_file(&formatContext_,
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
    formatContext_->max_analyze_duration = 2 * AV_TIME_BASE;
    
    err = av_find_stream_info(formatContext_);
    if (err < 0)
    {
      close();
      return false;
    }
    
    for (unsigned int i = 0; i < formatContext_->nb_streams; i++)
    {
      TrackPtr track(new Track(formatContext_->streams[i]));
      
      if (!track->open())
      {
        // unsupported codec, ignore it:
        continue;
      }
      
      const AVMediaType codecType = track->codecContext()->codec_type;
      if (codecType == CODEC_TYPE_VIDEO)
      {
        videoTracks_.push_back(track);
      }
      else if (codecType == CODEC_TYPE_AUDIO)
      {
        audioTracks_.push_back(track);
      }
      
      // close the track:
      track->close();
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
    if (formatContext_ == NULL)
    {
      return;
    }
    
    videoTracks_.clear();
    audioTracks_.clear();
    
    av_close_input_file(formatContext_);
    formatContext_ = NULL;
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
        std::cout << "first_protocol: " << first_protocol << std::endl;
        
        ffmpegInitialized_ = true;
      }
      
      // display availabled IO protocols:
      URLProtocol * up = first_protocol;
      while (up != NULL)
      {
        std::cout << "protocol: " << up->name << std::endl;
        up = up->next;
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
    t.time_ = 0;
    t.base_ = 1001;
    return true;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioDuration
  // 
  bool
  ReaderFFMPEG::getAudioDuration(TTime & t) const
  {
    t.time_ = 0;
    t.base_ = 1001;
    return true;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraits
  // 
  bool
  ReaderFFMPEG::getAudioTraits(AudioTraits & traits) const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraits
  // 
  bool
  ReaderFFMPEG::getVideoTraits(VideoTraits & traits) const
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoPosition
  // 
  void
  ReaderFFMPEG::getVideoPosition(TTime & t) const
  {}
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioPosition
  // 
  void
  ReaderFFMPEG::getAudioPosition(TTime & t) const
  {}
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::setVideoPosition
  // 
  bool
  ReaderFFMPEG::setVideoPosition(const TTime & t)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::setAudioPosition
  // 
  bool
  ReaderFFMPEG::setAudioPosition(const TTime & t)
  {
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
