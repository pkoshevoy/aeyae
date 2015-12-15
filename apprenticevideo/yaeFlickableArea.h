// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FLICKABLE_AREA_H_
#define YAE_FLICKABLE_AREA_H_

// boost includes:
#include <boost/shared_ptr.hpp>

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeCanvas.h"
#include "yaeInputArea.h"
#include "yaeVec.h"


namespace yae
{

  //----------------------------------------------------------------
  // FlickableArea
  //
  class FlickableArea : public QObject,
                        public InputArea
  {
    Q_OBJECT;

    FlickableArea(const FlickableArea &);
    FlickableArea & operator = (const FlickableArea &);

  public:
    FlickableArea(const char * id,
                  const Canvas::ILayer & canvasLayer,
                  Item & scrollbar);
    ~FlickableArea();

    // virtual:
    bool onScroll(const TVec2D & itemCSysOrigin,
                  const TVec2D & rootCSysPoint,
                  double degrees);

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd);

    // virtual:
    bool onDragEnd(const TVec2D & itemCSysOrigin,
                   const TVec2D & rootCSysDragStart,
                   const TVec2D & rootCSysDragEnd);

  protected slots:
    void onTimeout();

  protected:
    struct TPrivate;
    TPrivate * p_;
  };

}


#endif // YAE_FLICKABLE_AREA_H_
