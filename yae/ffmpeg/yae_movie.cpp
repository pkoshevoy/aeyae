// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu May 27 21:03:47 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// yae incudes:
#include "yae_closed_captions.h"
#include "yae_movie.h"


namespace yae
{

  //----------------------------------------------------------------
  // c608
  //
  const unsigned int c608 = MKTAG('c', '6', '0', '8');

  //----------------------------------------------------------------
  // c708
  //
  const unsigned int c708 = MKTAG('c', '7', '0', '8');

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
  // ensure_ffmpeg_initialized
  //
  static void
  ensure_ffmpeg_initialized()
  {
    // flag indicating whether av_register_all has been called already:
    static bool ffmpeg_initialized = false;

    if (!ffmpeg_initialized)
    {
      av_log_set_flags(AV_LOG_SKIP_REPEATED);
      avfilter_register_all();
      av_register_all();

      avformat_network_init();

      av_lockmgr_register(&lockManager);
      ffmpeg_initialized = true;
    }
  }


  //----------------------------------------------------------------
  // Movie::Movie
  //
  Movie::Movie():
    thread_(this),
    context_(NULL),
    selectedVideoTrack_(0),
    selectedAudioTrack_(0),
    skipLoopFilter_(false),
    skipNonReferenceFrames_(false),
    enableClosedCaptions_(0),
    dtsStreamIndex_(-1),
    dtsBytePos_(0),
    dts_(AV_NOPTS_VALUE),
    timeIn_(0.0),
    timeOut_(kMaxDouble),
    interruptDemuxer_(false),
    playbackEnabled_(false),
    looping_(false),
    mustStop_(true),
    mustSeek_(false),
    seekTime_(0.0),
    videoQueueSize_("video_queue_size"),
    audioQueueSize_("audio_queue_size")
  {
    ensure_ffmpeg_initialized();

    settings_.traits().addSetting(&videoQueueSize_);
    settings_.traits().addSetting(&audioQueueSize_);

    videoQueueSize_.traits().setValueMin(1);
    videoQueueSize_.traits().setValue(kQueueSizeSmall);

    audioQueueSize_.traits().setValueMin(1);
    audioQueueSize_.traits().setValue(kQueueSizeMedium);
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
  // Movie::open
  //
  bool
  Movie::open(const char * resourcePath)
  {
    // FIXME: avoid closing/reopening the same resource:
    close();

    YAE_ASSERT(!context_);
    context_ = avformat_alloc_context();

    YAE_ASSERT(!interruptDemuxer_);
    context_->interrupt_callback.callback = &Movie::demuxerInterruptCallback;
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
        std::map<int, int>::const_iterator
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
          videoTracks_.push_back(track);
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
          audioTracks_.push_back(track);
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

        // don't discard closed captions packets:
        stream->discard = AVDISCARD_DEFAULT;

        // don't add CEA-608 as a single track...
        // because it's actually 4 channels
        // and it makes a poor user experience
        if (stream->codecpar->codec_id != AV_CODEC_ID_NONE &&
            stream->codecpar->codec_id != AV_CODEC_ID_EIA_608)
        {
          program->subs_.push_back(subs_.size());
          subsIdx_[i] = subs_.size();
          TSubsTrackPtr subsTrk(new SubtitlesTrack(stream, subs_.size()));
          subs_.push_back(subsTrk);
        }
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

    attachments_.clear();
    videoTracks_.clear();
    audioTracks_.clear();
    subs_.clear();
    subsIdx_.clear();
    programs_.clear();
    streamIndexToProgramIndex_.clear();

    avformat_close_input(&context_);
  }

  //----------------------------------------------------------------
  // Movie::getVideoTrackInfo
  //
  void
  Movie::getVideoTrackInfo(std::size_t i, TTrackInfo & info) const
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
      info.program_ = get(streamIndexToProgramIndex_, t->stream().index);
    }
  }

  //----------------------------------------------------------------
  // Movie::getAudioTrackInfo
  //
  void
  Movie::getAudioTrackInfo(std::size_t i, TTrackInfo & info) const
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
      info.program_ = get(streamIndexToProgramIndex_, t->stream().index);
    }
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
    track->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
    track->skipLoopFilter(skipLoopFilter_);
    track->skipNonReferenceFrames(skipNonReferenceFrames_);
    track->enableClosedCaptions(enableClosedCaptions_);
    track->setSubs(&subs_);
    track->frameQueue_.setMaxSize(videoQueueSize_.traits().value());

    return track->initTraits();
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
    track->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
    track->frameQueue_.setMaxSize(audioQueueSize_.traits().value());

    return track->initTraits();
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
        TPacketPtr packetPtr(new AvPkt());
        AvPkt & packet = *packetPtr;
        bool demuxerInterrupted = false;
        {
          boost::lock_guard<boost::timed_mutex> lock(mutex_);

          if (mustStop_)
          {
            break;
          }

          if (mustSeek_)
          {
            bool dropPendingFrames = true;
            err = seekTo(seekTime_, dropPendingFrames);
            mustSeek_ = false;
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

        if (err)
        {
          if (!playbackEnabled_ && err == AVERROR_EOF)
          {
            // avoid constantly rewinding when playback is paused,
            // slow down a little:
            boost::this_thread::interruption_point();
            boost::this_thread::sleep(boost::posix_time::milliseconds(333));
          }
#ifndef NDEBUG
          else
          {
            dump_averror(std::cerr, err);
          }
#endif
          if (demuxerInterrupted)
          {
            boost::this_thread::yield();
            err = 0;
            continue;
          }

          if (audioTrack)
          {
            // flush out buffered frames with an empty packet:
            audioTrack->packetQueue().push(TPacketPtr(), &outputTerminator_);
          }

          if (videoTrack)
          {
            // flush out buffered frames with an empty packet:
            videoTrack->packetQueue().push(TPacketPtr(), &outputTerminator_);
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
            audioTrack->packetQueue().
              waitIndefinitelyForConsumerToBlock();

            audioTrack->frameQueue_.
              waitIndefinitelyForConsumerToBlock(&framestepTerminator_);
          }

          if (videoTrack)
          {
            videoTrack->packetQueue().
              waitIndefinitelyForConsumerToBlock();

            videoTrack->frameQueue_.
              waitIndefinitelyForConsumerToBlock(&framestepTerminator_);
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

        if (packet.dts != AV_NOPTS_VALUE)
        {
          // keep track of current DTS, so that we would know which way to seek
          // relative to the current position (back/forth)
          dts_ = packet.dts;
          dtsBytePos_ = packet.pos;
          dtsStreamIndex_ = packet.stream_index;
        }

        if (videoTrack &&
            videoTrack->streamIndex() == packet.stream_index)
        {
          if (!videoTrack->packetQueue().push(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else if (audioTrack &&
                 audioTrack->streamIndex() == packet.stream_index)
        {
          if (!audioTrack->packetQueue().push(packetPtr, &outputTerminator_))
          {
            break;
          }
        }
        else
        {
          AVStream * stream =
            packet.stream_index < context_->nb_streams ?
            context_->streams[packet.stream_index] :
            NULL;

          bool closedCaptions = false;
          if (stream)
          {
            if (stream->codecpar->codec_tag == c608)
            {
              // convert to CEA708 packets wrapping CEA608 data, it's
              // the only format ffmpeg captions decoder understands:
              closedCaptions = convert_quicktime_c608(packet);
            }
            else if (stream->codecpar->codec_tag == c708)
            {
              // convert to CEA708 packets wrapping CEA608 data, it's
              // the only format ffmpeg captions decoder understands:
              closedCaptions = convert_quicktime_c708(packet);
            }
          }

          SubtitlesTrack * subs = NULL;
          if (stream && videoTrack &&
              (closedCaptions || (subs = subsLookup(packet.stream_index))))
          {
            static const Rational tb(1, AV_TIME_BASE);

            // shortcut:
            AVCodecContext * subsDec = subs ? subs->codecContext_ : NULL;

            TSubsFrame sf;
            sf.time_.time_ = av_rescale_q(packet.pts,
                                          stream->time_base,
                                          tb);
            sf.time_.base_ = AV_TIME_BASE;
            sf.tEnd_ = TTime(std::numeric_limits<int64>::max(), AV_TIME_BASE);

            if (subs)
            {
              sf.render_ = subs->render_;
              sf.traits_ = subs->format_;
              sf.index_ = subs->index_;
              sf.extraData_ = subs->extraData_;
            }
            else
            {
              sf.traits_ = kSubsCEA608;
              sf.index_ = packet.stream_index;
            }

            // copy the reference frame size:
            if (subsDec)
            {
              sf.rw_ = subsDec->width;
              sf.rh_ = subsDec->height;
            }

            if (subs && subs->format_ == kSubsDVD && !(sf.rw_ && sf.rh_))
            {
              sf.rw_ = subs->vobsub_.w_;
              sf.rh_ = subs->vobsub_.h_;
            }

            if (packet.data && packet.size)
            {
              TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                      &IPlanarBuffer::deallocator);
              buffer->resize(0, packet.size, 1);
              unsigned char * dst = buffer->data(0);
              memcpy(dst, packet.data, packet.size);

              sf.data_ = buffer;
            }

            if (packet.side_data &&
                packet.side_data->data &&
                packet.side_data->size)
            {
              TPlanarBufferPtr buffer(new TPlanarBuffer(1),
                                      &IPlanarBuffer::deallocator);
              buffer->resize(0, packet.side_data->size, 1, 1);
              unsigned char * dst = buffer->data(0);
              memcpy(dst, packet.side_data->data, packet.side_data->size);

              sf.sideData_ = buffer;
            }

            if (subsDec)
            {
              // decode the subtitle:
              int gotSub = 0;
              AVSubtitle sub;
              err = avcodec_decode_subtitle2(subsDec,
                                             &sub,
                                             &gotSub,
                                             &packet);

              if (err >= 0 && gotSub)
              {
                const uint8_t * hdr = subsDec->subtitle_header;
                const std::size_t sz = subsDec->subtitle_header_size;
                sf.private_ = TSubsPrivatePtr(new TSubsPrivate(sub, hdr, sz),
                                              &TSubsPrivate::deallocator);

                static const Rational tb_msec(1, 1000);

                if (packet.pts != AV_NOPTS_VALUE)
                {
                  sf.time_.time_ = av_rescale_q(packet.pts,
                                                stream->time_base,
                                                tb);

                  sf.time_.time_ += av_rescale_q(sub.start_display_time,
                                                 tb_msec,
                                                 tb);
                }

                if (packet.pts != AV_NOPTS_VALUE &&
                    sub.end_display_time > sub.start_display_time)
                {
                  double dt =
                    double(sub.end_display_time - sub.start_display_time) *
                    double(tb_msec.num) /
                    double(tb_msec.den);

                  // avoid subs that are visible for more than 5 seconds:
                  if (dt > 0.5 && dt < 5.0)
                  {
                    sf.tEnd_ = sf.time_;
                    sf.tEnd_ += dt;
                  }
                }
              }

              err = 0;
            }
            else if (closedCaptions)
            {
              // let the captions decoder handle it:
              videoTrack->cc_.decode(stream->time_base,
                                     packet,
                                     &outputTerminator_);
            }

            if (subs)
            {
              subs->push(sf, &outputTerminator_);
            }
          }
        }
      }
    }
    catch (const std::exception & e)
    {
#ifndef NDEBUG
      std::cerr
        << "\nMovie::threadLoop caught exception: " << e.what()
        << std::endl;
#endif
    }
    catch (...)
    {
#ifndef NDEBUG
      std::cerr
        << "\nMovie::threadLoop caught unexpected exception"
        << std::endl;
#endif
    }

#if 0 // ndef NDEBUG
    std::cerr
      << "\nMovie::threadLoop terminated"
      << std::endl;
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

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      t->threadStart();
      t->packetQueue().waitIndefinitelyForConsumerToBlock();
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
      t->threadStart();
      t->packetQueue().waitIndefinitelyForConsumerToBlock();
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

    outputTerminator_.stopWaiting(true);
    framestepTerminator_.stopWaiting(true);

    thread_.stop();
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

    if (selectedVideoTrack_ < videoTracks_.size())
    {
      VideoTrackPtr t = videoTracks_[selectedVideoTrack_];
      if (t->getDuration(start, duration))
      {
        return true;
      }
    }

    if (selectedAudioTrack_ < audioTracks_.size())
    {
      AudioTrackPtr t = audioTracks_[selectedAudioTrack_];
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
  Movie::requestSeekTime(double seekTime)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      mustSeek_ = true;
      seekTime_ = seekTime;

      VideoTrackPtr videoTrack;
      AudioTrackPtr audioTrack;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->packetQueue().clear();
        do { videoTrack->frameQueue_.clear(); }
        while (!videoTrack->packetQueue().waitForConsumerToBlock(1e-2));
        videoTrack->frameQueue_.clear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::string ts = TTime(seekTime).to_hhmmss_usec(":");
        std::cerr << "\n\tCLEAR VIDEO FRAME QUEUE for seek: "
                  << ts << std::endl;
#endif
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->packetQueue().clear();
        do { audioTrack->frameQueue_.clear(); }
        while (!audioTrack->packetQueue().waitForConsumerToBlock(1e-2));
        audioTrack->frameQueue_.clear();

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
        std::string ts = TTime(seekTime).to_hhmmss_usec(":");
        std::cerr << "\n\tCLEAR AUDIO FRAME QUEUE for seek: "
                  << ts << std::endl;
#endif
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
  Movie::seekTo(double seekTime, bool dropPendingFrames)
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

    int streamIndex = -1;

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audioTracks_.size())
    {
      audioTrack = audioTracks_[selectedAudioTrack_];
    }

    VideoTrackPtr videoTrack;
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      videoTrack = videoTracks_[selectedVideoTrack_];
    }

    if ((context_->iformat->flags & AVFMT_TS_DISCONT) &&
        strcmp(context_->iformat->name, "ogg") != 0 &&
        audioTrack)
    {
      streamIndex = audioTrack->streamIndex();
    }

    int64_t ts = int64_t(seekTime * double(AV_TIME_BASE));
    int seekFlags = 0;

    if (streamIndex != -1)
    {
      AVRational tb;
      tb.num = 1;
      tb.den = AV_TIME_BASE;

      const AVStream * s = context_->streams[streamIndex];
      ts = av_rescale_q(ts, tb, s->time_base);
    }

    int err = avformat_seek_file(context_,
                                 streamIndex,
                                 kMinInt64,
                                 ts,
                                 ts, // kMaxInt64,
                                 seekFlags);

    if (err < 0)
    {
      if (!ts)
      {
        // must be trying to rewind a stream of undefined duration:
        YAE_ASSERT(!hasDuration());
        seekFlags |= AVSEEK_FLAG_BYTE;
      }

      err = avformat_seek_file(context_,
                               streamIndex,
                               kMinInt64,
                               ts,
                               ts, // kMaxInt64,
                               seekFlags | AVSEEK_FLAG_ANY);
    }

    if (err < 0)
    {
#ifndef NDEBUG
      std::cerr
        << "avformat_seek_file (" << seekTime << ") returned " << err
        << std::endl;
#endif
      return err;
    }

    if (videoTrack)
    {
      err = videoTrack->resetTimeCounters(seekTime, dropPendingFrames);
    }

    if (!err && audioTrack)
    {
      err = audioTrack->resetTimeCounters(seekTime, dropPendingFrames);
    }

    clock_.cancelWaitForOthers();
    clock_.resetCurrentTime();

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
      audioTrack->packetQueue().
        waitIndefinitelyForConsumerToBlock();

      audioTrack->frameQueue_.
        waitIndefinitelyForConsumerToBlock(&framestepTerminator_);
    }

    if (videoTrack)
    {
      videoTrack->packetQueue().
        waitIndefinitelyForConsumerToBlock();

      videoTrack->frameQueue_.
        waitIndefinitelyForConsumerToBlock(&framestepTerminator_);
    }

    boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
    requestMutex(lock);

    if (mustStop_)
    {
      // user must have switched to another item in the playlist:
      return AVERROR(EAGAIN);
    }

    double seekTime = seekToTimeIn ? timeIn_ : 0.0;
    bool dropPendingFrames = false;
    return seekTo(seekTime, dropPendingFrames);
  }

  //----------------------------------------------------------------
  // Movie::getPlaybackInterval
  //
  void
  Movie::getPlaybackInterval(double & timeIn, double & timeOut) const
  {
    timeIn = timeIn_;
    timeOut = timeOut_;
  }

  //----------------------------------------------------------------
  // Movie::setPlaybackIntervalStart
  //
  void
  Movie::setPlaybackIntervalStart(double timeIn)
  {
    try
    {
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      timeIn_ = timeIn;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

      timeOut_ = timeOut;

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
        videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
      }

      if (selectedAudioTrack_ < audioTracks_.size())
      {
        AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
        audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
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
        if (selectedVideoTrack_ < videoTracks_.size())
        {
          VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
          videoTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
        }

        if (selectedAudioTrack_ < audioTracks_.size())
        {
          AudioTrackPtr audioTrack = audioTracks_[selectedAudioTrack_];
          audioTrack->setPlaybackInterval(timeIn_, timeOut_, playbackEnabled_);
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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

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
      boost::unique_lock<boost::timed_mutex> lock(mutex_, boost::defer_lock);
      requestMutex(lock);

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

      if (selectedVideoTrack_ < videoTracks_.size())
      {
        VideoTrackPtr videoTrack = videoTracks_[selectedVideoTrack_];
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
    return subs_.size();
  }

  //----------------------------------------------------------------
  // Movie::subsInfo
  //
  TSubsFormat
  Movie::subsInfo(std::size_t i, TTrackInfo & info) const
  {
    info.program_ = context_ ? context_->nb_programs : 0;
    info.ntracks_ = subs_.size();
    info.index_ = i;
    info.lang_.clear();
    info.name_.clear();

    if (info.index_ < info.ntracks_)
    {
      const SubtitlesTrack & subs = *(subs_[i]);
      info.lang_ = subs.lang_;
      info.name_ = subs.name_;
      info.program_ = get(streamIndexToProgramIndex_, subs.stream_->index);
      return subs.format_;
    }

    return kSubsNone;
  }

  //----------------------------------------------------------------
  // Movie::setSubsRender
  //
  void
  Movie::setSubsRender(std::size_t i, bool render)
  {
    std::size_t nsubs = subs_.size();
    if (i < nsubs)
    {
      SubtitlesTrack & subs = *(subs_[i]);
      subs.render_ = render;
    }
  }

  //----------------------------------------------------------------
  // Movie::getSubsRender
  //
  bool
  Movie::getSubsRender(std::size_t i) const
  {
    std::size_t nsubs = subs_.size();
    if (i < nsubs)
    {
      SubtitlesTrack & subs = *(subs_[i]);
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
  // blockedOn
  //
  static bool
  blockedOn(const Track * a, const Track * b)
  {
    if (!a || !b)
    {
      return false;
    }

    bool blocked = (a->packetQueue().producerIsBlocked() &&
                    b->packetQueue().consumerIsBlocked());
    return blocked;
  }

  //----------------------------------------------------------------
  // Movie::blockedOnVideo
  //
  bool
  Movie::blockedOnVideo() const
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

    bool blocked = blockedOn(videoTrack.get(), audioTrack.get());

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      std::cerr << "BLOCKED ON VIDEO" << std::endl;
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
    if (selectedVideoTrack_ < videoTracks_.size())
    {
      videoTrack = videoTracks_[selectedVideoTrack_];
    }

    AudioTrackPtr audioTrack;
    if (selectedAudioTrack_ < audioTracks_.size())
    {
      audioTrack = audioTracks_[selectedAudioTrack_];
    }

    bool blocked = blockedOn(audioTrack.get(), videoTrack.get());

#if YAE_DEBUG_SEEKING_AND_FRAMESTEP
    if (blocked)
    {
      std::cerr << "BLOCKED ON AUDIO" << std::endl;
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

}
