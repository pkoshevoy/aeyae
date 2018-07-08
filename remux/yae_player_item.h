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
  class YAE_API PlayerItem : public QObject, public Item
  {
    Q_OBJECT;

  public:
    PlayerItem(const char * id, const yae::shared_ptr<IOpenGLContext> & ctx);

    // virtual:
    void uncache();
    void paintContent() const;

    void playback(const IReaderPtr & reader,
                  std::size_t vtrack = 0,
                  std::size_t atrack = 0,
                  std::size_t strack = 0,
                  unsigned int cc = 0,
                  const TTime & seek_time = TTime(0, 0));

  public slots:
    void user_is_seeking(bool seeking);
    void move_time_in(double seconds);
    void move_time_out(double seconds);
    void move_playhead(double seconds);

  public:
    // accessors:
    inline TimelineModel & timeline()
    { return timeline_; }

    inline bool paused() const
    { return paused_; }

    void toggle_playback();
    void playback_stop();

  protected:
    void stop_renderers();
    void prepare_to_render(IReader * reader, bool frame_stepping);

    void select_audio_track(IReader * reader, std::size_t audio_track);
    void select_video_track(IReader * reader, std::size_t video_track);
    void select_subtt_track(IReader * reader, std::size_t subtt_track);

    void adjust_audio_traits_override(IReader * reader);
    void resume_renderers(bool load_next_frame_if_paused = false);
    void skip_to_next_frame();

    BoolRef downmix_to_stereo_;
    BoolRef loop_playback_;
    BoolRef skip_loopfilter_;
    BoolRef skip_nonref_frames_;
    BoolRef skip_color_converter_;
    BoolRef deinterlace_;
    DataRef<double> playback_tempo_;

    IReaderPtr reader_;
    TimelineModel timeline_;
    TAudioRendererPtr audio_;
    TVideoRendererPtr video_;
    mutable Canvas canvas_;
    bool paused_;
    uint64_t reader_id_;
  };

}


#endif // YAE_PLAYER_ITEM_H_
