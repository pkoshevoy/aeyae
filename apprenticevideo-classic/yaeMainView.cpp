// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Nov 26 09:38:59 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yaeui:
#include "yaeInputArea.h"
#include "yaeRoundRect.h"
#include "yaeTexturedRect.h"

// local:
#include "yaeMainView.h"


namespace yae
{


  //----------------------------------------------------------------
  // OpacityAnimator
  //
  struct OpacityAnimator : public ItemView::IAnimator
  {
    OpacityAnimator(MainView & view):
      view_(view)
    {}

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      TransitionItem & opacity = view_.get_opacity_item();
      if (opacity.transition().is_done())
      {
        view_.delAnimator(animatorPtr);
      }

      opacity.uncache();
    }

    MainView & view_;
  };


  //----------------------------------------------------------------
  // MainView::MainView
  //
  MainView::MainView():
    ItemView("MainView"),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // MainView::setStyle
  //
  void
  MainView::setStyle(ItemViewStyle * new_style)
  {
    if (style_ == new_style)
    {
      return;
    }

    style_ = new_style;

    // shortcuts:
    const ItemViewStyle & style = *style_;
    MainView & view = *this;

    // redo the layout:
    setRoot(ItemPtr(new Item("MainView")));
    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = root.addExpr(new GetViewWidth(view));
    root.height_ = root.addExpr(new GetViewHeight(view));

    hidden_.reset(new Item("hidden"));
    Item & hidden = root.addHidden<Item>(hidden_);
    hidden.width_ = hidden.addExpr(new UnitSize(view));
    hidden.height_ = hidden.addExpr(new CalcTitleHeight(view, 24.0));

    typedef Transition::Polyline TPolyline;
    opacity_.reset(new TransitionItem("opacity",
                                      TPolyline(0.25, 0.0, 1.0, 10),
                                      TPolyline(1.75, 1.0, 1.0),
                                      TPolyline(1.0, 1.0, 0.0, 10)));
    TransitionItem & opacity = root.addHidden<TransitionItem>(opacity_);

    ColorRef color_bg = hidden.addExpr
      (style_color_ref(view, &ItemViewStyle::bg_controls_));

    RoundRect & controls = root.addNew<RoundRect>("controls");
    controls.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
    controls.anchors_.hcenter_ = ItemRef::reference(root, kPropertyHCenter);
    controls.height_ = ItemRef::scale(hidden, kPropertyHeight, 3.0);

    mouse_detect_.reset(new Item("mouse_detect"));
    Item & mouse_detect = root.add<Item>(mouse_detect_);
    mouse_detect.anchors_.fill(controls);

#if 0
    MouseTrap & mouse_trap = controls.addNew<MouseTrap>("mouse_trap");
    mouse_trap.onScroll_ = false;
    mouse_trap.anchors_.fill(controls);
#endif

    double cells = 2.0;
    controls.width_ = ItemRef::scale(controls, kPropertyHeight, cells);
    controls.radius_ = ItemRef::reference(controls, kPropertyHeight, 0.05, 0.5);
    controls.opacity_ = ItemRef::uncacheable(opacity, kPropertyTransition);
    controls.color_ = color_bg;

    CallOnClick<ContextCallback> & toggle =
      controls.add(new CallOnClick<ContextCallback>
                   ("playback_toggle", toggle_playback_));

    Item & playback_btn = controls.addNew<Item>("playback_btn");
    {
      playback_btn.anchors_.vcenter_ =
        ItemRef::reference(controls, kPropertyVCenter);

      playback_btn.anchors_.left_ =
        ItemRef::reference(controls, kPropertyLeft);

      playback_btn.margins_.
        set_left(ItemRef::scale(controls, kPropertyWidth, 0.5 / cells));

      playback_btn.width_ = ItemRef::reference(controls, kPropertyHeight);
      playback_btn.height_ = playback_btn.width_;

      TexturedRect & play = playback_btn.add(new TexturedRect("play"));
      play.anchors_.fill(playback_btn);
      play.margins_.set(ItemRef::scale(playback_btn,
                                       kPropertyHeight,
                                       0.15));
      play.visible_ = play.addExpr(new IsTrue(is_playback_paused_));
      play.visible_.disableCaching();
      play.texture_ = play.addExpr(new StylePlayTexture(view));
      play.opacity_ = controls.opacity_;

      TexturedRect & pause = playback_btn.add(new TexturedRect("pause"));
      pause.anchors_.fill(playback_btn);
      pause.margins_.set(ItemRef::scale(playback_btn,
                                        kPropertyHeight,
                                        0.2));
      pause.visible_ = pause.addExpr(new IsFalse(is_playback_paused_));
      pause.visible_.disableCaching();
      pause.texture_ = pause.addExpr(new StylePauseTexture(view));
      pause.opacity_ = controls.opacity_;

#if 1
      // while there is only one button in the controls container
      // use the entire container area for mouse clicks:
      toggle.anchors_.fill(controls);
#else
      toggle.anchors_.fill(playback_btn);
#endif
    }

    opacity_animator_.reset(new OpacityAnimator(view));
  }

  //----------------------------------------------------------------
  // MainView::processMouseTracking
  //
  bool
  MainView::processMouseTracking(const TVec2D & pt)
  {
    bool ok = ItemView::processMouseTracking(pt);
    maybe_animate_opacity();
    return ok;
  }

  //----------------------------------------------------------------
  // query
  //
  inline static bool
  query(const ContextQuery<bool> & query, bool default_value = false)
  {
    bool result = default_value;
    YAE_ASSERT(query(result));
    return result;
  }

  //----------------------------------------------------------------
  // MainView::maybe_animate_opacity
  //
  void
  MainView::maybe_animate_opacity()
  {
    // shortcuts:
    MainView & view = *this;
    Item & root = *root_;
    Item & mouse_detect = *mouse_detect_;
    TransitionItem & opacity = *opacity_;
    bool playback_paused = query(is_playback_paused_);
    const TVec2D & pt = view.mousePt();

    if (playback_paused ||
        opacity.is_paused() ||
        mouse_detect.overlaps(pt))
    {
      opacity.start();
      view.addAnimator(opacity_animator_);
    }

    opacity.uncache();
  }

  //----------------------------------------------------------------
  // MainView::force_animate_opacity
  //
  void
  MainView::force_animate_opacity()
  {
    TransitionItem & opacity = *opacity_;
    opacity.pause(ItemRef::constant(opacity.transition().get_spinup_value()));
    maybe_animate_opacity();
  }

}
