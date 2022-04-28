// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 00:58:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_H_
#define YAE_VIDEO_H_

// standard C++ library:
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

// ffmpeg includes:
extern "C"
{
#include <libavcodec/packet.h>
#include <libavutil/pixfmt.h>
}

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_linear_algebra.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_colorspace.h"
#include "yae/video/yae_pixel_formats.h"


namespace yae
{
  // forward declarations:
  struct IReader;


  //----------------------------------------------------------------
  // get_timeline
  //
  YAE_API bool
  get_timeline(const IReader * reader,
               TTime & t0,
               TTime & t1);

  //----------------------------------------------------------------
  // get_program_name
  //
  YAE_API std::string
  get_program_name(const IReader & reader, std::size_t program);

  //----------------------------------------------------------------
  // get_selected_subtt_track
  //
  // return a value in range:
  //
  //  -  [0, nsubs) for a Subtitles track
  //
  //  -  [nsubs, nsubs + 4) for Closed Captions CC1-4
  //
  //  -  nsubs + 4 for Disabled
  //
  //
  YAE_API int
  get_selected_subtt_track(const IReader & reader);

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
  // VideoTraits
  //
  struct YAE_API VideoTraits
  {
    VideoTraits();

    std::string summary() const;

    bool sameFrameSizeAndFormat(const VideoTraits & vt) const;
    bool sameColorSpaceAndRange(const VideoTraits & vt) const;

    bool operator == (const VideoTraits & vt) const;

    void setPixelFormat(TPixelFormatId fmt);

    //! frame rate:
    double frameRate_;

    // for HDR, etc... may change at runtime:
    Colorspace::DynamicRange dynamic_range_;

    //! ffmpeg pixel format:
    AVPixelFormat av_fmt_;

    //! color transform properties, expressed with ffmpeg types/constants:
    AVColorSpace av_csp_;
    AVColorPrimaries av_pri_;
    AVColorTransferCharacteristic av_trc_;
    AVColorRange av_rng_;

    // helper object, configured according to the above
    // colorspace specifications:
    const Colorspace * colorspace_;

    //! aeyae pixel format:
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

    //! camera orientation angle expressed in degrees (divisible by 90):
    int cameraRotation_;

    //! a flag indicating whether video is flipped:
    bool vflip_; // mirrored top-to-bottom
    bool hflip_; // mirrored left-to-right
  };

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
  // TRendererHints
  //
  enum TRendererHints
  {
    kRendererHintNone = 0,
    kRendererHintResetTimeCounters = 1,
    kRendererHintDropPendingFrames = 1 << 1
  };

  //----------------------------------------------------------------
  // TFrameBase
  //
  struct YAE_API TFrameBase
  {
    TFrameBase(unsigned int rendererHints = 0):
      rendererHints_(rendererHints),
      readerId_((unsigned int)~0),
      pos_(std::numeric_limits<int64_t>::min()),
      tempo_(1.0)
    {}

    //! renderer hints bitmask:
    unsigned int rendererHints_;

    //! reader ID tag:
    unsigned int readerId_;

    //! global track ID tag:
    std::string trackId_;

    //! approx. byte position of the packet that produced this frame:
    int64_t pos_;

    //! frame PTS:
    TTime time_;

    //! frame duration tempo scaling:
    double tempo_;

    //! frame buffer:
    TIPlanarBufferPtr data_;
  };

  //----------------------------------------------------------------
  // TFrame
  //
  template <typename Traits>
  struct YAE_API TFrame : public TFrameBase
  {
    typedef Traits TTraits;
    typedef TFrame<Traits> TSelf;

    //! frame traits:
    TTraits traits_;
  };

  //----------------------------------------------------------------
  // resetTimeCountersIndicated
  //
  inline bool resetTimeCountersIndicated(const TFrameBase * frame)
  {
    return frame && !!(frame->rendererHints_ & kRendererHintResetTimeCounters);
  }

  //----------------------------------------------------------------
  // dropPendingFramesIndicated
  //
  inline bool dropPendingFramesIndicated(const TFrameBase * frame)
  {
    return frame && !!(frame->rendererHints_ & kRendererHintDropPendingFrames);
  }

