// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Mar  9 19:22:53 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeAspectRatioView.h"
#include "yaeInputArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatio
  //
  struct AspectRatio
  {
    AspectRatio(double ar = 0.0, const char * label = NULL):
      ar_(ar)
    {
      if (label && *label)
      {
        label_ = label;
      }
      else if (ar)
      {
        label_ = strfmt("%.2f", ar);
      }
    }

    double ar_;
    std::string label_;
  };

  //----------------------------------------------------------------
  // ar_choices
  //
  static const AspectRatio ar_choices[] = {
    AspectRatio(1.0, "1:1"),
    AspectRatio(4.0 / 3.0, "4:3"),
    AspectRatio(16.0 / 10.0, "16:10"),
    AspectRatio(16.0 / 9.0, "16:9"),

    AspectRatio(1.85),
    AspectRatio(2.35),
    AspectRatio(2.40),
    AspectRatio(8.0 / 3.0, "8:3"),

    AspectRatio(3.0 / 4.0, "3:4"),
    AspectRatio(9.0 / 16.0, "9:16"),
    AspectRatio(0.0, "auto"),
    AspectRatio(-1.0, "custom"),
  };

  //----------------------------------------------------------------
  // num_ar_choices
  //
  static const std::size_t num_ar_choices =
    sizeof(ar_choices) / sizeof(ar_choices[0]);

  //----------------------------------------------------------------
  // calc_ar_cols
  //
  static const std::size_t
  calc_ar_cols(double w, double h)
  {
    double n = num_ar_choices;
    double cols = sqrt((n * w) / h);
    return std::max<std::size_t>(1, cols);
  }

  //----------------------------------------------------------------
  // CellSize
  //
  struct GridCols : TDoubleExpr
  {
    GridCols(const Item & grid):
      grid_(grid)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = grid_.width();
      double h = grid_.height();
      double cols = sqrt((num_ar_choices * w) / h);
      result = std::max<double>(1.0, floor(cols));
    }

    const Item & grid_;
  };

  //----------------------------------------------------------------
  // GridRows
  //
  struct GridRows : TDoubleExpr
  {
    GridRows(const ItemRef & cols):
      cols_(cols)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t cols = cols_.get();
      result = double((num_ar_choices + cols - 1) / cols);
    }

    const ItemRef & cols_;
  };

  //----------------------------------------------------------------
  // CellSize
  //
  struct CellSize : TDoubleExpr
  {
    CellSize(const Item & grid,
             const ItemRef & rows,
             const ItemRef & cols):
      grid_(grid),
      rows_(rows),
      cols_(cols)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = grid_.width();
      double h = grid_.height();

      std::size_t rows = std::size_t(rows_.get());
      std::size_t cols = std::size_t(cols_.get());
      double dx = w / cols;
      double dy = h / rows;
      result = std::min<double>(dx, dy);
    }

    const Item & grid_;
    const ItemRef & rows_;
    const ItemRef & cols_;
  };

  //----------------------------------------------------------------
  // CellPosX
  //
  struct CellPosX : TDoubleExpr
  {
    CellPosX(const Item & grid,
             const ItemRef & rows,
             const ItemRef & cols,
             const ItemRef & size,
             std::size_t index):
      grid_(grid),
      rows_(rows),
      cols_(cols),
      size_(size),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = grid_.width();
      double h = grid_.height();
      std::size_t rows = std::size_t(rows_.get());
      std::size_t cols = std::size_t(cols_.get());
      double size = size_.get();

      std::size_t row = index_ / cols;
      std::size_t col = index_ % cols;

      double padding = w - size * cols;
      if (row + 1 == rows)
      {
        padding += size * (cols * rows - num_ar_choices);
      }

      double offset = padding * 0.5;
      result = offset + size * col;
    }

    const Item & grid_;
    const ItemRef & rows_;
    const ItemRef & cols_;
    const ItemRef & size_;
    const std::size_t index_;
  };

  //----------------------------------------------------------------
  // CellPosY
  //
  struct CellPosY : TDoubleExpr
  {
    CellPosY(const Item & grid,
             const ItemRef & rows,
             const ItemRef & cols,
             const ItemRef & size,
             std::size_t index):
      grid_(grid),
      rows_(rows),
      cols_(cols),
      size_(size),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double w = grid_.width();
      double h = grid_.height();
      std::size_t rows = std::size_t(rows_.get());
      std::size_t cols = std::size_t(cols_.get());
      double size = size_.get();

      std::size_t row = index_ / cols;

      double padding = h - size * rows;
      double offset = padding * 0.5;
      result = offset + size * row;
    }

    const Item & grid_;
    const ItemRef & rows_;
    const ItemRef & cols_;
    const ItemRef & size_;
    const std::size_t index_;
  };

  //----------------------------------------------------------------
  // GetFrameWidth
  //
  struct GetFrameWidth : TDoubleExpr
  {
    GetFrameWidth(const AspectRatioView & view,
                  const Item & circle,
                  std::size_t index):
      view_(view),
      circle_(circle),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double a = view_.getAspectRatio(index_);
      double d = circle_.width(); // diameter
      result = (d * a) / sqrt(1 + a * a);
    }

    const AspectRatioView & view_;
    const Item & circle_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // GetFrameHeight
  //
  struct GetFrameHeight : TDoubleExpr
  {
    GetFrameHeight(const AspectRatioView & view,
                   const Item & circle,
                   std::size_t index):
      view_(view),
      circle_(circle),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double a = view_.getAspectRatio(index_);
      double d = circle_.width(); // diameter
      result = d / sqrt(1 + a * a);
    }

    const AspectRatioView & view_;
    const Item & circle_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // FixedAspectRatio
  //
  struct FixedAspectRatio : public InputArea
  {
    FixedAspectRatio(const char * id,
                     AspectRatioView & view,
                     std::size_t index):
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
      view_.selectAspectRatio(index_);
      return true;
    }

    AspectRatioView & view_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // Done
  //
  struct Done : public InputArea
  {
    Done(const char * id, ItemView & view):
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
  // AspectRatioView::AspectRatioView
  //
  AspectRatioView::AspectRatioView():
    ItemView("AspectRatioView"),
    bg_(ColorRef::constant(Color(0xFFFFFF, 0.9))),
    fg_(ColorRef::constant(Color(0x000000, 0.5))),
    style_(NULL),
    sel_(num_ar_choices - 2),
    current_(1.0),
    native_(1.0)
  {}

  //----------------------------------------------------------------
  // AspectRatioView::setStyle
  //
  void
  AspectRatioView::setStyle(ItemViewStyle * new_style)
  {
    style_ = new_style;

    Item & root = *root_;
    const ItemViewStyle & style = *style_;

    bg_ = ColorRef::constant(style.fg_.get().a_scaled(0.9));
    fg_ = style.bg_;

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = root.addExpr(new GetViewWidth(*this));
    root.height_ = root.addExpr(new GetViewHeight(*this));

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = bg_;

    Item & grid = root.addNew<Item>("grid");
    grid.anchors_.fill(root);
    // grid.anchors_.bottom_.reset();

    // dirty hacks to cache grid properties:
    Item & hidden = root.addHidden(new Item("hidden_grid_props"));

    hidden.anchors_.top_ = hidden.addExpr(new GridCols(grid));
    ItemRef & grid_cols = hidden.anchors_.top_;

    hidden.anchors_.left_ = hidden.addExpr(new GridRows(grid_cols));
    ItemRef & grid_rows = hidden.anchors_.left_;

    hidden.anchors_.right_ = hidden.addExpr(new CellSize(grid,
                                                         grid_rows,
                                                         grid_cols));
    ItemRef & cell_size = hidden.anchors_.right_;

    for (std::size_t i = 0; i < num_ar_choices; i++)
    {
      Item & item = grid.addNew<Item>(str("cell_", i).c_str());
      item.anchors_.left_ = item.addExpr(new CellPosX(grid,
                                                      grid_rows,
                                                      grid_cols,
                                                      cell_size,
                                                      i));
      item.anchors_.top_ = item.addExpr(new CellPosY(grid,
                                                     grid_rows,
                                                     grid_cols,
                                                     cell_size,
                                                     i));
      item.width_ = cell_size;
      item.height_ = cell_size;

      RoundRect & circle = item.addNew<RoundRect>("circle");
      circle.anchors_.fill(item);
      circle.radius_ = ItemRef::reference(circle, kPropertyHeight, 0.5);
      circle.background_ = ColorRef::transparent(bg, kPropertyColor);
      circle.color_ = ColorRef::transparent(bg, kPropertyColor);
      circle.colorBorder_ = fg_;
      circle.border_ = ItemRef::reference(circle, kPropertyHeight, 0.01, 1);

      Rectangle & frame = item.addNew<Rectangle>("frame");
      frame.anchors_.center(circle);
      frame.border_ = circle.border_;
      // frame.color_ = circle.color_;
      frame.color_ = circle.colorBorder_;
      // frame.colorBorder_ = circle.colorBorder_;
      frame.width_ = frame.addExpr(new GetFrameWidth(*this, circle, i));
      frame.height_ = frame.addExpr(new GetFrameHeight(*this, circle, i));

      Text & text = item.addNew<Text>("text");
      text.anchors_.center(item);
      text.text_ = TVarRef::constant(TVar(ar_choices[i].label_));
      // text.color_ = fg_;
      text.color_ = bg_;
      text.background_ = ColorRef::transparent(bg, kPropertyColor);
      text.fontSize_ = ItemRef::reference(item, kPropertyHeight, 0.2);
      text.elide_ = Qt::ElideNone;
      text.setAttr("oneline", true);
    }

#if 0
    RoundRect & bg_done = root.addNew<RoundRect>("bg_done");
    Text & tx_done = root.addNew<Text>("tx_done");

    tx_done.anchors_.top_ = ItemRef::reference(text, kPropertyBottom);
    tx_done.anchors_.right_ = ItemRef::reference(text, kPropertyHCenter);
    tx_done.margins_.set_top(ItemRef::reference(text, kPropertyFontHeight));
    tx_done.margins_.set_right(ItemRef::reference(style.title_height_, 2.0));
    tx_done.text_ = TVarRef::constant(TVar("Done"));
    tx_done.color_ = affirmative.fg_;
    tx_done.background_ = affirmative.bg_;
    tx_done.fontSize_ = text.fontSize_;
    tx_done.elide_ = Qt::ElideNone;
    tx_done.setAttr("oneline", true);

    bg_done.anchors_.fill(tx_done, -7.0);
    bg_done.margins_.set_left(ItemRef::reference(style.title_height_, -1));
    bg_done.margins_.set_right(ItemRef::reference(style.title_height_, -1));
    bg_done.color_ = affirmative.bg_;
    bg_done.background_ = ColorRef::constant(bg_.get().a_scaled(0.0));
    bg_done.radius_ = ItemRef::scale(bg_done, kPropertyHeight, 0.1);

    OnDone & on_done = bg_done.add(new OnDone("on_done", *this));
    on_done.anchors_.fill(bg_done);
#endif
  }

  //----------------------------------------------------------------
  // AspectRatioView::setEnabled
  //
  void
  AspectRatioView::setEnabled(bool enable)
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
  }

  //----------------------------------------------------------------
  // AspectRatioView::getAspectRatio
  //
  double
  AspectRatioView::getAspectRatio(std::size_t index) const
  {
    if (index < num_ar_choices - 2)
    {
      return ar_choices[index].ar_;
    }

    if (index == num_ar_choices - 2)
    {
      return native_;
    }

    if (index == num_ar_choices - 1)
    {
      return current_;
    }

    return -1.0;
  }

  //----------------------------------------------------------------
  // AspectRatioView::selectAspectRatio
  //
  void
  AspectRatioView::selectAspectRatio(std::size_t index)
  {
    if (index < num_ar_choices)
    {
      sel_ = index;
      double ar = ((index < num_ar_choices - 2) ? ar_choices[index].ar_ :
                   (index == num_ar_choices - 2) ? 0.0 : // auto
                   current_);
      emit aspectRatio(ar);
    }
  }

  //----------------------------------------------------------------
  // AspectRatioView::setNativeAspectRatio
  //
  void
  AspectRatioView::setNativeAspectRatio(double ar)
  {
    native_ = ar;
  }

  //----------------------------------------------------------------
  // AspectRatioView::setAspectRatio
  //
  void
  AspectRatioView::setAspectRatio(double ar)
  {
    if (ar > 0.0)
    {
      for (std::size_t i = 0; i < num_ar_choices - 1; i++)
      {
        if (close_enough(ar_choices[i].ar_, ar))
        {
          sel_ = i;
          current_ = ar_choices[i].ar_;
          emit aspectRatio(ar);
          break;
        }
      }

      sel_ = num_ar_choices - 1;
      current_ = ar;
      emit aspectRatio(ar);
    }
  }

}
