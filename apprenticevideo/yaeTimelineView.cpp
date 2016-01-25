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
#include "yaePlaylistView.h"
#include "yaePlaylistViewStyle.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeSegment.h"
#include "yaeText.h"
#include "yaeTextInput.h"
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
  // TimelineView::TimelineView
  //
  TimelineView::TimelineView():
    ItemView("timeline"),
    model_(NULL),
    playlist_(NULL)
  {}

  //----------------------------------------------------------------
  // TimelineView::setPlaylistView
  //
  void
  TimelineView::setPlaylistView(PlaylistView * playlist)
  {
    if (!playlist)
    {
      YAE_ASSERT(false);
      return;
    }

    YAE_ASSERT(!playlist_);
    playlist_ = playlist;

    Item & root = *root_;

    // re-apply style when playlist is enabled or disabled:
    playlist_->root()->
      addObserver(Item::kOnToggleItemView,
                  Item::TObserverPtr(new Repaint(*this, true)));

    Gradient & shadow = root.addNew<Gradient>("shadow");
    shadow.anchors_.fill(root);
    shadow.anchors_.top_.reset();
    shadow.anchors_.right_.reset();
    shadow.width_ = shadow.addExpr(new TimelineShadowWidth(*playlist));
    shadow.height_ = shadow.addExpr(new StyleTitleHeight(*playlist), 4.5);
    shadow.color_ = shadow.addExpr(new StyleTimelineShadow(*playlist));

    Item & container = root.addNew<Item>("container");
    container.anchors_.fill(root);
    container.anchors_.top_.reset();
    container.height_ = container.
      addExpr(new StyleTitleHeight(*playlist), 1.5);

    Item & mouseDetect = root.addNew<Item>("mouse_detect");
    mouseDetect.anchors_.fill(container);
    mouseDetect.anchors_.top_.reset();
    mouseDetect.height_ = ItemRef::scale(container, kPropertyHeight, 2.0);

    // setup mouse trap to prevent unintended click-through to playlist:
    MouseTrap & mouseTrap = root.addNew<MouseTrap>("mouse_trap");
    mouseTrap.anchors_.fill(container);
    mouseTrap.anchors_.right_ = ItemRef::reference(shadow, kPropertyRight);

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    timeline.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    timeline.anchors_.vcenter_ = ItemRef::reference(container, kPropertyTop);
    timeline.margins_.left_ = timeline.
      addExpr(new StyleTitleHeight(*playlist), 0.5);
    timeline.margins_.right_ = timeline.margins_.left_;
    timeline.height_ = timeline.addExpr(new OddRoundUp(container,
                                                       kPropertyHeight,
                                                       0.22222222), 1, 1);

    TimelineSeek & seek = timeline.add(new TimelineSeek(*this));
    seek.anchors_.fill(timeline);

    ColorRef colorCursor = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kCursor));

    ColorRef colorExcluded = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelineExcluded));

    ColorRef colorOutPt = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelineIncluded, 1, 1));

    ColorRef colorOutPtBg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelineIncluded, 0));

    ColorRef colorIncluded = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelineIncluded));

    ColorRef colorPlayed = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelinePlayed));

    ColorRef colorPlayedBg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kTimelinePlayed, 0));

    ColorRef colorTextBg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kBgTimecode));

    ColorRef colorTextFg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kFgTimecode));

    ColorRef colorFocusBg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kBgFocus));

    ColorRef colorFocusFg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kFgFocus));

    ColorRef colorHighlightBg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kBgEditSelected));

    ColorRef colorHighlightFg = timeline.addExpr
      (new StyleColor(*playlist, PlaylistViewStyle::kFgEditSelected));

    Rectangle & timelineIn = timeline.addNew<Rectangle>("timelineIn");
    timelineIn.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    timelineIn.anchors_.right_ =
      timelineIn.addExpr(new TimelineIn(*this, timeline));
    timelineIn.anchors_.vcenter_ =
      ItemRef::reference(timeline, kPropertyVCenter);
    timelineIn.height_ =
      timelineIn.addExpr(new TimelineHeight(*this, mouseDetect, timeline));
    timelineIn.color_ = colorExcluded;

    Rectangle & timelinePlayhead =
      timeline.addNew<Rectangle>("timelinePlayhead");
    timelinePlayhead.anchors_.left_ =
      ItemRef::reference(timelineIn, kPropertyRight);
    timelinePlayhead.anchors_.right_ =
      timelinePlayhead.addExpr(new TimelinePlayhead(*this, timeline));
    timelinePlayhead.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelinePlayhead.height_ = timelineIn.height_;
    timelinePlayhead.color_ = colorPlayed;

    Rectangle & timelineOut =
      timeline.addNew<Rectangle>("timelineOut");
    timelineOut.anchors_.left_ =
      ItemRef::reference(timelinePlayhead, kPropertyRight);
    timelineOut.anchors_.right_ =
      timelineOut.addExpr(new TimelineOut(*this, timeline));
    timelineOut.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelineOut.height_ = timelineIn.height_;
    timelineOut.color_ = colorIncluded;

    Rectangle & timelineEnd =
      timeline.addNew<Rectangle>("timelineEnd");
    timelineEnd.anchors_.left_ =
      ItemRef::reference(timelineOut, kPropertyRight);
    timelineEnd.anchors_.right_ = ItemRef::reference(timeline, kPropertyRight);
    timelineEnd.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelineEnd.height_ = timelineIn.height_;
    timelineEnd.color_ = colorExcluded;

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

    Rectangle & durationAuxBg = container.addNew<Rectangle>("durationAuxBg");
    Text & durationAux = container.addNew<Text>("durationAux");

    TextInputProxy & playheadFocus =
      root.add(new TextInputProxy("playheadFocus", playheadAux, playheadEdit));
    ItemFocus::singleton().setFocusable(*this, playheadFocus, 2);
    playheadFocus.copyViewToEdit_ = true;
    playheadFocus.bgNoFocus_ = colorTextBg;
    playheadFocus.bgOnFocus_ = colorFocusBg;

    playheadAux.anchors_.left_ =
      ItemRef::offset(timeline, kPropertyLeft, 3);
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
    (void)mousePt;

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

}
