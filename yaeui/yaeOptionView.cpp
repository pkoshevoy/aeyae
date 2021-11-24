// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:22:25 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// yaeui:
#include "yaeOptionView.h"


namespace yae
{

  //----------------------------------------------------------------
  // OptionView::OptionView
  //
  OptionView::OptionView(const char * name):
    ItemView(name),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // OptionView::setStyle
  //
  void
  OptionView::setStyle(ItemViewStyle * new_style)
  {
    style_ = new_style;
    optionItem_.reset(new OptionItem("OptionItem", *this));

    Item & root = *root_;
    root.clear();

    OptionItem & optionItem = root.add<OptionItem>(optionItem_);
    optionItem.anchors_.fill(root);
  }


  //----------------------------------------------------------------
  // OptionView::processKeyEvent
  //
  bool
  OptionView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    if (this->isEnabled() && optionItem_)
    {
      return optionItem_->processKeyEvent(canvas, e);
    }

    return ItemView::processKeyEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // OptionView::setEnabled
  //
  void
  OptionView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable || !optionItem_)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);

    optionItem_->setVisible(enable);
  }

}
