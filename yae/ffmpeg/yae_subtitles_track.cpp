// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae/ffmpeg/yae_ffmpeg_utils.h"
#include "yae/ffmpeg/yae_subtitles_track.h"
#include "yae/utils/yae_utils.h"


namespace yae
{

  //----------------------------------------------------------------
  // getSubsFormat
  //
  static TSubsFormat
  getSubsFormat(int id)
  {
    switch (id)
    {
      case AV_CODEC_ID_DVD_SUBTITLE:
        return kSubsDVD;

      case AV_CODEC_ID_DVB_SUBTITLE:
        return kSubsDVB;

      case AV_CODEC_ID_TEXT:
        return kSubsText;

      case AV_CODEC_ID_XSUB:
        return kSubsXSUB;

      case AV_CODEC_ID_SSA:
        return kSubsSSA;

      case AV_CODEC_ID_MOV_TEXT:
        return kSubsMovText;

      case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        return kSubsHDMVPGS;

      case AV_CODEC_ID_DVB_TELETEXT:
        return kSubsDVBTeletext;

      case AV_CODEC_ID_SRT:
        return kSubsSRT;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(56, 1, 0)
      case AV_CODEC_ID_MICRODVD:
        return kSubsMICRODVD;

      case AV_CODEC_ID_EIA_608:
        return kSubsCEA608;

      case AV_CODEC_ID_JACOSUB:
        return kSubsJACOSUB;

      case AV_CODEC_ID_SAMI:
        return kSubsSAMI;

      case AV_CODEC_ID_REALTEXT:
        return kSubsREALTEXT;

      case AV_CODEC_ID_SUBVIEWER:
        return kSubsSUBVIEWER;

      case AV_CODEC_ID_SUBRIP:
        return kSubsSUBRIP;

      case AV_CODEC_ID_WEBVTT:
        return kSubsWEBVTT;
#endif

      default:
        break;
    }

    return kSubsNone;
  }


  //----------------------------------------------------------------
  // TVobSubSpecs::TVobSubSpecs
  //
  TVobSubSpecs::TVobSubSpecs():
    x_(0),
    y_(0),
    w_(0),
    h_(0),
    scalex_(1.0),
    scaley_(1.0),
    alpha_(1.0)
  {}

  //----------------------------------------------------------------
  // TVobSubSpecs::init
  //
  void
  TVobSubSpecs::init(const unsigned char * extraData, std::size_t size)
  {
    const char * line = (const char *)extraData;
    const char * end = line + size;

    while (line < end)
    {
      // parse one line at a time:
      const char * lEnd = strstr(line, "\n");
      if (!lEnd)
      {
        lEnd = end;
      }

      const char * found = 0;
      if ((found = strstr(line, "size:")) && found < lEnd)
      {
        const char * strw = found + 5;
        const char * strh = strstr(strw, "x");
        if (strh)
        {
          strh++;

          w_ = toScalar<int, std::string>(std::string(strw, strh - 1));
          h_ = toScalar<int, std::string>(std::string(strh, lEnd));
        }
      }
      else if ((found = strstr(line, "org:")) && found < lEnd)
      {
        const char * strx = found + 4;
        const char * stry = strstr(strx, ",");
        if (stry)
        {
          stry++;

          x_ = toScalar<int, std::string>(std::string(strx, stry - 1));
          y_ = toScalar<int, std::string>(std::string(stry, lEnd));
        }
      }
      else if ((found = strstr(line, "scale:")) && found < lEnd)
      {
        const char * strx = found + 6;
        const char * stry = strstr(strx, ",");
        if (stry)
        {
          stry++;

          int x = toScalar<int, std::string>(std::string(strx, stry - 2));
          int y = toScalar<int, std::string>(std::string(stry, lEnd - 1));

          scalex_ = double(x) / 100.0;
          scaley_ = double(y) / 100.0;
        }
      }
      else if ((found = strstr(line, "alpha:")) && found < lEnd)
      {
        const char * str = found + 6;
        int x = toScalar<int, std::string>(std::string(str, lEnd - 1));
        alpha_ = double(x) / 100.0;
      }
      else if ((found = strstr(line, "palette:")) && found < lEnd)
      {
        const char * str = found + 8;
        std::list<std::string> colors;

        while (str && str < lEnd)
        {
          while (*str == ' ')
          {
            str++;
          }

          const char * next = strstr(str, ",");
          next = std::min<const char *>(next, lEnd);

          std::string color(str, next ? next : lEnd);
          color = std::string("#") + color;

          colors.push_back(color);
          str = next ? next + 1 : NULL;
        }

        palette_.assign(colors.begin(), colors.end());
      }

      line = lEnd + 1;
    }
  }


