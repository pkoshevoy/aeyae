// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 24 22:34:54 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// local:
#include "yae_player_item.h"
#include "yaeItem.h"
#ifdef __APPLE__
#include "yaeAudioUnitRenderer.h"
#else
#include "yaePortaudioRenderer.h"
#endif


namespace yae
{

  //----------------------------------------------------------------
  // PlayerItem::PlayerItem
  //
  PlayerItem::PlayerItem(const char * id,
                         const yae::shared_ptr<IOpenGLContext> & ctx):
    QObject(),
    Item(id),
    canvas_(ctx),
    paused_(false),
    reader_id_(0)
  {
    bool ok = true;

    canvas_.initializePrivateBackend();

#ifdef __APPLE__
    audio_.reset(AudioUnitRenderer::create());
#else
    audio_.reset(PortaudioRenderer::create());
#endif
    video_.reset(VideoRenderer::create());

    downmix_to_stereo_ = BoolRef::constant(true);
    loop_playback_ = BoolRef::constant(true);
    skip_loopfilter_ = BoolRef::constant(true);
    skip_nonref_frames_ = BoolRef::constant(true);
    skip_color_converter_ = BoolRef::constant(false);
    deinterlace_ = BoolRef::constant(true);
    playback_tempo_ = DataRef<double>::constant(1.0);

    ok = connect(&timeline_, SIGNAL(userIsSeeking(bool)),
                 this, SLOT(user_is_seeking(bool)));
    YAE_ASSERT(ok);

    ok = connect(&timeline_, SIGNAL(moveTimeIn(double)),
                 this, SLOT(move_time_in(double)));
    YAE_ASSERT(ok);

    ok = connect(&timeline_, SIGNAL(moveTimeOut(double)),
                 this, SLOT(move_time_out(double)));
    YAE_ASSERT(ok);

    ok = connect(&timeline_, SIGNAL(movePlayHead(double)),
                 this, SLOT(move_playhead(double)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerItem::uncache
  //
  void
  PlayerItem::uncache()
  {
    Item::uncache();

    downmix_to_stereo_.uncache();
    loop_playback_.uncache();
    skip_loopfilter_.uncache();
    skip_nonref_frames_.uncache();
    skip_color_converter_.uncache();
    deinterlace_.uncache();
    playback_tempo_.uncache();
  }

  //----------------------------------------------------------------
  // PlayerItem::paintContent
  //
  void
  PlayerItem::paintContent() const
  {
    TGLSaveState restore_state(GL_ENABLE_BIT);
    TGLSaveClientState restore_client_state(GL_CLIENT_ALL_ATTRIB_BITS);

    double x = this->left();
    double y = this->top();
    double w = this->width();
    double h = this->height();
    canvas_.resize(1.0, w, h);

    if (reader_)
    {
      VideoTraits vtts;

      if (reader_->getVideoTraits(vtts))
      {
        const pixelFormat::Traits * ptts =
          pixelFormat::getTraits(vtts.pixelFormat_);

        if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                     pixelFormat::kPaletted)))
        {
          canvas_.paint_checkerboard(x, y, w, h);
        }
      }
    }

    canvas_.paint_canvas(x, y, w, h);
  }

  //----------------------------------------------------------------
  // find_matching_track
  //
  template <typename TTraits>
  static std::size_t
  find_matching_track(const std::vector<TTrackInfo> & trackInfo,
                      const std::vector<TTraits> & trackTraits,
                      const TTrackInfo & selInfo,
                      const TTraits & selTraits)
  {
    std::size_t n = trackInfo.size();

    if (!selInfo.isValid())
    {
      // video was disabled, keep it disabled:
      return n;
    }

    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trackInfo.front();
      return info.index_;
    }

    // try to find a matching track:
    std::vector<TTrackInfo> trkInfo = trackInfo;
    std::vector<TTraits> trkTraits = trackTraits;

    std::vector<TTrackInfo> tmpInfo;
    std::vector<TTraits> tmpTraits;

