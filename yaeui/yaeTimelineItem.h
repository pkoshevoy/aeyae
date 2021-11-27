// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jun 17 19:29:05 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_TIMELINE_ITEM_H_
#define YAE_TIMELINE_ITEM_H_

// standard:
#include <list>

// Qt includes:
#include <QObject>

// local interfaces:
#include "yaeGradient.h"
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeTimelineModel.h"


namespace yae
{

  //----------------------------------------------------------------
  // TimelineHeight
  //
  struct YAEUI_API TimelineHeight : public TDoubleExpr
  {
    TimelineHeight(ItemView & view, Item & container, Item & timeline):
      view_(view),
      container_(container),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      int h = std::max<int>(2, ~1 & (int(0.5 + timeline_.height()) / 4));

      const std::list<VisibleItem> & items = view_.mouseOverItems();
      if (yae::find(items, container_) != items.end())
      {
        h *= 2;
      }

      result = double(h);
    }

    ItemView & view_;
    Item & container_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelineIn
  //
  struct YAEUI_API TimelineIn : public TDoubleExpr
  {
    TimelineIn(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerTimeIn();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelinePlayhead
  //
  struct YAEUI_API TimelinePlayhead : public TDoubleExpr
  {
    TimelinePlayhead(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerPlayhead();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // TimelineOut
  //
  struct YAEUI_API TimelineOut : public TDoubleExpr
  {
    TimelineOut(TimelineModel & model, Item & timeline):
      model_(model),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double x0 = timeline_.left();
      double w = timeline_.width();
      double t = model_.markerTimeOut();
      result = x0 + w * t;
    }

    TimelineModel & model_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // MarkerVisible
  //
  struct YAEUI_API MarkerVisible : public TBoolExpr
  {
    MarkerVisible(ItemView & view, Item & timeline):
      view_(view),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const TVec2D & pt = view_.mousePt();
      result = timeline_.overlaps(pt);
    }

    ItemView & view_;
    Item & timeline_;
  };


  //----------------------------------------------------------------
  // GetPlayheadAux
  //
  struct YAEUI_API GetPlayheadAux : public TVarExpr
  {
    GetPlayheadAux(TimelineModel & model):
      model_(model)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = QVariant(model_.auxPlayhead());
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // GetDurationAux
  //
  struct YAEUI_API GetDurationAux : public TVarExpr
  {
    GetDurationAux(TimelineModel & model):
      model_(model)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      result = QVariant(model_.auxDuration());
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // TimelineSeek
  //
  struct YAEUI_API TimelineSeek : public InputArea
  {
    TimelineSeek(TimelineModel & model):
      InputArea("timeline_seek"),
      model_(model)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      double x0 = this->left();
      double w = this->width();
      double x = rootCSysPoint.x() - itemCSysOrigin.x();
      double t = (x - x0) / w;
      model_.setMarkerPlayhead(t);
      return true;
    }

    TimelineModel & model_;
  };


  //----------------------------------------------------------------
  // SliderInPoint
  //
  struct YAEUI_API SliderInPoint : public InputArea
  {
    SliderInPoint(TimelineModel & model, Item & timeline):
      InputArea("slider_in_point"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerTimeIn();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerTimeIn(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // SliderPlayhead
  //
  struct YAEUI_API SliderPlayhead : public InputArea
  {
    SliderPlayhead(TimelineModel & model, Item & timeline):
      InputArea("slider_playhead"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerPlayhead();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerPlayhead(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // SliderOutPoint
  //
  struct YAEUI_API SliderOutPoint : public InputArea
  {
    SliderOutPoint(TimelineModel & model, Item & timeline):
      InputArea("slider_out_point"),
      model_(model),
      timeline_(timeline),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = model_.markerTimeOut();
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double t = dx / w;
      model_.setMarkerTimeOut(startPos_ + t);
      return true;
    }

    TimelineModel & model_;
    Item & timeline_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // TimelineConfig
  //
  class YAEUI_API TimelineItem : public QObject,
                                 public Item
  {
    Q_OBJECT;

  public:
    TimelineItem(const char * name, ItemView & view, TimelineModel & model);
    ~TimelineItem();

    void layout();

  public slots:
    void maybeAnimateOpacity();
    void maybeAnimateControls();
    void forceAnimateControls();

  public:
    // virtual:
    void uncache();

    // helper:
    void processMouseTracking(const TVec2D & pt);

  public slots:
    void modelChanged();
    void showTimeline(bool);
    void showPlaylist(bool);

  public:
    ItemView & view_;
    TimelineModel & model_;
    yae::shared_ptr<Gradient, Item> shadow_;

    ItemRef unit_size_;
    BoolRef is_playback_paused_;
    BoolRef is_fullscreen_;
    BoolRef is_playlist_visible_;
    BoolRef is_timeline_visible_;

    ContextQuery<bool> query_playlist_visible_;
    ContextQuery<bool> query_timeline_visible_;

    ContextCallback toggle_playback_;
    ContextCallback toggle_fullscreen_;
    ContextCallback toggle_playlist_;
    ContextCallback back_arrow_cb_;

    ContextCallback frame_crop_cb_;
    ContextCallback aspect_ratio_cb_;
    ContextCallback video_track_cb_;
    ContextCallback audio_track_cb_;
    ContextCallback subtt_track_cb_;
    ContextCallback back_to_prev_cb_;
    ContextCallback skip_to_next_cb_;
    ContextCallback select_all_cb_;
    ContextCallback remove_sel_cb_;
    ContextCallback delete_file_cb_;

    ItemView::TAnimatorPtr opacity_animator_;
    ItemView::TAnimatorPtr controls_animator_;
    Item::TObserverPtr animate_opacity_;
  };


  //----------------------------------------------------------------
  // AnimateOpacity
  //
  struct YAEUI_API AnimateOpacity : public Item::Observer
  {
    AnimateOpacity(TimelineItem & timeline):
      timeline_(timeline)
    {}

    // virtual:
    void observe(const Item & item, Item::Event e)
    {
      timeline_.maybeAnimateOpacity();
      timeline_.forceAnimateControls();
    }

    TimelineItem & timeline_;
  };

}


#endif // YAE_TIMELINE_ITEM_H_
