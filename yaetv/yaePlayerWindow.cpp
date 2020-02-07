// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Feb  2 21:29:58 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QCloseEvent>

// local:
#include "yaePlayerWidget.h"
#include "yaePlayerWindow.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerWindow::PlayerWindow
  //
  PlayerWindow::PlayerWindow(QWidget * parent, Qt::WindowFlags flags):
    QWidget(parent, flags | Qt::Window),
    playerWidget_(NULL),
    appView_(NULL)
  {
    setupUi(this);
    setAcceptDrops(false);
    setWindowTitle(trUtf8("yaetv player"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon =
      QString::fromUtf8(":/images/yaetv-logo.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif
  }

  //----------------------------------------------------------------
  // PlayerWindow::setAppView
  //
  void
  PlayerWindow::setAppView(AppView * appView)
  {
    appView_ = appView;
  }

  //----------------------------------------------------------------
  // PlayerWindow::playback
  //
  void
  PlayerWindow::playback(const IReaderPtr & reader,
                         TCanvasWidget * shared_ctx)
  {
    if (!playerWidget_)
    {
      QVBoxLayout * containerLayout = new QVBoxLayout(containerWidget);
      containerLayout->setMargin(0);
      containerLayout->setSpacing(0);

      playerWidget_ = new PlayerWidget(this, shared_ctx);

      PlayerView & view = playerWidget_->view_;
      view.setStyle(appView_->style());
      containerLayout->addWidget(playerWidget_);

      show();
      playerWidget_->initItemViews();
      QApplication::processEvents();

      menubar->addAction(view.menuPlayback_->menuAction());
      menubar->addAction(view.menuAudio_->menuAction());
      menubar->addAction(view.menuVideo_->menuAction());
      menubar->addAction(view.menuSubs_->menuAction());
      menubar->addAction(view.menuChapters_->menuAction());
    }
    else
    {
      show();
    }

    playerWidget_->playback(reader);
  }

  //----------------------------------------------------------------
  // PlayerWindow::changeEvent
  //
  void
  PlayerWindow::changeEvent(QEvent * event)
  {
    if (event->type() == QEvent::WindowStateChange)
    {
      if (window()->isFullScreen())
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
    playerWidget_->stop();
    hide();
  }

  //----------------------------------------------------------------
  // PlayerWindow::keyPressEvent
  //
  void
  PlayerWindow::keyPressEvent(QKeyEvent * event)
  {
    int key = event->key();
    event->ignore();

    if (key == Qt::Key_Escape)
    {
      event->accept();
      PlayerView & view = playerWidget_->view_;
      view.stopPlayback();
      hide();
    }
  }

}
