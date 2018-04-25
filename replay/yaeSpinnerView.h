// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Apr 24 21:38:58 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SPINNER_VIEW_H_
#define YAE_SPINNER_VIEW_H_

// Qt library:
#include <QObject>

// local:
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // layout_clock_spinner
  //
  YAE_API TransitionItem &
  layout_clock_spinner(ItemView & view,
                       const ItemViewStyle & style,
                       Item & root);


  //----------------------------------------------------------------
  // SpinnerView
  //
  class YAE_API SpinnerView : public ItemView
  {
    Q_OBJECT;

  public:
    SpinnerView();

    void setStyle(const ItemViewStyle * style);

    // virtual:
    const ItemViewStyle * style() const
    { return style_; }

    // virtual:
    void setEnabled(bool enable);

 protected:
    const ItemViewStyle * style_;
    TAnimatorPtr animator_;
 };

}


#endif // YAE_SPINNER_VIEW_H_
