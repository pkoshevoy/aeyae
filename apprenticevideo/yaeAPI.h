// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 00:58:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_API_H_
#define YAE_API_H_

// system includes:
#include <vector>

// boost includes:
#include <boost/cstdint.hpp>


//----------------------------------------------------------------
// YAE_API
// 
// http://gcc.gnu.org/wiki/Visibility
// 
#if defined _WIN32 || defined __CYGWIN__
#  ifdef YAE_DLL_EXPORTS
#    define YAE_API __declspec(dllexport)
#  else
#    define YAE_API __declspec(dllimport)
#  endif
#else
#  if __GNUC__ >= 4
#    define YAE_API __attribute__ ((visibility("default")))
#  else
#    define YAE_API
#  endif
#endif


namespace yae
{

  //----------------------------------------------------------------
  // uint64
  // 
  typedef boost::uint64_t uint64;
  
  //----------------------------------------------------------------
  // int64
  // 
  typedef boost::int64_t int64;
  
  //----------------------------------------------------------------
  // TTime
  // 
  struct YAE_API TTime
  {
    TTime();
    
    uint64 time_;
    uint64 base_;
  };

  //----------------------------------------------------------------
  // TAudioSampleFormat
  // 
  enum TAudioSampleFormat
  {
    kAudioInvalidFormat      = 0,
    kAudio8BitOffsetBinary   = 1, //!< [0, 255],
    kAudio16BitBigEndian     = 2, //!< [-32768, 32767]
    kAudio16BitLittleEndian  = 3, //!< [-32768, 32767]
    kAudio24BitLittleEndian  = 4, //!< [-8388608, 8388607]
    kAudio32BitFloat         = 5, //!< [-1, 1]
  };
  
  //----------------------------------------------------------------
  // TAudioChannelFormat
  // 
  enum TAudioChannelFormat
  {
    kAudioChannelFormatInvalid = 0,
    kAudioChannelsMuxed  = 1, //!< all channel samples are interleaved
    kAudioChannelsPlanar = 2  //!< same channel samples are grouped together
  };
  
  //----------------------------------------------------------------
  // TAudioChannelLayout
  // 
  enum TAudioChannelLayout
  {
    kAudioChannelLayoutInvalid = 0,
    kAudioMono   = 1,
    kAudioStereo = 2,
    kAudio2Pt1   = 3,
    kAudioQuad   = 4,
    kAudio4Pt1   = 5,
    kAudio5Pt1   = 6, //!< 5.1
    kAudio6Pt1   = 7, //!< 6.1
    kAuduo7Pt1   = 8  //!< 7.1
  };
  
  //----------------------------------------------------------------
  // AudioTraits
  // 
  struct YAE_API AudioTraits
  {
    AudioTraits();
    
    //! audio sample rate, Hz:
    unsigned int sampleRate_;

    //! 8-bit, 16-bit, float, etc...
    TAudioSampleFormat sampleFormat_;
    
    //! muxed, planar:
    TAudioChannelFormat channelFormat_;
    
    //! mono, stereo, etc...
    TAudioChannelLayout channelLayout_;
  };
  
  //----------------------------------------------------------------
  // TVideoColorFormat
  // 
  enum TVideoColorFormat
  {
    kInvalidColorFormat = 0,
    kColorFormatRGB  = 1, //!< muxed Red, Green, Blue
    kColorFormatARGB = 2, //!< muxed Alpha, Red, Green, Blue
    kColorFormatBGRA = 3, //!< muxed Blue, Green, Red, Alpha
    kColorFormatI420 = 4, //!< 8-bit Y plane, 2x2 subsampled U plane, 2x2 V
    kColorFormatYV12 = 5, //!< 8-bit Y plane, 2x2 subsampled V plane, 2x2 U
    kColorFormatUYVY = 6, //!< muxed, U and V are subsampled 2x horizontally
    kColorFormatYUYV = 7, //!< same as UYVY but with different ordering
  };
  
  //----------------------------------------------------------------
  // VideoTraits
  // 
  struct YAE_API VideoTraits
  {
    VideoTraits();
    
    //! frame rate:
    double frameRate_;
    
    //! frame color format:
    TVideoColorFormat colorFormat_;
    
    //! encoded frame size (including any padding):
    unsigned int encodedWidth_;
    unsigned int encodedHeight_;
    
    //! top/left corner offset to the visible portion of the encoded frame:
    unsigned int offsetTop_;
    unsigned int offsetLeft_;
    
    //! dimensions of the visible portion of the encoded frame:
    unsigned int visibleWidth_;
    unsigned int visibleHeight_;
    
    //! pixel aspect ration, used to calculate visible frame dimensions:
    double pixelAspectRatio_;
    
    //! a flag indicating whether video is upside-down:
    bool isUpsideDown_;
  };
  
  //----------------------------------------------------------------
  // TFrame
  // 
  template <typename traits_t>
  struct YAE_API TFrame
  {
    typedef traits_t TTraits;
    typedef std::vector<unsigned char> TData;
    
    //! frame position:
    TTime time_;
    
    //! frame traits:
    TTraits traits_;
    
    //! frame data:
    TData data_;
  };
  
  //----------------------------------------------------------------
  // TVideoFrame
  // 
  typedef TFrame<VideoTraits> TVideoFrame;
  
  //----------------------------------------------------------------
  // TAudioFrame
  // 
  typedef TFrame<AudioTraits> TAudioFrame;
  
}


#endif // YAE_API_H_
