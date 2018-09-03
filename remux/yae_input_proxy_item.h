// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Aug 18 11:25:26 MDT 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_INPUT_PROXY_ITEM_H_
#define YAE_INPUT_PROXY_ITEM_H_

// local:
#include "yaeInputArea.h"
#include "yaeItemFocus.h"


namespace yae
{

  //----------------------------------------------------------------
  // InputProxy
  //
  struct InputProxy : public InputArea
  {
    InputProxy(const char * id);
    ~InputProxy();

    // virtual:
    void onCancel();

    bool onMouseOver(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint);

    bool onScroll(const TVec2D & itemCSysOrigin,
                  const TVec2D & rootCSysPoint,
                  double degrees);

    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    bool onSingleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint);

    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint);

    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd);

    bool onDragEnd(const TVec2D & itemCSysOrigin,
                   const TVec2D & rootCSysDragStart,
                   const TVec2D & rootCSysDragEnd);

    bool draggable() const;

    void onFocus();
    void onFocusOut();

    bool processEvent(Canvas::ILayer & canvasLayer,
                      Canvas * canvas,
                      QEvent * event);

    yae::weak_ptr<InputArea, Item> ia_;
  };

}


#endif // YAE_INPUT_PROXY_ITEM_H_
