// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:22:25 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeInputArea.h"
#include "yaeOptionView.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeScrollview.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // OnDone
  //
  struct OnDone : public InputArea
  {
    OnDone(const char * id, ItemView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.setEnabled(false);
      return true;
    }

    ItemView & view_;
  };


  //----------------------------------------------------------------
  // OptionView::OptionView
  //
  OptionView::OptionView():
    ItemView("OptionView"),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // OptionView::setStyle
  //
  void
  OptionView::setStyle(ItemViewStyle * new_style)
  {
    style_ = new_style;

    // basic layout:
    OptionView & view = *this;
    const ItemViewStyle & style = *style_;
    Item & root = *root_;

    hidden_.reset(new Item("hidden"));
    Item & hidden = root.addHidden<Item>(hidden_);
    hidden.width_ = hidden.
      // addExpr(style_item_ref(view, &ItemViewStyle::unit_size_));
      addExpr(new UnitSize(view));

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = root.addExpr(new GetViewWidth(view));
    root.height_ = root.addExpr(new GetViewHeight(view));

#if 0
    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
#endif

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::fg_, 0.9));

    Scrollview & sv =
      layout_scrollview(kScrollbarVertical, view, style, root,
                        ItemRef::reference(hidden, kUnitSize, 0.33));

    Item & mainview = *(sv.content_);

    panel_.reset(new Item("panel"));
    Item & panel = mainview.add<Item>(panel_);
    panel.anchors_.top_ = ItemRef::reference(mainview, kPropertyTop);
    panel.anchors_.left_ = ItemRef::reference(mainview, kPropertyLeft);
    panel.anchors_.right_ = ItemRef::reference(mainview, kPropertyRight);
    // panel.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 1.0));
  }

  //----------------------------------------------------------------
  // OptionView::setTracks
  //
  void
  OptionView::setOptions
  (const std::vector<OptionView::Option> & options)
  {
    options_ = options;
  }

  //----------------------------------------------------------------
  // OptionView::processKeyEvent
  //
  bool
  OptionView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    e->ignore();

    QEvent::Type et = e->type();
    if (et == QEvent::KeyPress)
    {
      int key = e->key();

      if (key == Qt::Key_Return ||
          key == Qt::Key_Enter ||
          key == Qt::Key_Escape)
      {
        emit done();
        e->accept();
      }
    }

    return e->isAccepted();
  }

  //----------------------------------------------------------------
  // OptionView::setEnabled
  //
  void
  OptionView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);

    if (!enable)
    {
      emit done();
    }
    else
    {
      sync_ui();
    }
  }

  //----------------------------------------------------------------
  // OptionView::sync_ui
  //
  void
  OptionView::sync_ui()
  {
    OptionView & view = *this;
    const ItemViewStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = *hidden_;
    Item & panel = *panel_;
    panel.children_.clear();

    std::size_t num_options = options_.size();

    Item * prev = NULL;
    for (std::size_t i = 0; i < num_options; i++)
    {
      const Option & option = options_[i];

      Item & row = panel.addNew<Item>(str("option_", i).c_str());
      row.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);
      row.anchors_.right_ = ItemRef::reference(panel, kPropertyRight);
      row.anchors_.top_ = prev ?
        ItemRef::reference(*prev, kPropertyBottom) :
        ItemRef::reference(panel, kPropertyTop);
      row.height_ = ItemRef::reference(hidden, kUnitSize, 2.0);

      Rectangle & bg = row.addNew<Rectangle>("bg");
      bg.anchors_.fill(row);
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.3));
      bg.visible_ = BoolRef::constant(i % 2 == 1);

      Text & headline = row.addNew<Text>("headline");
      headline.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
      headline.anchors_.right_ = ItemRef::reference(row, kPropertyRight);
      headline.anchors_.bottom_ = ItemRef::reference(row, kPropertyVCenter);
      headline.margins_.
        set_left(ItemRef::reference(hidden, kUnitSize, 2.0));
      headline.margins_.
        set_right(ItemRef::reference(hidden, kUnitSize, 1.5));
      headline.text_ = TVarRef::constant(TVar(option.headline_));
      headline.color_ = headline.
        addExpr(style_color_ref(view, &ItemViewStyle::fg_edit_selected_));
      headline.background_ = ColorRef::transparent(headline, kPropertyColor);
      headline.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.55);
      headline.elide_ = Qt::ElideRight;
      headline.setAttr("oneline", true);

      Text & fineprint = row.addNew<Text>("fineprint");
      fineprint.anchors_.top_ = ItemRef::reference(headline, kPropertyBottom);
      fineprint.anchors_.left_ = ItemRef::reference(headline, kPropertyLeft);
      fineprint.anchors_.right_ = ItemRef::reference(headline, kPropertyRight);
      fineprint.text_ = TVarRef::constant(TVar(option.fineprint_));
      fineprint.color_ = fineprint.
        addExpr(style_color_ref(view, &ItemViewStyle::fg_edit_selected_, 0.7));
      fineprint.background_ = ColorRef::transparent(fineprint, kPropertyColor);
      fineprint.fontSize_ = ItemRef::scale(headline, kPropertyFontSize, 0.7);
      fineprint.elide_ = Qt::ElideRight;
      fineprint.setAttr("oneline", true);

      prev = &row;
    }
  }

}
