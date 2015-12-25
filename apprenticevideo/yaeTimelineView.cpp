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
  // OddRoundUp
  //
  struct OddRoundUp : public TDoubleExpr
  {
    OddRoundUp(const Item & item,
               Property property,
               double scale = 1.0,
               double translate = 0.0):
      item_(item),
      property_(property),
      scale_(scale),
      translate_(translate)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double v = 0.0;
      item_.get(property_, v);
      v *= scale_;
      v += translate_;

      int i = 1 | int(ceil(v));
      result = double(i);
    }

    const Item & item_;
    Property property_;
    double scale_;
    double translate_;
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
  // ShowPlayheadAux
  //
  struct ShowPlayheadAux : public TBoolExpr
  {
    ShowPlayheadAux(Item & focusProxy):
      focusProxy_(focusProxy)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = !ItemFocus::singleton().hasFocus(focusProxy_.id_);
    }

    Item & focusProxy_;
  };

  //----------------------------------------------------------------
  // ShowPlayheadEdit
  //
  struct ShowPlayheadEdit : public TBoolExpr
  {
    ShowPlayheadEdit(Item & focusProxy):
      focusProxy_(focusProxy)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = ItemFocus::singleton().hasFocus(focusProxy_.id_);
    }

    Item & focusProxy_;
  };

  //----------------------------------------------------------------
  // PlayheadAuxBg
  //
  struct PlayheadAuxBg : public TColorExpr
  {
    PlayheadAuxBg(Item & focusProxy,
                  const ColorRef & noFocus,
                  const ColorRef & focused):
      focusProxy_(focusProxy),
      noFocus_(noFocus),
      focused_(focused)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool hasFocus = ItemFocus::singleton().hasFocus(focusProxy_.id_);
      result = hasFocus ? focused_.get() : noFocus_.get();
    }

    Item & focusProxy_;
    ColorRef noFocus_;
    ColorRef focused_;
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
    model_(NULL)
  {
    Item & root = *root_;

    // setup an invisible item so its height property expression
    // could be computed once and the result reused in other places
    // that need to compute the same property expression:
    Item & title = root.addNewHidden<Item>("title_height");
    title.height_ = title.addExpr(new CalcTitleHeight(root, 24.0));

    Gradient & shadow = root.addNew<Gradient>("shadow");
    shadow.anchors_.fill(root);
    shadow.anchors_.top_.reset();
    shadow.height_ = ItemRef::scale(title, kPropertyHeight, 4.5);

    shadow.color_[0.000000] = Color(0x000000, 0.004);
    shadow.color_[0.135417] = Color(0x000000, 0.016);
    shadow.color_[0.208333] = Color(0x000000, 0.031);
    shadow.color_[0.260417] = Color(0x000000, 0.047);
    shadow.color_[0.354167] = Color(0x000000, 0.090);
    shadow.color_[0.447917] = Color(0x000000, 0.149);
    shadow.color_[0.500000] = Color(0x000000, 0.192);
    shadow.color_[1.000000] = Color(0x000000, 0.690);

    Item & container = root.addNew<Item>("container");
    container.anchors_.fill(root);
    container.anchors_.top_.reset();
    container.height_ = ItemRef::scale(title, kPropertyHeight, 1.5);

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    timeline.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    timeline.anchors_.vcenter_ = ItemRef::reference(container, kPropertyTop);
    timeline.margins_.left_ = ItemRef::scale(title, kPropertyHeight, 0.5);
    timeline.margins_.right_ = timeline.margins_.left_;
    timeline.height_ = timeline.addExpr(new OddRoundUp(title,
                                                       kPropertyHeight,
                                                       0.33333333), 1, 1);

    TimelineSeek & seek = timeline.add(new TimelineSeek(*this));
    seek.anchors_.fill(timeline);

    ColorRef colorExcluded = ColorRef::constant(Color(0xFFFFFF, 0.2));
    ColorRef colorIncluded = ColorRef::constant(Color(0xFFFFFF, 0.5));
    ColorRef colorPlayed = ColorRef::constant(Color(0xf12b24, 1.0));
    ColorRef colorOutPt = ColorRef::constant(Color(0xe6e6e6, 1.0));
    ColorRef colorPlayedBg = ColorRef::constant(Color(0xf12b24, 0.0));
    ColorRef colorOutPtBg = ColorRef::constant(Color(0xe6e6e6, 0.0));
    ColorRef colorTextBg = ColorRef::constant(Color(0x7f7f7f, 0.25));
    ColorRef colorTextFg = ColorRef::constant(Color(0xFFFFFF, 0.5));
    ColorRef colorFocusBg = ColorRef::constant(Color(0x7f7f7f, 0.5));
    ColorRef colorFocusFg = ColorRef::constant(Color(0xFFFFFF, 1.0));
    ColorRef colorHighlightBg = ColorRef::constant(Color(0xFFFFFF, 1.0));
    ColorRef colorHighlightFg = ColorRef::constant(Color(0x000000, 1.0));

    Rectangle & timelineIn = timeline.addNew<Rectangle>("timelineIn");
    timelineIn.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    timelineIn.anchors_.right_ =
      timelineIn.addExpr(new TimelineIn(*this, timeline));
    timelineIn.anchors_.vcenter_ =
      ItemRef::reference(timeline, kPropertyVCenter);
    timelineIn.height_ =
      timelineIn.addExpr(new TimelineHeight(*this, container, timeline));
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
    inPoint.visible_ = inPoint.addExpr(new MarkerVisible(*this, container));

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

    QFont timecodeFont("");
#if 1
    timecodeFont.setFamily("Menlo, "
                           "Monaco, "
                           "Droid Sans Mono, "
                           "DejaVu Sans Mono, "
                           "Bitstream Vera Sans Mono, "
                           "Consolas, "
                           "Lucida Sans Typewriter, "
                           "Lucida Console, "
                           "Courier New");
#endif

    timecodeFont.setStyleHint(QFont::Monospace);
    timecodeFont.setFixedPitch(true);
    timecodeFont.setStyleStrategy(QFont::OpenGLCompatible);

    Rectangle & playheadAuxBg = container.addNew<Rectangle>("playheadAuxBg");
    Text & playheadAux = container.addNew<Text>("playheadAux");
    TextInput & playheadEdit = root.addNew<TextInput>("playheadEdit");

    Rectangle & durationAuxBg = container.addNew<Rectangle>("durationAuxBg");
    Text & durationAux = container.addNew<Text>("durationAux");

    TextInputProxy & playheadFocus =
      root.add(new TextInputProxy("playheadFocus", playheadAux, playheadEdit));
    ItemFocus::singleton().setFocusable(playheadFocus, 2);

    playheadAux.anchors_.left_ =
      ItemRef::offset(timeline, kPropertyLeft, 3);
    playheadAux.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    playheadAux.visible_ =
      playheadAux.addExpr(new ShowPlayheadAux(playheadFocus));
    playheadAux.color_ = colorTextFg;
    playheadAux.text_ = playheadAux.addExpr(new GetPlayheadAux(*this));
    playheadAux.font_ = timecodeFont;
    playheadAux.fontSize_ =
      ItemRef::scale(container, kPropertyHeight, 0.33333333 * kDpiScale);

    playheadAuxBg.anchors_.offset(playheadAux, -3, 3, -3, 3);
    playheadAuxBg.color_ =
      playheadAuxBg.addExpr(new PlayheadAuxBg(playheadFocus,
                                              colorTextBg,
                                              colorFocusBg));

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
    // add a margin to account for cursor width:
    playheadEdit.margins_.left_ = ItemRef::constant(-1);
    playheadEdit.visible_ =
      playheadEdit.addExpr(new ShowPlayheadEdit(playheadFocus));
    playheadEdit.color_ = colorFocusFg;
    playheadEdit.cursorColor_ = colorPlayed;
    playheadEdit.font_ = playheadAux.font_;
    playheadEdit.fontSize_ = playheadAux.fontSize_;
    playheadEdit.selectionBg_ = colorHighlightBg;
    playheadEdit.selectionFg_ = colorHighlightFg;

    playheadFocus.anchors_.fill(playheadAuxBg);
  }

  //----------------------------------------------------------------
  // TimelineView::processMouseTracking
  //
  bool
  TimelineView::processMouseTracking(const TVec2D & mousePt)
  {
    (void)mousePt;
    return isEnabled();
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
      this->requestRepaint();
    }
  }

}
