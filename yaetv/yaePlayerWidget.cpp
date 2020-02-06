// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Feb  1 13:45:55 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// boost:
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

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
#include <QProcess>
#include <QShortcut>
#include <QSpacerItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

// local:
#include "yaePlayerWidget.h"


namespace yae
{

  //----------------------------------------------------------------
  // AspectRatioDialog::AspectRatioDialog
  //
  AspectRatioDialog::AspectRatioDialog(QWidget * parent):
    QDialog(parent),
    Ui::AspectRatioDialog()
  {
    Ui::AspectRatioDialog::setupUi(this);
  }


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
    fullscreen = widget->isFullScreen();
    return true;
  }

  //----------------------------------------------------------------
  // PlayerWidget::PlayerWidget
  //
  PlayerWidget::PlayerWidget(QWidget * parent,
                             TCanvasWidget * shared_ctx,
                             Qt::WindowFlags flags):
    QWidget(parent, flags),
    canvas_(NULL)
  {
    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;

    ok = connect(&view_, SIGNAL(adjust_canvas_height()),
                 this, SLOT(adjustCanvasHeight()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionVerticalScaling_, SIGNAL(triggered()),
                 this, SLOT(playbackVerticalScaling()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionShrinkWrap_, SIGNAL(triggered()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionFullScreen_, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionFillScreen_, SIGNAL(triggered()),
                 this, SLOT(playbackFillScreen()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionAspectRatioOther_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioOther()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionCropFrameOther_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameOther()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionHalfSize_, SIGNAL(triggered()),
                 this, SLOT(windowHalfSize()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionFullSize_, SIGNAL(triggered()),
                 this, SLOT(windowFullSize()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionDoubleSize_, SIGNAL(triggered()),
                 this, SLOT(windowDoubleSize()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionDecreaseSize_, SIGNAL(triggered()),
                 this, SLOT(windowDecreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(view_.actionIncreaseSize_, SIGNAL(triggered()),
                 this, SLOT(windowIncreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(&cropView_, SIGNAL(cropped(const TVideoFramePtr &,
                                            const TCropFrame &)),
                 &view_, SLOT(cropped(const TVideoFramePtr &,
                                      const TCropFrame &)));
    YAE_ASSERT(ok);

    ok = connect(&cropView_, SIGNAL(done()),
                 this, SLOT(dismissFrameCropView()));
    YAE_ASSERT(ok);

    shortcutFullScreen_ = new QShortcut(this);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 view_.actionFullScreen_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutFillScreen_ = new QShortcut(this);
    shortcutFillScreen_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutFillScreen_, SIGNAL(activated()),
                 view_.actionFillScreen_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutShowTimeline_ = new QShortcut(this);
    shortcutShowTimeline_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutShowTimeline_, SIGNAL(activated()),
                 view_.actionShowTimeline_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutPlay_ = new QShortcut(this);
    shortcutPlay_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutPlay_, SIGNAL(activated()),
                 view_.actionPlay_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutNextChapter_ = new QShortcut(this);
    shortcutNextChapter_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutNextChapter_, SIGNAL(activated()),
                 view_.actionNextChapter_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutNext_ = new QShortcut(this);
    shortcutNext_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutNext_, SIGNAL(activated()),
                 view_.actionNext_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutPrev_ = new QShortcut(this);
    shortcutPrev_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutPrev_, SIGNAL(activated()),
                 view_.actionPrev_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutLoop_ = new QShortcut(this);
    shortcutLoop_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutLoop_, SIGNAL(activated()),
                 view_.actionLoop_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCropNone_ = new QShortcut(this);
    shortcutCropNone_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCropNone_, SIGNAL(activated()),
                 view_.actionCropFrameNone_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCrop1_33_ = new QShortcut(this);
    shortcutCrop1_33_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCrop1_33_, SIGNAL(activated()),
                 view_.actionCropFrame1_33_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCrop1_78_ = new QShortcut(this);
    shortcutCrop1_78_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCrop1_78_, SIGNAL(activated()),
                 view_.actionCropFrame1_78_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCrop1_85_ = new QShortcut(this);
    shortcutCrop1_85_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCrop1_85_, SIGNAL(activated()),
                 view_.actionCropFrame1_85_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCrop2_40_ = new QShortcut(this);
    shortcutCrop2_40_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCrop2_40_, SIGNAL(activated()),
                 view_.actionCropFrame2_40_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutCropOther_ = new QShortcut(this);
    shortcutCropOther_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutCropOther_, SIGNAL(activated()),
                 view_.actionCropFrameOther_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutAutoCrop_ = new QShortcut(this);
    shortcutAutoCrop_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutAutoCrop_, SIGNAL(activated()),
                 view_.actionCropFrameAutoDetect_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutAspectRatioNone_ = new QShortcut(this);
    shortcutAspectRatioNone_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutAspectRatioNone_, SIGNAL(activated()),
                 view_.actionAspectRatioAuto_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutAspectRatio1_33_ = new QShortcut(this);
    shortcutAspectRatio1_33_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutAspectRatio1_33_, SIGNAL(activated()),
                 view_.actionAspectRatio1_33_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutAspectRatio1_78_ = new QShortcut(this);
    shortcutAspectRatio1_78_->setContext(Qt::ApplicationShortcut);

    ok = connect(shortcutAspectRatio1_78_, SIGNAL(activated()),
                 view_.actionAspectRatio1_78_, SLOT(trigger()));
    YAE_ASSERT(ok);

    shortcutRemove_ = new QShortcut(this);
    shortcutRemove_->setContext(Qt::ApplicationShortcut);
    shortcutRemove_->setKey(QKeySequence(QKeySequence::Delete));

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

    view_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);
    view_.query_fullscreen_.reset(&player_query_fullscreen, this);

    canvas_->setFocusPolicy(Qt::StrongFocus);
    canvas_->setAcceptDrops(false);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvas_);
  }

  //----------------------------------------------------------------
  // PlayerWidget::~PlayerWidget
  //
  PlayerWidget::~PlayerWidget()
  {
    canvas_->cropAutoDetectStop();
    delete canvas_;
  }

  //----------------------------------------------------------------
  // PlayerWidget::initItemViews
  //
  void
  PlayerWidget::initItemViews()
  {
    canvas_->initializePrivateBackend();
    canvas_->setGreeting(tr("yaetv player"));
    canvas_->append(&view_);

    canvas_->append(&spinner_);
    canvas_->append(&confirm_);
    canvas_->append(&cropView_);

    spinner_.setStyle(view_.style());
    confirm_.setStyle(view_.style());
    cropView_.init(&view_);

    spinner_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);
    confirm_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);
    cropView_.toggle_fullscreen_.reset(&player_toggle_fullscreen, this);

    spinner_.query_fullscreen_.reset(&player_query_fullscreen, this);
    confirm_.query_fullscreen_.reset(&player_query_fullscreen, this);
    cropView_.query_fullscreen_.reset(&player_query_fullscreen, this);

    CanvasRendererItem & rendererItem =
      cropView_.root()->get<CanvasRendererItem>("uncropped");

    onLoadFrame_.reset(new OnFrameLoaded(rendererItem));
    canvas_->addLoadFrameObserver(onLoadFrame_);

    bool ok = true;
    ok = connect(this, SIGNAL(setInPoint()),
                 &view_.timeline_model(), SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setOutPoint()),
                 &view_.timeline_model(), SLOT(setOutPoint()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playback
  //
  void
  PlayerWidget::playback(const IReaderPtr & reader)
  {
    view_.setEnabled(true);
    view_.playback(reader);
  }

  //----------------------------------------------------------------
  // PlayerWidget::stop
  //
  void
  PlayerWidget::stop()
  {
    view_.stopPlayback();
    view_.setEnabled(false);
    confirm_.setEnabled(false);
    spinner_.setEnabled(false);
    cropView_.setEnabled(false);
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackVerticalScaling
  //
  void
  PlayerWidget::playbackVerticalScaling()
  {
    canvasSizeBackup();
    bool enable = view_.actionVerticalScaling_->isChecked();
    canvas().enableVerticalScaling(enable);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackShrinkWrap
  //
  void
  PlayerWidget::playbackShrinkWrap()
  {
    if (isFullScreen())
    {
      return;
    }

    IReader * reader = view_.get_reader();
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
    if (isFullScreen())
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
    if (isFullScreen() && renderMode_ == renderMode)
    {
      exitFullScreen();
      return;
    }

    SignalBlocker blockSignals;
    blockSignals
      << view_.actionFullScreen_
      << view_.actionFillScreen_;

    if (renderMode == Canvas::kScaleToFit)
    {
      view_.actionFullScreen_->setChecked(true);
      view_.actionFillScreen_->setChecked(false);
    }

    if (renderMode == Canvas::kCropToFill)
    {
      view_.actionFillScreen_->setChecked(true);
      view_.actionFullScreen_->setChecked(false);
    }

    canvas_->setRenderMode(renderMode);
    renderMode_ = renderMode;

    if (isFullScreen())
    {
      return;
    }

    // enter full screen rendering:
    view_.actionShrinkWrap_->setEnabled(false);
    // menuBar()->hide();

    showFullScreen();
    swapShortcuts();
  }

  //----------------------------------------------------------------
  // PlayerWidget::exitFullScreen
  //
  void
  PlayerWidget::exitFullScreen()
  {
    if (!isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals
      << view_.actionFullScreen_
      << view_.actionFillScreen_;

    view_.actionFullScreen_->setChecked(false);
    view_.actionFillScreen_->setChecked(false);
    view_.actionShrinkWrap_->setEnabled(true);

    // menuBar()->show();

    showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    QTimer::singleShot(100, this, SLOT(adjustCanvasHeight()));

    swapShortcuts();
  }

  //----------------------------------------------------------------
  // PlayerWidget::playbackAspectRatioOther
  //
  void
  PlayerWidget::playbackAspectRatioOther()
  {
    static AspectRatioDialog * aspectRatioDialog = NULL;
    if (!aspectRatioDialog)
    {
      aspectRatioDialog = new AspectRatioDialog(this);
    }

    double w = 0.0;
    double h = 0.0;
    double dar = canvas().imageAspectRatio(w, h);
    dar = dar != 0.0 ? dar : 1.777777;
    aspectRatioDialog->doubleSpinBox->setValue(dar);

    int r = aspectRatioDialog->exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    dar = aspectRatioDialog->doubleSpinBox->value();
    canvas().overrideDisplayAspectRatio(dar);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerWidget::windowHalfSize
  //
  void
  PlayerWidget::windowHalfSize()
  {
    canvasSizeSet(0.5, 0.5);
  }

  //----------------------------------------------------------------
  // PlayerWidget::windowFullSize
  //
  void
  PlayerWidget::windowFullSize()
  {
    canvasSizeSet(1.0, 1.0);
  }

  //----------------------------------------------------------------
  // PlayerWidget::windowDoubleSize
  //
  void
  PlayerWidget::windowDoubleSize()
  {
    canvasSizeSet(2.0, 2.0);
  }

  //----------------------------------------------------------------
  // PlayerWidget::windowDecreaseSize
  //
  void
  PlayerWidget::windowDecreaseSize()
  {
    canvasSizeSet(xexpand_ * 0.5, yexpand_ * 0.5);
  }

  //----------------------------------------------------------------
  // PlayerWidget::windowIncreaseSize
  //
  void
  PlayerWidget::windowIncreaseSize()
  {
    canvasSizeSet(xexpand_ * 2.0, yexpand_ * 2.0);
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
  // PlayerWidget::playbackCropFrameOther
  //
  void
  PlayerWidget::playbackCropFrameOther()
  {
    CanvasRenderer * renderer = canvas().canvasRenderer();

    TVideoFramePtr frame;
    renderer->getFrame(frame);
    if (!frame)
    {
      return;
    }

    // pass current frame crop info to the view:
    {
      TCropFrame crop;
      renderer->getCroppedFrame(crop);

      SignalBlocker blockSignals;
      blockSignals << &cropView_;
      cropView_.setCrop(frame, crop);
    }

    view_.setEnabled(false);
    cropView_.setEnabled(true);
    onLoadFrame_->frameLoaded(canvas_, frame);
  }

  //----------------------------------------------------------------
  // PlayerWidget::dismissFrameCropView
  //
  void
  PlayerWidget::dismissFrameCropView()
  {
    cropView_.setEnabled(false);
    view_.setEnabled(true);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerWidget::adjustCanvasHeight
  //
  void
  PlayerWidget::adjustCanvasHeight()
  {
    IReader * reader = view_.get_reader();
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

    if (!isFullScreen())
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

    if (cropView_.isEnabled())
    {
      playbackCropFrameOther();
    }
  }

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeBackup
  //
  void
  PlayerWidget::canvasSizeBackup()
  {
    if (isFullScreen())
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
  // swapShortcuts
  //
  static inline void
  swapShortcuts(QShortcut * a, QAction * b)
  {
    QKeySequence tmp = a->key();
    a->setKey(b->shortcut());
    b->setShortcut(tmp);
  }

  //----------------------------------------------------------------
  // PlayerWidget::swapShortcuts
  //
  void
  PlayerWidget::swapShortcuts()
  {
    yae::swapShortcuts(shortcutFullScreen_, view_.actionFullScreen_);
    yae::swapShortcuts(shortcutFillScreen_, view_.actionFillScreen_);
    yae::swapShortcuts(shortcutShowTimeline_, view_.actionShowTimeline_);
    yae::swapShortcuts(shortcutPlay_, view_.actionPlay_);
    yae::swapShortcuts(shortcutNext_, view_.actionNext_);
    yae::swapShortcuts(shortcutPrev_, view_.actionPrev_);
    yae::swapShortcuts(shortcutLoop_, view_.actionLoop_);
    yae::swapShortcuts(shortcutCropNone_, view_.actionCropFrameNone_);
    yae::swapShortcuts(shortcutCrop1_33_, view_.actionCropFrame1_33_);
    yae::swapShortcuts(shortcutCrop1_78_, view_.actionCropFrame1_78_);
    yae::swapShortcuts(shortcutCrop1_85_, view_.actionCropFrame1_85_);
    yae::swapShortcuts(shortcutCrop2_40_, view_.actionCropFrame2_40_);
    yae::swapShortcuts(shortcutCropOther_, view_.actionCropFrameOther_);
    yae::swapShortcuts(shortcutAutoCrop_, view_.actionCropFrameAutoDetect_);
    yae::swapShortcuts(shortcutNextChapter_, view_.actionNextChapter_);
    yae::swapShortcuts(shortcutAspectRatioNone_, view_.actionAspectRatioAuto_);
    yae::swapShortcuts(shortcutAspectRatio1_33_, view_.actionAspectRatio1_33_);
    yae::swapShortcuts(shortcutAspectRatio1_78_, view_.actionAspectRatio1_78_);
  }

  //----------------------------------------------------------------
  // PlayerWidget::event
  //
  bool
  PlayerWidget::event(QEvent * e)
  {
    QEvent::Type et = e->type();

    if (et == QEvent::User)
    {
#ifdef __APPLE__
      RemoteControlEvent * rc = dynamic_cast<RemoteControlEvent *>(e);
      if (rc)
      {
#ifndef NDEBUG
        std::cerr
          << "received remote control event(" << rc
          << "), buttonId: " << rc->buttonId_
          << ", down: " << rc->pressedDown_
          << ", clicks: " << rc->clickCount_
          << ", held down: " << rc->heldDown_
          << std::endl;
#endif
        rc->accept();

        if (rc->buttonId_ == kRemoteControlPlayButton)
        {
          if (rc->pressedDown_)
          {
            if (rc->heldDown_)
            {
              toggleFullScreen();
            }
            else
            {
              view_.togglePlayback();
            }
          }
        }
        else if (rc->buttonId_ == kRemoteControlMenuButton)
        {
          if (rc->pressedDown_)
          {
            if (rc->heldDown_)
            {
              if (view_.actionCropFrameAutoDetect_->isChecked())
              {
                view_.actionCropFrameNone_->trigger();
              }
              else
              {
                view_.actionCropFrameAutoDetect_->trigger();
              }
            }
            else
            {
              emit menuButtonPressed();
            }
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeUp)
        {
          if (rc->pressedDown_)
          {
            // raise the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume + 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeDown)
        {
          if (rc->pressedDown_)
          {
            // lower the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume - 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlLeftButton ||
                 rc->buttonId_ == kRemoteControlRightButton)
        {
          if (rc->pressedDown_)
          {
            double offset =
              (rc->buttonId_ == kRemoteControlLeftButton) ? -3.0 : 7.0;

            view_.timeline_model().seekFromCurrentTime(offset);
            view_.timeline_->maybeAnimateOpacity();
          }
        }

        return true;
      }
#endif
    }

    return QWidget::event(e);
  }

  //----------------------------------------------------------------
  // PlayerWidget::keyPressEvent
  //
  void
  PlayerWidget::keyPressEvent(QKeyEvent * event)
  {
    int key = event->key();
    event->ignore();

    if (key == Qt::Key_Escape)
    {
      if (isFullScreen())
      {
        exitFullScreen();
        event->accept();
      }
    }
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
      event->accept();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
      event->accept();
    }
    else
    {
      QWidget::keyPressEvent(event);
    }
  }

  //----------------------------------------------------------------
  // PlayerWidget::mousePressEvent
  //
  void
  PlayerWidget::mousePressEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::RightButton)
    {
      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      view_.populateContextMenu();
      view_.contextMenu_->popup(globalPt);
      e->accept();
      return;
    }

    QWidget::mousePressEvent(e);
  }

  //----------------------------------------------------------------
  // PlayerWidget::canvasSizeSet
  //
  void
  PlayerWidget::canvasSizeSet(double xexpand, double yexpand)
  {
    xexpand_ = xexpand;
    yexpand_ = yexpand;

    if (isFullScreen())
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
    int ww = rectWindow.width();
    int wh = rectWindow.height();

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    // calculate width and height overhead:
    int ox = ww - cw;
    int oy = wh - ch;

    int ideal_w = ox + int(0.5 + vw * xexpand_);
    int ideal_h = oy + int(0.5 + vh * yexpand_);

    QRect rectMax = QApplication::desktop()->availableGeometry(this);
    int max_w = rectMax.width();
    int max_h = rectMax.height();

    if (ideal_w > max_w || ideal_h > max_h)
    {
      // image won't fit on screen, scale it to the largest size that fits:
      double vDAR = canvas_->imageWidth() / canvas_->imageHeight();
      double cDAR = double(max_w - ox) / double(max_h - oy);

      if (vDAR > cDAR)
      {
        ideal_w = max_w;
        ideal_h = oy + int(0.5 + double(max_w - ox) / vDAR);
      }
      else
      {
        ideal_h = max_h;
        ideal_w = ox + int(0.5 + double(max_h - oy) * vDAR);
      }
    }

    int new_w = std::min(ideal_w, max_w);
    int new_h = std::min(ideal_h, max_h);

    // apply the new window geometry:
    QRect rectClient = geometry();
    int cdx = rectWindow.width() - rectClient.width();
    int cdy = rectWindow.height() - rectClient.height();

#if 0
    int max_x0 = rectMax.x();
    int max_y0 = rectMax.y();
    int max_x1 = max_x0 + max_w - 1;
    int max_y1 = max_y0 + max_h - 1;

    int new_x0 = rectWindow.x();
    int new_y0 = rectWindow.y();
    int new_x1 = new_x0 + new_w - 1;
    int new_y1 = new_y0 + new_h - 1;

    int shift_x = std::min(0, max_x1 - new_x1);
    int shift_y = std::min(0, max_y1 - new_y1);

    int new_x = new_x0 + shift_x;
    int new_y = new_y0 + shift_y;

    std::cerr << "\ncanvas size set: " << xexpand << ", " << yexpand
              << std::endl
              << "canvas resize: " << new_w - cdx << ", " << new_h - cdy
              << std::endl
              << "canvas move to: " << new_x << ", " << new_y
              << std::endl;
#endif

    window->resize(new_w - cdx, new_h - cdy);
    // move(new_x, new_y);

    // repaint the frame:
    canvas_->refresh();
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

    boost::interprocess::unique_ptr<RemoteControlEvent>
      rc(new RemoteControlEvent(buttonId,
                                pressedDown,
                                clickCount,
                                heldDown));
#ifndef NDEBUG
    std::cerr
      << "posting remote control event(" << rc.get()
      << "), buttonId: " << buttonId
      << ", down: " << pressedDown
      << ", clicks: " << clickCount
      << ", held down: " << heldDown
      << std::endl;
#endif

    qApp->postEvent(widget, rc.release(), Qt::HighEventPriority);
  }
#endif

}
