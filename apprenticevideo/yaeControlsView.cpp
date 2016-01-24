// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 22 19:20:22 PST 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt includes:
#include <QPalette>

// local includes:
#include "yaeColor.h"
#include "yaeControlsView.h"
#include "yaeItemRef.h"
#include "yaeMainWindow.h"
#include "yaePlaylistView.h"
#include "yaePlaylistViewStyle.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeSegment.h"
#include "yaeTexturedRect.h"


namespace yae
{

  //----------------------------------------------------------------
  // ExposePlaylist
  //
  struct ExposePlaylist : public TBoolExpr
  {
    ExposePlaylist(ControlsView & view, Item & item):
      view_(view),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = (view_.mainWindow_ == NULL ||
                view_.mainWindow_->isPlaylistVisible() ||
                view_.mainWindow_->isTimelineVisible() ||
                item_.overlaps(pt));
    }

    ControlsView & view_;
    Item & item_;
  };

  //----------------------------------------------------------------
  // ExposeControls
  //
  struct ExposeControls : public TBoolExpr
  {
    ExposeControls(ControlsView & view, Item & item):
      view_(view),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = (view_.mainWindow_ == NULL ||
                (!view_.mainWindow_->isPlaylistVisible() &&
                 item_.overlaps(pt)));
    }

    ControlsView & view_;
    Item & item_;
  };

  //----------------------------------------------------------------
  // OnTimelineVisible
  //
  struct OnTimelineVisible : public TBoolExpr
  {
    OnTimelineVisible(ControlsView & view, bool result):
      view_(view),
      result_(result)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool visible = (view_.mainWindow_ == NULL ||
                      view_.mainWindow_->isPlaylistVisible());
      result = visible ? result_ : !result_;
    }

    ControlsView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // OnPlaybackPaused
  //
  struct OnPlaybackPaused : public TBoolExpr
  {
    OnPlaybackPaused(ControlsView & view, bool result):
      view_(view),
      result_(result)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool paused = (view_.mainWindow_ == NULL ||
                     view_.mainWindow_->isPlaybackPaused());
      result = paused ? result_ : !result_;
    }

    ControlsView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // TogglePlayback
  //
  struct TogglePlayback : public ClickableItem
  {
    TogglePlayback(ControlsView & view):
      ClickableItem("toggle_playback"),
      view_(view)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (view_.mainWindow_)
      {
        view_.mainWindow_->togglePlayback();
      }

      return true;
    }

    ControlsView & view_;
  };

  //----------------------------------------------------------------
  // OnPlaylistVisible
  //
  struct OnPlaylistVisible : public TBoolExpr
  {
    OnPlaylistVisible(ControlsView & view, bool result):
      view_(view),
      result_(result)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool visible = (view_.mainWindow_ == NULL ||
                      view_.mainWindow_->isPlaylistVisible());
      result = visible ? result_ : !result_;
    }

    ControlsView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // TogglePlaylist
  //
  struct TogglePlaylist : public ClickableItem
  {
    TogglePlaylist(ControlsView & view):
      ClickableItem("toggle_playlist"),
      view_(view)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (view_.mainWindow_)
      {
        view_.mainWindow_->togglePlaylist();
      }

      return true;
    }

    ControlsView & view_;
  };


  //----------------------------------------------------------------
  // ControlsView::ControlsView
  //
  ControlsView::ControlsView():
    ItemView("controls_view"),
    mainWindow_(NULL),
    playlist_(NULL)
  {}

  //----------------------------------------------------------------
  // ControlsView::setPlaylistView
  //
  void
  ControlsView::setup(MainWindow * mainWindow, PlaylistView * playlist)
  {
    if (!(mainWindow && playlist))
    {
      YAE_ASSERT(false);
      return;
    }

    YAE_ASSERT(!mainWindow_);
    mainWindow_ = mainWindow;

    YAE_ASSERT(!playlist_);
    playlist_ = playlist;


    Item & root = *root_;

    ColorRef colorControlsBg = root.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kBgTimecode));

    ColorRef colorControlsFg = root.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kFgTimecode));

    // re-apply style when playlist is enabled or disabled:
    playlist_->root()->
      addObserver(Item::kOnToggleItemView,
                  Item::TObserverPtr(new Repaint(*this, true)));


    Item & playlistButton = root.addNew<Item>("playlistButton");
    TogglePlaylist & playlistToggle = root.add(new TogglePlaylist(*this));
    {
      playlistButton.visible_ = playlistButton.
        addExpr(new ExposePlaylist(*this, playlistToggle));
      playlistButton.visible_.cachingEnabled_ = false;
      playlistButton.anchors_.top_ =
        ItemRef::offset(root, kPropertyTop, 2);
      playlistButton.anchors_.left_ =
        ItemRef::reference(root, kPropertyLeft);
      playlistButton.width_ = playlistButton.
        addExpr(new StyleTitleHeight(*playlist), 1.5);
      playlistButton.height_ = playlistButton.width_;

      TexturedRect & gridOn = playlistButton.add(new TexturedRect("gridOn"));
      gridOn.anchors_.fill(playlistButton);
      gridOn.margins_.set(ItemRef::scale(playlistButton, kPropertyHeight,
                                         0.2));
      gridOn.visible_ = gridOn.addExpr(new OnPlaylistVisible(*this, true));
      gridOn.texture_ = gridOn.addExpr(new StyleGridOnTexture(*playlist));

      TexturedRect & gridOff = playlistButton.add(new TexturedRect("gridOff"));
      gridOff.anchors_.fill(playlistButton);
      gridOff.margins_.set(ItemRef::scale(playlistButton, kPropertyHeight,
                                          0.2));
      gridOff.visible_ = gridOff.addExpr(new OnPlaylistVisible(*this, false));
      gridOff.texture_ = gridOff.addExpr(new StyleGridOffTexture(*playlist));

      playlistToggle.anchors_.fill(playlistButton);
    }


    RoundRect & controls = root.addNew<RoundRect>("controls");
    Item & mouseDetect = root.addNew<Item>("mouse_detect");
    MouseTrap & mouseTrap = controls.addNew<MouseTrap>("mouse_trap");
    controls.anchors_.vcenter_ =
      ItemRef::reference(root, kPropertyVCenter);
    controls.anchors_.hcenter_ =
      ItemRef::reference(root, kPropertyHCenter);
    controls.height_ = controls.
      addExpr(new StyleTitleHeight(*playlist), 3.0);
    controls.visible_ = controls.
      addExpr(new ExposeControls(*this, mouseDetect));
    controls.visible_.cachingEnabled_ = false;
    mouseTrap.anchors_.fill(controls);

    double cells = 2.0;
    controls.width_ = ItemRef::scale(controls, kPropertyHeight, cells);
    controls.radius_ = ItemRef::scale(controls, kPropertyHeight, 0.1);
    controls.color_ = colorControlsBg;

    Item & playbackButton = controls.addNew<Item>("playbackButton");
    TogglePlayback & playbackToggle = controls.add(new TogglePlayback(*this));
    {
      playbackButton.anchors_.vcenter_ =
        ItemRef::reference(controls, kPropertyVCenter);

      playbackButton.anchors_.left_ =
        ItemRef::reference(controls, kPropertyLeft);

      playbackButton.margins_.left_ =
        ItemRef::scale(controls, kPropertyWidth, 0.5 / cells);

      playbackButton.width_ = ItemRef::reference(controls, kPropertyHeight);
      playbackButton.height_ = playbackButton.width_;

      TexturedRect & play = playbackButton.add(new TexturedRect("play"));
      play.anchors_.fill(playbackButton);
      play.margins_.set(ItemRef::scale(playbackButton, kPropertyHeight, 0.15));
      play.visible_ = play.addExpr(new OnPlaybackPaused(*this, true));
      play.texture_ = play.addExpr(new StylePlayTexture(*playlist));

      TexturedRect & pause = playbackButton.add(new TexturedRect("pause"));
      pause.anchors_.fill(playbackButton);
      pause.margins_.set(ItemRef::scale(playbackButton, kPropertyHeight, 0.2));
      pause.visible_ = pause.addExpr(new OnPlaybackPaused(*this, false));
      pause.texture_ = pause.addExpr(new StylePauseTexture(*playlist));

      playbackToggle.anchors_.fill(playbackButton);
      mouseDetect.anchors_.fill(controls);
    }
  }

  //----------------------------------------------------------------
  // ControlsView::resizeTo
  //
  bool
  ControlsView::resizeTo(const Canvas * canvas)
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
  // ControlsView::processMouseTracking
  //
  bool
  ControlsView::processMouseTracking(const TVec2D & mousePt)
  {
    (void)mousePt;

    if (!this->isEnabled())
    {
      return false;
    }

    Item & root = *root_;
#if 0
    Item & controls = root["controls"];
    Item & playbackButton = controls["playbackButton"];
    requestUncache(&playbackButton);
#else
    // requestUncache(&root);
#endif

    return true;
  }

  //----------------------------------------------------------------
  // ControlsView::controlsChanged
  //
  void
  ControlsView::controlsChanged()
  {
    if (this->isEnabled())
    {
      this->requestUncache();
      this->requestRepaint();
    }
  }

}
