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
  // parse_ass_event_format
  //
  std::vector<std::string>
  parse_ass_event_format(const char * event_format, bool drop_timing)
  {
    std::string key;
    std::vector<std::string> fields;

    for (const char * i = event_format; i && *i; ++i)
    {
      char c = *i;
      if (c == ' ')
      {
        continue;
      }

      if (c == ',')
      {
        // finish current token:
        if (!drop_timing || (key != "Start" && key != "End"))
        {
          fields.push_back(key);
        }

        key = std::string();
      }
      else
      {
        key.push_back(c);
      }
    }

    if (!key.empty())
    {
      if (!drop_timing || (key != "Start" && key != "End"))
      {
        fields.push_back(key);
      }

      key = std::string();
    }

    return fields;
  }

  //----------------------------------------------------------------
  // parse_ass_event
  //
  void
  parse_ass_event(const char * event,
                  const std::vector<std::string> & event_format,
                  std::map<std::string, std::string> & fields)
  {
    std::size_t max_tokens = event_format.size();
    std::size_t num_tokens = 0;
    std::string token;
    const char * i = event;
    for (; i && *i && num_tokens + 1 < max_tokens; ++i)
    {
      char c = *i;
      if (c == ',')
      {
        // finish current token:
        const std::string & key = event_format.at(num_tokens);
        fields[key] = token;
        token = std::string();
        num_tokens++;
        continue;
      }

      token.push_back(c);
    }

    YAE_ASSERT(token.empty());
    const std::string & key = event_format.at(num_tokens);
    fields[key] = std::string(i);
  }

  //----------------------------------------------------------------
  // skip_whitespace
  //
  static const char *
  skip_whitespace(const char * str)
  {
    const char * s = str;
    for (; s && ::isspace(*s); ++s)
    {}
    return s;
  }

  //----------------------------------------------------------------
  // skip_to_endl
  //
  static const char *
  skip_to_endl(const char * str)
  {
    const char * s = str;
    for (; s && *s && *s != '\r' && *s != '\n'; ++s)
    {}
    return s;
  }

  //----------------------------------------------------------------
  // find_ass_events_format
  //
  bool
  find_ass_events_format(const char * subtitle_header,
                         std::string & event_format)
  {
    const char * start = strstr(subtitle_header, "[Events]");
    if (!start)
    {
      return false;
    }

    start += 8;
    start = skip_whitespace(start);
    start = strstr(start, "Format:");
    if (!start)
    {
      return false;
    }

    start += 7;
    start = skip_whitespace(start);

    const char * end = skip_to_endl(start);
    event_format = std::string(start, end);

    return true;
  }


  //----------------------------------------------------------------
  // TSubsPrivate::~TSubsPrivate
  //
  TSubsPrivate::~TSubsPrivate()
  {
    avsubtitle_free(&sub_);
  }

  //----------------------------------------------------------------
  // TSubsPrivate::TSubsPrivate
  //
  TSubsPrivate::TSubsPrivate(const AVSubtitle & sub,
                             const unsigned char * subsHeader,
                             std::size_t subsHeaderSize):
    sub_(sub),
    header_(subsHeader, subsHeader + subsHeaderSize)
  {}

  //----------------------------------------------------------------
  // TSubsPrivate::destroy
  //
  void
  TSubsPrivate::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // TSubsPrivate::headerSize
  //
  std::size_t
  TSubsPrivate::headerSize() const
  {
    return header_.size();
  }

  //----------------------------------------------------------------
  // TSubsPrivate::header
  //
  const unsigned char *
  TSubsPrivate::header() const
  {
    return header_.empty() ? NULL : &header_[0];
  }

  //----------------------------------------------------------------
  // TSubsPrivate::numRects
  //
  unsigned int
  TSubsPrivate::numRects() const
  {
    return sub_.num_rects;
  }

  //----------------------------------------------------------------
  // TSubsPrivate::getRect
  //
  void
  TSubsPrivate::getRect(unsigned int i, TSubsFrame::TRect & rect) const
  {
    if (i >= sub_.num_rects)
    {
      YAE_ASSERT(false);
      return;
    }

    const AVSubtitleRect * r = sub_.rects[i];
    rect.type_ = TSubsPrivate::getType(r);
    rect.x_ = r->x;
    rect.y_ = r->y;
    rect.w_ = r->w;
    rect.h_ = r->h;
    rect.numColors_ = r->nb_colors;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 5, 0)
