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
#include "yae_reader.h"


namespace yae
{

  //----------------------------------------------------------------
  // get_timeline
  //
  bool
  get_timeline(const IReader * reader, TTime & t0, TTime & t1)
  {
    t0 = TTime(0, 0);
    t1 = TTime(0, 0);

    if (reader)
    {
      TTime duration(0, 0);

      std::size_t nv = reader->getNumberOfVideoTracks();
      std::size_t na = reader->getNumberOfAudioTracks();

      std::size_t iv = reader->getSelectedVideoTrackIndex();
      std::size_t ia = reader->getSelectedAudioTrackIndex();

      if (ia < na)
      {
        reader->getAudioDuration(t0, duration);
      }
      else if (iv < nv)
      {
        reader->getVideoDuration(t0, duration);
      }

      if (t0.valid() && duration.valid())
      {
        t1 = t0 + duration;
      }
    }

    return t0.valid() && t1.valid();
  }

  //----------------------------------------------------------------
  // get_program_name
  //
  std::string
  get_program_name(const IReader & reader, std::size_t program)
  {
    TProgramInfo info;

    if (reader.getProgramInfo(program, info))
    {
      return get(info.metadata_, std::string("service_name"));
    }

    return std::string();
  }

  //----------------------------------------------------------------
  // get_selected_subtt_track
  //
  int
  get_selected_subtt_track(const IReader & reader)
  {
    int nsubs = int(reader.subsCount());
    unsigned int cc = reader.getRenderCaptions();

    // if closed captions are selected -- return that (offset by nsubs):
    if (cc)
    {
      return nsubs + cc - 1;
    }

    // find the 1st selected subtitles track, if any:
    int si = 0;
    for (; si < nsubs && !reader.getSubsRender(si); si++)
    {}

    if (si < nsubs)
    {
      return si;
    }

    // disabled:
    return nsubs + 4;
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
  // TVideoFrame::durationInSeconds
  //
  double
  TVideoFrame::durationInSeconds() const
  {
    double dt =
      traits_.frameRate_ > 0.0 ?
      1.0 / traits_.frameRate_ :
      0.0;
    return dt;
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
  // TAudioFrame::durationInSeconds
  //
  double
  TAudioFrame::durationInSeconds() const
  {
    std::size_t samples = numSamples();
    double sec = double(samples) / double(traits_.sampleRate_);
    return sec;
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
  TChapter::TChapter(const std::string & n, const Timespan & s):
    name_(n),
    span_(s)
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


  //----------------------------------------------------------------
  // IBookmark::IBookmark
  //
  IBookmark::IBookmark():
    atrack_(std::numeric_limits<std::size_t>::max()),
    vtrack_(std::numeric_limits<std::size_t>::max()),
    cc_(0),
    positionInSeconds_(0)
  {}

}
