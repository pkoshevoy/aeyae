// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeInputArea.h"


namespace yae
{

  //----------------------------------------------------------------
  // InputArea::InputArea
  //
  InputArea::InputArea(const char * id):
    Item(id)
  {}

  //----------------------------------------------------------------
  // InputArea::getInputHandlers
  //
  void
  InputArea::getInputHandlers(// coordinate system origin of
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
    if (!Item::overlaps(itemCSysPoint))
    {
      return;
    }

    inputHandlers.push_back(InputHandler(this, itemCSysOrigin));
  }

  //----------------------------------------------------------------
  // InputArea::onCancel
  //
  void
  InputArea::onCancel()
  {
    if (onCancel_)
    {
      onCancel_->process(parent<Item>());
    }
  }

  //----------------------------------------------------------------
  // InputArea::onMouseOver
  //
  bool
  InputArea::onMouseOver(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
  {
    if (onMouseOver_)
    {
      return onMouseOver_->process(parent<Item>(),
                                   itemCSysOrigin,
                                   rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onScroll
  //
  bool
  InputArea::onScroll(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint,
                      double degrees)
  {
    if (onScroll_)
    {
      return onScroll_->process(parent<Item>(),
                                itemCSysOrigin,
                                rootCSysPoint,
                                degrees);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onPress
  //
  bool
  InputArea::onPress(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint)
  {
    if (onPress_)
    {
      return onPress_->process(parent<Item>(),
                               itemCSysOrigin,
                               rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onClick
  //
  bool
  InputArea::onClick(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint)
  {
    if (onClick_)
    {
      return onClick_->process(parent<Item>(),
                               itemCSysOrigin,
                               rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDoubleClick
  //
  bool
  InputArea::onDoubleClick(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint)
  {
    if (onDoubleClick_)
    {
      return onDoubleClick_->process(parent<Item>(),
                                     itemCSysOrigin,
                                     rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDrag
  //
  bool
  InputArea::onDrag(const TVec2D & itemCSysOrigin,
                    const TVec2D & rootCSysDragStart,
                    const TVec2D & rootCSysDragEnd)
  {
    if (onDrag_)
    {
      return onDrag_->process(parent<Item>(),
                              itemCSysOrigin,
                              rootCSysDragStart,
                              rootCSysDragEnd);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDragEnd
  //
  bool
  InputArea::onDragEnd(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysDragStart,
                       const TVec2D & rootCSysDragEnd)
  {
    if (onDragEnd_)
    {
      return onDragEnd_->process(parent<Item>(),
                                 itemCSysOrigin,
                                 rootCSysDragStart,
                                 rootCSysDragEnd);
    }

    return this->onDrag(itemCSysOrigin,
                        rootCSysDragStart,
                        rootCSysDragEnd);
  }
}