#define get_rect(field) (r->field)
#else
#define get_rect(field) (r->pict.field)
#endif

    std::size_t nsrc = sizeof(get_rect(data)) / sizeof(get_rect(data)[0]);
    std::size_t ndst = sizeof(rect.data_) / sizeof(rect.data_[0]);
    YAE_ASSERT(nsrc == ndst);

    for (std::size_t j = 0; j < ndst && j < nsrc; j++)
    {
      rect.data_[j] = get_rect(data)[j];
      rect.rowBytes_[j] = get_rect(linesize)[j];
    }

#undef get_rect

    for (std::size_t j = nsrc; j < ndst; j++)
    {
      rect.data_[j] = NULL;
      rect.rowBytes_[j] = 0;
    }

    rect.text_ = r->text;
    rect.assa_ = r->ass;
  }

  //----------------------------------------------------------------
  // TSubsPrivate::getType
  //
  TSubtitleType
  TSubsPrivate::getType(const AVSubtitleRect * r)
  {
    switch (r->type)
    {
      case SUBTITLE_BITMAP:
        return kSubtitleBitmap;

      case SUBTITLE_TEXT:
        return kSubtitleText;

      case SUBTITLE_ASS:
        return kSubtitleASS;

      default:
        break;
    }

    return kSubtitleNone;
  }


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

          w_ = to_scalar<int, std::string>(std::string(strw, strh - 1));
          h_ = to_scalar<int, std::string>(std::string(strh, lEnd));
        }
      }
      else if ((found = strstr(line, "org:")) && found < lEnd)
      {
        const char * strx = found + 4;
        const char * stry = strstr(strx, ",");
        if (stry)
        {
          stry++;

          x_ = to_scalar<int, std::string>(std::string(strx, stry - 1));
          y_ = to_scalar<int, std::string>(std::string(stry, lEnd));
        }
      }
      else if ((found = strstr(line, "scale:")) && found < lEnd)
      {
        const char * strx = found + 6;
        const char * stry = strstr(strx, ",");
        if (stry)
        {
          stry++;

          int x = to_scalar<int, std::string>(std::string(strx, stry - 2));
          int y = to_scalar<int, std::string>(std::string(stry, lEnd - 1));

          scalex_ = double(x) / 100.0;
          scaley_ = double(y) / 100.0;
        }
      }
      else if ((found = strstr(line, "alpha:")) && found < lEnd)
      {
        const char * str = found + 6;
        int x = to_scalar<int, std::string>(std::string(str, lEnd - 1));
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
  SubtitlesTrack::SubtitlesTrack(AVStream * stream):
    Track(NULL, stream),
    render_(false),
    format_(kSubsNone)
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
  AVCodecContext *
  SubtitlesTrack::open()
  {
    if (codecContext_)
    {
      return codecContext_.get();
    }

    AVCodecContext * ctx = NULL;
    if (stream_)
    {
      ctx = Track::open();

      const AVCodecParameters & codecParams = *(stream_->codecpar);
      format_ = getSubsFormat(codecParams.codec_id);

      if (ctx && ctx->subtitle_header && ctx->subtitle_header_size)
      {
        std::string header((const char *)(ctx->subtitle_header),
                           (const char *)(ctx->subtitle_header +
                                          ctx->subtitle_header_size));
        std::string eventFormat;
        if (find_ass_events_format(header.c_str(), eventFormat))
        {
          setOutputEventFormat(eventFormat.c_str());
        }
      }

      if (ctx)
      {
        TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                &IPlanarBuffer::deallocator);
        buffer->resize(0, ctx->extradata_size, 1, 1);

        unsigned char * dst = buffer->data(0);
        memcpy(dst,
               ctx->extradata,
               ctx->extradata_size);

        extraData_ = buffer;

        if (format_ == kSubsDVD)
        {
          vobsub_.init(ctx->extradata,
                       ctx->extradata_size);
        }
      }

      active_.clear();
    }

    queue_.open();
    return ctx;
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::close
  //
  void
  SubtitlesTrack::close()
  {
    clear();
    Track::close();
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::setInputEventFormat
  //
  void
  SubtitlesTrack::setInputEventFormat(const char * eventFormat)
  {
    inputEventFormat_ = parse_ass_event_format(eventFormat, true);
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::setOutputEventFormat
  //
  void
  SubtitlesTrack::setOutputEventFormat(const char * eventFormat)
  {
    outputEventFormat_ = parse_ass_event_format(eventFormat);
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::addTimingEtc
  //
  // add Dialogue:, add h:mm:ss.cc Start and End time,
  // fix Event fields order to match AVCodecContext.subtitle_header
  //
  void
  SubtitlesTrack::addTimingEtc(TSubsFrame & sf)
  {
    TSubsPrivatePtr sf_private =
      boost::dynamic_pointer_cast<TSubsPrivate, TSubsFrame::IPrivate>
      (sf.private_);

    if (!sf_private)
    {
      return;
    }

    AVSubtitle & sub = sf_private->sub_;
    for (unsigned int i = 0; i < sub.num_rects; i++)
    {
      AVSubtitleRect & r = *(sub.rects[i]);
      if (r.type != SUBTITLE_ASS || !r.ass)
      {
        continue;
      }

      std::map<std::string, std::string> event;
      if (inputEventFormat_.empty())
      {
        parse_ass_event(r.ass, outputEventFormat_, event);
      }
      else
      {
        parse_ass_event(r.ass, inputEventFormat_, event);
      }

      // reformat:
      std::ostringstream oss;
      oss << "Dialogue: ";

      const char * separator = "";
      for (std::size_t j = 0, n = outputEventFormat_.size(); j < n; j++)
      {
        const std::string & key = outputEventFormat_[j];
        std::string value = yae::get(event, key);

        if (value.empty())
        {
          if (key == "Start")
          {
            value = sf.time_.to_hmmss_cc();
          }
          else if (key == "End")
          {
            value = sf.tEnd_.to_hmmss_cc();
          }
          else if (key == "Name")
          {
            value = yae::get(event, std::string("Actor"));
          }
          else if (key == "Actor")
          {
            value = yae::get(event, std::string("Name"));
          }
        }

        oss << separator << value;
        separator = ",";
      }

      av_freep(&r.ass);
      r.ass = av_strdup(oss.str().c_str());
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
      double s0 = prev.time_.sec();
      double s1 = next.time_.sec();

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
      double s1 = sf.tEnd_.sec();

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
      double s0 = sf.time_.sec();
      double s1 = sf.tEnd_.sec();

      if (s0 < v1 && v0 < s1)
      {
#if 0
        yae_debug
          << "FIXME: subs(" << s0 << ", " << s1 << ") = " << s1 - s0
          << " overlaps frame (" << v0 << ", " << v1 << ")";
#endif
        subs.push_back(sf);
      }
    }
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::push
  //
  void
  SubtitlesTrack::push(const TSubsFrame & sf, QueueWaitMgr * terminator)
  {
#if 0 // ndef NDEBUG
    std::ostringstream oss;
    oss
      << "SubtitlesTrack::push -- ["
      << sf.time_.to_hhmmss_frac(1000) << ", "
      << sf.tEnd_.to_hhmmss_frac(1000) << ")";

    if (sf.private_)
    {
      for (unsigned int i = 0, n = sf.private_->numRects(); i < n; i++)
      {
        TSubsFrame::TRect r;
        sf.private_->getRect(i, r);

        oss
          << ", r("<< i << ") = "
          << (r.assa_ ? r.getAssScript(sf).c_str() :
              r.text_ ? r.text_ : "BITMAP");
      }
    }

    yae_debug << oss.str();
#endif

    queue_.push(sf, terminator);
  }

}
