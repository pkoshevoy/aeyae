// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Mar  9 19:16:48 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ASPECT_RATIO_VIEW_H_
#define YAE_ASPECT_RATIO_VIEW_H_

// local:
#include "yaeAspectRatioItem.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatioView
  //
  class YAEUI_API AspectRatioView : public ItemView
  {
  public:
    AspectRatioView(const char * name);

    void init(ItemViewStyle * style,
              const AspectRatio * options,
              std::size_t num_options);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // virtual:
    void setEnabled(bool enable);

    // accessor:
    inline AspectRatioItem * item() const
    { return aspectRatioItem_.get(); }

  protected:
    ItemViewStyle * style_;
    yae::shared_ptr<AspectRatioItem, Item> aspectRatioItem_;
  };

}


#endif // YAE_ASPECT_RATIO_VIEW_H_
