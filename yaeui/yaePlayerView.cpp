// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:07:25 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QApplication>
#include <QProcess>

// yaeui:
#include "yaePlayerStyle.h"
#include "yaePlayerView.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerView::PlayerView
  //
  PlayerView::PlayerView(const char * name):
    ItemView(name)
  {
    style_.reset(new PlayerStyle("player_style", *this));
  }

  //----------------------------------------------------------------
  // PlayerView::~PlayerView
  //
  PlayerView::~PlayerView()
  {
    player_ux_.reset();
    style_.reset();
  }

  //----------------------------------------------------------------
  // PlayerView::setStyle
  //
  void
  PlayerView::setStyle(const yae::shared_ptr<ItemViewStyle, Item> & style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // PlayerView::setContext
  //
  void
  PlayerView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    ItemView::setContext(context);
    player_ux_.reset(new PlayerUxItem("player_ux", *this));
  }

  //----------------------------------------------------------------
  // PlayerView::setEnabled
  //
  void
  PlayerView::setEnabled(bool enable)
  {
    YAE_ASSERT(style_);

    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.children_.clear();

    if (enable)
    {
      layout(*this, *style_, root);
      player_ux_->timelineTimer_.start(1000);
    }
    else
    {
      player_ux_->timelineTimer_.stop();
    }

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // PlayerView::processEvent
  //
  bool
  PlayerView::processEvent(Canvas * canvas, QEvent * e)
  {
    QEvent::Type et = e->type();

    if (player_ux_ && player_ux_->processEvent(*this, canvas, e))
    {
      return true;
    }

    return ItemView::processEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // PlayerView::processKeyEvent
  //
  bool
  PlayerView::processKeyEvent(Canvas * canvas, QKeyEvent * event)
  {
    if (ItemView::processKeyEvent(canvas, event))
    {
      return true;
    }

    if (player_ux_ && player_ux_->processKeyEvent(canvas, event))
    {
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlayerView::processWheelEvent
  //
  bool
  PlayerView::processWheelEvent(Canvas * canvas, QWheelEvent * e)
  {
    if (player_ux_ && player_ux_->processWheelEvent(canvas, e))
    {
      return true;
    }

    // ignore it:
    return ItemView::processWheelEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // PlayerView::processMouseTracking
  //
  bool
  PlayerView::processMouseTracking(const TVec2D & mousePt)
  {
    if (player_ux_ && player_ux_->processMouseTracking(mousePt))
    {
      return true;
    }

    // ignore it:
    return ItemView::processMouseTracking(mousePt);
  }

  //----------------------------------------------------------------
  // PlayerView::layout
  //
  void
  PlayerView::layout(PlayerView & view,
                     const ItemViewStyle & style,
                     Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
    hidden.width_ = hidden.
      addExpr(style_item_ref(view, &ItemViewStyle::unit_size_));

    // add style to root item, so that it could be uncached
    // together with all the view:
    root.addHidden(view.style_);

    PlayerUxItem & player_ux = root.add(view.player_ux_);
    player_ux.anchors_.fill(root);
  }

}
