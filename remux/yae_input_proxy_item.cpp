// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Aug 18 11:47:27 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_log.h"

// local:
#include "yae_input_proxy_item.h"


namespace yae
{

  //----------------------------------------------------------------
  // InputProxy::InputProxy
  //
  InputProxy::InputProxy(const char * id):
    InputArea(id)
  {}

  //----------------------------------------------------------------
  // InputProxy::~InputProxy
  //
  InputProxy::~InputProxy()
  {
    ItemFocus::singleton().removeFocusable(this);
  }

  //----------------------------------------------------------------
  // InputProxy::onCancel
  //
  void
  InputProxy::onCancel()
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      ia->onCancel();
    }
  }

  //----------------------------------------------------------------
  // InputProxy::onMouseOver
  //
  bool
  InputProxy::onMouseOver(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onMouseOver(itemCSysOrigin, rootCSysPoint);
    }

    return InputArea::onMouseOver(itemCSysOrigin, rootCSysPoint);
  }

  //----------------------------------------------------------------
  // InputProxy::onScroll
  //
  bool
  InputProxy::onScroll(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint,
                       double degrees)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onScroll(itemCSysOrigin, rootCSysPoint, degrees);
    }

    return InputArea::onScroll(itemCSysOrigin, rootCSysPoint, degrees);
  }

  //----------------------------------------------------------------
  // InputProxy::onPress
  //
  bool
  InputProxy::onPress(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      if (!ia->onPress(itemCSysOrigin, rootCSysPoint))
      {
        return false;
      }

      if (!ItemFocus::singleton().hasFocus(this))
      {
        ItemFocus::singleton().setFocus(this);
      }

      return true;
    }

    return InputArea::onPress(itemCSysOrigin, rootCSysPoint);
  }

  //----------------------------------------------------------------
  // InputProxy::onClick
  //
  bool
  InputProxy::onClick(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onClick(itemCSysOrigin, rootCSysPoint);
    }

    return InputArea::onClick(itemCSysOrigin, rootCSysPoint);
  }

  //----------------------------------------------------------------
  // InputProxy::onSingleClick
  //
  bool
  InputProxy::onSingleClick(const TVec2D & itemCSysOrigin,
                            const TVec2D & rootCSysPoint)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onSingleClick(itemCSysOrigin, rootCSysPoint);
    }

    return InputArea::onSingleClick(itemCSysOrigin, rootCSysPoint);
  }

  //----------------------------------------------------------------
  // InputProxy::onDoubleClick
  //
  bool
  InputProxy::onDoubleClick(const TVec2D & itemCSysOrigin,
                            const TVec2D & rootCSysPoint)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onDoubleClick(itemCSysOrigin, rootCSysPoint);
    }

    return InputArea::onDoubleClick(itemCSysOrigin, rootCSysPoint);
  }

  //----------------------------------------------------------------
  // InputProxy::onDrag
  //
  bool
  InputProxy::onDrag(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysDragStart,
                     const TVec2D & rootCSysDragEnd)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onDrag(itemCSysOrigin, rootCSysDragStart, rootCSysDragEnd);
    }

    return InputArea::onDrag(itemCSysOrigin,
                             rootCSysDragStart,
                             rootCSysDragEnd);
  }

  //----------------------------------------------------------------
  // InputProxy::onDragEnd
  //
  bool
  InputProxy::onDragEnd(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->onDragEnd(itemCSysOrigin, rootCSysDragStart, rootCSysDragEnd);
    }

    return InputArea::onDragEnd(itemCSysOrigin,
                                rootCSysDragStart,
                                rootCSysDragEnd);
  }

  //----------------------------------------------------------------
  // InputProxy::draggable
  //
  bool
  InputProxy::draggable() const
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->draggable();
    }

    return InputArea::draggable();
  }

  //----------------------------------------------------------------
  // InputProxy::onFocus
  //
  void
  InputProxy::onFocus()
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      ia->onFocus();
      return;
    }

    InputArea::onFocus();
  }

  //----------------------------------------------------------------
  // InputProxy::onFocusOut
  //
  void
  InputProxy::onFocusOut()
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      ia->onFocusOut();
      return;
    }

    InputArea::onFocusOut();
  }

  //----------------------------------------------------------------
  // InputProxy::processEvent
  //
  bool
  InputProxy::processEvent(Canvas::ILayer & canvasLayer,
                           Canvas * canvas,
                           QEvent * event)
  {
    yae::shared_ptr<InputArea, Item> ia = ia_.lock();
    if (ia)
    {
      return ia->processEvent(canvasLayer, canvas, event);
    }

    return InputArea::processEvent(canvasLayer, canvas, event);
  }

}
