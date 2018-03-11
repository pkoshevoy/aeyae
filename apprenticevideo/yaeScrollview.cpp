// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeScrollview.h"


namespace yae
{

  //----------------------------------------------------------------
  // CalcSliderTop::CalcSliderTop
  //
  CalcSliderTop::CalcSliderTop(const Scrollview & view, const Item & slider):
    view_(view),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderTop::evaluate
  //
  void
  CalcSliderTop::evaluate(double & result) const
  {
    result = view_.top();

    double sceneHeight = view_.content_->height();
    double viewHeight = view_.height();
    if (sceneHeight <= viewHeight)
    {
      return;
    }

    double scale = viewHeight / sceneHeight;
    double minHeight = slider_.width() * 5.0;
    double height = minHeight + (viewHeight - minHeight) * scale;
    double y = (viewHeight - height) * view_.position_.y();
    result += y;
  }


  //----------------------------------------------------------------
  // CalcSliderHeight::CalcSliderHeight
  //
  CalcSliderHeight::CalcSliderHeight(const Scrollview & view,
                                     const Item & slider):
    view_(view),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderHeight::evaluate
  //
  void
  CalcSliderHeight::evaluate(double & result) const
  {
    double sceneHeight = view_.content_->height();
    double viewHeight = view_.height();
    if (sceneHeight <= viewHeight)
    {
      result = viewHeight;
      return;
    }

    double scale = viewHeight / sceneHeight;
    double minHeight = slider_.width() * 5.0;
    result = minHeight + (viewHeight - minHeight) * scale;
  }


  //----------------------------------------------------------------
  // CalcSliderLeft::CalcSliderLeft
  //
  CalcSliderLeft::CalcSliderLeft(const Scrollview & view, const Item & slider):
    view_(view),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderLeft::evaluate
  //
  void
  CalcSliderLeft::evaluate(double & result) const
  {
    result = view_.top();

    double sceneWidth = view_.content_->width();
    double viewWidth = view_.width();
    if (sceneWidth <= viewWidth)
    {
      return;
    }

    double scale = viewWidth / sceneWidth;
    double minWidth = slider_.height() * 5.0;
    double width = minWidth + (viewWidth - minWidth) * scale;
    double x = (viewWidth - width) * view_.position_.x();
    result += x;
  }


  //----------------------------------------------------------------
  // CalcSliderWidth::CalcSliderWidth
  //
  CalcSliderWidth::CalcSliderWidth(const Scrollview & view,
                                   const Item & slider):
    view_(view),
    slider_(slider)
  {}

  //----------------------------------------------------------------
  // CalcSliderWidth::evaluate
  //
  void
  CalcSliderWidth::evaluate(double & result) const
  {
    double sceneWidth = view_.content_->width();
    double viewWidth = view_.width();
    if (sceneWidth <= viewWidth)
    {
      result = viewWidth;
      return;
    }

    double scale = viewWidth / sceneWidth;
    double minWidth = slider_.height() * 5.0;
    result = minWidth + (viewWidth - minWidth) * scale;
  }


  //----------------------------------------------------------------
  // Scrollview::Scrollview
  //
  Scrollview::Scrollview(const char * id):
    Item(id),
    content_(new Item("content"))
  {
    content_->self_ = content_;
  }

  //----------------------------------------------------------------
  // Scrollview::uncache
  //
  void
  Scrollview::uncache()
  {
    Item::uncache();
    content_->uncache();
  }

  //----------------------------------------------------------------
  // Scrollview::getContentView
  //
  void
  Scrollview::getContentView(TVec2D & origin,
                             Segment & xView,
                             Segment & yView) const
  {
    double sceneHeight = this->content_->height();
    double viewHeight = this->height();

    double sceneWidth = this->content_->width();
    double viewWidth = this->width();

    const Segment & xExtent = this->xExtent();
    const Segment & yExtent = this->yExtent();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = this->position_.y() * range;
    }

    double dx = 0.0;
    if (sceneWidth > viewWidth)
    {
      double range = sceneWidth - viewWidth;
      dx = this->position_.x() * range;
    }

