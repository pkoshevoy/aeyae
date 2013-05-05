// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

//----------------------------------------------------------------
// __STDC_CONSTANT_MACROS
//
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

// system includes:
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <set>

// boost includes:
#include <boost/thread.hpp>

// yae includes:
#include <yaeAPI.h>
#include <yaeQueue.h>
#include <yaeReader.h>
#include <yaeReaderFFMPEG.h>
#include <yaeThreading.h>
#include <yaeUtils.h>
#include <yaePixelFormatFFMPEG.h>
#include <yaePixelFormatTraits.h>
#include <yaeAudioFragment.h>
#include <yaeAudioTempoFilter.h>

// ffmpeg includes:
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


//----------------------------------------------------------------
// YAE_ASSERT_NO_AVERROR_OR_RETURN
//
#define YAE_ASSERT_NO_AVERROR_OR_RETURN(err, ret)       \
  do {                                                  \
    if (err < 0)                                        \
    {                                                   \
      char tmp[1024];                                   \
      av_strerror(err, tmp, sizeof(tmp));               \
      std::cerr << "AVERROR: " << tmp << std::endl;     \
      YAE_ASSERT(false);                                \
      return ret;                                       \
    }                                                   \
  } while (0)


namespace yae
{

  //----------------------------------------------------------------
  // kMaxDouble
  //
  static const double kMaxDouble = std::numeric_limits<double>::max();

  //----------------------------------------------------------------
  // kMaxInt64
  //
  static const int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();

  //----------------------------------------------------------------
  // kMinInt64
  //
  static const int64_t kMinInt64 = std::numeric_limits<int64_t>::min();

  //----------------------------------------------------------------
  // kQueueSizeSmall
  //
  enum
  {
#if 1
    kQueueSizeSmall = 30,
    kQueueSizeMedium = 50,
    kQueueSizeLarge = 120
#else
    kQueueSizeSmall = 1,
    kQueueSizeMedium = 1,
    kQueueSizeLarge = 1
#endif
  };

  //----------------------------------------------------------------
  // Packet
  //
  struct Packet
  {
    Packet()
    {
      memset(&ffmpeg_, 0, sizeof(AVPacket));
    }

    ~Packet()
    {
      av_free_packet(&ffmpeg_);
    }

    bool set(const AVPacket & packet)
    {
      ffmpeg_ = packet;

      int err = av_dup_packet(&ffmpeg_);
      if (err)
      {
        memset(&ffmpeg_, 0, sizeof(AVPacket));
      }

      return !err;
    }

    // raw ffmpeg packet:
    AVPacket ffmpeg_;

  private:
    // intentionally disabled:
    Packet(const Packet &);
    Packet & operator = (const Packet &);
  };

  //----------------------------------------------------------------
  // TPacketPtr
  //
  typedef boost::shared_ptr<Packet> TPacketPtr;

  //----------------------------------------------------------------
  // TPacketQueue
  //
  typedef Queue<TPacketPtr> TPacketQueue;

  //----------------------------------------------------------------
  // TVideoFrameQueue
  //
  typedef Queue<TVideoFramePtr> TVideoFrameQueue;

  //----------------------------------------------------------------
  // TAudioFrameQueue
  //
  typedef Queue<TAudioFramePtr> TAudioFrameQueue;


  //----------------------------------------------------------------
  // TSubsPrivate
  //
  class TSubsPrivate : public TSubsFrame::IPrivate
  {
    // virtual:
    ~TSubsPrivate()
    {
      avsubtitle_free(&sub_);
    }

  public:
    TSubsPrivate(const AVSubtitle & sub,
                 const unsigned char * subsHeader,
                 std::size_t subsHeaderSize):
      sub_(sub),
      header_(subsHeader, subsHeader + subsHeaderSize)
    {}

    // virtual:
    void destroy()
    { delete this; }

    // virtual:
    std::size_t headerSize() const
    { return header_.size(); }

    // virtual:
    const unsigned char * header() const
    { return header_.empty() ? NULL : &header_[0]; }

    // virtual:
    unsigned int numRects() const
    { return sub_.num_rects; }

    // virtual:
    void getRect(unsigned int i, TSubsFrame::TRect & rect) const
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
      memcpy(rect.data_, r->pict.data, sizeof(r->pict.data));
      memcpy(rect.rowBytes_, r->pict.linesize, sizeof(r->pict.linesize));
      rect.text_ = r->text;
      rect.assa_ = r->ass;
    }

    // helper:
    static TSubtitleType getType(const AVSubtitleRect * r)
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

