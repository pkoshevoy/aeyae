// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Nov 22 16:37:45 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeInputArea.h"
#include "yaeItemViewStyle.h"
#include "yaeOptionItem.h"
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
    IsOptionSelected(const OptionItem & item, int index):
      item_(item),
      index_(index)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = item_.get_selected() == index_;
    }

    const OptionItem & item_;
    int index_;
  };


  //----------------------------------------------------------------
  // SelectOption
  //
  struct SelectOption : public InputArea
  {
    SelectOption(const char * id, OptionItem & item, int index):
      InputArea(id),
      item_(item),
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
      item_.set_selected(index_, true);
      return true;
    }

    OptionItem & item_;
    int index_;
  };


  //----------------------------------------------------------------
  // OptionItem::OptionItem
  //
  OptionItem::OptionItem(const char * id, ItemView & view):
    Item(id),
    view_(view),
    selected_(0)
  {
    const ItemViewStyle & style = *(view_.style());
    Item & root = *this;

    // basic layout:
    unit_size_.set(new UnitSize(view_));

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
    mouse_trap.onDoubleClick_ = false;

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = style_color_ref(view, &ItemViewStyle::fg_, 0.9);

    Scrollview & sv =
      layout_scrollview(kScrollbarVertical, view_, style, root,
                        ItemRef::reference(unit_size_, 0.33));

    Item & mainview = *(sv.content_);

    panel_.reset(new Item("panel"));
    Item & panel = mainview.add<Item>(panel_);
    panel.anchors_.top_ = ItemRef::reference(mainview, kPropertyTop);
    panel.anchors_.left_ = ItemRef::reference(mainview, kPropertyLeft);
    panel.anchors_.right_ = ItemRef::reference(mainview, kPropertyRight);
  }

  //----------------------------------------------------------------
  // OptionItem::~OptionItem
  //
  OptionItem::~OptionItem()
  {
    // clear children first, in order to avoid causing a temporarily
    // dangling DataRefSrc reference to font_size_:
    OptionItem::clear();
  }

  //----------------------------------------------------------------
  // OptionItem::setOptions
  //
  void
  OptionItem::setOptions(const std::vector<OptionItem::Option> & options,
                         int preselect_option)
  {
    options_ = options;
    selected_ = preselect_option;
  }

  //----------------------------------------------------------------
  // OptionItem::uncache
  //
  void
  OptionItem::uncache()
  {
    unit_size_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // OptionItem::processKeyEvent
  //
  bool
  OptionItem::processKeyEvent(Canvas * canvas, QKeyEvent * e)
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
  // OptionItem::setVisible
  //
  void
  OptionItem::setVisible(bool enable)
  {
    bool changing = visible() != enable;
    Item::setVisible(enable);

    if (enable)
    {
      sync_ui();
    }
    else if (changing)
    {
      emit done();
    }
  }

  //----------------------------------------------------------------
  // OptionItem::selected
  //
  void
  OptionItem::set_selected(int index, bool is_done)
  {
    if (index < int(options_.size()))
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
  // OptionItem::sync_ui
  //
  void
  OptionItem::sync_ui()
  {
    Item & panel = *panel_;
    panel.clear();

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
      row.height_.set(new RoundUp(ItemRef::reference(unit_size_), 2.0));


      Rectangle & bg = row.addNew<Rectangle>("bg");
      bg.anchors_.fill(row);
      bg.color_ = style_color_ref(view_, &ItemViewStyle::bg_, 0.3);
      bg.visible_ = bg.addExpr(new IsOptionSelected(*this, index));

      Text & headline = row.addNew<Text>("headline");
      headline.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
      headline.anchors_.right_ = ItemRef::reference(row, kPropertyRight);
      headline.anchors_.bottom_ = ItemRef::reference(row, kPropertyVCenter);
      headline.margins_.set_left(ItemRef::reference(unit_size_, 2.0));
      headline.margins_.set_right(ItemRef::reference(unit_size_, 1.5));
      headline.text_ = TVarRef::constant(TVar(option.headline_));
      headline.color_ =
        style_color_ref(view_, &ItemViewStyle::fg_edit_selected_);
      headline.background_ = ColorRef::transparent(headline, kPropertyColor);
      headline.fontSize_ = ItemRef::reference(unit_size_, 0.55);
      headline.elide_ = Qt::ElideRight;
      headline.setAttr("oneline", true);

      Text & fineprint = row.addNew<Text>("fineprint");
      fineprint.anchors_.top_ = ItemRef::reference(headline, kPropertyBottom);
      fineprint.anchors_.left_ = ItemRef::reference(headline, kPropertyLeft);
      fineprint.anchors_.right_ = ItemRef::reference(headline, kPropertyRight);
      fineprint.text_ = TVarRef::constant(TVar(option.fineprint_));
      fineprint.color_ =
        style_color_ref(view_, &ItemViewStyle::fg_edit_selected_, 0.7);
      fineprint.background_ = ColorRef::transparent(fineprint, kPropertyColor);
      fineprint.fontSize_ = ItemRef::scale(headline, kPropertyFontSize, 0.7);
      fineprint.elide_ = Qt::ElideRight;
      fineprint.setAttr("oneline", true);

      Rectangle & hline = row.addNew<Rectangle>("hline");
      hline.anchors_.fill(row);
      hline.anchors_.top_.reset();
      hline.height_ = ItemRef::constant(1);
      hline.color_ =
        style_color_ref(view_, &ItemViewStyle::bg_, 0.3);
      // hline.visible_ = BoolRef::constant(i % 2 == 1);

      SelectOption & sel = row.add(new SelectOption("sel", *this, index));
      sel.anchors_.fill(row);

      prev = &row;
    }
  }

}
