// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Feb  2 21:29:58 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QCloseEvent>

// yaeui:
#include "yaeAppleUtils.h"
#include "yaePlayerWidget.h"

// local:
#include "yaePlayerWindow.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerWindow::PlayerWindow
  //
  PlayerWindow::PlayerWindow(QWidget * parent, Qt::WindowFlags flags):
    QWidget(parent, flags | Qt::Window),
    containerLayout_(NULL),
    playerWidget_(NULL)
  {
    setupUi(this);
    setAcceptDrops(false);
    setWindowTitle(trUtf8("yaetv player"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/images/yaetv-logo.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    containerLayout_ = new QVBoxLayout(containerWidget);
    containerLayout_->setMargin(0);
    containerLayout_->setSpacing(0);
  }

  //----------------------------------------------------------------
  // PlayerWindow::playback
  //
  void
  PlayerWindow::playback(PlayerWidget * playerWidget,
                         const IReaderPtr & reader,
                         const IBookmark * bookmark,
                         bool start_from_zero_time)
  {
    playerWidget_ = playerWidget;
    playerWidget->playback(reader, bookmark, start_from_zero_time);
  }

  //----------------------------------------------------------------
  // PlayerWindow::stopPlayback
  //
  void
  PlayerWindow::stopPlayback()
  {
    if (playerWidget_)
    {
      playerWidget_->stop();
      playerWidget_->view().setEnabled(false);
      playerWidget_ = NULL;
    }
  }

  //----------------------------------------------------------------
  // Playerwindow::stopAndHide
  //
  void
  PlayerWindow::stopAndHide()
  {
    PlayerWidget * playerWidget = playerWidget_;

    stopPlayback();
    hide();

    if (playerWidget)
    {
      playerWidget->canvas_->sigs_.stopHideCursorTimer();
      playerWidget->canvas_->sigs_.showCursor();
    }

    yae::queue_call(*this, &PlayerWindow::emit_window_closed);
  }

  //----------------------------------------------------------------
  // PlayerWindow::changeEvent
  //
  void
  PlayerWindow::changeEvent(QEvent * event)
  {
    if (event->type() == QEvent::WindowStateChange)
    {
      if (isFullScreen())
      {
        menubar->hide();
      }
      else
      {
        menubar->show();
      }
    }

    event->ignore();
  }

  //----------------------------------------------------------------
  // PlayerWindow::closeEvent
  //
  void
  PlayerWindow::closeEvent(QCloseEvent * event)
  {
    event->ignore();
    stopAndHide();
  }

}
