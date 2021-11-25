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
  // player_toggle_fullscreen
  //
  static void
  player_toggle_fullscreen(void * context)
  {
    PlayerWidget * widget = (PlayerWidget *)context;
    widget->requestToggleFullScreen();
  }


  //----------------------------------------------------------------
  // player_query_fullscreen
  //
  static bool
  player_query_fullscreen(void * context, bool & fullscreen)
  {
    PlayerWidget * widget = (PlayerWidget *)context;
    fullscreen = widget->window()->isFullScreen();
    return true;
  }

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
    view_("PlayerWidget player view"),
    confirm_("PlayerWidget confirm view"),
    renderMode_(Canvas::kScaleToFit),
    xexpand_(1.0),
    yexpand_(1.0)
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

    ok = connect(&(canvas_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(escLong()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(maybeHideCursor()),
                 &(canvas_->sigs_), SLOT(hideCursor()));
    YAE_ASSERT(ok);

    view_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);
    view_.query_fullscreen_.reset(&player_query_fullscreen, this);

    confirm_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);
    confirm_.query_fullscreen_.reset(&player_query_fullscreen, this);
  }

  //----------------------------------------------------------------
  // PlayerWidget::~PlayerWidget
  //
  PlayerWidget::~PlayerWidget()
  {
    confirm_.clear();
    view_.clear();
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
    canvas_->append(&view_);

    canvas_->append(&confirm_);
    confirm_.setStyle(view_.style());

    // shortcut:
    yae::PlayerUxItem * pl_ux = view_.player_ux();

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;

    ok = connect(&(canvas_->sigs_), SIGNAL(escShort()),
                 pl_ux, SIGNAL(toggle_playlist()));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(adjust_canvas_height()),
                 this, SLOT(adjustCanvasHeight()));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(expand_canvas_size(double, double)),
                 this, SLOT(canvasSizeSet(double, double)));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(scale_canvas_size(double)),
                 this, SLOT(canvasSizeScaleBy(double)));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(playback_vertical_scaling(bool)),
                 this, SLOT(playbackVerticalScaling(bool)));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(playback_shrink_wrap()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(playback_full_screen()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(pl_ux, SIGNAL(playback_fill_screen()),
                 this, SLOT(playbackFillScreen()));
    YAE_ASSERT(ok);

    shortcuts_.reset(new PlayerShortcuts(this));
    pl_ux->set_shortcuts(shortcuts_);

    view_.setEnabled(true);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playback
  //
  void
  PlayerWidget::playback(const IReaderPtr & reader,
                         const IBookmark * bookmark,
                         bool start_from_zero_time)
  {
    view_.setEnabled(true);

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
  // PlayerWidget::playbackVerticalScaling
  //
  void
  PlayerWidget::playbackVerticalScaling(bool enable)
  {
    canvasSizeBackup();
    canvas().enableVerticalScaling(enable);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackShrinkWrap
  //
  void
  PlayerWidget::playbackShrinkWrap()
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    if (window()->isFullScreen())
    {
      return;
    }

    IReader * reader = pl_ux.get_reader();
    if (!reader)
    {
      return;
    }

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    canvasSizeBackup();

    double scale = std::min<double>(xexpand_, yexpand_);
    canvasSizeSet(scale, scale);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackFullScreen
  //
  void
  PlayerWidget::playbackFullScreen()
  {
    // enter full screen pillars-and-bars letterbox rendering:
    enterFullScreen(Canvas::kScaleToFit);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackFillScreen
  //
  void
  PlayerWidget::playbackFillScreen()
  {
    // enter full screen crop-to-fill rendering:
    enterFullScreen(Canvas::kCropToFill);
  }

  //----------------------------------------------------------------
  // PlayerWidget::requestToggleFullScreen
  //
  void
  PlayerWidget::requestToggleFullScreen()
  {
    // all this to work-around apparent QML bug where
    // toggling full-screen on double-click leaves Flickable in
    // a state where it never receives the button-up event
    // and ends up interpreting all mouse movement as dragging,
    // very annoying...
    //
    // The workaround is to delay fullscreen toggle to allow
    // Flickable time to receive the button-up event

    QTimer::singleShot(178, this, SLOT(toggleFullScreen()));
  }

  //----------------------------------------------------------------
  // PlayerWidget::toggleFullScreen
  //
  void
  PlayerWidget::toggleFullScreen()
  {
    if (window()->isFullScreen())
    {
      exitFullScreen();
    }
    else
    {
      enterFullScreen(renderMode_);
    }
  }

  //----------------------------------------------------------------
  // PlayerWidget::enterFullScreen
  //
  void
  PlayerWidget::enterFullScreen(Canvas::TRenderMode renderMode)
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    bool is_fullscreen = window()->isFullScreen();
    if (is_fullscreen && renderMode_ == renderMode)
    {
      exitFullScreen();
      return;
    }

    emit enteringFullScreen();

    SignalBlocker blockSignals;
    blockSignals
      << pl_ux.actionFullScreen_
      << pl_ux.actionFillScreen_;

    if (renderMode == Canvas::kScaleToFit)
    {
      pl_ux.actionFullScreen_->setChecked(true);
      pl_ux.actionFillScreen_->setChecked(false);
    }

    if (renderMode == Canvas::kCropToFill)
    {
      pl_ux.actionFillScreen_->setChecked(true);
      pl_ux.actionFullScreen_->setChecked(false);
    }

    canvas_->setRenderMode(renderMode);
    renderMode_ = renderMode;

    if (is_fullscreen)
    {
      return;
    }

    // enter full screen rendering:
    pl_ux.actionShrinkWrap_->setEnabled(false);

    window()->showFullScreen();
    // swapShortcuts();
  }

  //----------------------------------------------------------------
  // PlayerWidget::exitFullScreen
  //
  void
  PlayerWidget::exitFullScreen()
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    if (!window()->isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals
      << pl_ux.actionFullScreen_
      << pl_ux.actionFillScreen_;

    pl_ux.actionFullScreen_->setChecked(false);
    pl_ux.actionFillScreen_->setChecked(false);
    pl_ux.actionShrinkWrap_->setEnabled(true);

    window()->showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    QTimer::singleShot(100, this, SLOT(adjustCanvasHeight()));

    // swapShortcuts();

    emit exitingFullScreen();
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
  // PlayerWidget::adjustCanvasHeight
  //
  void
  PlayerWidget::adjustCanvasHeight()
  {
    // shortcuts:
    PlayerUxItem & pl_ux = get_player_ux();
    IReader * reader = pl_ux.get_reader();

    if (!reader)
    {
      return;
    }

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    if (!window()->isFullScreen())
    {
      double w = 1.0;
      double h = 1.0;
      double dar = canvas_->imageAspectRatio(w, h);

      if (dar)
      {
        double s = double(canvas_->width()) / w;
        canvasSizeSet(s, s);
      }
    }

    if (pl_ux.frame_crop_->visible())
    {
      pl_ux.playbackCropFrameOther();
    }
  }

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeBackup
  //
  void
  PlayerWidget::canvasSizeBackup()
  {
    if (window()->isFullScreen())
    {
      return;
    }

    int vw = int(0.5 + canvas_->imageWidth());
    int vh = int(0.5 + canvas_->imageHeight());
    if (vw < 1 || vh < 1)
    {
      return;
    }

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);

#if 0
    std::cerr << "\ncanvas size backup: " << xexpand_ << ", " << yexpand_
              << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeRestore
  //
  void
  PlayerWidget::canvasSizeRestore()
  {
    canvasSizeSet(xexpand_, yexpand_);
  }

  //----------------------------------------------------------------
  // PlayerWidget::swapShortcuts
  //
  void
  PlayerWidget::swapShortcuts()
  {
    // shortcut:
    PlayerUxItem & pl_ux = get_player_ux();

    pl_ux.swap_shortcuts();
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

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeSet
  //
  void
  PlayerWidget::canvasSizeSet(double xexpand, double yexpand)
  {
    xexpand_ = xexpand;
    yexpand_ = yexpand;

    if (window()->isFullScreen())
    {
      return;
    }

    double iw = canvas_->imageWidth();
    double ih = canvas_->imageHeight();

    int vw = int(0.5 + iw);
    int vh = int(0.5 + ih);

    if (vw < 1 || vh < 1)
    {
      return;
    }

    QWidget * window = QWidget::window();
    QRect rectWindow = window->frameGeometry();
    QRect rectClient = window->geometry();
    int ww = rectWindow.width();
    int wh = rectWindow.height();

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    // calculate width and height overhead:
    int ox = ww - cw;
    int oy = wh - ch;

    int ideal_w = int(0.5 + vw * xexpand_);
    int ideal_h = int(0.5 + vh * yexpand_);

    QRect rectMax = QApplication::desktop()->availableGeometry(this);
    int max_w = rectMax.width() - ox;
    int max_h = rectMax.height() - oy;

    if (ideal_w > max_w || ideal_h > max_h)
    {
      // image won't fit on screen, scale it to the largest size that fits:
      double vDAR = iw / ih;
      double cDAR = double(max_w) / double(max_h);

      if (vDAR > cDAR)
      {
        ideal_w = max_w;
        ideal_h = int(0.5 + double(max_w) / vDAR);
      }
      else
      {
        ideal_h = max_h;
        ideal_w = int(0.5 + double(max_h) * vDAR);
      }
    }

    int new_w = std::min(ideal_w, max_w);
    int new_h = std::min(ideal_h, max_h);

    int dx = new_w - cw;
    int dy = new_h - ch;

    // apply the new window geometry:
    window->resize(rectClient.width() + dx,
                   rectClient.height() + dy);

    // repaint the frame:
    canvas_->refresh();
  }

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeScaleBy
  //
  void
  PlayerWidget::canvasSizeScaleBy(double scale)
  {
    canvasSizeSet(xexpand_ * scale, yexpand_ * scale);
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
