// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jan 20 19:00:40 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CONFIRM_VIEW_H_
#define YAE_CONFIRM_VIEW_H_

// Qt library:
#include <QObject>
#include <QString>

// local:
#include "yaeConfirmItem.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // ConfirmView
  //
  class YAEUI_API ConfirmView : public ItemView
  {
    Q_OBJECT;

  public:
    ConfirmView();

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    void setEnabled(bool enable);

    yae::shared_ptr<ConfirmItem::Action> affirmative_;
    yae::shared_ptr<ConfirmItem::Action> negative_;

    TVarRef message_;
    ColorRef fg_;
    ColorRef bg_;

  protected:
    ItemViewStyle * style_;
    ConfirmItemPtr confirm_;
  };

}


#endif // YAE_CONFIRM_VIEW_H_
