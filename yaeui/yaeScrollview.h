// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_SCROLLVIEW_H_
#define YAE_SCROLLVIEW_H_

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeCanvas.h"
#include "yaeExpression.h"
#include "yaeInputArea.h"
#include "yaeItem.h"
#include "yaeVec.h"


namespace yae
{
  // forward declarations:
  class ItemView;
  struct ItemViewStyle;
  struct Scrollview;


  //----------------------------------------------------------------
  // CalcSliderTop
  //
  struct YAEUI_API CalcSliderTop : public TDoubleExpr
  {
    CalcSliderTop(const Scrollview & view,
                  const Item & scrollbar,
                  const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & scrollbar_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderHeight
  //
  struct YAEUI_API CalcSliderHeight : public TDoubleExpr
  {
    CalcSliderHeight(const Scrollview & view,
                     const Item & scrollbar,
                     const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & scrollbar_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderLeft
  //
  struct YAEUI_API CalcSliderLeft : public TDoubleExpr
  {
    CalcSliderLeft(const Scrollview & view,
                   const Item & scrollbar,
                   const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & scrollbar_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // CalcSliderWidth
  //
  struct YAEUI_API CalcSliderWidth : public TDoubleExpr
  {
    CalcSliderWidth(const Scrollview & view,
                    const Item & scrollbar,
                    const Item & slider);

    // virtual:
    void evaluate(double & result) const;

    const Scrollview & view_;
    const Item & scrollbar_;
    const Item & slider_;
  };


  //----------------------------------------------------------------
  // Scrollview
  //
  struct YAEUI_API Scrollview : public Item
  {
    Scrollview(const char * id);

    // helper:
    void getContentView(TVec2D & origin,
                        Segment & xView,
                        Segment & yView) const;

    // virtual:
    void uncache();
    void uncacheSelf();
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

    // virtual:
    void getVisibleItems(// coordinate system origin of
                         // the item, expressed in the
                         // coordinate system of the root item:
                         const TVec2D & itemCSysOrigin,

                         // point expressed in the coord. system of the item,
                         // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                         const TVec2D & itemCSysPoint,

                         // pass back visible items overlapping above point,
                         // along with its coord. system origin expressed
                         // in the coordinate system of the root item:
                         std::list<VisibleItem> & visibleItems);

    // virtual:
    void get(Property property, double & value) const;

    // get visible content range according to current
    // scrollview position and size:
    void get_content_view_x(double & x, double & w) const;
    void get_content_view_y(double & y, double & h) const;

    // [0, 1] view position relative to content size
    // identifying an origin point within content:
    double content_origin_x() const;
    double content_origin_y() const;

    // [0, 1] view position relative to content size
    // where 0 corresponds to the beginning of content
    // and 1 corresponds to the end of content
    double position_x() const;
    double position_y() const;

    void set_position_x(double x);
    void set_position_y(double y);

#ifndef NDEBUG
    // virtual:
    void dump(std::ostream & os,
              const std::string & indent = std::string()) const;
#endif

    // item container:
    ItemPtr content_;

    // for scrollbar size reference lifetime management:
    ItemRef scrollbar_size_ref_;

    // optional scrollview position ...
    // NOTE: if defined then set_position_* are ignored
    ItemRef position_x_;
    ItemRef position_y_;

    // set to true to clip content that extends beyond the scroll view:
    bool clipContent_;

    // set to false if content doesn't need to be uncached
    // together with the scrollview:
    bool uncacheContent_;

  protected:
    double offset_x_;
    double offset_y_;
  };

  //----------------------------------------------------------------
  // SliderDrag
  //
  struct YAEUI_API SliderDrag : public InputArea
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
                                  const ItemRef & vscrollbarWidth,
                                  const ItemRef & hscrollbarWidth);

  //----------------------------------------------------------------
  // ScrollbarRequired
  //
  struct YAEUI_API ScrollbarRequired : public TBoolExpr
  {
    ScrollbarRequired(const Scrollview & scrollview,
                      const ScrollbarId scrollbarId,
                      const ItemRef & vscrollbarWidth,
                      const ItemRef & hscrollbarWidth,
                      const ItemRef & left,
                      const ItemRef & right,
                      const ItemRef & top,
                      const ItemRef & bottom):
      scrollview_(scrollview),
      scrollbarId_(scrollbarId),
      vscrollbarWidth_(vscrollbarWidth),
      hscrollbarWidth_(hscrollbarWidth),
      left_(left),
      right_(right),
      top_(top),
      bottom_(bottom)
    {
      YAE_ASSERT(scrollbarId_ != kScrollbarNone);
      YAE_ASSERT(!(vscrollbarWidth_.isCacheable() ||
                   hscrollbarWidth_.isCacheable() ||
                   left_.isCacheable() ||
                   right_.isCacheable() ||
                   top_.isCacheable() ||
                   bottom_.isCacheable()));
    }

    void evaluate(bool & result) const
    {
      ScrollbarId required = scrollbars_required(*(scrollview_.content_),
                                                 left_,
                                                 right_,
                                                 top_,
                                                 bottom_,
                                                 vscrollbarWidth_,
                                                 hscrollbarWidth_);
      result = (scrollbarId_ & required) != 0;
    }

    const Scrollview & scrollview_;
    ScrollbarId scrollbarId_;
    ItemRef vscrollbarWidth_;
    ItemRef hscrollbarWidth_;
    ItemRef left_;
    ItemRef right_;
    ItemRef top_;
    ItemRef bottom_;
  };

  //----------------------------------------------------------------
  // layout_scrollview
  //
  YAEUI_API Scrollview &
  layout_scrollview(ScrollbarId scrollbars,
                    ItemView & view,
                    const ItemViewStyle & style,
                    Item & root,
                    const ItemRef & scrollbar_size_ref,
                    ScrollbarId inset = kScrollbarNone,
                    bool clipContent = true);

  //----------------------------------------------------------------
  // layout_scrollview
  //
  YAEUI_API Scrollview &
  layout_scrollview(ScrollbarId scrollbars,
                    ItemView & view,
                    const ItemViewStyle & style,
                    Item & root,
                    ScrollbarId inset = kScrollbarNone,
                    bool clipContent = true);

  //----------------------------------------------------------------
  // layout_scrollview
  //
  YAEUI_API Scrollview &
  layout_scrollview(ItemView & view,
                    Item & root,
                    ScrollbarId scroll = kScrollbarBoth,
                    bool clipContent = true);

  //----------------------------------------------------------------
  // get_scrollview
  //
  YAEUI_API Scrollview & get_scrollview(Item & root);

}


#endif // YAE_SCROLLVIEW_H_
