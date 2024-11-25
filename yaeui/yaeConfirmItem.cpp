// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 13 14:59:27 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeConfirmItem.h"
#include "yaeInputArea.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // OnAction
  //
  struct OnAction : public InputArea
  {
    OnAction(const char * id,
             ConfirmItem & confirm,
             const yae::shared_ptr<ConfirmItem::Action> & action):
      InputArea(id),
      confirm_(confirm),
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
      yae::shared_ptr<ConfirmItem, Item> keep_alive = confirm_.self_.lock();
      YAE_ASSERT(keep_alive);

      const ConfirmItem::Action & action = *action_;
      action.execute();

      // hmm, should onClick do this, or action.execute?
      confirm_.setVisible(false);

      return true;
    }

    ConfirmItem & confirm_;
    yae::shared_ptr<ConfirmItem::Action> action_;
  };


  //----------------------------------------------------------------
  // ConfirmItem::ConfirmItem
  //
  ConfirmItem::ConfirmItem(const char * id, ItemView & view):
    Item(id),
    view_(view),
    bg_(ColorRef::constant(Color(0xFFFFFF, 0.7))),
    fg_(ColorRef::constant(Color(0x000000, 1.0)))
  {
    const ItemViewStyle & style = *view_.style();
    font_size_ = ItemRef::reference(style.title_height_);
  }

  //----------------------------------------------------------------
  // ConfirmItem::~ConfirmItem
  //
  ConfirmItem::~ConfirmItem()
  {
    // clear children first, in order to avoid causing a temporarily
    // dangling DataRefSrc reference to font_size_:
    ConfirmItem::clear();
  }

  //----------------------------------------------------------------
  // ConfirmItem::layout
  //
  void
  ConfirmItem::layout()
  {
    // shortcuts:
    ConfirmItem & root = *this;
    const ItemViewStyle & style = *view_.style();
    const Action & affirmative = *affirmative_;
    const Action & negative = *negative_;

    root.clear();

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
    mouse_trap.onDoubleClick_ = false;

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_.set(bg_);

    Text & text = root.addNew<Text>("text");
    text.anchors_.top_ = ItemRef::scale(root, kPropertyHeight, 0.33);
    text.anchors_.hcenter_ = ItemRef::reference(root, kPropertyHCenter);
    text.width_ = ItemRef::scale(root, kPropertyWidth, 0.67);
    text.text_.set(message_);
    text.color_.set(fg_);
    text.background_.set(bg_);
    text.fontSize_.set(font_size_);
    text.elide_ = Qt::ElideNone;
    text.setAttr("linewrap", true);

    RoundRect & bg_yes = root.addNew<RoundRect>("bg_yes");
    RoundRect & bg_no = root.addNew<RoundRect>("bg_no");

    Text & tx_yes = root.addNew<Text>("tx_yes");
    Text & tx_no = root.addNew<Text>("tx_no");

    tx_yes.anchors_.top_ = ItemRef::reference(text, kPropertyBottom);
    tx_yes.anchors_.right_ = ItemRef::reference(text, kPropertyHCenter);
    tx_yes.margins_.set_top(ItemRef::reference(text, kPropertyFontHeight));
    tx_yes.margins_.set_right(ItemRef::reference(style.title_height_, 2.0));
    tx_yes.text_.set(affirmative.message_);
    tx_yes.color_.set(affirmative.fg_);
    tx_yes.background_.set(affirmative.bg_);
    tx_yes.fontSize_.set(text.fontSize_);
    tx_yes.elide_ = Qt::ElideNone;
    tx_yes.setAttr("oneline", true);

    bg_yes.anchors_.fill(tx_yes, -7.0);
    bg_yes.margins_.set_left(ItemRef::reference(style.title_height_, -1));
    bg_yes.margins_.set_right(ItemRef::reference(style.title_height_, -1));
    bg_yes.color_.set(affirmative.bg_);
    bg_yes.background_ = ColorRef::constant(bg_.get().a_scaled(0.0));
    bg_yes.radius_ = ItemRef::scale(bg_yes, kPropertyHeight, 0.1);

    OnAction & on_yes = bg_yes.add(new OnAction("on_yes", root, affirmative_));
    on_yes.anchors_.fill(bg_yes);

    tx_no.anchors_.top_ = ItemRef::reference(text, kPropertyBottom);
    tx_no.anchors_.left_ = ItemRef::reference(text, kPropertyHCenter);
    tx_no.margins_.set_top(ItemRef::reference(text, kPropertyFontHeight));
    tx_no.margins_.set_left(ItemRef::reference(style.title_height_, 2.0));
    tx_no.text_.set(negative.message_);
    tx_no.color_.set(negative.fg_);
    tx_no.background_.set(negative.bg_);
    tx_no.fontSize_.set(text.fontSize_);
    tx_no.elide_ = Qt::ElideNone;
    tx_no.setAttr("oneline", true);

    bg_no.anchors_.fill(tx_no, -7.0);
    bg_no.margins_.set_left(ItemRef::reference(style.title_height_, -1));
    bg_no.margins_.set_right(ItemRef::reference(style.title_height_, -1));
    bg_no.color_.set(negative.bg_);
    // bg_no.colorBorder_ = ColorRef::reference(negative.fg_);
    bg_no.background_ = ColorRef::constant(bg_.get().a_scaled(0.0));
    bg_no.radius_ = ItemRef::scale(bg_no, kPropertyHeight, 0.1);
    // bg_no.border_ = ItemRef::constant(0.25);

    OnAction & on_no = bg_no.add(new OnAction("on_no", root, negative_));
    on_no.anchors_.fill(bg_no);
  }

  //----------------------------------------------------------------
  // ConfirmItem::uncache
  //
  void
  ConfirmItem::uncache()
  {
    bg_.uncache();
    fg_.uncache();
    message_.uncache();
    font_size_.uncache();

    affirmative_->uncache();
    negative_->uncache();

    Item::uncache();
  }

}
