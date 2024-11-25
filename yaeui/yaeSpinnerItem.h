// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Nov 13 09:40:52 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SPINNER_ITEM_H_
#define YAE_SPINNER_ITEM_H_

// aeyae:
#include "yae/api/yae_api.h"

// Qt:
#include <QObject>
#include <QString>

// yaeui:
#include "yaeItem.h"
#include "yaeItemView.h"
#include "yaeText.h"
#include "yaeTransform.h"


namespace yae
{

  //----------------------------------------------------------------
  // SpinnerItem
  //
  class YAEUI_API SpinnerItem : public Item
  {
  public:
    SpinnerItem(const char * id, ItemView & view);
    ~SpinnerItem();

    // call this after initializing the data references:
    void layout();

    // virtual:
    void setVisible(bool visible);

    // virtual:
    void uncache();

    ItemView & view_;
    ColorRef fg_;
    ColorRef bg_;
    ColorRef text_color_;
    TVarRef message_;
    ItemRef font_size_;

    TransitionItemPtr transition_;
    TTextPtr text_;

  protected:
    ItemView::TAnimatorPtr animator_;
  };

  //----------------------------------------------------------------
  // SpinnerItemPtr
  //
  typedef yae::shared_ptr<SpinnerItem, Item> SpinnerItemPtr;

}


#endif // YAE_SPINNER_ITEM_H_
