// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 17 19:29:05 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_ITEM_H_
#define YAE_TIMELINE_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// standard:
#include <list>

// Qt:
#include <QObject>

// yaeui:
#include "yaeGradient.h"
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeTimelineModel.h"


namespace yae
{

  //----------------------------------------------------------------
  // TimelineConfig
  //
  class YAEUI_API TimelineItem : public QObject,
                                 public Item
  {
    Q_OBJECT;

  public:
    TimelineItem(const char * name, ItemView & view, TimelineModel & model);
    ~TimelineItem();

    void layout();

  public slots:
    void maybeAnimateOpacity();
    void maybeAnimateControls();
    void forceAnimateControls();

  public:
    // virtual:
    void uncache();

    // helper:
    void processMouseTracking(const TVec2D & pt);

    // helper:
    inline double getOpacity() const
    {
      return
        is_timeline_visible_.get() ? 1.0 :
        is_playlist_visible_.get() ? 1.0 :
        opacity_ ? opacity_->transition().get_value() :
        0.0;
    }

  public slots:
    void modelChanged();
    void showTimeline(bool);
    void showPlaylist(bool);

  public:
    ItemView & view_;
    TimelineModel & model_;
    yae::shared_ptr<Gradient, Item> shadow_;
    yae::shared_ptr<TransitionItem, Item> opacity_;

    ItemRef unit_size_;
    BoolRef is_playback_paused_;
    BoolRef is_fullscreen_;
    BoolRef is_playlist_visible_;
    BoolRef is_timeline_visible_;

    ContextQuery<bool> query_playlist_visible_;
    ContextQuery<bool> query_timeline_visible_;

    ContextCallback toggle_playback_;
    ContextCallback toggle_fullscreen_;
    ContextCallback toggle_playlist_;
    ContextCallback back_arrow_cb_;

    ContextCallback frame_crop_cb_;
    ContextCallback aspect_ratio_cb_;
    ContextCallback video_track_cb_;
    ContextCallback audio_track_cb_;
    ContextCallback subtt_track_cb_;
    ContextCallback back_to_prev_cb_;
    ContextCallback skip_to_next_cb_;
    ContextCallback select_all_cb_;
    ContextCallback remove_sel_cb_;
    ContextCallback delete_file_cb_;

    ItemView::TAnimatorPtr opacity_animator_;
    ItemView::TAnimatorPtr controls_animator_;
    Item::TObserverPtr animate_opacity_;
  };


  //----------------------------------------------------------------
  // AnimateOpacity
  //
  struct YAEUI_API AnimateOpacity : public Item::Observer
  {
    AnimateOpacity(TimelineItem & timeline):
      timeline_(timeline)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      timeline_.maybeAnimateOpacity();
      timeline_.forceAnimateControls();
    }

    TimelineItem & timeline_;
  };

}


#endif // YAE_TIMELINE_ITEM_H_
