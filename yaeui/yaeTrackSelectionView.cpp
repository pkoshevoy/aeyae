// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Mar 15 14:22:25 MDT 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <math.h>

// yaeui:
#include "yaeInputArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"
#include "yaeTrackSelectionView.h"


namespace yae
{

  //----------------------------------------------------------------
  // OnDone
  //
  struct OnDone : public InputArea
  {
    OnDone(const char * id, ItemView & view):
      InputArea(id),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.setEnabled(false);
      return true;
    }

    ItemView & view_;
  };


  //----------------------------------------------------------------
  // TrackSelectionView::TrackSelectionView
  //
  TrackSelectionView::TrackSelectionView():
    ItemView("TrackSelectionView"),
    style_(NULL)
  {}

  //----------------------------------------------------------------
  // TrackSelectionView::setStyle
  //
  void
  TrackSelectionView::setStyle(ItemViewStyle * new_style)
  {
    style_ = new_style;

    // basic layout:
    TrackSelectionView & view = *this;
    const ItemViewStyle & style = *style_;
    Item & root = *root_;

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = root.addExpr(new GetViewWidth(view));
    root.height_ = root.addExpr(new GetViewHeight(view));

#if 0
    // setup mouse trap to prevent unintended click-through:
    MouseTrap & mouse_trap = root.addNew<MouseTrap>("mouse_trap");
    mouse_trap.anchors_.fill(root);
#endif

    Rectangle & bg = root.addNew<Rectangle>("bg");
    bg.anchors_.fill(root);
    bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::fg_, 0.9));

    Item & grid = root.addNew<Item>("grid");
    Item & footer = root.addNew<Item>("footer");
    grid.anchors_.fill(root);
    grid.anchors_.bottom_ = ItemRef::reference(footer, kPropertyTop);
    footer.anchors_.fill(root);
    footer.anchors_.top_.reset();
    footer.height_ = ItemRef::reference(style.title_height_, 3.0);

#if 0
    // dirty hacks to cache grid properties:
    Item & hidden = root.addHidden(new Item("hidden_grid_props"));
#endif

    RoundRect & bg_done = footer.addNew<RoundRect>("bg_done");
    Text & tx_done = footer.addNew<Text>("tx_done");

    tx_done.anchors_.center(footer);
    tx_done.text_ = TVarRef::constant(TVar("Done"));
    tx_done.color_ = tx_done.
      addExpr(style_color_ref(view, &ItemViewStyle::fg_));
    tx_done.background_ = tx_done.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.0));
    tx_done.fontSize_ = ItemRef::reference(style.title_height_);
    tx_done.elide_ = Qt::ElideNone;
    tx_done.setAttr("oneline", true);

    bg_done.anchors_.fill(tx_done, -7.0);
    bg_done.margins_.set_left(ItemRef::reference(style.title_height_, -1));
    bg_done.margins_.set_right(ItemRef::reference(style.title_height_, -1));
    bg_done.color_ = bg_done.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.3));
    bg_done.background_ = bg_done.
      addExpr(style_color_ref(view, &ItemViewStyle::fg_, 0.0));
    bg_done.radius_ = ItemRef::scale(bg_done, kPropertyHeight, 0.1);

    OnDone & on_done = bg_done.add(new OnDone("on_done", view));
    on_done.anchors_.fill(bg_done);
  }

  //----------------------------------------------------------------
  // TrackSelectionView::setTracks
  //
  void
  TrackSelectionView::setTracks(const std::vector<TTrackInfo> & tracks)
  {
    tracks_ = tracks;
  }

  //----------------------------------------------------------------
  // TrackSelectionView::processKeyEvent
  //
  bool
  TrackSelectionView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    e->ignore();

    QEvent::Type et = e->type();
    if (et == QEvent::KeyPress)
    {
      int key = e->key();

      if (key == Qt::Key_Return ||
          key == Qt::Key_Enter ||
          key == Qt::Key_Escape)
      {
        emit done();
        e->accept();
      }
    }

    return e->isAccepted();
  }

  //----------------------------------------------------------------
  // TrackSelectionView::setEnabled
  //
  void
  TrackSelectionView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);

    if (!enable)
    {
      emit done();
    }
    else
    {
      sync_ui();
    }
  }

  //----------------------------------------------------------------
  // TrackSelectionView::sync_ui
  //
  void
  TrackSelectionView::sync_ui()
  {
#if 0
    TrackSelectionView & view = *this;
    const ItemViewStyle & style = *style_;
    Item & root = *root_;
#endif
  }

}
