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
#include <QString>

// local:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // SpinnerView
  //
  class YAEUI_API SpinnerView : public ItemView
  {
    Q_OBJECT;

  public:
    SpinnerView();

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    void setEnabled(bool enable);

    // helper:
    inline void setText(const std::string & text)
    { setText(QString::fromUtf8(text.c_str())); }

    void setText(const QString & text);

    inline const QString & text() const
    { return text_; }

  protected:
    ItemViewStyle * style_;
    TAnimatorPtr animator_;
    QString text_;
 };

}


#endif // YAE_SPINNER_VIEW_H_
