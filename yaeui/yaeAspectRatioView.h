// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Mar  9 19:16:48 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASPECT_RATIO_VIEW_H_
#define YAE_ASPECT_RATIO_VIEW_H_

// Qt library:
#include <QObject>
#include <QString>

// local:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatioView
  //
  class YAEUI_API AspectRatioView : public ItemView
  {
    Q_OBJECT;

  public:
    AspectRatioView();

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    void setEnabled(bool enable);

    double getAspectRatio(std::size_t index) const;

    inline double nativeAspectRatio() const
    { return native_; }

  signals:
    void aspectRatio(double ar);
    void done();

  public slots:
    void selectAspectRatio(std::size_t index);
    void setNativeAspectRatio(double ar);
    void setAspectRatio(double ar);

  public:
    ColorRef fg_;
    ColorRef bg_;

  protected:
    ItemViewStyle * style_;

    // current selection:
    std::size_t sel_;
    double current_;
    double native_;
  };

}


#endif // YAE_ASPECT_RATIO_VIEW_H_
