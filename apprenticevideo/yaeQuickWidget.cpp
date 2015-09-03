// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug  1 18:51:09 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt includes:
#include <QCursor>

// local includes:
#include "yaeQuickWidget.h"


namespace yae
{

  //----------------------------------------------------------------
  // TQuickWidget::TQuickWidget
  //
  TQuickWidget::TQuickWidget(QWidget * parent):
    QQuickWidget(parent)
  {
    timerHideCursor_.setSingleShot(true);
    timerHideCursor_.setInterval(3000);

    bool ok = true;
    ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                 this, SLOT(hideCursor()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // TQuickWidget::hideCursor
  //
  void
  TQuickWidget::hideCursor()
  {
    QQuickWidget::setCursor(QCursor(Qt::BlankCursor));
  }

  //----------------------------------------------------------------
  // TQuickWidget::dragEnterEvent
  //
  void
  TQuickWidget::dragEnterEvent(QDragEnterEvent * e)
  {
    QWidget * p = QQuickWidget::parentWidget();
    if (p)
    {
      // let the parent widget handle it:
      e->ignore();
    }
    else
    {
      QQuickWidget::dragEnterEvent(e);
    }
  }

  //----------------------------------------------------------------
  // TQuickWidget::dropEvent
  //
  void
  TQuickWidget::dropEvent(QDropEvent * e)
  {
    QWidget * p = QQuickWidget::parentWidget();
    if (p)
    {
      // let the parent widget handle it:
      e->ignore();
    }
    else
    {
      QQuickWidget::dropEvent(e);
    }
  }

  //----------------------------------------------------------------
  // TQuickWidget::mouseMoveEvent
  //
  void
  TQuickWidget::mouseMoveEvent(QMouseEvent * e)
  {
    timerHideCursor_.start();
    QQuickWidget::setCursor(QCursor(Qt::ArrowCursor));
    QQuickWidget::mouseMoveEvent(e);
  }
}
