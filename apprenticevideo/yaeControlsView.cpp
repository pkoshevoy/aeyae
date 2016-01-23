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
  // ControlsVisible
  //
  struct ControlsVisible : public TBoolExpr
  {
    ControlsVisible(ControlsView & view, Item & controls):
      view_(view),
      controls_(controls)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = controls_.overlaps(pt);
    }

    ControlsView & view_;
    Item & controls_;
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
      std::cerr << "FIXME: TogglePlayback" << std::endl;
      return true;
    }

    ControlsView & view_;
  };


  //----------------------------------------------------------------
  // ControlsView::ControlsView
  //
  ControlsView::ControlsView():
    ItemView("controls"),
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

    ColorRef color = root.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kFg, 0, 0.5));

    // re-apply style when playlist is enabled or disabled:
    playlist_->root()->
      addObserver(Item::kOnToggleItemView,
                  Item::TObserverPtr(new Repaint(*this, true)));

    Item & container = root.addNew<Item>("container");
    container.anchors_.fill(root);
    container.anchors_.top_.reset();
    container.height_ = container.
      addExpr(new StyleTitleHeight(*playlist), 1.5);

    Item & controls = root.addNew<Item>("controls");
    controls.anchors_.hcenter_ =
      ItemRef::reference(container, kPropertyHCenter);
    controls.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    controls.height_ = controls.
      addExpr(new StyleTitleHeight(*playlist), 0.67);
    controls.width_ = ItemRef::scale(controls, kPropertyHeight, 1.0);

    Item & playbackButton = controls.addNew<Item>("playbackButton");
    {
      playbackButton.anchors_.top_ =
        ItemRef::reference(controls, kPropertyTop);

      playbackButton.anchors_.left_ =
        ItemRef::reference(controls, kPropertyLeft);

      playbackButton.height_ = playbackButton.addExpr
        (new OddRoundUp(controls, kPropertyHeight));

      playbackButton.width_ =
        ItemRef::reference(playbackButton, kPropertyHeight);

      TexturedRect & play = playbackButton.add(new TexturedRect("play"));
      play.anchors_.fill(playbackButton);
      play.visible_ = play.addExpr(new OnPlaybackPaused(*this, true));
      play.texture_ = play.addExpr(new StyleCollapsedTexture(*playlist));

      Item & pause = playbackButton.addNew<Item>("pause");
      pause.anchors_.fill(playbackButton);
      pause.visible_ = pause.addExpr(new OnPlaybackPaused(*this, false));
      {
        Rectangle & p1 = pause.addNew<Rectangle>("p1");
        p1.anchors_.left_ = ItemRef::reference(pause, kPropertyLeft);
        p1.anchors_.top_ = ItemRef::reference(pause, kPropertyTop);
        p1.width_ = ItemRef::scale(pause, kPropertyWidth, 0.3);
        p1.height_ = ItemRef::reference(pause, kPropertyHeight);
        p1.margins_.left_ = p1.width_;
        p1.color_ = color;

        Rectangle & p2 = pause.addNew<Rectangle>("p2");
        p2.anchors_.left_ = ItemRef::reference(p1, kPropertyRight);
        p2.anchors_.top_ = ItemRef::reference(pause, kPropertyTop);
        p2.width_ = p1.width_;
        p2.height_ = ItemRef::reference(pause, kPropertyHeight);
        p2.margins_.left_ = p1.width_;
        p2.color_ = color;
      }
    }

    TogglePlayback & playback = controls.add(new TogglePlayback(*this));
    playback.anchors_.fill(playbackButton);
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
    Item & controls = root["controls"];

    Item & playbackButton = controls["playbackButton"];
    requestUncache(&playbackButton);

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
