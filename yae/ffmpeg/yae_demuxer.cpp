// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae includes:
#include "yae_demuxer.h"


namespace yae
{

  //----------------------------------------------------------------
  // Demuxer::Demuxer
  //
  Demuxer::Demuxer():
    context_(NULL),
    ato_(0),
    vto_(0),
    sto_(0),
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
    context_ = avformat_alloc_context();

    YAE_ASSERT(!interruptDemuxer_);
    context_->interrupt_callback.callback = &Demuxer::demuxerInterruptCallback;
    context_->interrupt_callback.opaque = this;

    AVDictionary * options = NULL;

    // set probesize to 64 MiB:
    av_dict_set(&options, "probesize", "67108864", 0);

    // set analyze duration to 10 seconds:
    av_dict_set(&options, "analyzeduration", "10000000", 0);

    // set genpts:
    av_dict_set(&options, "fflags", "genpts", 0);

    int err = avformat_open_input(&context_,
                                  resourcePath,
                                  NULL, // AVInputFormat to force
                                  &options);
    av_dict_free(&options);

    if (err != 0)
    {
      close();
      return false;
    }

    YAE_ASSERT(context_->flags & AVFMT_FLAG_GENPTS);

    err = avformat_find_stream_info(context_, NULL);
    if (err < 0)
    {
      close();
      return false;
    }

    // get the programs:
    for (unsigned int i = 0; i < context_->nb_programs; i++)
    {
      const AVProgram * p = context_->programs[i];
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

    if (context_->nb_programs < 1)
    {
      // there must be at least 1 implied program:
      programs_.push_back(TProgramInfo());

      for (unsigned int i = 0; i < context_->nb_streams; i++)
      {
        streamIndexToProgramIndex_[i] = 0;
      }
    }

    for (unsigned int i = 0; i < context_->nb_streams; i++)
    {
      AVStream * stream = context_->streams[i];

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

      TrackPtr baseTrack(new Track(context_, stream));

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
          track->setId(make_track_id('v', vto_ + videoTracks_.size()));
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
          track->setId(make_track_id('a', ato_ + audioTracks_.size()));
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
          track->setId(make_track_id('s', sto_ + subttTracks_.size()));
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
    if (context_ == NULL)
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

    avformat_close_input(&context_);
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
      info.program_ = get(streamIndexToProgramIndex_, t->streamIndex());
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
      info.program_ = get(streamIndexToProgramIndex_, t->streamIndex());
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
      info.program_ = get(streamIndexToProgramIndex_, t->streamIndex());
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
    int err = av_read_frame(context_, &packet);

    if (interruptDemuxer_)
    {
      interruptDemuxer_ = false;
    }

    if (!err)
    {
      TrackPtr track = get(tracks_, packet.stream_index);
      if (track)
      {
        packet.trackId_ = track->id();
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
    if (!context_ || !context_->pb || !context_->pb->seekable)
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
    if (!context_)
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
      streamIndex = get(trackIdToStreamIndex_, trackId, -1);
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

        const AVStream * s = context_->streams[streamIndex];
        ts = av_rescale_q(seekTime.time_, tb, s->time_base);
      }
    }
    else
    {
      ts = seekTime.time_;
    }

    int err = avformat_seek_file(context_,
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

}
