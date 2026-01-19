// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Nov 22 14:23:57 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeAspectRatioItem.h"
#include "yaeDashedRect.h"
#include "yaeInputArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatio::AspectRatio
  //
  AspectRatio::AspectRatio(double ar,
                           const char * label,
                           AspectRatio::Category category,
                           const char * select_subview):
    ar_(ar),
    category_(category)
  {
    if (label && *label)
    {
      label_ = label;
    }
    else if (ar)
    {
      label_ = strfmt("%.2f", ar);
    }

    if (select_subview && *select_subview)
    {
      subview_ = select_subview;
    }
  }


  //----------------------------------------------------------------
  // CellSize
  //
  struct GridCols : TDoubleExpr
  {
    GridCols(const AspectRatioItem & item, const Item & grid):
      item_(item),
      grid_(grid)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t n = item_.options().size();
      double w = grid_.width();
      double h = grid_.height();
      double cols = sqrt((n * w) / h);
      double rows = double((n + cols - 1) / cols);
      cols = double((n + rows - 1) / rows);
      result = std::max<double>(1.0, floor(cols));
    }

    const AspectRatioItem & item_;
    const Item & grid_;
  };

  //----------------------------------------------------------------
  // GridRows
  //
  struct GridRows : TDoubleExpr
  {
    GridRows(const AspectRatioItem & item, const ItemRef & cols):
      item_(item),
      cols_(cols)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t n = item_.options().size();
      std::size_t cols = cols_.get();
      result = double((n + cols - 1) / cols);
    }

    const AspectRatioItem & item_;
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
    CellPosX(const AspectRatioItem & item,
             const Item & grid,
             const ItemRef & rows,
             const ItemRef & cols,
             const ItemRef & size,
             std::size_t index):
      item_(item),
      grid_(grid),
      rows_(rows),
      cols_(cols),
      size_(size),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t n = item_.options().size();
      double w = grid_.width();

      std::size_t rows = std::size_t(rows_.get());
      std::size_t cols = std::size_t(cols_.get());
      double size = size_.get();

      std::size_t row = index_ / cols;
      std::size_t col = index_ % cols;

      double padding = w - size * cols;
      if (row + 1 == rows)
      {
        padding += size * (cols * rows - n);
      }

      double offset = padding * 0.5;
      result = offset + size * col;
    }

    const AspectRatioItem & item_;
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
    GetFrameWidth(const AspectRatioItem & item,
                  const Item & circle,
                  std::size_t index):
      item_(item),
      circle_(circle),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double a = item_.getAspectRatio(index_);
      double d = circle_.width(); // diameter
      result = (d * a) / sqrt(1 + a * a);
    }

    const AspectRatioItem & item_;
    const Item & circle_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // GetFrameHeight
  //
  struct GetFrameHeight : TDoubleExpr
  {
    GetFrameHeight(const AspectRatioItem & item,
                   const Item & circle,
                   std::size_t index):
      item_(item),
      circle_(circle),
      index_(index)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double a = item_.getAspectRatio(index_);
      double d = circle_.width(); // diameter
      result = d / sqrt(1 + a * a);
    }

    const AspectRatioItem & item_;
    const Item & circle_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // SelectAspectRatio
  //
  struct SelectAspectRatio : public InputArea
  {
    SelectAspectRatio(const char * id,
                      AspectRatioItem & item,
                      std::size_t index):
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
      item_.selectAspectRatio(index_);
      return true;
    }

    AspectRatioItem & item_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // LetterBoxColor
  //
  struct LetterBoxColor : TColorExpr
  {
    LetterBoxColor(const AspectRatioItem & item, std::size_t index):
      item_(item),
      index_(index)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const ItemView & view = item_.view();
      const ItemViewStyle & style = *(view.style());

      if (item_.currentSelection() == index_)
      {
        result = style.cursor_.get();
        return;
      }

      result = style.bg_.get().a_scaled(0.3);
    }

    const AspectRatioItem & item_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // LetterBoxText
  //
  struct LetterBoxText : TVarExpr
  {
    LetterBoxText(const AspectRatioItem & item, std::size_t index):
      item_(item),
      index_(index)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      std::size_t n = item_.options().size();
      if (index_ < n)
      {
        const AspectRatio & option = item_.options().at(index_);

        if (option.category_ == AspectRatio::kOther &&
            option.subview_.empty())
        {
          double ar = item_.getAspectRatio(index_);
          result = TVar(yae::strfmt("%.2f", ar));
        }
        else
        {
          result = TVar(option.label_);
        }
      }
    }

    const AspectRatioItem & item_;
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
                 AspectRatioItem & item,
                 const Item & circle,
                 const Item & rect,
                 Item & d01,
                 Item & d10,
                 Item & d12,
                 Item & d21):
      InputArea(id),
      item_(item),
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
        item_.setAspectRatio(ar);
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
        item_.setAspectRatio(ar);
      }

      ItemView & view = item_.view();
      view.requestUncache(parent_);
      view.requestRepaint();
      return true;
    }

    AspectRatioItem & item_;
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
  // OnDone
  //
  struct OnDone : public InputArea
  {
    OnDone(const char * id, Item & item):
      InputArea(id),
      item_(item)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      item_.setVisible(false);
      return true;
    }

    Item & item_;
  };

  //----------------------------------------------------------------
  // AspectRatioItem::AspectRatioItem
  //
  AspectRatioItem::AspectRatioItem(const char * id,
                                   ItemView & view,
                                   const AspectRatio * options,
                                   std::size_t num_options):
    Item(id),
    view_(view),
    sel_(0),
    current_(1.0),
    native_(1.0)
  {
    if (options && num_options)
    {
      options_.assign(options, options + num_options);
    }

    const ItemViewStyle & style = *(view_.style());
    Item & root = *this;

#if 1
    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
#endif

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = style_color_ref(view_, &ItemViewStyle::fg_, 0.9);

    Item & grid = root.addNew<Item>("grid");
    Item & footer = root.addNew<Item>("footer");
    grid.anchors_.fill(root);
    grid.anchors_.bottom_ = ItemRef::reference(footer, kPropertyTop);
    footer.anchors_.fill(root);
    footer.anchors_.top_.reset();
    footer.height_ = ItemRef::reference(style.title_height_, 3.0);

    // dirty hacks to cache grid properties:
    Item & hidden = root.addHidden(new Item("hidden_grid_props"));

    hidden.anchors_.top_ = hidden.addExpr(new GridCols(*this, grid));
    ItemRef & grid_cols = hidden.anchors_.top_;

    hidden.anchors_.left_ = hidden.addExpr(new GridRows(*this, grid_cols));
    ItemRef & grid_rows = hidden.anchors_.left_;

    hidden.anchors_.right_ = hidden.addExpr(new CellSize(grid,
                                                         grid_rows,
                                                         grid_cols));
    ItemRef & cell_size = hidden.anchors_.right_;

    std::size_t num_ar_choices = options_.size();
    for (std::size_t i = 0; i < num_ar_choices; i++)
    {
      const AspectRatio & option = options_[i];

      bool custom_option = (option.category_ == AspectRatio::kOther);

      Item & item = grid.addNew<Item>(str("cell_", i).c_str());
      item.anchors_.left_ = item.addExpr(new CellPosX(*this,
                                                      grid,
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
      circle.colorBorder_ = style_color_ref(view_, &ItemViewStyle::bg_, 0.3);
      circle.border_ = ItemRef::reference(circle, kPropertyHeight, 0.005, 1);

      SelectAspectRatio & sel = circle.
        add(new SelectAspectRatio("sel", *this, i));
      sel.anchors_.fill(circle);

      Rectangle & rect = item.addNew<Rectangle>("frame");
      rect.anchors_.center(circle);
      rect.color_ = rect.addExpr(new LetterBoxColor(*this, i));
      rect.width_ = rect.addExpr(new GetFrameWidth(*this, circle, i));
      rect.height_ = rect.addExpr(new GetFrameHeight(*this, circle, i));

      if (custom_option)
      {
        DashedRect & stripes = item.addNew<DashedRect>("stripes");
        stripes.anchors_.fill(rect);
        stripes.fg_ = style_color_ref(view_, &ItemViewStyle::fg_);
        stripes.bg_ = stripes.addExpr(new LetterBoxColor(*this, i));
        stripes.border_ = circle.border_;

        if (option.subview_.empty())
        {
          Item & d01 = item.addNew<Item>("d01");
          d01.anchors_.left_ = ItemRef::reference(stripes, kPropertyLeft);
          d01.anchors_.right_ = ItemRef::reference(stripes, kPropertyRight);
          d01.anchors_.bottom_ = ItemRef::reference(stripes, kPropertyTop);
          d01.height_ = ItemRef::reference(item, kPropertyHeight, 0.2);
          d01.margins_.set_bottom(ItemRef::reference(stripes,
                                                     kPropertyBorderWidth,
                                                     -1, -1));

          Item & d10 = item.addNew<Item>("d10");
          d10.anchors_.top_ = ItemRef::reference(stripes, kPropertyTop);
          d10.anchors_.bottom_ = ItemRef::reference(stripes, kPropertyBottom);
          d10.anchors_.right_ = ItemRef::reference(stripes, kPropertyLeft);
          d10.width_ = ItemRef::reference(item, kPropertyHeight, 0.2);
          d10.margins_.set_right(ItemRef::reference(stripes,
                                                    kPropertyBorderWidth,
                                                    -1, -1));

          Item & d12 = item.addNew<Item>("d12");
          d12.anchors_.top_ = ItemRef::reference(stripes, kPropertyTop);
          d12.anchors_.bottom_ = ItemRef::reference(stripes, kPropertyBottom);
          d12.anchors_.left_ = ItemRef::reference(stripes, kPropertyRight);
          d12.width_ = ItemRef::reference(item, kPropertyHeight, 0.2);
          d12.margins_.set_left(ItemRef::reference(stripes,
                                                   kPropertyBorderWidth,
                                                   -1, -1));

          Item & d21 = item.addNew<Item>("d21");
          d21.anchors_.left_ = ItemRef::reference(stripes, kPropertyLeft);
          d21.anchors_.right_ = ItemRef::reference(stripes, kPropertyRight);
          d21.anchors_.top_ = ItemRef::reference(stripes, kPropertyBottom);
          d21.height_ = ItemRef::reference(item, kPropertyHeight, 0.2);
          d21.margins_.set_top(ItemRef::reference(stripes,
                                                  kPropertyBorderWidth,
                                                  -1, -1));

          ReshapeFrame & reshaper = item.add(new ReshapeFrame("reshaper",
                                                              *this,
                                                              circle,
                                                              stripes,
                                                              d01,
                                                              d10,
                                                              d12,
                                                              d21));
          reshaper.anchors_.fill(item);
        }
      }

      Text & text = item.addNew<Text>("text");
      text.anchors_.center(item);

      if (custom_option)
      {
        text.text_ = text.addExpr(new LetterBoxText(*this, i));
      }
      else
      {
        text.text_ = TVarRef::constant(TVar(option.label_));
      }

      text.color_ = style_color_ref(view_, &ItemViewStyle::fg_);
      text.background_ = ColorRef::transparent(bg, kPropertyColor);
      text.fontSize_ = ItemRef::reference(style.title_height_);
      text.elide_ = Qt::ElideNone;
      text.setAttr("oneline", true);
    }

    RoundRect & bg_done = footer.addNew<RoundRect>("bg_done");
    Text & tx_done = footer.addNew<Text>("tx_done");

    tx_done.anchors_.center(footer);
    tx_done.text_ = TVarRef::constant(TVar("Done"));
    tx_done.color_ = style_color_ref(view_, &ItemViewStyle::fg_);
    tx_done.background_ = style_color_ref(view_, &ItemViewStyle::bg_, 0.0);
    tx_done.fontSize_ = ItemRef::reference(style.title_height_);
    tx_done.elide_ = Qt::ElideNone;
    tx_done.setAttr("oneline", true);

    bg_done.anchors_.fill(tx_done, -7.0);
    bg_done.margins_.set_left(ItemRef::reference(style.title_height_, -1));
    bg_done.margins_.set_right(ItemRef::reference(style.title_height_, -1));
    bg_done.color_ = style_color_ref(view_, &ItemViewStyle::bg_, 0.3);
    bg_done.background_ = style_color_ref(view_, &ItemViewStyle::fg_, 0.0);
    bg_done.radius_ = ItemRef::scale(bg_done, kPropertyHeight, 0.1);

    OnDone & on_done = bg_done.add(new OnDone("on_done", *this));
    on_done.anchors_.fill(bg_done);
  }

  //----------------------------------------------------------------
  // AspectRatioItem::processKeyEvent
  //
  bool
  AspectRatioItem::processKeyEvent(Canvas * canvas, QKeyEvent * e)
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
  // AspectRatioItem::setVisible
  //
  void
  AspectRatioItem::setVisible(bool enable)
  {
    bool changing = visible() != enable;
    Item::setVisible(enable);

    if (changing && !enable)
    {
      emit done();
    }
  }

  //----------------------------------------------------------------
  // AspectRatioItem::getAspectRatio
  //
  double
  AspectRatioItem::getAspectRatio(std::size_t index) const
  {
    std::size_t num_ar_choices = options_.size();
    if (index < num_ar_choices)
    {
      const AspectRatio & option = options_.at(index);

      if (option.category_ == AspectRatio::kNone ||
          option.category_ == AspectRatio::kAuto)
      {
        return native_;
      }

      if (option.category_ == AspectRatio::kOther)
      {
        return current_;
      }

      return option.ar_;
    }

    return -1.0;
  }

  //----------------------------------------------------------------
  // AspectRatioItem::selectAspectRatioCategory
  //
  void
  AspectRatioItem::selectAspectRatioCategory(AspectRatio::Category category)
  {
    std::size_t num_ar_choices = options_.size();
    for (std::size_t i = 0; i < num_ar_choices; i++)
    {
      const AspectRatio & option = options_.at(i);
      if (option.category_ == category)
      {
        sel_ = i;
        emit selected(option);

        view_.requestUncache(this);
        view_.requestRepaint();

        return;
      }
    }
  }

  //----------------------------------------------------------------
  // AspectRatioItem::selectAspectRatio
  //
  void
  AspectRatioItem::selectAspectRatio(std::size_t index)
  {
    std::size_t num_ar_choices = options_.size();
    if (index < num_ar_choices)
    {
      const AspectRatio & option = options_.at(index);

      sel_ = index;
      emit selected(option);

      view_.requestUncache(this);
      view_.requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // AspectRatioItem::setNativeAspectRatio
  //
  void
  AspectRatioItem::setNativeAspectRatio(double ar)
  {
    native_ = ar;
  }

  //----------------------------------------------------------------
  // AspectRatioItem::setAspectRatio
  //
  void
  AspectRatioItem::setAspectRatio(double ar)
  {
    view_.requestUncache(this);
    view_.requestRepaint();

    std::size_t num_ar_choices = options_.size();
    std::size_t custom_choice = num_ar_choices;

    for (std::size_t i = 0; i < num_ar_choices; i++)
    {
      const AspectRatio & option = options_.at(i);

      if (option.category_ == AspectRatio::kOther)
      {
        // there should be only one:
        YAE_ASSERT(custom_choice == num_ar_choices);
        if (custom_choice == num_ar_choices)
        {
          custom_choice = i;
        }
      }
      else if (close_enough(option.ar_, ar, 1e-2))
      {
        sel_ = i;
        current_ = option.ar_ ? option.ar_ : native_;
        emit aspectRatio(ar);
        return;
      }
    }

    if (ar > 0.0)
    {
      sel_ = custom_choice;
      current_ = ar;
      emit aspectRatio(ar);
    }
  }

}
