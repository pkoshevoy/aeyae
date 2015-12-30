// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Jul 26 12:49:35 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_QUICK_FBO_H_
#define YAE_CANVAS_QUICK_FBO_H_
#ifdef YAE_USE_PLAYER_QUICK_WIDGET

// apprenticevideo includes:
#include <yaeCanvas.h>

// Qt includes:
#include <QtQuick/QQuickFramebufferObject>


namespace yae
{
  //----------------------------------------------------------------
  // CanvasQuickFbo
  //
  class CanvasQuickFbo : public QQuickFramebufferObject
  {
    Q_OBJECT;

  public:
    CanvasQuickFbo();

    // virtual:
    QQuickFramebufferObject::Renderer * createRenderer() const;

    // the canvas does all the rendering, the renderer is just a wrapper for it:
    mutable Canvas canvas_;
  };
}


#endif // YAE_USE_PLAYER_QUICK_WIDGET
#endif // YAE_CANVAS_QUICK_FBO_H_
