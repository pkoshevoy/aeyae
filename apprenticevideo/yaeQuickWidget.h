// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug  1 18:42:14 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_QUICK_WIDGET_H_
#define YAE_QUICK_WIDGET_H_
#ifdef YAE_USE_PLAYER_QUICK_WIDGET

// Qt includes:
#include <QQuickWidget>
#include <QTimer>

// local includes:
#include "yaeCanvas.h"
#include "yaeScreenSaverInhibitor.h"


namespace yae
{

  //----------------------------------------------------------------
  // TQuickWidget
  //
  struct TQuickWidget : public QQuickWidget
  {
    Q_OBJECT;

  public:
    TQuickWidget(QWidget * parent = 0);

  public slots:
    void hideCursor();

   protected:
    // virtual:
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    void mouseMoveEvent(QMouseEvent * e);

    // a single shot timer for hiding the cursor:
    QTimer timerHideCursor_;
  };

}


#endif // YAE_USE_PLAYER_QUICK_WIDGET
#endif // YAE_QUICK_WIDGET_H_
