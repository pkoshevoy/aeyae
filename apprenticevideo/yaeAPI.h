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
#include <boost/shared_ptr.hpp>


//----------------------------------------------------------------
// YAE_API
// 
// http://gcc.gnu.org/wiki/Visibility
// 
#if defined _WIN32
#  ifdef YAE_DLL_EXPORTS
#    define YAE_API __declspec(dllexport)
#  elif !defined(YAE_STATIC)
#    define YAE_API __declspec(dllimport)
#  else
#    define YAE_API
#  endif
#else
#  if __GNUC__ >= 4
#    define YAE_API __attribute__ ((visibility("default")))
#  else
#    define YAE_API
#  endif
#endif


//----------------------------------------------------------------
// YAE_ALIGN
// 
#if defined(_MSC_VER)
# define YAE_ALIGN(N, T) __declspec(align(N)) T
#elif __GNUC__ >= 4
# define YAE_ALIGN(N, T) T __attribute__ ((aligned(N)))
#else
# define YAE_ALIGN(N, T) T
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
    TTime(int64 time, uint64 base);
    
    TTime & operator += (const TTime & dt);
    TTime operator + (const TTime & dt) const;
    
    TTime & operator += (double dtSec);
    TTime operator + (double dtSec) const;
    
    int64 time_;
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
  // getBitsPerSample
  // 
  YAE_API unsigned int getBitsPerSample(TAudioSampleFormat sampleFormat);
  
  //----------------------------------------------------------------
  // TAudioChannelFormat
  // 
  enum TAudioChannelFormat
  {
    kAudioChannelFormatInvalid = 0,
    kAudioChannelsPacked = 1, //!< all channel samples are interleaved
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
    kAudio7Pt1   = 8  //!< 7.1
  };
  
  //----------------------------------------------------------------
  // getNumberOfChannels
  // 
  YAE_API unsigned int getNumberOfChannels(TAudioChannelLayout channelLayout);
  
  //----------------------------------------------------------------
  // AudioTraits
  // 
  struct YAE_API AudioTraits
  {
    AudioTraits();
    
    //! audio sample rate, Hz:
    unsigned int sampleRate_;

    //! sample format -- 8-bit, 16-bit, float, etc...
    TAudioSampleFormat sampleFormat_;
    
    //! sample layout -- packed, planar:
    TAudioChannelFormat channelFormat_;
    
    //! channel layout -- mono, stereo, etc...
    TAudioChannelLayout channelLayout_;
  };
  
  //----------------------------------------------------------------
  // TVideoColorFormat
  // 
  enum TVideoColorFormat
  {
    kInvalidColorFormat = 0,
    
    //! packed Red, Green, Blue
    kColorFormatRGB  = 1,

    //! packed Red, Green, Blue
    kColorFormatBGR  = 2,

    //! packed Alpha, Red, Green, Blue
    kColorFormatARGB = 3,
    
    //! packed Blue, Green, Red, Alpha
    kColorFormatBGRA = 4,

    //! 8-bit Y plane, 2x2 subsampled U plane, 2x2 V, [16, 235]
    kColorFormatI420 = 5,

    //! 8-bit Y plane, 2x2 subsampled U plane, 2x2 V, [16, 235], Alpha Y plane
    kColorFormatI420Alpha = 6,
    
    //! 8-bit Y plane, 2x2 subsampled V plane, 2x2 U
    kColorFormatYV12 = 7,
    
    //! packed, U and V are subsampled 2x horizontally
    kColorFormatUYVY = 8,
    
    //! same as UYVY but with different ordering
    kColorFormatYUYV = 9,
    
    //! jpeg YUV420P, same as I420 but with full range [0, 255]
    kColorFormatYUVJ420P = 10,

    //! jpeg YUV422P, planar, 1 UV pair per 2 Y values, [0, 255]
    kColorFormatYUVJ422P = 11
  };

  //----------------------------------------------------------------
  // hasAlphaChannel
  // 
  YAE_API bool hasAlphaChannel(TVideoColorFormat colorFormat);

  //----------------------------------------------------------------
  // isFormatPlanar
  // 
  YAE_API bool isFormatPlanar(TVideoColorFormat colorFormat);
  
  //----------------------------------------------------------------
  // getBitsPerPixel
  // 
  YAE_API unsigned int getBitsPerPixel(TVideoColorFormat colorFormat);
  
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
    typedef TFrame<traits_t> TSelf;
    
    TFrame(): dataSize_(0)
    {}
    
    //! resize data buffer:
    template <typename TUnit>
    inline void
    setBufferSize(std::size_t numUnits)
    {
      dataSize_ = sizeof(TUnit) * numUnits;
      
      std::size_t paddingBytes = sizeof(TDataUnit) - 1;
      std::size_t n = ((dataSize_ + paddingBytes) /
                       sizeof(TDataUnit));
      
      data_.resize(n);
    }
    
    //! data buffer accessors:
    template <typename TUnit>
    inline TUnit * getBuffer()
    { return (TUnit *)(&(data_[0])); }
    
    template <typename TUnit>
    inline const TUnit * getBuffer() const
    { return (const TUnit *)(&(data_[0])); }
    
    template <typename TUnit>
    inline std::size_t getBufferSize() const
    { return dataSize_ / sizeof(TUnit); }
    
    //! frame position:
    TTime time_;
    
    //! frame traits:
    TTraits traits_;
    
  private:
    //! frame data (8-byte aligned):
    typedef boost::uint64_t TDataUnit;
    typedef std::vector<TDataUnit> TData;
    TData data_;
    
    //! frame payload size, in bytes, excluding padding:
    std::size_t dataSize_;
  };
  
  //----------------------------------------------------------------
  // TVideoFrame
  // 
  typedef TFrame<VideoTraits> TVideoFrame;
  
  //----------------------------------------------------------------
  // TVideoFramePtr
  // 
  typedef boost::shared_ptr<TVideoFrame> TVideoFramePtr;
  
  //----------------------------------------------------------------
  // TAudioFrame
  // 
  typedef TFrame<AudioTraits> TAudioFrame;
  
  //----------------------------------------------------------------
  // TAudioFramePtr
  // 
  typedef boost::shared_ptr<TAudioFrame> TAudioFramePtr;
  
}


#endif // YAE_API_H_
