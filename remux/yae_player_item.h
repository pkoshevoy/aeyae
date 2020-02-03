// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 24 22:23:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_ITEM_H_
#define YAE_PLAYER_ITEM_H_

// standard:
#include <limits>

// Qt library
#include <QObject>

// aeyae:
#include "yae/utils/yae_time.h"
#include "yae/video/yae_audio_renderer.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_synchronous.h"
#include "yae/video/yae_video_canvas.h"
#include "yae/video/yae_video_renderer.h"

// local:
#include "yaeCanvas.h"
#include "yaeItem.h"
#include "yaeTimelineModel.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerItem
  //
  class YAEUI_API PlayerItem : public QObject, public Item
  {
    Q_OBJECT;

  public:
    // NOTE: this will use the delegate windowCanvas by default:
    PlayerItem(const char * id);

    // canvas delegate calls screensaver inhibitor,
    // so set it if you need it:
    void setCanvasDelegate(const yae::shared_ptr<Canvas::IDelegate> & d);

    // NOTE: this will construct a separate personal canvas:
    void makePersonalCanvas(const yae::shared_ptr<IOpenGLContext> & ctx);

    // virtual:
    void uncache();
    void paintContent() const;

    void playback(const IReaderPtr & reader,
                  std::size_t vtrack = 0,
                  std::size_t atrack = 0,
                  std::size_t strack = 0,
                  unsigned int cc = 0,
                  const TTime & seek_time = TTime(0, 0));

  signals:
    void maybe_animate_opacity();

  public slots:
    void user_is_seeking(bool seeking);
    void move_time_in(double seconds);
    void move_time_out(double seconds);
    void move_playhead(double seconds);

  public:
    // accessors:
    inline Canvas * get_window_canvas() const
    { return canvas_delegate_ ? &(canvas_delegate_->windowCanvas()) : NULL; }

    inline Canvas * get_personal_canvas() const
    { return personal_canvas_ ? personal_canvas_.get() : NULL; }

    inline Canvas * get_canvas() const
    {
      return
        personal_canvas_ ? personal_canvas_.get() :
        canvas_delegate_ ? &(canvas_delegate_->windowCanvas()) :
        NULL;
    }

    inline const IReaderPtr & reader() const
    { return reader_; }

    inline const TimelineModel & timeline() const
    { return timeline_; }

    inline TimelineModel & timeline()
    { return timeline_; }

    inline const TAudioRendererPtr & audio() const
    { return audio_; }

    inline const TVideoRendererPtr & video() const
    { return video_; }

    inline bool paused() const
    { return paused_; }

    void toggle_playback();
    void playback_stop();

    void set_downmix_to_stereo(bool downmix);
    void set_loop_playback(bool loop_playback);
    void skip_color_converter(bool skip);
    void skip_loopfilter(bool skip);
    void skip_nonref_frames(bool skip);
    void set_deinterlace(bool deint);
    void set_playback_tempo(double tempo);

    //----------------------------------------------------------------
    // Tracks
    //
    struct Tracks
    {
      Tracks(std::size_t audio = 0,
             std::size_t video = 0,
             std::size_t subtt = 0):
        audio_(audio),
        video_(video),
        subtt_(subtt)
      {}

      std::size_t audio_;
      std::size_t video_;
      std::size_t subtt_;
    };

    // select a track, pass back updated track selection.
    //
    // NOTE: track selection can change for other tracks
    // if selected track belongs to a different program:
    //
    bool audio_select_track(std::size_t index, Tracks & curr_tracks);
    bool video_select_track(std::size_t index, Tracks & curr_tracks);
    bool subtt_select_track(std::size_t index, Tracks & curr_tracks);

    std::size_t get_current_chapter() const;
    bool skip_to_next_chapter();
    void skip_to_chapter(std::size_t index);
    void skip_to_next_frame();
    void skip_forward();
    void skip_back();

  protected:
    void stop_renderers();
    void prepare_to_render(IReader * reader, bool frame_stepping);

    void select_audio_track(IReader * reader, std::size_t audio_track);
    void select_video_track(IReader * reader, std::size_t video_track);
    void select_subtt_track(IReader * reader, std::size_t subtt_track);

    void adjust_audio_traits_override(IReader * reader);
    void resume_renderers(bool load_next_frame_if_paused = false);

    BoolRef downmix_to_stereo_;
    BoolRef loop_playback_;
    BoolRef skip_loopfilter_;
    BoolRef skip_nonref_frames_;
    BoolRef skip_color_converter_;
    BoolRef deinterlace_;
    DataRef<double> playback_tempo_;

    IReaderPtr reader_;
    uint64_t reader_id_;

    TimelineModel timeline_;
    TAudioRendererPtr audio_;
    TVideoRendererPtr video_;

    yae::shared_ptr<Canvas::IDelegate> canvas_delegate_;
    yae::shared_ptr<Canvas> personal_canvas_;
    bool paused_;

    // selected track info is used to select matching track(s)
    // when loading next file:
    TTrackInfo sel_video_;
    VideoTraits sel_video_traits_;

    TTrackInfo sel_audio_;
    AudioTraits sel_audio_traits_;

    TTrackInfo sel_subtt_;
    TSubsFormat sel_subtt_format_;
  };

}


#endif // YAE_PLAYER_ITEM_H_
