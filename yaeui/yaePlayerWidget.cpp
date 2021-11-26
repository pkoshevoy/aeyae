// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  1 13:45:55 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt includes:
#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QShortcut>
#include <QSpacerItem>
#include <QUrl>
#include <QVBoxLayout>

// local:
#include "yaePlayerWidget.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerWidget::PlayerWidget
  //
  PlayerWidget::PlayerWidget(QWidget * parent,
                             TCanvasWidget * shared_ctx,
                             Qt::WindowFlags flags):
    QWidget(parent, flags),
#ifdef __APPLE__
    appleRemoteControl_(NULL),
#endif
    canvas_(NULL),
    player_("PlayerWidget player view"),
    confirm_("PlayerWidget confirm view")
  {
    greeting_ = tr("hello");

    QVBoxLayout * canvasLayout = new QVBoxLayout(this);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // setup the canvas widget (QML quick widget):
#ifdef YAE_USE_QOPENGL_WIDGET
    canvas_ = new TCanvasWidget(this);
    canvas_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    canvas_ = new TCanvasWidget(contextFormat, this, shared_ctx);
#endif

    canvas_->setObjectName(tr("player view canvas"));

    canvas_->setFocusPolicy(Qt::StrongFocus);
    canvas_->setAcceptDrops(false);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvas_);

    bool ok = true;

    ok = connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                 this, SLOT(focusChanged(QWidget *, QWidget *)));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(maybeHideCursor()),
                 &(canvas_->sigs_), SLOT(hideCursor()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerWidget::~PlayerWidget
  //
  PlayerWidget::~PlayerWidget()
  {
    confirm_.clear();
    player_.clear();
    delete canvas_;
  }

  //----------------------------------------------------------------
  // PlayerWidget::initItemViews
  //
  void
  PlayerWidget::initItemViews()
  {
    canvas_->initializePrivateBackend();

    TMakeCurrentContext currentContext(canvas_->Canvas::context());
    canvas_->setGreeting(greeting_);
    canvas_->append(&player_);

    canvas_->append(&confirm_);
    confirm_.setStyle(player_.style());

    // shortcut:
    yae::PlayerUxItem * pl_ux = player_.player_ux();

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;

    ok = connect(&(canvas_->sigs_), SIGNAL(escShort()),
                 pl_ux, SIGNAL(toggle_playlist()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(toggleFullScreen()),
                 pl_ux, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(escLong()),
                 pl_ux, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);
#if 0
    ok = connect(pl_ux, SIGNAL(playback_finished(TTime)),
                 this, SIGNAL(playbackFinished()));
    YAE_ASSERT(ok);
#endif
    ok = connect(pl_ux, SIGNAL(enteringFullScreen()),
                 this, SIGNAL(enteringFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(exitingFullScreen()),
                 this, SIGNAL(exitingFullScreen()));
    YAE_ASSERT(ok);

    shortcuts_.reset(new PlayerShortcuts(this));
    pl_ux->set_shortcuts(shortcuts_);

    player_.setEnabled(true);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playback
  //
  void
  PlayerWidget::playback(const IReaderPtr & reader,
                         const IBookmark * bookmark,
                         bool start_from_zero_time)
  {
    player_.setEnabled(true);

    PlayerUxItem & pl_ux = get_player_ux();
    pl_ux.playback(reader, bookmark, start_from_zero_time);
  }

  //----------------------------------------------------------------
  // PlayerWidget::stop
  //
  void
  PlayerWidget::stop()
  {
    PlayerUxItem & pl_ux = get_player_ux();
    pl_ux.stopPlayback();
  }

  //----------------------------------------------------------------
  // PlayerWidget::requestToggleFullScreen
  //
  void
  PlayerWidget::requestToggleFullScreen()
  {
    PlayerUxItem & pl_ux = get_player_ux();
    pl_ux.requestToggleFullScreen();
  }

  //----------------------------------------------------------------
  // PlayerWidget::focusChanged
  //
  void
  PlayerWidget::focusChanged(QWidget * prev, QWidget * curr)
  {
#if 0
    std::cerr << "focus changed: " << prev << " -> " << curr;
    if (curr)
    {
      std::cerr << ", " << curr->objectName().toUtf8().constData()
                << " (" << curr->metaObject()->className() << ")";
    }
    std::cerr << std::endl;
#endif

#ifdef __APPLE__
    if (!appleRemoteControl_ && curr)
    {
      appleRemoteControl_ =
        appleRemoteControlOpen(true, // exclusive
                               false, // count clicks
                               false, // simulate hold
                               &PlayerWidget::appleRemoteControlObserver,
                               this);
    }
    else if (appleRemoteControl_ && !curr)
    {
      appleRemoteControlClose(appleRemoteControl_);
      appleRemoteControl_ = NULL;
    }
#endif
  }

  //----------------------------------------------------------------
  // PlayerWidget::swapShortcuts
  //
  void
  PlayerWidget::swapShortcuts()
  {
    // PlayerUxItem & pl_ux = get_player_ux();
    // pl_ux.swapShortcuts();
  }

  //----------------------------------------------------------------
  // PlayerWidget::populateContextMenu
  //
  void
  PlayerWidget::populateContextMenu()
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    pl_ux.populateContextMenu();
  }

  //----------------------------------------------------------------
  // PlayerWidget::mousePressEvent
  //
  void
  PlayerWidget::mousePressEvent(QMouseEvent * e)
  {
    if (this->processMousePressEvent(e))
    {
      e->accept();
      return;
    }

    QWidget::mousePressEvent(e);
  }

  //----------------------------------------------------------------
  // PlayerWidget::processMousePressEvent
  //
  bool
  PlayerWidget::processMousePressEvent(QMouseEvent * event)
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    if (event->button() == Qt::RightButton)
    {
      QPoint localPt = event->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      populateContextMenu();

      pl_ux.contextMenu_->popup(globalPt);
      return true;
    }

    return false;
  }

#ifdef __APPLE__
  //----------------------------------------------------------------
  // appleRemoteControlObserver
  //
  void
  PlayerWidget::appleRemoteControlObserver(void * observerContext,
                                           TRemoteControlButtonId buttonId,
                                           bool pressedDown,
                                           unsigned int clickCount,
                                           bool heldDown)
  {
    PlayerWidget * widget = (PlayerWidget *)observerContext;
    RemoteControlEvent * rc = new RemoteControlEvent(buttonId,
                                                     pressedDown,
                                                     clickCount,
                                                     heldDown);
#ifndef NDEBUG
    yae_debug
      << "posting remote control event(" << rc
      << "), buttonId: " << buttonId
      << ", down: " << pressedDown
      << ", clicks: " << clickCount
      << ", held down: " << heldDown;
#endif

    qApp->postEvent(widget->canvas_, rc, Qt::HighEventPriority);
  }
#endif

}
