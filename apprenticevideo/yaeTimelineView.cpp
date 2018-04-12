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
  // TimelineHeight
  //
  struct TimelineHeight : public TDoubleExpr
  {
    TimelineHeight(TimelineView & view, Item & container, Item & timeline):
      view_(view),
      container_(container),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      int h = std::max<int>(2, ~1 & (int(0.5 + timeline_.height()) / 4));

      const TVec2D & pt = view_.mousePt();
      if (container_.overlaps(pt))
      {
        h *= 2;
      }

      result = double(h);
    }

    TimelineView & view_;
    Item & container_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // TimelineIn
  //
  struct TimelineIn : public TDoubleExpr
  {
    TimelineIn(TimelineView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      if (!view_.model())
      {
        result = 0.0;
        return;
      }

      TimelineModel & model = *(view_.model());
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model.markerTimeIn();
      result = x0 + w * t;
    }

    TimelineView & view_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // TimelinePlayhead
  //
  struct TimelinePlayhead : public TDoubleExpr
  {
    TimelinePlayhead(TimelineView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      if (!view_.model())
      {
        result = 0.0;
        return;
      }

      TimelineModel & model = *(view_.model());
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model.markerPlayhead();
      result = x0 + w * t;
    }

    TimelineView & view_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // TimelineOut
  //
  struct TimelineOut : public TDoubleExpr
  {
    TimelineOut(TimelineView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      if (!view_.model())
      {
        result = 0.0;
        return;
      }

      TimelineModel & model = *(view_.model());
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model.markerTimeOut();
      result = x0 + w * t;
    }

    TimelineView & view_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // MarkerVisible
  //
  struct MarkerVisible : public TBoolExpr
  {
    MarkerVisible(TimelineView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = timeline_.overlaps(pt);
    }

    TimelineView & view_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // GetPlayheadAux
  //
  struct GetPlayheadAux : public TVarExpr
  {
    GetPlayheadAux(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (!view_.model())
      {
        result = QVariant(QObject::tr("00:00:00:00"));
        return;
      }

      TimelineModel & model = *(view_.model());
      result = QVariant(model.auxPlayhead());
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // GetDurationAux
  //
  struct GetDurationAux : public TVarExpr
  {
    GetDurationAux(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (!view_.model())
      {
        result = QVariant(QObject::tr("00:00:00:00"));
        return;
      }

      TimelineModel & model = *(view_.model());
      result = QVariant(model.auxDuration());
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // TimelineSeek
  //
  struct TimelineSeek : public InputArea
  {
    TimelineSeek(TimelineView & view):
      InputArea("timeline_seek"),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return !!(view_.model()); }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      double x0 = this->left();
      double w = this->width();
      double x = rootCSysPoint.x() - itemCSysOrigin.x();
      double t = (x - x0) / w;
      model.setMarkerPlayhead(t);
      return true;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // SliderInPoint
  //
  struct SliderInPoint : public InputArea
  {
    SliderInPoint(TimelineView & view, Item & timeline):
      InputArea("slider_in_point"),
      view_(view),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      startPos_ = model.markerTimeIn();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model.setMarkerTimeIn(startPos_ + t);
      return true;
    }

    TimelineView & view_;
    Item & timeline_;
    double startPos_;
  };

  //----------------------------------------------------------------
  // SliderPlayhead
  //
  struct SliderPlayhead : public InputArea
  {
    SliderPlayhead(TimelineView & view, Item & timeline):
      InputArea("slider_playhead"),
      view_(view),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      startPos_ = model.markerPlayhead();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model.setMarkerPlayhead(startPos_ + t);
      return true;
    }

    TimelineView & view_;
    Item & timeline_;
    double startPos_;
  };

  //----------------------------------------------------------------
  // SliderOutPoint
  //
  struct SliderOutPoint : public InputArea
  {
    SliderOutPoint(TimelineView & view, Item & timeline):
      InputArea("slider_out_point"),
      view_(view),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      startPos_ = model.markerTimeOut();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      if (!(view_.model()))
      {
        return false;
      }

      TimelineModel & model = *(view_.model());
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model.setMarkerTimeOut(startPos_ + t);
      return true;
    }

    TimelineView & view_;
    Item & timeline_;
    double startPos_;
  };

  //----------------------------------------------------------------
  // ExposeControls
  //
  struct ExposeControls : public TBoolExpr
  {
    ExposeControls(TimelineView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = (view_.mainWindow_ == NULL ||
                !view_.mainWindow_->isPlaylistVisible());
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // OnPlaybackPaused
  //
  struct OnPlaybackPaused : public TBoolExpr
  {
    OnPlaybackPaused(TimelineView & view, bool result):
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

    TimelineView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // TogglePlayback
  //
  struct TogglePlayback : public ClickableItem
  {
    TogglePlayback(TimelineView & view):
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

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // OnFullscreen
  //
  struct OnFullscreen : public TBoolExpr
  {
    OnFullscreen(TimelineView & view, bool result):
      view_(view),
      result_(result)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool fullscreen = (view_.mainWindow_ == NULL ||
                         view_.mainWindow_->isFullScreen());
      result = fullscreen ? result_ : !result_;
    }

    TimelineView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // ToggleFullscreen
  //
  struct ToggleFullscreen : public ClickableItem
  {
    ToggleFullscreen(TimelineView & view):
      ClickableItem("toggle_fullscreen"),
      view_(view)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (view_.mainWindow_)
      {
        view_.mainWindow_->requestToggleFullScreen();
      }

      return true;
    }

    TimelineView & view_;
  };

  //----------------------------------------------------------------
  // OnPlaylistVisible
  //
  struct OnPlaylistVisible : public TBoolExpr
  {
    OnPlaylistVisible(TimelineView & view, bool result):
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

    TimelineView & view_;
    bool result_;
  };

  //----------------------------------------------------------------
  // TogglePlaylist
  //
  struct TogglePlaylist : public ClickableItem
  {
    TogglePlaylist(TimelineView & view):
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

    TimelineView & view_;
    PostponeEvent postpone_;
    boost::shared_ptr<CancelableEvent::Ticket> singleClickEventTicket_;
  };


  //----------------------------------------------------------------
  // Animator
  //
  struct Animator : public ItemView::IAnimator
  {
    Animator(const PlaylistView & playlist,
             TimelineView & timeline,
             Item & controlsContainer):
      playlist_(playlist),
      timeline_(timeline),
      controlsContainer_(controlsContainer)
    {}

    bool needToPause() const
    {
      const ItemFocus::Target * focus = ItemFocus::singleton().focus();
      const TVec2D & pt = timeline_.mousePt();
      Item & root = *(timeline_.root());

      bool alwaysShowTimeline = (timeline_.mainWindow_ &&
                                 timeline_.mainWindow_->isTimelineVisible());

      bool shouldPause = (alwaysShowTimeline ||
                          playlist_.isEnabled() ||
                          controlsContainer_.overlaps(pt) ||
                          (focus && focus->view_ == &timeline_));
      return shouldPause;
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      Item & root = *(timeline_.root());
      TransitionItem & opacity = root.get<TransitionItem>("opacity");

      if (needToPause() && opacity.transition().is_steady())
      {
        opacity.pause(ItemRef::constant(opacity.transition().get_value()));
        timeline_.delAnimator(animatorPtr);
      }
      else if (opacity.transition().is_done())
      {
        timeline_.delAnimator(animatorPtr);
      }

      opacity.uncache();
    }

    const PlaylistView & playlist_;
    TimelineView & timeline_;
    Item & controlsContainer_;
  };


  //----------------------------------------------------------------
  // AnimatorForControls
  //
  struct AnimatorForControls : public ItemView::IAnimator
  {
    AnimatorForControls(const PlaylistView & playlist,
                        TimelineView & timeline,
                        Item & controlsContainer):
      playlist_(playlist),
      timeline_(timeline),
      controlsContainer_(controlsContainer)
    {}

    bool needToPause() const
    {
#if 0
      bool playbackPaused = (timeline_.mainWindow_ == NULL ||
                             timeline_.mainWindow_->isPlaybackPaused());
#endif
      const TVec2D & pt = timeline_.mousePt();
      bool shouldPause = (// controlsContainer_.overlaps(pt) ||
                          playlist_.isEnabled());

      return shouldPause;
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      Item & root = *(timeline_.root());
      TransitionItem & opacity =
        root.get<TransitionItem>("opacity_for_controls");

      if (needToPause() && opacity.transition().is_steady())
      {
        opacity.pause(ItemRef::constant(opacity.transition().get_value()));
        timeline_.delAnimator(animatorPtr);
      }
      else if (opacity.transition().is_done())
      {
        timeline_.delAnimator(animatorPtr);
      }

      opacity.uncache();
    }

    const PlaylistView & playlist_;
    TimelineView & timeline_;
    Item & controlsContainer_;
  };


  //----------------------------------------------------------------
  // AnimateOpacity
  //
  struct AnimateOpacity : public Item::Observer
  {
    AnimateOpacity(TimelineView & timeline):
      timeline_(timeline)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      timeline_.maybeAnimateOpacity();
      timeline_.forceAnimateControls();
    }

    TimelineView & timeline_;
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
  TimelineView::setup(MainWindow * mainWindow, PlaylistView * playlist)
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

    // setup opacity caching item:
    typedef Transition::Polyline TPolyline;
    TransitionItem & opacity = root.
      addHidden(new TransitionItem("opacity",
                                   TPolyline(0.25, 0.0, 1.0, 10),
                                   TPolyline(1.75, 1.0, 1.0),
                                   TPolyline(1.0, 1.0, 0.0, 10)));

    ExpressionItem & titleHeight = root.
      addHidden(new ExpressionItem("style_title_height",
                                   new StyleTitleHeight(*playlist)));

    Gradient & shadow = root.addNew<Gradient>("shadow");
    shadow.anchors_.fill(root);
    shadow.anchors_.top_.reset();
    shadow.anchors_.right_.reset();
    shadow.width_ = shadow.addExpr(new TimelineShadowWidth(*playlist));
    shadow.height_ = ItemRef::scale(titleHeight, kPropertyExpression, 4.5);
    shadow.color_ = shadow.addExpr(new StyleTimelineShadow(*playlist));
    shadow.opacity_ = ItemRef::uncacheable(opacity, kPropertyTransition);

    Item & container = root.addNew<Item>("container");
    container.anchors_.fill(root);
    container.anchors_.top_.reset();
    container.height_ = ItemRef::scale(titleHeight, kPropertyExpression, 1.5);

    Item & mouseDetect = root.addNew<Item>("mouse_detect");
    mouseDetect.anchors_.fill(container);
    mouseDetect.anchors_.top_.reset();
    mouseDetect.height_ = ItemRef::scale(container, kPropertyHeight, 2.0);

    // setup mouse trap to prevent unintended click-through to playlist:
    MouseTrap & mouseTrap = root.addNew<MouseTrap>("mouse_trap");
    mouseTrap.onScroll_ = false;
    mouseTrap.anchors_.fill(container);
    mouseTrap.anchors_.right_ = ItemRef::reference(shadow, kPropertyRight);

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    timeline.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    timeline.anchors_.vcenter_ = ItemRef::reference(container, kPropertyTop);
    timeline.margins_.left_ =
      ItemRef::scale(titleHeight, kPropertyExpression, 0.5);
    timeline.margins_.right_ = timeline.margins_.left_;
    timeline.height_ = timeline.addExpr(new OddRoundUp(container,
                                                       kPropertyHeight,
                                                       0.22222222, 1));

    TimelineSeek & seek = timeline.add(new TimelineSeek(*this));
    seek.anchors_.fill(timeline);

    ColorRef colorCursor = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::cursor_));

    ColorRef colorExcluded = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::timeline_excluded_));

    ColorRef colorOutPt = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::timeline_included_, 1, 1));

    ColorRef colorOutPtBg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::timeline_included_, 0));

    ColorRef colorIncluded = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::timeline_included_));

    ColorRef colorPlayed = timeline.addExpr
      (style_color_ref(*playlist, &PlaylistViewStyle::timeline_played_));

    ColorRef colorPlayedBg = timeline.addExpr
      (style_color_ref(*playlist, &PlaylistViewStyle::timeline_played_, 0));

    ColorRef colorTextBg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::bg_timecode_));

    ColorRef colorTextFg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::fg_timecode_));

    ColorRef colorFocusBg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::bg_focus_));

    ColorRef colorFocusFg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::fg_focus_));

    ColorRef colorHighlightBg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::bg_edit_selected_));

    ColorRef colorHighlightFg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::fg_edit_selected_));

    ColorRef colorFullscreenToggleBg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::fg_timecode_, 0.64));

    ColorRef colorFullscreenToggleFg = timeline.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::fg_timecode_));

    Rectangle & timelineIn = timeline.addNew<Rectangle>("timelineIn");
    timelineIn.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    timelineIn.anchors_.right_ =
      timelineIn.addExpr(new TimelineIn(*this, timeline));
    timelineIn.anchors_.vcenter_ =
      ItemRef::reference(timeline, kPropertyVCenter);
    timelineIn.height_ =
      timelineIn.addExpr(new TimelineHeight(*this, mouseDetect, timeline));
    timelineIn.color_ = colorExcluded;
    timelineIn.opacity_ = shadow.opacity_;

    Rectangle & timelinePlayhead =
      timeline.addNew<Rectangle>("timelinePlayhead");
    timelinePlayhead.anchors_.left_ =
      ItemRef::reference(timelineIn, kPropertyRight);
    timelinePlayhead.anchors_.right_ =
      timelinePlayhead.addExpr(new TimelinePlayhead(*this, timeline));
    timelinePlayhead.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelinePlayhead.height_ = timelineIn.height_;
    timelinePlayhead.color_ = colorPlayed;
    timelinePlayhead.opacity_ = shadow.opacity_;

    Rectangle & timelineOut =
      timeline.addNew<Rectangle>("timelineOut");
    timelineOut.anchors_.left_ =
      ItemRef::reference(timelinePlayhead, kPropertyRight);
    timelineOut.anchors_.right_ =
      timelineOut.addExpr(new TimelineOut(*this, timeline));
    timelineOut.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelineOut.height_ = timelineIn.height_;
    timelineOut.color_ = colorIncluded;
    timelineOut.opacity_ = shadow.opacity_;

    Rectangle & timelineEnd =
      timeline.addNew<Rectangle>("timelineEnd");
    timelineEnd.anchors_.left_ =
      ItemRef::reference(timelineOut, kPropertyRight);
    timelineEnd.anchors_.right_ = ItemRef::reference(timeline, kPropertyRight);
    timelineEnd.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelineEnd.height_ = timelineIn.height_;
    timelineEnd.color_ = colorExcluded;
    timelineEnd.opacity_ = shadow.opacity_;

    RoundRect & inPoint = root.addNew<RoundRect>("inPoint");
    inPoint.anchors_.hcenter_ =
      ItemRef::reference(timelineIn, kPropertyRight);
    inPoint.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    inPoint.width_ = ItemRef::scale(timeline, kPropertyHeight, 0.67);
    inPoint.height_ = inPoint.width_;
    inPoint.radius_ = ItemRef::scale(inPoint, kPropertyHeight, 0.5);
    inPoint.color_ = colorPlayed;
    inPoint.background_ = colorPlayedBg;
    inPoint.visible_ = inPoint.addExpr(new MarkerVisible(*this, mouseDetect));
    inPoint.opacity_ = shadow.opacity_;

    RoundRect & playhead = root.addNew<RoundRect>("playhead");
    playhead.anchors_.hcenter_ =
      ItemRef::reference(timelinePlayhead, kPropertyRight);
    playhead.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    playhead.width_ = ItemRef::offset(timeline, kPropertyHeight, -1.0);
    playhead.height_ = playhead.width_;
    playhead.radius_ = ItemRef::scale(playhead, kPropertyHeight, 0.5);
    playhead.color_ = colorPlayed;
    playhead.background_ = colorPlayedBg;
    playhead.visible_ = inPoint.visible_;
    playhead.opacity_ = shadow.opacity_;

    RoundRect & outPoint = root.addNew<RoundRect>("outPoint");
    outPoint.anchors_.hcenter_ =
      ItemRef::reference(timelineOut, kPropertyRight);
    outPoint.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    outPoint.width_ = inPoint.width_;
    outPoint.height_ = outPoint.width_;
    outPoint.radius_ = ItemRef::scale(outPoint, kPropertyHeight, 0.5);
    outPoint.color_ = colorOutPt;
    outPoint.background_ = colorOutPtBg;
    outPoint.visible_ = inPoint.visible_;
    outPoint.opacity_ = shadow.opacity_;

    SliderInPoint & sliderInPoint =
      root.add(new SliderInPoint(*this, timeline));
    sliderInPoint.anchors_.offset(inPoint, -1, 0, -1, 0);

    SliderPlayhead & sliderPlayhead =
      root.add(new SliderPlayhead(*this, timeline));
    sliderPlayhead.anchors_.offset(playhead, -1, 0, -1, 0);

    SliderOutPoint & sliderOutPoint =
      root.add(new SliderOutPoint(*this, timeline));
    sliderOutPoint.anchors_.offset(outPoint, -1, 0, -1, 0);

    const PlaylistViewStyle & style = playlist->playlistViewStyle();
    QFont timecodeFont = style.font_fixed_;

    Rectangle & playheadAuxBg = container.addNew<Rectangle>("playheadAuxBg");
    Text & playheadAux = container.addNew<Text>("playheadAux");
    TextInput & playheadEdit = root.addNew<TextInput>("playheadEdit");
    playheadAuxBg.opacity_ = shadow.opacity_;
    playheadAux.opacity_ = shadow.opacity_;
    playheadEdit.opacity_ = shadow.opacity_;

    Rectangle & durationAuxBg = container.addNew<Rectangle>("durationAuxBg");
    Text & durationAux = container.addNew<Text>("durationAux");
    durationAuxBg.opacity_ = shadow.opacity_;
    durationAux.opacity_ = shadow.opacity_;

    TextInputProxy & playheadFocus =
      root.add(new TextInputProxy("playheadFocus", playheadAux, playheadEdit));
    ItemFocus::singleton().setFocusable(*this, playheadFocus, 2);
    playheadFocus.copyViewToEdit_ = true;
    playheadFocus.bgNoFocus_ = colorTextBg;
    playheadFocus.bgOnFocus_ = colorFocusBg;
    playheadAux.anchors_.left_ =
      ItemRef::offset(timeline, kPropertyLeft, 3);
    playheadAux.margins_.left_ =
      ItemRef::reference(container, kPropertyHeight);
    playheadAux.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    playheadAux.visible_ =
      playheadAux.addExpr(new ShowWhenFocused(playheadFocus, false));
    playheadAux.color_ = colorTextFg;
    playheadAux.text_ = playheadAux.addExpr(new GetPlayheadAux(*this));
    playheadAux.font_ = timecodeFont;
    playheadAux.fontSize_ =
      ItemRef::scale(container, kPropertyHeight, 0.33333333 * kDpiScale);

    playheadAuxBg.anchors_.offset(playheadAux, -3, 3, -3, 3);
    playheadAuxBg.color_ = playheadAuxBg.
      addExpr(new ColorWhenFocused(playheadFocus));

    durationAux.anchors_.right_ =
      ItemRef::offset(timeline, kPropertyRight, -3);
    durationAux.margins_.right_ =
      ItemRef::reference(container, kPropertyHeight);
    durationAux.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    durationAux.color_ = colorTextFg;
    durationAux.text_ = durationAux.addExpr(new GetDurationAux(*this));
    durationAux.font_ = playheadAux.font_;
    durationAux.fontSize_ = playheadAux.fontSize_;

    durationAuxBg.anchors_.offset(durationAux, -3, 3, -3, 3);
    durationAuxBg.color_ = colorTextBg;

    playheadEdit.anchors_.fill(playheadAux);
    playheadEdit.margins_.left_ =
      ItemRef::scale(playheadEdit, kPropertyCursorWidth, -1.0);
    playheadEdit.visible_ =
      playheadEdit.addExpr(new ShowWhenFocused(playheadFocus, true));
    playheadEdit.color_ = colorFocusFg;
    playheadEdit.cursorColor_ = colorCursor;
    playheadEdit.font_ = playheadAux.font_;
    playheadEdit.fontSize_ = playheadAux.fontSize_;
    playheadEdit.selectionBg_ = colorHighlightBg;
    playheadEdit.selectionFg_ = colorHighlightFg;

    playheadFocus.anchors_.fill(playheadAuxBg);

    TogglePlayback & playbackToggle =
      root.add(new TogglePlayback(*this));
    Item & playbackBtn = container.addNew<Item>("playback_btn");
    {
      playbackBtn.anchors_.vcenter_ =
        ItemRef::reference(container, kPropertyVCenter);

      playbackBtn.anchors_.left_ =
        ItemRef::reference(root, kPropertyLeft);

      playbackBtn.anchors_.right_ =
        ItemRef::offset(playheadAuxBg, kPropertyLeft);

      playbackBtn.height_ =
        ItemRef::reference(playheadAuxBg, kPropertyHeight);

      playbackBtn.margins_.left_ = timeline.margins_.left_;
      playbackBtn.margins_.right_ = timeline.margins_.left_;

      Item & square = playbackBtn.addNew<Item>("square");
      square.anchors_.vcenter_ = ItemRef::reference(playbackBtn,
                                                    kPropertyVCenter);
      square.anchors_.hcenter_ = ItemRef::reference(playbackBtn,
                                                    kPropertyHCenter);
      square.width_ = ItemRef::reference(playheadAuxBg, kPropertyHeight);
      square.height_ = square.width_;

      TexturedRect & play = square.add(new TexturedRect("play"));
      play.anchors_.fill(square);
      play.visible_ = play.addExpr(new OnPlaybackPaused(*this, true));
      play.texture_ = play.addExpr(new StylePlayTexture(*playlist));
      play.opacity_ = shadow.opacity_;

      TexturedRect & pause = square.add(new TexturedRect("pause"));
      pause.anchors_.fill(square);
      pause.margins_.set(ItemRef::scale(square, kPropertyHeight, 0.05));
      pause.visible_ = pause.addExpr(new OnPlaybackPaused(*this, false));
      pause.texture_ = pause.addExpr(new StylePauseTexture(*playlist));
      pause.opacity_ = shadow.opacity_;

      playbackToggle.anchors_.fill(playbackBtn);
    }

    ToggleFullscreen & fullscreenToggle =
      root.add(new ToggleFullscreen(*this));
    Item & fullscreenBtn = container.addNew<Item>("fullscreen_btn");
    {
      fullscreenBtn.anchors_.vcenter_ =
        ItemRef::reference(container, kPropertyVCenter);

      fullscreenBtn.anchors_.left_ =
        ItemRef::reference(durationAuxBg, kPropertyRight);

      fullscreenBtn.anchors_.right_ =
        ItemRef::reference(root, kPropertyRight);

      fullscreenBtn.height_ = playbackBtn.height_;

      fullscreenBtn.margins_.left_ = timeline.margins_.left_;
      fullscreenBtn.margins_.right_ = timeline.margins_.left_;

      Item & square = fullscreenBtn.addNew<Item>("square");
      square.anchors_.vcenter_ = ItemRef::reference(fullscreenBtn,
                                                    kPropertyVCenter);
      square.anchors_.hcenter_ = ItemRef::reference(fullscreenBtn,
                                                    kPropertyHCenter);
      square.width_ = ItemRef::reference(playheadAuxBg, kPropertyHeight);
      square.height_ = square.width_;

      // fullscreen:
      Rectangle & bl_small = square.add(new Rectangle("bl_small"));
      bl_small.anchors_.bottomLeft(square);
      bl_small.width_ = ItemRef::scale(square, kPropertyWidth, 0.6);
      bl_small.height_ = ItemRef::scale(square, kPropertyHeight, 0.5);
      bl_small.visible_ = bl_small.addExpr(new OnFullscreen(*this, false));
      bl_small.opacity_ = shadow.opacity_;
      bl_small.color_ = colorFullscreenToggleBg;

      Rectangle & tr_large = square.add(new Rectangle("tr_large"));
      tr_large.anchors_.topRight(square);
      tr_large.width_ = ItemRef::scale(square, kPropertyWidth, 0.8);
      tr_large.height_ = ItemRef::scale(square, kPropertyHeight, 0.7);
      tr_large.visible_ = bl_small.visible_;
      tr_large.opacity_ = shadow.opacity_;
      tr_large.color_ = colorFullscreenToggleFg;

      // windowed:
      Rectangle & bl_large = square.add(new Rectangle("bl_large"));
      bl_large.anchors_.bottomLeft(square);
      bl_large.width_ = tr_large.width_;
      bl_large.height_ = tr_large.height_;
      bl_large.visible_ = bl_large.addExpr(new OnFullscreen(*this, true));
      bl_large.opacity_ = shadow.opacity_;
      bl_large.color_ = bl_small.color_;

      Rectangle & tr_small = square.add(new Rectangle("tr_small"));
      tr_small.anchors_.topRight(square);
      tr_small.width_ = bl_small.width_;
      tr_small.height_ = bl_small.height_;
      tr_small.visible_ = bl_large.visible_;
      tr_small.opacity_ = shadow.opacity_;
      tr_small.color_ = tr_large.color_;

      fullscreenToggle.anchors_.fill(fullscreenBtn);
    }

    // add other player controls:
    ColorRef colorControlsBg = root.addExpr
      (style_color_ref(*playlist, &ItemViewStyle::bg_controls_));

    Item & playlistButton = root.addNew<Item>("playlistButton");
    TogglePlaylist & playlistToggle = root.add(new TogglePlaylist(*this));
    {
      playlistButton.anchors_.top_ =
        ItemRef::offset(root, kPropertyTop, 2);
      playlistButton.anchors_.left_ =
        ItemRef::reference(root, kPropertyLeft);
      playlistButton.width_ =
        ItemRef::scale(titleHeight, kPropertyExpression, 1.5);
      playlistButton.height_ = playlistButton.width_;

      RoundRect & bg = playlistButton.addNew<RoundRect>("bg");
      bg.anchors_.fill(playlistButton);
      bg.margins_.set(ItemRef::reference(playlistButton, kPropertyHeight,
                                         0.1, -1.0));
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBg;
      bg.opacity_ = shadow.opacity_;
      bg.visible_ = bg.addExpr(new OnPlaylistVisible(*this, false));

      TexturedRect & gridOn = playlistButton.add(new TexturedRect("gridOn"));
      gridOn.anchors_.fill(playlistButton);
      gridOn.margins_.set(ItemRef::scale(playlistButton, kPropertyHeight,
                                         0.2));
      gridOn.visible_ = gridOn.addExpr(new OnPlaylistVisible(*this, true));
      gridOn.texture_ = gridOn.addExpr(new StyleGridOnTexture(*playlist));
      gridOn.opacity_ = shadow.opacity_;

      TexturedRect & gridOff = playlistButton.add(new TexturedRect("gridOff"));
      gridOff.anchors_.fill(playlistButton);
      gridOff.margins_.set(ItemRef::scale(playlistButton, kPropertyHeight,
                                          0.2));
      gridOff.visible_ = gridOff.addExpr(new OnPlaylistVisible(*this, false));
      gridOff.texture_ = gridOff.addExpr(new StyleGridOffTexture(*playlist));
      gridOff.opacity_ = shadow.opacity_;

      playlistToggle.anchors_.fill(playlistButton);
    }

    TransitionItem & opacityForControls = root.
      addHidden(new TransitionItem("opacity_for_controls",
                                   TPolyline(0.25, 0.0, 1.0, 10),
                                   TPolyline(1.75, 1.0, 1.0),
                                   TPolyline(1.0, 1.0, 0.0, 10)));

    RoundRect & controls = root.addNew<RoundRect>("controls");
    Item & mouseDetectForControls =
      root.addNew<Item>("mouse_detect_for_controls");
    MouseTrap & mouseTrapForControls =
      controls.addNew<MouseTrap>("mouse_trap_for_controls");
    controls.anchors_.vcenter_ =
      ItemRef::reference(root, kPropertyVCenter);
    controls.anchors_.hcenter_ =
      ItemRef::reference(root, kPropertyHCenter);
    controls.height_ =
      ItemRef::scale(titleHeight, kPropertyExpression, 3.0);
    controls.visible_ = controls.
      addExpr(new ExposeControls(*this));
    controls.visible_.cachingEnabled_ = false;
    mouseTrapForControls.onScroll_ = false;
    mouseTrapForControls.anchors_.fill(controls);

    double cells = 2.0;
    controls.width_ = ItemRef::scale(controls, kPropertyHeight, cells);
    controls.radius_ =
      ItemRef::reference(controls, kPropertyHeight, 0.05, 0.5);
    controls.opacity_ =
      ItemRef::uncacheable(opacityForControls, kPropertyTransition);
    controls.color_ = colorControlsBg;


    TogglePlayback & bigPlaybackToggle =
      controls.add(new TogglePlayback(*this));
    Item & bigPlaybackButton = controls.addNew<Item>("bigPlaybackButton");
    {
      bigPlaybackButton.anchors_.vcenter_ =
        ItemRef::reference(controls, kPropertyVCenter);

      bigPlaybackButton.anchors_.left_ =
        ItemRef::reference(controls, kPropertyLeft);

      bigPlaybackButton.margins_.left_ =
        ItemRef::scale(controls, kPropertyWidth, 0.5 / cells);

      bigPlaybackButton.width_ = ItemRef::reference(controls, kPropertyHeight);
      bigPlaybackButton.height_ = bigPlaybackButton.width_;

      TexturedRect & play = bigPlaybackButton.add(new TexturedRect("play"));
      play.anchors_.fill(bigPlaybackButton);
      play.margins_.set(ItemRef::scale(bigPlaybackButton,
                                       kPropertyHeight,
                                       0.15));
      play.visible_ = play.addExpr(new OnPlaybackPaused(*this, true));
      play.texture_ = play.addExpr(new StylePlayTexture(*playlist));
      play.opacity_ = controls.opacity_;

      TexturedRect & pause = bigPlaybackButton.add(new TexturedRect("pause"));
      pause.anchors_.fill(bigPlaybackButton);
      pause.margins_.set(ItemRef::scale(bigPlaybackButton,
                                        kPropertyHeight,
                                        0.2));
      pause.visible_ = pause.addExpr(new OnPlaybackPaused(*this, false));
      pause.texture_ = pause.addExpr(new StylePauseTexture(*playlist));
      pause.opacity_ = controls.opacity_;

#if 1
      // while there is only one button in the controls container
      // use the entire container area for mouse clicks:
      bigPlaybackToggle.anchors_.fill(controls);
#else
      bigPlaybackToggle.anchors_.fill(bigPlaybackButton);
#endif

      mouseDetectForControls.anchors_.fill(controls);
    }

    animator_.reset(new Animator(*playlist, *this, mouseDetect));
    maybeAnimateOpacity();

    animatorForControls_.reset
      (new AnimatorForControls(*playlist, *this, mouseDetectForControls));
    forceAnimateControls();

    // re-apply style when playlist is enabled or disabled:
    Item::TObserverPtr repaintTimeline(new Repaint(*this, true));
    playlist_->root()->addObserver(Item::kOnToggleItemView, repaintTimeline);

    Item::TObserverPtr animateOpacity(new AnimateOpacity(*this));
    playlist_->root()->addObserver(Item::kOnToggleItemView, animateOpacity);
    playheadFocus.addObserver(Item::kOnFocus, animateOpacity);
    playheadFocus.addObserver(Item::kOnFocusOut, animateOpacity);
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

    Item & root = *root_;
    Item & timeline = root["timeline"];

    Item & timelineIn = timeline["timelineIn"];
    requestUncache(&timelineIn);

    Item & timelinePlayhead = timeline["timelinePlayhead"];
    requestUncache(&timelinePlayhead);

    Item & timelineOut = timeline["timelineOut"];
    requestUncache(&timelineOut);

    Item & timelineEnd = timeline["timelineEnd"];
    requestUncache(&timelineEnd);

    Item & inPoint = root["inPoint"];
    requestUncache(&inPoint);

    Item & playhead = root["playhead"];
    requestUncache(&playhead);

    Item & outPoint = root["outPoint"];
    requestUncache(&outPoint);

    // update the opacity transitions:
    maybeAnimateOpacity();
    maybeAnimateControls();

    return true;
  }

  //----------------------------------------------------------------
  // TimelineView::setModel
  //
  void
  TimelineView::setModel(TimelineModel * model)
  {
    if (model == model_)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!model_);

    model_ = model;

    Item & root = *root_;
    TextInput & playheadEdit = root.get<TextInput>("playheadEdit");

    // connect new model:
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

    ok = connect(&playheadEdit, SIGNAL(editingFinished(const QString &)),
                 model_, SLOT(seekTo(const QString &)));
    YAE_ASSERT(ok);
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

  //----------------------------------------------------------------
  // TimelineView::maybeAnimateOpacity
  //
  void
  TimelineView::maybeAnimateOpacity()
  {
    Item & root = *root_;
    TransitionItem & opacity = root.get<TransitionItem>("opacity");
    Animator & animator = dynamic_cast<Animator &>(*(animator_.get()));

    if (animator.needToPause() && opacity.transition().is_steady())
    {
      opacity.pause(ItemRef::constant(opacity.transition().get_value()));
      delAnimator(animator_);
    }
    else
    {
      opacity.start();
      addAnimator(animator_);
    }

    opacity.uncache();
  }

  //----------------------------------------------------------------
  // TimelineView::maybeAnimateControls
  //
  void
  TimelineView::maybeAnimateControls()
  {
    Item & root = *root_;
    TransitionItem & opacity =
      root.get<TransitionItem>("opacity_for_controls");
    AnimatorForControls & animator =
      dynamic_cast<AnimatorForControls &>(*(animatorForControls_.get()));

    Item & mouseDetectForControls = root["mouse_detect_for_controls"];
    const TVec2D & pt = mousePt();

    bool playbackPaused = (mainWindow_ == NULL ||
                           mainWindow_->isPlaybackPaused());

    bool needToPause = animator.needToPause();
    if (needToPause && opacity.transition().is_steady())
    {
      opacity.pause(ItemRef::constant(opacity.transition().get_value()));
      delAnimator(animatorForControls_);
    }
    else if ((!needToPause && opacity.is_paused()) ||
             mouseDetectForControls.overlaps(pt) ||
             playbackPaused)
    {
      opacity.start();
      addAnimator(animatorForControls_);
    }

    opacity.uncache();
  }

  //----------------------------------------------------------------
  // TimelineView::forceAnimateControls
  //
  void
  TimelineView::forceAnimateControls()
  {
    Item & root = *root_;
    TransitionItem & opacity =
      root.get<TransitionItem>("opacity_for_controls");
    opacity.pause(ItemRef::constant(opacity.transition().get_spinup_value()));
    maybeAnimateControls();
  }

}
