// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Nov 26 08:57:50 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_VIEW_H_
#define YAE_MAIN_VIEW_H_

// Qt includes:
#include <QObject>

// yaeui:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"


namespace yae
{

  //----------------------------------------------------------------
  // MainView
  //
  class MainView : public ItemView
  {
    Q_OBJECT;

  public:
    MainView();

    void setStyle(ItemViewStyle * style);

    // virtual:
    ItemViewStyle * style() const
    { return style_; }

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // accessor:
    inline TransitionItem & get_opacity_item()
    { return *opacity_; }

    // helpers:
    void maybe_animate_opacity();
    void force_animate_opacity();

  protected:
    ItemViewStyle * style_;
    yae::shared_ptr<Item> hidden_;
    yae::shared_ptr<Item> mouse_detect_;
    yae::shared_ptr<TransitionItem, Item> opacity_;
    ItemView::TAnimatorPtr opacity_animator_;

  public:
    ContextCallback toggle_playback_;
    ContextQuery<bool> is_playback_paused_;
  };

}


#endif // YAE_MAIN_VIEW_H_
