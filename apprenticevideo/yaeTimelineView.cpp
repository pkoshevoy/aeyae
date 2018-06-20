// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 23:01:16 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt includes:
#include <QPalette>

// local includes:
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeItemFocus.h"
#include "yaeItemRef.h"
#include "yaeMainWindow.h"
#include "yaePlaylistView.h"
#include "yaePlaylistViewStyle.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeSegment.h"
#include "yaeText.h"
#include "yaeTextInput.h"
#include "yaeTexturedRect.h"
#include "yaeTimelineModel.h"
#include "yaeTimelineView.h"

namespace yae
{

  //----------------------------------------------------------------
  // TimelineShadowWidth
  //
  struct TimelineShadowWidth : public GetScrollviewWidth
  {
    TimelineShadowWidth(const PlaylistView & playlist):
      GetScrollviewWidth(playlist)
    {}

    void evaluate(double & result) const
    {
      if (playlist_.isEnabled())
      {
        GetScrollviewWidth::evaluate(result);
      }
      else
      {
        result = playlist_.width();
      }
    }
  };

  //----------------------------------------------------------------
  // IsTimelineVisible
  //
  struct IsTimelineVisible : public TBoolExpr
  {
    IsTimelineVisible(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool visible = (!view_.mainWindow_ ||
                      view_.mainWindow_->isTimelineVisible());
      result = visible;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool paused = (view_.mainWindow_ == NULL ||
                     view_.mainWindow_->isPlaybackPaused());
      result = paused;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->togglePlayback();
  }


  //----------------------------------------------------------------
  // IsFullscreen
  //
  struct IsFullscreen : public TBoolExpr
  {
    IsFullscreen(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool fullscreen = (view_.mainWindow_ == NULL ||
                         view_.mainWindow_->isFullScreen());
      result = fullscreen;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // toggle_fullscreen
  //
  static void
  toggle_fullscreen(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->requestToggleFullScreen();
  }


  //----------------------------------------------------------------
  // IsPlaylistVisible
  //
  struct IsPlaylistVisible : public TBoolExpr
  {
    IsPlaylistVisible(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool visible = (view_.mainWindow_ == NULL ||
                      view_.mainWindow_->isPlaylistVisible());
      result = visible;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playlist
  //
  static void
  toggle_playlist(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->togglePlaylist();
  }


  //----------------------------------------------------------------
  // AnimateOpacity
  //
  struct AnimateOpacity : public Item::Observer
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


  //----------------------------------------------------------------
  // TimelineView::TimelineView
  //
  TimelineView::TimelineView():
    ItemView("timeline"),
    mainWindow_(NULL),
    playlist_(NULL),
    model_(NULL)
  {}

  //----------------------------------------------------------------
  // TimelineView::setPlaylistView
  //
  void
  TimelineView::setup(MainWindow * mainWindow,
                      PlaylistView * playlist,
                      TimelineModel * model)
  {
    if (!(mainWindow && playlist && model))
    {
      YAE_ASSERT(false);
      return;
    }

    YAE_ASSERT(!mainWindow_);
    mainWindow_ = mainWindow;

    YAE_ASSERT(!playlist_);
    playlist_ = playlist;

    YAE_ASSERT(!model_);
    model_ = model;

    setRoot(ItemPtr(new TimelineItem("timeline_item", *this, *model_)));
    TimelineItem & timeline = timelineItem();

    timeline.is_playback_paused_ = timeline.addExpr
      (new IsPlaybackPaused(*this));

    timeline.is_fullscreen_ = timeline.addExpr
      (new IsFullscreen(*this));

    timeline.is_playlist_visible_ = timeline.addExpr
      (new IsPlaylistVisible(*this));

    timeline.is_timeline_visible_ = timeline.addExpr
      (new IsTimelineVisible(*this));

    timeline.toggle_playback_.reset(&toggle_playback, mainWindow_);
    timeline.toggle_fullscreen_.reset(&toggle_fullscreen, mainWindow_);
    timeline.toggle_playlist_.reset(&toggle_playlist, mainWindow_);

    timeline.layout();

    // re-apply style when playlist is enabled or disabled:
    Item::TObserverPtr repaintTimeline(new Repaint(*this, true));
    playlist_->root()->addObserver(Item::kOnToggleItemView, repaintTimeline);

    Item::TObserverPtr animateOpacity(new AnimateOpacity(timeline));
    playlist_->root()->addObserver(Item::kOnToggleItemView, animateOpacity);

    TextInputProxy & playheadFocus =
      timeline.get<TextInputProxy>("playheadFocus");
    playheadFocus.addObserver(Item::kOnFocus, animateOpacity);
    playheadFocus.addObserver(Item::kOnFocusOut, animateOpacity);

    // connect the model:
    bool ok = true;

    ok = connect(model_, SIGNAL(markerTimeInChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(markerTimeOutChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(markerPlayheadChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    TextInput & playheadEdit = timeline.get<TextInput>("playheadEdit");
    ok = connect(&playheadEdit, SIGNAL(editingFinished(const QString &)),
                 model_, SLOT(seekTo(const QString &)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TimelineView::style
  //
  const ItemViewStyle *
  TimelineView::style() const
  {
    return playlist_ ? playlist_->style() : NULL;
  }

  //----------------------------------------------------------------
  // TimelineView::setEnabled
  //
  void
  TimelineView::setEnabled(bool enable)
  {
    ItemView::setEnabled(enable);

    if (enable)
    {
      maybeAnimateOpacity();
    }
  }

  //----------------------------------------------------------------
  // TimelineView::resizeTo
  //
  bool
  TimelineView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }

    if (playlist_)
    {
      PlaylistViewStyle & style = playlist_->playlistViewStyle();
      requestUncache(&style);
    }

    return true;
  }

  //----------------------------------------------------------------
  // TimelineView::processMouseTracking
  //
  bool
  TimelineView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    TimelineItem & timelineItem = this->timelineItem();
    Item & timeline = timelineItem["timeline"];

    Item & timelineIn = timeline["timelineIn"];
    requestUncache(&timelineIn);

    Item & timelinePlayhead = timeline["timelinePlayhead"];
    requestUncache(&timelinePlayhead);

    Item & timelineOut = timeline["timelineOut"];
    requestUncache(&timelineOut);

    Item & timelineEnd = timeline["timelineEnd"];
    requestUncache(&timelineEnd);

    Item & inPoint = timelineItem["inPoint"];
    requestUncache(&inPoint);

    Item & playhead = timelineItem["playhead"];
    requestUncache(&playhead);

    Item & outPoint = timelineItem["outPoint"];
    requestUncache(&outPoint);

    // update the opacity transitions:
    maybeAnimateOpacity();
    maybeAnimateControls();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineView::modelChanged
  //
  void
  TimelineView::modelChanged()
  {
    if (this->isEnabled())
    {
      this->requestUncache();
      this->requestRepaint();
    }
  }

}
