// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/ffmpeg/yae_closed_captions.h"
#include "yae/ffmpeg/yae_movie.h"
#include "yae/ffmpeg/yae_reader_ffmpeg.h"


namespace yae
{

  //----------------------------------------------------------------
  // Movie::Movie
  //
  Movie::Movie():
    thread_(this),
    context_(NULL),
    selectedVideoTrack_(0),
    selectedAudioTrack_(0),
    hwdec_(false),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    enableClosedCaptions_(0),
    adjustTimestamps_(NULL),
    adjustTimestampsCtx_(NULL),
    file_size_(0),
    packet_pos_(-1),
    posIn_(new TimePos(TTime::min_flicks_as_sec())),
    posOut_(new TimePos(TTime::max_flicks_as_sec())),
    interruptDemuxer_(false),
    playbackEnabled_(false),
    looping_(false),
    mustStop_(true)
  {
    ensure_ffmpeg_initialized();
  }

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
  // Movie::requestMutex
  //
  void
  Movie::requestMutex(boost::unique_lock<boost::timed_mutex> & lk)
  {
    YAE_ASSERT(!interruptDemuxer_);

    while (!lk.timed_lock(boost::posix_time::milliseconds(1000)))
    {
      interruptDemuxer_ = true;
      boost::this_thread::yield();
    }
  }

  //----------------------------------------------------------------
  // Movie::demuxerInterruptCallback
  //
  int
  Movie::demuxerInterruptCallback(void * context)
  {
    Movie * movie = (Movie *)context;
    if (movie->interruptDemuxer_)
    {
      return 1;
    }

    return 0;
  }

  //----------------------------------------------------------------
  // Movie::create_video_track
  //
  VideoTrackPtr
  Movie::create_video_track(AVStream * stream)
  {
    VideoTrackPtr video_track;

    do
    {
      // shortcut:
      AVCodecParameters & codecpar = *(stream->codecpar);
      const AVMediaType codec_type = codecpar.codec_type;
      YAE_ASSERT(codec_type ==  AVMEDIA_TYPE_VIDEO);

      // assume codec is unsupported,
      // discard all packets unless proven otherwise:
      stream->discard = AVDISCARD_ALL;

      // check whether we have a decoder for this codec:
      const AVCodec * decoder = avcodec_find_decoder(codecpar.codec_id);
      if (!decoder)
      {
        // unsupported codec, ignore it:
        codecpar.codec_type = AVMEDIA_TYPE_UNKNOWN;
        break;
      }

      TrackPtr base_track(new Track(context_, stream, hwdec_));
      if (codecpar.format == AV_PIX_FMT_NONE)
      {
        // keep-alive:
        AvCodecContextPtr codec_ctx_ptr = base_track->open();
        AVCodecContext * codec_ctx = codec_ctx_ptr.get();
        if (codec_ctx)
        {
          YAE_ASSERT(avcodec_parameters_from_context(stream->codecpar,
                                                     codec_ctx) >= 0);
          base_track->close();
        }
      }

      video_track.reset(new VideoTrack(base_track.get()));

      VideoTraits traits;
      if (video_track->getTraits(traits) &&
          // avfilter does not support these pixel formats:
          traits.pixelFormat_ != kPixelFormatUYYVYY411)
      {
        stream->discard = AVDISCARD_DEFAULT;
      }
      else
      {
        // unsupported codec, ignore it:
        codecpar.codec_type = AVMEDIA_TYPE_UNKNOWN;
      }
    }
    while (false);
    return video_track;
  }

  //----------------------------------------------------------------
  // Movie::create_audio_track
  //
  AudioTrackPtr
  Movie::create_audio_track(AVStream * stream)
  {
    AudioTrackPtr audio_track;

    do
    {
      // shortcut:
      AVCodecParameters & codecpar = *(stream->codecpar);
      const AVMediaType codec_type = codecpar.codec_type;
      YAE_ASSERT(codec_type ==  AVMEDIA_TYPE_AUDIO);

      // assume codec is unsupported,
      // discard all packets unless proven otherwise:
      stream->discard = AVDISCARD_ALL;

      // check whether we have a decoder for this codec:
      const AVCodec * decoder = avcodec_find_decoder(codecpar.codec_id);
      if (!decoder)
      {
        // unsupported codec, ignore it:
        codecpar.codec_type = AVMEDIA_TYPE_UNKNOWN;
        break;
      }

      TrackPtr base_track(new Track(context_, stream, hwdec_));
      audio_track.reset(new AudioTrack(base_track.get()));

      AudioTraits traits;
      if (audio_track->getTraits(traits))
      {
        stream->discard = AVDISCARD_DEFAULT;
      }
      else
      {
        // unsupported codec, ignore it:
        codecpar.codec_type = AVMEDIA_TYPE_UNKNOWN;
      }
    }
    while (false);
    return audio_track;
  }