  //----------------------------------------------------------------
  // TSubsFrame
  //
  struct YAE_API TSubsFrame : public TFrame<TSubsFormat>
  {
    typedef TFrame<TSubsFormat> TBase;

    struct YAE_API TRect
    {
      std::string getAssScript(const TSubsFrame & sf) const;

      TSubtitleType type_;

      int x_;
      int y_;
      int w_;
      int h_;
      int numColors_;

      const unsigned char * data_[4];
      int rowBytes_[4];

      const char * text_;
      const char * assa_;
    };

    struct YAE_API IPrivate
    {
    protected:
      virtual ~IPrivate() {}

    public:
      static void deallocator(IPrivate * p);

      virtual void destroy() = 0;
      virtual boost::shared_ptr<IPrivate> clone() const = 0;
      virtual std::size_t headerSize() const = 0;
      virtual const unsigned char * header() const = 0;

      virtual unsigned int numRects() const = 0;
      virtual void getRect(unsigned int i, TRect & rect) const = 0;
    };

    TSubsFrame();

    bool operator == (const TSubsFrame & s) const;

    // a flag indicating whether embedded start/end ASS timings
    // must be rewritten prior to being passed to libass:
    bool rewriteTimings_;

    // a flag indicating whether this subtitle frame should be rendered:
    bool render_;

    // reference frame width and height, if known:
    int rh_;
    int rw_;

    // track specific data (sequence header, etc...)
    TIPlanarBufferPtr extraData_;

    // additional frame data:
    std::map<AVPacketSideDataType, std::list<TIPlanarBufferPtr> > sideData_;

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
    // helper:
    double durationInSeconds() const;

    std::list<TSubsFrame> subs_;
  };

  //----------------------------------------------------------------
  // TVideoFramePtr
  //
  typedef boost::shared_ptr<TVideoFrame> TVideoFramePtr;

  //----------------------------------------------------------------
  // TAudioFrame
  //
  struct YAE_API TAudioFrame : public TFrame<AudioTraits>
  {
    // helper:
    std::size_t numSamples() const;
    double durationInSeconds() const;
  };

  //----------------------------------------------------------------
  // TAudioFramePtr
  //
  typedef boost::shared_ptr<TAudioFrame> TAudioFramePtr;

  //----------------------------------------------------------------
  // to_hhmmss_ms
  //
  template <typename Traits>
  inline std::string
  to_hhmmss_ms(const boost::shared_ptr<TFrame<Traits> > & f,
               const char * mm_separator = ":",
               const char * ms_separator = ".")
  {
    return f->time_.to_hhmmss_ms(mm_separator, ms_separator);
  }

  //----------------------------------------------------------------
  // to_hhmmss_us
  //
  template <typename Traits>
  inline std::string
  to_hhmmss_us(const boost::shared_ptr<TFrame<Traits> > & f,
               const char * mm_separator = ":",
               const char * us_separator = ".")
  {
    return f->time_.to_hhmmss_us(mm_separator, us_separator);
  }

  //----------------------------------------------------------------
  // TProgramInfo
  //
  struct YAE_API TProgramInfo
  {
    TProgramInfo();

    int id_;
    int program_;
    int pmt_pid_;
    int pcr_pid_;
    TDictionary metadata_;
    std::list<std::size_t> video_;
    std::list<std::size_t> audio_;
    std::list<std::size_t> subs_;
  };

  //----------------------------------------------------------------
  // TTrackInfo
  //
  struct YAE_API TTrackInfo
  {
    TTrackInfo(std::size_t program = 0,
               std::size_t ntracks = 0,
               std::size_t index = 0);

    inline void clear()
    { *this = TTrackInfo(); }

    bool isValid() const;
    bool hasLang() const;
    bool hasName() const;

    const char * lang() const;
    const char * name() const;

    void setLang(const char * lang);
    void setName(const char * name);

    std::size_t nprograms_;
    std::size_t program_;
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
    TChapter(const std::string & name = std::string(),
             const Timespan & span = Timespan());

    inline double t0_sec() const
    { return span_.t0_.sec(); }

    inline double t1_sec() const
    { return span_.t1_.sec(); }

    inline double dt_sec() const
    { return span_.duration_sec(); }

