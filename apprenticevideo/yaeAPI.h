// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 00:58:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_API_H_
#define YAE_API_H_

// yae includes:
#include <yaePixelFormats.h>

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
  // VideoTraits
  // 
  struct YAE_API VideoTraits
  {
    VideoTraits();
    
    //! frame rate:
    double frameRate_;
    
    //! frame color format:
    TPixelFormatId pixelFormat_;
    
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
  // ISampleBuffer
  // 
  class ISampleBuffer
  {
  protected:
    virtual ~ISampleBuffer();
    
  public:
    //! The de/structor is intentionally hidden, use destroy() method instead.
    //! This is necessary in order to avoid conflicting memory manager
    //! problems that arise on windows when various libs are linked to
    //! different versions of runtime library.  Each library uses its own
    //! memory manager, so allocating in one library call and deallocating
    //! in another library will not work.  This can be avoided by hiding
    //! the standard constructor/destructor and providing an explicit
    //! interface for de/allocating an object instance, thus ensuring that
    //! the same memory manager will perform de/allocation.
    virtual void destroy() = 0;
    
    //! number of contiguous sample planes:
    virtual std::size_t samplePlanes() const = 0;
    
    //! samples plane accessor:
    virtual unsigned char * samples(std::size_t samplePlane) const = 0;
    
    //! bytes per plane row:
    virtual std::size_t rowBytes(std::size_t samplePlaneIndex) const = 0;
    
    //! this will call ISampleBuffer::destroy()
    static void deallocator(ISampleBuffer * sb);
  };
  
  //----------------------------------------------------------------
  // TISampleBufferPtr
  // 
  typedef boost::shared_ptr<ISampleBuffer> TISampleBufferPtr;

  //----------------------------------------------------------------
  // TSamplePlane
  // 
  struct TSamplePlane
  {
    TSamplePlane();
    ~TSamplePlane();
    
    // resize the sample plane:
    void resize(std::size_t rowBytes,
                std::size_t rows,
                unsigned char alignment = 16);
    
    // samples accessors:
    inline unsigned char * data() const
    { return data_ + alignmentOffset_; }
    
    // bytes per sample row:
    inline int rowBytes() const
    { return rowBytes_; }
    
  protected:
    unsigned char * data_;
    std::size_t alignmentOffset_;
    int rowBytes_;
    int rows_;
  };
  
  //----------------------------------------------------------------
  // TSampleBuffer
  //
  struct TSampleBuffer : public ISampleBuffer
  {
    TSampleBuffer(std::size_t samplePlanes);
    
    // virtual:
    void destroy();
    
    // virtual:
    std::size_t samplePlanes() const;
    
    // virtual:
    unsigned char * samples(std::size_t samplePlane) const;
    
    // virtual:
    std::size_t rowBytes(std::size_t samplePlane) const;
    
    // helper:
    void resize(std::size_t samplePlane,
                std::size_t rowBytes,
                std::size_t rows,
                std::size_t alignment = 16);
    
  protected:
    // sample planes:
    std::vector<TSamplePlane> plane_;
  };
  
  //----------------------------------------------------------------
  // TSampleBufferPtr
  // 
  typedef boost::shared_ptr<TSampleBuffer> TSampleBufferPtr;
  
  //----------------------------------------------------------------
  // TFrame
  // 
  template <typename traits_t>
  struct YAE_API TFrame
  {
    typedef traits_t TTraits;
    typedef TFrame<traits_t> TSelf;
    
    //! frame position:
    TTime time_;
    
    //! frame traits:
    TTraits traits_;
    
    //! frame sample buffer:
    TISampleBufferPtr sampleBuffer_;
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
