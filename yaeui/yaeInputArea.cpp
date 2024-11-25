// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_api.h"

// yaeui:
#include "yaeInputArea.h"
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // InputArea::InputArea
  //
  InputArea::InputArea(const char * id, bool draggable, uint32_t buttons):
    Item(id),
    draggable_(draggable),
    allowed_buttons_(buttons),
    pressed_button_(kNoButtons)
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

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);
    }
  }

  //----------------------------------------------------------------
  // InputArea::onCancel
  //
  void
  InputArea::onCancel()
  {
    pressed_button_ = kNoButtons;
  }

  //----------------------------------------------------------------
  // InputArea::onDragEnd
  //
  bool
  InputArea::onDragEnd(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysDragStart,
                       const TVec2D & rootCSysDragEnd)
  {
    return this->onDrag(itemCSysOrigin,
                        rootCSysDragStart,
                        rootCSysDragEnd);
  }

  //----------------------------------------------------------------
  // InputArea::postponeSingleClickEvent
  //
  yae::shared_ptr<CancelableEvent::Ticket>
  InputArea::postponeSingleClickEvent(PostponeEvent & postponeEvent,
                                      int msec,
                                      QObject * view,
                                      const TVec2D & itemCSysOrigin,
                                      const TVec2D & rootCSysPoint) const
  {
    yae::shared_ptr<CancelableEvent::Ticket>
      ticket(new CancelableEvent::Ticket());

    ItemPtr itemPtr = self_.lock();
    InputAreaPtr inputAreaPtr = itemPtr.cast<InputArea>();

    postponeEvent.postpone(msec, view, new SingleClickEvent(ticket,
                                                            inputAreaPtr,
                                                            itemCSysOrigin,
                                                            rootCSysPoint));
    return ticket;
  }

}
