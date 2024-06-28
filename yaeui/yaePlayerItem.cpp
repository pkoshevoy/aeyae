// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 24 22:34:54 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/video/yae_video.h"

// yaeui:
#ifdef __APPLE__
#include "yaeAudioUnitRenderer.h"
#else
#include "yaePortaudioRenderer.h"
#endif
#include "yaePlayerItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerItem::PlayerItem
  //
  PlayerItem::PlayerItem(const char * id):
    QObject(),
    Item(id),
    reader_id_(0),
    paused_(false),
    sel_video_initialized_(false),
    sel_audio_initialized_(false),
    sel_subtt_initialized_(false),
    sel_subtt_cc_(0)
  {
    bool ok = true;

#ifdef __APPLE__
    audio_.reset(AudioUnitRenderer::create());
#else
    audio_.reset(PortaudioRenderer::create());
#endif
    video_.reset(VideoRenderer::create());

    downmix_to_stereo_ = BoolRef::constant(true);
    loop_playback_ = BoolRef::constant(true);
    skip_loopfilter_ = BoolRef::constant(false);
    skip_nonref_frames_ = BoolRef::constant(false);
    skip_color_converter_ = BoolRef::constant(false);
    deinterlace_ = BoolRef::constant(false);
    playback_tempo_ = DataRef<double>::constant(1.0);
#if 0
    ok = connect(this, SIGNAL(set_in_point()),
                 &timeline_, SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(set_out_point()),
                 &timeline_, SLOT(setOutPoint()));
    YAE_ASSERT(ok);
#endif
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
#if 0
    ok = connect(&timeline_, SIGNAL(clockStopped(const SharedClock &)),
                 this, SLOT(playback_finished(const SharedClock &)));
    YAE_ASSERT(ok);
#endif
  }

  //----------------------------------------------------------------
  // PlayerItem::setCanvasDelegate
  //
  void
  PlayerItem::setCanvasDelegate(const yae::shared_ptr<Canvas::IDelegate> & d)
  {
    canvas_delegate_ = d;

    if (personal_canvas_)
    {
      Canvas & canvas = *personal_canvas_;
      canvas.setDelegate(d);
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::makePersonalCanvas
  //
  void
  PlayerItem::makePersonalCanvas(const yae::shared_ptr<IOpenGLContext> & ctx)
  {
    personal_canvas_.reset(new Canvas(ctx));
    Canvas & canvas = *personal_canvas_;
    canvas.setDelegate(canvas_delegate_);
    canvas.initializePrivateBackend();
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
    if (!personal_canvas_)
    {
      return;
    }

    Canvas & canvas = *personal_canvas_;
    TGLSaveState restore_state(GL_ENABLE_BIT);
    TGLSaveClientState restore_client_state(GL_CLIENT_ALL_ATTRIB_BITS);

    double x = this->left();
    double y = this->top();
    double w = this->width();
    double h = this->height();
    double device_pixel_ratio =
      canvas_delegate_ ? canvas_delegate_->device_pixel_ratio() : 1.0;
    canvas.resize(device_pixel_ratio,
                  w / device_pixel_ratio,
                  h / device_pixel_ratio);

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
          canvas.paint_checkerboard(x, y, w, h);
        }
      }
    }

    canvas.paint_canvas(x, y, w, h);
  }

  //----------------------------------------------------------------
  // PlayerItem::playback
  //
  void
  PlayerItem::playback(const IReaderPtr & reader,
                       const std::vector<TTrackInfo> & audioInfo,
                       const std::vector<AudioTraits> & audioTraits,
                       const std::vector<TTrackInfo> & videoInfo,
                       const std::vector<VideoTraits> & videoTraits,
                       const std::vector<TTrackInfo> & subsInfo,
                       const std::vector<TSubsFormat> & subsFormat,
                       const IBookmark * bookmark,
                       const TTime & seekTime)
  {
    if (!reader)
    {
      return;
    }

    audio_info_ = audioInfo;
    audio_traits_ = audioTraits;

    video_info_ = videoInfo;
    video_traits_ = videoTraits;

    subtt_info_ = subsInfo;
    subtt_format_ =  subsFormat;

    // keep track of current closed caption selection:
    unsigned int cc = sel_subtt_cc_;

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t numSubttTracks = reader->subsCount();

    bool rememberSelectedVideoTrack = !reader_;

    std::size_t program = find_matching_program(videoInfo, sel_video_);
    std::size_t vtrack = !sel_video_initialized_ ? 0 :
      find_matching_track<VideoTraits>(videoInfo,
                                       videoTraits,
                                       sel_video_,
                                       sel_video_traits_,
                                       program);

    if (bookmark && bookmark->vtrack_ <= numVideoTracks)
    {
      vtrack = bookmark->vtrack_;
      rememberSelectedVideoTrack = numVideoTracks > 0;
    }

    if (vtrack < numVideoTracks)
    {
      program = videoInfo[vtrack].program_;
    }

    bool rememberSelectedAudioTrack = !reader_;
    std::size_t atrack = !sel_audio_initialized_ ? 0 :
      find_matching_track<AudioTraits>(audioInfo,
                                       audioTraits,
                                       sel_audio_,
                                       sel_audio_traits_,
                                       program);

    if (bookmark && bookmark->atrack_ <= numAudioTracks)
    {
      atrack = bookmark->atrack_;
      rememberSelectedAudioTrack = numAudioTracks > 0;
    }

    if (atrack < numAudioTracks)
    {
      YAE_ASSERT(program == audioInfo[atrack].program_);
      program = audioInfo[atrack].program_;
    }

    if (vtrack >= numVideoTracks &&
        atrack >= numAudioTracks)
    {
      // avoid disabling both audio and video due to
      // previous custom or bookmarked track selections:

      if (numVideoTracks)
      {
        vtrack = 0;
      }
      else if (numAudioTracks)
      {
        atrack = 0;
      }
    }

    bool rememberSelectedSubtitlesTrack = !reader_;
    std::size_t strack = !sel_subtt_initialized_ ? numSubttTracks :
      find_matching_track<TSubsFormat>(subsInfo,
                                       subsFormat,
                                       sel_subtt_,
                                       sel_subtt_format_,
                                       program);
    if (bookmark)
    {
      if (!bookmark->subs_.empty() && bookmark->subs_.front() < numSubttTracks)
      {
        strack = bookmark->subs_.front();
        rememberSelectedSubtitlesTrack = true;
      }
      else if (bookmark->subs_.empty())
      {
        strack = numSubttTracks;
        rememberSelectedSubtitlesTrack = true;
      }

      if (bookmark->cc_)
      {
        cc = bookmark->cc_;
      }
    }

    TTime startHere = seekTime;
    if (startHere.invalid() && bookmark)
    {
      startHere = TTime(bookmark->positionInSeconds_);
    }

    playback(reader, vtrack, atrack, strack, cc, startHere);

    if (rememberSelectedVideoTrack)
    {
      reader->getSelectedVideoTrackInfo(sel_video_);
      reader->getVideoTraits(sel_video_traits_);
      sel_video_initialized_ = true;
    }

    if (rememberSelectedAudioTrack)
    {
      reader->getSelectedAudioTrackInfo(sel_audio_);
      reader->getAudioTraits(sel_audio_traits_);
      sel_audio_initialized_ = true;
    }

    if (rememberSelectedSubtitlesTrack)
    {
      sel_subtt_format_ = reader->subsInfo(strack, sel_subtt_);
      sel_subtt_initialized_ = true;
    }
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
    reader->threadStop();

    Canvas * canvas = get_canvas();
    if (canvas)
    {
      // prevent Canvas from rendering any pending frames from previous reader:
      canvas->acceptFramesWithReaderId(reader_id_);
    }

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

    std::size_t num_subtt_tracks = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackEnabled(!paused_);

    select_video_track(reader.get(), vtrack);
    select_audio_track(reader.get(), atrack);
    select_subtt_track(reader.get(),
                       (strack < num_subtt_tracks) ? strack :
                       num_subtt_tracks + cc);

    if (canvas)
    {
      // reset overlay plane to clean state, reset libass wrapper:
      canvas->clearOverlay();
    }

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
            if (canvas)
            {
              canvas->libassAddFont(filename, att->data_, att->size_);
            }

            break;
          }
        }
      }
    }

    // replace the previous reader before we start the new reader,
    // in case it's a DemuxerReader that shares some Tracks
    // with previous DemuxerReader instance:
    reader_ = reader;

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepare_to_render(reader.get(), paused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // allow renderers to read from output frame queues:
    resume_renderers(true);
  }

  //----------------------------------------------------------------
  // PlayerItem::user_is_seeking
  //
  void
  PlayerItem::user_is_seeking(bool seeking)
  {
    if (!reader_)
    {
      return;
    }

    reader_->setPlaybackEnabled(!seeking && !paused_);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_time_in
  //
  void
  PlayerItem::move_time_in(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalStart(seconds);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_time_out
  //
  void
  PlayerItem::move_time_out(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalEnd(seconds);
  }

  //----------------------------------------------------------------
  // PlayerItem::move_playhead
  //
  void
  PlayerItem::move_playhead(double seconds)
  {
    if (!reader_)
    {
      return;
    }

    video_->pause();
    audio_->pause();

    reader_->seek(seconds);

    // this avoids weird non-rendering of subtitles:
    Canvas * canvas = get_canvas();
    if (canvas)
    {
      canvas->libassFlushTrack();
    }

    resume_renderers(true);
  }

  //----------------------------------------------------------------
  // PlayerItem::toggle_playback
  //
  void
  PlayerItem::toggle_playback()
  {
    paused_ = !paused_;

    if (reader_)
    {
      reader_->setPlaybackEnabled(!paused_);
    }

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

    Canvas * canvas = get_canvas();
    if (canvas)
    {
      canvas->acceptFramesWithReaderId(reader_id_);
    }
  }

  //----------------------------------------------------------------
  // PlayerItem::set_downmix_to_stereo
  //
  void
  PlayerItem::set_downmix_to_stereo(bool downmix)
  {
    if (downmix_to_stereo_.get() == downmix)
    {
      return;
    }

    downmix_to_stereo_ = BoolRef::constant(downmix);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    // reset reader:
    TIgnoreClockStop ignore_clock_stop(timeline_);
    reader->threadStop();

    stop_renderers();
    prepare_to_render(reader, paused_);

    double t = timeline_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resume_renderers();
  }

  //----------------------------------------------------------------
  // PlayerItem::set_loop_playback
  //
  void
  PlayerItem::set_loop_playback(bool loop_playback)
  {
    if (loop_playback_.get() == loop_playback)
    {
      return;
    }

    loop_playback_ = BoolRef::constant(loop_playback);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    reader->setPlaybackLooping(loop_playback);
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_color_converter
  //
  void
  PlayerItem::skip_color_converter(bool skip)
  {
    if (skip_color_converter_.get() == skip)
    {
      return;
    }

    skip_color_converter_ = BoolRef::constant(skip);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    TIgnoreClockStop ignore_clock_stop(timeline_);
    reader->threadStop();

    stop_renderers();

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    select_video_track(reader, videoTrack);
    prepare_to_render(reader, paused_);

    double t = timeline_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resume_renderers(true);
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_loopfilter
  //
  void
  PlayerItem::skip_loopfilter(bool skip)
  {
    if (skip_loopfilter_.get() == skip)
    {
      return;
    }

    skip_loopfilter_ = BoolRef::constant(skip);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    reader->skipLoopFilter(skip);
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_nonref_frames
  //
  void
  PlayerItem::skip_nonref_frames(bool skip)
  {
    if (skip_nonref_frames_.get() == skip)
    {
      return;
    }

    skip_nonref_frames_ = BoolRef::constant(skip);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    reader->skipNonReferenceFrames(skip);
  }

  //----------------------------------------------------------------
  // PlayerItem::set_deinterlace
  //
  void
  PlayerItem::set_deinterlace(bool deint)
  {
    if (deinterlace_.get() == deint)
    {
      return;
    }

    deinterlace_ = BoolRef::constant(deint);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    reader->setDeinterlacing(deint);
  }

  //----------------------------------------------------------------
  // PlayerItem::set_playback_tempo
  //
  void
  PlayerItem::set_playback_tempo(double tempo)
  {
    if (playback_tempo_.get() == tempo)
    {
      return;
    }

    playback_tempo_ = DataRef<double>::constant(tempo);

    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    reader->setTempo(tempo);
  }

  //----------------------------------------------------------------
  // PlayerItem::audio_select_track
  //
  bool
  PlayerItem::audio_select_track(std::size_t index, Tracks & curr_tracks)
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      sel_audio_.clear();
      sel_audio_initialized_ = true;
      return false;
    }

    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program = get_curr_program(reader, vinfo, ainfo, sinfo);

    TIgnoreClockStop ignore_clock_stop(timeline_);
    reader->threadStop();
    stop_renderers();

    select_audio_track(reader, index);

    reader->getSelectedAudioTrackInfo(sel_audio_);
    reader->getAudioTraits(sel_audio_traits_);
    sel_audio_initialized_ = true;

    // if the audio program is not the same as the video program
    // then change the video track to a matching audio program:
    if (sel_audio_.isValid() && sel_audio_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(sel_audio_.program_, program));

      if (vinfo.isValid())
      {
        // select another video track:
        std::size_t i = program.video_.empty() ?
          reader->getNumberOfVideoTracks() : program.video_.front();
        select_video_track(reader, i);

        reader->getSelectedVideoTrackInfo(sel_video_);
        reader->getVideoTraits(sel_video_traits_);
        sel_video_initialized_ = true;
      }

      if (sinfo.isValid())
      {
        // select another subtitle track:
        std::size_t i = program.subs_.empty() ?
          reader->subsCount() : program.subs_.front();
        select_subtt_track(reader, i);
        reader->subsInfo(i, sel_subtt_);
        sel_subtt_initialized_ = true;
      }
    }

    prepare_to_render(reader, paused_);

    if (sel_audio_.isValid() && sel_audio_.program_ == prev_program)
    {
      double t = timeline_.currentTime();
      reader->seek(t);
    }
    else
    {
      timeline_.resetFor(reader);
    }

    curr_tracks.audio_ = reader->getSelectedAudioTrackIndex();
    curr_tracks.video_ = reader->getSelectedVideoTrackIndex();
    curr_tracks.subtt_ = get_selected_subtt_track(*reader);

    reader->threadStart();

    resume_renderers();
    return sel_audio_.isValid();
  }

  //----------------------------------------------------------------
  // PlayerItem::video_select_track
  //
  bool
  PlayerItem::video_select_track(std::size_t index, Tracks & curr_tracks)
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      sel_video_.clear();
      sel_video_initialized_ = true;
      return false;
    }

    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program = get_curr_program(reader, vinfo, ainfo, sinfo);

    TIgnoreClockStop ignore_clock_stop(timeline_);
    reader->threadStop();
    stop_renderers();

    select_video_track(reader, index);

    reader->getSelectedVideoTrackInfo(sel_video_);
    reader->getVideoTraits(sel_video_traits_);
    sel_video_initialized_ = true;

    // if the video program is not the same as the audio program
    // then change the audio track to a matching video program:
    if (sel_video_.isValid() && sel_video_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(sel_video_.program_, program));

      if (ainfo.isValid())
      {
        // select another audio track:
        std::size_t i = program.audio_.empty() ?
          reader->getNumberOfAudioTracks() : program.audio_.front();
        select_audio_track(reader, i);

        reader->getSelectedAudioTrackInfo(sel_audio_);
        reader->getAudioTraits(sel_audio_traits_);
        sel_audio_initialized_ = true;
      }

      if (sinfo.isValid())
      {
        // select another subtitle track:
        std::size_t i = program.subs_.empty() ?
          reader->subsCount() : program.subs_.front();
        select_subtt_track(reader, i);
        reader->subsInfo(i, sel_subtt_);
        sel_subtt_initialized_ = true;
      }
    }

    prepare_to_render(reader, paused_);

    if (sel_video_.isValid() && sel_video_.program_ == prev_program)
    {
      double t = timeline_.currentTime();
      reader->seek(t);
    }
    else
    {
      timeline_.resetFor(reader);
    }

    curr_tracks.audio_ = reader->getSelectedAudioTrackIndex();
    curr_tracks.video_ = reader->getSelectedVideoTrackIndex();
    curr_tracks.subtt_ = get_selected_subtt_track(*reader);

    reader->threadStart();

    resume_renderers(true);
    return sel_video_.isValid();
  }

  //----------------------------------------------------------------
  // PlayerItem::subtt_select_track
  //
  bool
  PlayerItem::subtt_select_track(std::size_t index, Tracks & curr_tracks)
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      sel_subtt_.clear();
      sel_subtt_initialized_ = true;
      sel_subtt_cc_ = index;
      return false;
    }

    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program = get_curr_program(reader, vinfo, ainfo, sinfo);

    TIgnoreClockStop ignore_clock_stop(timeline_);
    reader->threadStop();
    stop_renderers();

    select_subtt_track(reader, index);
    sel_subtt_format_ = reader->subsInfo(index, sel_subtt_);
    sel_subtt_initialized_ = true;

    // if the subtitles program is not the same as the audio/video program
    // then change the audio/video track to a matching subtitles program:
    if (sel_subtt_.isValid() && sel_subtt_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(sel_subtt_.program_, program));

      if (vinfo.isValid())
      {
        // select another video track:
        std::size_t i = program.video_.empty() ?
          reader->getNumberOfVideoTracks() : program.video_.front();
        select_video_track(reader, i);

        reader->getSelectedVideoTrackInfo(sel_video_);
        reader->getVideoTraits(sel_video_traits_);
        sel_video_initialized_ = true;
      }

      if (ainfo.isValid())
      {
        // select another audio track:
        std::size_t i = program.audio_.empty() ?
          reader->getNumberOfAudioTracks() : program.audio_.front();
        select_audio_track(reader, i);

        reader->getSelectedAudioTrackInfo(sel_audio_);
        reader->getAudioTraits(sel_audio_traits_);
        sel_audio_initialized_ = true;
      }
    }

    prepare_to_render(reader, paused_);

    if (sel_subtt_.isValid() && sel_subtt_.program_ == prev_program)
    {
      double t = timeline_.currentTime();
      reader->seek(t);
    }
    else
    {
      timeline_.resetFor(reader);
    }

    curr_tracks.audio_ = reader->getSelectedAudioTrackIndex();
    curr_tracks.video_ = reader->getSelectedVideoTrackIndex();
    curr_tracks.subtt_ = get_selected_subtt_track(*reader);

    reader->threadStart();

    resume_renderers();
    return sel_subtt_.isValid();
  }

  //----------------------------------------------------------------
  // PlayerItem::get_current_chapter
  //
  std::size_t
  PlayerItem::get_current_chapter() const
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      return 0;
    }

    const double playheadInSeconds = timeline_.currentTime();
    const std::size_t numChapters = reader->countChapters();
    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      if (reader->getChapterInfo(i, ch))
      {
        double t0_sec = ch.t0_sec();
        double t1_sec = ch.t1_sec();

        if ((playheadInSeconds >= t0_sec &&
             playheadInSeconds < t1_sec) ||
            (playheadInSeconds < t0_sec && i > 0))
        {
          std::size_t index = (playheadInSeconds >= t0_sec) ? i : i - 1;
          return index;
        }
      }
      else
      {
        YAE_ASSERT(false);
      }
    }

    return numChapters;
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_to_next_chapter
  //
  bool
  PlayerItem::skip_to_next_chapter()
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      return false;
    }

    const double playheadInSeconds = timeline_.currentTime();
    const std::size_t numChapters = reader->countChapters();

    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      if (reader->getChapterInfo(i, ch))
      {
        double t0_sec = ch.t0_sec();

        if (playheadInSeconds < t0_sec)
        {
          timeline_.seekTo(t0_sec);
          return true;
        }
      }
      else
      {
        YAE_ASSERT(false);
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_to_chapter
  //
  void
  PlayerItem::skip_to_chapter(std::size_t index)
  {
    IReader * reader = reader_.get();
    if (!reader)
    {
      return;
    }

    TChapter ch;
    bool ok = reader->getChapterInfo(index, ch);
    YAE_ASSERT(ok);

    double t0_sec = ch.t0_sec();
    timeline_.seekTo(t0_sec);
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_to_next_frame
  //
  void
  PlayerItem::skip_to_next_frame()
  {
    if (!paused_ || !reader_)
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

    yae::TTime startTime = yae::TTime::now();
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
        yae::TTime now = yae::TTime::now();
        if ((now - startTime).get(1000) > 2000)
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

  //----------------------------------------------------------------
  // PlayerItem::skip_forward
  //
  void
  PlayerItem::skip_forward()
  {
    timeline_.seekFromCurrentTime(7.0);
  }

  //----------------------------------------------------------------
  // PlayerItem::skip_back
  //
  void
  PlayerItem::skip_back()
  {
    timeline_.seekFromCurrentTime(-3.0);
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
    if (!reader)
    {
      return;
    }

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

    Canvas * canvas = get_canvas();
    video_->open(canvas, reader);

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
    Canvas * canvas = get_canvas();

    std::size_t num_video_tracks = reader->getNumberOfVideoTracks();
    reader->selectVideoTrack(video_track);

    VideoTraits vtts;
    if (canvas && reader->getVideoTraits(vtts))
    {
      const bool luminance16_not_supported = !yae::get_supports_luminance16();

      bool skip_color_converter = skip_color_converter_.get();
      canvas->skipColorConverter(skip_color_converter);

      TPixelFormatId format = kInvalidPixelFormat;
      if (canvas->
          canvasRenderer()->
          adjustPixelFormatForOpenGL(skip_color_converter, vtts, format) ||
          luminance16_not_supported)
      {
        const pixelFormat::Traits * ptts_native =
          pixelFormat::getTraits(vtts.pixelFormat_);

        const pixelFormat::Traits * ptts_output =
          pixelFormat::getTraits(format);

        const bool adjusted_pixel_format = (format != vtts.pixelFormat_);
        vtts.setPixelFormat(format);

        const unsigned int native_w = vtts.encodedWidth_;
        const unsigned int native_h = vtts.encodedHeight_;

        if (luminance16_not_supported)
        {
          while (vtts.encodedWidth_ > 1280 ||
                 vtts.encodedHeight_ > 720)
          {
            vtts.encodedWidth_ >>= 1;
            vtts.encodedHeight_ >>= 1;
            vtts.offsetTop_ >>= 1;
            vtts.offsetLeft_ >>= 1;
            vtts.visibleWidth_ >>= 1;
            vtts.visibleHeight_ >>= 1;
          }
        }
        else
        {
          // NOTE: overriding frame size implies scaling, so don't do it
          // unless you really want to scale the images in the reader;
          // In general, leave scaling to OpenGL:
          vtts.encodedWidth_ = 0;
          vtts.encodedHeight_ = 0;
        }

        // preserve pixel aspect ratio:
        vtts.pixelAspectRatio_ = 0.0;

        yae_dlog("native: %s %ux%u, output: %s %ux%u",
                 ptts_native ? ptts_native->name_ : "none",
                 native_w,
                 native_h,
                 ptts_output ? ptts_output->name_ : "none",
                 vtts.encodedWidth_ ? vtts.encodedWidth_ : native_w,
                 vtts.encodedHeight_ ? vtts.encodedHeight_ : native_h);

        reader->setVideoTraitsOverride(vtts);
      }
    }

    if (reader->getVideoTraitsOverride(vtts))
    {
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

      if (!ptts && vtts.pixelFormat_ != kInvalidPixelFormat)
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
    sel_subtt_cc_ = cc;

    for (std::size_t i = 0; i < nsubs; i++)
    {
      bool enable = (i == subtt_track);
      reader->setSubsRender(i, enable);
    }

    Canvas * canvas = get_canvas();
    if (canvas)
    {
      canvas->setSubs(std::list<TSubsFrame>());
    }
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
      if (native.ch_layout_.nb_channels > 2 &&
          downmix_to_stereo_.get())
      {
        native.ch_layout_.set_default_layout(2);
      }

      if (!native.ch_layout_.u.mask)
      {
        native.ch_layout_.set_default_layout(native.ch_layout_.nb_channels);
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
}
