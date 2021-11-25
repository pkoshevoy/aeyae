// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:05:08 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_VIEW_H_
#define YAE_PLAYER_VIEW_H_

// yaeui:
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaePlayerUxItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerView
  //
  class YAEUI_API PlayerView : public ItemView
  {
  public:
    PlayerView(const char * name);
    ~PlayerView();

    // virtual:
    void clear();

    inline PlayerUxItem * player_ux() const
    { return player_ux_.get(); }

    void setStyle(const yae::shared_ptr<ItemViewStyle, Item> & style);

    // virtual:
    ItemViewStyle * style() const
    { return style_.get(); }

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // virtual:
    void setEnabled(bool enable);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);
    bool processWheelEvent(Canvas * canvas, QWheelEvent * event);
    bool processMouseTracking(const TVec2D & mousePt);

  protected:
    void layout(PlayerView & view, const ItemViewStyle & style, Item & root);

  public:
    // items:
    yae::shared_ptr<ItemViewStyle, Item> style_;
    yae::shared_ptr<PlayerUxItem, Item> player_ux_;
  };

}


#endif // YAE_PLAYER_VIEW_H_
