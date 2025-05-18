// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 00:58:32 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_VIDEO_H_
#define YAE_VIDEO_H_

// aeyae:
#include "yae/api/yae_api.h"
#include "yae/utils/yae_linear_algebra.h"
#include "yae/utils/yae_time.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_colorspace.h"
#include "yae/video/yae_pixel_formats.h"

// standard:
#include <cstring>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

// ffmpeg:
extern "C"
{
#include <libavcodec/packet.h>
#include <libavutil/channel_layout.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS


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
  // ChannelLayout
  //
  struct YAE_API ChannelLayout : public AVChannelLayout
  {
    ChannelLayout(int nb_channels = 0);
    ChannelLayout(const AVChannelLayout & other);
    ChannelLayout(const ChannelLayout & other);
    ~ChannelLayout();

    ChannelLayout & assign(const AVChannelLayout & other);

    inline ChannelLayout & operator = (const AVChannelLayout & other)
    { return this->assign(other); }

    inline ChannelLayout & operator = (const ChannelLayout & other)
    { return this->assign(other); }

    inline bool operator == (const AVChannelLayout & other) const
    { return av_channel_layout_compare(this, &other) == 0; }

    inline bool operator != (const AVChannelLayout & other) const
    { return av_channel_layout_compare(this, &other) == 1; }

    inline void set_default_layout(int nb_channels)
    { av_channel_layout_default(this, nb_channels); }

    std::string describe() const;
  };

  //----------------------------------------------------------------
  // AudioTraits
  //
  struct YAE_API AudioTraits
  {
    AudioTraits():
      sample_format_(AV_SAMPLE_FMT_NONE),
      sample_rate_(0)
    {}

    inline bool operator == (const AudioTraits & other) const
    {
      return (sample_format_ == other.sample_format_ &&
              sample_rate_ == other.sample_rate_ &&
              ch_layout_ == other.ch_layout_);
    }

    inline bool is_invalid_format() const
    { return sample_format_ <= AV_SAMPLE_FMT_NONE; }

    inline bool is_packed_format() const
    { return av_sample_fmt_is_planar(sample_format_) == 0; }

    inline bool is_planar_format() const
    { return av_sample_fmt_is_planar(sample_format_) == 1; }

    inline AVSampleFormat get_packed_format() const
    { return av_get_packed_sample_fmt(sample_format_); }

    inline AVSampleFormat get_planar_format() const
    { return av_get_planar_sample_fmt(sample_format_); }

    //! bytes required to store 1 sample, for 1 channel:
    inline int get_bytes_per_sample() const
    { return av_get_bytes_per_sample(sample_format_); }

    //! channel layout -- mono, stereo, etc...
    ChannelLayout ch_layout_;

    //! sample format -- 8-bit, 16-bit, float, etc...
    AVSampleFormat sample_format_;

    //! audio sample rate, Hz:
    unsigned int sample_rate_;
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

    void setSomeTraits(const VideoTraits & vt);

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

    template <typename TSample>
    inline TSample * get() const
    { return (TSample *)(data_ + alignmentOffset_); }

    template <typename TSample>
    inline TSample * end() const
    { return (TSample *)(data_ + alignmentOffset_+ rows_ * rowBytes_); }

    template <typename TSample>
    inline std::size_t num() const
    { return this->size() / sizeof(TSample); }

    inline unsigned char * data() const
    { return data_ + alignmentOffset_; }

    inline unsigned char * get() const
    { return data_ + alignmentOffset_; }

    inline unsigned char * end() const
    { return data_ + alignmentOffset_+ rows_ * rowBytes_; }

    inline std::size_t size() const
    { return rows_ * rowBytes_; }

    // bytes per sample row:
    inline std::size_t rowBytes() const
    { return rowBytes_; }

    // rows per plane:
    inline std::size_t rows() const
    { return rows_; }

    // helper:
    inline void memset(uint8_t c)
    { std::memset(this->data(), c, this->size()); }

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
      tempo_(1.0)
    {}

    //! renderer hints bitmask:
    unsigned int rendererHints_;

    //! reader ID tag:
    unsigned int readerId_;

    //! global track ID tag:
    std::string trackId_;

    //! frame PTS timeline:
    TTime time_;

    //! file position timeline, may be used if the PTS timeline
    //! is unreliable (contains gaps, jumps back in time, etc...)
    TTime pos_;

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
  to_hhmmss_ms(const TFrame<Traits> & f,
               const char * mm_separator = ":",
               const char * ms_separator = ".")
  {
    return f.time_.to_hhmmss_ms(mm_separator, ms_separator);
  }

  //----------------------------------------------------------------
  // to_hhmmss_us
  //
  template <typename Traits>
  inline std::string
  to_hhmmss_us(const TFrame<Traits> & f,
               const char * mm_separator = ":",
               const char * us_separator = ".")
  {
    return f.time_.to_hhmmss_us(mm_separator, us_separator);
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
    std::list<std::size_t> subtt_;
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

    inline void setLang(const std::string & lang)
    { lang_ = lang; }

    inline void setName(const std::string & name)
    { name_ = name; }

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

    inline bool operator == (const TAttachment & other) const
    { return metadata_ == other.metadata_ && data_ == other.data_; }

    inline bool operator != (const TAttachment & other) const
    { return !this->operator == (other); }

    yae::Data data_;
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
