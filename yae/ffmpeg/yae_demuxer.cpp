// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard lib:
#include <limits>

// yae includes:
#include "yae_demuxer.h"
#include "../utils/yae_utils.h"


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

      const AVDictionaryEntry * start = NULL;
      while (true)
      {
        AVDictionaryEntry * found =
          av_dict_get(p->metadata, "", start, AV_DICT_IGNORE_SUFFIX);

        if (!found)
        {
          break;
        }

        info.metadata_[std::string(found->key)] = std::string(found->value);
        start = found;
      }

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

        const AVDictionaryEntry * prev = NULL;
        while (true)
        {
          AVDictionaryEntry * found =
            av_dict_get(stream->metadata, "", prev, AV_DICT_IGNORE_SUFFIX);

          if (!found)
          {
            break;
          }

          att.metadata_[std::string(found->key)] = std::string(found->value);
          prev = found;
        }

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
          videoTracks_.push_back(track);
          tracks_[stream->index] = track;
          trackIdToStreamIndex_[track->id()] = stream->index;
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
          audioTracks_.push_back(track);
          tracks_[stream->index] = track;
          trackIdToStreamIndex_[track->id()] = stream->index;
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
          subttTracks_.push_back(track);
          tracks_[stream->index] = track;
          trackIdToStreamIndex_[track->id()] = stream->index;
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
    trackIdToStreamIndex_.clear();

    context_.reset();
  }

  //----------------------------------------------------------------
  // Demuxer::getTrack
  //
  TrackPtr
  Demuxer::getTrack(const std::string & trackId) const
  {
    std::map<std::string, int>::const_iterator
      found = trackIdToStreamIndex_.find(trackId);
    if (found == trackIdToStreamIndex_.end())
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
  Demuxer::demux(AvPkt & packet)
  {
    int err = av_read_frame(context_.get(), &packet);

    if (interruptDemuxer_)
    {
      interruptDemuxer_ = false;
    }

    if (!err)
    {
      TrackPtr track = yae::get(tracks_, packet.stream_index);
      if (track)
      {
        packet.trackId_ = track->id();
      }
      else
      {
        packet.trackId_ = make_track_id('_', packet.stream_index);
      }

      // make up a demuxer name for this packet:
      {
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << ix_;
        packet.demuxer_ = oss.str();
      }

      // make up a program name for this packet:
      {
        const TProgramInfo * info = getProgram(packet.stream_index);
        int program = info ? info->id_ : 0;

        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << program;
        packet.program_ = oss.str();
      }
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
      streamIndex = yae::get(trackIdToStreamIndex_, trackId, -1);
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
        ts = seekTime.getTime(AV_TIME_BASE);
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
                                 ts, // kMaxInt64,
                                 seekFlags);

    if (err < 0)
    {
#ifndef NDEBUG
      std::cerr
        << "avformat_seek_file (" << ts << ") returned " << err
        << std::endl;
#endif
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
    std::cout
      << "file opened: " << filePath
      << ", programs: " << primary.programs().size()
      << ", a: " << primary.audioTracks().size()
      << ", v: " << primary.videoTracks().size()
      << ", s: " << primary.subttTracks().size()
      << std::endl;

    if (primary.programs().size() < 2)
    {
      // add auxiliary resources:
      std::size_t trackOffset = 100;
      TOpenFolder folder(folderPath);
      while (folder.parseNextItem())
      {
        std::string nm = folder.itemName();
        if (!al::starts_with(nm, baseName) || nm == fileName)
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
        std::cout
          << "file opened: " << nm
          << ", programs: " << aux.programs().size()
          << ", a: " << aux.audioTracks().size()
          << ", v: " << aux.videoTracks().size()
          << ", s: " << aux.subttTracks().size()
          << std::endl;

        trackOffset += 100;
      }
    }

    return true;
  }

  //----------------------------------------------------------------
  // get_dts
  //
  bool
  get_dts(TTime & dts, const AVStream * stream, const AVPacket & pkt)
  {
    if (pkt.dts != AV_NOPTS_VALUE)
    {
      dts = TTime(stream->time_base.num * pkt.dts,
                  stream->time_base.den);
      return true;
    }
    else if (pkt.pts != AV_NOPTS_VALUE)
    {
      dts = TTime(stream->time_base.num * pkt.pts,
                  stream->time_base.den);
      return true;
    }

    return false;
  }


  //----------------------------------------------------------------
  // ProgramBuffer::ProgramBuffer
  //
  ProgramBuffer::ProgramBuffer():
    t0_(std::numeric_limits<int64_t>::max(), 1),
    t1_(std::numeric_limits<int64_t>::min(), 1),
    num_packets_(0)
  {}

  //----------------------------------------------------------------
  // ProgramBuffer::push
  //
  void
  ProgramBuffer::push(const TPacketPtr & packet, const AVStream * stream)
  {
    if (!(packet && stream))
    {
      YAE_ASSERT(false);
      return;
    }

    // shortcut:
    AvPkt & pkt = *packet;

    TTime dts = next_dts_[pkt.stream_index];
    if (!get_dts(dts, stream, pkt))
    {
      YAE_ASSERT(false);
      YAE_ASSERT(pkt.duration);
      pkt.dts = dts.getTime(stream->time_base.den) / stream->time_base.num;
    }

    TTime dur(stream->time_base.num * pkt.duration,
              stream->time_base.den);
    next_dts_[pkt.stream_index] = dts + dur;

    t0_ = std::min<TTime>(t0_, dts);
    t1_ = std::max<TTime>(t1_, dts);

    packets_[pkt.stream_index].push_back(packet);
    num_packets_++;
  }

  //----------------------------------------------------------------
  // ProgramBuffer::choose
  //
  int
  ProgramBuffer::choose(const AVFormatContext & ctx, TTime & dts_min) const
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
      const AVStream * stream = ctx.streams[pkt.stream_index];
      if (!stream)
      {
        YAE_ASSERT(false);
        continue;
      }

      TTime dts;
      if (!get_dts(dts, stream, pkt))
      {
        YAE_ASSERT(false);
        return pkt.stream_index;
      }

      if (dts < dts_min)
      {
        dts_min = dts;
        stream_index = pkt.stream_index;
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

    if (stream_index < 0 || stream_index >= ctx.nb_streams)
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
    TPacketPtr pkt = pkts.front();
    return pkt;
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
    TPacketPtr pkt = peek(ctx, dts, stream_index);

    if (pkt)
    {
      unsigned int stream_index = pkt->stream_index;
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

    return pkt;
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
      const AVStream * stream = ctx.streams[next->stream_index];

      // adjust t0:
      get_dts(t0_, stream, *next);
    }
  }


  //----------------------------------------------------------------
  // PacketBuffer::PacketBuffer
  //
  PacketBuffer::PacketBuffer(const TDemuxerPtr & demuxer, double buffer_sec):
    demuxer_(demuxer),
    buffer_sec_(buffer_sec)
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

      std::size_t num_packets = 0;
      for (std::map<int, TProgramBufferPtr>::const_iterator
             i = program_.begin(); i != program_.end(); ++i)
      {
        const ProgramBuffer & buffer = *(i->second);
        double duration = buffer.duration_sec();
        num_packets += buffer.num_packets();

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
        break;
      }

      if (max_duration > buffer_sec_ * 5.0)
      {
        std::cerr
          << std::setw(3) << std::setfill('0') << min_id
          << " min buffer duration: " << min_duration << '\n'
          << std::setw(3) << std::setfill('0') << max_id
          << " max buffer duration: " << max_duration << ", giving up"
          << std::endl;
        break;
      }

      TPacketPtr packet(new AvPkt());
      AvPkt & pkt = *packet;

      int err = demuxer.demux(pkt);
      if (err)
      {
        return err;
      }

      const AVStream * stream = ctx.streams[pkt.stream_index];
      if (!stream)
      {
        YAE_ASSERT(false);
        continue;
      }

      TProgramBufferPtr program = yae::get(stream_, pkt.stream_index);
      if (!program)
      {
        YAE_ASSERT(false);
        continue;
      }

      YAE_ASSERT(!packet->trackId_.empty());
      program->push(packet, stream);
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
      double duration = buffer->duration_sec();
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
    TPacketPtr pkt = buffer->peek(ctx, dts_min, stream_index);
    return pkt;
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
    TPacketPtr pkt = buffer->get(ctx, src, stream_index);

    if (pkt)
    {
      // refill the buffer:
      populate();
      buffer->update_duration(ctx);
    }

    return pkt;
  }


  //----------------------------------------------------------------
  // DemuxerBuffer::DemuxerBuffer
  //
  DemuxerBuffer::DemuxerBuffer(const std::list<TDemuxerPtr> & src,
                               double buffer_sec)
  {
    for (std::list<TDemuxerPtr>::const_iterator
           i = src.begin(); i != src.end(); ++i)
    {
      src_.push_back(PacketBuffer(*i, buffer_sec));
    }
  }

  //----------------------------------------------------------------
  // DemuxerBuffer::get
  //
  TPacketPtr
  DemuxerBuffer::get(AVStream *& stream)
  {
    TTime best_dts(std::numeric_limits<int64_t>::max(), 1);
    PacketBuffer * best_src = NULL;
    TProgramBufferPtr best_program;
    int best_stream_index = -1;

    for (std::list<PacketBuffer>::iterator
           i = src_.begin(); i != src_.end(); ++i)
    {
      PacketBuffer & buffer = *i;

      // refill the buffer, otherwise peek won't work:
      buffer.populate();

      int stream_index = -1;
      TTime dts(std::numeric_limits<int64_t>::max(), 1);
      TProgramBufferPtr program = buffer.choose(dts, stream_index);
      if (!program)
      {
        continue;
      }

      if (dts < best_dts)
      {
        best_dts = dts;
        best_src = &buffer;
        best_program = program;
        best_stream_index = stream_index;
      }
    }

    if (!best_src)
    {
      return TPacketPtr();
    }

    return best_src->get(stream, best_program, best_stream_index);
  }

}
