// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
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
  // TSubsPrivate::clone
  //
  boost::shared_ptr<TSubsFrame::IPrivate>
  TSubsPrivate::clone() const
  {
    TSubsPrivatePtr sp_ptr(new TSubsPrivate(*this),
                           &TSubsPrivate::deallocator);
    TSubsPrivate & sp = *sp_ptr;

    // copy AVSubtitle
    if (sub_.num_rects)
    {
      sp.sub_.rects = (AVSubtitleRect **)
        av_malloc_array(sub_.num_rects, sizeof(*(sub_.rects)));

      for (unsigned int i = 0; i < sub_.num_rects; i++)
      {
        sp.sub_.rects[i] = (AVSubtitleRect *)
          av_mallocz(sizeof(*(sub_.rects[0])));

        // shortcuts:
        const AVSubtitleRect & src = *(sub_.rects[i]);
        AVSubtitleRect & dst = *(sp.sub_.rects[i]);

        // shallow copy:
        dst = src;

        // deep copy:
        if (src.text)
        {
          dst.text = av_strdup(src.text);
        }

        if (src.ass)
        {
          dst.ass = av_strdup(src.ass);
        }

        // AVSubtitleRect.data is poorly documented:
        if (src.data[0])
        {
          YAE_ASSERT(src.linesize[0]);
          if (src.linesize[0])
          {
            std::size_t sz = src.linesize[0] * src.h;
            dst.data[0] = (uint8_t *)av_mallocz(sz);
            memcpy(dst.data[0], src.data[0], sz);
          }
          else
          {
            dst.data[0] =  NULL;
          }
        }

        if (src.data[1])
        {
          YAE_ASSERT(!src.linesize[1]);
          YAE_ASSERT(src.nb_colors);
          if (src.nb_colors < AVPALETTE_SIZE)
          {
            dst.data[1] = (uint8_t *)av_mallocz(AVPALETTE_SIZE);
            memcpy(dst.data[1], src.data[1], AVPALETTE_SIZE);
          }
          else
          {
            dst.data[1] =  NULL;
          }
        }

        YAE_ASSERT(!src.data[2]);
        YAE_ASSERT(!src.data[3]);
        dst.data[2] = NULL;
        dst.data[3] = NULL;
      }
    }

    return sp_ptr;
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
      case AV_CODEC_ID_ASS:
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
    Track(NULL, stream, false),
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
  AvCodecContextPtr
  SubtitlesTrack::open()
  {
    AvCodecContextPtr ctx_ptr = codecContext_;
    if (ctx_ptr)
    {
      return ctx_ptr;
    }

    if (stream_)
    {
      ctx_ptr = Track::open();
      AVCodecContext * ctx = ctx_ptr.get();

      const AVCodecParameters & codecParams = *(stream_->codecpar);
      format_ = getSubsFormat(codecParams.codec_id);

      if (ctx && ctx->subtitle_header && ctx->subtitle_header_size)
      {
        std::string header((const char *)(ctx->subtitle_header),
                           (const char *)(ctx->subtitle_header +
                                          ctx->subtitle_header_size));

        if (!strstr(header.c_str(), "[Events]"))
        {
          std::vector<std::string> lines;
          yae::split(lines, "\n",
                     (const char *)(ctx->subtitle_header),
                     (const char *)(ctx->subtitle_header +
                                    ctx->subtitle_header_size));
          for (std::vector<std::string>::iterator
                 i = lines.begin(); i != lines.end(); ++i)
          {
            std::string & line = *i;
            yae::strip_tail_ws(line);
          }

          const char * events_format =
            "Format: Layer, Start, End, Style, Name, "
            "MarginL, MarginR, MarginV, Effect, Text";

          lines.push_back(std::string(""));
          lines.push_back(std::string("[Events]"));
          lines.push_back(std::string(events_format));
          lines.push_back(std::string(""));

          header = yae::join("\n", lines);

          // store the modified subtitle header as extradata for libass:
          TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                  &IPlanarBuffer::deallocator);
          buffer->resize(0, header.size(), 1, 1);
          unsigned char * dst = buffer->data(0);
          memcpy(dst, &header[0], header.size());
          extraData_ = buffer;
        }

        std::string eventFormat;
        if (find_ass_events_format(header.c_str(), eventFormat))
        {
          if (inputEventFormat_.empty())
          {
            // ass.c claims to output [Events] with
            // Format: Layer, Start, End, Style, Name, \
            //         MarginL, MarginR, MarginV, Effect, Text
            //
            // but it actually outputs this:
            // Format: ReadOrder, Layer, Style, Name, \
            //         MarginL, MarginR, MarginV, Effect, Text);
            //
            // Start and End are expected to be omitted since
            // FF_SUB_TEXT_FMT_ASS_WITH_TIMINGS was removed,
            // but ReadOrder should really be mentioned
            // in AVCodecContext.subtitle_header
            //
            std::string inputFormat = eventFormat;
            if (!al::starts_with(inputFormat, "ReadOrder"))
            {
              inputFormat = "ReadOrder, " + eventFormat;
            }

            setInputEventFormat(inputFormat.c_str());
          }

          setOutputEventFormat(eventFormat.c_str());
        }
      }

      if (ctx && !extraData_)
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
    return ctx_ptr;
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
  // SubtitlesTrack::setTimingEtc
  //
  // add Dialogue:, add h:mm:ss.cc Start and End time,
  // fix Event fields order to match AVCodecContext.subtitle_header
  //
  void
  SubtitlesTrack::setTimingEtc(TSubsFrame & sf,
                               const std::vector<std::string> & eventFormat)
  {
    TSubsPrivatePtr sf_private =
      boost::dynamic_pointer_cast<TSubsPrivate, TSubsFrame::IPrivate>
      (sf.private_);

    if (!sf_private)
    {
      return;
    }

    if (eventFormat.empty())
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

      const char * dialogue = skip_whitespace(r.ass);
      if (al::starts_with(dialogue, "Dialogue:"))
      {
        dialogue += 9;
        dialogue = skip_whitespace(dialogue);
      }

      std::map<std::string, std::string> event;
      parse_ass_event(dialogue, eventFormat, event);

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
  // to_str
  //
  std::string
  to_str(const TSubsFrame & sf)
  {
    std::ostringstream oss;
    oss
      << "["
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

    return oss.str();
  }

  //----------------------------------------------------------------
  // SubtitlesTrack::push
  //
  void
  SubtitlesTrack::push(const TSubsFrame & sf, QueueWaitMgr * terminator)
  {
#if 0 // ndef NDEBUG
    yae_debug << "SubtitlesTrack::push " << to_str(sf);
#endif

    queue_.push(sf, terminator);
  }

}