    AVSubtitle sub_;
    std::vector<unsigned char> header_;
  };

  //----------------------------------------------------------------
  // TSubsPrivatePtr
  //
  typedef boost::shared_ptr<TSubsPrivate> TSubsPrivatePtr;

  //----------------------------------------------------------------
  // TSubsFrameQueue
  //
  typedef Queue<TSubsFrame> TSubsFrameQueue;

  //----------------------------------------------------------------
  // PacketTime
  //
  struct PacketTime
  {
    PacketTime(int64_t pts = AV_NOPTS_VALUE,
               int64_t dts = AV_NOPTS_VALUE,
               int64_t duration = AV_NOPTS_VALUE):
      pts_(pts),
      dts_(dts),
      duration_(duration)
    {}

    int64_t pts_;
    int64_t dts_;
    int64_t duration_;
  };

  //----------------------------------------------------------------
  // lockManager
  //
  static int
  lockManager(void ** context, enum AVLockOp op)
  {
    try
    {
      switch (op)
      {
        case AV_LOCK_CREATE:
        {
          *context = new boost::mutex();
        }
        break;

        case AV_LOCK_OBTAIN:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->lock();
        }
        break;

        case AV_LOCK_RELEASE:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          mtx->unlock();
        }
        break;

        case AV_LOCK_DESTROY:
        {
          boost::mutex * mtx = (boost::mutex *)(*context);
          delete mtx;
        }
        break;

        default:
          YAE_ASSERT(false);
          return -1;
      }

      return 0;
    }
    catch (...)
    {}

    return -1;
  }

  //----------------------------------------------------------------
  // getTrackLang
  //
  static const char *
  getTrackLang(AVDictionary * metadata)
  {
    AVDictionaryEntry * lang = av_dict_get(metadata,
                                            "language",
                                            NULL,
                                            0);

    if (lang)
    {
      return lang->value;
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // getTrackName
  //
  static const char *
  getTrackName(AVDictionary * metadata)
  {
    AVDictionaryEntry * name = av_dict_get(metadata,
                                           "name",
                                           NULL,
                                           0);
    if (name)
    {
      return name->value;
    }

    AVDictionaryEntry * title = av_dict_get(metadata,
                                            "title",
                                            NULL,
                                            0);
    if (title)
    {
      return title->value;
    }

    return NULL;
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

      default:
        break;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // TVobSubSpecs
  //
  struct TVobSubSpecs
  {
    TVobSubSpecs():
      x_(0),
      y_(0),
      w_(0),
      h_(0),
      scalex_(1.0),
      scaley_(1.0),
      alpha_(1.0)
    {}

    void init(const unsigned char * extraData, std::size_t size)
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

    // reference frame origin and dimensions:
    int x_;
    int y_;
    int w_;
    int h_;

    double scalex_;
    double scaley_;
    double alpha_;

    // color palette:
    std::vector<std::string> palette_;
  };

  //----------------------------------------------------------------
  // SubtitlesTrack
  //
  struct SubtitlesTrack
  {
    SubtitlesTrack(AVStream * stream = NULL, std::size_t index = 0):
      stream_(stream),
      codec_(NULL),
      render_(false),
      format_(kSubsNone),
      index_(index),
      queue_(kQueueSizeLarge)
    {
      open();
    }

    ~SubtitlesTrack()
    {
      close();
    }

    void clear()
    {
      queue_.clear();
      active_.clear();
    }

    void open()
    {
      if (stream_)
      {
        format_ = getSubsFormat(stream_->codec->codec_id);
        codec_ = avcodec_find_decoder(stream_->codec->codec_id);
        active_.clear();

        if (codec_)
        {
          av_opt_set(stream_->codec, "threads", "auto", 0);
          av_opt_set_int(stream_->codec, "refcounted_frames", 1, 0);
          int err = avcodec_open2(stream_->codec, codec_, NULL);
          if (err < 0)
          {
            // unsupported codec:
            codec_ = NULL;
          }
          else if (stream_->codec->extradata &&
                   stream_->codec->extradata_size)
          {
            TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                    &IPlanarBuffer::deallocator);
            buffer->resize(0, stream_->codec->extradata_size, 1, 1);

            unsigned char * dst = buffer->data(0);
            memcpy(dst,
                   stream_->codec->extradata,
                   stream_->codec->extradata_size);

            extraData_ = buffer;

            if (format_ == kSubsDVD)
            {
              vobsub_.init(stream_->codec->extradata,
                           stream_->codec->extradata_size);
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

    void close()
    {
      clear();

      if (stream_ && codec_)
      {
        avcodec_close(stream_->codec);
        codec_ = NULL;
      }
    }

    void fixupEndTime(double v1, TSubsFrame & prev, const TSubsFrame & next)
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

    void fixupEndTimes(double v1, const TSubsFrame & last)
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

    void expungeOldSubs(double v0)
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

    void get(double v0, double v1, std::list<TSubsFrame> & subs)
    {
      for (std::list<TSubsFrame>::const_iterator i = active_.begin();
           i != active_.end(); ++i)
      {
        const TSubsFrame & sf = *i;
        double s0 = sf.time_.toSeconds();
        double s1 = sf.tEnd_.toSeconds();

        if (s0 < v1 && v0 < s1)
        {
          subs.push_back(sf);
        }
      }
    }

  private:
    SubtitlesTrack(const SubtitlesTrack & given);
    SubtitlesTrack & operator = (const SubtitlesTrack & given);

  public:
    AVStream * stream_;
    AVCodec * codec_;

    bool render_;
    TSubsFormat format_;
    std::string name_;
    std::string lang_;
    std::size_t index_;

    TIPlanarBufferPtr extraData_;
    TSubsFrameQueue queue_;
    std::list<TSubsFrame> active_;

    TVobSubSpecs vobsub_;
  };

  //----------------------------------------------------------------
  // TSubsTrackPtr
  //
  typedef boost::shared_ptr<SubtitlesTrack> TSubsTrackPtr;

  //----------------------------------------------------------------
  // Track
  //
  struct Track
  {
    // NOTE: constructor does not open the stream:
    Track(AVFormatContext * context, AVStream * stream);

    // NOTE: destructor will close the stream:
    virtual ~Track();

    // open the stream for decoding:
    bool open();

    // close the stream:
    virtual void close();

    // get track name:
    const char * getName() const;
    const char * getLang() const;

    // accessor to stream index of this track within AVFormatContext:
    inline int streamIndex() const
    { return stream_->index; }

    // accessor to the codec context:
    inline AVCodecContext * codecContext() const
    { return stream_->codec; }

    // accessor to the codec:
    inline AVCodec * codec() const
    { return codec_; }

    // get track duration:
    void getDuration(TTime & start, TTime & duration) const;

    // accessor to the packet queue:
    inline TPacketQueue & packetQueue()
    { return packetQueue_; }

    // decode a given packet:
    virtual bool decoderStartup()
    { return false; }

    virtual bool decoderShutdown()
    { return false; }

    virtual bool decode(const TPacketPtr & packetPtr)
    { return false; }

    // packet decoding thread:
    virtual void threadLoop() {}
    virtual bool threadStart();
    virtual bool threadStop();

    // adjust frame duration:
    virtual bool setTempo(double tempo);

  private:
    // intentionally disabled:
    Track(const Track &);
    Track & operator = (const Track &);

  protected:
    // worker thread:
    Thread<Track> thread_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;
    AVStream * stream_;
    AVCodec * codec_;
    TPacketQueue packetQueue_;

    double timeIn_;
    double timeOut_;
    bool playbackInterval_;
    int64_t startTime_;

    // for adjusting frame duration (playback tempo scaling):
    mutable boost::mutex tempoMutex_;
    double tempo_;

  public:
    uint64_t discarded_;
  };

  //----------------------------------------------------------------
  // TrackPtr
  //
  typedef boost::shared_ptr<Track> TrackPtr;

  //----------------------------------------------------------------
  // Track::Track
  //
  Track::Track(AVFormatContext * context, AVStream * stream):
    thread_(this),
    context_(context),
    stream_(stream),
    codec_(NULL),
    packetQueue_(kQueueSizeLarge),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackInterval_(false),
    startTime_(0),
    tempo_(1.0),
    discarded_(0)
  {
    if (context_ && stream_)
    {
      YAE_ASSERT(context_->streams[stream_->index] == stream_);
    }
  }

  //----------------------------------------------------------------
  // Track::~Track
  //
  Track::~Track()
  {
    threadStop();
    close();
  }

  //----------------------------------------------------------------
  // Track::open
  //
  bool
  Track::open()
  {
    if (!stream_)
    {
      return false;
    }

    threadStop();
    close();

    codec_ = avcodec_find_decoder(stream_->codec->codec_id);
    if (!codec_ && stream_->codec->codec_id != CODEC_ID_TEXT)
    {
      // unsupported codec:
      return false;
    }

    int err = 0;
    if (codec_)
    {
      av_opt_set(stream_->codec, "threads", "auto", 0);
      av_opt_set_int(stream_->codec, "refcounted_frames", 1, 0);
      err = avcodec_open2(stream_->codec, codec_, NULL);
    }

    if (err < 0)
    {
      // unsupported codec:
      codec_ = NULL;
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Track::close
  //
  void
  Track::close()
  {
    if (stream_ && codec_)
    {
      avcodec_close(stream_->codec);
      codec_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // Track::getName
  //
  const char *
  Track::getName() const
  {
    return stream_ ? getTrackName(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getLang
  //
  const char *
  Track::getLang() const
  {
    return stream_ ? getTrackLang(stream_->metadata) : NULL;
  }

  //----------------------------------------------------------------
  // Track::getDuration
  //
  void
  Track::getDuration(TTime & start, TTime & duration) const
  {
    if (!stream_)
    {
      YAE_ASSERT(false);
      return;
    }

    if (stream_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // return track duration:
      start.base_ = stream_->time_base.den;
      start.time_ =
        stream_->start_time != int64_t(AV_NOPTS_VALUE) ?
        stream_->time_base.num * stream_->start_time : 0;

      duration.time_ = stream_->time_base.num * stream_->duration;
      duration.base_ = stream_->time_base.den;
      return;
    }

    if (!context_)
    {
      YAE_ASSERT(false);
      return;
    }

    if (context_->duration != int64_t(AV_NOPTS_VALUE))
    {
      // track duration is unknown, return movie duration instead:
      start.base_ = AV_TIME_BASE;
      start.time_ =
        context_->start_time != int64_t(AV_NOPTS_VALUE) ?
        context_->start_time : 0;

      duration.time_ = context_->duration;
      duration.base_ = AV_TIME_BASE;
      return;
    }

    int64 fileSize = avio_size(context_->pb);
    int64 fileBits = fileSize * 8;

    start.base_ = AV_TIME_BASE;
    start.time_ = 0;

    if (context_->bit_rate)
    {
      double t =
        double(fileBits / context_->bit_rate) +
        double(fileBits % context_->bit_rate) /
        double(context_->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return;
    }

    if (context_->nb_streams == 1 && stream_->codec->bit_rate)
    {
      double t =
        double(fileBits / stream_->codec->bit_rate) +
        double(fileBits % stream_->codec->bit_rate) /
        double(stream_->codec->bit_rate);

      duration.time_ = int64_t(0.5 + t * double(AV_TIME_BASE));
      duration.base_ = AV_TIME_BASE;
      return;
    }

    // unknown duration:
    duration.time_ = std::numeric_limits<int64>::max();
    duration.base_ = AV_TIME_BASE;
  }

  //----------------------------------------------------------------
  // Track::threadStart
  //
  bool
  Track::threadStart()
  {
    terminator_.stopWaiting(false);
    packetQueue_.open();
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Track::threadStop
  //
  bool
  Track::threadStop()
  {
    terminator_.stopWaiting(true);
    packetQueue_.close();
    thread_.stop();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Track::setTempo
  //
  bool
  Track::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);
    tempo_ = tempo;
    return true;
  }


  //----------------------------------------------------------------
  // VideoFilterGraph
  //
  struct VideoFilterGraph
  {
    VideoFilterGraph();
    ~VideoFilterGraph();

    void reset();

    bool setup(int srcWidth,
               int srcHeight,
               const AVRational & srcTimeBase,
               const AVRational & srcPAR,
               PixelFormat srcPixFmt,
               PixelFormat dstPixFmt,
               const char * filterChain = NULL,
               bool * frameTraitsChanged = NULL);

    bool push(AVFrame * in);
    bool pull(AVFrame * out);

  protected:
    std::string filterChain_;
    int srcWidth_;
    int srcHeight_;
    AVRational srcTimeBase_;
    AVRational srcPAR_;
    PixelFormat srcPixFmt_;
    PixelFormat dstPixFmt_[2];

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };

  //----------------------------------------------------------------
  // VideoFilterGraph::VideoFilterGraph
  //
  VideoFilterGraph::VideoFilterGraph():
    src_(NULL),
    sink_(NULL),
    in_(NULL),
    out_(NULL),
    graph_(NULL)
  {
    reset();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::~VideoFilterGraph
  //
  VideoFilterGraph::~VideoFilterGraph()
  {
    reset();
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::reset
  //
  void
  VideoFilterGraph::reset()
  {
    avfilter_graph_free(&graph_);
    avfilter_inout_free(&in_);
    avfilter_inout_free(&out_);

    srcTimeBase_.num = 0;
    srcTimeBase_.den = 1;

    srcPAR_.num = 0;
    srcPAR_.den = 1;

    srcWidth_  = 0;
    srcHeight_ = 0;

    srcPixFmt_    = PIX_FMT_NONE;
    dstPixFmt_[0] = PIX_FMT_NONE;
    dstPixFmt_[1] = PIX_FMT_NONE;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::setup
  //
  bool
  VideoFilterGraph::setup(int srcWidth,
                          int srcHeight,
                          const AVRational & srcTimeBase,
                          const AVRational & srcPAR,
                          PixelFormat srcPixFmt,
                          PixelFormat dstPixFmt,
                          const char * filterChain,
                          bool * frameTraitsChanged)
  {
    filterChain = filterChain ? filterChain : "";
    bool sameTraits = (srcWidth_ == srcWidth &&
                       srcHeight_ == srcHeight &&
                       srcPixFmt_ == srcPixFmt &&
                       dstPixFmt_[0] == dstPixFmt &&
                       filterChain_ == filterChain);

    if (frameTraitsChanged)
    {
      *frameTraitsChanged = !sameTraits;
    }

    if (sameTraits)
    {
      return true;
    }

    reset();

    srcWidth_ = srcWidth;
    srcHeight_ = srcHeight;
    srcPixFmt_ = srcPixFmt;
    dstPixFmt_[0] = dstPixFmt;

    srcTimeBase_ = srcTimeBase;
    srcPAR_ = srcPAR;

    AVFilter * srcFilterDef = avfilter_get_by_name("buffer");
    AVFilter * dstFilterDef = avfilter_get_by_name("buffersink");

    graph_ = avfilter_graph_alloc();

    std::string srcCfg;
    {
      std::ostringstream os;

      const char * txtPixFmt = av_get_pix_fmt_name(srcPixFmt_);
      os << "video_size=" << srcWidth_ << "x" << srcHeight_
         << ":pix_fmt=" << txtPixFmt
         << ":time_base=" << srcTimeBase_.num
         << "/" << srcTimeBase_.den
         << ":pixel_aspect=" << srcPAR_.num
         << "/" << srcPAR_.den;
      srcCfg = os.str().c_str();
    }

    int err = avfilter_graph_create_filter(&src_,
                                           srcFilterDef,
                                           "in",
                                           srcCfg.c_str(),
                                           NULL,
                                           graph_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    AVBufferSinkParams params;
    params.pixel_fmts = dstPixFmt_;
    err = avfilter_graph_create_filter(&sink_,
                                       dstFilterDef,
                                       "out",
                                       NULL,
                                       &params,
                                       graph_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    out_ = avfilter_inout_alloc();
    err = out_ ? 0 : AVERROR(ENOMEM);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    out_->name = av_strdup("in");
    out_->filter_ctx = src_;
    out_->pad_idx = 0;
    out_->next = NULL;

    in_ = avfilter_inout_alloc();
    err = in_ ? 0 : AVERROR(ENOMEM);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    in_->name = av_strdup("out");
    in_->filter_ctx = sink_;
    in_->pad_idx = 0;
    in_->next = NULL;

    std::string filters;
    {
      std::ostringstream os;
      if (*filterChain && strcmp(filterChain, "null") != 0)
      {
        os << filterChain << ",";
      }

      const char * txtPixFmt = av_get_pix_fmt_name(dstPixFmt_[0]);
      os << "format=" << txtPixFmt;

      filters = os.str().c_str();
      filterChain_ = filterChain;
    }

    err = avfilter_graph_parse(graph_, filters.c_str(), &in_, &out_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = avfilter_graph_config(graph_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    return true;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::push
  //
  bool
  VideoFilterGraph::push(AVFrame * frame)
  {
    int err = av_buffersrc_add_frame_flags(src_, frame, 0);

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }

  //----------------------------------------------------------------
  // VideoFilterGraph::pull
  //
  bool
  VideoFilterGraph::pull(AVFrame * frame)
  {
    int err = av_buffersink_get_frame(sink_, frame);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
    {
      return false;
    }

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }


  //----------------------------------------------------------------
  // AutoFree
  //
  template <typename TData>
  struct AutoFree
  {
    AutoFree(TData * data):
      data_(data)
    {}

    ~AutoFree()
    {
      av_free(data_);
    }

    inline TData * get() const
    {
      return data_;
    }

    inline operator TData * () const
    {
      return data_;
    }

    inline void reset(TData * data)
    {
      av_free(data_);
      data_ = data;
    }

  protected:
    TData * data_;

  private:
    // intentionally disabled:
    AutoFree(const AutoFree &);
    AutoFree & operator = (const AutoFree &);
  };

  //----------------------------------------------------------------
  // FrameWithAutoCleanup
  //
  struct FrameWithAutoCleanup : AutoFree<AVFrame>
  {
    typedef AutoFree<AVFrame> TBase;

    FrameWithAutoCleanup():
      TBase(avcodec_alloc_frame())
    {
      data_->opaque = NULL;
    }

    operator AVPicture * () const
    {
      return (AVPicture *)data_;
    }

    void reset()
    {
      TBase::reset(avcodec_alloc_frame());
      data_->opaque = NULL;
    }
  };

  //----------------------------------------------------------------
  // FrameAutoUnref
  //
  struct FrameAutoUnref
  {
    FrameAutoUnref(AVFrame * frame):
      frame_(frame)
    {}

    ~FrameAutoUnref()
    {
      av_frame_unref(frame_);
    }

    AVFrame * frame_;
  };

  //----------------------------------------------------------------
  // VideoTrack
  //
  struct VideoTrack : public Track
  {
    VideoTrack(AVFormatContext * context, AVStream * stream);

    // virtual:
    bool open();

    // these are used to speed up video decoding:
    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    // helpers: these are used to re-configure output buffers
    // when frame traits change:
    void refreshTraits();
    bool reconfigure();

    // virtual:
    bool decoderStartup();
    bool decoderShutdown();
    bool decode(const TPacketPtr & packetPtr);

    // virtual:
    void threadLoop();
    bool threadStop();

    // video traits, not overridden:
    bool getTraits(VideoTraits & traits) const;

    // use this for video frame conversion (pixel format and size)
    bool setTraitsOverride(const VideoTraits & override, bool deint);
    bool getTraitsOverride(VideoTraits & override) const;

    inline bool setTraitsOverride(const VideoTraits & override)
    { return setTraitsOverride(override, deinterlace_); }

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(double timeIn, double timeOut, bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(double seekTime);

    // adjust frame duration:
    bool setDeinterlacing(bool enabled);

    void setSubs(std::vector<TSubsTrackPtr> * subs)
    { subs_ = subs; }

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    bool deinterlace_;

    TVideoFrameQueue frameQueue_;
    VideoTraits override_;
    VideoTraits native_;
    VideoTraits output_;

    // output sample buffer properties;
    unsigned char numSamplePlanes_;
    std::size_t samplePlaneSize_[4];
    std::size_t sampleLineSize_[4];
    AVRational frameRate_;

    int64 ptsBestEffort_;
    TTime prevPTS_;
    bool hasPrevPTS_;

    uint64 framesDecoded_;

    FrameWithAutoCleanup frameAutoCleanup_;

    std::vector<TSubsTrackPtr> * subs_;

    VideoFilterGraph filterGraph_;
  };

  //----------------------------------------------------------------
  // VideoTrackPtr
  //
  typedef boost::shared_ptr<VideoTrack> VideoTrackPtr;

  //----------------------------------------------------------------
  // descendingSortOrder
  //
  static bool
  aFollowsB(const TVideoFramePtr & a,
            const TVideoFramePtr & b)
  {
    TTime framePosition;
    if (a->time_.base_ == b->time_.base_)
    {
      return a->time_.time_ > b->time_.time_;
    }

    double ta = double(a->time_.time_) / double(a->time_.base_);
    double tb = double(b->time_.time_) / double(b->time_.base_);
    return ta > tb;
  }

  //----------------------------------------------------------------
  // VideoTrack::VideoTrack
  //
  VideoTrack::VideoTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    deinterlace_(false),
    frameQueue_(kQueueSizeSmall),
    numSamplePlanes_(0),
    hasPrevPTS_(false),
    framesDecoded_(0),
    subs_(NULL)
  {
    YAE_ASSERT(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO);

    // make sure the frames are sorted from oldest to newest:
    frameQueue_.setSortFunc(&aFollowsB);
  }

  //----------------------------------------------------------------
  // VideoTrack::open
  //
  bool
  VideoTrack::open()
  {
    if (Track::open())
    {
      skipLoopFilter(skipLoopFilter_);
      skipNonReferenceFrames(skipNonReferenceFrames_);

      bool ok = getTraits(override_);
      framesDecoded_ = 0;

      return ok;
    }

    return false;
  }

  //----------------------------------------------------------------
  // VideoTrack::skipLoopFilter
  //
  void
  VideoTrack::skipLoopFilter(bool skip)
  {
    skipLoopFilter_ = skip;

    if (stream_->codec)
    {
      if (skipLoopFilter_)
      {
        stream_->codec->skip_loop_filter = AVDISCARD_ALL;
        stream_->codec->flags2 |= CODEC_FLAG2_FAST;
      }
      else
      {
        stream_->codec->skip_loop_filter = AVDISCARD_DEFAULT;
        stream_->codec->flags2 &= ~(CODEC_FLAG2_FAST);
      }
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::skipNonReferenceFrames
  //
  void
  VideoTrack::skipNonReferenceFrames(bool skip)
  {
    skipNonReferenceFrames_ = skip;

    if (stream_->codec)
    {
      if (skipNonReferenceFrames_)
      {
        stream_->codec->skip_frame = AVDISCARD_NONREF;
      }
      else
      {
        stream_->codec->skip_frame = AVDISCARD_DEFAULT;
      }
    }
  }

  //----------------------------------------------------------------
  // insertPacketTime
  //
  static void
  insertPacketTime(std::list<PacketTime> & packetTimes,
                   const PacketTime & t)
  {
    if (packetTimes.empty())
    {
      packetTimes.push_front(t);
      return;
    }

    for (std::list<PacketTime>::iterator i = packetTimes.begin();
         i != packetTimes.end(); ++i)
    {
      const PacketTime & ti = *i;
      if (t.pts_ > ti.pts_)
      {
        packetTimes.insert(i, t);
        return;
      }
    }

    packetTimes.push_back(t);
  }

  //----------------------------------------------------------------
  // verifyPTS
  //
  // verify that presentation timestamps are monotonically increasing
  //
  static inline bool
  verifyPTS(bool hasPrevPTS, const TTime & prevPTS, const TTime & nextPTS,
            const char * debugMessage = NULL)
  {
    bool ok = (!hasPrevPTS ||
               (prevPTS.base_ == nextPTS.base_ ?
                prevPTS.time_ < nextPTS.time_ :
                prevPTS.toSeconds() < nextPTS.toSeconds()));
#if 0
    if (ok && debugMessage)
    {
      std::cerr << "PTS OK: "
                << nextPTS.time_ << "/" << nextPTS.base_
                << ", " << debugMessage << std::endl;
    }
#else
    (void)debugMessage;
#endif
    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::refreshTraits
  //
  void
  VideoTrack::refreshTraits()
  {
    // shortcut to native frame format traits:
    getTraits(native_);

    // frame size may have changed, so update the override accordingly:
    override_.encodedWidth_ = native_.encodedWidth_;
    override_.encodedHeight_ = native_.encodedHeight_;
    override_.visibleWidth_ = native_.visibleWidth_;
    override_.visibleHeight_ = native_.visibleHeight_;

    output_ = override_;
    if (output_.pixelFormat_ == kPixelFormatY400A &&
        native_.pixelFormat_ != kPixelFormatY400A)
    {
      // sws_getContext doesn't support Y400A, so drop the alpha channel:
      output_.pixelFormat_ = kPixelFormatGRAY8;
    }
  }

  //----------------------------------------------------------------
  // VideoTrack::reconfigure
  //
  bool
  VideoTrack::reconfigure()
  {
    refreshTraits();

    // pixel format shortcut:
    const pixelFormat::Traits * ptts =
      pixelFormat::getTraits(output_.pixelFormat_);
    if (!ptts)
    {
      YAE_ASSERT(false);
      return false;
    }

    // get number of contiguous sample planes,
    // sample set stride (in bits) for each plane:
    unsigned char samplePlaneStride[4] = { 0 };
    numSamplePlanes_ = ptts->getPlanes(samplePlaneStride);

    if (output_.pixelFormat_ != native_.pixelFormat_)
    {
      const unsigned int kRowAlignment = 32;
      output_.encodedWidth_ =
        (override_.encodedWidth_ + (kRowAlignment - 1)) &
        ~(kRowAlignment - 1);
    }

    if (ptts->chromaBoxW_ > 1)
    {
      unsigned int remainder = output_.encodedWidth_ % ptts->chromaBoxW_;
      if (remainder)
      {
        output_.encodedWidth_ += ptts->chromaBoxW_ - remainder;
      }
    }

    if (ptts->chromaBoxH_ > 1)
    {
      unsigned int remainder = output_.encodedHeight_ % ptts->chromaBoxH_;
      if (remainder)
      {
        output_.encodedHeight_ += ptts->chromaBoxH_ - remainder;
      }
    }

    // calculate number of bytes for each sample plane:
    std::size_t totalPixels = output_.encodedWidth_ * output_.encodedHeight_;

    memset(samplePlaneSize_, 0, sizeof(samplePlaneSize_));
    samplePlaneSize_[0] =
      totalPixels * (samplePlaneStride[0] / ptts->samples_[0]) / 8;

    memset(sampleLineSize_, 0, sizeof(sampleLineSize_));
    sampleLineSize_[0] =
      output_.encodedWidth_ * (samplePlaneStride[0] / ptts->samples_[0]) / 8;

    for (unsigned char i = 1; i < numSamplePlanes_; i++)
    {
      samplePlaneSize_[i] = totalPixels * samplePlaneStride[i] / 8;
      sampleLineSize_[i] = output_.encodedWidth_ * samplePlaneStride[i] / 8;
    }

    // account for sub-sampling of UV plane(s):
    std::size_t chromaBoxArea = ptts->chromaBoxW_ * ptts->chromaBoxH_;
    if (chromaBoxArea > 1)
    {
      unsigned char uvSamplePlanes =
        (ptts->flags_ & pixelFormat::kAlpha) ?
        numSamplePlanes_ - 2 :
        numSamplePlanes_ - 1;

      for (unsigned char i = 1; i < 1 + uvSamplePlanes; i++)
      {
        samplePlaneSize_[i] /= chromaBoxArea;
        sampleLineSize_[i] /= ptts->chromaBoxW_;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderStartup
  //
  bool
  VideoTrack::decoderStartup()
  {
    refreshTraits();

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    // shortcut to the frame rate:
    frameRate_ =
      (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den) ?
      stream_->avg_frame_rate :
      stream_->r_frame_rate;

    frameAutoCleanup_.reset();
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    // framesDecoded_ = 0;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::decoderShutdown
  //
  bool
  VideoTrack::decoderShutdown()
  {
    filterGraph_.reset();
    frameAutoCleanup_.reset();
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    frameQueue_.close();
    return true;
  }

  //----------------------------------------------------------------
  // TSubsPredicate
  //
  struct TSubsPredicate
  {
    TSubsPredicate(double now):
      now_(now)
    {}

    bool operator() (const TSubsFrame & sf) const
    {
      double s0 = sf.time_.toSeconds();
      return s0 <= now_;
    }

    double now_;
  };

  //----------------------------------------------------------------
  // VideoTrack::decode
  //
  bool
  VideoTrack::decode(const TPacketPtr & packetPtr)
  {
    try
    {
      // make a local shallow copy of the packet:
      AVPacket packet = packetPtr->ffmpeg_;

      // Decode video frame
      int gotFrame = 0;
      AVFrame * avFrame = frameAutoCleanup_;
      avcodec_get_frame_defaults(avFrame);
      AVCodecContext * codecContext = this->codecContext();
      avcodec_decode_video2(codecContext,
                            avFrame,
                            &gotFrame,
                            &packet);
      if (!gotFrame)
      {
        return true;
      }

      avFrame->pts = av_frame_get_best_effort_timestamp(avFrame);
      framesDecoded_++;

      enum PixelFormat ffmpegPixelFormat = yae_to_ffmpeg(output_.pixelFormat_);
      const char * filterChain = NULL;
      if (deinterlace_)
      {
        // when non-reference frames are discarded deinterlacing filter
        // loses ability to detect interlaced frames, therefore
        // it is better to simply drop a field:
        filterChain = skipNonReferenceFrames_ ? "yadif=2:0:0" : "yadif=0:0:1";
      }

      bool frameTraitsChanged = false;
      if (!filterGraph_.setup(avFrame->width,
                              avFrame->height,
                              stream_->time_base,
                              codecContext->sample_aspect_ratio,
                              (PixelFormat)avFrame->format,
                              ffmpegPixelFormat,
                              filterChain,
                              &frameTraitsChanged))
      {
        YAE_ASSERT(false);
        return true;
      }

      if (frameTraitsChanged && !reconfigure())
      {
        YAE_ASSERT(false);
        return true;
      }

      if (!filterGraph_.push(avFrame))
      {
        YAE_ASSERT(false);
        return true;
      }

      while (filterGraph_.pull(avFrame))
      {
        FrameAutoUnref autoUnref(avFrame);

        TVideoFramePtr vfPtr(new TVideoFrame());
        TVideoFrame & vf = *vfPtr;

        bool gotPTS = false;
        vf.time_.base_ = stream_->time_base.den;
        vf.time_.time_ = (stream_->time_base.num *
                          avFrame->best_effort_timestamp);

        // std::cerr << "T: " << avFrame->best_effort_timestamp << std::endl;

        if (!gotPTS && !hasPrevPTS_)
        {
          ptsBestEffort_ = 0;
          vf.time_.time_ = stream_->time_base.num * startTime_;
          gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t0");
        }

        if (!gotPTS)
        {
          if (ptsBestEffort_ < avFrame->best_effort_timestamp)
          {
            ptsBestEffort_ = avFrame->best_effort_timestamp;
          }
          else
          {
            ptsBestEffort_++;
          }

          vf.time_.time_ = stream_->time_base.num * ptsBestEffort_;
          gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_,
                             "avFrame->best_effort_timestamp");
        }

        if (!gotPTS && hasPrevPTS_ && frameRate_.num && frameRate_.den)
        {
          // increment by average frame duration:
          vf.time_ = prevPTS_;
          vf.time_ += TTime(frameRate_.den, frameRate_.num);
          gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t += 1/fps");
        }

        YAE_ASSERT(gotPTS);
        if (!gotPTS && hasPrevPTS_)
        {
          vf.time_ = prevPTS_;
          vf.time_.time_++;

          gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, vf.time_, "t++");
        }

        YAE_ASSERT(gotPTS);
        if (gotPTS)
        {
#if 0 // ndef NDEBUG
          if (hasPrevPTS_)
          {
            double ta = prevPTS_.toSeconds();
            double tb = vf.time_.toSeconds();
            // std::cerr << "video pts: " << tb << std::endl;
            double dt = tb - ta;
            double fd = 1.0 / native_.frameRate_;
            // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
            if (dt > 2.01 * fd)
            {
              std::cerr
                << "\nNOTE: detected large PTS jump: " << std::endl
                << "frame\t:" << framesDecoded_ - 2 << " - " << ta << std::endl
                << "frame\t:" << framesDecoded_ - 1 << " - " << tb << std::endl
                << "difference " << dt << " seconds, equivalent to "
                << dt / fd << " frames" << std::endl
                << std::endl;
            }
          }
#endif

          hasPrevPTS_ = true;
          prevPTS_ = vf.time_;
        }

        // make sure the frame is in the in/out interval:
        if (playbackInterval_)
        {
          double t = vf.time_.toSeconds();
          double dt = 1.0 / double(output_.frameRate_);
          if (t > timeOut_ || (t + dt) < timeIn_)
          {
            if (t > timeOut_)
            {
              discarded_++;
            }

#if 0
            std::cerr << "discarding video frame: " << t
                      << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                      << std::endl;
#endif
            return true;
          }

          discarded_ = 0;
        }

        vf.traits_ = output_;

        TPlanarBufferPtr sampleBuffer(new TPlanarBuffer(numSamplePlanes_),
                                      &IPlanarBuffer::deallocator);
        for (unsigned char i = 0; i < numSamplePlanes_; i++)
        {
          std::size_t rowBytes = sampleLineSize_[i];
          std::size_t rows = samplePlaneSize_[i] / rowBytes;
          sampleBuffer->resize(i, rowBytes, rows);
        }
        vf.data_ = sampleBuffer;

        // don't forget about tempo scaling:
        {
          boost::lock_guard<boost::mutex> lock(tempoMutex_);
          vf.tempo_ = tempo_;
        }

        // check for applicable subtitles:
        {
          double v0 = vf.time_.toSeconds();
          double v1 = v0 + (vf.traits_.frameRate_ ?
                            1.0 / vf.traits_.frameRate_ :
                            0.042);

          TSubsPredicate subSelector(v1);

          std::size_t nsubs = subs_ ? subs_->size() : 0;
          for (std::size_t i = 0; i < nsubs; i++)
          {
            SubtitlesTrack & subs = *((*subs_)[i]);
            subs.queue_.get(subSelector, subs.active_, &terminator_);

            TSubsFrame next;
            subs.queue_.peek(next, &terminator_);
            subs.fixupEndTimes(v1, next);
            subs.expungeOldSubs(v0);

            subs.get(v0, v1, vf.subs_);
          }
        }

        // copy the sample planes:
        const pixelFormat::Traits * ptts =
          pixelFormat::getTraits(output_.pixelFormat_);

        for (unsigned char i = 0; i < numSamplePlanes_; i++)
        {
          std::size_t dstRowBytes = sampleBuffer->rowBytes(i);
          std::size_t dstRows = sampleBuffer->rows(i);
          unsigned char * dst = sampleBuffer->data(i);

          std::size_t srcRowBytes = avFrame->linesize[i];
          std::size_t srcRows = avFrame->height;
          if (i > 0)
          {
            srcRows /= ptts->chromaBoxH_;
          }
          const unsigned char * src = avFrame->data[i];

          std::size_t copyRowBytes = std::min(srcRowBytes, dstRowBytes);
          std::size_t copyRows = std::min(srcRows, dstRows);
          for (std::size_t i = 0; i < copyRows; i++)
          {
            memcpy(dst, src, copyRowBytes);
            src += srcRowBytes;
            dst += dstRowBytes;
          }
        }

        // put the output frame into frame queue:
        if (!frameQueue_.push(vfPtr, &terminator_))
        {
          return false;
        }

        // std::cerr << "V: " << vf.time_.toSeconds() << std::endl;

        // put repeated output frames into frame queue:
        for (int i = 0; i < avFrame->repeat_pict; i++)
        {
          TVideoFramePtr rvfPtr(new TVideoFrame(vf));
          TVideoFrame & rvf = *rvfPtr;

          if (frameRate_.num && frameRate_.den)
          {
            rvf.time_ += TTime((i + 1) * frameRate_.den, frameRate_.num);
          }
          else
          {
            rvf.time_.time_++;
          }

#if 0 // ndef NDEBUG
          std::cerr << "frame repeated at " << rvf.time_.toSeconds() << " sec"
                    << std::endl;
#endif

          if (!frameQueue_.push(rvfPtr, &terminator_))
          {
            return false;
          }
        }
      }
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::threadLoop
  //
  void
  VideoTrack::threadLoop()
  {
    decoderStartup();

    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();

        TPacketPtr packetPtr;
        if (!packetQueue_.pop(packetPtr, &terminator_))
        {
          break;
        }

        if (!decode(packetPtr))
        {
          break;
        }
      }
      catch (...)
      {
        break;
      }
    }

    decoderShutdown();
  }

  //----------------------------------------------------------------
  // VideoTrack::threadStop
  //
  bool
  VideoTrack::threadStop()
  {
    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraits
  //
  bool
  VideoTrack::getTraits(VideoTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    // shortcut:
    const AVCodecContext * context = stream_->codec;

    //! pixel format:
    t.pixelFormat_ = ffmpeg_to_yae(context->pix_fmt);

    //! frame rate:
    if (stream_->avg_frame_rate.num && stream_->avg_frame_rate.den)
    {
      t.frameRate_ =
        double(stream_->avg_frame_rate.num) /
        double(stream_->avg_frame_rate.den);
    }
    else if (stream_->r_frame_rate.num && stream_->r_frame_rate.den)
    {
      t.frameRate_ =
        double(stream_->r_frame_rate.num) /
        double(stream_->r_frame_rate.den);

      if (context_->metadata)
      {
        AVDictionaryEntry * frameRateTag =
          av_dict_get(context_->metadata, "framerate", NULL, 0);

        AVDictionaryEntry * totalFramesTag =
          av_dict_get(context_->metadata, "totalframes", NULL, 0);

        if (frameRateTag)
        {
          t.frameRate_ = toScalar<double, const char *>(frameRateTag->value);
        }
        else if (totalFramesTag &&
                 context_->duration &&
                 context_->duration != int64_t(AV_NOPTS_VALUE))
        {
          // estimate frame rate based on duration
          // and metadata for total number of frames:
          double totalSeconds =
            double(context_->duration) / double(AV_TIME_BASE);

          int64_t totalFrames =
            toScalar<int64_t, const char *>(totalFramesTag->value);

          if (totalFrames)
          {
            double r = double(totalFrames) / totalSeconds;
            t.frameRate_ = std::min<double>(t.frameRate_, r);
          }
        }
      }
    }
    else
    {
      t.frameRate_ = 0.0;
      YAE_ASSERT(false);
    }

    //! encoded frame size (including any padding):
    t.encodedWidth_ = context->coded_width;
    t.encodedHeight_ = context->coded_height;

    //! top/left corner offset to the visible portion of the encoded frame:
    t.offsetTop_ = 0;
    t.offsetLeft_ = 0;

    //! dimensions of the visible portion of the encoded frame:
    t.visibleWidth_ = context->width;
    t.visibleHeight_ = context->height;

    //! pixel aspect ration, used to calculate visible frame dimensions:
    t.pixelAspectRatio_ = 1.0;

    if (context->sample_aspect_ratio.num &&
        context->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(context->sample_aspect_ratio.num) /
                             double(context->sample_aspect_ratio.den));
    }
    else if (stream_->sample_aspect_ratio.num &&
             stream_->sample_aspect_ratio.den)
    {
      t.pixelAspectRatio_ = (double(stream_->sample_aspect_ratio.num) /
                             double(stream_->sample_aspect_ratio.den));
    }

    //! a flag indicating whether video is upside-down:
    t.isUpsideDown_ = false;

    return
      t.frameRate_ > 0.0 &&
      t.encodedWidth_ > 0 &&
      t.encodedHeight_ > 0 &&
      t.pixelFormat_ != kInvalidPixelFormat;
  }

  //----------------------------------------------------------------
  // VideoTrack::setTraitsOverride
  //
  bool
  VideoTrack::setTraitsOverride(const VideoTraits & override, bool deint)
  {
    if (compare<VideoTraits>(override_, override) == 0 &&
        deinterlace_ == deint)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(!alreadyDecoding);

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = override;
    deinterlace_ = deint;

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getTraitsOverride
  //
  bool
  VideoTrack::getTraitsOverride(VideoTraits & override) const
  {
    override = override_;
    return true;
  }

  //----------------------------------------------------------------
  // VideoTrack::getNextFrame
  //
  bool
  VideoTrack::getNextFrame(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || !playbackInterval_)
      {
        break;
      }

      // discard outlier frames:
      double t = frame->time_.toSeconds();
      double dt = 1.0 / double(frame->traits_.frameRate_);
      if (t < timeOut_ && (t + dt) > timeIn_)
      {
        break;
      }

#if 0
      std::cerr << "ignoring video frame: " << t
                << ", expecting [" << timeIn << ", " << timeOut << ")"
                << std::endl;
#endif
    }

    return ok;
  }

  //----------------------------------------------------------------
  // VideoTrack::setPlaybackInterval
  //
  void
  VideoTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackInterval_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // VideoTrack::resetTimeCounters
  //
  int
  VideoTrack::resetTimeCounters(double seekTime)
  {
    packetQueue().clear();
    frameQueue_.clear();
    packetQueue().waitForConsumerToBlock();
    frameQueue_.clear();

    // push a NULL frame into frame queue to resetTimeCounter
    // down the line:
    frameQueue_.startNewSequence(TVideoFramePtr());

    int err = 0;
    if (stream_ && stream_->codec)
    {
      avcodec_flush_buffers(stream_->codec);
#if 1
      avcodec_close(stream_->codec);
      codec_ = avcodec_find_decoder(stream_->codec->codec_id);

      av_opt_set(stream_->codec, "threads", "auto", 0);
      av_opt_set_int(stream_->codec, "refcounted_frames", 1, 0);
      err = avcodec_open2(stream_->codec, codec_, NULL);
      YAE_ASSERT(err >= 0);
#endif
    }

    setPlaybackInterval(seekTime, timeOut_, playbackInterval_);
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
    hasPrevPTS_ = false;
    ptsBestEffort_ = 0;
    framesDecoded_ = 0;

    return err;
  }

  //----------------------------------------------------------------
  // VideoTrack::setDeinterlacing
  //
  bool
  VideoTrack::setDeinterlacing(bool deint)
  {
    return setTraitsOverride(override_, deint);
  }

  //----------------------------------------------------------------
  // ffmpeg_to_yae
  //
  static bool
  ffmpeg_to_yae(enum AVSampleFormat givenFormat,
                TAudioSampleFormat & sampleFormat,
                TAudioChannelFormat & channelFormat)
  {
    channelFormat =
      (givenFormat == AV_SAMPLE_FMT_U8  ||
       givenFormat == AV_SAMPLE_FMT_S16 ||
       givenFormat == AV_SAMPLE_FMT_S32 ||
       givenFormat == AV_SAMPLE_FMT_FLT ||
       givenFormat == AV_SAMPLE_FMT_DBL) ?
      kAudioChannelsPacked : kAudioChannelsPlanar;

    switch (givenFormat)
    {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
        sampleFormat = kAudio8BitOffsetBinary;
        break;

      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
#ifdef __BIG_ENDIAN__
        sampleFormat = kAudio16BitBigEndian;
#else
        sampleFormat = kAudio16BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
#ifdef __BIG_ENDIAN__
        sampleFormat = kAudio32BitBigEndian;
#else
        sampleFormat = kAudio32BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        sampleFormat = kAudio32BitFloat;
        break;

      case AV_SAMPLE_FMT_DBL:
      case AV_SAMPLE_FMT_DBLP:
        sampleFormat = kAudio64BitDouble;
        break;

      default:
        channelFormat = kAudioChannelFormatInvalid;
        sampleFormat = kAudioInvalidFormat;
        return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // yae_to_ffmpeg
  //
  static enum AVSampleFormat
  yae_to_ffmpeg(TAudioSampleFormat sampleFormat,
                TAudioChannelFormat channelFormat)
  {
    bool planar = channelFormat == kAudioChannelsPlanar;

    switch (sampleFormat)
    {
      case kAudio8BitOffsetBinary:
        return (planar ? AV_SAMPLE_FMT_U8P : AV_SAMPLE_FMT_U8);

      case kAudio16BitBigEndian:
      case kAudio16BitLittleEndian:
        YAE_ASSERT(sampleFormat == kAudio16BitNative);
        return (planar ? AV_SAMPLE_FMT_S16P : AV_SAMPLE_FMT_S16);

      case kAudio24BitLittleEndian:
        YAE_ASSERT(false);

      case kAudio32BitBigEndian:
      case kAudio32BitLittleEndian:
        YAE_ASSERT(sampleFormat == kAudio32BitNative);
        return (planar ? AV_SAMPLE_FMT_S32P : AV_SAMPLE_FMT_S32);

      case kAudio32BitFloat:
        return (planar ? AV_SAMPLE_FMT_FLTP : AV_SAMPLE_FMT_FLT);

      case kAudio64BitDouble:
        return (planar ? AV_SAMPLE_FMT_DBLP : AV_SAMPLE_FMT_DBL);

      default:
        break;
    }

    YAE_ASSERT(false);
    return AV_SAMPLE_FMT_NONE;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph
  //
  struct AudioFilterGraph
  {
    AudioFilterGraph();
    ~AudioFilterGraph();

    void reset();

    bool setup(// input format:
               const AVRational & srcTimeBase,
               enum AVSampleFormat srcSampleFmt,
               int srcSampleRate,
               int64 srcChannelLayout,

               // output format:
               enum AVSampleFormat dstSampleFmt,
               int dstSampleRate,
               int64 dstChannelLayout,

               const char * filterChain = NULL,
               bool * frameTraitsChanged = NULL);

    bool push(AVFrame * in);
    bool pull(AVFrame * out);

  protected:
    std::string filterChain_;

    AVRational srcTimeBase_;

    enum AVSampleFormat srcSampleFmt_;
    enum AVSampleFormat dstSampleFmt_[2];

    int srcSampleRate_;
    int dstSampleRate_[2];

    int64 srcChannelLayout_;
    int64 dstChannelLayout_[2];

    AVFilterContext * src_;
    AVFilterContext * sink_;

    AVFilterInOut * in_;
    AVFilterInOut * out_;
    AVFilterGraph * graph_;
  };

  //----------------------------------------------------------------
  // AudioFilterGraph::AudioFilterGraph
  //
  AudioFilterGraph::AudioFilterGraph():
    src_(NULL),
    sink_(NULL),
    in_(NULL),
    out_(NULL),
    graph_(NULL)
  {
    reset();
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::~AudioFilterGraph
  //
  AudioFilterGraph::~AudioFilterGraph()
  {
    reset();
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::reset
  //
  void
  AudioFilterGraph::reset()
  {
    avfilter_graph_free(&graph_);
    avfilter_inout_free(&in_);
    avfilter_inout_free(&out_);

    srcTimeBase_.num = 0;
    srcTimeBase_.den = 1;

    srcSampleFmt_    = AV_SAMPLE_FMT_NONE;
    dstSampleFmt_[0] = AV_SAMPLE_FMT_NONE;
    dstSampleFmt_[1] = AV_SAMPLE_FMT_NONE;

    srcSampleRate_    = -1;
    dstSampleRate_[0] = -1;
    dstSampleRate_[1] = -1;

    srcChannelLayout_    = -1;
    dstChannelLayout_[0] = -1;
    dstChannelLayout_[1] = -1;
  }


  //----------------------------------------------------------------
  // AudioFilterGraph::setup
  //
  bool
  AudioFilterGraph::setup(// input format:
                          const AVRational & srcTimeBase,
                          enum AVSampleFormat srcSampleFmt,
                          int srcSampleRate,
                          int64 srcChannelLayout,

                          // output format:
                          enum AVSampleFormat dstSampleFmt,
                          int dstSampleRate,
                          int64 dstChannelLayout,

                          const char * filterChain,
                          bool * frameTraitsChanged)
  {
    filterChain = filterChain ? filterChain : "";
    bool sameTraits = (srcSampleRate_ == srcSampleRate &&
                       srcChannelLayout_ == srcChannelLayout &&
                       srcSampleFmt_ == srcSampleFmt &&
                       dstSampleRate_[0] == dstSampleRate &&
                       dstSampleFmt_[0] == dstSampleFmt &&
                       dstChannelLayout_[0] == dstChannelLayout &&
                       filterChain_ == filterChain);

    if (frameTraitsChanged)
    {
      *frameTraitsChanged = !sameTraits;
    }

    if (sameTraits)
    {
      return true;
    }

    reset();

    srcTimeBase_ = srcTimeBase;

    srcSampleFmt_    = srcSampleFmt;
    dstSampleFmt_[0] = dstSampleFmt;

    srcSampleRate_    = srcSampleRate;
    dstSampleRate_[0] = dstSampleRate;

    srcChannelLayout_    = srcChannelLayout;
    dstChannelLayout_[0] = dstChannelLayout;

    AVFilter * srcFilterDef = avfilter_get_by_name("abuffer");
    AVFilter * dstFilterDef = avfilter_get_by_name("abuffersink");

    graph_ = avfilter_graph_alloc();

    std::string srcCfg;
    {
      std::ostringstream os;

      const char * txtSampleFmt = av_get_sample_fmt_name(srcSampleFmt_);
      os << "time_base=" << srcTimeBase_.num << "/" << srcTimeBase_.den << ':'
         << "sample_rate=" << srcSampleRate_ << ':'
         << "sample_fmt=" << txtSampleFmt << ':'
         << "channel_layout=0x" << std::hex << srcChannelLayout;
      srcCfg = os.str().c_str();
    }

    int err = avfilter_graph_create_filter(&src_,
                                           srcFilterDef,
                                           "in",
                                           srcCfg.c_str(),
                                           NULL,
                                           graph_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = avfilter_graph_create_filter(&sink_,
                                       dstFilterDef,
                                       "out",
                                       NULL,
                                       NULL,
                                       graph_);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = av_opt_set_int_list(sink_, "sample_fmts",
                              dstSampleFmt_, AV_SAMPLE_FMT_NONE,
                              AV_OPT_SEARCH_CHILDREN);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = av_opt_set_int_list(sink_, "channel_layouts",
                              dstChannelLayout_, -1,
                              AV_OPT_SEARCH_CHILDREN);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = av_opt_set_int_list(sink_, "sample_rates",
                              dstSampleRate_, -1,
                              AV_OPT_SEARCH_CHILDREN);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    out_ = avfilter_inout_alloc();
    err = out_ ? 0 : AVERROR(ENOMEM);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    out_->name = av_strdup("in");
    out_->filter_ctx = src_;
    out_->pad_idx = 0;
    out_->next = NULL;

    in_ = avfilter_inout_alloc();
    err = in_ ? 0 : AVERROR(ENOMEM);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    in_->name = av_strdup("out");
    in_->filter_ctx = sink_;
    in_->pad_idx = 0;
    in_->next = NULL;

    std::string filters;
    {
      std::ostringstream os;
      if (*filterChain && strcmp(filterChain, "anull") != 0)
      {
        os << filterChain << ",";
      }
      else if (!filterChain || !*filterChain)
      {
        os << "anull";
      }

      filters = os.str().c_str();
      filterChain_ = filterChain;
    }

    err = avfilter_graph_parse(graph_, filters.c_str(), &in_, &out_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    err = avfilter_graph_config(graph_, NULL);
    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);

    return true;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::push
  //
  bool
  AudioFilterGraph::push(AVFrame * frame)
  {
    int err = av_buffersrc_add_frame_flags(src_, frame, 0);

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }

  //----------------------------------------------------------------
  // AudioFilterGraph::pull
  //
  bool
  AudioFilterGraph::pull(AVFrame * frame)
  {
    int err = av_buffersink_get_frame(sink_, frame);
    if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
    {
      return false;
    }

    YAE_ASSERT_NO_AVERROR_OR_RETURN(err, false);
    return true;
  }


  //----------------------------------------------------------------
  // AudioTrack
  //
  struct AudioTrack : public Track
  {
    AudioTrack(AVFormatContext * context, AVStream * stream);

    // virtual:
    ~AudioTrack();

    // virtual:
    bool open();

    // virtual: FIXME: write me!
    bool decoderStartup();
    bool decoderShutdown();
    bool decode(const TPacketPtr & packetPtr);

    // virtual:
    void threadLoop();
    bool threadStop();

    // flush and reset filter graph if native traits change during decoding:
    void noteNativeTraitsChanged();

    // audio traits, not overridden:
    bool getTraits(AudioTraits & traits) const;

    // use this for audio format conversion (sample rate, channels, etc...)
    bool setTraitsOverride(const AudioTraits & override);
    bool getTraitsOverride(AudioTraits & override) const;

    // retrieve a decoded/converted frame from the queue:
    bool getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator);

    // adjust playback interval (used when seeking or looping):
    void setPlaybackInterval(double timeIn, double timeOut, bool enabled);

    // reset time counters, setup to output frames
    // starting from a given time point:
    int resetTimeCounters(double seekTime);

    // adjust frame duration:
    bool setTempo(double tempo);

    TAudioFrameQueue frameQueue_;
    AudioTraits override_;
    AudioTraits native_;
    AudioTraits output_;

    // output sample buffer properties:
    int nativeChannels_;
    int outputChannels_;
    unsigned int nativeBytesPerSample_;
    unsigned int outputBytesPerSample_;

    TTime prevPTS_;
    bool hasPrevPTS_;
    uint64 prevNumSamples_;
    uint64 samplesDecoded_;

    FrameWithAutoCleanup frameAutoCleanup_;

    // for adjusting audio frame duration:
    std::vector<unsigned char> tempoBuffer_;
    IAudioTempoFilter * tempoFilter_;

    AudioFilterGraph filterGraph_;
  };

  //----------------------------------------------------------------
  // AudioTrackPtr
  //
  typedef boost::shared_ptr<AudioTrack> AudioTrackPtr;

  //----------------------------------------------------------------
  // AudioTrack::AudioTrack
  //
  AudioTrack::AudioTrack(AVFormatContext * context, AVStream * stream):
    Track(context, stream),
    frameQueue_(kQueueSizeMedium),
    nativeBytesPerSample_(0),
    outputBytesPerSample_(0),
    hasPrevPTS_(false),
    prevNumSamples_(0),
    samplesDecoded_(0),
    tempoFilter_(NULL)
  {
    YAE_ASSERT(stream->codec->codec_type == AVMEDIA_TYPE_AUDIO);

    // match output queue size to input queue size:
    frameQueue_.setMaxSize(packetQueue_.getMaxSize());
  }

  //----------------------------------------------------------------
  // AudioTrack::~AudioTrack
  //
  AudioTrack::~AudioTrack()
  {
    delete tempoFilter_;
    tempoFilter_ = NULL;
  }

  //----------------------------------------------------------------
  // AudioTrack::open
  //
  bool
  AudioTrack::open()
  {
    if (Track::open())
    {
      bool ok = getTraits(override_);
      samplesDecoded_ = 0;

      AVCodecContext * context = this->codecContext();
      if (!context->channel_layout)
      {
        context->channel_layout =
          av_get_default_channel_layout(context->channels);
      }

      return ok;
    }

    return false;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderStartup
  //
  bool
  AudioTrack::decoderStartup()
  {
    output_ = override_;

    outputChannels_ = getNumberOfChannels(output_.channelLayout_);

    outputBytesPerSample_ =
      outputChannels_ * getBitsPerSample(output_.sampleFormat_) / 8;

    if (!getTraits(native_))
    {
      return false;
    }

    noteNativeTraitsChanged();

    startTime_ = stream_->start_time;
    if (startTime_ == AV_NOPTS_VALUE)
    {
      startTime_ = 0;
    }

    frameAutoCleanup_.reset();
    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    samplesDecoded_ = 0;

    frameQueue_.open();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decoderShutdown
  //
  bool
  AudioTrack::decoderShutdown()
  {
    frameAutoCleanup_.reset();

    frameQueue_.close();
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::decode
  //
  bool
  AudioTrack::decode(const TPacketPtr & packetPtr)
  {
    try
    {
      // make a local shallow copy of the packet:
      AVPacket packet = packetPtr->ffmpeg_;
      AVCodecContext * codecContext = this->codecContext();

      // Decode audio frame, piecewise:
      std::list<std::vector<unsigned char> > chunks;
      std::size_t outputBytes = 0;

      // shortcuts:
      enum AVSampleFormat outputFormat =
        yae_to_ffmpeg(output_.sampleFormat_, output_.channelFormat_);

      int64 outputChannelLayout =
        av_get_default_channel_layout(outputChannels_);

      while (packet.size)
      {
        // Decode audio frame
        int gotFrame = 0;
        AVFrame * avFrame = frameAutoCleanup_;
        avcodec_get_frame_defaults(avFrame);

        int bytesUsed = avcodec_decode_audio4(codecContext,
                                              avFrame,
                                              &gotFrame,
                                              &packet);

        if (bytesUsed < 0)
        {
          break;
        }

        // adjust the packet (the copy, not the original):
        packet.size -= bytesUsed;
        packet.data += bytesUsed;

        if (!gotFrame || !avFrame->nb_samples)
        {
          continue;
        }

        avFrame->pts = av_frame_get_best_effort_timestamp(avFrame);

        const char * filterChain = NULL;
        bool frameTraitsChanged = false;
        if (!filterGraph_.setup(// input format:
                                stream_->time_base,
                                (enum AVSampleFormat)avFrame->format,
                                avFrame->sample_rate,
                                avFrame->channel_layout,

                                // output format:
                                outputFormat,
                                output_.sampleRate_,
                                outputChannelLayout,

                                filterChain,
                                &frameTraitsChanged))
        {
          YAE_ASSERT(false);
          return true;
        }

        if (frameTraitsChanged)
        {
          // detected a change in the number of audio channels,
          // or detected a change in audio sample rate,
          // prepare to remix or resample accordingly:
          if (!getTraits(native_))
          {
            return false;
          }

          noteNativeTraitsChanged();
        }

        if (!filterGraph_.push(avFrame))
        {
          YAE_ASSERT(false);
          return true;
        }

        while (filterGraph_.pull(avFrame))
        {
          FrameAutoUnref autoUnref(avFrame);

          const int bufferSize = avFrame->nb_samples * outputBytesPerSample_;
          chunks.push_back(std::vector<unsigned char>
                           (avFrame->data[0],
                            avFrame->data[0] + bufferSize));
          outputBytes += bufferSize;
        }
      }

      if (!outputBytes)
      {
        return true;
      }

      std::size_t numOutputSamples = outputBytes / outputBytesPerSample_;
      samplesDecoded_ += numOutputSamples;

      TAudioFramePtr afPtr(new TAudioFrame());
      TAudioFrame & af = *afPtr;

      af.traits_ = output_;
      af.time_.base_ = stream_->time_base.den;

      bool gotPTS = false;

      if (!gotPTS &&
          packet.pts != AV_NOPTS_VALUE)
      {
        af.time_.time_ = stream_->time_base.num * packet.pts;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_, "packet.pts");
      }

      if (!gotPTS &&
          packet.dts != AV_NOPTS_VALUE)
      {
        af.time_.time_ = stream_->time_base.num * packet.dts;
        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_, "packet.dts");
      }

      if (!gotPTS)
      {
        af.time_.base_ = output_.sampleRate_;
        af.time_.time_ = samplesDecoded_ - numOutputSamples;
        af.time_ += TTime(startTime_, stream_->time_base.den);

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_ += TTime(prevNumSamples_, output_.sampleRate_);

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      YAE_ASSERT(gotPTS);
      if (!gotPTS && hasPrevPTS_)
      {
        af.time_ = prevPTS_;
        af.time_.time_++;

        gotPTS = verifyPTS(hasPrevPTS_, prevPTS_, af.time_);
      }

      YAE_ASSERT(gotPTS);
      if (gotPTS)
      {
#if 0 // ndef NDEBUG
        if (hasPrevPTS_)
        {
          double ta = prevPTS_.toSeconds();
          double tb = af.time_.toSeconds();
          // std::cerr << "audio pts: " << tb << std::endl;
          double dt = tb - ta;
          // std::cerr << ta << " ... " << tb << ", dt: " << dt << std::endl;
          if (dt > 0.67)
          {
            std::cerr
              << "\nNOTE: detected large audio PTS jump -- " << std::endl
              << dt << " seconds" << std::endl
              << std::endl;
          }
        }
#endif

        hasPrevPTS_ = true;
        prevPTS_ = af.time_;
        prevNumSamples_ = numOutputSamples;
      }

      // make sure the frame is in the in/out interval:
      if (playbackInterval_)
      {
        double t = af.time_.toSeconds();
        double dt = double(numOutputSamples) / double(output_.sampleRate_);
        if (t > timeOut_ || (t + dt) < timeIn_)
        {
          if (t > timeOut_)
          {
            discarded_++;
          }

#if 0
          std::cerr << "discarding audio frame: " << t
                    << ", expecting [" << timeIn_ << ", " << timeOut_ << ")"
                    << std::endl;
#endif
          return true;
        }

        discarded_ = 0;
      }

      TPlanarBufferPtr sampleBuffer(new TPlanarBuffer(1),
                                    &IPlanarBuffer::deallocator);
      af.data_ = sampleBuffer;

      bool shouldAdjustTempo = true;
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);
        shouldAdjustTempo = tempoFilter_ && tempo_ != 1.0;
        af.tempo_ = shouldAdjustTempo ? tempo_ : 1.0;
      }

      if (!shouldAdjustTempo)
      {
        // concatenate chunks into a contiguous frame buffer:
        sampleBuffer->resize(0, outputBytes, 1);
        unsigned char * afSampleBuffer = sampleBuffer->data(0);

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();
          memcpy(afSampleBuffer, chunk, chunkSize);

          afSampleBuffer += chunkSize;
          chunks.pop_front();
        }
      }
      else
      {
        boost::lock_guard<boost::mutex> lock(tempoMutex_);

        // pass source samples through the tempo filter:
        std::size_t frameSize = 0;

        while (!chunks.empty())
        {
          const unsigned char * chunk = &(chunks.front().front());
          std::size_t chunkSize = chunks.front().size();

          const unsigned char * srcStart = chunk;
          const unsigned char * srcEnd = srcStart + chunkSize;
          const unsigned char * src = srcStart;

          while (src < srcEnd)
          {
            unsigned char * dstStart = &tempoBuffer_[0];
            unsigned char * dstEnd = dstStart + tempoBuffer_.size();
            unsigned char * dst = dstStart;
            tempoFilter_->apply(&src, srcEnd, &dst, dstEnd);

            std::size_t tmpSize = dst - dstStart;
            sampleBuffer->resize(frameSize + tmpSize);

            unsigned char * afSampleBuffer = sampleBuffer->data(0);
            memcpy(afSampleBuffer + frameSize, dstStart, tmpSize);
            frameSize += tmpSize;
          }

          YAE_ASSERT(src == srcEnd);
          chunks.pop_front();
        }
      }

      // put the decoded frame into frame queue:
      if (!frameQueue_.push(afPtr, &terminator_))
      {
        return false;
      }

      // std::cerr << "A: " << af.time_.toSeconds() << std::endl;
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::threadLoop
  //
  void
  AudioTrack::threadLoop()
  {
    if (!decoderStartup())
    {
      return;
    }

    while (true)
    {
      try
      {
        boost::this_thread::interruption_point();

        TPacketPtr packetPtr;
        if (!packetQueue_.pop(packetPtr, &terminator_))
        {
          break;
        }

        if (!decode(packetPtr))
        {
          break;
        }
      }
      catch (...)
      {
        break;
      }
    }

    decoderShutdown();
  }

  //----------------------------------------------------------------
  // AudioTrack::threadStop
  //
  bool
  AudioTrack::threadStop()
  {
    frameQueue_.close();
    return Track::threadStop();
  }

  //----------------------------------------------------------------
  // AudioTrack::noteNativeTraitsChanged
  //
  void
  AudioTrack::noteNativeTraitsChanged()
  {
    // reset the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      delete tempoFilter_;
      tempoFilter_ = NULL;
    }

    unsigned int bitsPerSample = getBitsPerSample(native_.sampleFormat_);
    nativeChannels_ = getNumberOfChannels(native_.channelLayout_);
    nativeBytesPerSample_ = (nativeChannels_ * bitsPerSample / 8);

    // initialize the tempo filter:
    {
      boost::lock_guard<boost::mutex> lock(tempoMutex_);
      YAE_ASSERT(!tempoFilter_);

      if ((output_.channelFormat_ != kAudioChannelsPlanar ||
           output_.channelLayout_ == kAudioMono))
      {
        if (output_.sampleFormat_ == kAudio8BitOffsetBinary)
        {
          tempoFilter_ = new TAudioTempoFilterU8();
        }
        else if (output_.sampleFormat_ == kAudio16BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI16();
        }
        else if (output_.sampleFormat_ == kAudio32BitNative)
        {
          tempoFilter_ = new TAudioTempoFilterI32();
        }
        else if (output_.sampleFormat_ == kAudio32BitFloat)
        {
          tempoFilter_ = new TAudioTempoFilterF32();
        }
        else if (output_.sampleFormat_ == kAudio64BitDouble)
        {
          tempoFilter_ = new TAudioTempoFilterF64();
        }

        if (tempoFilter_)
        {
          tempoFilter_->reset(output_.sampleRate_, outputChannels_);
          tempoFilter_->setTempo(tempo_);

          std::size_t fragmentSize = tempoFilter_->fragmentSize();
          tempoBuffer_.resize(fragmentSize * 3);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraits
  //
  bool
  AudioTrack::getTraits(AudioTraits & t) const
  {
    if (!stream_)
    {
      return false;
    }

    // shortcut:
    const AVCodecContext * context = stream_->codec;

    switch (context->sample_fmt)
    {
      case AV_SAMPLE_FMT_U8:
      case AV_SAMPLE_FMT_U8P:
        t.sampleFormat_ = kAudio8BitOffsetBinary;
        break;

      case AV_SAMPLE_FMT_S16:
      case AV_SAMPLE_FMT_S16P:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio16BitBigEndian;
#else
        t.sampleFormat_ = kAudio16BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_S32:
      case AV_SAMPLE_FMT_S32P:
#ifdef __BIG_ENDIAN__
        t.sampleFormat_ = kAudio32BitBigEndian;
#else
        t.sampleFormat_ = kAudio32BitLittleEndian;
#endif
        break;

      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        t.sampleFormat_ = kAudio32BitFloat;
        break;

      default:
        t.sampleFormat_ = kAudioInvalidFormat;
        break;
    }

    switch (context->channels)
    {
      case 1:
        t.channelLayout_ = kAudioMono;
        break;

      case 2:
        t.channelLayout_ = kAudioStereo;
        break;

      case 3:
        t.channelLayout_ = kAudio2Pt1;
        break;

      case 4:
        t.channelLayout_ = kAudioQuad;
        break;

      case 5:
        t.channelLayout_ = kAudio4Pt1;
        break;

      case 6:
        t.channelLayout_ = kAudio5Pt1;
        break;

      case 7:
        t.channelLayout_ = kAudio6Pt1;
        break;

      case 8:
        t.channelLayout_ = kAudio7Pt1;
        break;

      default:
        t.channelLayout_ = kAudioChannelLayoutInvalid;
        break;
    }

    //! audio sample rate, Hz:
    t.sampleRate_ = context->sample_rate;

    //! packed, planar:
    switch (context->sample_fmt)
    {
      case AV_SAMPLE_FMT_U8P:
      case AV_SAMPLE_FMT_S16P:
      case AV_SAMPLE_FMT_S32P:
      case AV_SAMPLE_FMT_FLTP:
        t.channelFormat_ = kAudioChannelsPlanar;
        break;

      default:
        t.channelFormat_ = kAudioChannelsPacked;
        break;
    }

    return
      t.sampleRate_ > 0 &&
      t.sampleFormat_ != kAudioInvalidFormat &&
      t.channelLayout_ != kAudioChannelLayoutInvalid;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTraitsOverride
  //
  bool
  AudioTrack::setTraitsOverride(const AudioTraits & override)
  {
    if (compare<AudioTraits>(override_, override) == 0)
    {
      // nothing changed:
      return true;
    }

    bool alreadyDecoding = thread_.isRunning();
    YAE_ASSERT(!alreadyDecoding);

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(true);
      frameQueue_.clear();
      thread_.stop();
      thread_.wait();
    }

    override_ = override;

    if (alreadyDecoding)
    {
      terminator_.stopWaiting(false);
      return thread_.run();
    }

    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getTraitsOverride
  //
  bool
  AudioTrack::getTraitsOverride(AudioTraits & override) const
  {
    override = override_;
    return true;
  }

  //----------------------------------------------------------------
  // AudioTrack::getNextFrame
  //
  bool
  AudioTrack::getNextFrame(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    bool ok = true;
    while (ok)
    {
      ok = frameQueue_.pop(frame, terminator);
      if (!ok || !frame || !playbackInterval_)
      {
        break;
      }

      // discard outlier frames:
      const AudioTraits & atraits = frame->traits_;
      unsigned int sampleSize = getBitsPerSample(atraits.sampleFormat_) / 8;
      int channels = getNumberOfChannels(atraits.channelLayout_);
      std::size_t frameSize = frame->data_->rowBytes(0);
      std::size_t numSamples = frameSize / (channels * sampleSize);

      double t = frame->time_.toSeconds();
      double dt = double(numSamples) / double(atraits.sampleRate_);

      if (t < timeOut_ && (t + dt) > timeIn_)
      {
        break;
      }

#if 0
      std::cerr << "ignoring audio frame: " << t
                << ", expecting [" << timeIn << ", " << timeOut << ")"
                << std::endl;
#endif
    }

    return ok;
  }

  //----------------------------------------------------------------
  // AudioTrack::setPlaybackInterval
  //
  void
  AudioTrack::setPlaybackInterval(double timeIn, double timeOut, bool enabled)
  {
    timeIn_ = timeIn;
    timeOut_ = timeOut;
    playbackInterval_ = enabled;
    discarded_ = 0;
  }

  //----------------------------------------------------------------
  // AudioTrack::resetTimeCounters
  //
  int
  AudioTrack::resetTimeCounters(double seekTime)
  {
    packetQueue().clear();
    frameQueue_.clear();
    packetQueue().waitForConsumerToBlock();
    frameQueue_.clear();

    // push a NULL frame into frame queue to resetTimeCounter
    // down the line:
    frameQueue_.startNewSequence(TAudioFramePtr());

    int err = 0;
    if (stream_ && stream_->codec)
    {
      avcodec_flush_buffers(stream_->codec);
#if 1
      avcodec_close(stream_->codec);
      codec_ = avcodec_find_decoder(stream_->codec->codec_id);

      av_opt_set(stream_->codec, "threads", "auto", 0);
      av_opt_set_int(stream_->codec, "refcounted_frames", 1, 0);
      err = avcodec_open2(stream_->codec, codec_, NULL);
      YAE_ASSERT(err >= 0);
#endif
    }

    setPlaybackInterval(seekTime, timeOut_, playbackInterval_);
    hasPrevPTS_ = false;
    prevNumSamples_ = 0;
    startTime_ = 0; // int64_t(double(stream_->time_base.den) * seekTime);
    samplesDecoded_ = 0;

    return err;
  }

  //----------------------------------------------------------------
  // AudioTrack::setTempo
  //
  bool
  AudioTrack::setTempo(double tempo)
  {
    boost::lock_guard<boost::mutex> lock(tempoMutex_);

    tempo_ = tempo;

    if (tempoFilter_ && !tempoFilter_->setTempo(tempo_))
    {
      return false;
    }

    if (tempo_ == 1.0 && tempoFilter_)
    {
      tempoFilter_->clear();
    }

    return true;
  }


  //----------------------------------------------------------------
  // Movie
  //
  struct Movie
  {
    Movie();

    // NOTE: destructor will close the movie:
    ~Movie();

    bool getUrlProtocols(std::list<std::string> & protocols) const;

    bool open(const char * resourcePath);
    void close();

    inline const std::vector<VideoTrackPtr> & getVideoTracks() const
    { return videoTracks_; }

    inline const std::vector<AudioTrackPtr> & getAudioTracks() const
    { return audioTracks_; }

    inline std::size_t getSelectedVideoTrack() const
    { return selectedVideoTrack_; }

    inline std::size_t getSelectedAudioTrack() const
    { return selectedAudioTrack_; }

    bool selectVideoTrack(std::size_t i);
    bool selectAudioTrack(std::size_t i);

    // this will read the file and push packets to decoding queues:
    void threadLoop();
    bool threadStart();
    bool threadStop();

    bool requestSeekTime(double seekTime);

  protected:
    int seekTo(double seekTime);

  public:
    int rewind(const AudioTrackPtr & audioTrack,
               const VideoTrackPtr & videoTrack,
               bool seekToTimeIn = true);

    void getPlaybackInterval(double & timeIn, double & timeOut) const;
    void setPlaybackIntervalStart(double timeIn);
    void setPlaybackIntervalEnd(double timeOut);
    void setPlaybackInterval(bool enabled);
    void setPlaybackLooping(bool enabled);

    void skipLoopFilter(bool skip);
    void skipNonReferenceFrames(bool skip);

    bool setTempo(double tempo);
    bool setDeinterlacing(bool enabled);

    std::size_t subsCount() const;
    TSubsFormat subsInfo(std::size_t i, TTrackInfo & info) const;
    void subsRender(std::size_t i, bool render);

    SubtitlesTrack * subsLookup(unsigned int streamIndex);

    std::size_t countChapters() const;
    bool getChapterInfo(std::size_t i, TChapter & c) const;

  private:
    // intentionally disabled:
    Movie(const Movie &);
    Movie & operator = (const Movie &);

  protected:
    // worker thread:
    Thread<Movie> thread_;
    mutable boost::mutex mutex_;

    // deadlock avoidance mechanism:
    QueueWaitMgr terminator_;

    AVFormatContext * context_;

    std::vector<VideoTrackPtr> videoTracks_;
    std::vector<AudioTrackPtr> audioTracks_;
    std::vector<TSubsTrackPtr> subs_;
    std::map<unsigned int, std::size_t> subsIdx_;

    // index of the selected video/audio track:
    std::size_t selectedVideoTrack_;
    std::size_t selectedAudioTrack_;

    // these are used to speed up video decoding:
    bool skipLoopFilter_;
    bool skipNonReferenceFrames_;

    // demuxer current position (DTS):
    TTime dts_;
    int dtsStreamIndex_;

    double timeIn_;
    double timeOut_;
    bool playbackInterval_;
    bool looping_;

    bool mustSeek_;
    double seekTime_;
  };


  //----------------------------------------------------------------
  // Movie::Movie
  //
  Movie::Movie():
    thread_(this),
    context_(NULL),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    selectedVideoTrack_(0),
    selectedAudioTrack_(0),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    playbackInterval_(false),
    looping_(false),
    mustSeek_(false),
    seekTime_(0.0)
  {}

  //----------------------------------------------------------------
  // Movie::~Movie
  //
  Movie::~Movie()
  {
    close();
  }

  //----------------------------------------------------------------
  // Movie::getUrlProtocols
  //
  bool
  Movie::getUrlProtocols(std::list<std::string> & protocols) const
  {
    protocols.clear();

    void * opaque = NULL;
    const char * name = NULL;
    while ((name = avio_enum_protocols(&opaque, 0)))
    {
      protocols.push_back(std::string(name));
    }

    return true;
  }

  //----------------------------------------------------------------
  // Movie::open
  //
  bool
  Movie::open(const char * resourcePath)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();

    int err = avformat_open_input(&context_,
                                  resourcePath,
                                  NULL, // AVInputFormat to force
                                  NULL);// AVDictionary of options
    if (err != 0)
    {
      close();
      return false;
    }

    // spend at most 2 seconds trying to analyze the file:
    context_->max_analyze_duration = 30 * AV_TIME_BASE;

    err = avformat_find_stream_info(context_, NULL);
    if (err < 0)
    {
      close();
      return false;
    }

    for (unsigned int i = 0; i < context_->nb_streams; i++)
    {
      AVStream * stream = context_->streams[i];
      TrackPtr track(new Track(context_, stream));

      if (!track->open())
      {
        // unsupported codec, ignore it:
        continue;
      }

      const AVMediaType codecType = stream->codec->codec_type;
      if (codecType == AVMEDIA_TYPE_VIDEO)
      {
        VideoTrackPtr track(new VideoTrack(context_, stream));
        VideoTraits traits;
        if (track->getTraits(traits))
        {
          videoTracks_.push_back(track);
        }
      }
      else if (codecType == AVMEDIA_TYPE_AUDIO)
      {
        AudioTrackPtr track(new AudioTrack(context_, stream));
        AudioTraits traits;
        if (track->getTraits(traits))
        {
          audioTracks_.push_back(track);
        }
      }
      else if (codecType == AVMEDIA_TYPE_SUBTITLE)
      {
        // avoid codec instance sharing between a temporary Track object
        // and SubtitlesTrack object:
        track = TrackPtr();

        subsIdx_[i] = subs_.size();
        TSubsTrackPtr subsTrk(new SubtitlesTrack(stream, subs_.size()));
        subs_.push_back(subsTrk);
      }
    }

    if (videoTracks_.empty() &&
        audioTracks_.empty())
    {
      // no decodable video/audio tracks present:
      close();
      return false;
    }

    // by default do not select any tracks:
    selectedVideoTrack_ = videoTracks_.size();
    selectedAudioTrack_ = audioTracks_.size();

    return true;
  }

  //----------------------------------------------------------------
  // Movie::close
  //
  void
  Movie::close()
  {
    if (context_ == NULL)
    {
      return;
    }

    threadStop();

    const std::size_t numVideoTracks = videoTracks_.size();
    selectVideoTrack(numVideoTracks);

    const std::size_t numAudioTracks = audioTracks_.size();
    selectAudioTrack(numAudioTracks);

    videoTracks_.clear();
    audioTracks_.clear();
    subs_.clear();
    subsIdx_.clear();

    avformat_close_input(&context_);
  }

  //----------------------------------------------------------------
  // Movie::selectVideoTrack
  //
  bool
  Movie::selectVideoTrack(std::size_t i)
  {
    const std::size_t numVideoTracks = videoTracks_.size();
    if (selectedVideoTrack_ < numVideoTracks)
    {
      // close currently selected track:
      VideoTrackPtr track = videoTracks_[selectedVideoTrack_];
      track->close();
    }

    selectedVideoTrack_ = i;
    if (selectedVideoTrack_ >= numVideoTracks)
    {
      return false;
    }

    VideoTrackPtr track = videoTracks_[selectedVideoTrack_];
    track->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
    track->skipLoopFilter(skipLoopFilter_);
    track->skipNonReferenceFrames(skipNonReferenceFrames_);
    track->setSubs(&subs_);

    return track->open();
  }

  //----------------------------------------------------------------
  // Movie::selectAudioTrack
  //
  bool
  Movie::selectAudioTrack(std::size_t i)
  {
    const std::size_t numAudioTracks = audioTracks_.size();
    if (selectedAudioTrack_ < numAudioTracks)
    {
      // close currently selected track:
      AudioTrackPtr track = audioTracks_[selectedAudioTrack_];
      track->close();
    }

    selectedAudioTrack_ = i;
    if (selectedAudioTrack_ >= numAudioTracks)
    {
      return false;
    }

    AudioTrackPtr track = audioTracks_[selectedAudioTrack_];
    track->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
    return track->open();
  }

  //----------------------------------------------------------------
  // copyPacket
  //
  static TPacketPtr
  copyPacket(const AVPacket & ffmpeg)
  {
    try
    {
      TPacketPtr p(new Packet());
      if (p->set(ffmpeg))
      {
        return p;
      }
    }
    catch (...)
    {}

    return TPacketPtr();
  }

  //----------------------------------------------------------------
  // PacketQueueCloseOnExit
  //
  struct PacketQueueCloseOnExit
  {
    TrackPtr track_;

    PacketQueueCloseOnExit(TrackPtr track):
      track_(track)
    {
      if (track_ && track_->packetQueue().isClosed())
      {
        track_->packetQueue().open();
      }
    }

    ~PacketQueueCloseOnExit()
    {
      if (track_)
      {
        track_->packetQueue().close();
      }
    }
  };

  //----------------------------------------------------------------
  // Movie::threadLoop
  //
  void
  Movie::threadLoop()
  {
    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      videoTrack = videoTracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audioTracks_.size())
    {
      audioTrack = audioTracks_[selectedAudioTrack_];
    }

    PacketQueueCloseOnExit videoCloseOnExit(videoTrack);
    PacketQueueCloseOnExit audioCloseOnExit(audioTrack);

    AVPacket ffmpeg;
    try
    {
      int err = 0;
      while (!err)
      {
        boost::this_thread::interruption_point();

        // check whether it's time to rewind to the in-point:
        bool mustRewind = true;

        if (audioTrack && audioTrack->discarded_ < 1)
        {
          mustRewind = false;
        }
        else if (videoTrack && videoTrack->discarded_ < 3)
        {
          mustRewind = false;
        }

        if (mustRewind)
        {
          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
          }
          else
          {
            break;
          }
        }

        // service seek request, read a packet:
        {
          boost::lock_guard<boost::mutex> lock(mutex_);

          if (mustSeek_)
          {
            err = seekTo(seekTime_);
            mustSeek_ = false;
          }

          if (!err)
          {
            err = av_read_frame(context_, &ffmpeg);
          }
        }

        if (err)
        {
          av_free_packet(&ffmpeg);

          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          if (!playbackInterval_)
          {
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          // done:
          if (audioTrack)
          {
            audioTrack->packetQueue().waitForConsumerToBlock();
            audioTrack->frameQueue_.waitForConsumerToBlock();
          }

          if (videoTrack)
          {
            videoTrack->packetQueue().waitForConsumerToBlock();
            videoTrack->frameQueue_.waitForConsumerToBlock();
          }

          break;
        }

        if (ffmpeg.dts != AV_NOPTS_VALUE)
        {
          // keep track of current DTS, so that we would know which way to seek
          // relative to the current position (back/forth)
          const AVStream * stream = context_->streams[ffmpeg.stream_index];
          const AVRational & timebase = stream->time_base;
          TTime dts(ffmpeg.dts * timebase.num, timebase.den);

          dts_ = dts;
          dtsStreamIndex_ = ffmpeg.stream_index;
        }

        TPacketPtr packet = copyPacket(ffmpeg);
        if (packet)
        {
          if (videoTrack &&
              videoTrack->streamIndex() == ffmpeg.stream_index)
          {
            if (!videoTrack->packetQueue().push(packet, &terminator_))
            {
              break;
            }
          }
          else if (audioTrack &&
                   audioTrack->streamIndex() == ffmpeg.stream_index)
          {
            if (!audioTrack->packetQueue().push(packet, &terminator_))
            {
              break;
            }
          }
          else
          {
            SubtitlesTrack * subs = NULL;
            if (videoTrack &&
                (subs = subsLookup(ffmpeg.stream_index)))
            {
              AVRational tb;
              tb.num = 1;
              tb.den = AV_TIME_BASE;

              TSubsFrame sf;
              sf.time_.time_ = av_rescale_q(ffmpeg.pts,
                                            subs->stream_->time_base,
                                            tb);
              sf.time_.base_ = AV_TIME_BASE;
              sf.tEnd_ = TTime(std::numeric_limits<int64>::max(),
                               AV_TIME_BASE);

              sf.render_ = subs->render_;
              sf.traits_ = subs->format_;
              sf.index_ = subs->index_;
              sf.extraData_ = subs->extraData_;

              // copy the reference frame size:
              if (subs->stream_->codec)
              {
                sf.rw_ = subs->stream_->codec->width;
                sf.rh_ = subs->stream_->codec->height;
              }

              if (subs->format_ == kSubsDVD && !(sf.rw_ && sf.rh_))
              {
                sf.rw_ = subs->vobsub_.w_;
                sf.rh_ = subs->vobsub_.h_;
              }

              if (ffmpeg.data && ffmpeg.size)
              {
                TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                        &IPlanarBuffer::deallocator);
                buffer->resize(0, ffmpeg.size, 1);
                unsigned char * dst = buffer->data(0);
                memcpy(dst, ffmpeg.data, ffmpeg.size);

                sf.data_ = buffer;
              }

              if (ffmpeg.side_data &&
                  ffmpeg.side_data->data &&
                  ffmpeg.side_data->size)
              {
                TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                        &IPlanarBuffer::deallocator);
                buffer->resize(0, ffmpeg.side_data->size, 1, 1);
                unsigned char * dst = buffer->data(0);
                memcpy(dst, ffmpeg.side_data->data, ffmpeg.side_data->size);

                sf.sideData_ = buffer;
              }

              if (subs->codec_)
              {
                // decode the subtitle:
                int gotSub = 0;
                AVSubtitle sub;
                err = avcodec_decode_subtitle2(subs->stream_->codec,
                                               &sub,
                                               &gotSub,
                                               &ffmpeg);
                if (err >= 0 && gotSub)
                {
                  const unsigned char * header =
                    subs->stream_->codec->subtitle_header;

                  std::size_t headerSize =
                    subs->stream_->codec->subtitle_header_size;

                  sf.private_ = TSubsPrivatePtr(new TSubsPrivate(sub,
                                                                 header,
                                                                 headerSize),
                                                &TSubsPrivate::deallocator);

                  if (ffmpeg.pts != AV_NOPTS_VALUE &&
                      sub.start_display_time != AV_NOPTS_VALUE)
                  {
                    sf.time_.time_ = av_rescale_q(ffmpeg.pts +
                                                  sub.start_display_time,
                                                  subs->stream_->time_base,
                                                  tb);
                  }

                  if (ffmpeg.pts != AV_NOPTS_VALUE &&
                      sub.end_display_time != AV_NOPTS_VALUE &&
                      sub.end_display_time > sub.start_display_time)
                  {
                    double dt =
                      double(sub.end_display_time -
                             sub.start_display_time) *
                      double(subs->stream_->time_base.num) /
                      double(subs->stream_->time_base.den);

                    // avoid subs that are visible for more than 5 seconds:
                    if (dt > 0.5 && dt < 5.0)
                    {
                      sf.tEnd_.time_ = av_rescale_q(ffmpeg.pts +
                                                    sub.end_display_time,
                                                    subs->stream_->time_base,
                                                    tb);
                    }
                  }
                }
                err = 0;
              }

              subs->queue_.push(sf, &terminator_);
            }
          }
        }
        else
        {
          av_free_packet(&ffmpeg);
        }
      }
    }
    catch (...)
    {
#if 0
      std::cerr << "\nMovie::threadLoop caught exception" << std::endl;
#endif
    }
  }

  //----------------------------------------------------------------
  // Movie::threadStart
  //
  bool
  Movie::threadStart()
  {
    if (!context_)
    {
      return false;
    }

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      t->threadStart();
      t->packetQueue().waitForConsumerToBlock();
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->threadStart();
      t->packetQueue().waitForConsumerToBlock();
    }

    terminator_.stopWaiting(false);
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Movie::threadStop
  //
  bool
  Movie::threadStop()
  {
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      t->threadStop();
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->threadStop();
    }

    terminator_.stopWaiting(true);
    thread_.stop();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Movie::requestSeekTime
  //
  bool
  Movie::requestSeekTime(double seekTime)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      mustSeek_ = true;
      seekTime_ = seekTime;

      VideoTrackPtr videoTrack;
      AudioTrackPtr audioTrack;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->packetQueue().clear();
        videoTrack->frameQueue_.clear();
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->packetQueue().clear();
        audioTrack->frameQueue_.clear();
      }

      return true;
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::seekTo
  //
  int
  Movie::seekTo(double seekTime)
  {
    if (!context_)
    {
      return -1;
    }

    double tCurr = dts_.toSeconds();
    int seekFlags = AVSEEK_FLAG_BACKWARD;

    int64_t ts = int64_t(seekTime * double(AV_TIME_BASE));
    int err = avformat_seek_file(context_,
                                 -1,
                                 kMinInt64,
                                 ts,
                                 kMaxInt64,
                                 seekFlags);
    if (err < 0)
    {
      seekFlags |= AVSEEK_FLAG_ANY;
      err = avformat_seek_file(context_,
                               -1,
                               kMinInt64,
                               ts,
                               kMaxInt64,
                               seekFlags);
    }

    if (err < 0)
    {
#if 0
      std::cerr << "Movie::seek(" << seekTime << ") returned " << err
                << std::endl;
#endif
      return err;
    }

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
      err = videoTrack->resetTimeCounters(seekTime);
    }

    if (!err && selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
      err = audioTrack->resetTimeCounters(seekTime);
    }

    const std::size_t nsubs = subs_.size();
    for (std::size_t i = 0; i < nsubs; i++)
    {
      SubtitlesTrack & subs = *(subs_[i]);
      subs.clear();
    }

    return err;
  }

  //----------------------------------------------------------------
  // Movie::rewind
  //
  int
  Movie::rewind(const AudioTrackPtr & audioTrack,
                const VideoTrackPtr & videoTrack,
                bool seekToTimeIn)
  {
    // wait for the the frame queues to empty out:
    if (audioTrack)
    {
      audioTrack->packetQueue().waitForConsumerToBlock();
      audioTrack->frameQueue_.waitForConsumerToBlock();
    }
    else if (videoTrack)
    {
      videoTrack->packetQueue().waitForConsumerToBlock();
      videoTrack->frameQueue_.waitForConsumerToBlock();
    }

    boost::lock_guard<boost::mutex> lock(mutex_);
    double seekTime = seekToTimeIn ? timeIn_ : 0.0;
    return seekTo(seekTime);
  }

  //----------------------------------------------------------------
  // Movie::getPlaybackInterval
  //
  void
  Movie::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeIn = timeIn_;
      timeOut = timeOut_;
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalStart
  //
  void
  Movie::setPlaybackIntervalStart(double timeIn)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeIn_ = timeIn;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalEnd
  //
  void
  Movie::setPlaybackIntervalEnd(double timeOut)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      timeOut_ = timeOut;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackInterval
  //
  void
  Movie::setPlaybackInterval(bool enabled)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      playbackInterval_ = enabled;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackInterval_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackLooping
  //
  void
  Movie::setPlaybackLooping(bool enabled)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      looping_ = enabled;
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::skipLoopFilter
  //
  void
  Movie::skipLoopFilter(bool skip)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      skipLoopFilter_ = skip;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->skipLoopFilter(skipLoopFilter_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::skipNonReferenceFrames
  //
  void
  Movie::skipNonReferenceFrames(bool skip)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      skipNonReferenceFrames_ = skip;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->skipNonReferenceFrames(skipNonReferenceFrames_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setTempo
  //
  bool
  Movie::setTempo(double tempo)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);

      // first set audio tempo -- this may fail:
      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        if (!audioTrack->setTempo(tempo))
        {
          return false;
        }
      }

      // then set video tempo -- this can't fail:
      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        return videoTrack->setTempo(tempo);
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::setDeinterlacing
  //
  bool
  Movie::setDeinterlacing(bool enabled)
  {
    try
    {
      boost::lock_guard<boost::mutex> lock(mutex_);

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        return videoTrack->setDeinterlacing(enabled);
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::subsCount
  //
  std::size_t
  Movie::subsCount() const
  {
    return subs_.size();
  }

  //----------------------------------------------------------------
  // Movie::subsInfo
  //
  TSubsFormat
  Movie::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    info.ntracks_ = subs_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      const SubtitlesTrack & subs = *(subs_[i]);
      info.lang_ = subs.lang_;
      info.name_ = subs.name_;
      return subs.format_;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // Movie::subsRender
  //
  void
  Movie::subsRender(std::size_t i, bool render)
  {
    std::size_t nsubs = subs_.size();
    if (i < nsubs)
    {
      SubtitlesTrack & subs = *(subs_[i]);
      subs.render_ = render;
    }
  }

  //----------------------------------------------------------------
  // Movie::subsLookup
  //
  SubtitlesTrack *
  Movie::subsLookup(unsigned int streamIndex)
  {
    std::map<unsigned int, std::size_t>::const_iterator
      found = subsIdx_.find(streamIndex);

    if (found != subsIdx_.end())
    {
      return subs_[found->second].get();
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // Movie::countChapters
  //
  std::size_t
  Movie::countChapters() const
  {
    return context_ ? context_->nb_chapters : 0;
  }

  //----------------------------------------------------------------
  // Movie::getChapterInfo
  //
  bool
  Movie::getChapterInfo(std::size_t i, TChapter & c) const
  {
    if (!context_ || i >= context_->nb_chapters)
    {
      return false;
    }

    std::ostringstream os;
    os << "Chapter " << i + 1;

    const AVChapter * av = context_->chapters[i];
    AVDictionaryEntry * name = av_dict_get(av->metadata, "title", NULL, 0);
    c.name_ = name ? name->value : os.str().c_str();

    double timebase = (double(av->time_base.num) /
                       double(av->time_base.den));
    c.start_ = double(av->start) * timebase;

    double end = double(av->end) * timebase;
    c.duration_ = end - c.start_;

    return true;
  }


  //----------------------------------------------------------------
  // ReaderFFMPEG::Private
  //
  class ReaderFFMPEG::Private
  {
  private:
    // intentionally disabled:
    Private(const Private &);
    Private & operator = (const Private &);

    // flag indicating whether av_register_all has been called already:
    static bool ffmpegInitialized_;

  public:
    Private():
      frame_(NULL),
      readerId_((unsigned int)~0)
    {
      if (!ffmpegInitialized_)
      {
        av_log_set_flags(AV_LOG_SKIP_REPEATED);
        avfilter_register_all();
        av_register_all();

        avformat_network_init();

        av_lockmgr_register(&lockManager);
        ffmpegInitialized_ = true;
      }
    }

    Movie movie_;
    AVFrame * frame_;
    AVPacket packet_;
    unsigned int readerId_;
  };

  //----------------------------------------------------------------
  // ReaderFFMPEG::Private::ffmpegInitialized_
  //
  bool
  ReaderFFMPEG::Private::ffmpegInitialized_ = false;


  //----------------------------------------------------------------
  // ReaderFFMPEG::ReaderFFMPEG
  //
  ReaderFFMPEG::ReaderFFMPEG():
    IReader(),
    private_(new ReaderFFMPEG::Private())
  {}

  //----------------------------------------------------------------
  // ReaderFFMPEG::~ReaderFFMPEG
  //
  ReaderFFMPEG::~ReaderFFMPEG()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::create
  //
  ReaderFFMPEG *
  ReaderFFMPEG::create()
  {
    return new ReaderFFMPEG();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::destroy
  //
  void
  ReaderFFMPEG::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getName
  //
  const char *
  ReaderFFMPEG::getName() const
  {
    return typeid(*this).name();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getUrlProtocols
  //
  bool
  ReaderFFMPEG::getUrlProtocols(std::list<std::string> & protocols) const
  {
    return private_->movie_.getUrlProtocols(protocols);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::open
  //
  bool
  ReaderFFMPEG::open(const char * resourcePathUTF8)
  {
    return private_->movie_.open(resourcePathUTF8);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::close
  //
  void
  ReaderFFMPEG::close()
  {
    return private_->movie_.close();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfVideoTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfVideoTracks() const
  {
    return private_->movie_.getVideoTracks().size();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getNumberOfAudioTracks
  //
  std::size_t
  ReaderFFMPEG::getNumberOfAudioTracks() const
  {
    return private_->movie_.getAudioTracks().size();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedVideoTrackIndex
  //
  std::size_t
  ReaderFFMPEG::getSelectedVideoTrackIndex() const
  {
    return private_->movie_.getSelectedVideoTrack();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackIndex
  //
  std::size_t
  ReaderFFMPEG::getSelectedAudioTrackIndex() const
  {
    return private_->movie_.getSelectedAudioTrack();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::selectVideoTrack
  //
  bool
  ReaderFFMPEG::selectVideoTrack(std::size_t i)
  {
    return private_->movie_.selectVideoTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::selectAudioTrack
  //
  bool
  ReaderFFMPEG::selectAudioTrack(std::size_t i)
  {
    return private_->movie_.selectAudioTrack(i);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedVideoTrackName
  //
  void
  ReaderFFMPEG::getSelectedVideoTrackInfo(TTrackInfo & info) const
  {
    info.ntracks_ = private_->movie_.getVideoTracks().size();
    info.index_ = private_->movie_.getSelectedVideoTrack();
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
    }
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getSelectedAudioTrackInfo
  //
  void
  ReaderFFMPEG::getSelectedAudioTrackInfo(TTrackInfo & info) const
  {
    info.ntracks_ = private_->movie_.getAudioTracks().size();
    info.index_ = private_->movie_.getSelectedAudioTrack();
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
    }
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoDuration
  //
  bool
  ReaderFFMPEG::getVideoDuration(TTime & start, TTime & duration) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      private_->movie_.getVideoTracks()[i]->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioDuration
  //
  bool
  ReaderFFMPEG::getAudioDuration(TTime & start, TTime & duration) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      private_->movie_.getAudioTracks()[i]->getDuration(start, duration);
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraits
  //
  bool
  ReaderFFMPEG::getVideoTraits(VideoTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      return private_->movie_.getVideoTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraits
  //
  bool
  ReaderFFMPEG::getAudioTraits(AudioTraits & traits) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      return private_->movie_.getAudioTracks()[i]->getTraits(traits);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::setAudioTraitsOverride(const AudioTraits & override)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->setTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::setVideoTraitsOverride(const VideoTraits & override)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->setTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getAudioTraitsOverride
  //
  bool
  ReaderFFMPEG::getAudioTraitsOverride(AudioTraits & override) const
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (i < private_->movie_.getAudioTracks().size())
    {
      AudioTrackPtr t = private_->movie_.getAudioTracks()[i];
      return t->getTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getVideoTraitsOverride
  //
  bool
  ReaderFFMPEG::getVideoTraitsOverride(VideoTraits & override) const
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (i < private_->movie_.getVideoTracks().size())
    {
      VideoTrackPtr t = private_->movie_.getVideoTracks()[i];
      return t->getTraitsOverride(override);
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::seek
  //
  bool
  ReaderFFMPEG::seek(double seekTime)
  {
    return private_->movie_.requestSeekTime(seekTime);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::readVideo
  //
  bool
  ReaderFFMPEG::readVideo(TVideoFramePtr & frame, QueueWaitMgr * terminator)
  {
    std::size_t i = private_->movie_.getSelectedVideoTrack();
    if (private_->movie_.getVideoTracks().size() <= i)
    {
      return false;
    }

    VideoTrackPtr track = private_->movie_.getVideoTracks()[i];
    bool ok = track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = private_->readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::readAudio
  //
  bool
  ReaderFFMPEG::readAudio(TAudioFramePtr & frame, QueueWaitMgr * terminator)
  {
    std::size_t i = private_->movie_.getSelectedAudioTrack();
    if (private_->movie_.getAudioTracks().size() <= i)
    {
      return false;
    }

    AudioTrackPtr track = private_->movie_.getAudioTracks()[i];
    bool ok = track->getNextFrame(frame, terminator);
    if (ok && frame)
    {
      frame->readerId_ = private_->readerId_;
    }

    return ok;
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::threadStart
  //
  bool
  ReaderFFMPEG::threadStart()
  {
    return private_->movie_.threadStart();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::threadStop
  //
  bool
  ReaderFFMPEG::threadStop()
  {
    return private_->movie_.threadStop();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getPlaybackInterval
  //
  void
  ReaderFFMPEG::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    private_->movie_.getPlaybackInterval(timeIn, timeOut);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalStart
  //
  void
  ReaderFFMPEG::setPlaybackIntervalStart(double timeIn)
  {
    private_->movie_.setPlaybackIntervalStart(timeIn);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackIntervalEnd
  //
  void
  ReaderFFMPEG::setPlaybackIntervalEnd(double timeOut)
  {
    private_->movie_.setPlaybackIntervalEnd(timeOut);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackInterval
  //
  void
  ReaderFFMPEG::setPlaybackInterval(bool enabled)
  {
    private_->movie_.setPlaybackInterval(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setPlaybackLooping
  //
  void
  ReaderFFMPEG::setPlaybackLooping(bool enabled)
  {
    private_->movie_.setPlaybackLooping(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::skipLoopFilter
  //
  void
  ReaderFFMPEG::skipLoopFilter(bool skip)
  {
    private_->movie_.skipLoopFilter(skip);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::skipNonReferenceFrames
  //
  void
  ReaderFFMPEG::skipNonReferenceFrames(bool skip)
  {
    private_->movie_.skipNonReferenceFrames(skip);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setTempo
  //
  bool
  ReaderFFMPEG::setTempo(double tempo)
  {
    return private_->movie_.setTempo(tempo);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setDeinterlacing
  //
  bool
  ReaderFFMPEG::setDeinterlacing(bool enabled)
  {
    return private_->movie_.setDeinterlacing(enabled);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsCount
  //
  std::size_t
  ReaderFFMPEG::subsCount() const
  {
    return private_->movie_.subsCount();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsInfo
  //
  TSubsFormat
  ReaderFFMPEG::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    return private_->movie_.subsInfo(i, info);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::subsRender
  //
  void
  ReaderFFMPEG::subsRender(std::size_t i, bool render)
  {
    private_->movie_.subsRender(i, render);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::countChapters
  //
  std::size_t
  ReaderFFMPEG::countChapters() const
  {
    return private_->movie_.countChapters();
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::getChapterInfo
  //
  bool
  ReaderFFMPEG::getChapterInfo(std::size_t i, TChapter & c) const
  {
    return private_->movie_.getChapterInfo(i, c);
  }

  //----------------------------------------------------------------
  // ReaderFFMPEG::setReaderId
  //
  void
  ReaderFFMPEG::setReaderId(unsigned int readerId)
  {
    private_->readerId_ = readerId;
  }
}
