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
  // IsOptionSelected
  //
  struct IsOptionSelected : TBoolExpr
  {
    IsOptionSelected(const OptionView & view, int index):
      view_(view),
      index_(index)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.get_selected() == index_;
    }

    const OptionView & view_;
    int index_;
  };


  //----------------------------------------------------------------
  // SelectOption
  //
  struct SelectOption : public InputArea
  {
    SelectOption(const char * id, OptionView & view, int index):
      InputArea(id),
      view_(view),
      index_(index)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.set_selected(index_, true);
      return true;
    }

    OptionView & view_;
    int index_;
  };


  //----------------------------------------------------------------
  // OptionView::OptionView
  //
  OptionView::OptionView():
    ItemView("OptionView"),
    style_(NULL),
    selected_(0)
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

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
    mouse_trap.onDoubleClick_ = false;

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
  // OptionView::setOptions
  //
  void
  OptionView::setOptions(const std::vector<OptionView::Option> & options,
                         int preselect_option)
  {
    options_ = options;
    selected_ = preselect_option;
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
  // OptionView::selected
  //
  void
  OptionView::set_selected(int index, bool is_done)
  {
    if (index < options_.size())
    {
      selected_ = index;
      emit option_selected(selected_);
    }

    if (is_done)
    {
      emit done();
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

    int num_options = int(options_.size());

    Item * prev = NULL;
    for (int index = 0; index < num_options; index++)
    {
      const Option & option = options_[index];

      Item & row = panel.addNew<Item>(str("option_", index).c_str());
      row.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);
      row.anchors_.right_ = ItemRef::reference(panel, kPropertyRight);
      row.anchors_.top_ = prev ?
        ItemRef::reference(*prev, kPropertyBottom) :
        ItemRef::reference(panel, kPropertyTop);
      row.height_ = row.addExpr(new RoundUp(hidden, kUnitSize, 2.0));


      Rectangle & bg = row.addNew<Rectangle>("bg");
      bg.anchors_.fill(row);
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.3));
      bg.visible_ = bg.addExpr(new IsOptionSelected(view, index));

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

      Rectangle & hline = row.addNew<Rectangle>("hline");
      hline.anchors_.fill(row);
      hline.anchors_.top_.reset();
      hline.height_ = ItemRef::constant(1);
      hline.color_ = hline.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.3));
      // hline.visible_ = BoolRef::constant(i % 2 == 1);

      SelectOption & sel = row.add(new SelectOption("sel", view, index));
      sel.anchors_.fill(row);

      prev = &row;
    }
  }

}
