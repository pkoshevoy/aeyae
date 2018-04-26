// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Apr 24 21:41:30 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// local:
#include "yaeSpinnerView.h"
#include "yaeRectangle.h"
#include "yaeText.h"
#include "yaeTransform.h"


namespace yae
{

  //----------------------------------------------------------------
  // SpinnerAnimator
  //
  struct SpinnerAnimator : public ItemView::IAnimator
  {
    SpinnerAnimator(SpinnerView & view):
      view_(view)
    {}

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      Item & root = *(view_.root());
      Item & spinner = root.get<Item>("spinner");
      view_.requestUncache(&spinner);
      view_.requestRepaint();
    }

    SpinnerView & view_;
  };


  //----------------------------------------------------------------
  // GetSpinnerText
  //
  struct GetSpinnerText : public TVarExpr
  {
    GetSpinnerText(const SpinnerView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = view_.text();
    }

    const SpinnerView & view_;
  };


  //----------------------------------------------------------------
  // layout_clock_spinner
  //
  TransitionItem &
  layout_clock_spinner(SpinnerView & view,
                       const ItemViewStyle & style,
                       Item & root)
  {
    Rectangle & spinner = root.addNew<Rectangle>("spinner");
    spinner.anchors_.fill(root);
    spinner.color_ = spinner.
      addExpr(style_color_ref(view, &ItemViewStyle::fg_edit_selected_, 0.75));

    typedef Transition::Polyline TPolyline;
    TransitionItem & transition = root.
      addHidden(new TransitionItem("spinner_transition",
                                   TPolyline(0.0, 0.1, 0.1),
                                   TPolyline(0.0, 0.1, 0.1),
                                   TPolyline(12.0, 1.0, 0.1, 10)));

    for (int i = 0; i < 3; i++)
    {
      Transform & xform = spinner.
        addNew<Transform>(str("xform_", i + 1).c_str());

      xform.anchors_.hcenter_ =
        ItemRef::reference(root, kPropertyHCenter);

      xform.anchors_.vcenter_ =
        ItemRef::reference(root, kPropertyVCenter);

      xform.rotation_ = ItemRef::constant(M_PI * double(i) / 6.0);

      Rectangle & r0 = xform.
        addNew<Rectangle>(str("hh_", i).c_str());

      Rectangle & r3 = xform.
        addNew<Rectangle>(str("hh_", i + 3).c_str());

      Rectangle & r6 = xform.
        addNew<Rectangle>(str("hh_", i + 6).c_str());

      Rectangle & r9 = xform.
        addNew<Rectangle>(str("hh_", i + 9).c_str());

      r0.anchors_.hcenter_ = ItemRef::constant(0.0);
      r3.anchors_.vcenter_ = ItemRef::constant(0.0);
      r6.anchors_.hcenter_ = r0.anchors_.hcenter_;
      r9.anchors_.vcenter_ = r3.anchors_.vcenter_;

      r0.width_ = r0.
        addExpr(new OddRoundUp(root, kPropertyHeight, 0.005, 1));
      r3.height_ = ItemRef::reference(r0, kPropertyWidth);
      r6.width_ = ItemRef::reference(r0, kPropertyWidth);
      r9.height_ = ItemRef::reference(r0, kPropertyWidth);

      r0.height_ = ItemRef::reference(root, kPropertyHeight, 0.03);
      r3.width_ = ItemRef::reference(r0, kPropertyHeight);
      r6.height_ = ItemRef::reference(r0, kPropertyHeight);
      r9.width_ = ItemRef::reference(r0, kPropertyHeight);

      r0.anchors_.bottom_ =
        ItemRef::reference(root, kPropertyHeight, -0.04);

      r3.anchors_.left_ =
        ItemRef::reference(root, kPropertyHeight, 0.04);

      r6.anchors_.top_ =
        ItemRef::reference(root, kPropertyHeight, 0.04);

      r9.anchors_.right_ =
        ItemRef::reference(root, kPropertyHeight, -0.04);

      r0.color_ = r0.
        addExpr(style_color_ref(view, &ItemViewStyle::fg_));

      r3.color_ = ColorRef::reference(r0, kPropertyColor);
      r6.color_ = ColorRef::reference(r0, kPropertyColor);
      r9.color_ = ColorRef::reference(r0, kPropertyColor);

      r0.opacity_ = r0.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(12 - i)));

      r3.opacity_ = r3.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(9 - i)));

      r6.opacity_ = r6.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(6 - i)));

      r9.opacity_ = r9.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(3 - i)));
    }

    Text & text = root.addNew<Text>("text");
    text.anchors_.hcenter_ = ItemRef::reference(spinner, kPropertyHCenter);
    text.anchors_.vcenter_ = ItemRef::reference(spinner, kPropertyHeight, 0.25);
    text.width_ = ItemRef::reference(spinner, kPropertyWidth, 0.9);
    text.background_ = ColorRef::transparent(spinner, kPropertyColor);
    text.color_ = text.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_timecode_, 0, 0.78));
    text.text_ = text.addExpr(new GetSpinnerText(view));
    text.fontSize_ = ItemRef::reference(style.title_height_, 0.3);
    text.elide_ = Qt::ElideMiddle;

    return transition;
  }


  //----------------------------------------------------------------
  // SpinnerView::SpinnerView
  //
  SpinnerView::SpinnerView():
    ItemView("SpinnerView"),
    style_(NULL),
    text_(tr("please wait"))
  {}

  //----------------------------------------------------------------
  // SpinnerView::setStyle
  //
  void
  SpinnerView::setStyle(const ItemViewStyle * style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // SpinnerView::setEnabled
  //
  void
  SpinnerView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    if (!enable && animator_)
    {
      delAnimator(animator_);
      animator_.reset();
    }

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
      TransitionItem & transition = layout_clock_spinner(*this, *style_, root);
      transition.start();
      animator_.reset(new SpinnerAnimator(*this));
      addAnimator(animator_);
    }

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // SpinnerView::setText
  //
  void
  SpinnerView::setText(const QString & text)
  {
    text_ = text;
    requestUncache(root_.get());
    requestRepaint();
  }

}