    // chapter name:
    std::string name_;

    // chapter time span:
    Timespan span_;

    // chapter metadata:
    TDictionary metadata_;
  };

  //----------------------------------------------------------------
  // TAttachment
  //
  struct YAE_API TAttachment
  {
    TAttachment(const unsigned char * data = NULL, std::size_t size = 0);

    const unsigned char * data_;
    std::size_t size_;
    TDictionary metadata_;
  };

  //----------------------------------------------------------------
  // make_track_id
  //
  YAE_API std::string
  make_track_id(const char track_type, std::size_t track_index);

  //----------------------------------------------------------------
  // IBookmark
  //
  struct YAE_API IBookmark
  {
    IBookmark();
    virtual ~IBookmark() {}

    std::size_t atrack_;
    std::size_t vtrack_;
    std::list<std::size_t> subs_;
    unsigned int cc_;
    double positionInSeconds_;
  };

  //----------------------------------------------------------------
  // get_curr_program
  //
  YAE_API std::size_t
  get_curr_program(IReader * reader,
                   TTrackInfo & vinfo,
                   TTrackInfo & ainfo,
                   TTrackInfo & sinfo);

  //----------------------------------------------------------------
  // find_matching_program
  //
  YAE_API std::size_t
  find_matching_program(const std::vector<TTrackInfo> & track_info,
                        const TTrackInfo & target);

  //----------------------------------------------------------------
  // find_matching_track
  //
  template <typename TTraits>
  static std::size_t
  find_matching_track(const std::vector<TTrackInfo> & trackInfo,
                      const std::vector<TTraits> & trackTraits,
                      const TTrackInfo & selInfo,
                      const TTraits & selTraits,
                      std::size_t program)
  {
    std::size_t n = trackInfo.size();

    if (!selInfo.isValid())
    {
      // track was disabled, keep it disabled:
      return n;
    }

    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trackInfo.front();
      return info.index_;
    }

    // try to find a matching track:
    std::vector<TTrackInfo> trkInfo = trackInfo;
    std::vector<TTraits> trkTraits = trackTraits;

    std::vector<TTrackInfo> tmpInfo;
    std::vector<TTraits> tmpTraits;

    // try to match the language code:
    if (selInfo.hasLang())
    {
      const char * selLang = selInfo.lang();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        if (program != info.program_)
        {
          continue;
        }

        const TTraits & traits = trkTraits[i];
        if (info.hasLang() && strcmp(info.lang(), selLang) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track name:
    if (selInfo.hasName())
    {
      const char * selName = selInfo.name();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        const TTraits & traits = trkTraits[i];

        if (program != info.program_)
        {
          continue;
        }

        if (info.hasName() && strcmp(info.name(), selName) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track traits:
    for (std::size_t i = 0; i < n; i++)
    {
      const TTrackInfo & info = trkInfo[i];
      const TTraits & traits = trkTraits[i];

      if (program != info.program_)
      {
        continue;
      }

      if (selTraits == traits)
      {
        tmpInfo.push_back(info);
        tmpTraits.push_back(traits);
      }
    }

    if (!tmpInfo.empty())
    {
      trkInfo = tmpInfo;
      trkTraits = tmpTraits;
      tmpInfo.clear();
      tmpTraits.clear();
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      if (program == info.program_)
      {
        return info.index_;
      }
    }

    // try to match track index:
    if (trackInfo.size() == selInfo.ntracks_ &&
        trackInfo[selInfo.index_].program_ == program)
    {
      return selInfo.index_;
    }

    // try to find the first track of the matching program:
    n = trackInfo.size();
    for (std::size_t i = 0; i < n; i++)
    {
      const TTrackInfo & info = trackInfo[i];
      if (info.program_ == program)
      {
        return i;
      }
    }

    // disable track:
    return n;
  }

  //----------------------------------------------------------------
  // hsv_to_rgb
  //
  YAE_API v3x1_t hsv_to_rgb(const v3x1_t & HSV);

  //----------------------------------------------------------------
  // rgb_to_hsv
  //
  YAE_API v3x1_t rgb_to_hsv(const v3x1_t & RGB);

}


#endif // YAE_VIDEO_H_
