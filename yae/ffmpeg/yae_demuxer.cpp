// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard lib:
#include <algorithm>
#include <inttypes.h>
#include <iomanip>
#include <iterator>
#include <limits>
#include <stdio.h>

// yae includes:
#include "yae_demuxer.h"
#include "yae_pixel_format_ffmpeg.h"
#include "../utils/yae_utils.h"
#include "../video/yae_pixel_format_traits.h"

// ffmpeg:
extern "C"
{
#include <libavutil/avstring.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}

namespace yae
{

  //----------------------------------------------------------------
  // AvInputContextPtr::AvInputContextPtr
  //
  AvInputContextPtr::AvInputContextPtr(AVFormatContext * ctx):
    boost::shared_ptr<AVFormatContext>(ctx, &AvInputContextPtr::destroy)
  {}

  //----------------------------------------------------------------
  // AvInputContextPtr::destroy
  //
  void
  AvInputContextPtr::destroy(AVFormatContext * ctx)
  {
    if (!ctx)
    {
      return;
    }

    avformat_close_input(&ctx);
  }

  //----------------------------------------------------------------
  // AvOutputContextPtr::AvOutputContextPtr
  //
  AvOutputContextPtr::AvOutputContextPtr(AVFormatContext * ctx):
    boost::shared_ptr<AVFormatContext>(ctx, &AvOutputContextPtr::destroy)
  {}

  //----------------------------------------------------------------
  // AvOutputContextPtr::destroy
  //
  void
  AvOutputContextPtr::destroy(AVFormatContext * ctx)
  {
    if (!ctx)
    {
      return;
    }

    for (unsigned int i = 0; i < ctx->nb_streams; i++)
    {
      av_freep(&(ctx->streams[i]));
    }

    if (ctx->pb)
    {
      avio_close(ctx->pb);
    }

    av_freep(&ctx);
  }


  //----------------------------------------------------------------
  // Demuxer::Demuxer
  //
  Demuxer::Demuxer(std::size_t demuxer_index,
                   std::size_t track_offset):
    context_(NULL),
    ix_(demuxer_index),
    to_(track_offset),
    interruptDemuxer_(false)
  {
    ensure_ffmpeg_initialized();
  }

  //----------------------------------------------------------------
  // Demuxer::~Demuxer
  //
  Demuxer::~Demuxer()
  {
    close();
  }

  //----------------------------------------------------------------
  // Demuxer::requestMutex
  //
  void
  Demuxer::requestDemuxerInterrupt()
  {
    YAE_ASSERT(!interruptDemuxer_);
    interruptDemuxer_ = true;
  }

  //----------------------------------------------------------------
  // Demuxer::demuxerInterruptCallback
  //
  int
  Demuxer::demuxerInterruptCallback(void * context)
  {
    Demuxer * movie = (Demuxer *)context;
    if (movie->interruptDemuxer_)
    {
      return 1;
    }

    return 0;
  }

  //----------------------------------------------------------------
  // Demuxer::open
  //
  bool
  Demuxer::open(const char * resourcePath)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();

    YAE_ASSERT(!context_);
    YAE_ASSERT(!interruptDemuxer_);

    AVDictionary * options = NULL;

    // set probesize to 64 MiB:
    av_dict_set(&options, "probesize", "67108864", 0);

    // set analyze duration to 10 seconds:
    av_dict_set(&options, "analyzeduration", "10000000", 0);

    // set genpts:
    av_dict_set(&options, "fflags", "genpts", 0);

    AVFormatContext * ctx = NULL;
    int err = avformat_open_input(&ctx,
                                  resourcePath,
                                  NULL, // AVInputFormat to force
                                  &options);
    av_dict_free(&options);

    if (err != 0)
    {
      close();
      return false;
    }

    YAE_ASSERT(ctx->flags & AVFMT_FLAG_GENPTS);

    resourcePath_ = resourcePath;
    context_.reset(ctx);

    ctx->interrupt_callback.callback = &Demuxer::demuxerInterruptCallback;
    ctx->interrupt_callback.opaque = this;

    err = avformat_find_stream_info(ctx, NULL);
    if (err < 0)
    {
      close();
      return false;
    }

    // get the programs:
    for (unsigned int i = 0; i < ctx->nb_programs; i++)
    {
      const AVProgram * p = ctx->programs[i];
      programs_.push_back(TProgramInfo());
      TProgramInfo & info = programs_.back();
      info.id_ = p->id;
      info.program_ = p->program_num;
      info.pmt_pid_ = p->pmt_pid;
      info.pcr_pid_ = p->pcr_pid;

      getDictionary(info.metadata_, p->metadata);

      for (unsigned int j = 0; j < p->nb_stream_indexes; j++)
      {
        unsigned int streamIndex = p->stream_index[j];
        streamIndexToProgramIndex_[streamIndex] = i;
      }
    }

    if (ctx->nb_programs < 1)
    {
      // there must be at least 1 implied program:
      programs_.push_back(TProgramInfo());

      for (unsigned int i = 0; i < ctx->nb_streams; i++)
      {
        streamIndexToProgramIndex_[i] = 0;
      }
    }

    for (unsigned int i = 0; i < ctx->nb_streams; i++)
    {
      AVStream * stream = ctx->streams[i];
      trackId_[stream->index] = make_track_id('_', to_ + stream->index);

      // lookup which program this stream belongs to:
      TProgramInfo * program = NULL;
      {
        std::map<int, unsigned int>::const_iterator
          found = streamIndexToProgramIndex_.find(i);

        if (found != streamIndexToProgramIndex_.end())
        {
          program = &programs_[found->second];
        }
      }

      // extract attachments:
      if (stream->codecpar->codec_type == AVMEDIA_TYPE_ATTACHMENT)
      {
        attachments_.push_back(TAttachment(stream->codecpar->extradata,
                                           stream->codecpar->extradata_size));
        TAttachment & att = attachments_.back();
        getDictionary(att.metadata_, stream->metadata);
        continue;
      }

      // shortcut:
      const AVMediaType codecType = stream->codecpar->codec_type;

      // assume codec is unsupported,
      // discard all packets unless proven otherwise:
      stream->discard = AVDISCARD_ALL;

      // check whether we have a decoder for this codec:
      AVCodec * decoder = avcodec_find_decoder(stream->codecpar->codec_id);
      if (!decoder && codecType != AVMEDIA_TYPE_SUBTITLE)
      {
        // unsupported codec, ignore it:
        stream->codecpar->codec_type = AVMEDIA_TYPE_UNKNOWN;
        continue;
      }

      if (!program)
      {
        YAE_ASSERT(false);
        continue;
      }

      TrackPtr baseTrack(new Track(ctx, stream));

      if (codecType == AVMEDIA_TYPE_VIDEO)
      {
        VideoTrackPtr track(new VideoTrack(*baseTrack));
        VideoTraits traits;
        if (track->getTraits(traits) &&
            // avfilter does not support these pixel formats:
            traits.pixelFormat_ != kPixelFormatUYYVYY411)
        {
          program->video_.push_back(videoTracks_.size());
          stream->discard = AVDISCARD_DEFAULT;

          track->setId(make_track_id('v', to_ + videoTracks_.size()));
          streamIndex_[track->id()] = stream->index;
          trackId_[stream->index] = track->id();

          videoTracks_.push_back(track);
          tracks_[stream->index] = track;
        }
        else
        {
          // unsupported codec, ignore it:
          stream->codecpar->codec_type = AVMEDIA_TYPE_UNKNOWN;
        }
      }
      else if (codecType == AVMEDIA_TYPE_AUDIO)
      {
        AudioTrackPtr track(new AudioTrack(*baseTrack));
        AudioTraits traits;
        if (track->getTraits(traits))
        {
          program->audio_.push_back(audioTracks_.size());
          stream->discard = AVDISCARD_DEFAULT;

          track->setId(make_track_id('a', to_ + audioTracks_.size()));
          streamIndex_[track->id()] = stream->index;
          trackId_[stream->index] = track->id();

          audioTracks_.push_back(track);
          tracks_[stream->index] = track;
        }
        else
        {
          // unsupported codec, ignore it:
          stream->codecpar->codec_type = AVMEDIA_TYPE_UNKNOWN;
        }
      }
      else if (codecType == AVMEDIA_TYPE_SUBTITLE)
      {
        // avoid codec instance sharing between a temporary Track object
        // and SubtitlesTrack object:
        baseTrack = TrackPtr();

        if (stream->codecpar->codec_id != AV_CODEC_ID_NONE)
        {
          program->subs_.push_back(subttTracks_.size());
          stream->discard = AVDISCARD_DEFAULT;

          SubttTrackPtr track(new SubtitlesTrack(stream));
          track->setId(make_track_id('s', to_ + subttTracks_.size()));
          streamIndex_[track->id()] = stream->index;
          trackId_[stream->index] = track->id();

          subttTracks_.push_back(track);
          tracks_[stream->index] = track;
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // Demuxer::close
  //
  void
  Demuxer::close()
  {
    if (!context_)
    {
      return;
    }

    attachments_.clear();
    videoTracks_.clear();
    audioTracks_.clear();
    subttTracks_.clear();
    tracks_.clear();
    programs_.clear();
    streamIndexToProgramIndex_.clear();
    streamIndex_.clear();

    context_.reset();
  }

  //----------------------------------------------------------------
  // Demuxer::getTrack
  //
  TrackPtr
  Demuxer::getTrack(const std::string & trackId) const
  {
    std::map<std::string, int>::const_iterator
      found = streamIndex_.find(trackId);
    if (found == streamIndex_.end())
    {
      return TrackPtr();
    }

    return getTrack(found->second);
  }

  //----------------------------------------------------------------
  // Demuxer::getTrack
  //
  TrackPtr
  Demuxer::getTrack(int streamIndex) const
  {
    std::map<int, TrackPtr>::const_iterator found = tracks_.find(streamIndex);
    if (found == tracks_.end())
    {
      return TrackPtr();
    }

    return found->second;
  }

  //----------------------------------------------------------------
  // Demuxer::getProgram
  //
  const TProgramInfo *
  Demuxer::getProgram(int streamIndex) const
  {
    std::map<int, unsigned int>::const_iterator
      found = streamIndexToProgramIndex_.find(streamIndex);

    if (found == streamIndexToProgramIndex_.end())
    {
      return NULL;
    }

    const TProgramInfo * program = &programs_[found->second];
    return program;
  }

  //----------------------------------------------------------------
  // Demuxer::getVideoTrackInfo
  //
  void
  Demuxer::getVideoTrackInfo(std::size_t i, TTrackInfo & info) const
  {
    info.program_ = context_ ? context_->nb_programs : 0;
    info.ntracks_ = videoTracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      VideoTrackPtr t = videoTracks_[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
      info.program_ = yae::get(streamIndexToProgramIndex_, t->streamIndex());
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getAudioTrackInfo
  //
  void
  Demuxer::getAudioTrackInfo(std::size_t i, TTrackInfo & info) const
  {
    info.program_ = context_ ? context_->nb_programs : 0;
    info.ntracks_ = audioTracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      AudioTrackPtr t = audioTracks_[info.index_];
      info.setLang(t->getLang());
      info.setName(t->getName());
      info.program_ = yae::get(streamIndexToProgramIndex_, t->streamIndex());
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getSubttTrackInfo
  //
  TSubsFormat
  Demuxer::getSubttTrackInfo(std::size_t i, TTrackInfo & info) const
  {
    info.program_ = context_ ? context_->nb_programs : 0;
    info.ntracks_ = subttTracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      SubttTrackPtr t = subttTracks_[i];
      info.setLang(t->getLang());
      info.setName(t->getName());
      info.program_ = yae::get(streamIndexToProgramIndex_, t->streamIndex());
      return t->format_;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // Demuxer::demux
  //
  int
  Demuxer::demux(AvPkt & pkt)
  {
    AVPacket & packet = pkt.get();
    int err = av_read_frame(context_.get(), &packet);

    if (interruptDemuxer_)
    {
      interruptDemuxer_ = false;
    }

    if (err < 0)
    {
      if (err != AVERROR_EOF)
      {
        av_log(NULL, AV_LOG_ERROR,
               "av_read_frame(%s) error %i: \"%s\"\n",
               resourcePath_.c_str(), err, yae::av_strerr(err).c_str());
      }
    }
    else
    {
      TrackPtr track = yae::get(tracks_, packet.stream_index);
      if (track)
      {
        pkt.trackId_ = track->id();
      }
      else
      {
        pkt.trackId_ = make_track_id('_', to_ + packet.stream_index);
      }

      const TProgramInfo * info = getProgram(packet.stream_index);
      pkt.program_ = info ? info->id_ : 0;
      pkt.demuxer_ = this;
    }

    return err;
  }

  //----------------------------------------------------------------
  // Demuxer::isSeekable
  //
  bool
  Demuxer::isSeekable() const
  {
    const AVFormatContext * ctx = context_.get();

    if (!ctx || !ctx->pb || !ctx->pb->seekable)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Demuxer::seekTo
  //
  int
  Demuxer::seekTo(int seekFlags,
                  const TTime & seekTime,
                  const std::string & trackId)
  {
    AVFormatContext * ctx = context_.get();

    if (!ctx)
    {
      return AVERROR_UNKNOWN;
    }

    if (!isSeekable())
    {
      // don't bother attempting to seek an un-seekable stream:
      return AVERROR_EOF;
    }

    int streamIndex = -1;

    if (!trackId.empty())
    {
      streamIndex = yae::get(streamIndex_, trackId, -1);
      if (streamIndex == -1)
      {
        return AVERROR_STREAM_NOT_FOUND;
      }
    }

    int64_t ts = 0;

    if ((seekFlags & (AVSEEK_FLAG_BYTE | AVSEEK_FLAG_FRAME)) == 0)
    {
      if (streamIndex == -1)
      {
        ts = seekTime.get(AV_TIME_BASE);
      }
      else
      {
        AVRational tb;
        tb.num = 1;
        tb.den = seekTime.base_;

        const AVStream * s = ctx->streams[streamIndex];
        ts = av_rescale_q(seekTime.time_, tb, s->time_base);
      }
    }
    else
    {
      ts = seekTime.time_;
    }

    int err = avformat_seek_file(ctx,
                                 streamIndex,
                                 kMinInt64,
                                 ts,
                                 kMaxInt64,
                                 seekFlags);

    if (err < 0)
    {
      av_log(NULL, AV_LOG_WARNING,
             "avformat_seek_file(%" PRId64 ") error %i: \"%s\"\n",
             ts, err, yae::av_strerr(err).c_str());
    }

    return err;
  }

  //----------------------------------------------------------------
  // Demuxer::countChapters
  //
  std::size_t
  Demuxer::countChapters() const
  {
    return context_ ? context_->nb_chapters : 0;
  }

  //----------------------------------------------------------------
  // Demuxer::getChapterInfo
  //
  bool
  Demuxer::getChapterInfo(std::size_t i, TChapter & c) const
  {
    if (!context_ || i >= context_->nb_chapters)
    {
      return false;
    }

    std::ostringstream oss;
    oss << "Chapter " << i + 1;

    const AVChapter * av = context_->chapters[i];

    getDictionary(c.metadata_, av->metadata);
    c.name_ = yae::get(c.metadata_, "title", oss.str());

    c.span_.t0_.reset(av->time_base.num * av->start,
                      av->time_base.den);

    c.span_.t1_.reset(av->time_base.num * av->end,
                      av->time_base.den);

    return true;
  }

  //----------------------------------------------------------------
  // Demuxer::getChapters
  //
  void
  Demuxer::getChapters(std::map<TTime, TChapter> & chapters) const
  {
    const std::size_t num_chapters = countChapters();
    if (num_chapters < 1)
    {
      return;
    }

    for (std::size_t i = 0; i < num_chapters; i++)
    {
      TChapter c;
      getChapterInfo(i, c);
      chapters[c.span_.t0_] = c;
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getMetadata
  //
  void
  Demuxer::getMetadata(std::map<std::string, TDictionary> & track_meta,
                       TDictionary & metadata) const
  {
    const AVFormatContext & ctx = getFormatContext();
    getDictionary(metadata, ctx.metadata);

    for (unsigned int i = 0; i < ctx.nb_streams; i++)
    {
      const AVStream * stream = ctx.streams[i];
      std::string track_id = yae::get(trackId_, stream->index);

      if (track_id.empty())
      {
        YAE_ASSERT(false);
        track_id = make_track_id('_', to_ + stream->index);
      }

      getDictionary(track_meta[track_id], stream->metadata);
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getPrograms
  //
  void
  Demuxer::getPrograms(std::map<int, TProgramInfo> & programs) const
  {
    const std::size_t num_progs = programs_.size();
    for (std::size_t i = 0; i < num_progs; i++)
    {
      const TProgramInfo & program = programs_[i];
      programs[program.id_] = program;
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getTrackPrograms
  //
  void
  Demuxer::getTrackPrograms(std::map<std::string, int> & prog_lut) const
  {
    const AVFormatContext & ctx = getFormatContext();
    for (unsigned int i = 0; i < ctx.nb_streams; i++)
    {
      const AVStream * stream = ctx.streams[i];
      std::string track_id = yae::get(trackId_, stream->index);

      if (track_id.empty())
      {
        YAE_ASSERT(false);
        track_id = make_track_id('_', to_ + stream->index);
      }

      const TProgramInfo * prog = getProgram(stream->index);
      prog_lut[track_id] = prog ? prog->id_ : 0;
    }
  }

  //----------------------------------------------------------------
  // Demuxer::getDecoders
  //
  void
  Demuxer::getDecoders(std::map<std::string, TrackPtr> & decoders) const
  {
    for (std::map<std::string, int>::const_iterator i =
           streamIndex_.begin(); i != streamIndex_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const int & stream_index = i->second;
      TrackPtr decoder = yae::get(tracks_, stream_index);
      decoders[track_id] = decoder;
    }
  }


  //----------------------------------------------------------------
  // open_demuxer
  //
  TDemuxerPtr
  open_demuxer(const char * resourcePath, std::size_t track_offset)
  {
    YAE_ASSERT(!(track_offset % 100));
    TDemuxerPtr demuxer(new Demuxer(track_offset / 100, track_offset));

    std::string path(resourcePath);
    if (al::ends_with(path, ".eyetv"))
    {
      std::set<std::string> mpg_path;
      CollectMatchingFiles visitor(mpg_path, "^.+\\.mpg$");
      for_each_file_at(path, visitor);

      if (mpg_path.size() == 1)
      {
        path = *(mpg_path.begin());
      }
    }

    if (!demuxer->open(path.c_str()))
    {
      return TDemuxerPtr();
    }

    return demuxer;
  }

  //----------------------------------------------------------------
  // open_primary_and_aux_demuxers
  //
  bool
  open_primary_and_aux_demuxers(const std::string & filePath,
                                std::list<TDemuxerPtr> & src)
  {
    std::string folderPath;
    std::string fileName;
    parseFilePath(filePath, folderPath, fileName);

    std::string baseName;
    std::string ext;
    parseFileName(fileName, baseName, ext);

    if (!ext.empty())
    {
      baseName += '.';
    }

    src.push_back(open_demuxer(filePath.c_str()));
    if (!src.back())
    {
      // failed to open the primary resource:
      return false;
    }

    Demuxer & primary = *(src.back().get());
#ifndef NDEBUG
    av_log(NULL, AV_LOG_INFO,
           "file opened: %s, programs: %i, a: %i, v: %i, s: %i\n",
           filePath.c_str(),
           int(primary.programs().size()),
           int(primary.audioTracks().size()),
           int(primary.videoTracks().size()),
           int(primary.subttTracks().size()));
#endif

    if (primary.programs().size() < 2)
    {
      // add auxiliary resources:
      std::size_t trackOffset = 100;
      TOpenFolder folder(folderPath);
      while (folder.parseNextItem())
      {
        std::string nm = folder.itemName();
        if (nm == fileName || !al::starts_with(nm, baseName))
        {
          continue;
        }

        src.push_back(open_demuxer(folder.itemPath().c_str(),
                                   trackOffset));
        if (!src.back())
        {
          src.pop_back();
          continue;
        }

        Demuxer & aux = *(src.back().get());
        if (!(primary.videoTracks().empty() || aux.videoTracks().empty()))
        {
          // if it has a video track -- it's probably not auxiliary:
#ifndef NDEBUG
          av_log(NULL, AV_LOG_WARNING,
                 "skipping auxiliary video \"%s\"\n",
                 aux.resourcePath().c_str());
#endif

          src.pop_back();
          continue;
        }

#ifndef NDEBUG
        av_log(NULL, AV_LOG_INFO,
               "file opened: %s, programs: %i, a: %i, v: %i, s: %i\n",
               nm.c_str(),
               int(aux.programs().size()),
               int(aux.audioTracks().size()),
               int(aux.videoTracks().size()),
               int(aux.subttTracks().size()));
#endif
        trackOffset += 100;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // get_dts
  //
  bool
  get_dts(TTime & dts, const AVStream * stream, const AVPacket & packet)
  {
    if (packet.dts != AV_NOPTS_VALUE)
    {
      dts = TTime(stream->time_base.num * packet.dts,
                  stream->time_base.den);
      return true;
    }
#if 0
    else if (packet.pts != AV_NOPTS_VALUE)
    {
      dts = TTime(stream->time_base.num * packet.pts,
                  stream->time_base.den);
    }
#endif
    return false;
  }

  //----------------------------------------------------------------
  // get_pts
  //
  bool
  get_pts(TTime & pts, const AVStream * stream, const AVPacket & packet)
  {
    if (packet.pts != AV_NOPTS_VALUE)
    {
      // check that PTS is not less than DTS, and that PTS and DTS
      // are reasonably close together:
      int64_t diff =
        packet.dts != AV_NOPTS_VALUE ?
        stream->time_base.num * (packet.pts - packet.dts) /
        stream->time_base.den :
        0;

      if (diff >= 0 && diff < 10)
      {
        pts = TTime(stream->time_base.num * packet.pts,
                    stream->time_base.den);
        return true;
      }
    }

    return false;
  }


  //----------------------------------------------------------------
  // ProgramBuffer::ProgramBuffer
  //
  ProgramBuffer::ProgramBuffer():
    num_packets_(0),
    t0_(TTime::max_flicks()),
    t1_(TTime::min_flicks())
  {}

  //----------------------------------------------------------------
  // ProgramBuffer::clear
  //
  void
  ProgramBuffer::clear()
  {
    packets_.clear();
    num_packets_ = 0;
    t0_ = TTime::max_flicks();
    t1_ = TTime::min_flicks();
    next_dts_.clear();
  }

  //----------------------------------------------------------------
  // append
  //
  static bool
  append(std::list<TPacketPtr> & packets, const TPacketPtr & packet_ptr)
  {
    const AVPacket & packet = packet_ptr->get();

    if (packets.empty() || packets.back()->get().dts <= packet.dts)
    {
      packets.push_back(packet_ptr);
      return true;
    }

    return false;
  }

#if 0
  //----------------------------------------------------------------
  // insert
  //
  static void
  insert(std::list<TPacketPtr> & packets, const TPacketPtr & packet_ptr)
  {
    int64 packet_dts = packet_ptr->dts;

    if (packets.empty() || packets.back()->get().dts <= packet_dts)
    {
      packets.push_back(packet_ptr);
      return;
    }

    for (std::list<TPacketPtr>::reverse_iterator
           i = packets.rbegin(); i != packets.rend(); ++i)
    {
      const AVPacket & packet = (*i)->get();
      if (packet.dts <= packet_dts)
      {
        packets.insert(i.base(), packet_ptr);
        return;
      }
    }

    packets.push_front(packet_ptr);
  }
#endif

  //----------------------------------------------------------------
  // ProgramBuffer::push
  //
  void
  ProgramBuffer::push(const TPacketPtr & packet_ptr, const AVStream * stream)
  {
    if (!(packet_ptr && stream))
    {
      YAE_ASSERT(false);
      return;
    }

    // shortcut:
    AvPkt & pkt = *packet_ptr;
    AVPacket & packet = pkt.get();

    TTime delay;
    if (stream->avg_frame_rate.num &&
        stream->avg_frame_rate.den &&
        stream->codecpar->video_delay)
    {
      delay.reset(stream->avg_frame_rate.den * stream->codecpar->video_delay,
                  stream->avg_frame_rate.num);
    }

    TTime pts;
    get_pts(pts, stream, packet);

    TTime next_dts = yae::get(next_dts_, packet.stream_index, pts - delay);
    next_dts.time_ = (av_rescale_q(next_dts.time_,
                                   Rational(1, next_dts.base_),
                                   stream->time_base) *
                      stream->time_base.num);
    next_dts.base_ = stream->time_base.den;

    TTime dts = next_dts;
    bool has_dts = get_dts(dts, stream, packet);

    if (has_dts && dts < next_dts)
    {
      int64 cts = (packet.pts != AV_NOPTS_VALUE) ? packet.pts - packet.dts : 0;

      dts = next_dts;
      packet.dts = av_rescale_q(dts.time_,
                                Rational(1, dts.base_),
                                stream->time_base);
      if (packet.pts != AV_NOPTS_VALUE)
      {
        packet.pts = packet.dts + cts;
      }
    }
    else if (!has_dts)
    {
      dts = next_dts;
      packet.dts = av_rescale_q(dts.time_,
                                Rational(1, dts.base_),
                                stream->time_base);
    }

    std::list<TPacketPtr> & packets = packets_[packet.stream_index];
    bool ok = append(packets, packet_ptr);
    YAE_ASSERT(ok);
    if (!ok)
    {
      return;
    }

    num_packets_++;

    t0_ = std::min<TTime>(t0_, dts);
    t1_ = std::max<TTime>(t1_, dts);

    TTime dur(stream->time_base.num * packet.duration,
              stream->time_base.den);
    next_dts_[packet.stream_index] = dts + dur;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::choose
  //
  int
  ProgramBuffer::choose(const AVFormatContext & ctx, TTime & ts_min) const
  {
    // shortcut:
    unsigned int stream_index = ctx.nb_streams;

    for (TPackets::const_iterator
           i = packets_.begin(); i != packets_.end(); ++i)
    {
      const std::list<TPacketPtr> & pkts = i->second;
      if (pkts.empty())
      {
        continue;
      }

      const AvPkt & pkt = *(pkts.front());
      const AVPacket & packet = pkt.get();
      const AVStream * stream = ctx.streams[packet.stream_index];
      if (!stream)
      {
        YAE_ASSERT(false);
        continue;
      }

      TTime ts;
      if (!get_dts(ts, stream, packet) &&
          !get_pts(ts, stream, packet))
      {
        YAE_ASSERT(false);
        return packet.stream_index;
      }

      if (ts < ts_min)
      {
        ts_min = ts;
        stream_index = packet.stream_index;
      }
    }

    return stream_index;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::peek
  //
  // lookup next packet and its DTS:
  //
  TPacketPtr
  ProgramBuffer::peek(const AVFormatContext & ctx,
                      TTime & dts_min,
                      int stream_index) const
  {
    if (stream_index < 0)
    {
      stream_index = choose(ctx, dts_min);
    }

    if (((unsigned int)stream_index) >= ctx.nb_streams)
    {
      return TPacketPtr();
    }

    TPackets::const_iterator found = packets_.find(stream_index);
    if (found == packets_.end())
    {
      YAE_ASSERT(false);
      return TPacketPtr();
    }

    const std::list<TPacketPtr> & pkts = found->second;
    TPacketPtr packet_ptr = pkts.front();
    return packet_ptr;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::get
  //
  // remove next packet, pass back its AVStream
  //
  TPacketPtr
  ProgramBuffer::get(const AVFormatContext & ctx,
                     AVStream *& src,
                     int stream_index)
  {
    TTime dts = TTime::max_flicks();
    TPacketPtr packet_ptr = peek(ctx, dts, stream_index);

    if (packet_ptr)
    {
      const AVPacket & packet = packet_ptr->get();
      unsigned int stream_index = packet.stream_index;
      src = ctx.streams[stream_index];

      std::list<TPacketPtr> & pkts = packets_[stream_index];
      pkts.pop_front();

      YAE_ASSERT(num_packets_ > 0);
      num_packets_--;

      if (!num_packets_)
      {
        // reset the timespan:
        t0_ = TTime::max_flicks();
        t1_ = TTime::min_flicks();
      }
    }

    return packet_ptr;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::pop
  //
  bool
  ProgramBuffer::pop(const TPacketPtr & packet_ptr)
  {
    if (!packet_ptr)
    {
      return true;
    }

    const AVPacket & packet = packet_ptr->get();
    TPackets::iterator found = packets_.find(packet.stream_index);
    if (found != packets_.end())
    {
      std::list<TPacketPtr> & pkts = found->second;

      // NOTE: packet_ptr being popped may be a clone created by
      // the serial demuxer or trimmed demuxer, so we can't compare
      // pkts.front() and packet_ptr directly...
      //
      if (!pkts.empty() /* && pkts.front() == packet_ptr */)
      {
        pkts.pop_front();

        YAE_ASSERT(num_packets_ > 0);
        num_packets_--;

        if (!num_packets_)
        {
          // reset the timespan:
          t0_ = TTime::max_flicks();
          t1_ = TTime::min_flicks();
        }

        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::update_duration
  //
  void
  ProgramBuffer::update_duration(const AVFormatContext & ctx)
  {
    TTime dts_min = TTime::max_flicks();
    TPacketPtr next = peek(ctx, dts_min);
    if (next)
    {
      const AVPacket & packet = next->get();
      const AVStream * stream = ctx.streams[packet.stream_index];

      // adjust t0:
      bool ok = get_dts(t0_, stream, packet) || get_pts(t0_, stream, packet);
      YAE_ASSERT(ok);
    }
  }

  //----------------------------------------------------------------
  // ProgramBuffer::avg_track_duration
  //
  double
  ProgramBuffer::avg_track_duration(const AVFormatContext & ctx) const
  {
    std::size_t num = 0;
    double sum = 0.0;

    for (TPackets::const_iterator
           i = packets_.begin(); i != packets_.end(); ++i)
    {
      const int & stream_index = i->first;
      const AVStream * stream = ctx.streams[stream_index];
      if (!stream)
      {
        YAE_ASSERT(false);
        continue;
      }

      if (stream->discard == AVDISCARD_ALL)
      {
        // ignore it:
        continue;
      }

      num++;

      const std::list<TPacketPtr> & pkts = i->second;
      if (pkts.empty())
      {
        continue;
      }

      const AVPacket & head = pkts.front()->get();
      const AVPacket & tail = pkts.back()->get();

      TTime t0;
      bool ok = get_dts(t0, stream, head) || get_pts(t0, stream, head);
      YAE_ASSERT(ok);

      if (ok)
      {
        TTime t1;
        ok = get_dts(t1, stream, tail) || get_pts(t1, stream, tail);
        YAE_ASSERT(ok);

        if (ok)
        {
          double dt = (t1 - t0).sec();
          sum += dt;
        }
      }
    }

    double avg = num ? sum / double(num) : 0.0;
    return avg;
  }


  //----------------------------------------------------------------
  // PacketBuffer::PacketBuffer
  //
  PacketBuffer::PacketBuffer(const TDemuxerPtr & demuxer, double buffer_sec):
    demuxer_(demuxer),
    buffer_sec_(buffer_sec),
    gave_up_(false)
  {
    const AVFormatContext & ctx = demuxer_->getFormatContext();
    for (unsigned int i = 0; i < ctx.nb_programs; i++)
    {
      TProgramBufferPtr buffer(new ProgramBuffer());

      const AVProgram * p = ctx.programs[i];
      if (!p)
      {
        YAE_ASSERT(false);
        continue;
      }

      program_[p->id] = buffer;

      for (unsigned int j = 0; j < p->nb_stream_indexes; j++)
      {
        unsigned int stream_index = p->stream_index[j];
        stream_[stream_index] = buffer;
      }
    }

    if (!ctx.nb_programs)
    {
      // since there are no explicitly defined programs,
      // then all streams implicitly belong to one program:
      TProgramBufferPtr buffer(new ProgramBuffer());
      program_[0] = buffer;

      for (unsigned int i = 0; i < ctx.nb_streams; i++)
      {
        const AVStream * s = ctx.streams[i];
        if (!s)
        {
          YAE_ASSERT(false);
          continue;
        }

        stream_[s->index] = buffer;
      }
    }
  }

  //----------------------------------------------------------------
  // PacketBuffer::get_metadata
  //
  void
  PacketBuffer::get_metadata(std::map<std::string, TDictionary> & track_meta,
                             TDictionary & metadata) const
  {
    const Demuxer & demuxer = *demuxer_;
    demuxer.getMetadata(track_meta, metadata);
  }

  //----------------------------------------------------------------
  // PacketBuffer::get_decoders
  //
  void
  PacketBuffer::get_decoders(std::map<std::string, TrackPtr> & decoders) const
  {
    const Demuxer & demuxer = *demuxer_;
    demuxer.getDecoders(decoders);
  }

  //----------------------------------------------------------------
  // PacketBuffer::get_programs
  //
  void
  PacketBuffer::get_programs(std::map<int, TProgramInfo> & programs) const
  {
    const Demuxer & demuxer = *demuxer_;
    demuxer.getPrograms(programs);
  }

  //----------------------------------------------------------------
  // PacketBuffer::get_trk_prog
  //
  void
  PacketBuffer::get_trk_prog(std::map<std::string, int> & lut) const
  {
    const Demuxer & demuxer = *demuxer_;
    demuxer.getTrackPrograms(lut);
  }

  //----------------------------------------------------------------
  // PacketBuffer::get_chapters
  //
  void
  PacketBuffer::get_chapters(std::map<TTime, TChapter> & chapters) const
  {
    const Demuxer & demuxer = *demuxer_;
    demuxer.getChapters(chapters);
  }

  //----------------------------------------------------------------
  // PacketBuffer::seek
  //
  int
  PacketBuffer::seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                     const TTime & seekTime,
                     const std::string & trackId)
  {
    // shortcuts:
    Demuxer & demuxer = *demuxer_;

    clear();

    int err =
      (trackId.empty() || demuxer.has(trackId)) ?
      demuxer.seekTo(seekFlags, seekTime, trackId) :
      demuxer.seekTo(seekFlags, seekTime);

    return err;
  }

  //----------------------------------------------------------------
  // PacketBuffer::clear
  //
  void
  PacketBuffer::clear()
  {
    for (std::map<int, TProgramBufferPtr>::iterator
           i = program_.begin(); i != program_.end(); ++i)
    {
      ProgramBuffer & buffer = *(i->second);
      buffer.clear();
    }

    gave_up_ = false;
  }

  //----------------------------------------------------------------
  // PacketBuffer::populate
  //
  int
  PacketBuffer::populate()
  {
    // shortcuts:
    Demuxer & demuxer = *demuxer_;
    const AVFormatContext & ctx = demuxer.getFormatContext();

    while (true)
    {
      // find min/max buffer duration across all programs:
      double min_duration = std::numeric_limits<double>::max();
      double max_duration = 0.0;
      int min_id = -1;
      int max_id = -1;

      for (std::map<int, TProgramBufferPtr>::const_iterator
             i = program_.begin(); i != program_.end(); ++i)
      {
        const ProgramBuffer & buffer = *(i->second);
        // double duration = buffer.duration();
        double duration = buffer.avg_track_duration(ctx);

        if (duration < min_duration)
        {
          min_duration = duration;
          min_id = i->first;
        }

        if (duration > max_duration)
        {
          max_duration = duration;
          max_id = i->first;
        }
      }

      if (min_duration > buffer_sec_)
      {
        // buffer is sufficiently filled:
        gave_up_ = false;
        break;
      }

      if (max_duration > buffer_sec_ * 10.0)
      {
        if (!gave_up_)
        {
          gave_up_ = true;
          av_log(NULL, AV_LOG_WARNING,
                 "%03i min buffer duration: %.3f\n"
                 "%03i max buffer duration: %.3f, gave up\n",
                 min_id, min_duration,
                 max_id, max_duration);
        }

        break;
      }

      TPacketPtr packet_ptr(new AvPkt());
      AvPkt & pkt = *packet_ptr;
      pkt.pbuffer_ = this;

      AVPacket & packet = pkt.get();
      int err = demuxer.demux(pkt);
      if (err == AVERROR_EOF)
      {
        return err;
      }
      else if (err < 0)
      {
        // we may be able to recover if we keep demuxing:
        continue;
      }

      const AVStream * stream = ctx.streams[packet.stream_index];
      if (!stream)
      {
        YAE_ASSERT(false);
        continue;
      }

      TProgramBufferPtr program = yae::get(stream_, packet.stream_index);
      if (!program)
      {
        YAE_ASSERT(false);
        continue;
      }

      YAE_ASSERT(!pkt.trackId_.empty());
      program->push(packet_ptr, stream);
#if 0
      TTime dts(packet.dts * stream->time_base.num, stream->time_base.den);
      TTime pts(packet.pts * stream->time_base.num, stream->time_base.den);
      TTime dur(stream->time_base.num * packet.duration,
                stream->time_base.den);
      bool keyframe =
        al::starts_with(pkt.trackId_, "v:") &&
        (packet.flags & AV_PKT_FLAG_KEY);
      std::cout
        << "PacketBuffer::populate: " << pkt.trackId_
        << ", dts: " << Timespan(dts, dts + dur)
        << ", pts: " << Timespan(pts, pts + dur)
        << (keyframe ? ", keyframe" : "")
        << std::endl;
#endif
    }

    return 0;
  }

  //----------------------------------------------------------------
  // PacketBuffer::choose
  //
  TProgramBufferPtr
  PacketBuffer::choose(TTime & dts_min, int & stream_index) const
  {
    // shortcut:
    const AVFormatContext & ctx = demuxer_->getFormatContext();

    TProgramBufferPtr max_buffer;
    double max_duration = 0;

    for (std::map<int, TProgramBufferPtr>::const_iterator
           i = program_.begin(); i != program_.end(); ++i)
    {
      const TProgramBufferPtr & buffer = i->second;
      // double duration = buffer->duration();
      double duration = buffer->avg_track_duration(ctx);

      if (max_duration < duration)
      {
        max_duration = duration;
        max_buffer = buffer;
      }
    }

    if (max_buffer)
    {
      stream_index = max_buffer->choose(ctx, dts_min);
    }

    return max_buffer;
  }

  //----------------------------------------------------------------
  // PacketBuffer::peek
  //
  // lookup next packet and its DTS:
  //
  TPacketPtr
  PacketBuffer::peek(TTime & dts_min,
                    TProgramBufferPtr buffer,
                    int stream_index) const
  {
    if (!buffer || stream_index < 0)
    {
      buffer = choose(dts_min, stream_index);

      if (!buffer)
      {
        return TPacketPtr();
      }
    }

    const AVFormatContext & ctx = demuxer_->getFormatContext();
    TPacketPtr packet_ptr = buffer->peek(ctx, dts_min, stream_index);
    return packet_ptr;
  }

  //----------------------------------------------------------------
  // PacketBuffer::get
  //
  // remove next packet, pass back its AVStream
  //
  TPacketPtr
  PacketBuffer::get(AVStream *& src,
                    TProgramBufferPtr buffer,
                    int stream_index)
  {
    // refill the buffer:
    populate();

    if (!buffer || stream_index < 0)
    {
      TTime dts_min = TTime::max_flicks();
      buffer = choose(dts_min, stream_index);

      if (!buffer)
      {
        return TPacketPtr();
      }
    }

    const AVFormatContext & ctx = demuxer_->getFormatContext();
    TPacketPtr packet_ptr = buffer->get(ctx, src, stream_index);

    if (packet_ptr)
    {
      // refill the buffer:
      populate();
      buffer->update_duration(ctx);
    }

    return packet_ptr;
  }

  //----------------------------------------------------------------
  // PacketBuffer::pop
  //
  bool
  PacketBuffer::pop(const TPacketPtr & packet_ptr)
  {
    if (!packet_ptr)
    {
      return true;
    }

    const AVPacket & packet = packet_ptr->get();
    TProgramBufferPtr buffer = yae::get(stream_, packet.stream_index);
    if (buffer)
    {
      return buffer->pop(packet_ptr);
    }

    YAE_ASSERT(buffer);
    return false;
  }

  //----------------------------------------------------------------
  // PacketBuffer::stream
  //
  AVStream *
  PacketBuffer::stream(const TPacketPtr & packet_ptr) const
  {
    if (!packet_ptr)
    {
      return NULL;
    }

    const AVPacket & packet = packet_ptr->get();
    int stream_index = packet.stream_index;
    return stream(stream_index);
  }

  //----------------------------------------------------------------
  // PacketBuffer::stream
  //
  AVStream *
  PacketBuffer::stream(int stream_index) const
  {
    const AVFormatContext & ctx = demuxer_->getFormatContext();

    AVStream * s =
      (((unsigned int)stream_index) < ctx.nb_streams) ?
      ctx.streams[stream_index] :
      NULL;

    return s;
  }


  //----------------------------------------------------------------
  // DemuxerSummary::extend
  //
  void
  DemuxerSummary::extend(const DemuxerSummary & s,
                         const std::map<int, TTime> & prog_offset,
                         double tolerance)
  {
    yae::extend(metadata_, s.metadata_);

    for (std::map<std::string, TDictionary>::const_iterator
           i = s.trk_meta_.begin(); i != s.trk_meta_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TDictionary & metadata = i->second;

      if (metadata.empty())
      {
        continue;
      }

      yae::extend(trk_meta_[track_id], metadata);
    }

    for (std::map<std::string, const AVStream *>::const_iterator
           i = s.streams_.begin(); i != s.streams_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const AVStream * stream = i->second;
      if (yae::get(streams_, track_id))
      {
        continue;
      }

      streams_[track_id] = stream;
    }

    for (std::map<std::string, TrackPtr>::const_iterator
           i = s.decoders_.begin(); i != s.decoders_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TrackPtr & decoder = i->second;

      TrackPtr found = yae::get(decoders_, track_id);
      if (found)
      {
        YAE_ASSERT(same_codec(found, decoder));
        continue;
      }

      decoders_[track_id] = decoder;
    }

    for (std::map<int, TProgramInfo>::const_iterator
           i = s.programs_.begin(); i != s.programs_.end(); ++i)
    {
      const int & prog_id = i->first;
      const TProgramInfo & info = i->second;
      if (yae::has(programs_, prog_id))
      {
        continue;
      }

      programs_[prog_id] = info;
    }

    for (std::map<std::string, int>::const_iterator
           i = s.trk_prog_.begin(); i != s.trk_prog_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const int & prog_id = i->second;

      std::map<std::string, int>::iterator found = trk_prog_.find(track_id);
      if (found == trk_prog_.end())
      {
        trk_prog_[track_id] = prog_id;
        continue;
      }

      const int & prev_id = found->second;
      if (prev_id != prog_id)
      {
        std::ostringstream oss;
        oss << "track " << track_id << ", program id " << prog_id
            << " does not match program id " << prev_id;
        throw std::runtime_error(oss.str().c_str());
      }
    }

    for (std::map<std::string, FramerateEstimator>::const_iterator
           i = s.fps_.begin(); i != s.fps_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const FramerateEstimator & src = i->second;
      int prog_id = prog_offset.empty() ? 0 : s.find_program(track_id);
      TTime offset = -yae::get(prog_offset, prog_id, TTime(0, 1));
      fps_[track_id].add(src, offset);
    }

    for (std::map<int, Timeline>::const_iterator
           i = s.timeline_.begin(); i != s.timeline_.end(); ++i)
    {
      const int & prog_id = i->first;
      const Timeline & timeline = i->second;
      TTime offset = -yae::get(prog_offset, prog_id, TTime(0, 1));
      timeline_[prog_id].extend(timeline, offset, tolerance);
    }

    if (!s.chapters_.empty())
    {
      // chapters aren't stored per-program, so use any available program id.,
      // but there should only be 1:
      YAE_ASSERT(prog_offset.size() < 2);

      int prog_id = prog_offset.empty() ? 0 : prog_offset.begin()->first;
      TTime offset = -yae::get(prog_offset, prog_id, TTime(0, 1));

      for (std::map<TTime, TChapter>::const_iterator j =
             s.chapters_.begin(); j != s.chapters_.end(); ++j)
      {
        TTime t0 = j->first;
        TChapter ch = j->second;
        YAE_ASSERT(t0 == ch.span_.t0_);

        t0 += offset;
        ch.span_ += offset;
        YAE_ASSERT(t0 == ch.span_.t0_);

        chapters_[t0] = ch;
      }
    }

    if (rewind_.first.empty())
    {
      rewind_.first = s.rewind_.first;
      rewind_.second = TTime(0, s.rewind_.second.base_);
    }
  }

  //----------------------------------------------------------------
  // DemuxerSummary::replace_missing_durations
  //
  bool
  DemuxerSummary::replace_missing_durations()
  {
    bool replaced = false;

    for (std::map<int, Timeline>::iterator
           i = timeline_.begin(); i != timeline_.end(); ++i)
    {
      Timeline & t = i->second;
      for (std::map<std::string, Timeline::Track>::iterator
             j = t.tracks_.begin(); j != t.tracks_.end(); ++j)
      {
        Timeline::Track & tt = j->second;
        if (tt.dts_.size() < 2)
        {
          continue;
        }

        FramerateEstimator estimator;
        estimator.push(tt.dts_[0]);

        for (size_t k = 1, end = tt.dts_.size(); k < end; k++)
        {
          const TTime & t0 = tt.dts_[k - 1];
          TTime & dur = tt.dur_[k - 1];

          if (!dur.time_)
          {
            const TTime & t1 = tt.dts_[k];
            dur = t1 - t0;
            replaced = (dur.time_ != 0);
          }

          estimator.push(estimator.dts().back() + dur);
        }

        if (!tt.dur_.back().time_)
        {
          FramerateEstimator::Framerate stats;
          double fps = estimator.get(stats);

          const TTime & head = tt.dur_.front();
          TTime & tail = tt.dur_.back();
          tail = frameDurationForFrameRate(fps).rebased(head.base_);
          replaced = (tail.time_ != 0);
        }
      }
    }

    return replaced;
  }

  //----------------------------------------------------------------
  // DemuxerSummary::find_program
  //
  int
  DemuxerSummary::find_program(const std::string & trackId) const
  {
    std::map<std::string, int>::const_iterator found = trk_prog_.find(trackId);

    if (found == trk_prog_.end())
    {
      throw std::out_of_range(trackId);
    }

    return found->second;
  }

  //----------------------------------------------------------------
  // DemuxerSummary::get_track_timeline
  //
  const Timeline::Track &
  DemuxerSummary::get_track_timeline(const std::string & trackId) const
  {
    int program = find_program(trackId);
    const Timeline & timeline = yae::at(timeline_, program);
    const Timeline::Track & t = yae::at(timeline.tracks_, trackId);
    return t;
  }


  //----------------------------------------------------------------
  // operator <<
  //
  std::ostream &
  operator << (std::ostream & oss, const DemuxerSummary & summary)
  {
    if (!summary.metadata_.empty())
    {
      oss << "global metadata: " << summary.metadata_.size()
          << '\n' << summary.metadata_ << std::endl;
    }

    for (std::map<std::string, TDictionary>::const_iterator
           i = summary.trk_meta_.begin(); i != summary.trk_meta_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TDictionary & metadata = i->second;
      if (metadata.empty())
      {
        continue;
      }

      oss << track_id << " metadata: " << summary.metadata_.size()
          << '\n' << metadata << std::endl;
    }

    for (std::map<TTime, TChapter>::const_iterator j =
           summary.chapters_.begin(); j != summary.chapters_.end(); ++j)
    {
      const TTime & t0 = j->first;
      const TChapter & ch = j->second;
      YAE_ASSERT(t0 == ch.span_.t0_);

      oss << ch.span_ << ": " << ch.name_;

      if (!ch.metadata_.empty())
      {
        oss << ", metadata: " << ch.metadata_.size()
            << '\n' << ch.metadata_ << std::endl;
      }

      oss << std::endl;
    }

    for (std::map<int, Timeline>::const_iterator
           i = summary.timeline_.begin(); i != summary.timeline_.end(); ++i)
    {
      // shortcuts:
      const int & prog_id = i->first;
      const Timeline & timeline = i->second;

      const TProgramInfo & info = yae::at(summary.programs_, prog_id);
      std::string service_name = yae::get(info.metadata_, "service_name");
      if (!service_name.empty())
      {
        oss << service_name << "\n";
      }

      oss << "program " << std::setw(3) << prog_id << ", " << timeline
          << std::endl;
    }

    oss << std::endl;

    for (std::map<std::string, FramerateEstimator>::const_iterator
           i = summary.fps_.begin(); i != summary.fps_.end(); ++i)
    {
      const std::string & track_id = i->first;
      if (!al::starts_with(track_id, "v:"))
      {
        continue;
      }

      const FramerateEstimator & estimator = i->second;
      oss << i->first << "\n"
          << estimator
          << std::endl;
    }

    oss << "rewind: " << summary.rewind_.first
        << " to " << summary.rewind_.second
        << std::endl;

    return oss;
  }


  //----------------------------------------------------------------
  // DemuxerInterface::pop
  //
  bool
  DemuxerInterface::pop(const TPacketPtr & packet_ptr)
  {
    if (!packet_ptr)
    {
      return true;
    }

    const AvPkt & pkt = *packet_ptr;
    PacketBuffer * buffer = pkt.pbuffer_;
    YAE_ASSERT(buffer);

    if (!buffer)
    {
      return false;
    }

    return buffer->pop(packet_ptr);
  }

  //----------------------------------------------------------------
  // DemuxerInterface::get
  //
  TPacketPtr
  DemuxerInterface::get(AVStream *& src)
  {
    // refill the buffer, otherwise peek won't work:
    this->populate();

    TPacketPtr pkt = this->peek(src);
    bool removed = this->pop(pkt);
    YAE_ASSERT(removed);

    return pkt;
  }


  //----------------------------------------------------------------
  // DemuxerBuffer::DemuxerBuffer
  //
  DemuxerBuffer::DemuxerBuffer(const TDemuxerPtr & src, double buffer_sec):
    src_(src, buffer_sec)
  {}

  //----------------------------------------------------------------
  // DemuxerBuffer::populate
  //
  void
  DemuxerBuffer::populate()
  {
    src_.populate();
  }

  //----------------------------------------------------------------
  // DemuxerBuffer::seek
  //
  int
  DemuxerBuffer::seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                      const TTime & dts,
                      const std::string & trackId)
  {
    TTime seekTime = dts;

    if (!trackId.empty())
    {
      // if the input format implements seeking by PTS,
      // then convert given seekTime (referencing DTS timeline) to PTS,
      const AVFormatContext & context = src_.context();

      if ((context.iformat->flags & AVFMT_SEEK_TO_PTS) == AVFMT_SEEK_TO_PTS)
      {
        const DemuxerSummary & summary = this->summary();
        const Timeline::Track & track = summary.get_track_timeline(trackId);

        std::size_t sample_index = 0;
        if (track.find_sample_by_dts(dts, sample_index))
        {
          seekTime = track.pts_[sample_index];
        }
      }
    }

    return src_.seek(seekFlags, seekTime, trackId);
  }

  //----------------------------------------------------------------
  // DemuxerBuffer::peek
  //
  TPacketPtr
  DemuxerBuffer::peek(AVStream *& src) const
  {
    TTime dts = TTime::max_flicks();
    int stream_index = -1;

    TProgramBufferPtr program = src_.choose(dts, stream_index);
    if (program)
    {
      src = src_.stream(stream_index);
      YAE_ASSERT(src);

      if (src)
      {
        const ProgramBuffer::TPackets & packets = program->packets();

        ProgramBuffer::TPackets::const_iterator found =
          packets.find(stream_index);
        YAE_ASSERT(found != packets.end());

        if (found != packets.end())
        {
          const std::list<TPacketPtr> & stream = found->second;
          YAE_ASSERT(!stream.empty());

          if (!stream.empty())
          {
            TPacketPtr packet_ptr = found->second.front();
#if 0
            const AvPkt & pkt = *packet_ptr;
            const AVPacket & packet = pkt.get();
            TTime dts(packet.dts * src->time_base.num, src->time_base.den);
            TTime pts(packet.pts * src->time_base.num, src->time_base.den);
            TTime dur(packet.duration * src->time_base.num, src->time_base.den);
            bool keyframe = al::starts_with(pkt.trackId_, "v:") && (packet.flags & AV_PKT_FLAG_KEY);
            std::cout
              << "DemuxerBuffer::peek: " << pkt.trackId_
              << ", dts: " << Timespan(dts, dts + dur)
              << ", pts: " << Timespan(pts, pts + dur)
              << (keyframe ? ", keyframe" : "")
              << std::endl;
#endif
            return packet_ptr;
          }
        }
      }
    }

    return TPacketPtr();
  }


  //----------------------------------------------------------------
  // get_rewind_info
  //
  static bool
  get_rewind_info(DemuxerInterface & demuxer,
                  std::string & track_id,
                  TTime & dts)
  {
    // if the buffer is not populated 'peek' won't work:
    demuxer.populate();

    // get current time position:
    TTime saved_pos(kMinInt64, 1);
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.peek(src);
      if (src && packet_ptr)
      {
        const AVPacket & packet = packet_ptr->get();
        if (!get_dts(saved_pos, src, packet) &&
            !get_pts(saved_pos, src, packet))
        {
          YAE_ASSERT(false);
          return false;
        }
      }
    }

    TTime min_dts(kMinInt64, saved_pos.base_);
    int err = demuxer.seek(AVSEEK_FLAG_BACKWARD, min_dts);
    if (err < 0)
    {
      YAE_ASSERT(false);
      return false;
    }

    demuxer.populate();

    bool found = false;
    while (!found)
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.peek(src);
      if (!(src && packet_ptr))
      {
        break;
      }

      const AvPkt & pkt = *packet_ptr;
      const AVPacket & packet = pkt.get();
      if (get_dts(dts, src, packet) || get_pts(dts, src, packet))
      {
        track_id = pkt.trackId_;
        found = true;
        break;
      }

      // this probably won't happen, but I'd like to know if it does:
      YAE_ASSERT(false);
      demuxer.pop(packet_ptr);
    }

    // restore demuxer position:
    err = demuxer.seek(AVSEEK_FLAG_BACKWARD, saved_pos);
    YAE_ASSERT(err >= 0);

    demuxer.populate();

    YAE_ASSERT(found);
    return found;
  }

  //----------------------------------------------------------------
  // summarize
  //
  void
  summarize(DemuxerInterface & demuxer,
            DemuxerSummary & summary,
            double tolerance)
  {
    // get current time position:
    TTime saved_pos;
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.peek(src);
      if (src && packet_ptr)
      {
        const AVPacket & packet = packet_ptr->get();
        get_dts(saved_pos, src, packet) || get_pts(saved_pos, src, packet);
      }
    }

    // analyze from the start:
    demuxer.seek(AVSEEK_FLAG_BACKWARD, TTime(0, 1));
    analyze_timeline(demuxer,
                     summary.streams_,
                     summary.fps_,
                     summary.timeline_,
                     tolerance);

    // restore previous time position:
    demuxer.seek(AVSEEK_FLAG_BACKWARD, saved_pos);

    // get the track id and time position of the "first" packet:
    get_rewind_info(demuxer, summary.rewind_.first, summary.rewind_.second);
  }

  //----------------------------------------------------------------
  // DemuxerBuffer::summarize
  //
  void
  DemuxerBuffer::summarize(DemuxerSummary & summary, double tolerance)
  {
    yae::summarize(*this, summary, tolerance);

    src_.get_decoders(summary.decoders_);
    src_.get_chapters(summary.chapters_);
    src_.get_programs(summary.programs_);
    src_.get_trk_prog(summary.trk_prog_);
    src_.get_metadata(summary.trk_meta_,
                      summary.metadata_);
  }


  //----------------------------------------------------------------
  // ParallelDemuxer::append
  //
  void
  ParallelDemuxer::append(const TDemuxerInterfacePtr & src)
  {
    src_.push_back(src);
  }

  //----------------------------------------------------------------
  // ParallelDemuxer::populate
  //
  void
  ParallelDemuxer::populate()
  {
    for (std::list<TDemuxerInterfacePtr>::iterator
           i = src_.begin(); i != src_.end(); ++i)
    {
      DemuxerInterface & demuxer = *(i->get());
      demuxer.populate();
    }
  }

  //----------------------------------------------------------------
  // ParallelDemuxer::seek
  //
  int
  ParallelDemuxer::seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                        const TTime & dts,
                        const std::string & trackId)
  {
    int result = 0;

    for (std::list<TDemuxerInterfacePtr>::iterator
           i = src_.begin(); i != src_.end() && !result; ++i)
    {
      DemuxerInterface & demuxer = *(i->get());
      int err = demuxer.seek(seekFlags, dts, trackId);

      if (err < 0 && !result)
      {
        result = err;
      }
    }

    return result;
  }

  //----------------------------------------------------------------
  // ParallelDemuxer::peek
  //
  TPacketPtr
  ParallelDemuxer::peek(AVStream *& src) const
  {
    TDemuxerInterfacePtr best;
    TPacketPtr best_pkt;
    AVStream * best_stream = NULL;
    TTime best_dts = TTime::max_flicks();

    for (std::list<TDemuxerInterfacePtr>::const_iterator
           i = src_.begin(); i != src_.end(); ++i)
    {
      DemuxerInterface & demuxer = *(i->get());
      AVStream * stream = NULL;
      TPacketPtr packet_ptr = demuxer.peek(stream);

      if (packet_ptr && stream)
      {
        const AVPacket & packet = packet_ptr->get();

        TTime dts;
        get_dts(dts, stream, packet) || get_pts(dts, stream, packet);

        if (dts < best_dts)
        {
          best = *i;
          best_pkt = packet_ptr;
          best_dts = dts;
          best_stream = stream;
        }
      }
    }

    src = best_stream;
    return best_pkt;
  }

  //----------------------------------------------------------------
  // ParallelDemuxer::summarize
  //
  void
  ParallelDemuxer::summarize(DemuxerSummary & summary, double tolerance)
  {
    for (std::list<TDemuxerInterfacePtr>::const_iterator
           i = src_.begin(); i != src_.end(); ++i)
    {
      const DemuxerSummary & src = (*i)->summary();
      std::map<int, TTime> prog_offset;
      summary.extend(src, prog_offset, tolerance);
    }

    // get the track id and time position of the "first" packet:
    get_rewind_info(*this, summary.rewind_.first, summary.rewind_.second);
  }


  //----------------------------------------------------------------
  // analyze_timeline
  //
  void
  analyze_timeline(DemuxerInterface & demuxer,
                   std::map<std::string, const AVStream *> & streams,
                   std::map<std::string, FramerateEstimator> & fps,
                   std::map<int, Timeline> & programs,
                   double tolerance)
  {
    while (true)
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.get(src);
      if (!packet_ptr)
      {
        break;
      }

      const AvPkt & pkt = *packet_ptr;
      const AVPacket & packet = pkt.get();

      if (!yae::get(streams, pkt.trackId_))
      {
        streams[pkt.trackId_] = src;
      }

      Timeline & timeline = programs[pkt.program_];

      TTime dts;
      bool dts_ok = get_dts(dts, src, packet);

      TTime pts;
      bool pts_ok = get_pts(pts, src, packet);

      if (!dts_ok)
      {
        dts = pts;
      }

      if (!pts_ok)
      {
        pts = dts;
      }

      bool ok = dts_ok || pts_ok;
      YAE_ASSERT(ok);

      TTime dur(src->time_base.num * packet.duration,
                src->time_base.den);

      if (ok)
      {
        bool keyframe = false;
        if (al::starts_with(pkt.trackId_, "v:"))
        {
          fps[pkt.trackId_].push(dts);

          if ((packet.flags & AV_PKT_FLAG_KEY))
          {
            keyframe = true;
          }
        }

        timeline.add_packet(pkt.trackId_,
                            keyframe,
                            packet.size,
                            dts,
                            pts,
                            dur,
                            tolerance);
      }
    }
  }

  //----------------------------------------------------------------
  // remux
  //
  int
  remux(const char * output_path, DemuxerInterface & demuxer)
  {
    // shortcut:
    const DemuxerSummary & summary = demuxer.summary();

    AvOutputContextPtr muxer_ptr(avformat_alloc_context());

    // setup output format:
    AVFormatContext * muxer = muxer_ptr.get();
    muxer->oformat = av_guess_format(NULL, output_path, NULL);

    // setup output streams:
    std::map<std::string, AVStream *> lut;
    for (std::map<std::string, const AVStream *>::const_iterator
           i = summary.streams_.begin(); i != summary.streams_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const AVStream * src = i->second;
      AVStream * dst = avformat_new_stream(muxer, NULL);
      lut[i->first] = dst;

      avcodec_parameters_copy(dst->codecpar, src->codecpar);
      dst->time_base = src->time_base;
      dst->duration = src->duration;

      if (dst->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        const FramerateEstimator & fps = yae::at(summary.fps_, track_id);
        double estimated = fps.best_guess();
        TTime frame_dur = frameDurationForFrameRate(estimated);

        dst->avg_frame_rate.num = frame_dur.base_;
        dst->avg_frame_rate.den = frame_dur.time_;
      }
    }

    // setup output programs:
    for (std::map<int, Timeline>::const_iterator
           i = summary.timeline_.begin(); i != summary.timeline_.end(); ++i)
    {
      const int prog_id = i->first;
      const Timeline & timeline = i->second;
      const TProgramInfo & info = yae::at(summary.programs_, prog_id);

      AVProgram * p = av_new_program(muxer, info.id_);
      YAE_ASSERT(p->id == info.id_);
      p->pmt_pid = info.pmt_pid_;
      p->pcr_pid = info.pcr_pid_;

      for (Timeline::TTracks::const_iterator
             i = timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
      {
        const std::string & track_id = i->first;
        AVStream * dst = yae::get(lut, track_id);
        YAE_ASSERT(dst);

        av_program_add_stream_index(muxer, prog_id, dst->index);
      }

      setDictionary(p->metadata, info.metadata_);
    }

    // setup chapters:
    {
      const std::map<TTime, TChapter> & chapters = summary.chapters_;

      muxer->nb_chapters = chapters.size();
      muxer->chapters = (AVChapter **)av_malloc_array(muxer->nb_chapters,
                                                      sizeof(AVChapter *));


      std::size_t ix = 0;
      for (std::map<TTime, TChapter>::const_iterator j =
             chapters.begin(); j != chapters.end(); ++j, ix++)
      {
        const TChapter & ch = j->second;

        AVChapter * av = (AVChapter *)av_mallocz(sizeof(AVChapter));
        muxer->chapters[ix] = av;
        av->id = ix;

        TTime dt = ch.span_.t1_ - ch.span_.t0_;
        av->time_base.num = 1;
        av->time_base.den = dt.base_;
        av->start = ch.span_.t0_.get(dt.base_);
        av->end = ch.span_.t1_.get(dt.base_);

        setDictionary(av->metadata, ch.metadata_);
      }
    }

    // setup metadata:
    setDictionary(muxer->metadata, summary.metadata_);
    for (std::map<std::string, TDictionary>::const_iterator
           i = summary.trk_meta_.begin(); i != summary.trk_meta_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TDictionary & metadata = i->second;

      if (metadata.empty())
      {
        continue;
      }

      AVStream * dst = yae::get(lut, track_id);
      if (dst)
      {
        setDictionary(dst->metadata, metadata);
      }
    }

    // open the muxer:
    AVDictionary * options = NULL;
    int err = avio_open2(&(muxer->pb),
                         output_path,
                         AVIO_FLAG_WRITE,
                         NULL,
                         &options);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avio_open2(%s) error %i: \"%s\"\n",
             output_path, err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return err;
    }

    // write the header:
    err = avformat_write_header(muxer, NULL);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_header(%s) error %i: \"%s\"\n",
             output_path, err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return err;
    }

    while (true)
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.get(src);
      if (!packet_ptr)
      {
        break;
      }

      const AvPkt & pkt = *packet_ptr;
      const AVPacket & packet = pkt.get();
      AVPacket * tmp = av_packet_clone(&packet);

      AVStream * dst = get(lut, pkt.trackId_);
      tmp->stream_index = dst->index;

      tmp->dts = av_rescale_q(packet.dts, src->time_base, dst->time_base);
      tmp->pts = av_rescale_q(packet.pts, src->time_base, dst->time_base);
      tmp->duration = av_rescale_q(packet.duration,
                                   src->time_base,
                                   dst->time_base);

      err = av_interleaved_write_frame(muxer, tmp);
      if (err < 0)
      {
        av_log(NULL, AV_LOG_ERROR,
               "av_interleaved_write_frame(%s) error %i: \"%s\"\n",
               output_path, err, yae::av_strerr(err).c_str());
        YAE_ASSERT(false);
      }
    }

    err = av_write_trailer(muxer);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_trailer(%s) error %i: \"%s\"\n",
             output_path, err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
    }

    muxer_ptr.reset();
    lut.clear();

    return err;
  }


  //----------------------------------------------------------------
  // calc_src_start_offset
  //
  static TTime
  calc_src_gap(const Timeline & a, const Timeline & b)
  {
    TTime ta_max = TTime::min_flicks();
    TTime tb_min = TTime::max_flicks();
    std::map<std::string, TTime> ta_lut;
    std::map<std::string, TTime> tb_lut;

    for (Timeline::TTracks::const_iterator
           i = a.tracks_.begin(); i != a.tracks_.end(); ++i)
    {
      const std::string & track_id = i->first;
      if (!yae::has(b.tracks_, track_id))
      {
        continue;
      }

      const Timeline::Track & track_a = i->second;
      const Timeline::Track & track_b = yae::at(b.tracks_, track_id);
      const TTime & ta = track_a.dts_span_.back().t1_;
      const TTime & tb = track_b.dts_span_.front().t0_;
      ta_max = std::max(ta, ta_max);
      tb_min = std::min(tb, tb_min);
      ta_lut[track_id] = ta;
      tb_lut[track_id] = tb;
    }

    TTime min_gap = TTime::max_flicks();
    for (std::map<std::string, TTime>::const_iterator
           i = ta_lut.begin(); i != ta_lut.end(); ++i)
    {
      const std::string & track_id = i->first;
      const TTime & ta = i->second;
      const TTime & tb = yae::at(tb_lut, track_id);

      TTime da = ta_max - ta;
      TTime db = tb - tb_min;
      TTime gap = da + db;
      min_gap = std::min(gap, min_gap);
    }

    return min_gap;
  }

  //----------------------------------------------------------------
  // SerialDemuxer::append
  //
  void
  SerialDemuxer::append(const TDemuxerInterfacePtr & src)
  {
    const DemuxerSummary & summary = src->summary();

    for (std::map<int, yae::Timeline>::const_iterator
           i = summary.timeline_.begin(); i != summary.timeline_.end(); ++i)
    {
      const int & prog_id = i->first;
      const Timeline & timeline = i->second;

      for (Timeline::TTracks::const_iterator
             j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
      {
        const std::string & track_id = j->first;
        prog_lut_[track_id] = prog_id;
      }

      std::vector<TTime> & t0 = t0_[prog_id];
      std::vector<TTime> & t1 = t1_[prog_id];
      std::vector<TTime> & offset = offset_[prog_id];

      TTime src_dt = timeline.bbox_dts_.t1_ - timeline.bbox_dts_.t0_;
      TTime src_t0 = t1.empty() ? timeline.bbox_dts_.t0_ : t1.back();

      if (!t1.empty())
      {
        const DemuxerSummary & prev_summary = src_.back()->summary();
        const Timeline & prev = yae::at(prev_summary.timeline_, prog_id);
        TTime src_gap = calc_src_gap(prev, timeline);
        src_t0 -= src_gap;
      }

      TTime src_t1 = src_t0 + src_dt;
      TTime src_offset = timeline.bbox_dts_.t0_ - src_t0;

      t0.push_back(src_t0);
      t1.push_back(src_t1);
      offset.push_back(src_offset);
    }

    src_.push_back(src);
  }

  //----------------------------------------------------------------
  // SerialDemuxer::populate
  //
  void
  SerialDemuxer::populate()
  {
    if (curr_ >= src_.size())
    {
      return;
    }

    DemuxerInterface & src = *(src_[curr_].get());
    src.populate();
  }

  //----------------------------------------------------------------
  // SerialDemuxer::seek
  //
  int
  SerialDemuxer::seek(int seek_flags, // AVSEEK_FLAG_* bitmask
                      const TTime & seek_time,
                      const std::string & track_id)
  {
    int prog_id =
      !track_id.empty() ? yae::at(prog_lut_, track_id) :
      !prog_lut_.empty() ? prog_lut_.begin()->second :
      0;

    std::size_t i = find(seek_time, prog_id);

    if (i >= src_.size())
    {
      return AVERROR(ERANGE);
    }

    const std::vector<TTime> & offset = yae::at(offset_, prog_id);
    TTime t = seek_time + offset[i];
    DemuxerInterface & src = *(src_[i]);

    int err = src.seek(seek_flags, t, track_id);
    if (err >= 0)
    {
      curr_ = i;
    }

    return err;
  }

  //----------------------------------------------------------------
  // SerialDemuxer::peek
  //
  TPacketPtr
  SerialDemuxer::peek(AVStream *& src) const
  {
    while (curr_ < src_.size())
    {
      const DemuxerInterface & curr = *(src_[curr_]);
      TPacketPtr packet_ptr = yae::clone(curr.peek(src));
      if (packet_ptr)
      {
        // adjust pts/dts:
        AvPkt & pkt = *packet_ptr;
        AVPacket & packet = pkt.get();
        const std::vector<TTime> & offsets = yae::at(offset_, pkt.program_);
        const TTime & offset = offsets[curr_];

        int64_t shift = av_rescale_q(-offset.time_,
                                     Rational(1, offset.base_),
                                     src->time_base);
        packet.pts += shift;
        packet.dts += shift;
        return packet_ptr;
      }

      curr_++;

      if (curr_ < src_.size())
      {
        DemuxerInterface & next = const_cast<DemuxerInterface &>(*src_[curr_]);
        const DemuxerSummary & summary = next.summary();

        // rewind the source:
        int err = next.seek(AVSEEK_FLAG_BACKWARD,
                            summary.rewind_.second,
                            summary.rewind_.first);
        YAE_ASSERT(err >= 0);
        next.populate();
      }
    }

    return TPacketPtr();
  }

  //----------------------------------------------------------------
  // SerialDemuxer::find
  //
  std::size_t
  SerialDemuxer::find(const TTime & seek_time, int prog_id) const
  {
    const std::vector<TTime> & t1 = yae::at(t1_, prog_id);
    std::vector<TTime>::const_iterator found = std::upper_bound(t1.begin(),
                                                                t1.end(),
                                                                seek_time);
    if (found == t1.end())
    {
      return src_.size();
    }

    std::size_t i = std::distance(t1.begin(), found);
    const TTime & end_time = t1[i];
    return (seek_time < end_time) ? i : src_.size();
  }

  //----------------------------------------------------------------
  // SerialDemuxer::summarize
  //
  void
  SerialDemuxer::summarize(DemuxerSummary & summary, double tolerance)
  {
    for (std::size_t i = 0; i < src_.size(); i++)
    {
      std::map<int, TTime> prog_offset;
      for (std::map<int, std::vector<TTime> >::const_iterator
             j = offset_.begin(); j != offset_.end(); ++j)
      {
        const int & prog_id = j->first;
        const std::vector<TTime> & offsets = j->second;
        prog_offset[prog_id] = offsets[i];
      }

      // add a chapter marker to the summary if there isn't one already:
      DemuxerSummary src_summary = src_[i]->summary();

      if (src_summary.chapters_.empty())
      {
        std::ostringstream oss;
        oss << "Part " << std::setw(2) << std::setfill('0') << src_.size();

        const Timeline & timeline = src_summary.timeline_.begin()->second;
        TChapter c(oss.str(), timeline.bbox_pts_);
        src_summary.chapters_[timeline.bbox_pts_.t0_] = c;
      }

      summary.extend(src_summary, prog_offset, tolerance);
    }

    // get the track id and time position of the "first" packet:
    get_rewind_info(*this, summary.rewind_.first, summary.rewind_.second);
  }


  //----------------------------------------------------------------
  // TrimmedDemuxer::trim
  //
  void
  TrimmedDemuxer::trim(const TDemuxerInterfacePtr & src,
                       const std::string & trackId,
                       const Timespan & ptsSpan)
  {
    YAE_ASSERT(!src_);

    src_ = src;
    src_summary_ = src->summary();
    track_ = trackId;
    timespan_ = ptsSpan;

    // zero-duration packets could cause problems, try to prevent them:
    src_summary_.replace_missing_durations();

    // figure out the origin and region of interest:
    Timeline::Track::Trim x;
    {
      const Timeline::Track & tt = src_summary_.get_track_timeline(trackId);
      if (!tt.find_samples_for(ptsSpan, x))
      {
        YAE_ASSERT(false);
        throw std::out_of_range("track does not overlap span, trim failed");
      }

      program_ = src_summary_.find_program(trackId);
      // origin_[program_] = tt.pts_[x.ia_];
      origin_[program_] = tt.pts_[x.ka_];
      trim_[trackId] = Trim(tt.dts_[x.ka_],
                            tt.dts_[x.kb_],
                            tt.dts_[x.kc_],
                            tt.dts_[x.ib_]);
      x_[trackId] = x;
    }

    // shortcuts:
    const std::map<int, Timeline> & timelines = src_summary_.timeline_;
    const Timeline & ref = yae::at(timelines, program_);

    // trim every other program/track to match:
    for (std::map<int, Timeline>::const_iterator
           i = timelines.begin(); i != timelines.end(); ++i)
    {
      const int & program = i->first;
      const Timeline & t = i->second;

      // adjust PTS span based on track origin offset
      // between reference program and this program:
      TTime offset = t.bbox_dts_.t0_ - ref.bbox_dts_.t0_;
      Timespan span = ptsSpan + offset;

      for (Timeline::TTracks::const_iterator
             j = t.tracks_.begin(); j != t.tracks_.end(); ++j)
      {
        const std::string & track_id = j->first;
        const Timeline::Track & tt = j->second;
        if (!tt.find_samples_for(span, x))
        {
          YAE_ASSERT(false);
          continue;
        }

        if (!yae::has(origin_, program))
        {
          // origin_[program] = tt.pts_[x.ia_];
          origin_[program] = tt.pts_[x.ka_];
        }

        trim_[track_id] = Trim(tt.dts_[x.ka_],
                               tt.dts_[x.kb_],
                               tt.dts_[x.kc_],
                               tt.dts_[x.ib_]);
        x_[track_id] = x;
      }
    }

    // seek to the starting position, to lower peek latency:
    src_->seek(AVSEEK_FLAG_BACKWARD, yae::at(trim_, track_).a_, track_);
    src_->populate();
    // NOTE: if re-encoding then do it here, once, and store
    // the resulting re-encoded samples in TrimmedDemuxer both
    // for the leading and trailing re-encoded regions
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::populate
  //
  void
  TrimmedDemuxer::populate()
  {
    if (src_)
    {
      src_->populate();
    }
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::seek
  //
  int
  TrimmedDemuxer::seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                       const TTime & dts,
                       const std::string & trackId)
  {
    if (!src_)
    {
      return AVERROR_STREAM_NOT_FOUND;
    }

    std::string track_id = trackId.empty() ? track_ : trackId;
    const int program = src_summary_.find_program(track_id);
    const Timeline::Track & track = src_summary_.get_track_timeline(track_id);
    const Trim & trim = yae::at(trim_, track_id);
    const TTime & origin = yae::at(origin_, program);

    TTime ts = (dts.time_ < 0) ? origin : dts + origin;

    // clamp to trimmed region:
    ts = std::min(trim.d_, std::max(trim.a_, ts));

    int r = src_->seek(seekFlags, ts, track_id);
    return r;
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::peek
  //
  TPacketPtr
  TrimmedDemuxer::peek(AVStream *& src) const
  {
    TPacketPtr packet_ptr;
    while (src_)
    {
      packet_ptr = yae::clone(src_->peek(src));
      if (!packet_ptr)
      {
        break;
      }

      AvPkt & pkt = *packet_ptr;
      AVPacket & packet = pkt.get();

      const Trim & trim = yae::at(trim_, pkt.trackId_);
      const TTime & origin = yae::at(origin_, pkt.program_);

      TTime t0(packet.dts * src->time_base.num, src->time_base.den);
      TTime dt(packet.duration * src->time_base.num, src->time_base.den);
      TTime t1 = t0 + dt;

      if (t1 <= trim.a_)
      {
        bool keyframe = (al::starts_with(pkt.trackId_, "v:") &&
                         (packet.flags & AV_PKT_FLAG_KEY));
        TTime pts(packet.pts * src->time_base.num, src->time_base.den);
        TTime end = pts + dt;

        if (timespan_.overlaps(Timespan(pts, end)) && keyframe)
        {
          YAE_ASSERT(false);
          av_log(NULL, AV_LOG_WARNING,
                 "TrimmedDemuxer::peek, dropping keyframe PTS [%s, %s)",
                 pts.to_hhmmss_ms().c_str(),
                 end.to_hhmmss_ms().c_str());
        }
#if 0
        std::cout
          << "TrimmedDemuxer::peek: packet too old: "
          << t1 << " <= " << trim.a_
          << std::endl;
#endif
        src_->get(src);
        continue;
      }

      if (trim.d_ <= t0)
      {
#if 0
        std::cout
          << "TrimmedDemuxer::peek: packet beyond trim end: "
          << trim.d_ << " <= " << t0
          << std::endl;
#endif
        if (pkt.trackId_ == track_)
        {
          return TPacketPtr();
        }

        src_->get(src);
        continue;
      }

      // adjust pts/dts:
      int64_t shift = av_rescale_q(-origin.time_,
                                   Rational(1, origin.base_),
                                   src->time_base);
      packet.pts += shift;
      packet.dts += shift;
#if 0
      dts.reset(packet.dts * src->time_base.num, src->time_base.den);
      pts.reset(packet.pts * src->time_base.num, src->time_base.den);
      dur.reset(packet.duration * src->time_base.num, src->time_base.den);

      std::cout
        << "TrimmedDemuxer::peek: out: " << pkt.trackId_
        << ", dts: " << Timespan(dts, dts + dur)
        << ", pts: " << Timespan(pts, pts + dur)
        << (keyframe ? ", keyframe" : "")
        << std::endl;
#endif
      break;
    }

    return packet_ptr;
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::summarize
  //
  void
  TrimmedDemuxer::summarize(DemuxerSummary & summary, double tolerance)
  {
    YAE_ASSERT(src_);
    if (!src_)
    {
      return;
    }

    // shortcut:
    const std::map<int, Timeline> & timelines = src_summary_.timeline_;

    // re-summarize the trimmed region:
    for (std::map<int, Timeline>::const_iterator
           i = timelines.begin(); i != timelines.end(); ++i)
    {
      const int & program = i->first;
      const Timeline & timeline = i->second;
      const TTime & origin = yae::at(origin_, program);
      Timeline & trimmed_timeline = summary.timeline_[program];

      for (Timeline::TTracks::const_iterator
             j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
      {
        const std::string & track_id = j->first;
        const Timeline::Track & tt = j->second;
        const bool is_video_track = al::starts_with(track_id, "v:");
        const bool is_subtt_track = al::starts_with(track_id, "s:");

        YAE_ASSERT(tt.dts_.size() == tt.pts_.size() &&
                   tt.dts_.size() == tt.dur_.size());

        const Trim & trim = yae::at(trim_, track_id);
        const Timeline::Track::Trim & x = yae::at(x_, track_id);

        // NOTE: starting from keyframe ka to avoid decoding artifacts,
        // but not sure whether should be starting from ia instead...
        //
        // if the [ka, kb) region is re-encoded then ia could shift
        // so that's probably not right either...
        //
        FramerateEstimator & fps = summary.fps_[track_id];
        for (std::size_t k = x.ka_; k <= x.ib_; k++)
        {
          const bool keyframe = yae::has(tt.keyframes_, k);
          const std::size_t size = tt.size_[k];
          const TTime & dts = tt.dts_[k];
          const TTime & pts = tt.pts_[k];
          const TTime & dur = tt.dur_[k];

          if (is_subtt_track && pts < origin)
          {
            // don't bother with partial subs:
            continue;
          }

          trimmed_timeline.add_packet(track_id,
                                      keyframe,
                                      size,
                                      dts - origin,
                                      pts - origin,
                                      dur,
                                      tolerance);


          if (is_video_track)
          {
            fps.push(dts);
          }
        }
      }
    }

    // trim the chapters, under the assumption that chapters refer
    // to the track that was trimmed:
    const std::map<TTime, TChapter> & src_chapters = src_summary_.chapters_;
    YAE_ASSERT(src_chapters.empty() || origin_.size() == 1);

    std::vector<TChapter> chapters;
    for (std::map<TTime, TChapter>::const_iterator
           i = src_chapters.begin(); i != src_chapters.end(); ++i)
    {
      TChapter chapter = i->second;
      if (!timespan_.overlaps(chapter.span_))
      {
        continue;
      }

      chapter.span_ -= timespan_.t0_;
      chapter.span_.t0_ = std::max(chapter.span_.t0_, TTime());
      chapter.span_.t1_ = std::min(chapter.span_.t1_, timespan_.dt());
      chapters.push_back(chapter);
    }

    for (std::size_t i = 1, z = chapters.size(); i < z; i++)
    {
      TChapter & c0 = chapters[i - 1];
      const TChapter & c1 = chapters[i];
      c0.span_.t1_ = c1.span_.t0_;
    }

    for (std::size_t i = 0, z = chapters.size(); i < z; i++)
    {
      const TChapter & chapter = chapters[i];
      summary.chapters_[chapter.span_.t0_] = chapter;
    }

    // copy the rest:
    summary.metadata_ = src_summary_.metadata_;
    summary.trk_meta_ = src_summary_.trk_meta_;
    summary.streams_ = src_summary_.streams_;
    summary.decoders_ = src_summary_.decoders_;
    summary.programs_ = src_summary_.programs_;
    summary.trk_prog_ = src_summary_.trk_prog_;

    // get the track id and time position of the "first" packet:
    get_rewind_info(*this, summary.rewind_.first, summary.rewind_.second);
  }


  //----------------------------------------------------------------
  // convert_to
  //
  static int
  convert_to(AvFrm & frame, const TVideoFramePtr & vf_ptr)
  {
    int err = 0;
    AVFrame & dst = frame.get();
    TVideoFrame & vf = *(vf_ptr.get());

    boost::shared_ptr<TAVFrameBuffer> avfb =
      boost::dynamic_pointer_cast<TAVFrameBuffer, IPlanarBuffer>(vf.data_);

    if (avfb)
    {
      const AVFrame & src = avfb->frame_.get();
      return av_frame_ref(&dst, &src);
    }

    dst.width = vf.traits_.encodedWidth_;
    dst.height = vf.traits_.encodedHeight_;

    AVPixelFormat src_format = yae_to_ffmpeg(vf.traits_.pixelFormat_);
    dst.format = src_format;

    dst.colorspace = to_ffmpeg_color_space(vf.traits_.colorSpace_);
    dst.color_range = to_ffmpeg_color_range(vf.traits_.colorRange_);
    dst.sample_aspect_ratio.num = int(1000000 * vf.traits_.pixelAspectRatio_ +
                                      0.5);
    dst.sample_aspect_ratio.den = 1000000;

    err = av_frame_get_buffer(&dst, 32);
    if (!err)
    {
      const IPlanarBuffer & buffer = *(vf.data_.get());
      const uint8_t * src_data[4] = { NULL };
      int src_linesize[4] = { 0 };
      for (std::size_t i = 0; i < buffer.planes(); i++)
      {
        src_data[i] = buffer.data(i);
        src_linesize[i] = buffer.rowBytes(i);
      }

      av_image_copy(dst.data,
                    dst.linesize,
                    src_data,
                    src_linesize,
                    src_format,
                    dst.width,
                    dst.height);
    }

    return err;
  }

  //----------------------------------------------------------------
  // decode_keyframe
  //
  TVideoFramePtr
  decode_keyframe(const VideoTrackPtr & decoder_ptr,
                  const TPacketPtr & packet_ptr,
                  TPixelFormatId pixel_format,
                  unsigned int envelope_w,
                  unsigned int envelope_h,
                  double source_dar,
                  double output_par)
  {
    if (!(decoder_ptr && packet_ptr))
    {
      return TVideoFramePtr();
    }

    const AvPkt & pkt = *packet_ptr;
    const AVPacket & packet = pkt.get();
    if (!(packet.flags & AV_PKT_FLAG_KEY))
    {
      return TVideoFramePtr();
    }

    // decode the keyframe packet:
    VideoTrack & decoder = *decoder_ptr;
    if (!decoder.threadIsRunning())
    {
      VideoTraits traits;
      decoder.getTraits(traits);

      double native_dar =
        double(traits.visibleWidth_) / double(traits.visibleHeight_);

      double source_par =
        source_dar ? (source_dar / native_dar) : 0.0;

      traits.pixelFormat_ = pixel_format;
      traits.offsetTop_ = 0;
      traits.offsetLeft_ = 0;
      traits.visibleWidth_ = envelope_w;
      traits.visibleHeight_ = envelope_h;
      traits.pixelAspectRatio_ = output_par;
      traits.cameraRotation_ = 0;
      traits.isUpsideDown_ = false;

      bool deint = false;
      decoder.setTraitsOverride(traits, deint, source_par);

      const pixelFormat::Traits * ptts = NULL;
      if (!decoder.getTraitsOverride(traits) ||
          !(ptts = pixelFormat::getTraits(traits.pixelFormat_)))
      {
        return TVideoFramePtr();
      }

      decoder.threadStart();
      decoder.packetQueue_.waitIndefinitelyForConsumerToBlock();
    }

    // feed the decoder:
    decoder.packetQueue_.push(packet_ptr);

    // flush, so we don't get stuck:
    decoder.packetQueue_.push(TPacketPtr());

    TVideoFramePtr vf_ptr;
    decoder.frameQueue_.pop(vf_ptr);

    // once the decoder is flushed it can't be used again
    // so shut it down so it can be restarted the next time:
    decoder.threadStop();
    decoder.close();

    return vf_ptr;
  }

  //----------------------------------------------------------------
  // save_keyframe
  //
  bool
  save_keyframe(const std::string & path,
                const VideoTrackPtr & decoder_ptr,
                const TPacketPtr & packet_ptr,
                unsigned int envelope_w,
                unsigned int envelope_h,
                double source_dar,
                double output_par)
  {
    std::string url = al::starts_with(path, "file:") ? path : ("file:" + path);

    // setup output format:
    AvOutputContextPtr muxer_ptr(avformat_alloc_context());
    AVFormatContext * muxer = muxer_ptr.get();
    muxer->url = av_strndup(url.c_str(), url.size());
    muxer->oformat = av_guess_format(NULL, path.c_str(), NULL);

    const AVCodec * codec = avcodec_find_encoder(muxer->oformat->video_codec);
    if (!codec)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_find_encoder(%i) failed, %s\n",
             muxer->oformat->video_codec, path.c_str());
      return false;
    }

    TPixelFormatId pixel_format = ffmpeg_to_yae(codec->pix_fmts[0]);
    TVideoFramePtr vf_ptr = decode_keyframe(decoder_ptr,
                                            packet_ptr,
                                            pixel_format,
                                            envelope_w,
                                            envelope_h,
                                            source_dar,
                                            output_par);
    if (!vf_ptr)
    {
      YAE_ASSERT(false);
      return false;
    }

    // shortcuts:
    VideoTrack & decoder = *decoder_ptr;
    const AvPkt & pkt = *packet_ptr;
    const AVPacket & packet = pkt.get();

    // convert TVideoFramePtr to AVFrame:
    AvFrm frm;
    int err = convert_to(frm, vf_ptr);
    if (err)
    {
      av_log(NULL, AV_LOG_ERROR,
             "convert_to(..) failed %i: \"%s\"\n",
             err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // feed it to the encoder:
    TVideoFrame & vf = *vf_ptr;
    AVFrame & frame = frm.get();

    VideoTraits traits;
    decoder.getTraitsOverride(traits);

    // for configuring timebase, etc...
    TTime frame_dur = frameDurationForFrameRate(traits.frameRate_);

    // setup the encoder:
    AvCodecContextPtr encoder_ptr(avcodec_alloc_context3(codec));
    if (!encoder_ptr)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_alloc_context3(%i) failed, %s\n",
             muxer->oformat->video_codec, path.c_str());
      YAE_ASSERT(false);
      return false;
    }

    AVCodecContext & encoder = *encoder_ptr;
    encoder.bit_rate = frame.width * frame.height * 8;
    // encoder.rc_max_rate = encoder.bit_rate * 2;
    encoder.width = frame.width;
    encoder.height = frame.height;
    encoder.time_base.num = 1;
    encoder.time_base.den = frame_dur.base_;
    encoder.ticks_per_frame = frame_dur.time_;
    encoder.framerate.num = frame_dur.base_;
    encoder.framerate.den = frame_dur.time_;
    encoder.gop_size = 1; // expressed as number of frames
    encoder.max_b_frames = 0;
    encoder.pix_fmt = (AVPixelFormat)(frame.format);
    encoder.sample_aspect_ratio = frame.sample_aspect_ratio;

    AVDictionary * encoder_opts = NULL;
    err = avcodec_open2(&encoder, codec, &encoder_opts);
    av_dict_free(&encoder_opts);

    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_open2 error %i: \"%s\"\n",
             err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // setup output streams:
    AVStream * dst = avformat_new_stream(muxer, NULL);
    dst->time_base.num = 1;
    dst->time_base.den = frame_dur.base_;
    dst->avg_frame_rate.num = frame_dur.base_;
    dst->avg_frame_rate.den = frame_dur.time_;
    dst->duration = 1;
    dst->sample_aspect_ratio = frame.sample_aspect_ratio;

    err = avcodec_parameters_from_context(dst->codecpar, &encoder);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_parameters_from_context error %i: \"%s\"\n",
             err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // open the muxer:
    AVDictionary * io_opts = NULL;
    err = avio_open2(&(muxer->pb),
                     path.c_str(),
                     AVIO_FLAG_WRITE,
                     NULL,
                     &io_opts);
    av_dict_free(&io_opts);

    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avio_open2(%s) error %i: \"%s\"\n",
             path.c_str(), err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // write the header:
    AVDictionary * muxer_opts = NULL;
    av_dict_set_int(&muxer_opts, "update", 1, 0);
    err = avformat_write_header(muxer, &muxer_opts);
    av_dict_free(&muxer_opts);

    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_header(%s) error %i: \"%s\"\n",
             path.c_str(), err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    // send the frame to the encoder:
    frame.key_frame = 1;
    frame.pict_type = AV_PICTURE_TYPE_I;
    frame.pts = av_rescale_q(vf.time_.time_,
                             Rational(1, vf.time_.base_),
                             dst->time_base);
    err = avcodec_send_frame(&encoder, &frame);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_send_frame error %i: \"%s\"\n",
             err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    while (!err)
    {
      AvPkt out_pkt;
      AVPacket & out = out_pkt.get();
      err = avcodec_receive_packet(&encoder, &out);
      if (err)
      {
        if (err < 0 && err != AVERROR(EAGAIN) && err != AVERROR_EOF)
        {
          av_log(NULL, AV_LOG_ERROR,
                 "avcodec_receive_packet error %i: \"%s\"\n",
                 err, yae::av_strerr(err).c_str());
          YAE_ASSERT(false);
        }

        break;
      }

      out.stream_index = dst->index;
      out.dts = av_rescale_q(out.dts, encoder.time_base, dst->time_base);
      out.pts = av_rescale_q(out.pts, encoder.time_base, dst->time_base);
      out.duration = av_rescale_q(packet.duration,
                                  encoder.time_base,
                                  dst->time_base);

      err = av_interleaved_write_frame(muxer, &out);
      if (err < 0)
      {
        av_log(NULL, AV_LOG_ERROR,
               "av_interleaved_write_frame(%s) error %i: \"%s\"\n",
               path.c_str(), err, yae::av_strerr(err).c_str());
        YAE_ASSERT(false);
        return false;
      }

      // flush-out the encoder:
      err = avcodec_send_frame(&encoder, NULL);
    }

    err = av_write_trailer(muxer);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_trailer(%s) error %i: \"%s\"\n",
             path.c_str(), err, yae::av_strerr(err).c_str());
      YAE_ASSERT(false);
      return false;
    }

    muxer_ptr.reset();
    return true;
  }

  //----------------------------------------------------------------
  // pull
  //
  static void
  pull(VideoTrack & decoder,
       const Timespan & pts_span,
       TVideoFrameCallback callback,
       void * context)
  {
    while (true)
    {
      TVideoFramePtr vf_ptr;
      if (!decoder.frameQueue_.pop(vf_ptr, NULL, false))
      {
        break;
      }

      if (!vf_ptr)
      {
        continue;
      }

      const TVideoFrame & vf = *vf_ptr;
      if (!pts_span.contains(vf.time_))
      {
        continue;
      }

      if (callback)
      {
        callback(vf_ptr, context);
      }
    }
  }

  //----------------------------------------------------------------
  // decode_gop
  //
  bool
  decode_gop(// source:
             const TDemuxerInterfacePtr & demuxer_ptr,
             const std::string & track_id,
             std::size_t k0,
             std::size_t k1,
             // output:
             TPixelFormatId pixel_format,
             unsigned int envelope_w,
             unsigned int envelope_h,
             double source_dar,
             double output_par,
             // delivery:
             TVideoFrameCallback callback,
             void * context)
  {
    const DemuxerSummary & summary = demuxer_ptr->summary();

    // configure decoder:
    VideoTrackPtr decoder_ptr;
    {
      TrackPtr track = yae::get(summary.decoders_, track_id);
      decoder_ptr = boost::dynamic_pointer_cast<VideoTrack, Track>(track);
    }

    if (!decoder_ptr)
    {
      YAE_ASSERT(false);
      return false;
    }

    VideoTrack & decoder = *decoder_ptr;
    {
      VideoTraits traits;
      decoder.getTraits(traits);

      double native_dar =
        double(traits.visibleWidth_) / double(traits.visibleHeight_);

      double source_par =
        source_dar ? (source_dar / native_dar) : 0.0;

      traits.offsetTop_ = 0;
      traits.offsetLeft_ = 0;
      traits.visibleWidth_ = envelope_w;
      traits.visibleHeight_ = envelope_h;
      traits.pixelAspectRatio_ = output_par;
      traits.cameraRotation_ = 0;
      traits.isUpsideDown_ = false;

      if (pixel_format != kInvalidPixelFormat)
      {
        traits.pixelFormat_ = pixel_format;
      }

      bool deint = false;
      decoder.setTraitsOverride(traits, deint, source_par);

      const pixelFormat::Traits * ptts = NULL;
      if (!decoder.getTraitsOverride(traits) ||
          !(ptts = pixelFormat::getTraits(traits.pixelFormat_)))
      {
        YAE_ASSERT(false);
        return false;
      }

      decoder.frameQueue_.setMaxSizeUnlimited();
    }

    // configure demuxer:
    const Timeline::Track & track = summary.get_track_timeline(track_id);
    const TTime & dts_a = track.dts_[k0];
    const TTime & pts_a = track.pts_[k0];

    const TTime dts_b =
      (k1 < track.dts_.size()) ?
      track.dts_[k1] :
      track.dts_.back() + track.dur_.back();

    const TTime pts_b =
      (k1 < track.pts_.size()) ?
      track.pts_[k1] :
      track.pts_.back() + track.dur_.back();

    Timespan dts_span(dts_a, dts_b);
    Timespan pts_span(pts_a, pts_b);

    DemuxerInterface & demuxer = *(demuxer_ptr.get());
    demuxer.seek(AVSEEK_FLAG_BACKWARD, dts_span.t0_, track_id);
    demuxer.populate();

    // decode:
    decoder.resetTimeCounters(dts_span.t0_.sec(), false);
    decoder.decoderStartup();

    while (true)
    {
      AVStream * src = NULL;
      TPacketPtr packet_ptr = demuxer.get(src);
      if (!packet_ptr)
      {
        break;
      }

      const AvPkt & pkt = *packet_ptr;
      if (pkt.trackId_ != track_id)
      {
        continue;
      }

      const AVPacket & packet = pkt.get();
      TTime t0(packet.dts * src->time_base.num, src->time_base.den);
      TTime dt(packet.duration * src->time_base.num, src->time_base.den);
      TTime t1 = t0 + dt;

#if 1
      if (t1 < dts_span.t0_)
      {
#if 0 // ndef NDEBUG
        av_log(NULL, AV_LOG_INFO,
               "decode_gop skipping %spacket [%s, %s) "
               "because its outside GOP range [%s, %s)\n",
               (packet.flags & AV_PKT_FLAG_KEY) ? "KEYFRAME " : "",
               t0.to_hhmmss_ms().c_str(),
               t1.to_hhmmss_ms().c_str(),
               dts_span.t0_.to_hhmmss_ms().c_str(),
               dts_span.t1_.to_hhmmss_ms().c_str());
#endif
        continue;
      }
#endif

      if (dts_span.t1_ < t0)
      {
        break;
      }

      decoder.decode(packet_ptr);
      pull(decoder, pts_span, callback, context);
    }

    // flush:
    decoder.flush();
    pull(decoder, pts_span, callback, context);

    // done:
    decoder.decoderShutdown();
    return true;
  }

}
