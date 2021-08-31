// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:18:35 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++ library:
#include <algorithm>
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

    av_fmt_ = AV_PIX_FMT_NONE;
    av_rng_ = AVCOL_RANGE_UNSPECIFIED;
    av_pri_ = AVCOL_PRI_UNSPECIFIED;
    av_trc_ = AVCOL_TRC_UNSPECIFIED;
    av_csp_ = AVCOL_SPC_UNSPECIFIED;

    pixelFormat_ = kInvalidPixelFormat;
    pixelAspectRatio_ = 1.0;
  }

  //----------------------------------------------------------------
  // VideoTraits::sameFrameSizeAndFormat
  //
  bool
  VideoTraits::sameFrameSizeAndFormat(const VideoTraits & vt) const
  {
    return (av_fmt_ == vt.av_fmt_ &&
            pixelFormat_ == vt.pixelFormat_ &&
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
    return (av_rng_ == vt.av_rng_ &&
            av_pri_ == vt.av_pri_ &&
            av_trc_ == vt.av_trc_ &&
            av_csp_ == vt.av_csp_);
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
    nprograms_(0),
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


  //----------------------------------------------------------------
  // get_curr_program
  //
  std::size_t
  get_curr_program(IReader * reader,
                   TTrackInfo & vinfo,
                   TTrackInfo & ainfo,
                   TTrackInfo & sinfo)
  {
    vinfo = TTrackInfo(0, 0);
    ainfo = TTrackInfo(0, 0);
    sinfo = TTrackInfo(0, 0);

    std::size_t ix_vtrack = reader->getSelectedVideoTrackIndex();
    std::size_t n_vtracks = reader->getNumberOfVideoTracks();
    if (ix_vtrack < n_vtracks)
    {
      reader->getSelectedVideoTrackInfo(vinfo);
    }

    std::size_t ix_atrack = reader->getSelectedAudioTrackIndex();
    std::size_t n_atracks = reader->getNumberOfAudioTracks();
    if (ix_atrack < n_atracks)
    {
      reader->getSelectedAudioTrackInfo(ainfo);
    }

    std::size_t n_subs = reader->subsCount();
    for (std::size_t i = 0; i < n_subs; i++)
    {
      if (reader->getSubsRender(i))
      {
        reader->subsInfo(i, sinfo);
        break;
      }
    }

    return (vinfo.isValid() ? vinfo.program_ :
            ainfo.isValid() ? ainfo.program_ :
            sinfo.isValid() ? sinfo.program_ :
            0);
  }

  //----------------------------------------------------------------
  // find_matching_program
  //
  std::size_t
  find_matching_program(const std::vector<TTrackInfo> & track_info,
                        const TTrackInfo & target)
  {
    std::size_t program = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0, n = track_info.size(); i < n; i++)
    {
      const TTrackInfo & info = track_info[i];
      if (target.nprograms_ == info.nprograms_ &&
          target.program_ == info.program_ &&
          target.ntracks_ == n)
      {
        return target.program_;
      }

      program = std::min(program, info.program_);
    }

    return track_info.empty() ? 0 : program;
  }

  //----------------------------------------------------------------
  // hsv_to_rgb
  //
  v3x1_t
  hsv_to_rgb(const v3x1_t & HSV)
  {
    double H = HSV[0];
    double S = HSV[1];
    double V = HSV[2];

    v3x1_t RGB;
    double & R = RGB[0];
    double & G = RGB[1];
    double & B = RGB[2];

    if (S == 0.0)
    {
      // monochromatic:
      R = V;
      G = V;
      B = V;
      return RGB;
    }

    H *= 6.0;
    double i = floor(H);
    double f = H - i;

    double p = V * (1.0 - S);
    double q = V * (1.0 - S * f);
    double t = V * (1.0 - S * (1.0 - f));

    if (i == 0.0)
    {
      R = V;
      G = t;
      B = p;
    }
    else if (i == 1.0)
    {
      R = q;
      G = V;
      B = p;
    }
    else if (i == 2.0)
    {
      R = p;
      G = V;
      B = t;
    }
    else if (i == 3.0)
    {
      R = p;
      G = q;
      B = V;
    }
    else if (i == 4.0)
    {
      R = t;
      G = p;
      B = V;
    }
    else
    {
      // i == 5.0
      R = V;
      G = p;
      B = q;
    }

    return RGB;
  }

  //----------------------------------------------------------------
  // rgb_to_hsv
  //
  v3x1_t
  rgb_to_hsv(const v3x1_t & RGB)
  {
    double R = RGB[0];
    double G = RGB[1];
    double B = RGB[2];

    v3x1_t HSV;
    double & H = HSV[0];
    double & S = HSV[1];
    double & V = HSV[2];

    double min = std::min(R, std::min(G, B));
    double max = std::max(R, std::max(G, B));
    V = max;

    double delta = max - min;
    if (max == 0)
    {
      S = 0;
      H = -1;
    }
    else
    {
      S = delta / max;

      if (delta == 0)
      {
        delta = 1;
      }

      if (R == max)
      {
        // between yellow & magenta
        H = (G - B) / delta;
      }
      else if (G == max)
      {
        // between cyan & yellow
        H = (B - R) / delta + 2;
      }
      else
      {
        // between magenta & cyan
        H = (R - G) / delta + 4;
      }

      H /= 6.0;

      if (H < 0.0)
      {
        H = H + 1.0;
      }
    }

    return HSV;
  }


}
