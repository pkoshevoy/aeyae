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
#include "yaeDashedRect.h"
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
  // SelectAspectRatio
  //
  struct SelectAspectRatio : public InputArea
  {
    SelectAspectRatio(const char * id,
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
  // LetterBoxColor
  //
  struct LetterBoxColor : TColorExpr
  {
    LetterBoxColor(const AspectRatioView & view, std::size_t index):
      view_(view),
      index_(index)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const ItemViewStyle & style = *(view_.style());
      if (view_.currentSelection() == index_)
      {
        result = style.cursor_.get();
        return;
      }

      result = style.bg_.get().a_scaled(0.3);
    }

    const AspectRatioView & view_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // LetterBoxText
  //
  struct LetterBoxText : TVarExpr
  {
    LetterBoxText(const AspectRatioView & view, std::size_t index):
      view_(view),
      index_(index)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (index_ == num_ar_choices - 1)
      {
        double ar = view_.getAspectRatio(index_);
        result = TVar(yae::strfmt("%.2f", ar));
      }
      else if (index_ < num_ar_choices)
      {
        result = TVar(ar_choices[index_].label_);
      }
    }

    const AspectRatioView & view_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // ReshapeFrame
  //
  // d00 d01 d02
  // d10     d12
  // d20 d21 d22
  //
  struct ReshapeFrame : public InputArea
  {
    ReshapeFrame(const char * id,
                 AspectRatioView & view,
                 const Item & circle,
                 const Item & rect,
                 Item & d01,
                 Item & d10,
                 Item & d12,
                 Item & d21):
      InputArea(id),
      view_(view),
      circle_(circle),
      rect_(rect),
      d01_(d01),
      d10_(d10),
      d12_(d12),
      d21_(d21)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      TVec2D itemCSysPoint = rootCSysPoint - itemCSysOrigin;
      if (d12_.overlaps(itemCSysPoint))
      {
        dragging_ = &d12_;
      }
      else if (d10_.overlaps(itemCSysPoint))
      {
        dragging_ = &d10_;
      }
      else if (d21_.overlaps(itemCSysPoint))
      {
        dragging_ = &d21_;
      }
      else if (d01_.overlaps(itemCSysPoint))
      {
        dragging_ = &d01_;
      }
      else
      {
        dragging_ = NULL;
      }

      if (dragging_)
      {
        rect_.Item::get(kPropertyBBox, anchor_);
        std::cerr << "FIXME: pkoshevoy: select: " << dragging_->id_
                  << ", anchor: { x: " << anchor_.x_ << ", y: " << anchor_.y_
                  << ", w: " << anchor_.w_ << ", h: " << anchor_.h_ << " }"
                  << std::endl;
      }

      return dragging_ != NULL;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      if (!dragging_)
      {
        return false;
      }

      TVec2D drag = rootCSysDragEnd - rootCSysDragStart;
      std::cerr << "FIXME: pkoshevoy: dragging: " << dragging_->id_
                << ", drag: " << drag
                << std::endl;

      double r = circle_.height() * 0.5;

      if (dragging_ == &d21_ || dragging_ == &d01_)
      {
        double h = anchor_.h_ * 0.5;
        if (dragging_ == &d21_)
        {
          h += drag.y();
        }
        else
        {
          h -= drag.y();
        }

        h = std::max(1.0, std::min(r - 1.0, h));
        double w = sqrt(r * r - h * h);
        double ar = w / h;
        view_.setAspectRatio(ar);
      }
      else
      {
        double w = anchor_.w_ * 0.5;
        if (dragging_ == &d12_)
        {
          w += drag.x();
        }
        else
        {
          w -= drag.x();
        }

        w = std::max(1.0, std::min(r - 1.0, w));
        double h = sqrt(r * r - w * w);
        double ar = w / h;
        view_.setAspectRatio(ar);
      }

      view_.requestUncache(parent_);
      view_.requestRepaint();
      return true;
    }

    AspectRatioView & view_;
    const Item & circle_;
    const Item & rect_;

    // d00 d01 d02
    // d10     d12
    // d20 d21 d22
    //
    Item & d01_;
    Item & d10_;
    Item & d12_;
    Item & d21_;

    // a pointer to the dragged handle:
    Item * dragging_;

    // bounding box of the donut hole at the time of drag start:
    BBox anchor_;
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

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = root.addExpr(new GetViewWidth(*this));
    root.height_ = root.addExpr(new GetViewHeight(*this));

    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = bg.addExpr(style_color_ref(*this, &ItemViewStyle::fg_, 0.9));

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
      bool custom = (i == num_ar_choices - 1);
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
      circle.margins_.set(ItemRef::reference(item, kPropertyHeight, 0.05));
      circle.anchors_.fill(item);
      circle.radius_ = ItemRef::reference(circle, kPropertyHeight, 0.5);
      circle.background_ = ColorRef::transparent(bg, kPropertyColor);
      circle.color_ = ColorRef::transparent(bg, kPropertyColor);
      circle.colorBorder_ = circle.
        addExpr(style_color_ref(*this, &ItemViewStyle::bg_, 0.3));
      circle.border_ = ItemRef::reference(circle, kPropertyHeight, 0.005, 1);

      SelectAspectRatio & sel = circle.
        add(new SelectAspectRatio("sel", *this, i));
      sel.anchors_.fill(circle);

      Item * frame = NULL;
      if (custom)
      {
        DashedRect & rect = item.addNew<DashedRect>("frame");
        rect.anchors_.center(circle);
        rect.fg_ = rect.addExpr(style_color_ref(*this, &ItemViewStyle::fg_));
        rect.bg_ = rect.addExpr(new LetterBoxColor(*this, i));
        rect.border_ = circle.border_;
        frame = &rect;

        Item & d01 = item.addNew<Item>("d01");
        d01.anchors_.left_ = ItemRef::reference(rect, kPropertyLeft);
        d01.anchors_.right_ = ItemRef::reference(rect, kPropertyRight);
        d01.anchors_.bottom_ = ItemRef::reference(rect, kPropertyTop);
        d01.margins_.set_bottom(ItemRef::scale(rect, kPropertyBorderWidth, -1));
        d01.height_ = ItemRef::reference(item, kPropertyHeight, 0.2);

        Item & d10 = item.addNew<Item>("d10");
        d10.anchors_.top_ = ItemRef::reference(rect, kPropertyTop);
        d10.anchors_.bottom_ = ItemRef::reference(rect, kPropertyBottom);
        d10.anchors_.right_ = ItemRef::reference(rect, kPropertyLeft);
        d10.margins_.set_right(ItemRef::scale(rect, kPropertyBorderWidth, -1));
        d10.width_ = ItemRef::reference(item, kPropertyHeight, 0.2);

        Item & d12 = item.addNew<Item>("d12");
        d12.anchors_.top_ = ItemRef::reference(rect, kPropertyTop);
        d12.anchors_.bottom_ = ItemRef::reference(rect, kPropertyBottom);
        d12.anchors_.left_ = ItemRef::reference(rect, kPropertyRight);
        d12.margins_.set_left(ItemRef::scale(rect, kPropertyBorderWidth, -1));
        d12.width_ = ItemRef::reference(item, kPropertyHeight, 0.2);

        Item & d21 = item.addNew<Item>("d21");
        d21.anchors_.left_ = ItemRef::reference(rect, kPropertyLeft);
        d21.anchors_.right_ = ItemRef::reference(rect, kPropertyRight);
        d21.anchors_.top_ = ItemRef::reference(rect, kPropertyBottom);
        d21.margins_.set_top(ItemRef::scale(rect, kPropertyBorderWidth, -1));
        d21.height_ = ItemRef::reference(item, kPropertyHeight, 0.2);

        ReshapeFrame & reshaper = item.add(new ReshapeFrame("reshaper",
                                                            *this,
                                                            circle,
                                                            rect,
                                                            d01,
                                                            d10,
                                                            d12,
                                                            d21));
        reshaper.anchors_.fill(item);
      }
      else
      {
        Rectangle & rect = item.addNew<Rectangle>("frame");
        rect.anchors_.center(circle);
        // rect.border_ = circle.border_;
        rect.color_ = rect.addExpr(new LetterBoxColor(*this, i));
        frame = &rect;
      }

      // frame->colorBorder_ = circle.colorBorder_;
      frame->width_ = frame->addExpr(new GetFrameWidth(*this, circle, i));
      frame->height_ = frame->addExpr(new GetFrameHeight(*this, circle, i));

      Text & text = item.addNew<Text>("text");
      text.anchors_.center(item);

      if (custom)
      {
        text.text_ = text.addExpr(new LetterBoxText(*this, i));
        text.color_ = text.
          addExpr(style_color_ref(*this, &ItemViewStyle::bg_, 0.7));
      }
      else
      {
        text.text_ = TVarRef::constant(TVar(ar_choices[i].label_));
        text.color_ = text.
          addExpr(style_color_ref(*this, &ItemViewStyle::fg_));
      }

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

      requestUncache();
      requestRepaint();
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
        if (close_enough(ar_choices[i].ar_, ar, 1e-2))
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
