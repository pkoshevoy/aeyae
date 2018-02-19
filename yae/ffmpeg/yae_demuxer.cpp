// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard lib:
#include <inttypes.h>
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
  // AvOutputContextPtr::destroy
  //
  void
  AvOutputContextPtr::destroy(AVFormatContext * ctx)
  {
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
      pts = TTime(stream->time_base.num * packet.pts,
                  stream->time_base.den);
      return true;
    }

    return false;
  }


  //----------------------------------------------------------------
  // ProgramBuffer::ProgramBuffer
  //
  ProgramBuffer::ProgramBuffer():
    num_packets_(0),
    t0_(std::numeric_limits<int64_t>::max(), 1),
    t1_(std::numeric_limits<int64_t>::min(), 1)
  {}

  //----------------------------------------------------------------
  // ProgramBuffer::clear
  //
  void
  ProgramBuffer::clear()
  {
    packets_.clear();
    num_packets_ = 0;
    t0_.reset(std::numeric_limits<int64_t>::max(), 1);
    t1_.reset(std::numeric_limits<int64_t>::min(), 1);
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

    TTime next_dts = yae::get(next_dts_, packet.stream_index,
                              TTime(-stream->codecpar->video_delay *
                                    stream->avg_frame_rate.den,
                                    stream->avg_frame_rate.num));

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
    TTime dts(std::numeric_limits<int64_t>::max(), 1);
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
        t0_ = TTime(std::numeric_limits<int64_t>::max(), 1);
        t1_ = TTime(std::numeric_limits<int64_t>::min(), 1);
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
      if (!pkts.empty() && pkts.front() == packet_ptr)
      {
        pkts.pop_front();

        YAE_ASSERT(num_packets_ > 0);
        num_packets_--;

        if (!num_packets_)
        {
          // reset the timespan:
          t0_ = TTime(std::numeric_limits<int64_t>::max(), 1);
          t1_ = TTime(std::numeric_limits<int64_t>::min(), 1);
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
    TTime dts_min(std::numeric_limits<int64_t>::max(), 1);
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
      TTime dts_min(std::numeric_limits<int64_t>::max(), 1);
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

    for (std::map<std::string, FramerateEstimator>::const_iterator
           i = s.fps_.begin(); i != s.fps_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const FramerateEstimator & src = i->second;
      fps_[track_id] += src;
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
    int program = -1;

    for (std::map<int, Timeline>::const_iterator
           i = timeline_.begin(); i != timeline_.end(); ++i)
    {
      const Timeline & t = i->second;
      if (yae::has(t.tracks_, trackId))
      {
        program = i->first;
        break;
      }
    }

    if (program == -1)
    {
      throw std::out_of_range(trackId);
    }

    return program;
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
                      const TTime & seekTime,
                      const std::string & trackId)
  {
    return src_.seek(seekFlags, seekTime, trackId);
  }

  //----------------------------------------------------------------
  // DemuxerBuffer::peek
  //
  TPacketPtr
  DemuxerBuffer::peek(AVStream *& src) const
  {
    TTime dts(std::numeric_limits<int64_t>::max(), 1);
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
            TPacketPtr pkt = found->second.front();
            return pkt;
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
    src_.get_metadata(summary.trk_meta_,
                      summary.metadata_);
  }


  //----------------------------------------------------------------
  // ParallelDemuxer::append
  //
  void
  ParallelDemuxer::append(const TDemuxerInterfacePtr & src,
                          const DemuxerSummary & summary)
  {
    src_.push_back(src);
    summary_.push_back(summary);
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
                        const TTime & seekTime,
                        const std::string & trackId)
  {
    int result = 0;

    for (std::list<TDemuxerInterfacePtr>::iterator
           i = src_.begin(); i != src_.end() && !result; ++i)
    {
      DemuxerInterface & demuxer = *(i->get());
      int err = demuxer.seek(seekFlags, seekTime, trackId);

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
    TTime best_dts(std::numeric_limits<int64_t>::max(), 1);

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
    for (std::list<DemuxerSummary>::const_iterator
           i = summary_.begin(); i != summary_.end(); ++i)
    {
      const DemuxerSummary & src = *i;
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

#if 0
      if (al::starts_with(pkt.trackId_, "_:"))
      {
        // skip non audio/video/subtitle tracks:
        continue;
      }
#endif

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

        timeline.add_frame(pkt.trackId_, keyframe, dts, pts, dur, tolerance);
      }
    }
  }

  //----------------------------------------------------------------
  // remux
  //
  int
  remux(const char * output_path,
        const DemuxerSummary & summary,
        DemuxerInterface & demuxer)
  {
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
      AVStream * dst = avformat_new_stream(muxer,
#if 0
                                           src->codec ?
                                           src->codec->codec :
#endif
                                           NULL);
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
        av->time_base.num = 1;
        av->time_base.den = ch.span_.t0_.base_;
        av->start = ch.span_.t0_.get(ch.span_.t1_.base_);
        av->end = ch.span_.t1_.time_;
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
      return err;
    }

    // write the header:
    err = avformat_write_header(muxer, NULL);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_header(%s) error %i: \"%s\"\n",
             output_path, err, yae::av_strerr(err).c_str());
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
      }
    }

    err = av_write_trailer(muxer);
    if (err < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avformat_write_trailer(%s) error %i: \"%s\"\n",
             output_path, err, yae::av_strerr(err).c_str());
    }

    muxer_ptr.reset();
    lut.clear();

    return err;
  }


  //----------------------------------------------------------------
  // SerialDemuxer::append
  //
  void
  SerialDemuxer::append(const TDemuxerInterfacePtr & src,
                        const DemuxerSummary & summary)
  {
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

      std::vector<TTime> & t1 = t1_[prog_id];
      std::vector<TTime> & offset = offset_[prog_id];

      TTime src_dt = timeline.bbox_dts_.t1_ - timeline.bbox_dts_.t0_;
      TTime src_t0 = t1.empty() ? timeline.bbox_dts_.t0_ : t1.back();
      TTime src_t1 = src_t0 + src_dt;
      TTime src_offset = timeline.bbox_dts_.t0_ - src_t0;

      t1.push_back(src_t1);
      offset.push_back(src_offset);

      std::map<TTime, std::size_t> & t0 = t0_[prog_id];
      t0[src_t0] = src_.size();
    }

    src_.push_back(src);
    summary_.push_back(summary);
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
      TPacketPtr packet_ptr = curr.peek(src);
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
        const DemuxerSummary & summary = summary_[curr_];

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
    const std::map<TTime, std::size_t> & t0 = yae::at(t0_, prog_id);

    std::map<TTime, std::size_t>::const_iterator
      found = t0.lower_bound(seek_time);

    if (found == t0.end())
    {
      return 0;
    }

    std::size_t i = found->second;
    const std::vector<TTime> & t1 = yae::at(t1_, prog_id);
    const TTime & end_time = t1[i];
    return (seek_time < end_time) ? i : src_.size();
  }

  //----------------------------------------------------------------
  // SerialDemuxer::summarize
  //
  void
  SerialDemuxer::summarize(DemuxerSummary & summary, double tolerance)
  {
    for (std::size_t i = 0; i < summary_.size(); i++)
    {
      std::map<int, TTime> prog_offset;
      for (std::map<int, std::vector<TTime> >::const_iterator
             j = offset_.begin(); j != offset_.end(); ++j)
      {
        const int & prog_id = j->first;
        const std::vector<TTime> & offsets = j->second;
        prog_offset[prog_id] = offsets[i];
      }

      const DemuxerSummary & src = summary_[i];
      summary.extend(src, prog_offset, tolerance);
    }

    // get the track id and time position of the "first" packet:
    get_rewind_info(*this, summary.rewind_.first, summary.rewind_.second);
  }


  //----------------------------------------------------------------
  // TrimmedDemuxer::trim
  //
  void
  TrimmedDemuxer::trim(const TDemuxerInterfacePtr & src,
                       const DemuxerSummary & summary,
                       const std::string & trackId,
                       const Timespan & ptsSpan)
  {
    src_ = src;
    summary_ = summary;

    // zero-duration packets could cause problems, try to prevent them:
    summary_.replace_missing_durations();

    // figure out the origin and region of interest:
    std::size_t ka, kb, kc, kd, ia, ib;
    {
      const Timeline::Track & tt = summary_.get_track_timeline(trackId);
      if (!tt.find_samples_for(ptsSpan, ka, kb, kc, kd, ia, ib))
      {
        YAE_ASSERT(false);
        throw std::out_of_range("track does not overlap span, trim failed");
      }

      program_ = summary_.find_program(trackId);
      origin_[program_] = tt.pts_[ia];
      trim_[trackId] = Trim(tt.dts_[ka],
                            tt.dts_[kb],
                            tt.dts_[kc],
                            tt.dts_[ib]);
    }

    // shortcut:
    const Timeline & ref = yae::at(summary_.timeline_, program_);

    // trim every other program/track to match:
    for (std::map<int, Timeline>::const_iterator
           i = summary_.timeline_.begin(); i != summary_.timeline_.end(); ++i)
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
        if (!tt.find_samples_for(span, ka, kb, kc, kd, ia, ib))
        {
          YAE_ASSERT(false);
          continue;
        }

        if (!yae::has(origin_, program))
        {
          origin_[program] = tt.pts_[ia];
        }

        trim_[track_id] = Trim(tt.dts_[ka],
                               tt.dts_[kb],
                               tt.dts_[kc],
                               tt.dts_[ib]);
      }
    }
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::populate
  //
  void
  TrimmedDemuxer::populate()
  {
    // FIXME: pkoshevoy: write me!
    YAE_ASSERT(false);
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::seek
  //
  int
  TrimmedDemuxer::seek(int seekFlags, // AVSEEK_FLAG_* bitmask
                       const TTime & seekTime,
                       const std::string & trackId)
  {
    // FIXME: pkoshevoy: write me!
    YAE_ASSERT(false);
    return -1;
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::peek
  //
  TPacketPtr
  TrimmedDemuxer::peek(AVStream *& src) const
  {
    // FIXME: pkoshevoy: write me!
    YAE_ASSERT(false);
    return TPacketPtr();
  }

  //----------------------------------------------------------------
  // TrimmedDemuxer::summarize
  //
  void
  TrimmedDemuxer::summarize(DemuxerSummary & summary, double tolerance)
  {
    // FIXME: pkoshevoy: write me!
    YAE_ASSERT(false);
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
    if (!(decoder_ptr && packet_ptr))
    {
      return false;
    }

    const AvPkt & pkt = *packet_ptr;
    const AVPacket & packet = pkt.get();
    if (!(packet.flags & AV_PKT_FLAG_KEY))
    {
      return false;
    }

    // setup output format:
    AvOutputContextPtr muxer_ptr(avformat_alloc_context());
    AVFormatContext * muxer = muxer_ptr.get();
    av_strlcpy(muxer->filename, path.c_str(), sizeof(muxer->filename));
    muxer->oformat = av_guess_format(NULL, path.c_str(), NULL);

    const AVCodec * codec = avcodec_find_encoder(muxer->oformat->video_codec);
    if (!codec)
    {
      av_log(NULL, AV_LOG_ERROR,
             "avcodec_find_encoder(%i) failed, %s\n",
             muxer->oformat->video_codec, path.c_str());
      return false;
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

      traits.pixelFormat_ = ffmpeg_to_yae(codec->pix_fmts[0]);
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
        return false;
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

    if (!vf_ptr)
    {
      YAE_ASSERT(false);
      return false;
    }

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
      return false;
    }

    muxer_ptr.reset();
    return true;
  }

}
