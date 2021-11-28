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
  ConfirmView::ConfirmView(const char * name):
    ItemView(name),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // ConfirmView::~ConfirmView
  //
  ConfirmView::~ConfirmView()
  {
    TMakeCurrentContext currentContext(this->context().get());
    ConfirmView::clear();
    confirm_.reset();
  }

  //----------------------------------------------------------------
  // ConfirmView::setStyle
  //
  void
  ConfirmView::setStyle(ItemViewStyle * style)
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
    root.clear();

    if (enable)
    {
      confirm_.reset(new ConfirmItem("ConfirmItem", *this));
      ConfirmItem & confirm = root.add<ConfirmItem>(confirm_);
      confirm.anchors_.fill(root);
      confirm.fg_.set(fg_);
      confirm.bg_.set(bg_);
      confirm.message_.set(message_);
      confirm.affirmative_ = affirmative_;
      confirm.negative_ = negative_;
      confirm.layout();
      confirm.setVisible(true);
    }
    else if (confirm_)
    {
      confirm_->setVisible(false);
      confirm_.reset();
    }

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);
  }

}
