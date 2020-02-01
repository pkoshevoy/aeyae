// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jan 20 19:16:27 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yaeui:
#include "yaeConfirmView.h"
#include "yaeInputArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"


namespace yae
{



  //----------------------------------------------------------------
  // ConfirmView::ConfirmView
  //
  ConfirmView::ConfirmView():
    ItemView("ConfirmView"),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // ConfirmView::setStyle
  //
  void
  ConfirmView::setStyle(const ItemViewStyle * style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // ConfirmView::setEnabled
  //
  void
  ConfirmView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

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
      layout();
    }

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // OnAction
  //
  struct OnAction : public InputArea
  {
    OnAction(const char * id,
             ConfirmView & view,
             const yae::shared_ptr<ConfirmView::Action> & action):
      InputArea(id),
      view_(view),
      action_(action)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      const ConfirmView::Action & action = *action_;
      action();
      view_.setEnabled(false);
      return true;
    }

    ConfirmView & view_;
    yae::shared_ptr<ConfirmView::Action> action_;
  };

  //----------------------------------------------------------------
  // ConfirmView::layout
  //
  void
  ConfirmView::layout()
  {
    Item & root = *root_;
    const ItemViewStyle & style = *style_;
    const Action & affirmative = *affirmative_;
    const Action & negative = *negative_;

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = bg_;

    Text & text = root.addNew<Text>("text");
    text.anchors_.top_ = ItemRef::scale(root, kPropertyHeight, 0.33);
    text.anchors_.hcenter_ = ItemRef::reference(root, kPropertyHCenter);
    text.width_ = ItemRef::scale(root, kPropertyWidth, 0.5);
    text.text_ = message_;
    text.color_ = fg_;
    text.background_ = bg_;
    text.fontSize_ = ItemRef::reference(style.unit_size_, 0.625);
    text.elide_ = Qt::ElideNone;
    text.setAttr("linewrap", true);

    RoundRect & bg_yes = root.addNew<RoundRect>("bg_yes");
    RoundRect & bg_no = root.addNew<RoundRect>("bg_no");

    Text & tx_yes = root.addNew<Text>("tx_yes");
    Text & tx_no = root.addNew<Text>("tx_no");

    tx_yes.anchors_.top_ = ItemRef::reference(text, kPropertyBottom);
    tx_yes.anchors_.right_ = ItemRef::reference(text, kPropertyHCenter);
    tx_yes.margins_.set_top(ItemRef::reference(text, kPropertyFontHeight));
    tx_yes.margins_.set_right(ItemRef::reference(style.unit_size_));
    tx_yes.text_ = affirmative.message_;
    tx_yes.color_ = affirmative.fg_;
    tx_yes.background_ = affirmative.bg_;
    tx_yes.fontSize_ = text.fontSize_;
    tx_yes.elide_ = Qt::ElideNone;
    tx_yes.setAttr("oneline", true);

    bg_yes.anchors_.fill(tx_yes, -7.0);
    bg_yes.color_ = affirmative.bg_;
    bg_yes.background_ = bg_;
    bg_yes.radius_ = ItemRef::scale(bg_yes, kPropertyHeight, 0.1);

    OnAction & on_yes = bg_yes.
      add(new OnAction("on_yes", *this, affirmative_));
    on_yes.anchors_.fill(bg_yes);

    tx_no.anchors_.top_ = ItemRef::reference(text, kPropertyBottom);
    tx_no.anchors_.left_ = ItemRef::reference(text, kPropertyHCenter);
    tx_no.margins_.set_top(ItemRef::reference(text, kPropertyFontHeight));
    tx_no.margins_.set_left(ItemRef::reference(style.unit_size_));
    tx_no.text_ = negative.message_;
    tx_no.color_ = negative.fg_;
    tx_no.background_ = negative.bg_;
    tx_no.fontSize_ = text.fontSize_;
    tx_no.elide_ = Qt::ElideNone;
    tx_no.setAttr("oneline", true);

    bg_no.anchors_.fill(tx_no, -7.0);
    bg_no.color_ = negative.bg_;
    // bg_no.colorBorder_ = negative.fg_;
    bg_no.background_ = bg_;
    bg_no.radius_ = ItemRef::scale(bg_no, kPropertyHeight, 0.1);
    // bg_no.border_ = ItemRef::constant(0.25);

    OnAction & on_no = bg_no.
      add(new OnAction("on_no", *this, negative_));
    on_no.anchors_.fill(bg_no);
  }

}
