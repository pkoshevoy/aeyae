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
  // ScrollbarId
  //
  enum ScrollbarId
  {
    kScrollbarNone = 0,
    kScrollbarVertical = 1,
    kScrollbarHorizontal = 2,
    kScrollbarBoth = 3
  };

  //----------------------------------------------------------------
  // scrollbars_required
  //
  ScrollbarId scrollbars_required(const Item & content,
                                  const ItemRef & left,
                                  const ItemRef & right,
                                  const ItemRef & top,
                                  const ItemRef & bottom,
                                  const ItemRef & scrollbarWidth);

  //----------------------------------------------------------------
  // ScrollbarRequired
  //
  struct ScrollbarRequired : public TBoolExpr
  {
    ScrollbarRequired(const Item & content,
                      const ScrollbarId scrollbarId,
                      const ItemRef & scrollbarWidth,
                      const ItemRef & left,
                      const ItemRef & right,
                      const ItemRef & top,
                      const ItemRef & bottom):
      content_(content),
      scrollbarId_(scrollbarId),
      scrollbarWidth_(scrollbarWidth),
      left_(left),
      right_(right),
      top_(top),
      bottom_(bottom)
    {
      YAE_ASSERT(scrollbarId_ != kScrollbarNone);
    }

    void evaluate(bool & result) const
    {
      ScrollbarId required = scrollbars_required(content_,
                                                 left_,
                                                 right_,
                                                 top_,
                                                 bottom_,
                                                 scrollbarWidth_);
      result = (scrollbarId_ & required) != 0;
    }

    const Item & content_;
    ScrollbarId scrollbarId_;
    ItemRef scrollbarWidth_;
    ItemRef left_;
    ItemRef right_;
    ItemRef top_;
    ItemRef bottom_;
  };

}


#endif // YAE_SCROLLVIEW_H_
