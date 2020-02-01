// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:07:25 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local:
#include "yaeAppView.h"
#include "yaePlayerView.h"


namespace yae
{

  //----------------------------------------------------------------
  // ShowPlayer
  //
  struct ShowPlayer : public TBoolExpr
  {
    ShowPlayer(const PlayerView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.recording_;
    }

    const PlayerView & view_;
  };


  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(const PlayerView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.is_playback_paused();
    }

    const PlayerView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->toggle_playback();
  }


  //----------------------------------------------------------------
  // PlayerView::PlayerView
  //
  PlayerView::PlayerView():
    ItemView("PlayerView"),
    style_(NULL),
    sync_ui_(this)
  {
    bool ok = connect(&sync_ui_, SIGNAL(timeout()), this, SLOT(sync_ui()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerView::setStyle
  //
  void
  PlayerView::setStyle(const AppStyle * style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // PlayerView::setContext
  //
  void
  PlayerView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    ItemView::setContext(context);

    player_.reset(new PlayerItem("player", context));
    PlayerItem & player = *player_;

    YAE_ASSERT(delegate_);
    player.setCanvasDelegate(delegate_);

    timeline_.reset(new TimelineItem("player_timeline",
                                     *this,
                                     player.timeline()));

    TimelineItem & timeline = *timeline_;
    timeline.is_playback_paused_ = timeline.addExpr
      (new IsPlaybackPaused(*this));

    timeline.is_fullscreen_ = timeline.addExpr
      (new IsFullscreen(*this));

    timeline.is_playlist_visible_ = BoolRef::constant(false);
    timeline.is_timeline_visible_ = BoolRef::constant(false);
    timeline.toggle_playback_.reset(&yae::toggle_playback, this);
    timeline.toggle_fullscreen_ = this->toggle_fullscreen_;
    timeline.layout();
  }

  //----------------------------------------------------------------
  // PlayerView::setEnabled
  //
  void
  PlayerView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.children_.clear();
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);
    root.uncache();
    uncache_.clear();

    if (enable)
    {
      layout(*this, *style_, root);
      sync_ui_.start(1000);
    }
    else
    {
      sync_ui_.stop();
    }

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // PlayerView::processMouseTracking
  //
  bool
  PlayerView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    Item & root = *root_;
    if (recording_)
    {
      Item & player = root["player"];
      TimelineItem & timeline = player.get<TimelineItem>("timeline_item");
      timeline.processMouseTracking(mousePt);
    }

    return true;
  }

  //----------------------------------------------------------------
  // PlayerView::toggle_playback
  //
  void
  PlayerView::toggle_playback()
  {
    player_->toggle_playback();

    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();

    if (!is_playback_paused())
    {
      timeline_->forceAnimateControls();
    }
    else
    {
      timeline_->maybeAnimateControls();
    }
  }

  //----------------------------------------------------------------
  // PlayerView::sync_ui
  //
  void
  PlayerView::sync_ui()
  {
    requestRepaint();
  }

  //----------------------------------------------------------------
  // PlayerView::is_playback_paused
  //
  bool
  PlayerView::is_playback_paused() const
  {
    return player_->paused();
  }

  //----------------------------------------------------------------
  // PlayerView::layout
  //
  void
  PlayerView::layout(PlayerView & view, const AppStyle & style, Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
    hidden.width_ = hidden.
      addExpr(style_item_ref(view, &AppStyle::unit_size_));

    Rectangle & bg = root.addNew<Rectangle>("background");
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_));
    bg.anchors_.fill(root);

    PlayerItem & player = root.add(view.player_);
    player.anchors_.fill(bg);

    TimelineItem & timeline = player.add(view.timeline_);
    timeline.anchors_.fill(player);
  }

}
