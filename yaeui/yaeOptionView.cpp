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
      addExpr(style_item_ref(view, &ItemViewStyle::unit_size_));

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
#if 1
    OptionView & view = *this;
    const ItemViewStyle & style = *style_;
    Item & root = *root_;
#endif

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
      row.height_ = row.
        addExpr(style_item_ref(view, &ItemViewStyle::unit_size_));

      Rectangle & bg = row.addNew<Rectangle>("bg");
      bg.anchors_.fill(row);
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.3));
      bg.visible_ = BoolRef::constant(i % 2 == 1);

      Text & text = row.addNew<Text>("text");
      text.anchors_.vcenter(row);
      text.text_ = TVarRef::constant(TVar(option.text_));
      text.color_ = text.addExpr(style_color_ref(view, &ItemViewStyle::fg_));
      text.background_ = ColorRef::transparent(text, kPropertyColor);
      text.fontSize_ = text.
        addExpr(style_item_ref(view, &ItemViewStyle::title_height_));
      text.elide_ = Qt::ElideNone;
      text.setAttr("oneline", true);

      prev = &row;
    }
  }

}
