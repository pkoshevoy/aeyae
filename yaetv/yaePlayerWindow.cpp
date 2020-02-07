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
  }

  //----------------------------------------------------------------
  // PlayerWindow::playback
  //
  void
  PlayerWindow::playback(const TRecordingPtr & rec_ptr,
                         const IReaderPtr & reader,
                         TCanvasWidget * shared_ctx,
                         bool start_from_zero_time)
  {
    if (!playerWidget_)
    {
      QVBoxLayout * containerLayout = new QVBoxLayout(containerWidget);
      containerLayout->setMargin(0);
      containerLayout->setSpacing(0);

      playerWidget_ = new PlayerWidget(this, shared_ctx);

      PlayerView & view = playerWidget_->view_;
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

    if (rec_ptr)
    {
      const Recording & rec = *rec_ptr;
      std::string time_str = yae::unix_epoch_time_to_localdate(rec.utc_t0_);
      std::string title = strfmt("%i-%i %s, %s",
                                 rec.channel_major_,
                                 rec.channel_minor_,
                                 rec.title_.c_str(),
                                 time_str.c_str());
      window()->setWindowTitle(QString::fromUtf8(title.c_str()));
    }

    playerWidget_->playback(rec_ptr, reader, start_from_zero_time);
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