    // try to match the language code:
    if (selInfo.hasLang())
    {
      const char * selLang = selInfo.lang();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        const TTraits & traits = trkTraits[i];
        if (info.hasLang() && strcmp(info.lang(), selLang) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track name:
    if (selInfo.hasName())
    {
      const char * selName = selInfo.name();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        const TTraits & traits = trkTraits[i];
        if (info.hasName() && strcmp(info.name(), selName) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track traits:
    for (std::size_t i = 0; i < n; i++)
    {
      const TTrackInfo & info = trkInfo[i];
      const TTraits & traits = trkTraits[i];
      if (selTraits == traits)
      {
        tmpInfo.push_back(info);
        tmpTraits.push_back(traits);
      }
    }

    if (!tmpInfo.empty())
    {
      trkInfo = tmpInfo;
      trkTraits = tmpTraits;
      tmpInfo.clear();
      tmpTraits.clear();
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track index:
    if (trackInfo.size() == selInfo.ntracks_)
    {
      return selInfo.index_;
    }

    // default to first track:
    return 0;
  }

  //----------------------------------------------------------------
  // PlayerItem::playback
  //
  void
  PlayerItem::playback(const IReaderPtr & reader,
                       std::size_t vtrack,
                       std::size_t atrack,
                       std::size_t strack,
                       unsigned int cc,
                       const TTime & seek_time)
  {
    if (!reader)
    {
      return;
    }

    ++reader_id_;
    reader->setReaderId(reader_id_);

    // prevent Canvas from rendering any pending frames from previous reader:
    canvas_.acceptFramesWithReaderId(reader_id_);

    // disconnect timeline from renderers:
    timeline_.observe(SharedClock());

    // shut down the audio renderer before calling
    // adjust_audio_traits_override, hopefully
    // that will avoid triggering the portaudio deadlock:
    if (reader_)
    {
      reader_->close();
    }

    video_->close();
    audio_->close();

    std::size_t num_video_tracks = reader->getNumberOfVideoTracks();
    std::size_t num_audio_tracks = reader->getNumberOfAudioTracks();
    std::size_t num_subtt_tracks = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackEnabled(!paused_);

    select_video_track(reader.get(), vtrack);
    select_audio_track(reader.get(), atrack);
    select_subtt_track(reader.get(),
                       (strack < num_subtt_tracks) ? strack :
                       num_subtt_tracks + cc);

    // reset overlay plane to clean state, reset libass wrapper:
    canvas_.clearOverlay();

    // reset timeline start, duration, playhead, in/out points:
    timeline_.resetFor(reader.get());

    if (seek_time.valid())
    {
      // skip to specified position:
      reader->seek(seek_time.sec());
    }

    // process attachments:
    std::size_t num_attachments = reader->getNumberOfAttachments();
    for (std::size_t i = 0; i < num_attachments; i++)
    {
      const TAttachment * att = reader->getAttachmentInfo(i);

      typedef std::map<std::string, std::string>::const_iterator TIter;
      TIter mimetypeFound = att->metadata_.find(std::string("mimetype"));
      TIter filenameFound = att->metadata_.find(std::string("filename"));

      const char * filename =
        filenameFound != att->metadata_.end() ?
        filenameFound->second.c_str() :
        NULL;

      if (mimetypeFound != att->metadata_.end())
      {
        static const std::string fontTypes[] = {
          std::string("application/x-truetype-font"),
          std::string("application/vnd.ms-opentype"),
          std::string("application/x-font-ttf"),
          std::string("application/x-font")
        };

        static const std::size_t numFontTypes =
          sizeof(fontTypes) / sizeof(fontTypes[0]);

        std::string mimetype(mimetypeFound->second);
        boost::algorithm::to_lower(mimetype);

        for (std::size_t j = 0; j < numFontTypes; j++)
        {
          if (mimetype == fontTypes[j])
          {
            canvas_.libassAddFont(filename, att->data_, att->size_);
            break;
          }
        }
      }
    }

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepare_to_render(reader.get(), paused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // replace the previous reader:
    reader_ = reader;

    // allow renderers to read from output frame queues:
    resume_renderers(true);
  }

  //----------------------------------------------------------------
  // PlayerItem::user_is_seeking
  //
  void
  PlayerItem::user_is_seeking(bool seeking)
  {
    reader_->setPlaybackEnabled(!seeking && !paused_);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_time_in
  //
  void
  PlayerItem::move_time_in(double seconds)
  {
    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalStart(seconds);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_time_out
  //
  void
  PlayerItem::move_time_out(double seconds)
  {
    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalEnd(seconds);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_playhead
  //
  void
  PlayerItem::move_playhead(double seconds)
  {
    video_->pause();
    audio_->pause();

    reader_->seek(seconds);

    // this avoids weird non-rendering of subtitles:
    canvas_.libassFlushTrack();

    resume_renderers(true);
  }

  //----------------------------------------------------------------
  // PlayerItem::toggle_playback
  //
  void
  PlayerItem::toggle_playback()
  {
    paused_ = !paused_;
    reader_->setPlaybackEnabled(!paused_);

    if (paused_)
    {
      TIgnoreClockStop ignore_clock_stop(timeline_);
      stop_renderers();
      prepare_to_render(reader_.get(), paused_);
    }
    else
    {
      prepare_to_render(reader_.get(), paused_);
      resume_renderers();
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::playback_stop
  //
  void
  PlayerItem::playback_stop()
  {
    timeline_.observe(SharedClock());
    timeline_.resetFor(NULL);

    if (reader_)
    {
      reader_->close();
    }

    video_->close();
    audio_->close();

    ++reader_id_;
    canvas_.acceptFramesWithReaderId(reader_id_);
    reader_.reset();
  }

  //----------------------------------------------------------------
  // PlayerItem::stop_renderers
  //
  void
  PlayerItem::stop_renderers()
  {
    video_->stop();
    audio_->stop();

    video_->pause();
    audio_->pause();
  }

  //----------------------------------------------------------------
  // PlayerItem::prepare_to_render
  //
  void
  PlayerItem::prepare_to_render(IReader * reader, bool frame_stepping)
  {
    video_->pause();
    audio_->pause();

    std::size_t video_track = reader->getSelectedVideoTrackIndex();
    std::size_t audio_track = reader->getSelectedAudioTrackIndex();

    std::size_t num_video_tracks = reader->getNumberOfVideoTracks();
    std::size_t num_audio_tracks = reader->getNumberOfAudioTracks();

    SharedClock shared_clock;
    timeline_.observe(shared_clock);
    reader->setSharedClock(shared_clock);

    if ((audio_track < num_audio_tracks) &&
        (video_track >= num_video_tracks || !frame_stepping))
    {
      // sync to audio clock:
      audio_->takeThisClock(shared_clock);
      audio_->obeyThisClock(audio_->clock());

      if (video_track < num_video_tracks)
      {
        video_->obeyThisClock(audio_->clock());
      }
    }
    else if (video_track < num_video_tracks)
    {
      // sync to video clock:
      video_->takeThisClock(shared_clock);
      video_->obeyThisClock(video_->clock());

      if (audio_track < num_audio_tracks)
      {
        audio_->obeyThisClock(video_->clock());
      }
    }
    else
    {
      // all tracks disabled!
      return;
    }

    // update the renderers:
    if (!frame_stepping)
    {
      adjust_audio_traits_override(reader);

      if (!audio_->open(reader))
      {
        video_->takeThisClock(shared_clock);
        video_->obeyThisClock(video_->clock());
      }
    }

    video_->open(&canvas_, reader);

    if (!frame_stepping)
    {
      timeline_.adjustTo(reader);

      // request playback at currently selected playback rate:
      double tempo = playback_tempo_.get();
      reader->setTempo(tempo);
    }

    bool enable_looping = loop_playback_.get();
    reader->setPlaybackLooping(enable_looping);

    bool skip_loopfilter = skip_loopfilter_.get();
    reader->skipLoopFilter(skip_loopfilter);

    bool skip_nonref_frames = skip_nonref_frames_.get();
    reader->skipNonReferenceFrames(skip_nonref_frames);

    bool deinterlace = deinterlace_.get();
    reader->setDeinterlacing(deinterlace);
  }

  //----------------------------------------------------------------
  // PlayerItem::select_audio_track
  //
  void
  PlayerItem::select_audio_track(IReader * reader, std::size_t audio_track)
  {
    reader->selectAudioTrack(audio_track);
    adjust_audio_traits_override(reader);
  }

  //----------------------------------------------------------------
  // PlayerItem::select_video_track
  //
  void
  PlayerItem::select_video_track(IReader * reader, std::size_t video_track)
  {
    std::size_t num_video_tracks = reader->getNumberOfVideoTracks();
    reader->selectVideoTrack(video_track);

    VideoTraits vtts;
    if (reader->getVideoTraits(vtts))
    {
      bool skip_color_converter = skip_color_converter_.get();
      canvas_.skipColorConverter(skip_color_converter);

      TPixelFormatId format = kInvalidPixelFormat;
      if (canvas_.
          canvasRenderer()->
          adjustPixelFormatForOpenGL(skip_color_converter, vtts, format))
      {
        vtts.pixelFormat_ = format;

        // NOTE: overriding frame size implies scaling, so don't do it
        // unless you really want to scale the images in the reader;
        // In general, leave scaling to OpenGL:
        vtts.encodedWidth_ = 0;
        vtts.encodedHeight_ = 0;
        vtts.pixelAspectRatio_ = 0.0;

        reader->setVideoTraitsOverride(vtts);
      }
    }

    if (reader->getVideoTraitsOverride(vtts))
    {
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

      if (!ptts)
      {
        // unsupported pixel format:
        reader->selectVideoTrack(num_video_tracks);
      }
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::select_subtt_track
  //
  void
  PlayerItem::select_subtt_track(IReader * reader, std::size_t subtt_track)
  {
    const std::size_t nsubs = reader->subsCount();
    const std::size_t cc = nsubs < subtt_track ? subtt_track - nsubs : 0;
    reader->setRenderCaptions(cc);

    for (std::size_t i = 0; i < nsubs; i++)
    {
      bool enable = (i == subtt_track);
      reader->setSubsRender(i, enable);
    }

    canvas_.setSubs(std::list<TSubsFrame>());
  }

  //----------------------------------------------------------------
  // PlayerItem::adjust_audio_traits_override
  //
  void
  PlayerItem::adjust_audio_traits_override(IReader * reader)
  {
    AudioTraits native;

    if (reader->getAudioTraits(native))
    {
      if (getNumberOfChannels(native.channelLayout_) > 2 &&
          downmix_to_stereo_.get())
      {
        native.channelLayout_ = kAudioStereo;
      }

      AudioTraits supported;
      audio_->match(native, supported);

      reader->setAudioTraitsOverride(supported);
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::resume_renderers
  //
  void
  PlayerItem::resume_renderers(bool load_next_frame_if_paused)
  {
    if (!paused_)
    {
      // allow renderers to read from output frame queues:
      audio_->resume();
      video_->resume();
    }
    else if (load_next_frame_if_paused)
    {
      // render the next video frame:
      skip_to_next_frame();
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_to_next_frame
  //
  void
  PlayerItem::skip_to_next_frame()
  {
    if (!paused_)
    {
      return;
    }

    std::size_t num_video_tracks = reader_->getNumberOfVideoTracks();
    std::size_t video_track_index = reader_->getSelectedVideoTrackIndex();

    if (video_track_index >= num_video_tracks)
    {
      return;
    }

    std::size_t num_audio_tracks = reader_->getNumberOfAudioTracks();
    std::size_t audio_track_index = reader_->getSelectedAudioTrackIndex();
    bool hasAudio = audio_track_index < num_audio_tracks;

    TIgnoreClockStop ignore_clock_stop(timeline_);
    IReaderPtr reader = reader_;

    QTime startTime = QTime::currentTime();
    bool done = false;
    while (!done && reader && reader == reader_)
    {
      if (hasAudio && reader_->blockedOnAudio())
      {
        // VFR source (a slide show) may require the audio output
        // queues to be pulled in order to allow the demuxer
        // to push new packets into audio/video queues:

        TTime dt(1001, 60000);
        audio_->skipForward(dt, reader_.get());
      }

      TTime t;
      done = video_->skipToNextFrame(t);

      if (!done)
      {
        if (startTime.elapsed() > 2000)
        {
          // avoid blocking the UI indefinitely:
          break;
        }

        continue;
      }

      if (hasAudio)
      {
        // attempt to nudge the audio reader to the same position:
        audio_->skipToTime(t, reader_.get());
      }
    }
  }
}
