// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 13 09:57:59 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php


// local:
#include "yaeItemViewStyle.h"
#include "yaeRectangle.h"
#include "yaeSpinnerItem.h"
#include "yaeText.h"
#include "yaeTransform.h"


namespace yae
{

  //----------------------------------------------------------------
  // SpinnerAnimator
  //
  struct SpinnerAnimator : public ItemView::IAnimator
  {
    SpinnerAnimator(SpinnerItem & root):
      root_(root)
    {}

    // virtual:
    void animate(Canvas::ILayer & layer, ItemView::TAnimatorPtr animatorPtr)
    {
      root_.view_.requestUncache(&root_);
      root_.view_.requestRepaint();
    }

    SpinnerItem & root_;
  };


  //----------------------------------------------------------------
  // SpinnerItem::SpinnerItem
  //
  SpinnerItem::SpinnerItem(const char * id, ItemView & view):
    Item(id),
    view_(view)
  {
    bg_.set(style_color_ref(view, &ItemViewStyle::fg_edit_selected_, 0.75));
    fg_.set(style_color_ref(view, &ItemViewStyle::fg_));

    text_color_.set
      (style_color_ref(view, &ItemViewStyle::fg_timecode_, 0, 0.78));

    SpinnerItem::setVisible(false);
  }

  //----------------------------------------------------------------
  // SpinnerItem::~SpinnerItem
  //
  SpinnerItem::~SpinnerItem()
  {
    // clear children first, in order to avoid causing a temporarily
    // dangling DataRefSrc reference to font_size_:
    SpinnerItem::clear();
  }

  //----------------------------------------------------------------
  // SpinnerItem::layout
  //
  void
  SpinnerItem::layout()
  {
    const ItemViewStyle & style = *(view_.style());
    font_size_ = ItemRef::reference(style.title_height_, 0.625);

    this->clear();
    Rectangle & spinner = addNew<Rectangle>("spinner");
    spinner.anchors_.fill(*this);
    spinner.color_.set(bg_);

    typedef Transition::Polyline TPolyline;
    transition_.reset(new TransitionItem("spinner_transition",
                                         TPolyline(0.0, 0.1, 0.1),
                                         TPolyline(0.0, 0.1, 0.1),
                                         TPolyline(12.0, 1.0, 0.1, 10)));

    TransitionItem & transition = addHidden(transition_);
    for (int i = 0; i < 3; i++)
    {
      Transform & xform = spinner.
        addNew<Transform>(str("xform_", i + 1).c_str());

      xform.anchors_.hcenter_ =
        ItemRef::reference(*this, kPropertyHCenter);

      xform.anchors_.vcenter_ =
        ItemRef::reference(*this, kPropertyVCenter);

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
        addExpr(new OddRoundUp(*this, kPropertyHeight, 0.005, 1));
      r3.height_ = ItemRef::reference(r0, kPropertyWidth);
      r6.width_ = ItemRef::reference(r0, kPropertyWidth);
      r9.height_ = ItemRef::reference(r0, kPropertyWidth);

      r0.height_ = ItemRef::reference(*this, kPropertyHeight, 0.03);
      r3.width_ = ItemRef::reference(r0, kPropertyHeight);
      r6.height_ = ItemRef::reference(r0, kPropertyHeight);
      r9.width_ = ItemRef::reference(r0, kPropertyHeight);

      r0.anchors_.bottom_ =
        ItemRef::reference(*this, kPropertyHeight, -0.04);

      r3.anchors_.left_ =
        ItemRef::reference(*this, kPropertyHeight, 0.04);

      r6.anchors_.top_ =
        ItemRef::reference(*this, kPropertyHeight, 0.04);

      r9.anchors_.right_ =
        ItemRef::reference(*this, kPropertyHeight, -0.04);

      r0.color_.set(fg_);
      r3.color_.set(fg_);
      r6.color_.set(fg_);
      r9.color_.set(fg_);

      r0.opacity_ = r0.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(12 - i)));

      r3.opacity_ = r3.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(9 - i)));

      r6.opacity_ = r6.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(6 - i)));

      r9.opacity_ = r9.addExpr
        (new Periodic(transition, 1.0 / 4.0, 1e+9 * double(3 - i)));
    }

    text_.reset(new Text("text"));
    Text & text = add<Text>(text_);

    text.anchors_.hcenter_ = ItemRef::reference(spinner, kPropertyHCenter);
    text.anchors_.vcenter_ = ItemRef::scale(spinner, kPropertyHeight, 0.25);
    text.width_ = ItemRef::reference(spinner, kPropertyWidth, 0.9);
    text.background_ = ColorRef::transparent(spinner, kPropertyColor);
    text.color_.set(text_color_);
    text.text_.set(message_);
    text.fontSize_.set(font_size_);
    text.elide_ = Qt::ElideMiddle;
  }

  //----------------------------------------------------------------
  // SpinnerItem::setVisible
  //
  void
  SpinnerItem::setVisible(bool enable)
  {
    bool changing = visible() != enable;

    if (!enable)
    {
      if (animator_)
      {
        view_.delAnimator(animator_);
        animator_.reset();
      }
    }

    if (enable && changing)
    {
      transition_->start();
      animator_.reset(new SpinnerAnimator(*this));
      view_.addAnimator(animator_);
    }

    Item::setVisible(enable);
  }

  //----------------------------------------------------------------
  // SpinnerItem::uncache
  //
  void
  SpinnerItem::uncache()
  {
    fg_.uncache();
    bg_.uncache();
    text_color_.uncache();
    message_.uncache();
    font_size_.uncache();

    Item::uncache();
  }

}
