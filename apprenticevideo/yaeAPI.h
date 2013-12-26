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
#include <assert.h>
#include <list>
#include <string>
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

//----------------------------------------------------------------
// YAE_ASSERT
//
#if defined(NDEBUG)
# define YAE_ASSERT(expr)
#else
# if defined(__APPLE__)
#  if defined(__ppc__)
#   define YAE_ASSERT(expr) if (!(expr)) __asm { trap }
#  else
#   define YAE_ASSERT(expr) if (!(expr)) asm("int $3")
#  endif
# elif __GNUC__
#   define YAE_ASSERT(expr) if (!(expr)) asm("int $3")
# else
#  define YAE_ASSERT(expr) assert(expr)
# endif
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
    TTime(double seconds);

    TTime & operator += (const TTime & dt);
    TTime operator + (const TTime & dt) const;

    TTime & operator += (double dtSec);
    TTime operator + (double dtSec) const;

    TTime & operator -= (const TTime & dt);
    TTime operator - (const TTime & dt) const;

    TTime & operator -= (double dtSec);
    TTime operator - (double dtSec) const;

    bool operator < (const TTime & t) const;
    bool operator <= (const TTime & t) const;

    int64 getTime(uint64 base) const;

    void to_hhmmss(std::string & ts,
                   const char * separator = "") const;

    void to_hhmmss_usec(std::string & ts,
                        const char * separator = "",
                        const char * usec_separator = ".") const;

    void to_hhmmss_frame(std::string & ts,
                         double frameRate = 29.97,
                         const char * separator = ":",
                         const char * framenum_separator = ":") const;

    inline std::string to_hhmmss(const char * separator = "") const
    {
      std::string ts;
      to_hhmmss(ts, separator);
      return ts;
    }

    // return timestamp in hhmmss.uuuuuu format
    inline std::string to_hhmmss_usec(const char * separator = "",
                                      const char * usec_separator = ".") const
    {
      std::string ts;
      to_hhmmss_usec(ts, separator, usec_separator);
      return ts;
    }

    inline std::string to_hhmmss_frame(double frameRate = 29.97,
                                       const char * separator = ":",
                                       const char * fnum_separator = ":") const
    {
      std::string ts;
      to_hhmmss_frame(ts, frameRate, separator, fnum_separator);
      return ts;
    }

    inline double toSeconds() const
    { return double(time_) / double(base_); }

    inline bool operator == (const TTime & t) const
    { return time_ == t.time_ && base_ == t.base_; }

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
    kAudio32BitBigEndian     = 6, //!< [-2147483648, 2147483647]
    kAudio32BitLittleEndian  = 7, //!< [-2147483648, 2147483647]
    kAudio64BitDouble        = 8, //!< [-1, 1]

#ifdef __BIG_ENDIAN__
    kAudio16BitNative        = kAudio16BitBigEndian,
    kAudio32BitNative        = kAudio32BitBigEndian,
#else
    kAudio16BitNative        = kAudio16BitLittleEndian,
    kAudio32BitNative        = kAudio32BitLittleEndian,