    origin.x() = floor(xExtent.origin_ - dx);
    origin.y() = floor(yExtent.origin_ - dy);
    xView = Segment(dx, xExtent.length_);
    yView = Segment(dy, yExtent.length_);
  }

  //----------------------------------------------------------------
  // Scrollview::getInputHandlers
  //
  void
  Scrollview::getInputHandlers(// coordinate system origin of
                               // the input area, expressed in the
                               // coordinate system of the root item:
                               const TVec2D & itemCSysOrigin,

                               // point expressed in the coord.sys. of the item,
                               // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                               const TVec2D & itemCSysPoint,

                               // pass back input areas overlapping above point,
                               // along with its coord. system origin expressed
                               // in the coordinate system of the root item:
                               std::list<InputHandler> & inputHandlers)
  {
    Item::getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(origin, xView, yView);

    TVec2D ptInViewCoords = itemCSysPoint - origin;
    TVec2D offsetToView = itemCSysOrigin + origin;
    content_->getInputHandlers(offsetToView, ptInViewCoords, inputHandlers);
  }

  //----------------------------------------------------------------
  // Scrollview::paint
  //
  bool
  Scrollview::paint(const Segment & xregion,
                    const Segment & yregion,
                    Canvas * canvas) const
  {
    const Item & content = *content_;

    if (!Item::paint(xregion, yregion, canvas))
    {
      content.unpaint();
      return false;
    }

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(origin, xView, yView);

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(origin.x(), origin.y(), 0.0));
    content.paint(xView, yView, canvas);

    return true;
  }

  //----------------------------------------------------------------
  // Scrollview::unpaint
  //
  void
  Scrollview::unpaint() const
  {
    Item::unpaint();
    content_->unpaint();
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Scrollview::dump
  //
  void
  Scrollview::dump(std::ostream & os, const std::string & indent) const
  {
    Item::dump(os, indent);
    content_->dump(os, indent + "  ");
  }
#endif


  //----------------------------------------------------------------
  // SliderDrag::SliderDrag
  //
  SliderDrag::SliderDrag(const char * id,
                         const Canvas::ILayer & canvasLayer,
                         Scrollview & scrollview,
                         Item & scrollbar):
    InputArea(id),
    canvasLayer_(canvasLayer),
    scrollview_(scrollview),
    scrollbar_(scrollbar),
    startPos_(0.0)
  {}

  //----------------------------------------------------------------
  // SliderDrag::onPress
  //
  bool
  SliderDrag::onPress(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint)
  {
    bool vertical = scrollbar_.attr<bool>("vertical", true);
    startPos_ = vertical ?
      scrollview_.position_.y() :
      scrollview_.position_.x();
    return true;
  }

  //----------------------------------------------------------------
  // SliderDrag::onDrag
  //
  bool
  SliderDrag::onDrag(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysDragStart,
                     const TVec2D & rootCSysDragEnd)
  {
    bool vertical = scrollbar_.attr<bool>("vertical", true);
    double bz = vertical ? scrollbar_.height() : scrollbar_.width();
    double sz = vertical ? this->height() : this->width();
    double range = bz - sz;

    double dz = vertical ?
      rootCSysDragEnd.y() - rootCSysDragStart.y() :
      rootCSysDragEnd.x() - rootCSysDragStart.x();

    double ds = dz / range;
    double t = std::min<double>(1.0, std::max<double>(0.0, startPos_ + ds));

    if (vertical)
    {
      scrollview_.position_.set_y(t);
    }
    else
    {
      scrollview_.position_.set_x(t);
    }

    parent_->uncache();
    canvasLayer_.delegate()->requestRepaint();

    return true;
  }

  //----------------------------------------------------------------
  // scrollbars_required
  //
  ScrollbarId
  scrollbars_required(const Item & content,
                      const ItemRef & left,
                      const ItemRef & right,
                      const ItemRef & top,
                      const ItemRef & bottom,
                      const ItemRef & scrollbarWidth)
  {
    double z = scrollbarWidth.get();
    double sceneWidth = content.width();
    double sceneHeight = content.height();

    double x0 = left.get();
    double x1 = right.get();
    double viewWidth = x1 - x0;

    double y0 = top.get();
    double y1 = bottom.get();
    double viewHeight = y1 - y0;

    bool horizontal = viewWidth < sceneWidth;
    bool vertical = viewHeight < sceneHeight;

    if (horizontal)
    {
      viewHeight -= z;
    }

    if (vertical)
    {
      viewWidth -= z;
    }

    vertical = viewHeight < sceneHeight;
    horizontal = viewWidth < sceneWidth;

    int required = kScrollbarNone;

    if (vertical)
    {
      required |= kScrollbarVertical;
    }

    if (horizontal)
    {
      required |= kScrollbarHorizontal;
    }

    return (ScrollbarId)required;
  }
}
