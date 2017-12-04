// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:18:35 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// yae includes:
#include "yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime():
    time_(0),
    base_(1001)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime(int64 time, uint64 base):
    time_(time),
    base_(base)
  {}

  //----------------------------------------------------------------
  // TTime::TTime
  //
  TTime::TTime(double seconds):
    time_((int64)(1000000.0 * seconds)),
    base_(1000000)
  {}

  //----------------------------------------------------------------
  // TTime::operator +=
  //
  TTime &
  TTime::operator += (const TTime & dt)
  {
    if (base_ == dt.base_)
    {
      time_ += dt.time_;
      return *this;
    }

    return operator += (dt.toSeconds());
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime
  TTime::operator + (const TTime & dt) const
  {
    TTime t(*this);
    t += dt;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime &
  TTime::operator += (double dtSec)
  {
    time_ += int64(dtSec * double(base_));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator +
  //
  TTime
  TTime::operator + (double dtSec) const
  {
    TTime t(*this);
    t += dtSec;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator -=
  //
  TTime &
  TTime::operator -= (const TTime & dt)
  {
    if (base_ == dt.base_)
    {
      time_ -= dt.time_;
      return *this;
    }

    return operator -= (dt.toSeconds());
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime
  TTime::operator - (const TTime & dt) const
  {
    TTime t(*this);
    t -= dt;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime &
  TTime::operator -= (double dtSec)
  {
    time_ -= int64(dtSec * double(base_));
    return *this;
  }

  //----------------------------------------------------------------
  // TTime::operator -
  //
  TTime
  TTime::operator - (double dtSec) const
  {
    TTime t(*this);
    t -= dtSec;
    return t;
  }

  //----------------------------------------------------------------
  // TTime::operator <
  //
  bool
  TTime::operator < (const TTime & t) const
  {
    if (t.base_ == base_)
    {
      return time_ < t.time_;
    }

    return toSeconds() < t.toSeconds();
  }

  //----------------------------------------------------------------
  // TTime::operator <=
  //
  bool
  TTime::operator <= (const TTime & t) const
  {
    if (t.base_ == base_)
    {
      return time_ <= t.time_;
    }

    double dt = toSeconds() - t.toSeconds();
    return dt <= 0;
  }

  //----------------------------------------------------------------
  // TTime::getTime
  //
  int64
  TTime::getTime(uint64 base) const
  {
    if (base_ == base)
    {
      return time_;
    }

    TTime t(0, base);
    t += *this;
    return t.time_;
  }

  //----------------------------------------------------------------
  // to_hhmmss
  //
  static bool
  to_hhmmss(int64 time,
            uint64 base,
            std::string & ts,
            const char * separator,
            bool includeNegativeSign = false)
  {
    bool negative = (time < 0);

    int64 t = negative ? -time : time;
    t /= base;

    int64 seconds = t % 60;
    t /= 60;

    int64 minutes = t % 60;
    int64 hours = t / 60;

    std::ostringstream os;

    if (negative && includeNegativeSign && (seconds || minutes || hours))
    {
      os << '-';
    }

    os << std::setw(2) << std::setfill('0') << (int64)(hours) << separator
       << std::setw(2) << std::setfill('0') << (int)(minutes) << separator
       << std::setw(2) << std::setfill('0') << (int)(seconds);

    ts = std::string(os.str().c_str());

    return negative;
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss
  //
  void
  TTime::to_hhmmss(std::string & ts, const char * separator) const
  {
    yae::to_hhmmss(time_, base_, ts, separator, true);
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_frac
  //
  void
  TTime::to_hhmmss_frac(std::string & ts,
                        unsigned int precision,
                        const char * separator,
                        const char * remainder_separator) const
  {
    bool negative = yae::to_hhmmss(time_, base_, ts, separator);

    uint64 t = negative ? -time_ : time_;
    uint64 remainder = t % base_;
    uint64 frac = (precision * remainder) / base_;

    // count number of digits required for given precision:
    uint64 digits = 0;
    for (unsigned int i = precision - 1; precision && i; i /= 10, digits++) ;

    std::ostringstream os;

    if (negative && (frac || t >= base_))
    {
      os << '-';
    }

    os << ts;

    if (digits)
    {
      os << remainder_separator
         << std::setw(digits) << std::setfill('0') << (int)(frac);
    }

    ts = std::string(os.str().c_str());
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_usec
  //
  void
  TTime::to_hhmmss_usec(std::string & ts,
                        const char * separator,
                        const char * usec_separator) const
  {
    to_hhmmss_frac(ts, 1000000, separator, usec_separator);
  }

  //----------------------------------------------------------------
  // TTime::to_hhmmss_frame
  //
  void
  TTime::to_hhmmss_frame(std::string & ts,
                         double frameRate,
                         const char * separator,
                         const char * framenum_separator) const
  {
    bool negative = (time_ < 0);

    // round to nearest frame:
    double seconds = toSeconds();

    if (negative)
    {
      seconds = -seconds;
    }

    double fpsWhole = ceil(frameRate);
    seconds = (seconds * fpsWhole + 0.5) / fpsWhole;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double frame = remainder * fpsWhole;
    uint64 frameNo = int(frame);

    TTime tmp(seconds);
    tmp.to_hhmmss(ts, separator);

    std::ostringstream os;

    if (negative && (frameNo || (uint64)tmp.time_ >= tmp.base_))
    {
      os << '-';
    }

    os << ts << framenum_separator
       << std::setw(2) << std::setfill('0') << frameNo;

    ts = std::string(os.str().c_str());
  }


  //----------------------------------------------------------------
  // standardFrameRates
  //
  static const double standardFrameRates[] = {
    24000.0 / 1001.0,
    24.0,
    25.0,
    30000.0 / 1001.0,
    30.0,
    50.0,
    60000.0 / 1001.0,
    60.0,
    120.0,
    120000.0 / 1001.0,
    240.0,
    240000.0 / 1001.0,
    480.0,
    480000.0 / 1001.0
  };

  //----------------------------------------------------------------
  // closestStandardFrameRate
  //
  double
  closestStandardFrameRate(double fps)
  {
    const std::size_t n = sizeof(standardFrameRates) / sizeof(double);
    double bestErr = std::numeric_limits<double>::max();
    double closest = fps;
    for (std::size_t i = 0; i < n; i++)
    {
      double err = fabs(fps - standardFrameRates[i]);
      if (err <= bestErr)
      {
        bestErr = err;
        closest = standardFrameRates[i];
      }
    }

    return (bestErr < 1e-3) ? closest : fps;
  }

  //----------------------------------------------------------------
  // timeBaseForFrameRate
  //
  // base_ -- ticks per second
  // time_ -- frame duration expressed per base_
  //
  TTime
  frameDurationForFrameRate(double fps)
  {
    double frameDuration = 1000000.0;

    double frac = ceil(fps) - fps;
    if (frac == 0.0)
    {
      frameDuration = 1000.0;
    }
    else
    {
      double stdFps = closestStandardFrameRate(fps);
      double fpsErr = fabs(stdFps - fps);
      if (fpsErr < 1e-3)
      {
        frac = ceil(stdFps) - stdFps;
        frameDuration = (frac > 0) ? 1001.0 : 1000.0;
        fps = stdFps;
      }
    }

    return TTime(int64(frameDuration), uint64(frameDuration * fps));
  }

  //----------------------------------------------------------------
  // getBitsPerSample
  //
  unsigned int
  getBitsPerSample(TAudioSampleFormat sampleFormat)
  {
    switch (sampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return 8;

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        return 16;

      case kAudio24BitLittleEndian:
        return 24;

      case kAudio32BitFloat:
      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        return 32;

      case kAudio64BitDouble:
        return 64;

      default:
        break;
    }

    YAE_ASSERT(false);
    return 0;
  }

  //----------------------------------------------------------------
  // getNumberOfChannels
  //
  unsigned int
  getNumberOfChannels(TAudioChannelLayout channelLayout)
  {
    switch (channelLayout)
    {
      case kAudioMono:
        return 1;

      case kAudioStereo:
        return 2;

      case kAudio2Pt1:
        return 3;

      case kAudioQuad:
        return 4;

      case kAudio4Pt1:
        return 5;

      case kAudio5Pt1:
        return 6;

      case kAudio6Pt1:
        return 7;

      case kAudio7Pt1:
        return 8;

      default:
        break;
    }

    YAE_ASSERT(false);
    return 0;
  }

  //----------------------------------------------------------------
  // AudioTraits::AudioTraits
  //
  AudioTraits::AudioTraits()
  {
    memset(this, 0, sizeof(AudioTraits));

    sampleFormat_ = kAudioInvalidFormat;
    channelFormat_ = kAudioChannelFormatInvalid;
    channelLayout_ = kAudioChannelLayoutInvalid;
  }

  //----------------------------------------------------------------
  // AudioTraits::operator
  //
  bool
  AudioTraits::operator == (const AudioTraits & at) const
  {
    return memcmp(this, &at, sizeof(AudioTraits)) == 0;
  }

  //----------------------------------------------------------------
  // VideoTraits::VideoTraits
  //
  VideoTraits::VideoTraits()
  {
    memset(this, 0, sizeof(VideoTraits));

    pixelFormat_ = kInvalidPixelFormat;
    colorSpace_ = kColorSpaceUnspecified;
    colorRange_ = kColorRangeUnspecified;
    pixelAspectRatio_ = 1.0;
  }

  //----------------------------------------------------------------
  // VideoTraits::sameFrameSizeAndFormat
  //
  bool
  VideoTraits::sameFrameSizeAndFormat(const VideoTraits & vt) const
  {
    return (pixelFormat_ == vt.pixelFormat_ &&
            encodedWidth_ == vt.encodedWidth_ &&
            encodedHeight_ == vt.encodedHeight_ &&
            offsetTop_ == vt.offsetTop_ &&
            offsetLeft_ == vt.offsetLeft_ &&
            visibleWidth_ == vt.visibleWidth_ &&
            visibleHeight_ == vt.visibleHeight_ &&
            pixelAspectRatio_ == vt.pixelAspectRatio_ &&
            cameraRotation_ == vt.cameraRotation_ &&
            isUpsideDown_ == vt.isUpsideDown_);
  }

  //----------------------------------------------------------------
  // VideoTraits::sameColorSpaceAndRange
  //
  bool
  VideoTraits::sameColorSpaceAndRange(const VideoTraits & vt) const
  {
    return (colorSpace_ == vt.colorSpace_ &&
            colorRange_ == vt.colorRange_);
  }

  //----------------------------------------------------------------
  // VideoTraits::operator
  //
  bool
  VideoTraits::operator == (const VideoTraits & vt) const
  {
    return memcmp(this, &vt, sizeof(VideoTraits)) == 0;
  }

  //----------------------------------------------------------------
  // IPlanarBuffer::~IPlanarBuffer
  //
  IPlanarBuffer::~IPlanarBuffer()
  {}

  //----------------------------------------------------------------
  // IPlanarBuffer::deallocator
  //
  void
  IPlanarBuffer::deallocator(IPlanarBuffer * sb)
  {
    if (sb)
    {
      sb->destroy();
    }
  }

  //----------------------------------------------------------------
  // TDataBuffer::TDataBuffer
  //
  TDataBuffer::TDataBuffer():
    data_(NULL),
    alignmentOffset_(0),
    rowBytes_(0),
    rows_(0),
    alignment_(0)
  {}

  //----------------------------------------------------------------
  // TDataBuffer::~TDataBuffer
  //
  TDataBuffer::~TDataBuffer()
  {
    if (data_)
    {
      free(data_);
    }
  }

  //----------------------------------------------------------------
  // TDataBuffer::TDataBuffer
  //
  TDataBuffer::TDataBuffer(const TDataBuffer & src):
    data_(NULL),
    alignmentOffset_(0),
    rowBytes_(0),
    rows_(0),
    alignment_(0)
  {
    *this = src;
  }

  //----------------------------------------------------------------
  // TDataBuffer::operator =
  //
  TDataBuffer &
  TDataBuffer::operator = (const TDataBuffer & src)
  {
    YAE_ASSERT(this != &src);

    if (this != &src)
    {
      resize(src.rowBytes_, src.rows_, src.alignment_);
      memcpy(this->data(), src.data(), src.rowBytes_ * src.rows_);
    }

    return *this;
  }

  //----------------------------------------------------------------
  // TDataBuffer::resize
  //
  void
  TDataBuffer::resize(std::size_t rowBytes,
                      std::size_t rows,
                      std::size_t alignment)
  {
    std::size_t planeSize = (rowBytes * rows);
    std::size_t alignmentOffset = 0;
    std::size_t currentSize = rows_ * rowBytes_;

    if (alignment_ == alignment && currentSize == planeSize)
    {
      rowBytes_ = rowBytes;
      rows_ = rows;
      return;
    }

    if (planeSize)
    {
      // should not use realloc because it may return a pointer with
      // a different alignment offset than was returned previousely,
      // and will require memmove to shift previous data to the new
      // alignment offset; it's simpler to malloc and memcpy instead:
      unsigned char * newData =
        (unsigned char *)malloc(planeSize + alignment - 1);

      if (!newData)
      {
        throw std::bad_alloc();
      }

      alignmentOffset =
        alignment && ((std::size_t)(newData) & (alignment - 1)) ?
        alignment -  ((std::size_t)(newData) & (alignment - 1)) : 0;

      if (data_)
      {
        const unsigned char * src = data_ + alignmentOffset_;
        unsigned char * dst = newData + alignmentOffset;
        memcpy(dst, src, currentSize);
      }

      free(data_);
      data_ = newData;
    }
    else if (data_)
    {
      free(data_);
      data_ = NULL;
    }

    rowBytes_ = rowBytes;
    rows_ = rows;
    alignment_ = alignment;
    alignmentOffset_ = alignmentOffset;
  }


  //----------------------------------------------------------------
  // TPlanarBuffer::TPlanarBuffer
  //
  TPlanarBuffer::TPlanarBuffer(std::size_t numSamplePlanes):
    plane_(numSamplePlanes)
  {}

  //----------------------------------------------------------------
  // TPlanarBuffer::destroy
  //
  void
  TPlanarBuffer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // TPlanarBuffer::planes
  //
  std::size_t
  TPlanarBuffer::planes() const
  {
    return plane_.size();
  }

  //----------------------------------------------------------------
  // TPlanarBuffer::data
  //
  unsigned char *
  TPlanarBuffer::data(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].data() : NULL;
  }

  //----------------------------------------------------------------
  // TPlanarBuffer::rowBytes
  //
  std::size_t
  TPlanarBuffer::rowBytes(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].rowBytes() : 0;
  }

  //----------------------------------------------------------------
  // TPlanarBuffer::rows
  //
  std::size_t
  TPlanarBuffer::rows(std::size_t samplePlane) const
  {
    return samplePlane < plane_.size() ? plane_[samplePlane].rows() : 0;
  }

  //----------------------------------------------------------------
  // TPlanarBuffer::resize
  //
  void
  TPlanarBuffer::resize(std::size_t samplePlane,
                        std::size_t rowBytes,
                        std::size_t rows,
                        std::size_t alignment)
  {
    YAE_ASSERT(samplePlane < plane_.size());
    if (samplePlane < plane_.size())
    {
      plane_[samplePlane].resize(rowBytes, rows, alignment);
    }
  }

  //----------------------------------------------------------------
  // getSubsFormatLabel
  //
  const char *
  getSubsFormatLabel(TSubsFormat fmt)
  {
    switch (fmt)
    {
      case kSubsDVD:
        return "DVD";

      case kSubsDVB:
        return "DVB";

      case kSubsText:
        return "plain text";

      case kSubsXSUB:
        return "XSUB";

      case kSubsSSA:
        return "SSA/ASS";

      case kSubsMovText:
        return "QuickTime";

      case kSubsHDMVPGS:
        return "HDMV PGS";

      case kSubsDVBTeletext:
        return "DVB Teletext";

      case kSubsSRT:
        return "SRT";

      case kSubsMICRODVD:
        return "MICRODVD";

      case kSubsCEA608:
        return "CEA-608";

      case kSubsJACOSUB:
        return "JACOSUB";

      case kSubsSAMI:
        return "SAMI";

      case kSubsREALTEXT:
        return "REALTEXT";

      case kSubsSUBVIEWER:
        return "SUBVIEWER";

      case kSubsSUBRIP:
        return "SUBRIP";

      case kSubsWEBVTT:
        return "WEBVTT";

      case kSubsNone:
        return "none";

      default:
        YAE_ASSERT(false);
        break;
    }

    return "unknown";
  }

  //----------------------------------------------------------------
  // TSubsFrame::IPrivate::deallocator
  //
  void
  TSubsFrame::IPrivate::deallocator(IPrivate * p)
  {
    if (p)
    {
      p->destroy();
    }
  }

  //----------------------------------------------------------------
  // TSubsFrame::TSubsFrame
  //
  TSubsFrame::TSubsFrame():
    rewriteTimings_(false),
    render_(false),
    rh_(0),
    rw_(0)
  {}

  //----------------------------------------------------------------
  // TSubsFrame::operator ==
  //
  bool
  TSubsFrame::operator == (const TSubsFrame & s) const
  {
    bool same = (rewriteTimings_ == s.rewriteTimings_ &&
                 render_         == s.render_ &&
                 trackId_        == s.trackId_ &&
                 rh_             == s.rh_ &&
                 rw_             == s.rw_ &&
                 extraData_      == s.extraData_ &&
                 sideData_       == s.sideData_ &&
                 tEnd_           == s.tEnd_ &&
                 private_        == s.private_ &&
                 time_           == s.time_ &&
                 tempo_          == s.tempo_ &&
                 traits_         == s.traits_ &&
                 data_           == s.data_);

    return same;
  }


  //----------------------------------------------------------------
  // TSubsFrame::TRect::getAssScript
  //
  std::string
  TSubsFrame::TRect::getAssScript(const TSubsFrame & sf) const
  {
    if (!sf.rewriteTimings_)
    {
      return std::string(assa_);
    }

    // must rewrite Start and End timestamps:
    std::string t0;
    sf.time_.to_hhmmss_frac(t0, 100);

    std::string t1;
    sf.tEnd_.to_hhmmss_frac(t1, 100);

    if (!(assa_ && strncmp(assa_, "Dialogue:", 9) == 0))
    {
      std::string();
    }

    // Dialogue: 0,24:25:00.98,24:25:01.98,Default,,0,0,0,,text here
    std::string tmp(assa_);
    std::string::size_type c0 = tmp.find(',', 9);
    std::string::size_type c1 = tmp.find(',', c0 + 1);
    std::string::size_type c2 = tmp.find(',', c1 + 1);

    std::ostringstream oss;
    oss << tmp.substr(0, c0) << ','
        << t0 << ','
        << t1 << tmp.substr(c2);
    return std::string(oss.str().c_str());
  }


  //----------------------------------------------------------------
  // TAudioFrame::numSamples
  //
  std::size_t
  TAudioFrame::numSamples() const
  {
    unsigned int sampleSize = getBitsPerSample(traits_.sampleFormat_) / 8;
    YAE_ASSERT(sampleSize > 0);

    int channels = getNumberOfChannels(traits_.channelLayout_);
    YAE_ASSERT(channels > 0);

    std::size_t bytesPerSample = channels * sampleSize;
    std::size_t frameSize = data_->rowBytes(0);
    std::size_t samples = bytesPerSample ? (frameSize / bytesPerSample) : 0;
    return samples;
  }


  //----------------------------------------------------------------
  // TProgramInfo::TProgramInfo
  //
  TProgramInfo::TProgramInfo():
    id_(0),
    program_(0),
    pmt_pid_(0),
    pcr_pid_(0)
  {}


  //----------------------------------------------------------------
  // TTrackInfo::TTrackInfo
  //
  TTrackInfo::TTrackInfo(std::size_t program,
                         std::size_t ntracks,
                         std::size_t index):
    program_(program),
    ntracks_(ntracks),
    index_(index)
  {}

  //----------------------------------------------------------------
  // TTrackInfo::isValid
  //
  bool
  TTrackInfo::isValid() const
  {
    return index_ < ntracks_;
  }

  //----------------------------------------------------------------
  // TTrackInfo::hasLang
  //
  bool
  TTrackInfo::hasLang() const
  {
    return lang_.size() > 0 && lang_[0];
  }

  //----------------------------------------------------------------
  // TTrackInfo::hasName
  //
  bool
  TTrackInfo::hasName() const
  {
    return name_.size() > 0 && name_[0] && name_ != "und";
  }

  //----------------------------------------------------------------
  // TTrackInfo::lang
  //
  const char *
  TTrackInfo::lang() const
  {
    return hasLang() ? lang_.c_str() : NULL;
  }

  //----------------------------------------------------------------
  // TTrackInfo::name
  //
  const char *
  TTrackInfo::name() const
  {
    return hasName() ? name_.c_str() : NULL;
  }

  //----------------------------------------------------------------
  // TTrackInfo::setLang
  //
  void
  TTrackInfo::setLang(const char * lang)
  {
    if (lang)
    {
      lang_ = lang;
    }
    else
    {
      lang_.clear();
    }
  }

  //----------------------------------------------------------------
  // TTrackInfo::setName
  //
  void
  TTrackInfo::setName(const char * name)
  {
    if (name)
    {
      name_ = name;
    }
    else
    {
      name_.clear();
    }
  }


  //----------------------------------------------------------------
  // TChapter::TChapter
  //
  TChapter::TChapter(const std::string & n, double t, double dt):
    name_(n),
    start_(t),
    duration_(dt)
  {}


  //----------------------------------------------------------------
  // TAttachment::TAttachment
  //
  TAttachment::TAttachment(const unsigned char * data, std::size_t size):
    data_(data),
    size_(size)
  {}

  //----------------------------------------------------------------
  // make_track_id
  //
  std::string
  make_track_id(const char track_type, std::size_t track_index)
  {
    std::ostringstream oss;
    oss << track_type << ':'
        << std::setw(3) << std::setfill('0') << track_index;
    return oss.str();
  }

}