#endif
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

    bool operator == (const AudioTraits & at) const;

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
  // TColorSpaceId
  //
  enum TColorSpaceId
  {
    kColorSpaceRGB         =  0,
    kColorSpaceBT709       =  1, // SMPTE RP177 Annex B
    kColorSpaceUnspecified =  2,
    kColorSpaceFCC         =  4,
    kColorSpaceBT470BG     =  5, // BT601-6 625 PAL
    kColorSpaceSMPTE170M   =  6, // BT601-6 525 NTSC
    kColorSpaceSMPTE240M   =  7,
    kColorSpaceYCOCG       =  8,
    kColorSpaceBT2020NCL   =  9,
    kColorSpaceBT2020CL    = 10
  };

  //----------------------------------------------------------------
  // TColorRangeId
  //
  enum TColorRangeId
  {
    kColorRangeUnspecified = 0,
    kColorRangeBroadcast   = 1, // 16..235, mpeg
    kColorRangeFull        = 2, // 0...255, jpeg
  };

  //----------------------------------------------------------------
  // VideoTraits
  //
  struct YAE_API VideoTraits
  {
    VideoTraits();

    bool sameFrameSizeAndFormat(const VideoTraits & vt) const;
    bool sameColorSpaceAndRange(const VideoTraits & vt) const;

    bool operator == (const VideoTraits & vt) const;

    //! frame rate:
    double frameRate_;

    //! frame color format:
    TPixelFormatId pixelFormat_;

    //! color transform hints:
    TColorSpaceId colorSpace_;
    TColorRangeId colorRange_;

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

    //! camera orientation angle expressed in degrees (divisible by 90):
    int cameraRotation_;

    //! a flag indicating whether video is upside-down:
    bool isUpsideDown_;
  };

  //----------------------------------------------------------------
  // init_abc_to_rgb_matrix
  //
  // Fill in the m3x4 matrix for color conversion from
  // input color format ABC to full-range RGB:
  //
  // [R, G, B]T = m3x4 * [A, B, C, 1]T
  //
  // NOTE: ABC and RGB are expressed in the [0, 1] range,
  //       not [0, 255].
  //
  // NOTE: Here ABC typically refers to YUV input color format,
  //       however it doesn't have to be YUV.
  //
  YAE_API bool
  init_abc_to_rgb_matrix(double * m3x4, const VideoTraits & vtts);

  //----------------------------------------------------------------
  // TSubsFormat
  //
  enum TSubsFormat
  {
    kSubsNone,
    kSubsDVD,
    kSubsDVB,
    kSubsText,
    kSubsXSUB,
    kSubsSSA,
    kSubsMovText,
    kSubsHDMVPGS,
    kSubsDVBTeletext,
    kSubsSRT,
    kSubsMICRODVD,
    kSubsCEA608,
    kSubsJACOSUB,
    kSubsSAMI,
    kSubsREALTEXT,
    kSubsSUBVIEWER,
    kSubsSUBRIP,
    kSubsWEBVTT
  };

  //----------------------------------------------------------------
  // TSubtitleType
  //
  enum TSubtitleType
  {
    kSubtitleNone,
    kSubtitleBitmap,
    kSubtitleText,
    kSubtitleASS
  };

  //----------------------------------------------------------------
  // getSubsFormatLabel
  //
  YAE_API const char * getSubsFormatLabel(TSubsFormat fmt);

  //----------------------------------------------------------------
  // IPlanarBuffer
  //
  class YAE_API IPlanarBuffer
  {
  protected:
    virtual ~IPlanarBuffer();

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

    //! this will call IPlanarBuffer::destroy()
    static void deallocator(IPlanarBuffer * sb);

    //! number of contiguous sample planes:
    virtual std::size_t planes() const = 0;

    //! data plane accessor:
    virtual unsigned char * data(std::size_t plane) const = 0;

    //! bytes per plane row:
    virtual std::size_t rowBytes(std::size_t planeIndex) const = 0;
  };

  //----------------------------------------------------------------
  // TIPlanarBufferPtr
  //
  typedef boost::shared_ptr<IPlanarBuffer> TIPlanarBufferPtr;

  //----------------------------------------------------------------
  // TDataBuffer
  //
  struct YAE_API TDataBuffer
  {
    TDataBuffer();
    ~TDataBuffer();

    TDataBuffer(const TDataBuffer & src);
    TDataBuffer & operator = (const TDataBuffer & src);

    // resize the sample plane:
    void resize(std::size_t rowBytes,
                std::size_t rows = 1,
                std::size_t alignment = 32);

    template <typename TSample>
    void resize(std::size_t samples,
                std::size_t rows = 1,
                std::size_t alignment = 32)
    { this->resize(samples * sizeof(TSample), rows, alignment); }

    // data accessors:
    template <typename TSample>
    inline TSample * data() const
    { return (TSample *)(data_ + alignmentOffset_); }

    inline unsigned char * data() const
    { return data_ + alignmentOffset_; }

    // bytes per sample row:
    inline std::size_t rowBytes() const
    { return rowBytes_; }

    // rows per plane:
    inline std::size_t rows() const
    { return rows_; }

  protected:
    unsigned char * data_;
    std::size_t alignmentOffset_;
    std::size_t rowBytes_;
    std::size_t rows_;
    std::size_t alignment_;
  };

  //----------------------------------------------------------------
  // TPlanarBuffer
  //
  struct YAE_API TPlanarBuffer : public IPlanarBuffer
  {
    TPlanarBuffer(std::size_t planes);

    // virtual:
    void destroy();

    // virtual:
    std::size_t planes() const;

    // virtual:
    unsigned char * data(std::size_t plane) const;

    // virtual:
    std::size_t rowBytes(std::size_t plane) const;

    // helper:
    std::size_t rows(std::size_t plane) const;

    // helper:
    void resize(std::size_t plane,
                std::size_t rowBytes,
                std::size_t rows,
                std::size_t alignment = 32);

    // helper, useful for audio sample buffer:
    inline void resize(std::size_t numBytes, std::size_t alignment = 32)
    { resize(0, numBytes, 1, alignment); }

  protected:
    // data planes:
    std::vector<TDataBuffer> plane_;
  };

  //----------------------------------------------------------------
  // TPlanarBufferPtr
  //
  typedef boost::shared_ptr<TPlanarBuffer> TPlanarBufferPtr;

  //----------------------------------------------------------------
  // TFrame
  //
  template <typename traits_t>
  struct YAE_API TFrame
  {
    typedef traits_t TTraits;
    typedef TFrame<traits_t> TSelf;

    TFrame(): readerId_((unsigned int)~0), tempo_(1.0) {}

    bool operator == (const TSelf & s) const
    {
      return (time_   == s.time_ &&
              tempo_  == s.tempo_ &&
              traits_ == s.traits_ &&
              data_   == s.data_);
    }

    //! reader ID tag:
    unsigned int readerId_;

    //! frame position:
    TTime time_;

    //! frame duration tempo scaling:
    double tempo_;

    //! frame traits:
    TTraits traits_;

    //! frame buffer:
    TIPlanarBufferPtr data_;
  };

  //----------------------------------------------------------------
  // TSubsFrame
  //
  struct YAE_API TSubsFrame : public TFrame<TSubsFormat>
  {
    typedef TFrame<TSubsFormat> TBase;

    struct TRect
    {
      TSubtitleType type_;

      int x_;
      int y_;
      int w_;
      int h_;
      int numColors_;

      const unsigned char * data_[8];
      int rowBytes_[8];

      const char * text_;
      const char * assa_;
    };

    struct IPrivate
    {
    protected:
      virtual ~IPrivate() {}

    public:
      virtual void destroy() = 0;
      static void deallocator(IPrivate * p);

      virtual std::size_t headerSize() const = 0;
      virtual const unsigned char * header() const = 0;

      virtual unsigned int numRects() const = 0;
      virtual void getRect(unsigned int i, TRect & rect) const = 0;
    };

    TSubsFrame();

    bool operator == (const TSubsFrame & s) const;

    // a flag indicating whether this subtitle frame should be rendered:
    bool render_;

    // track index:
    std::size_t index_;

    // reference frame width and height, if known:
    int rh_;
    int rw_;

    // track specific data (sequence header, etc...)
    TIPlanarBufferPtr extraData_;

    // additional frame data:
    TIPlanarBufferPtr sideData_;

    // frame expiration time:
    TTime tEnd_;

    // decoded subtitle data:
    boost::shared_ptr<IPrivate> private_;
  };

  //----------------------------------------------------------------
  // TVideoFrame
  //
  struct YAE_API TVideoFrame : public TFrame<VideoTraits>
  {
    std::list<TSubsFrame> subs_;
  };

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

  //----------------------------------------------------------------
  // TTrackInfo
  //
  struct YAE_API TTrackInfo
  {
    TTrackInfo(std::size_t index = 0, std::size_t ntracks = 0);

    bool isValid() const;
    bool hasLang() const;
    bool hasName() const;

    const char * lang() const;
    const char * name() const;

    void setLang(const char * lang);
    void setName(const char * name);

    std::size_t ntracks_;
    std::size_t index_;
    std::string lang_;
    std::string name_;
  };

  //----------------------------------------------------------------
  // TChapter
  //
  struct YAE_API TChapter
  {
    TChapter(const std::string & n = std::string(),
             double t = 0.0,
             double dt = 0.0);

    // chapter name:
    std::string name_;

    // expressed in seconds:
    double start_;
    double duration_;
  };

}


#endif // YAE_API_H_
