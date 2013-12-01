// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:18:35 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// std includes:
#include <new>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// yae includes:
#include <yaeAPI.h>


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
    time_(1000000.0 * seconds),
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
  // TTime::to_hhmmss
  //
  void
    TTime::to_hhmmss(std::string & ts, const char * separator) const
  {
    int64 t = time_;
    t /= base_;

    int64 seconds = t % 60;
    t /= 60;

    int64 minutes = t % 60;
    int64 hours = t / 60;

    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << (int)(hours) << separator
       << std::setw(2) << std::setfill('0') << (int)(minutes) << separator
       << std::setw(2) << std::setfill('0') << (int)(seconds);

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
    to_hhmmss(ts, separator);

    int64 remainder = time_ % base_;
    int64 usec = (1000000 * remainder) / base_;

    std::ostringstream os;
    os << ts << usec_separator
       << std::setw(6) << std::setfill('0') << (int)(usec);

    ts = std::string(os.str().c_str());
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
    // round to nearest frame:
    double seconds = toSeconds();
    double fpsWhole = ceil(frameRate);
    seconds = (seconds * fpsWhole + 0.5) / fpsWhole;

    double secondsWhole = floor(seconds);
    double remainder = seconds - secondsWhole;
    double frame = remainder * fpsWhole;
    uint64 frameNo = int(frame);

    TTime tmp(seconds);
    tmp.to_hhmmss(ts, separator);

    std::ostringstream os;
    os << ts << framenum_separator
       << std::setw(2) << std::setfill('0') << frameNo;

    ts = std::string(os.str().c_str());
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
  AudioTraits::AudioTraits():
    sampleRate_(0),
    sampleFormat_(kAudioInvalidFormat),
    channelFormat_(kAudioChannelFormatInvalid),
    channelLayout_(kAudioChannelLayoutInvalid)
  {}

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
  VideoTraits::VideoTraits():
    frameRate_(0.0),
    pixelFormat_(kInvalidPixelFormat),
    encodedWidth_(0),
    encodedHeight_(0),
    offsetTop_(0),
    offsetLeft_(0),
    visibleWidth_(0),
    visibleHeight_(0),
    pixelAspectRatio_(1.0),
    isUpsideDown_(false)
  {}

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
            isUpsideDown_ == vt.isUpsideDown_);
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

    if (planeSize)
    {
      void * newData = realloc(data_, planeSize + alignment - 1);
      if (!newData)
      {
        throw std::bad_alloc();
      }

      data_ = (unsigned char *)newData;
    }
    else if (data_)
    {
      free(data_);
      data_ = NULL;
    }

    alignmentOffset_ =
      alignment && ((std::size_t)(data_) & (alignment - 1)) ?
      alignment -  ((std::size_t)(data_) & (alignment - 1)) : 0;

    rowBytes_ = rowBytes;
    rows_ = rows;
    alignment_ = alignment;
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
    render_(false),
    index_(~0),
    rh_(0),
    rw_(0)
  {}

  //----------------------------------------------------------------
  // TSubsFrame::operator ==
  //
  bool
  TSubsFrame::operator == (const TSubsFrame & s) const
  {
    bool same = (render_    == s.render_ &&
                 index_     == s.index_ &&
                 rh_        == s.rh_ &&
                 rw_        == s.rw_ &&
                 extraData_ == s.extraData_ &&
                 sideData_  == s.sideData_ &&
                 tEnd_      == s.tEnd_ &&
                 private_   == s.private_ &&
                 TBase::operator == (s));
    return same;
  }

  //----------------------------------------------------------------
  // TTrackInfo::TTrackInfo
  //
  TTrackInfo::TTrackInfo(std::size_t index, std::size_t ntracks):
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

}
