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
  static const char * kProtocolName = "fileUtf8";
  
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
  urlWrite(URLContext * h, unsigned char * buf, int size)
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
  // ReaderFFMPEG::Private
  // 
  class ReaderFFMPEG::Private
  {
    static bool ffmpegInitialized_;
    
  public:
    Private():
      formatContext_(NULL),
      codecContext_(NULL),
      codec_(NULL),
      frame_(NULL)
    {
      if (!ffmpegInitialized_)
      {
        av_register_protocol(&fileUtf8::urlProtocol);
        av_register_all();
        
        ffmpegInitialized_ = true;
      }
    }
    
    AVFormatContext * formatContext_;
    AVCodecContext * codecContext_;
    AVCodec * codec_;
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
    private_ = NULL;
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
  // ReaderFFMPEG::getNumberOfVideoTracks
  // 
  std::size_t
  ReaderFFMPEG::getNumberOfVideoTracks() const
  {
    return 0;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfAudioTracks
  // 
  std::size_t
  ReaderFFMPEG::getNumberOfAudioTracks() const
  {
    return 0;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedVideoTrackIndex
  // 
  std::size_t
  ReaderFFMPEG::getSelectedVideoTrackIndex() const
  {
    return 0;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackIndex
  // 
  std::size_t
  ReaderFFMPEG::getSelectedAudioTrackIndex() const
  {
    return 0;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::selectVideoTrack
  // 
  bool
  ReaderFFMPEG::selectVideoTrack(std::size_t i)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::selectAudioTrack
  // 
  bool
  ReaderFFMPEG::selectAudioTrack(std::size_t i)
  {
    return false;
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedVideoTrackName
  // 
  const char *
  ReaderFFMPEG::getSelectedVideoTrackName() const
  {
    return "no name";
  }
  
  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackName
  // 
  const char *
  ReaderFFMPEG::getSelectedAudioTrackName() const
  {
    return "no name";
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
