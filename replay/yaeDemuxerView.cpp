// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:24:38 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>

// local:
#include "yaeDemuxerView.h"
#include "yaeFlickableArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTextInput.h"


namespace yae
{

  //----------------------------------------------------------------
  // IsMouseOver
  //
  struct IsMouseOver : public TBoolExpr
  {
    IsMouseOver(const ItemView & view,
                const Scrollview & sview,
                const Item & item):
      view_(view),
      sview_(sview),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      TVec2D origin;
      Segment xView;
      Segment yView;
      sview_.getContentView(origin, xView, yView);

      const TVec2D & pt = view_.mousePt();
      TVec2D lcs_pt = pt - origin;
      result = item_.overlaps(lcs_pt);
    }

    const ItemView & view_;
    const Scrollview & sview_;
    const Item & item_;
  };

  //----------------------------------------------------------------
  // ClearTextInput
  //
  struct ClearTextInput : public InputArea
  {
    ClearTextInput(const char * id, TextInput & edit, Text & view):
      InputArea(id),
      edit_(edit),
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
      edit_.setText(QString());
      edit_.uncache();
      view_.uncache();
      return true;
    }

    TextInput & edit_;
    Text & view_;
  };


  //----------------------------------------------------------------
  // GetFontSize
  //
  struct GetFontSize : public TDoubleExpr
  {
    GetFontSize(const Item & titleHeight, double titleHeightScale,
                const Item & cellHeight, double cellHeightScale):
      titleHeight_(titleHeight),
      cellHeight_(cellHeight),
      titleHeightScale_(titleHeightScale),
      cellHeightScale_(cellHeightScale)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double t = 0.0;
      titleHeight_.get(kPropertyHeight, t);
      t *= titleHeightScale_;

      double c = 0.0;
      cellHeight_.get(kPropertyHeight, c);
      c *= cellHeightScale_;

      result = std::min(t, c);
    }

    const Item & titleHeight_;
    const Item & cellHeight_;

    double titleHeightScale_;
    double cellHeightScale_;
  };


  //----------------------------------------------------------------
  // FrameColor
  //
  struct FrameColor : public TColorExpr
  {
    FrameColor(const Clip & clip,
               const Timespan & span,
               const Color & drop,
               const Color & keep):
      clip_(clip),
      span_(span),
      drop_(drop),
      keep_(keep)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool selected = clip_.keep_.contains(span_.t1_);
      result = selected ? keep_ : drop_;
    }

    const Clip & clip_;
    Timespan span_;
    Color drop_;
    Color keep_;
  };

  //----------------------------------------------------------------
  // layout_gop
  //
  static void
  layout_gop(const Clip & clip,
             const Timeline::Track & track,
             RemuxView & view,
             const RemuxViewStyle & style,
             Item & root,
             std::size_t i0,
             std::size_t i1)
  {
    const Item * prev = NULL;
    for (std::size_t i = i0; i < i1; ++i)
    {
      RoundRect & frame = root.addNew<RoundRect>("frame");
      frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      frame.anchors_.left_ = prev ?
        frame.addExpr(new OddRoundUp(*prev, kPropertyRight)) :
        ItemRef::reference(root, kPropertyLeft, 0.0, 1);

      frame.height_ = ItemRef::reference(style.title_height_, 0.9);
      frame.width_ = ItemRef::reference(frame.height_, 16.0 / 9.0);
      frame.radius_ = ItemRef::constant(3);

      frame.background_ = frame.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));

      Timespan span(track.dts_[i], track.pts_[i] + track.dur_[i]);
      frame.color_ = frame.
        addExpr(new FrameColor(clip, span,
                               style.scrollbar_.get(),
                               style.cursor_.get()));

      prev = &frame;
    }
  }

  //----------------------------------------------------------------
  // RemuxLayoutGops
  //
  struct RemuxLayoutGops : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
    {
      if (model.remux_.size() <= model.current_)
      {
        return;
      }

      const Clip & clip = *(model.remux_[model.current_]);

      const Timeline::Track & track =
        clip.summary_.get_track_timeline(clip.track_);

      const Item * prev = NULL;
      for (std::set<std::size_t>::const_iterator
             i = track.keyframes_.begin(); i != track.keyframes_.end(); ++i)
      {
        Item & gop = root.addNew<Item>("gop");
        gop.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        gop.anchors_.top_ = prev ?
          gop.addExpr(new OddRoundUp(*prev, kPropertyBottom)) :
          ItemRef::reference(root, kPropertyTop, 0.0, 1);

        std::set<std::size_t>::const_iterator i1 = i;
        std::advance(i1, 1);

        layout_gop(clip, track, view, style, gop,
                   *i, i1 == track.keyframes_.end() ? track.dts_.size() : *i1);
        prev = &gop;
      }
    }
  };

  //----------------------------------------------------------------
  // layout_clip
  //
  static void
  layout_clip(RemuxModel & model,
              RemuxView & view,
              const RemuxViewStyle & style,
              Item & root,
              std::size_t index)
  {
    root.height_ = ItemRef::reference(style.row_height_);

    const std::size_t num_clips = model.remux_.size();
    if (index == num_clips)
    {
      RoundRect & btn = root.addNew<RoundRect>("append");
      btn.border_ = ItemRef::constant(1.0);
      btn.radius_ = ItemRef::constant(3.0);
      btn.width_ = ItemRef::reference(root.height_, 0.8);
      btn.height_ = btn.width_;
      btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
      btn.anchors_.left_ = ItemRef::reference(root.height_, 1.6);
      btn.color_ = btn.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

      Text & label = btn.addNew<Text>("label");
      label.anchors_.center(btn);
      label.text_ = TVarRef::constant(TVar("+"));
      return;
    }

    RoundRect & btn = root.addNew<RoundRect>("remove");
    btn.border_ = ItemRef::constant(1.0);
    btn.radius_ = ItemRef::constant(3.0);
    btn.width_ = ItemRef::reference(root.height_, 0.8);
    btn.height_ = btn.width_;
    btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
    btn.anchors_.left_ = ItemRef::reference(root.height_, 0.6);
    btn.color_ = btn.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

    Text & label = btn.addNew<Text>("label");
    label.anchors_.center(btn);
    label.text_ = TVarRef::constant(TVar("-"));
  }

  //----------------------------------------------------------------
  // RemuxLayoutClips
  //
  struct RemuxLayoutClips : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
    {
      const Item * prev = NULL;
      for (std::size_t i = 0; i <= model.remux_.size(); ++i)
      {
        Rectangle & row = root.addNew<Rectangle>("clip");
        row.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
        row.anchors_.top_ = prev ?
          ItemRef::reference(*prev, kPropertyBottom) :
          ItemRef::reference(root, kPropertyTop);
        row.color_ = row.addExpr(style_color_ref
                                 (view,
                                  &ItemViewStyle::bg_controls_,
                                  (i % 2) ? 1.0 : 0.5));

        layout_clip(model, view, style, row, i);
        prev = &row;
      }
    }
  };

  //----------------------------------------------------------------
  // RemuxLayout
  //
  struct RemuxLayout : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
    {
      Rectangle & bg = root.addNew<Rectangle>("background");
      bg.anchors_.fill(root);
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_));

      Item & gops = root.addNew<Item>("gops");
      Item & clips = root.addNew<Item>("clips");

      Rectangle & sep = root.addNew<Rectangle>("separator");
      sep.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      sep.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      // sep.anchors_.vcenter_ = ItemRef::scale(root, kPropertyHeight, 0.75);
      sep.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom, -100);
      sep.height_ = ItemRef::reference(style.title_height_, 0.1);
      sep.color_ = sep.addExpr(style_color_ref(view, &ItemViewStyle::fg_));

      gops.anchors_.fill(root);
      gops.anchors_.bottom_ = ItemRef::reference(sep, kPropertyTop);

      clips.anchors_.fill(root);
      clips.anchors_.top_ = ItemRef::reference(sep, kPropertyBottom);

      Item & gops_container =
        layout_scrollview(kScrollbarBoth, view, style, gops,
                          kScrollbarBoth);

      Item & clips_container =
        layout_scrollview(kScrollbarVertical, view, style, clips);

      style.layout_clips_->layout(clips_container, view, model, style);
      style.layout_gops_->layout(gops_container, view, model, style);
    }
  };


  //----------------------------------------------------------------
  // RemuxViewStyle::RemuxViewStyle
  //
  RemuxViewStyle::RemuxViewStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view),
    layout_root_(new RemuxLayout()),
    layout_clips_(new RemuxLayoutClips()),
    layout_gops_(new RemuxLayoutGops())
  {
    row_height_ = ItemRef::reference(title_height_, 0.55);
  }

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView():
    ItemView("RemuxView"),
    style_("RemuxViewStyle", *this),
    model_(NULL)
  {}

  //----------------------------------------------------------------
  // RemuxView::setModel
  //
  void
  RemuxView::setModel(RemuxModel * model)
  {
    if (model_ == model)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!model_);

    model_ = model;

    // connect new model:
    /*
    bool ok = true;

    ok = connect(model_, SIGNAL(layoutChanged()),
                 this, SLOT(layoutChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(dataChanged()),
                 this, SLOT(dataChanged()));
    YAE_ASSERT(ok);
    */

    /*
    Item & root = *root_;
    Scrollview & sview = root.get<Scrollview>("scrollview");
    Item & sviewContent = *(sview.content_);
    Item & footer = sviewContent["footer"];
    */
  }

  //----------------------------------------------------------------
  // RemuxView::processMouseEvent
  //
  bool
  RemuxView::processMouseEvent(Canvas * canvas, QMouseEvent * e)
  {
    bool processed = ItemView::processMouseEvent(canvas, e);
    /*
    QEvent::Type et = e->type();
    if (et == QEvent::MouseButtonPress &&
        (e->button() == Qt::LeftButton) &&
        !inputHandlers_.empty())
    {
      const TModelInputArea * ia = findModelInputArea(inputHandlers_);
      if (ia)
      {
        QModelIndex index = ia->modelIndex();
        int groupRow = -1;
        int itemRow = -1;
        RemuxModel::mapToGroupRowItemRow(index, groupRow, itemRow);

        if (!(groupRow < 0 || itemRow < 0))
        {
          // update selection:
          SelectionFlags selectionFlags = get_selection_flags(e);
          select_items(*model_, groupRow, itemRow, selectionFlags);
        }
      }
    }
    */
    return processed;
  }

  //----------------------------------------------------------------
  // RemuxView::resizeTo
  //
  bool
  RemuxView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }
    /*
    if (model_)
    {
      QModelIndex currentIndex = model_->currentItem();

      Item & root = *root_;
      Scrollview & sview = root.get<Scrollview>("scrollview");
      FlickableArea & ma_sview = sview.get<FlickableArea>("ma_sview");

      if (!ma_sview.isAnimating())
      {
        ensureVisible(currentIndex);
      }
    }
    */
    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::layoutChanged
  //
  void
  RemuxView::layoutChanged()
  {
#if 0 // ndef NDEBUG
    std::cerr << "RemuxView::layoutChanged" << std::endl;
#endif

    TMakeCurrentContext currentContext(*context());

    if (pressed_)
    {
      if (dragged_)
      {
        InputArea * ia = dragged_->inputArea();
        if (ia)
        {
          ia->onCancel();
        }

        dragged_ = NULL;
      }

      InputArea * ia = pressed_->inputArea();
      if (ia)
      {
        ia->onCancel();
      }

      pressed_ = NULL;
    }
    inputHandlers_.clear();

    Item & root = *root_;
    root.children_.clear();
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);
    root.uncache();
    uncache_.clear();

    style_.layout_root_->layout(root, *this, *model_, style_);

#ifndef NDEBUG
    root.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // RemuxView::dataChanged
  //
  void
  RemuxView::dataChanged()
  {
    requestUncache();
    requestRepaint();
  }

}
