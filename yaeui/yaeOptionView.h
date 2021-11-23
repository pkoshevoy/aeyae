// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:18:55 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_OPTION_VIEW_H_
#define YAE_OPTION_VIEW_H_

// yaeui:
#include "yaeItemView.h"
#include "yaeOptionItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // OptionView
  //
  class YAEUI_API OptionView : public ItemView
  {
  public:
    OptionView(const char * name);

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

    // accessor:
    inline OptionItem * item() const
    { return optionItem_.get(); }

  protected:
    ItemViewStyle * style_;
    yae::shared_ptr<OptionItem, Item> optionItem_;
  };

}


#endif // YAE_OPTION_VIEW_H_