  //----------------------------------------------------------------
  // Movie::create_subtt_track
  //
  SubttTrackPtr
  Movie::create_subtt_track(AVStream * stream)
  {
    SubttTrackPtr subtt_track;

    do
    {
      // shortcut:
      const AVCodecParameters & codecpar = *(stream->codecpar);
      const AVMediaType codec_type = codecpar.codec_type;
      YAE_ASSERT(codec_type ==  AVMEDIA_TYPE_SUBTITLE);

      // don't discard closed captions packets, though they don't
      // get to have their own stand-alone subtitles track;
      stream->discard = AVDISCARD_DEFAULT;

      // don't add CEA-608 as a single track...
      // because it's actually 4 channels
      // and it makes a poor user experience
      if (codecpar.codec_id != AV_CODEC_ID_NONE &&
          codecpar.codec_id != AV_CODEC_ID_EIA_608)
      {
        subtt_track.reset(new SubtitlesTrack(stream));

        if (strcmp(context_->iformat->name, "matroska,webm") == 0)
        {
          // https://matroska.org/technical/subtitles.html
          //
          // matroska container does not store ASS/SSA Events
          // as defined in the [Events] Format:, they remove
          // timing info and store some fields in fixed order:
          //   ReadOrder, Layer, Style, Name (or Actor),
          //   MarginL, MarginR, MarginV, Effect, Text
          //
          // ffmpeg mkv demuxer does not account for the (potential)
          // re-ordering of the [Events] Format: fields, it outputs
          // events as-is, without timing info, and in all likelihood
          // not matching the [Events] Format: defined in the
          // AVCodecContext.subtitle_header.  This makes the decoded
          // subtitles unusable with libass renderer.
          //
          // We will have to handle the re-ordering and adding the
          // timing info ourselves, in order to be able to process
          // these Events with libass

          subtt_track->setInputEventFormat("ReadOrder, Layer, Style, "
                                           "Name, MarginL, MarginR, "
                                           "MarginV, Effect, Text");
        }
      }
    }
    while (false);
    return subtt_track;
  }

  //----------------------------------------------------------------
  // Movie::extract_attachment
  //
  bool
  Movie::extract_attachment(const AVStream * stream,
                            std::vector<TAttachment> & attachments)
  {
    // shortcuts:
    const AVCodecParameters & codecpar = *(stream->codecpar);
    const AVMediaType codec_type = codecpar.codec_type;
    if (codec_type != AVMEDIA_TYPE_ATTACHMENT)
    {
      return false;
    }

    // extract attachments:
    attachments.push_back(TAttachment(codecpar.extradata,
                                      codecpar.extradata_size));
    TAttachment & att = attachments.back();

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

    return true;
  }

  //----------------------------------------------------------------
  // Movie::get_program_info
  //
  void
  Movie::get_program_info(std::vector<TProgramInfo> & program_infos,
                          std::map<int, int> & stream_ix_to_prog_ix)
  {
    for (unsigned int i = 0; i < context_->nb_programs; i++)
    {
      const AVProgram * p = context_->programs[i];
      program_infos.push_back(TProgramInfo());

      TProgramInfo & info = program_infos.back();
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
        stream_ix_to_prog_ix[streamIndex] = i;
      }
    }