  //----------------------------------------------------------------
  // SubtitlesTrack::SubtitlesTrack
  //
  SubtitlesTrack::SubtitlesTrack(AVStream * stream, std::size_t index):
    stream_(stream),
    codec_(NULL),
    codecContext_(NULL),
    render_(false),
    format_(kSubsNone),
    index_(index)
  {
    queue_.setMaxSizeUnlimited();
    open();
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::~SubtitlesTrack
  //
  SubtitlesTrack::~SubtitlesTrack()
  {
    close();
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::clear
  //
  void
  SubtitlesTrack::clear()
  {
    queue_.clear();
    active_.clear();
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::open
  //
  void
  SubtitlesTrack::open()
  {
    if (stream_)
    {
      format_ = getSubsFormat(stream_->codecpar->codec_id);
      codec_ = find_best_decoder_for(*(stream_->codecpar));
      active_.clear();

      if (codec_)
      {
        YAE_ASSERT(!codecContext_);
        codecContext_ = avcodec_alloc_context3(codec_);

        AVDictionary * opts = NULL;
        av_dict_set(&opts, "threads", "auto", 0);
        av_dict_set(&opts, "refcounted_frames", "1", 0);

        avcodec_parameters_to_context(codecContext_, stream_->codecpar);
        codecContext_->time_base = stream_->time_base;

        int err = avcodec_open2(codecContext_, codec_, &opts);
        if (err < 0)
        {
          // unsupported codec:
          avcodec_free_context(&codecContext_);
          codec_ = NULL;
        }
        else if (codecContext_->extradata &&
                 codecContext_->extradata_size)
        {
          TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                  &IPlanarBuffer::deallocator);
          buffer->resize(0, codecContext_->extradata_size, 1, 1);

          unsigned char * dst = buffer->data(0);
          memcpy(dst,
                 codecContext_->extradata,
                 codecContext_->extradata_size);

          extraData_ = buffer;

          if (format_ == kSubsDVD)
          {
            vobsub_.init(codecContext_->extradata,
                         codecContext_->extradata_size);
          }
        }
      }

      const char * name = getTrackName(stream_->metadata);
      if (name)
      {
        name_.assign(name);
      }
      else
      {
        name_.clear();
      }

      const char * lang = getTrackLang(stream_->metadata);
      if (lang)
      {
        lang_.assign(lang);
      }
      else
      {
        lang_.clear();
      }

      queue_.open();
    }
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::close
  //
  void
  SubtitlesTrack::close()
  {
    clear();

    if (stream_ && codecContext_)
    {
      avcodec_close(codecContext_);
      avcodec_free_context(&codecContext_);
      codec_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::fixupEndTime
  //
  void
  SubtitlesTrack::fixupEndTime(double v1,
                               TSubsFrame & prev,
                               const TSubsFrame & next)
  {
    if (prev.tEnd_.time_ == std::numeric_limits<int64>::max())
    {
      double s0 = prev.time_.toSeconds();
      double s1 = next.time_.toSeconds();

      if (next.time_.time_ != std::numeric_limits<int64>::max() &&
          s0 < s1)
      {
        // calculate the end time based in display time
        // of the next subtitle frame:
        double ds = std::min<double>(5.0, s1 - s0);

        prev.tEnd_ = prev.time_;
        prev.tEnd_ += ds;
      }
      else if (v1 - s0 > 5.0)
      {
        prev.tEnd_ = prev.time_;
        prev.tEnd_ += 5.0;
      }
    }
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::fixupEndTimes
  //
  void
  SubtitlesTrack::fixupEndTimes(double v1, const TSubsFrame & last)
  {
    if (active_.empty())
    {
      return;
    }

    std::list<TSubsFrame>::iterator i = active_.begin();
    TSubsFrame * prev = &(*i);
    ++i;

    for (; i != active_.end(); ++i)
    {
      TSubsFrame & next = *i;
      fixupEndTime(v1, *prev, next);
      prev = &next;
    }

    fixupEndTime(v1, *prev, last);
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::expungeOldSubs
  //
  void
  SubtitlesTrack::expungeOldSubs(double v0)
  {
    for (std::list<TSubsFrame>::iterator i = active_.begin();
         i != active_.end(); )
    {
      const TSubsFrame & sf = *i;
      double s1 = sf.tEnd_.toSeconds();

      if (s1 <= v0)
      {
        i = active_.erase(i);
      }
      else
      {
        ++i;
      }
    }
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::get
  //
  void
  SubtitlesTrack::get(double v0, double v1, std::list<TSubsFrame> & subs)
  {
    for (std::list<TSubsFrame>::const_iterator i = active_.begin();
         i != active_.end(); ++i)
    {
      const TSubsFrame & sf = *i;
      double s0 = sf.time_.toSeconds();
      double s1 = sf.tEnd_.toSeconds();

      if (s0 < v1 && v0 < s1)
      {
#if 0
        std::cerr
          << "FIXME: subs(" << s0 << ", " << s1 << ") = " << s1 - s0
          << " overlaps frame (" << v0 << ", " << v1 << ")"
          << std::endl;
#endif
        subs.push_back(sf);
      }
    }
  }
}
