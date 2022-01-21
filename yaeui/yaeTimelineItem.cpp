// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 17 19:34:43 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeArrowItem.h"
#include "yaeItemFocus.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"
#include "yaeTextInput.h"
#include "yaeTexturedRect.h"
#include "yaeTimelineItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // TimelineHeight
  //
  struct YAEUI_API TimelineHeight : public TDoubleExpr
  {
    TimelineHeight(ItemView & view, Item & container, Item & timeline):
      view_(view),
      container_(container),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      int h = std::max<int>(2, ~1 & (int(0.5 + timeline_.height()) / 4));

      const std::list<VisibleItem> & items = view_.mouseOverItems();
      if (yae::find(items, container_) != items.end())
      {
        h *= 2;
      }

      result = double(h);
    }

    ItemView & view_;
    Item & container_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelineIn
  //
  struct YAEUI_API TimelineIn : public TDoubleExpr
  {
    TimelineIn(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerTimeIn();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelinePlayhead
  //
  struct YAEUI_API TimelinePlayhead : public TDoubleExpr
  {
    TimelinePlayhead(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerPlayhead();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelineOut
  //
  struct YAEUI_API TimelineOut : public TDoubleExpr
  {
    TimelineOut(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerTimeOut();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // MarkerVisible
  //
  struct YAEUI_API MarkerVisible : public TBoolExpr
  {
    MarkerVisible(ItemView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = timeline_.overlaps(pt);
    }

    ItemView & view_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // GetPlayheadAux
  //
  struct YAEUI_API GetPlayheadAux : public TVarExpr
  {
    GetPlayheadAux(TimelineModel & model):
      model_(model)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = QVariant(model_.auxPlayhead());
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // GetDurationAux
  //
  struct YAEUI_API GetDurationAux : public TVarExpr
  {
    GetDurationAux(TimelineModel & model):
      model_(model)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = QVariant(model_.auxDuration());
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // TimelineSeek
  //
  struct YAEUI_API TimelineSeek : public InputArea
  {
    TimelineSeek(TimelineModel & model):
      InputArea("timeline_seek"),
      model_(model)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      double x0 = this->left();
      double w = this->width();
      double x = rootCSysPoint.x() - itemCSysOrigin.x();
      double t = (x - x0) / w;
      model_.setMarkerPlayhead(t);
      return true;
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // SliderInPoint
  //
  struct YAEUI_API SliderInPoint : public InputArea
  {
    SliderInPoint(TimelineModel & model, Item & timeline):
      InputArea("slider_in_point"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerTimeIn();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerTimeIn(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // SliderPlayhead
  //
  struct YAEUI_API SliderPlayhead : public InputArea
  {
    SliderPlayhead(TimelineModel & model, Item & timeline):
      InputArea("slider_playhead"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerPlayhead();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerPlayhead(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // SliderOutPoint
  //
  struct YAEUI_API SliderOutPoint : public InputArea
  {
    SliderOutPoint(TimelineModel & model, Item & timeline):
      InputArea("slider_out_point"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerTimeOut();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerTimeOut(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // Animator
  //
  struct Animator : public ItemView::IAnimator
  {
    Animator(TimelineItem & timeline, Item & controlsContainer):
      timeline_(timeline),
      controlsContainer_(controlsContainer)
    {}

    // helper:
    bool needToPause() const
    {
      const ItemFocus::Target * focus = ItemFocus::singleton().focus();
      const TVec2D & pt = timeline_.view_.mousePt();

      bool alwaysShowTimeline = timeline_.is_timeline_visible_.get();

      bool shouldPause = (alwaysShowTimeline ||
                          timeline_.is_playlist_visible_.get() ||
                          controlsContainer_.overlaps(pt) ||
                          (focus && focus->view_ == &timeline_.view_));
      return shouldPause;
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      TransitionItem & opacity = *(timeline_.opacity_);

      if (needToPause() && opacity.transition().is_steady())
      {
        opacity.pause(ItemRef::constant(opacity.transition().get_value()));
        timeline_.view_.delAnimator(animatorPtr);
      }
      else if (opacity.transition().is_done())
      {
        timeline_.view_.delAnimator(animatorPtr);
      }

      opacity.uncache();
    }

    TimelineItem & timeline_;
    Item & controlsContainer_;
  };


  //----------------------------------------------------------------
  // AnimatorForControls
  //
  struct AnimatorForControls : public ItemView::IAnimator
  {
    AnimatorForControls(TimelineItem & timeline):
      timeline_(timeline)
    {}

    // helper:
    bool needToPause() const
    {
      bool shouldPause = timeline_.is_playlist_visible_.get();
      return shouldPause;
    }

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      TransitionItem & opacity =
        timeline_.get<TransitionItem>("opacity_for_controls");

      if (needToPause() && opacity.transition().is_steady())
      {
        opacity.pause(ItemRef::constant(opacity.transition().get_value()));
        timeline_.view_.delAnimator(animatorPtr);
      }
      else if (opacity.transition().is_done())
      {
        timeline_.view_.delAnimator(animatorPtr);
      }

      opacity.uncache();
    }

    TimelineItem & timeline_;
  };

  //----------------------------------------------------------------
  // ToolButtonOpacity
  //
  struct ToolButtonOpacity : public TDoubleExpr
  {
    ToolButtonOpacity(ItemView & view,
                      const TransitionItem & opacity,
                      const Item & item,
                      const InputArea & ia):
      view_(view),
      opacity_(opacity),
      item_(item),
      ia_(ia)
    {}

    void evaluate(double & result) const
    {
      opacity_.get(kPropertyTransition, result);

      if (!view_.isMouseOverItem(item_))
      {
        result *= 0.25;
      }
      else if (!view_.isMousePressed(ia_))
      {
        result *= 0.5;
      }
    }

    const ItemView & view_;
    const TransitionItem & opacity_;
    const Item & item_;
    const InputArea & ia_;
  };

  //----------------------------------------------------------------
  // TimelineItem::TimelineItem
  //
  TimelineItem::TimelineItem(const char * name,
                             ItemView & view,
                             TimelineModel & model):
    QObject(),
    Item(name),
    view_(view),
    model_(model)
  {
    is_timeline_visible_ = addExpr(new IsTrue(query_timeline_visible_));
    is_playlist_visible_ = addExpr(new IsTrue(query_playlist_visible_));
  }

  //----------------------------------------------------------------
  // TimelineItem::~TimelineItem
  //
  TimelineItem::~TimelineItem()
  {
    TimelineItem::clear();
    shadow_.reset();
    opacity_animator_.reset();
    controls_animator_.reset();
    animate_opacity_.reset();
  }

  //----------------------------------------------------------------
  // TimelineItem::layout
  //
  void
  TimelineItem::layout()
  {
    // setup opacity caching item:
    typedef Transition::Polyline TPolyline;

    opacity_.reset(new TransitionItem("opacity",
                                      TPolyline(0.25, 0.0, 1.0, 10),
                                      TPolyline(1.75, 1.0, 1.0),
                                      TPolyline(1.0, 1.0, 0.0, 10)));
    TransitionItem & opacity = this->addHidden(opacity_);

    unit_size_.set(new StyleTitleHeight(view_));

    shadow_.reset(new Gradient("shadow"));
    Gradient & shadow = this->add<Gradient>(shadow_);
    shadow.anchors_.fill(*this);
    shadow.anchors_.top_.reset();
    shadow.height_ = ItemRef::scale(unit_size_, 4.5);
    shadow.color_ = shadow.addExpr(new StyleTimelineShadow(view_));
    shadow.opacity_ = ItemRef::uncacheable(opacity, kPropertyTransition);

    Item & container = this->addNew<Item>("container");
    container.anchors_.fill(*this);
    container.anchors_.top_.reset();
    container.height_ = ItemRef::scale(unit_size_, 1.5);

    Item & mouseDetect = this->addNew<Item>("mouse_detect");
    mouseDetect.anchors_.fill(container);
    mouseDetect.anchors_.top_.reset();
    mouseDetect.height_ = ItemRef::scale(container, kPropertyHeight, 2.0);

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouseTrap = this->addNew<MouseTrap>("mouse_trap");
    mouseTrap.onScroll_ = false;
    mouseTrap.anchors_.fill(container);
    mouseTrap.anchors_.right_ = ItemRef::reference(shadow, kPropertyRight);

    Item & timeline = this->addNew<Item>("timeline");
    timeline.anchors_.left_ = ItemRef::reference(*this, kPropertyLeft);
    timeline.anchors_.right_ = ItemRef::reference(*this, kPropertyRight);
    timeline.anchors_.vcenter_ = ItemRef::reference(container, kPropertyTop);
    timeline.margins_.set_left(ItemRef::scale(unit_size_, 0.5));
    timeline.margins_.set_right(timeline.margins_.get_left());
    timeline.height_ = timeline.addExpr(new OddRoundUp(container,
                                                       kPropertyHeight,
                                                       0.22222222, 1));

    TimelineSeek & seek = timeline.add(new TimelineSeek(model_));
    seek.anchors_.fill(timeline);

    ColorRef colorCursor = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::cursor_));

    ColorRef colorExcluded = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_excluded_));

    ColorRef colorOutPt = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_included_, 1, 1));

    ColorRef colorOutPtBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_included_, 0));

    ColorRef colorIncluded = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_included_));

    ColorRef colorPlayed = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_played_));

    ColorRef colorPlayedBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::timeline_played_, 0));

    ColorRef colorTextBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_timecode_));

    ColorRef colorTextFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_timecode_));

    ColorRef colorFocusBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_focus_));

    ColorRef colorFocusFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_focus_));

    ColorRef colorHighlightBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_edit_selected_));

    ColorRef colorHighlightFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_edit_selected_));

    ColorRef colorFullscreenToggleBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_timecode_, 0.64));

    ColorRef colorFullscreenToggleFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_timecode_));

    ColorRef colorFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_));

    ColorRef colorBg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_));

    ColorRef colorControlsFg = timeline.addExpr
      (style_color_ref(view_, &ItemViewStyle::fg_controls_));

    ColorRef colorControlsBg = this->addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_controls_));

    ColorRef colorControlsBgTransparent = this->addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_controls_, 0.0));

    ColorRef colorControlsBgOpaque = this->addExpr
      (style_color_ref(view_, &ItemViewStyle::bg_controls_, 1.0, 1.0));

    Rectangle & timelineIn = timeline.addNew<Rectangle>("timelineIn");
    timelineIn.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    timelineIn.anchors_.right_ =
      timelineIn.addExpr(new TimelineIn(model_, timeline));
    timelineIn.anchors_.vcenter_ =
      ItemRef::reference(timeline, kPropertyVCenter);
    timelineIn.height_ =
      timelineIn.addExpr(new TimelineHeight(view_, mouseDetect, timeline));
    timelineIn.color_ = colorExcluded;
    timelineIn.opacity_ = shadow.opacity_;

    Rectangle & timelinePlayhead =
      timeline.addNew<Rectangle>("timelinePlayhead");
    timelinePlayhead.anchors_.left_ =
      ItemRef::reference(timelineIn, kPropertyRight);
    timelinePlayhead.anchors_.right_ =
      timelinePlayhead.addExpr(new TimelinePlayhead(model_, timeline));
    timelinePlayhead.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    timelinePlayhead.height_ = timelineIn.height_;
    timelinePlayhead.color_ = colorPlayed;
    timelinePlayhead.opacity_ = shadow.opacity_;

    Rectangle & timelineOut =
      timeline.addNew<Rectangle>("timelineOut");
    timelineOut.anchors_.left_ =
      ItemRef::reference(timelinePlayhead, kPropertyRight);
    timelineOut.anchors_.right_ =
      timelineOut.addExpr(new TimelineOut(model_, timeline));
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

    RoundRect & inPoint = this->addNew<RoundRect>("inPoint");
    inPoint.anchors_.hcenter_ =
      ItemRef::reference(timelineIn, kPropertyRight);
    inPoint.anchors_.vcenter_ = timelineIn.anchors_.vcenter_;
    inPoint.width_ = ItemRef::scale(timeline, kPropertyHeight, 0.67);
    inPoint.height_ = inPoint.width_;
    inPoint.radius_ = ItemRef::scale(inPoint, kPropertyHeight, 0.5);
    inPoint.color_ = colorPlayed;
    inPoint.background_ = colorPlayedBg;
    inPoint.visible_ = inPoint.addExpr(new MarkerVisible(view_, mouseDetect));
    inPoint.opacity_ = shadow.opacity_;

    RoundRect & playhead = this->addNew<RoundRect>("playhead");
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

    RoundRect & outPoint = this->addNew<RoundRect>("outPoint");
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
      this->add(new SliderInPoint(model_, timeline));
    sliderInPoint.anchors_.offset(inPoint, -1, 0, -1, 0);

    SliderPlayhead & sliderPlayhead =
      this->add(new SliderPlayhead(model_, timeline));
    sliderPlayhead.anchors_.offset(playhead, -1, 0, -1, 0);

    SliderOutPoint & sliderOutPoint =
      this->add(new SliderOutPoint(model_, timeline));
    sliderOutPoint.anchors_.offset(outPoint, -1, 0, -1, 0);

    const ItemViewStyle & style = *(view_.style());
    QFont timecodeFont = style.font_fixed_;

    Rectangle & playheadAuxBg = container.addNew<Rectangle>("playheadAuxBg");
    Text & playheadAux = container.addNew<Text>("playheadAux");
    TextInput & playheadEdit = this->addNew<TextInput>("playheadEdit");
    playheadAuxBg.opacity_ = shadow.opacity_;
    playheadAux.opacity_ = shadow.opacity_;
    playheadEdit.opacity_ = shadow.opacity_;

    Rectangle & durationAuxBg = container.addNew<Rectangle>("durationAuxBg");
    Text & durationAux = container.addNew<Text>("durationAux");
    durationAuxBg.opacity_ = shadow.opacity_;
    durationAux.opacity_ = shadow.opacity_;

    TextInputProxy & playheadFocus =
      this->add(new TextInputProxy("playheadFocus", playheadAux, playheadEdit));
    ItemFocus::singleton().setFocusable(view_, playheadFocus, "player", 2);
    playheadFocus.copyViewToEdit_ = true;
    playheadFocus.bgNoFocus_ = colorTextBg;
    playheadFocus.bgOnFocus_ = colorFocusBg;
    playheadAux.anchors_.left_ =
      ItemRef::offset(timeline, kPropertyLeft, 3);
    playheadAux.margins_.
      set_left(ItemRef::reference(container, kPropertyHeight));
    playheadAux.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    playheadAux.visible_ =
      playheadAux.addExpr(new ShowWhenFocused(playheadFocus, false));
    playheadAux.color_ = colorTextFg;
    playheadAux.text_ = playheadAux.addExpr(new GetPlayheadAux(model_));
    playheadAux.font_ = timecodeFont;
    playheadAux.fontSize_ =
      ItemRef::scale(container, kPropertyHeight, 0.33333333);

    playheadAuxBg.anchors_.offset(playheadAux, -3, 3, -3, 3);
    playheadAuxBg.color_ = playheadAuxBg.
      addExpr(new ColorWhenFocused(playheadFocus));

    durationAux.anchors_.right_ =
      ItemRef::offset(timeline, kPropertyRight, -3);
    durationAux.margins_.
      set_right(ItemRef::reference(container, kPropertyHeight));
    durationAux.anchors_.vcenter_ =
      ItemRef::reference(container, kPropertyVCenter);
    durationAux.color_ = colorTextFg;
    durationAux.text_ = durationAux.addExpr(new GetDurationAux(model_));
    durationAux.font_ = playheadAux.font_;
    durationAux.fontSize_ = playheadAux.fontSize_;

    durationAuxBg.anchors_.offset(durationAux, -3, 3, -3, 3);
    durationAuxBg.color_ = colorTextBg;

    playheadEdit.anchors_.fill(playheadAux);
    playheadEdit.margins_.
      set_left(ItemRef::scale(playheadEdit, kPropertyCursorWidth, -1.0));
    playheadEdit.visible_ =
      playheadEdit.addExpr(new ShowWhenFocused(playheadFocus, true));
    playheadEdit.color_ = colorFocusFg;
    playheadEdit.cursorColor_ = colorCursor;
    playheadEdit.font_ = playheadAux.font_;
    playheadEdit.fontSize_ = playheadAux.fontSize_;
    playheadEdit.selectionBg_ = colorHighlightBg;
    playheadEdit.selectionFg_ = colorHighlightFg;

    playheadFocus.anchors_.fill(playheadAuxBg);

    CallOnClick<ContextCallback> & playbackToggle =
      this->add(new CallOnClick<ContextCallback>("playback_toggle_on_click",
                                                 this->toggle_playback_));
    Item & playbackBtn = container.addNew<Item>("playback_btn");
    {
      playbackBtn.anchors_.vcenter_ =
        ItemRef::reference(container, kPropertyVCenter);

      playbackBtn.anchors_.left_ =
        ItemRef::reference(*this, kPropertyLeft);

      playbackBtn.anchors_.right_ =
        ItemRef::offset(playheadAuxBg, kPropertyLeft);

      playbackBtn.height_ =
        ItemRef::reference(playheadAuxBg, kPropertyHeight);

      playbackBtn.margins_.set_left(timeline.margins_.get_left());
      playbackBtn.margins_.set_right(timeline.margins_.get_left());

      Item & square = playbackBtn.addNew<Item>("square");
      square.anchors_.vcenter_ = ItemRef::reference(playbackBtn,
                                                    kPropertyVCenter);
      square.anchors_.hcenter_ = ItemRef::reference(playbackBtn,
                                                    kPropertyHCenter);
      square.width_ = ItemRef::reference(playheadAuxBg, kPropertyHeight);
      square.height_ = square.width_;

      TexturedRect & play = square.add(new TexturedRect("play"));
      play.anchors_.fill(square);
      play.visible_ = BoolRef::reference(this->is_playback_paused_);
      play.texture_ = play.addExpr(new StylePlayTexture(view_));
      play.opacity_ = shadow.opacity_;

      TexturedRect & pause = square.add(new TexturedRect("pause"));
      pause.anchors_.fill(square);
      pause.margins_.set(ItemRef::scale(square, kPropertyHeight, 0.05));
      pause.visible_ = BoolRef::inverse(this->is_playback_paused_);
      pause.texture_ = pause.addExpr(new StylePauseTexture(view_));
      pause.opacity_ = shadow.opacity_;

      playbackToggle.anchors_.fill(playbackBtn);
    }

    CallOnClick<ContextCallback> & fullscreenToggle =
      this->add(new CallOnClick<ContextCallback>("fullscreen_toggle_on_click",
                                                 this->toggle_fullscreen_));
    Item & fullscreenBtn = container.addNew<Item>("fullscreen_btn");
    {
      fullscreenBtn.anchors_.vcenter_ =
        ItemRef::reference(container, kPropertyVCenter);

      fullscreenBtn.anchors_.left_ =
        ItemRef::reference(durationAuxBg, kPropertyRight);

      fullscreenBtn.anchors_.right_ =
        ItemRef::reference(*this, kPropertyRight);

      fullscreenBtn.height_ = playbackBtn.height_;

      fullscreenBtn.margins_.set_left(timeline.margins_.get_left());
      fullscreenBtn.margins_.set_right(timeline.margins_.get_left());

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
      bl_small.visible_ = BoolRef::inverse(this->is_fullscreen_);
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
      bl_large.visible_ = BoolRef::reference(this->is_fullscreen_);
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

    // other on-screen controls:
    ItemRef tool_btn_size = ItemRef::scale(unit_size_, 1.5);

    // playlist burger button:
    Item & playlistButton = this->addNew<Item>("playlistButton");
    playlistButton.anchors_.top_ = ItemRef::offset(*this, kPropertyTop, 0);
    playlistButton.anchors_.left_ = ItemRef::reference(*this, kPropertyLeft);
    playlistButton.width_ = tool_btn_size;
    playlistButton.height_ = tool_btn_size;
    playlistButton.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
    playlistButton.margins_.set_left(ItemRef::scale(unit_size_, 0.5));
    playlistButton.visible_.set(new IsValid(toggle_playlist_));
    {
      RoundRect & bg = playlistButton.addNew<RoundRect>("bg");
      bg.anchors_.fill(playlistButton);
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.visible_ = BoolRef::inverse(this->is_playlist_visible_);

      TexturedRect & gridOn = playlistButton.add(new TexturedRect("gridOn"));
      gridOn.anchors_.fill(playlistButton);
      gridOn.margins_.set(ItemRef::scale(unit_size_, 0.2));
      gridOn.visible_ = BoolRef::reference(this->is_playlist_visible_);
      gridOn.texture_ = gridOn.addExpr(new StyleGridOnTexture(view_));
      gridOn.opacity_ = shadow.opacity_;

      TexturedRect & gridOff = playlistButton.add(new TexturedRect("gridOff"));
      gridOff.anchors_.fill(playlistButton);
      gridOff.margins_.set(ItemRef::scale(unit_size_, 0.2));
      gridOff.visible_ = BoolRef::inverse(this->is_playlist_visible_);
      gridOff.texture_ = gridOff.addExpr(new StyleGridOffTexture(view_));
      gridOff.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & playlistToggle = playlistButton.
        add(new Call<TimelineItem, ContextCallback>
            ("toggle_playlist", *this, &TimelineItem::toggle_playlist_));
      playlistToggle.anchors_.fill(playlistButton);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, playlistToggle));
      bg.opacity_.disableCaching();
    }

    Item & other = this->addNew<Item>("other_controls");
    other.anchors_.fill(*this);
    other.visible_ = BoolRef::inverse(this->is_playlist_visible_);

    // back button:
    Item & arrow_btn = other.addNew<Item>("arrow_btn");
    arrow_btn.anchors_.top_ = ItemRef::offset(other, kPropertyTop);
    arrow_btn.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    arrow_btn.visible_ = arrow_btn.addExpr(new IsValid(back_arrow_cb_));
    arrow_btn.height_ = arrow_btn.
      addExpr(new InvisibleItemZeroHeight(arrow_btn));
    {
      RoundRect & bg = arrow_btn.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(arrow_btn, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(arrow_btn, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      ArrowItem & arrow = bg.add<ArrowItem>
        (new ArrowItem("arrow", ArrowItem::kLeft));
      arrow.anchors_.fill(bg);
      arrow.margins_.set(ItemRef::scale(unit_size_, 0.3));
      arrow.weight_ = ItemRef::reference(arrow, kPropertyHeight, 0.178);
      arrow.color_ = colorControlsFg;
      arrow.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & arrow_btn_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("arrow_btn_on_click", *this, &TimelineItem::back_arrow_cb_));
      arrow_btn_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, arrow_btn_ia));
      bg.opacity_.disableCaching();
    }

    // prev item button:
    Item & back_to_prev = other.addNew<Item>("back_to_prev");
    back_to_prev.anchors_.top_ = ItemRef::offset(playlistButton,
                                                 kPropertyBottom);
    back_to_prev.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    back_to_prev.visible_ = back_to_prev.addExpr(new IsValid(back_to_prev_cb_));
    back_to_prev.height_ = back_to_prev.
      addExpr(new InvisibleItemZeroHeight(back_to_prev));
    {
      RoundRect & bg = back_to_prev.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(back_to_prev, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("PREV"));
      txt.alignment_ = Qt::AlignCenter;
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & back_to_prev_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("back_to_prev_on_click", *this, &TimelineItem::back_to_prev_cb_));
      back_to_prev_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, back_to_prev_ia));
      bg.opacity_.disableCaching();
    }

    // next item button:
    Item & skip_to_next = other.addNew<Item>("skip_to_next");
    skip_to_next.anchors_.top_ = ItemRef::offset(back_to_prev,
                                                 kPropertyBottom);
    skip_to_next.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    skip_to_next.visible_ = skip_to_next.addExpr(new IsValid(skip_to_next_cb_));
    skip_to_next.height_ = skip_to_next.
      addExpr(new InvisibleItemZeroHeight(skip_to_next));
    {
      RoundRect & bg = skip_to_next.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(skip_to_next, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("NEXT"));
      txt.alignment_ = Qt::AlignCenter;
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & skip_to_next_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("skip_to_next_on_click", *this, &TimelineItem::skip_to_next_cb_));
      skip_to_next_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, skip_to_next_ia));
      bg.opacity_.disableCaching();
    }

    // select all button:
    Item & select_all = other.addNew<Item>("select_all");
    select_all.anchors_.top_ = ItemRef::offset(skip_to_next,
                                               kPropertyBottom);
    select_all.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    select_all.visible_ = select_all.addExpr(new IsValid(select_all_cb_));
    select_all.height_ = select_all.
      addExpr(new InvisibleItemZeroHeight(select_all));
    {
      RoundRect & bg = select_all.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(select_all, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("SEL *"));
      txt.alignment_ = Qt::AlignCenter;
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & select_all_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("select_all_on_click", *this, &TimelineItem::select_all_cb_));
      select_all_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, select_all_ia));
      bg.opacity_.disableCaching();
    }

    // remove selected button:
    Item & remove_sel = other.addNew<Item>("remove_sel");
    remove_sel.anchors_.top_ = ItemRef::offset(select_all,
                                               kPropertyBottom);
    remove_sel.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    remove_sel.visible_ = remove_sel.addExpr(new IsValid(remove_sel_cb_));
    remove_sel.height_ = remove_sel.
      addExpr(new InvisibleItemZeroHeight(remove_sel));
    {
      RoundRect & bg = remove_sel.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(remove_sel, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("DEL"));
      txt.alignment_ = Qt::AlignCenter;
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & remove_sel_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("remove_sel_on_click", *this, &TimelineItem::remove_sel_cb_));
      remove_sel_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, remove_sel_ia));
      bg.opacity_.disableCaching();
    }

    // trashcan button:
    Item & delete_file = other.addNew<Item>("delete_file");
    delete_file.anchors_.top_ = ItemRef::offset(remove_sel, kPropertyBottom);
    delete_file.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
    delete_file.visible_ = delete_file.addExpr(new IsValid(delete_file_cb_));
    delete_file.height_ = delete_file.
      addExpr(new InvisibleItemZeroHeight(delete_file));
    {
      RoundRect & bg = delete_file.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(delete_file, kPropertyTop);
      bg.anchors_.left_ = ItemRef::reference(other, kPropertyLeft);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_left(ItemRef::scale(unit_size_, 0.5));

      TexturedRect & trashcan = bg.addNew<TexturedRect>("trashcan");
      trashcan.anchors_.fill(bg);
      trashcan.margins_.set(ItemRef::scale(bg, kPropertyHeight, 0.1));
      trashcan.texture_ = trashcan.
        addExpr(new GetTexTrashcan(view_, trashcan));
      trashcan.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & delete_file_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("delete_file_on_click", *this, &TimelineItem::delete_file_cb_));
      delete_file_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, delete_file_ia));
      bg.opacity_.disableCaching();
    }

    // crop:
    Item & frame_crop = other.addNew<Item>("frame_crop");
    frame_crop.anchors_.top_ = arrow_btn.anchors_.top_;
    frame_crop.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
    frame_crop.visible_ = frame_crop.addExpr(new IsValid(frame_crop_cb_));
    frame_crop.height_ = frame_crop.
      addExpr(new InvisibleItemZeroHeight(frame_crop));
    {
      RoundRect & bg = frame_crop.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(frame_crop, kPropertyTop);
      bg.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_right(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("CROP"));
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & frame_crop_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("frame_crop_on_click", *this, &TimelineItem::frame_crop_cb_));
      frame_crop_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, frame_crop_ia));
      bg.opacity_.disableCaching();
    }

    // aspect ratio:
    Item & aspect_ratio = other.addNew<Item>("aspect_ratio");
    aspect_ratio.anchors_.top_ = ItemRef::offset(frame_crop, kPropertyBottom);
    aspect_ratio.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
    aspect_ratio.visible_ = aspect_ratio.
      addExpr(new IsValid(aspect_ratio_cb_));
    aspect_ratio.height_ = aspect_ratio.
      addExpr(new InvisibleItemZeroHeight(aspect_ratio));
    {
      RoundRect & bg = aspect_ratio.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(aspect_ratio, kPropertyTop);
      bg.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_right(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("DAR"));
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & aspect_ratio_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("aspect_ratio_on_click", *this, &TimelineItem::aspect_ratio_cb_));
      aspect_ratio_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, aspect_ratio_ia));
      bg.opacity_.disableCaching();
    }

    // video track selection:
    Item & video_track = other.addNew<Item>("video_track");
    video_track.anchors_.top_ = ItemRef::offset(aspect_ratio, kPropertyBottom);
    video_track.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
    video_track.visible_ = video_track.addExpr(new IsValid(video_track_cb_));
    video_track.height_ = video_track.
      addExpr(new InvisibleItemZeroHeight(video_track));
    {
      RoundRect & bg = video_track.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(video_track, kPropertyTop);
      bg.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_right(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("VID"));
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & video_track_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("video_track_on_click", *this, &TimelineItem::video_track_cb_));
      video_track_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, video_track_ia));
      bg.opacity_.disableCaching();
    }

    // audio track selection:
    Item & audio_track = other.addNew<Item>("audio_track");
    audio_track.anchors_.top_ = ItemRef::offset(video_track, kPropertyBottom);
    audio_track.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
    audio_track.visible_ = audio_track.addExpr(new IsValid(audio_track_cb_));
    audio_track.height_ = audio_track.
      addExpr(new InvisibleItemZeroHeight(audio_track));
    {
      RoundRect & bg = audio_track.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(audio_track, kPropertyTop);
      bg.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_right(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("AUD"));
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & audio_track_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("audio_track_on_click", *this, &TimelineItem::audio_track_cb_));
      audio_track_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, audio_track_ia));
      bg.opacity_.disableCaching();
    }

    // subtitles/captions selection:
    Item & subtt_track = other.addNew<Item>("subtt_track");
    subtt_track.anchors_.top_ = ItemRef::offset(audio_track, kPropertyBottom);
    subtt_track.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
    subtt_track.visible_ = subtt_track.addExpr(new IsValid(subtt_track_cb_));
    subtt_track.height_ = subtt_track.
      addExpr(new InvisibleItemZeroHeight(subtt_track));
    {
      RoundRect & bg = subtt_track.addNew<RoundRect>("bg");
      bg.width_ = tool_btn_size;
      bg.height_ = tool_btn_size;
      bg.radius_ = ItemRef::reference(bg, kPropertyHeight, 0.05, 0.5);
      bg.color_ = colorControlsBgOpaque;
      bg.anchors_.top_ = ItemRef::reference(subtt_track, kPropertyTop);
      bg.anchors_.right_ = ItemRef::reference(other, kPropertyRight);
      bg.margins_.set_top(ItemRef::scale(unit_size_, 0.5));
      bg.margins_.set_right(ItemRef::scale(unit_size_, 0.5));

      Text & txt = bg.addNew<Text>("txt");
      txt.anchors_.center(bg);
      txt.font_ = style.font_large_;
      txt.font_.setWeight(57);
      txt.fontSize_ = ItemRef::reference(bg, kPropertyHeight, 0.33);
      txt.text_ = TVarRef::constant(TVar("TXT"));
      txt.elide_ = Qt::ElideNone;
      txt.color_ = colorControlsFg;
      txt.background_ = colorControlsBgTransparent;
      txt.opacity_ = shadow.opacity_;

      Call<TimelineItem, ContextCallback> & subtt_track_ia = bg.
        add(new Call<TimelineItem, ContextCallback>
            ("subtt_track_on_click", *this, &TimelineItem::subtt_track_cb_));
      subtt_track_ia.anchors_.fill(bg);

      bg.opacity_ = bg.
        addExpr(new ToolButtonOpacity(view_, opacity, bg, subtt_track_ia));
      bg.opacity_.disableCaching();
    }


    TransitionItem & opacityForControls = this->
      addHidden(new TransitionItem("opacity_for_controls",
                                   TPolyline(0.25, 0.0, 1.0, 10),
                                   TPolyline(1.75, 1.0, 1.0),
                                   TPolyline(1.0, 1.0, 0.0, 10)));

    RoundRect & controls = this->addNew<RoundRect>("controls");
    Item & mouseDetectForControls =
      this->addNew<Item>("mouse_detect_for_controls");
    MouseTrap & mouseTrapForControls =
      controls.addNew<MouseTrap>("mouse_trap_for_controls");
    controls.anchors_.vcenter_ =
      ItemRef::reference(*this, kPropertyVCenter);
    controls.anchors_.hcenter_ =
      ItemRef::reference(*this, kPropertyHCenter);
    controls.height_ = ItemRef::scale(unit_size_, 3.0);
    controls.visible_ = BoolRef::inverse(this->is_playlist_visible_);
    controls.visible_.disableCaching();
    mouseTrapForControls.onScroll_ = false;
    mouseTrapForControls.anchors_.fill(controls);

    double cells = 2.0;
    controls.width_ = ItemRef::scale(controls, kPropertyHeight, cells);
    controls.radius_ =
      ItemRef::reference(controls, kPropertyHeight, 0.05, 0.5);
    controls.opacity_ =
      ItemRef::uncacheable(opacityForControls, kPropertyTransition);
    controls.color_ = colorControlsBg;


    CallOnClick<ContextCallback> & bigPlaybackToggle =
      controls.add(new CallOnClick<ContextCallback>
                   ("big_playback_toggle_on_click",
                    this->toggle_playback_));
    Item & bigPlaybackButton = controls.addNew<Item>("bigPlaybackButton");
    {
      bigPlaybackButton.anchors_.vcenter_ =
        ItemRef::reference(controls, kPropertyVCenter);

      bigPlaybackButton.anchors_.left_ =
        ItemRef::reference(controls, kPropertyLeft);

      bigPlaybackButton.margins_.
        set_left(ItemRef::scale(controls, kPropertyWidth, 0.5 / cells));

      bigPlaybackButton.width_ = ItemRef::reference(controls, kPropertyHeight);
      bigPlaybackButton.height_ = bigPlaybackButton.width_;

      TexturedRect & play = bigPlaybackButton.add(new TexturedRect("play"));
      play.anchors_.fill(bigPlaybackButton);
      play.margins_.set(ItemRef::scale(bigPlaybackButton,
                                       kPropertyHeight,
                                       0.15));
      play.visible_ = BoolRef::reference(this->is_playback_paused_);
      play.texture_ = play.addExpr(new StylePlayTexture(view_));
      play.opacity_ = controls.opacity_;

      TexturedRect & pause = bigPlaybackButton.add(new TexturedRect("pause"));
      pause.anchors_.fill(bigPlaybackButton);
      pause.margins_.set(ItemRef::scale(bigPlaybackButton,
                                        kPropertyHeight,
                                        0.2));
      pause.visible_ = BoolRef::inverse(this->is_playback_paused_);
      pause.texture_ = pause.addExpr(new StylePauseTexture(view_));
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

    this->opacity_animator_.reset(new Animator(*this, mouseDetect));
    this->controls_animator_.reset(new AnimatorForControls(*this));

    animate_opacity_.reset(new AnimateOpacity(*this));
    playheadFocus.addObserver(Item::kOnFocus, animate_opacity_);
    playheadFocus.addObserver(Item::kOnFocusOut, animate_opacity_);

    // connect the model:
    bool ok = true;

    ok = connect(&model_, SIGNAL(markerTimeInChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(markerTimeOutChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(&model_, SIGNAL(markerPlayheadChanged()),
                 this, SLOT(modelChanged()));
    YAE_ASSERT(ok);

    ok = connect(&playheadEdit, SIGNAL(editingFinished(const QString &)),
                 &model_, SLOT(seekTo(const QString &)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TimelineItem::maybeAnimateOpacity
  //
  void
  TimelineItem::maybeAnimateOpacity()
  {
    TransitionItem & opacity = *opacity_;
    Animator & animator = dynamic_cast<Animator &>(*(opacity_animator_.get()));

    if (animator.needToPause() && opacity.transition().is_steady())
    {
      opacity.pause(ItemRef::constant(opacity.transition().get_value()));
      view_.delAnimator(opacity_animator_);
    }
    else
    {
      opacity.start();
      view_.addAnimator(opacity_animator_);
    }

    opacity.uncache();
  }

  //----------------------------------------------------------------
  // TimelineItem::maybeAnimateControls
  //
  void
  TimelineItem::maybeAnimateControls()
  {
    TransitionItem & opacity =
      this->get<TransitionItem>("opacity_for_controls");

    AnimatorForControls & animator =
      dynamic_cast<AnimatorForControls &>(*(controls_animator_.get()));

    Item & mouseDetectForControls =
      this->get<Item>("mouse_detect_for_controls");

    bool playbackPaused = is_playback_paused_.get();
    bool needToPause = animator.needToPause();
    const TVec2D & pt = view_.mousePt();

    if (needToPause && opacity.transition().is_steady())
    {
      opacity.pause(ItemRef::constant(opacity.transition().get_value()));
      view_.delAnimator(controls_animator_);
    }
    else if ((!needToPause && opacity.is_paused()) ||
             mouseDetectForControls.overlaps(pt) ||
             playbackPaused)
    {
      opacity.start();
      view_.addAnimator(controls_animator_);
    }

    opacity.uncache();
  }

  //----------------------------------------------------------------
  // TimelineItem::forceAnimateControls
  //
  void
  TimelineItem::forceAnimateControls()
  {
    TransitionItem & opacity =
      this->get<TransitionItem>("opacity_for_controls");

    opacity.pause(ItemRef::constant(opacity.transition().get_spinup_value()));

    this->maybeAnimateControls();
  }

  //----------------------------------------------------------------
  // TimelineItem::processMouseTracking
  //
  void
  TimelineItem::processMouseTracking(const TVec2D & pt)
  {
    Item & root = *this;
    Item & timeline = root["timeline"];

    Item & timelineIn = timeline["timelineIn"];
    view_.requestUncache(&timelineIn);

    Item & timelinePlayhead = timeline["timelinePlayhead"];
    view_.requestUncache(&timelinePlayhead);

    Item & timelineOut = timeline["timelineOut"];
    view_.requestUncache(&timelineOut);

    Item & timelineEnd = timeline["timelineEnd"];
    view_.requestUncache(&timelineEnd);

    Item & inPoint = root["inPoint"];
    view_.requestUncache(&inPoint);

    Item & playhead = root["playhead"];
    view_.requestUncache(&playhead);

    Item & outPoint = root["outPoint"];
    view_.requestUncache(&outPoint);

    // update the opacity transitions:
    maybeAnimateOpacity();
    maybeAnimateControls();
  }

  //----------------------------------------------------------------
  // TimelineItem::uncache
  //
  void
  TimelineItem::uncache()
  {
    TMakeCurrentContext currentContext(*view_.context());
    Item::uncache();

    unit_size_.uncache();
    is_playback_paused_.uncache();
    is_fullscreen_.uncache();
    is_playlist_visible_.uncache();
    is_timeline_visible_.uncache();
  }

  //----------------------------------------------------------------
  // TimelineView::modelChanged
  //
  void
  TimelineItem::modelChanged()
  {
    if (view_.isEnabled())
    {
      view_.requestUncache(this);
      view_.requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // TimelineItem::showTimeline
  //
  void
  TimelineItem::showTimeline(bool show_timeline)
  {
    uncache();
    maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // TimelineItem::showPlaylist
  //
  void
  TimelineItem::showPlaylist(bool show_playlist)
  {
    uncache();
    maybeAnimateOpacity();
  }

}