    if (context_->nb_programs < 1)
    {
      // there must be at least 1 implied program:
      program_infos.push_back(TProgramInfo());

      for (unsigned int i = 0; i < context_->nb_streams; i++)
      {
        stream_ix_to_prog_ix[i] = 0;
      }
    }
  }

  //----------------------------------------------------------------
  // Movie::flatten_program_tracks_map
  //
  void
  Movie::flatten_program_tracks(const TProgramTracksLut & program_tracks_lut,
                                std::vector<TProgramInfo> & program_infos,
                                std::vector<VideoTrackPtr> & video_tracks,
                                std::vector<AudioTrackPtr> & audio_tracks,
                                std::vector<SubttTrackPtr> & subtt_tracks,
                                std::map<int, int> & stream_ix_to_subtt_ix)
  {
    // flatten program tracks map:
    for (TProgramTracksLut::const_iterator
           i = program_tracks_lut.begin(); i != program_tracks_lut.end(); ++i)
    {
      const int program_id = i->first;
      const ProgramTracks & program_tracks = i->second;

      TProgramInfo * program_info = NULL;
      for (std::size_t j = 0, n = program_infos.size(); j < n; j++)
      {
        if (program_infos[j].id_ == program_id)
        {
          program_info = &(program_infos[j]);
          break;
        }
      }

      if (!program_info)
      {
        continue;
      }

      // video:
      typedef std::map<int, std::list<VideoTrackPtr> > TVideoTracks;
      const TVideoTracks & video_track_map = program_tracks.video_;

      for (TVideoTracks::const_iterator
             j = video_track_map.begin(); j != video_track_map.end(); ++j)
      {
        // int pid = j->first;
        const std::list<VideoTrackPtr> & tracks = j->second;

        for (std::list<VideoTrackPtr>::const_iterator
               k = tracks.begin(); k != tracks.end(); ++k)
        {
          const VideoTrackPtr & track = *k;
          std::string track_id = make_track_id('v', video_tracks.size());
          track->set_track_id(track_id);
          program_info->video_.push_back(video_tracks.size());
          video_tracks.push_back(track);
        }
      }

      // audio:
      typedef std::map<int, std::list<AudioTrackPtr> > TAudioTracks;
      const TAudioTracks & audio_track_map = program_tracks.audio_;

      for (TAudioTracks::const_iterator
             j = audio_track_map.begin(); j != audio_track_map.end(); ++j)
      {
        // int pid = j->first;
        const std::list<AudioTrackPtr> & tracks = j->second;

        for (std::list<AudioTrackPtr>::const_iterator
               k = tracks.begin(); k != tracks.end(); ++k)
        {
          const AudioTrackPtr & track = *k;
          std::string track_id = make_track_id('a', audio_tracks.size());
          track->set_track_id(track_id);
          program_info->audio_.push_back(audio_tracks.size());
          audio_tracks.push_back(track);
        }
      }

      // subtt:
      typedef std::map<int, std::list<SubttTrackPtr> > TSubttTracks;
      const TSubttTracks & subtt_track_map = program_tracks.subtt_;

      for (TSubttTracks::const_iterator
             j = subtt_track_map.begin(); j != subtt_track_map.end(); ++j)
      {
        // int pid = j->first;
        const std::list<SubttTrackPtr> & tracks = j->second;

        for (std::list<SubttTrackPtr>::const_iterator
               k = tracks.begin(); k != tracks.end(); ++k)
        {
          const SubttTrackPtr & track = *k;
          std::string track_id = make_track_id('s', subtt_tracks.size());
          track->set_track_id(track_id);
          program_info->subtt_.push_back(subtt_tracks.size());
          stream_ix_to_subtt_ix[track->streamIndex()] = subtt_tracks.size();
          subtt_tracks.push_back(track);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // Movie::open
  //
  bool
  Movie::open(const char * resourcePath, bool hwdec)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();

    hwdec_ = hwdec;

    YAE_ASSERT(!context_);
    context_ = avformat_alloc_context();

    YAE_ASSERT(!interruptDemuxer_);
    context_->interrupt_callback.callback = &Movie::demuxerInterruptCallback;
    context_->interrupt_callback.opaque = this;

    AVDictionary * options = NULL;

    // set probesize to 128 MiB:
    av_dict_set(&options, "probesize", "134217728", 0);

    // set analyze duration to 20 seconds:
    av_dict_set(&options, "analyzeduration", "20000000", 0);

    // set AVFMT_FLAG_GENPTS:
    av_dict_set(&options, "fflags", "+genpts", AV_DICT_APPEND);

    // set AVFMT_FLAG_DISCARD_CORRUPT:
    av_dict_set(&options, "fflags", "+discardcorrupt", AV_DICT_APPEND);

    // copied from ffprobe.c -- scan and combine all PMTs:
    //  av_dict_set(&options, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

    resourcePath_ = resourcePath ? resourcePath : "";
    int err = avformat_open_input(&context_,
                                  resourcePath_.c_str(),
                                  NULL, // AVInputFormat to force
                                  &options);
    av_dict_free(&options);

    if (err != 0)
    {
      close();
      return false;
    }

    YAE_ASSERT((context_->flags & AVFMT_FLAG_GENPTS) ==
               AVFMT_FLAG_GENPTS);

    YAE_ASSERT((context_->flags & AVFMT_FLAG_DISCARD_CORRUPT) ==
               AVFMT_FLAG_DISCARD_CORRUPT);

    err = avformat_find_stream_info(context_, NULL);
    if (err < 0)
    {
      close();
      return false;
    }

    // get the file size, so we can implement byte-position seeking:
    file_size_ = avio_size(context_->pb);

    // get the programs:
    std::vector<TProgramInfo> program_infos;
    std::map<int, int> stream_ix_to_prog_ix;
    this->get_program_info(program_infos, stream_ix_to_prog_ix);

    // sort tracks by PID if applicable (AVStream.id):
    TProgramTracksLut program_tracks_lut;
    std::vector<TAttachment> attachments;
    std::vector<VideoTrackPtr> video_tracks;
    std::vector<AudioTrackPtr> audio_tracks;
    std::vector<SubttTrackPtr> subtt_tracks;

    for (unsigned int i = 0; i < context_->nb_streams; i++)
    {
      // shortcuts:
      AVStream * stream = context_->streams[i];
      const AVCodecParameters & codecpar = *(stream->codecpar);
      const AVMediaType codec_type = codecpar.codec_type;

      if (codecpar.codec_type == AVMEDIA_TYPE_ATTACHMENT)
      {
        this->extract_attachment(stream, attachments);
        continue;
      }

      // lookup which program this stream belongs to:
      TProgramInfo * program = NULL;
      {
        std::map<int, int>::const_iterator
          found = stream_ix_to_prog_ix.find(i);

        if (found != stream_ix_to_prog_ix.end())
        {
          program = &program_infos[found->second];
        }
      }

      if (!program)
      {
        continue;
      }

      // group tracks by program:
      ProgramTracks & program_tracks = program_tracks_lut[program->id_];

      if (codec_type == AVMEDIA_TYPE_VIDEO)
      {
        VideoTrackPtr track = this->create_video_track(stream);
        if (!track)
        {
          continue;
        }

        program_tracks.video_[stream->id].push_back(track);
      }
      else if (codec_type == AVMEDIA_TYPE_AUDIO)
      {
        AudioTrackPtr track = this->create_audio_track(stream);
        if (!track)
        {
          continue;
        }

        program_tracks.audio_[stream->id].push_back(track);
      }
      else if (codec_type == AVMEDIA_TYPE_SUBTITLE)
      {
        SubttTrackPtr track = this->create_subtt_track(stream);
        if (!track)
        {
          continue;
        }

        program_tracks.subtt_[stream->id].push_back(track);
      }
      else
      {
        // assume codec is unsupported,
        // discard all packets unless proven otherwise:
        stream->discard = AVDISCARD_ALL;
      }
    }

    std::map<int, int> stream_ix_to_subtt_ix;
    this->flatten_program_tracks(program_tracks_lut,
                                 program_infos,
                                 video_tracks,
                                 audio_tracks,
                                 subtt_tracks,
                                 stream_ix_to_subtt_ix);

    if (video_tracks.empty() &&
        audio_tracks.empty())
    {
      // no decodable video/audio tracks present:
      close();
      return false;
    }

    // FIXME: pkoshevoy: this needs a mutex?
    attachments_.swap(attachments);
    program_infos_.swap(program_infos);
    video_tracks_.swap(video_tracks);
    audio_tracks_.swap(audio_tracks);
    subtt_tracks_.swap(subtt_tracks);
    stream_ix_to_prog_ix_.swap(stream_ix_to_prog_ix);
    stream_ix_to_subtt_ix_.swap(stream_ix_to_subtt_ix);

    // by default do not select any tracks:
    selectedVideoTrack_ = video_tracks_.size();
    selectedAudioTrack_ = audio_tracks_.size();

    return true;
  }

  //----------------------------------------------------------------
  // find
  //
  template <typename TTrack>
  boost::shared_ptr<TTrack>
  find(const std::vector<boost::shared_ptr<TTrack> > & tracks,
       int stream_index)
  {
    for (std::size_t i = 0, n = tracks.size(); i < n; ++i)
    {
      const boost::shared_ptr<TTrack> & track_ptr = tracks[i];
      const TTrack & track = *track_ptr;
      const AVStream & stream = track.stream();
      if (stream.index == stream_index)
      {
        return track_ptr;
      }
    }

    // not found:
    return boost::shared_ptr<TTrack>();
  }

  //----------------------------------------------------------------
  // Movie::refresh
  //
  void
  Movie::refresh()
  {
    // refresh the programs:
    std::vector<TProgramInfo> program_infos;
    std::map<int, int> stream_ix_to_prog_ix;
    this->get_program_info(program_infos, stream_ix_to_prog_ix);

    // sort tracks by PID if applicable (AVStream.id):
    TProgramTracksLut program_tracks_lut;
    std::vector<TAttachment> attachments;
    std::vector<VideoTrackPtr> video_tracks;
    std::vector<AudioTrackPtr> audio_tracks;
    std::vector<SubttTrackPtr> subtt_tracks;

    for (unsigned int i = 0; i < context_->nb_streams; i++)
    {
      // shortcuts:
      AVStream * stream = context_->streams[i];
      const AVCodecParameters & codecpar = *(stream->codecpar);
      const AVMediaType codec_type = codecpar.codec_type;

      if (codecpar.codec_type == AVMEDIA_TYPE_ATTACHMENT)
      {
        this->extract_attachment(stream, attachments);
        continue;
      }

      // lookup which program this stream belongs to:
      TProgramInfo * program = NULL;
      {
        std::map<int, int>::const_iterator
          found = stream_ix_to_prog_ix.find(i);

        if (found != stream_ix_to_prog_ix.end())
        {
          program = &program_infos[found->second];
        }
      }

      if (!program)
      {
        continue;
      }

      // group tracks by program:
      ProgramTracks & program_tracks = program_tracks_lut[program->id_];

      // check if we already have a Track for this AVStream:
      if (codec_type == AVMEDIA_TYPE_VIDEO)
      {
        VideoTrackPtr track = yae::find(video_tracks_, stream->index);

        if (!track)
        {
          track = create_video_track(stream);
        }

        if (!track)
        {
          continue;
        }

        program_tracks.video_[stream->id].push_back(track);
      }
      else if (codec_type == AVMEDIA_TYPE_AUDIO)
      {
        AudioTrackPtr track = yae::find(audio_tracks_, stream->index);

        if (!track)
        {
          track = create_audio_track(stream);
        }

        if (!track)
        {
          continue;
        }

        program_tracks.audio_[stream->id].push_back(track);
      }
      else if (codec_type == AVMEDIA_TYPE_SUBTITLE)
      {
        SubttTrackPtr track = yae::find(subtt_tracks_, stream->index);

        if (!track)
        {
          track = create_subtt_track(stream);
        }

        if (!track)
        {
          continue;
        }

        program_tracks.subtt_[stream->id].push_back(track);
      }
    }

    std::map<int, int> stream_ix_to_subtt_ix;
    this->flatten_program_tracks(program_tracks_lut,
                                 program_infos,
                                 video_tracks,
                                 audio_tracks,
                                 subtt_tracks,
                                 stream_ix_to_subtt_ix);

    if (video_tracks.empty() &&
        audio_tracks.empty())
    {
      // no decodable video/audio tracks present:
      close();
      return;
    }

    // FIXME: pkoshevoy: this needs a mutex?
    attachments_.swap(attachments);
    program_infos_.swap(program_infos);
    video_tracks_.swap(video_tracks);
    audio_tracks_.swap(audio_tracks);
    subtt_tracks_.swap(subtt_tracks);
    stream_ix_to_prog_ix_.swap(stream_ix_to_prog_ix);
    stream_ix_to_subtt_ix_.swap(stream_ix_to_subtt_ix);

    this->selectVideoTrack(selectedVideoTrack_);
    this->selectAudioTrack(selectedAudioTrack_);
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

    const std::size_t numVideoTracks = video_tracks_.size();
    selectVideoTrack(numVideoTracks);

    const std::size_t numAudioTracks = audio_tracks_.size();
    selectAudioTrack(numAudioTracks);

    attachments_.clear();
    video_tracks_.clear();
    audio_tracks_.clear();
    subtt_tracks_.clear();
    stream_ix_to_subtt_ix_.clear();
    program_infos_.clear();
    stream_ix_to_prog_ix_.clear();

    avformat_close_input(&context_);
    YAE_ASSERT(!context_);

    // reset last known AVPacket.pos:
    packet_pos_ = -1;
    file_size_ = 0;
  }

  //----------------------------------------------------------------
  // Movie::getVideoTrackInfo
  //
  bool
  Movie::getVideoTrackInfo(std::size_t i, TTrackInfo & info) const
  {
    info.nprograms_ = context_ ? context_->nb_programs : 0;
    info.program_ = info.nprograms_;
    info.ntracks_ = video_tracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      VideoTrackPtr t = video_tracks_[info.index_];

      // keep alive:
      Track::TInfoPtr track_info_ptr = t->get_info();
      const Track::Info & track_info = *track_info_ptr;

      info.setLang(track_info.lang_);
      info.setName(track_info.name_);
      info.program_ = get(stream_ix_to_prog_ix_, t->streamIndex());
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // Movie::getAudioTrackInfo
  //
  bool
  Movie::getAudioTrackInfo(std::size_t i, TTrackInfo & info) const
  {
    info.nprograms_ = context_ ? context_->nb_programs : 0;
    info.program_ =  info.nprograms_;
    info.ntracks_ = audio_tracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      AudioTrackPtr t = audio_tracks_[info.index_];

      // keep alive:
      Track::TInfoPtr track_info_ptr = t->get_info();
      const Track::Info & track_info = *track_info_ptr;

      info.setLang(track_info.lang_);
      info.setName(track_info.name_);
      info.program_ = get(stream_ix_to_prog_ix_, t->streamIndex());
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // Movie::selectVideoTrack
  //
  bool
  Movie::selectVideoTrack(std::size_t i)
  {
    const std::size_t numVideoTracks = video_tracks_.size();
    if (selectedVideoTrack_ < numVideoTracks)
    {
      // close currently selected track:
      VideoTrackPtr track = video_tracks_[selectedVideoTrack_];
      track->close();
    }

    selectedVideoTrack_ = i;
    if (selectedVideoTrack_ >= numVideoTracks)
    {
      return false;
    }

    VideoTrackPtr track = video_tracks_[selectedVideoTrack_];
    track->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
    track->skipLoopFilter(skipLoopFilter_);
    track->skipNonReferenceFrames(skipNonReferenceFrames_);
    track->enableClosedCaptions(enableClosedCaptions_);
    track->setSubs(&subtt_tracks_);

    return track->initTraits();
  }

  //----------------------------------------------------------------
  // Movie::selectAudioTrack
  //
  bool
  Movie::selectAudioTrack(std::size_t i)
  {
    const std::size_t numAudioTracks = audio_tracks_.size();
    if (selectedAudioTrack_ < numAudioTracks)
    {
      // close currently selected track:
      AudioTrackPtr track = audio_tracks_[selectedAudioTrack_];
      track->close();
    }

    selectedAudioTrack_ = i;
    if (selectedAudioTrack_ >= numAudioTracks)
    {
      return false;
    }

    AudioTrackPtr track = audio_tracks_[selectedAudioTrack_];
    track->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);

    return track->initTraits();
  }

  //----------------------------------------------------------------
  // Movie::thread_loop
  //
  void
  Movie::thread_loop()
  {
    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < video_tracks_.size())
    {
      videoTrack = video_tracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      audioTrack = audio_tracks_[selectedAudioTrack_];
    }

    PacketQueueCloseOnExit videoCloseOnExit(videoTrack);
    PacketQueueCloseOnExit audioCloseOnExit(audioTrack);

    try
    {
      int err_count = 0;
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
        TPacketPtr packetPtr(new AvPkt());
        AVPacket & packet = packetPtr->get();
        bool demuxerInterrupted = false;
        {
          boost::lock_guard<boost::timed_mutex> lock(mutex_);

          if (mustStop_)
          {
            break;
          }

          if (seekPos_)
          {
            bool dropPendingFrames = true;
            err = seekTo(seekPos_, dropPendingFrames);
            seekPos_.reset();
          }

          if (!err)
          {
            err = av_read_frame(context_, &packet);

            if (interruptDemuxer_)
            {
              demuxerInterrupted = true;
              interruptDemuxer_ = false;
            }
          }
        }

        if (!err)
        {
          err_count = 0;

          if (packet.pos != -1)
          {
            packet_pos_ = packet.pos;
          }
          else if (packet_pos_ != -1)
          {
            packet.pos = packet_pos_;
          }
        }
        else
        {
          if (!playbackEnabled_ && err == AVERROR_EOF)
          {
            // avoid constantly rewinding when playback is paused,
            // slow down a little:
            boost::this_thread::interruption_point();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(333));
          }
#ifndef NDEBUG
          else
          {
            yae_elog("AVERROR: %s", yae::av_errstr(err).c_str());
          }
#endif

          if (err != AVERROR_EOF)
          {
            err_count++;
            if (err_count > 100)
            {
              break;
            }

            // keep trying, it may be able recover:
            err = 0;
            continue;
          }

          if (demuxerInterrupted)
          {
            boost::this_thread::yield();
            err = 0;
            continue;
          }

          if (audioTrack)
          {
            // flush out buffered frames with an empty packet:
            audioTrack->packetQueuePush(TPacketPtr(), &outputTerminator_);
          }

          if (videoTrack)
          {
            // flush out buffered frames with an empty packet:
            videoTrack->packetQueuePush(TPacketPtr(), &outputTerminator_);
          }

          if (!playbackEnabled_)
          {
            // during framestep do not stop when end of file is reached,
            // simply rewind to the beginning:
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          // it appears playback has finished, unless user starts
          // framestep (playback disabled) while this thread waits
          // for all queues to empty:
          if (audioTrack)
          {
            audioTrack->packetQueueWaitForConsumerToBlock();
            audioTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
          }

          if (videoTrack)
          {
            videoTrack->packetQueueWaitForConsumerToBlock();
            videoTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
          }

          // check whether user tried to seek
          // while we were waiting for the queues:
          if (seekPos_)
          {
            err = 0;
            continue;
          }

          // check whether user disabled playback while
          // we were waiting for the queues:
          if (!playbackEnabled_)
          {
            // during framestep do not stop when end of file is reached,
            // simply rewind to the beginning:
            err = rewind(audioTrack, videoTrack, false);
            continue;
          }

          // check whether user enabled playback looping while
          // we were waiting for the queues:
          if (looping_)
          {
            err = rewind(audioTrack, videoTrack);
            continue;
          }

          break;
        }

        AVStream * stream =
          packet.stream_index < int(context_->nb_streams) ?
          context_->streams[packet.stream_index] :
          NULL;

        if (adjustTimestamps_)
        {
          int ref_stream_index =
            videoTrack ? videoTrack->streamIndex() :
            audioTrack ? audioTrack->streamIndex() :
            -1;

          int ref_prog_index =
            ref_stream_index != -1 ?
            get(stream_ix_to_prog_ix_, ref_stream_index) :
            -1;

          int pkt_prog_index =
            get(stream_ix_to_prog_ix_, packet.stream_index);

          if (ref_prog_index == pkt_prog_index)
          {
            adjustTimestamps_(adjustTimestampsCtx_, context_, &packet);
          }
          else
          {
            YAE_EXPECT(videoTrack->streamIndex() != packet.stream_index);
            continue;
          }
        }

#if 0 // ndef NDEBUG
        for (int i = 0; i < packet.side_data_elems; i++)
        {
          const AVPacketSideData & side_data = packet.side_data[i];
          yae_dlog("stream %i packet side data %i: %s, size %i",
                   packet.stream_index,
                   i,
                   av_packet_side_data_name(side_data.type),
                   side_data.size);
        }
#endif
        if (videoTrack &&
            videoTrack->streamIndex() == packet.stream_index)
        {
          if (!videoTrack->packetQueuePush(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else if (audioTrack &&
                 audioTrack->streamIndex() == packet.stream_index)
        {
          if (!audioTrack->packetQueuePush(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else
        {
          SubtitlesTrack * subs = this->subsLookup(packet.stream_index);
          CaptionsDecoder * cc = videoTrack ? &(videoTrack->cc_) : NULL;
          process_subs_and_cc(stream, packet, subs, cc, outputTerminator_);
        }
      }
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      yae_debug
        << "\nMovie::thread_loop caught exception: " << e.what() << "\n";
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      yae_debug
        << "\nMovie::thread_loop caught unexpected exception\n";
#endif
    }

#if 0 // ndef NDEBUG
    yae_debug
      << "\nMovie::thread_loop terminated\n";
#endif
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

    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      mustStop_ = false;
    }
    catch (...)
    {}

    if (selectedVideoTrack_ < video_tracks_.size())
    {
      VideoTrackPtr t = video_tracks_[selectedVideoTrack_];
      t->threadStart();
      t->packetQueueWaitForConsumerToBlock();
    }

    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      AudioTrackPtr t = audio_tracks_[selectedAudioTrack_];
      t->threadStart();
      t->packetQueueWaitForConsumerToBlock();
    }

    outputTerminator_.stopWaiting(false);
    framestepTerminator_.stopWaiting(!playbackEnabled_);
    return thread_.run();
  }

  //----------------------------------------------------------------
  // Movie::threadStop
  //
  bool
  Movie::threadStop()
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      mustStop_ = true;
    }
    catch (...)
    {}

    if (selectedVideoTrack_ < video_tracks_.size())
    {
      VideoTrackPtr t = video_tracks_[selectedVideoTrack_];
      t->threadStop();
    }

    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      AudioTrackPtr t = audio_tracks_[selectedAudioTrack_];
      t->threadStop();
    }

    outputTerminator_.stopWaiting(true);
    framestepTerminator_.stopWaiting(true);

    thread_.interrupt();
    return thread_.wait();
  }

  //----------------------------------------------------------------
  // Movie::isSeekable
  //
  bool
  Movie::isSeekable() const
  {
    if (!context_ || !context_->pb || !context_->pb->seekable)
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Movie::hasDuration
  //
  bool
  Movie::hasDuration() const
  {
    TTime start;
    TTime duration;

    if (selectedVideoTrack_ < video_tracks_.size())
    {
      VideoTrackPtr t = video_tracks_[selectedVideoTrack_];
      if (t->getDuration(start, duration))
      {
        return true;
      }
    }

    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      AudioTrackPtr t = audio_tracks_[selectedAudioTrack_];
      if (t->getDuration(start, duration))
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Movie::requestSeekTime
  //
  bool
  Movie::requestSeek(const TSeekPosPtr & seekPos)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      seekPos_ = seekPos;

      VideoTrackPtr videoTrack;
      AudioTrackPtr audioTrack;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        videoTrack = video_tracks_[selectedVideoTrack_];
        videoTrack->packetQueueClear();
        do { videoTrack->frameQueueClear(); }
        while (!videoTrack->packetQueueWaitForConsumerToBlock(1e-2));
        videoTrack->frameQueueClear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
       yae_debug << "\n\tCLEAR VIDEO FRAME QUEUE for seek: "
                 << seekPos->to_str() << "\n";
#endif
      }

      if (selectedAudioTrack_ < audio_tracks_.size())
      {
        audioTrack = audio_tracks_[selectedAudioTrack_];
        audioTrack->packetQueueClear();
        do { audioTrack->frameQueueClear(); }
        while (!audioTrack->packetQueueWaitForConsumerToBlock(1e-2));
        audioTrack->frameQueueClear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        yae_debug << "\n\tCLEAR AUDIO FRAME QUEUE for seek: "
                  << seekPos->to_str() << "\n";
#endif
      }

      return true;
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::setAdjustTimestamps
  //
  void
  Movie::setAdjustTimestamps(TAdjustTimestamps cb, void * context)
  {
    adjustTimestamps_ = cb;
    adjustTimestampsCtx_ = context;
  }

  //----------------------------------------------------------------
  // Movie::seekTo
  //
  int
  Movie::seekTo(const TSeekPosPtr & pos, bool dropPendingFrames)
  {
    if (!context_)
    {
      return AVERROR_UNKNOWN;
    }

    if (!isSeekable())
    {
      // don't bother attempting to seek an un-seekable stream:
      return 0;
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      audioTrack = audio_tracks_[selectedAudioTrack_];
    }

    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < video_tracks_.size())
    {
      videoTrack = video_tracks_[selectedVideoTrack_];
    }

    const AVStream * stream = NULL;
    if ((context_->iformat->flags & AVFMT_TS_DISCONT) &&
        strcmp(context_->iformat->name, "ogg") != 0 &&
        (audioTrack || videoTrack))
    {
      int streamIndex =
        videoTrack ? videoTrack->streamIndex() : audioTrack->streamIndex();
      if (streamIndex >= 0)
      {
        stream = context_->streams[streamIndex];
      }
    }

    // reset last known AVPacket.pos:
    packet_pos_ = -1;

    int err = pos->seek(context_, stream);
    if (err < 0)
    {
#ifndef NDEBUG
      yae_debug
        << "avformat_seek_file (" << pos->to_str() << ") returned "
        << yae::av_errstr(err)
        << "\n";
#endif
      return err;
    }

    if (videoTrack)
    {
      err = videoTrack->resetTimeCounters(pos, dropPendingFrames);
    }

    if (!err && audioTrack)
    {
      err = audioTrack->resetTimeCounters(pos, dropPendingFrames);
    }

    clock_.cancelWaitForOthers();
    clock_.resetCurrentTime();

    const std::size_t nsubs = subtt_tracks_.size();
    for (std::size_t i = 0; i < nsubs; i++)
    {
      SubtitlesTrack & subs = *(subtt_tracks_[i]);
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
                bool seekToInPoint)
  {
    // wait for the the frame queues to empty out:
    if (audioTrack)
    {
      audioTrack->packetQueueWaitForConsumerToBlock();
      audioTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
    }

    if (videoTrack)
    {
      videoTrack->packetQueueWaitForConsumerToBlock();
      videoTrack->frameQueueWaitForConsumerToBlock(&framestepTerminator_);
    }

    boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
    requestMutex(lock);

    if (mustStop_)
    {
      // user must have switched to another item in the playlist:
      return AVERROR(EAGAIN);
    }

    static const TSeekPosPtr zeroTime(new TimePos(0.0));
    const TSeekPosPtr & seekPos = seekToInPoint ? posIn_ : zeroTime;
    bool dropPendingFrames = false;
    return seekTo(seekPos, dropPendingFrames);
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalStart
  //
  void
  Movie::setPlaybackIntervalStart(const TSeekPosPtr & posIn)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      posIn_ = posIn;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
      }

      if (selectedAudioTrack_ < audio_tracks_.size())
      {
        AudioTrackPtr audioTrack = audio_tracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalEnd
  //
  void
  Movie::setPlaybackIntervalEnd(const TSeekPosPtr & posOut)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      posOut_ = posOut;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
      }

      if (selectedAudioTrack_ < audio_tracks_.size())
      {
        AudioTrackPtr audioTrack = audio_tracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackEnabled
  //
  void
  Movie::setPlaybackEnabled(bool enabled)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      playbackEnabled_ = enabled;
      clock_.setRealtime(playbackEnabled_);
      framestepTerminator_.stopWaiting(!playbackEnabled_);

      if (playbackEnabled_ && looping_)
      {
        if (selectedVideoTrack_ < video_tracks_.size())
        {
          VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
          videoTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
        }

        if (selectedAudioTrack_ < audio_tracks_.size())
        {
          AudioTrackPtr audioTrack = audio_tracks_[selectedAudioTrack_];
          audioTrack->setPlaybackInterval(posIn_, posOut_, playbackEnabled_);
        }
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      skipLoopFilter_ = skip;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      skipNonReferenceFrames_ = skip;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      // first set audio tempo -- this may fail:
      if (selectedAudioTrack_ < audio_tracks_.size())
      {
        AudioTrackPtr audioTrack = audio_tracks_[selectedAudioTrack_];
        if (!audioTrack->setTempo(tempo))
        {
          return false;
        }
      }

      // then set video tempo -- this can't fail:
      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
        videoTrack->setDeinterlacing(enabled);
        return true;
      }
    }
    catch (...)
    {}

    return false;
  }

  //----------------------------------------------------------------
  // Movie::setRenderCaptions
  //
  void
  Movie::setRenderCaptions(unsigned int cc)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      enableClosedCaptions_ = cc;

      if (selectedVideoTrack_ < video_tracks_.size())
      {
        VideoTrackPtr videoTrack = video_tracks_[selectedVideoTrack_];
        videoTrack->enableClosedCaptions(cc);
      }
    }
    catch (...)
    {}
  }

  //----------------------------------------------------------------
  // Movie::getRenderCaptions
  //
  unsigned int
  Movie::getRenderCaptions() const
  {
    return enableClosedCaptions_;
  }

  //----------------------------------------------------------------
  // Movie::subsCount
  //
  std::size_t
  Movie::subsCount() const
  {
    return subtt_tracks_.size();
  }

  //----------------------------------------------------------------
  // Movie::subsInfo
  //
  TSubsFormat
  Movie::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    info.nprograms_ = context_ ? context_->nb_programs : 0;
    info.program_ = info.nprograms_;
    info.ntracks_ = subtt_tracks_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      SubttTrackPtr t = subtt_tracks_[i];

      // keep alive:
      Track::TInfoPtr track_info_ptr = t->get_info();
      const Track::Info & track_info = *track_info_ptr;

      info.setLang(track_info.lang_);
      info.setName(track_info.name_);
      info.program_ = get(stream_ix_to_prog_ix_, t->streamIndex());
      return t->format_;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // Movie::setSubsRender
  //
  void
  Movie::setSubsRender(std::size_t i, bool render)
  {
    std::size_t nsubs = subtt_tracks_.size();
    if (i < nsubs)
    {
      SubtitlesTrack & subs = *(subtt_tracks_[i]);
      subs.render_ = render;
    }
  }

  //----------------------------------------------------------------
  // Movie::getSubsRender
  //
  bool
  Movie::getSubsRender(std::size_t i) const
  {
    std::size_t nsubs = subtt_tracks_.size();
    if (i < nsubs)
    {
      SubtitlesTrack & subs = *(subtt_tracks_[i]);
      return subs.render_;
    }

    return false;
  }

  //----------------------------------------------------------------
  // Movie::subsLookup
  //
  SubtitlesTrack *
  Movie::subsLookup(unsigned int streamIndex)
  {
    std::map<int, int>::const_iterator
      found = stream_ix_to_subtt_ix_.find(int(streamIndex));

    if (found != stream_ix_to_subtt_ix_.end())
    {
      return subtt_tracks_[found->second].get();
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

    c.span_.t0_.reset(av->time_base.num * av->start,
                      av->time_base.den);

    c.span_.t1_.reset(av->time_base.num * av->end,
                      av->time_base.den);

    return true;
  }

  //----------------------------------------------------------------
  // Movie::blockedOnVideo
  //
  bool
  Movie::blockedOnVideo() const
  {
    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < video_tracks_.size())
    {
      videoTrack = video_tracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      audioTrack = audio_tracks_[selectedAudioTrack_];
    }

    bool blocked = blockedOn(videoTrack.get(), audioTrack.get());

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      yae_debug << "BLOCKED ON VIDEO\n";
    }
#endif

    return blocked;
  }

  //----------------------------------------------------------------
  // Movie::blockedOnAudio
  //
  bool
  Movie::blockedOnAudio() const
  {
    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < video_tracks_.size())
    {
      videoTrack = video_tracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audio_tracks_.size())
    {
      audioTrack = audio_tracks_[selectedAudioTrack_];
    }

    bool blocked = blockedOn(audioTrack.get(), videoTrack.get());

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      yae_debug << "BLOCKED ON AUDIO\n";
    }
#endif

    return blocked;
  }

  //----------------------------------------------------------------
  // Movie::setSharedClock
  //
  void
  Movie::setSharedClock(const SharedClock & clock)
  {
    boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
    requestMutex(lock);

    clock_.cancelWaitForOthers();
    clock_ = clock;
    clock_.setMasterClock(clock_);
    clock_.setRealtime(playbackEnabled_);
  }

  //----------------------------------------------------------------
  // Movie::setEventObserver
  //
  void
  Movie::setEventObserver(const TEventObserverPtr & eo)
  {
    eo_ = eo;

    for (std::size_t i = 0, n = video_tracks_.size(); i < n; ++i)
    {
      VideoTrack & track = *(video_tracks_[i]);
      track.setEventObserver(eo);
    }

    for (std::size_t i = 0, n = audio_tracks_.size(); i < n; ++i)
    {
      AudioTrack & track = *(audio_tracks_[i]);
      track.setEventObserver(eo);
    }

    for (std::size_t i = 0, n = subtt_tracks_.size(); i < n; ++i)
    {
      SubtitlesTrack & track = *(subtt_tracks_[i]);
      track.setEventObserver(eo);
    }
  }

}
