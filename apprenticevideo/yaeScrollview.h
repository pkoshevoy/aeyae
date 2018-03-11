// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SCROLLVIEW_H_
#define YAE_SCROLLVIEW_H_

// local interfaces:
#include "yaeCanvas.h"
#include "yaeExpression.h"
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeVec.h"


namespace yae
{
  // forward declarations:
  struct Scrollview;


  //----------------------------------------------------------------
  // CalcSliderTop
  //
  struct CalcSliderTop : public TDoubleExpr
  {
    CalcSliderTop(const Scrollview & view, const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderHeight
  //
  struct CalcSliderHeight : public TDoubleExpr
  {
    CalcSliderHeight(const Scrollview & view, const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderLeft
  //
  struct CalcSliderLeft : public TDoubleExpr
  {
    CalcSliderLeft(const Scrollview & view, const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderWidth
  //
  struct CalcSliderWidth : public TDoubleExpr
  {
    CalcSliderWidth(const Scrollview & view, const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // Scrollview
  //
  struct Scrollview : public Item
  {
    Scrollview(const char * id);

    // helper:
    void getContentView(TVec2D & origin,
                        Segment & xView,
                        Segment & yView) const;

    // virtual:
    void uncache();
    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const;
    void unpaint() const;

    // virtual:
    void getInputHandlers(// coordinate system origin of
                          // the input area, expressed in the
                          // coordinate system of the root item:
                          const TVec2D & itemCSysOrigin,

                          // point expressed in the coord. system of the item,
                          // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                          const TVec2D & itemCSysPoint,

                          // pass back input areas overlapping above point,
                          // along with its coord. system origin expressed
                          // in the coordinate system of the root item:
                          std::list<InputHandler> & inputHandlers);

#ifndef NDEBUG
    // virtual:
    void dump(std::ostream & os,
              const std::string & indent = std::string()) const;
#endif

    // item container:
    ItemPtr content_;

    // [0, 1] view position relative to content size
    // where 0 corresponds to the beginning of content
    // and 1 corresponds to the end of content
    TVec2D position_;
  };

  //----------------------------------------------------------------
  // SliderDrag
  //
  struct SliderDrag : public InputArea
  {
    SliderDrag(const char * id,
               const Canvas::ILayer & canvasLayer,
               Scrollview & scrollview,
               Item & scrollbar);

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd);

    const Canvas::ILayer & canvasLayer_;
    Scrollview & scrollview_;
    Item & scrollbar_;
    double startPos_;
  };


  //----------------------------------------------------------------
  // HorizontalScrollbarRequired
  //
  struct HorizontalScrollbarRequired : public TBoolExpr
  {
    HorizontalScrollbarRequired(const Scrollview & view,
                              const ItemRef & left,
                              const ItemRef & right):
      view_(view),
      left_(left),
      right_(right)
    {}

    void evaluate(bool & result) const
    {
      double x0 = left_.get();
      double x1 = right_.get();
      double viewWidth = x1 - x0;
      double sceneWidth = view_.content_->width();
      result = viewWidth < sceneWidth;
    }

    const Scrollview & view_;
    ItemRef left_;
    ItemRef right_;
  };

  //----------------------------------------------------------------
  // VerticalScrollbarRequired
  //
  struct VerticalScrollbarRequired : public TBoolExpr
  {
    VerticalScrollbarRequired(const Scrollview & view,
                              const ItemRef & top,
                              const ItemRef & bottom):
      view_(view),
      top_(top),
      bottom_(bottom)
    {}

    void evaluate(bool & result) const
    {
      double y0 = top_.get();
      double y1 = bottom_.get();
      double viewHeight = y1 - y0;
      double sceneHeight = view_.content_->height();
      result = viewHeight < sceneHeight;
    }

    const Scrollview & view_;
    ItemRef top_;
    ItemRef bottom_;
  };

}


#endif // YAE_SCROLLVIEW_H_
