// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 22:22:54 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_WIDGET_H_
#define YAE_PLAYER_WIDGET_H_

// Qt:
#include <QDialog>
#include <QWidget>

// local:
#ifdef __APPLE__
#include "yaeAppleRemoteControl.h"
#include "yaeAppleUtils.h"
#endif
#include "yaeCanvasWidget.h"
#include "yaeConfirmView.h"
#include "yaeFrameCropView.h"
#include "yaePlayerView.h"
#include "yaeSpinnerView.h"

// uic:
#include "ui_yaeAspectRatioDialog.h"


namespace yae
{
  // forward declarations:
  class PlayerWidget;

  //----------------------------------------------------------------
  // TCanvasWidget
  //
#if defined(YAE_USE_QOPENGL_WIDGET)
  typedef CanvasWidget<QOpenGLWidget> TCanvasWidget;
#else
  typedef CanvasWidget<QGLWidget> TCanvasWidget;
#endif


  //----------------------------------------------------------------
  // AspectRatioDialog
  //
  class AspectRatioDialog : public QDialog,
                            public Ui::AspectRatioDialog
  {
    Q_OBJECT;

  public:
    AspectRatioDialog(QWidget * parent = 0);
  };


#ifdef __APPLE__
  //----------------------------------------------------------------
  // RemoteControlEvent
  //
  struct RemoteControlEvent : public QEvent
  {
    RemoteControlEvent(TRemoteControlButtonId buttonId,
                       bool pressedDown,
                       unsigned int clickCount,
                       bool heldDown):
      QEvent(QEvent::User),
      buttonId_(buttonId),
      pressedDown_(pressedDown),
      clickCount_(clickCount),
      heldDown_(heldDown)
    {}

    TRemoteControlButtonId buttonId_;
    bool pressedDown_;
    unsigned int clickCount_;
    bool heldDown_;
  };
#endif


  //----------------------------------------------------------------
  // PlayerWidget
  //
  class PlayerWidget : public QWidget
  {
    Q_OBJECT;

  public:
    PlayerWidget(QWidget * parent = NULL,
                 TCanvasWidget * shared_ctx = NULL,
                 Qt::WindowFlags f = Qt::WindowFlags());
    ~PlayerWidget();

    void initItemViews();

    void playback(const IReaderPtr & reader,
                  bool start_from_zero_time = false);
    void stop();

    // accessors:
    inline const TCanvasWidget & canvas() const
    { return *canvas_; }

    inline TCanvasWidget & canvas()
    { return *canvas_; }

    inline const PlayerView & view() const
    { return view_; }

    inline PlayerView & view()
    { return view_; }

  signals:
    void setInPoint();
    void setOutPoint();
    void menuButtonPressed();
    void playbackFinished();

  public slots:

    // helpers:
    void playbackVerticalScaling();
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();

    // prompt user for aspect ratio via a dialog:
    //
    // FIXME: this could be done with an ItemView/TextEdit instead
    void playbackAspectRatioOther();

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();
    void windowDecreaseSize();
    void windowIncreaseSize();

    // helpers:
    void focusChanged(QWidget * prev, QWidget * curr);
    void playbackCropFrameOther();

    void dismissFrameCropView();
    void adjustCanvasHeight();
    void canvasSizeBackup();
    void canvasSizeRestore();
    void swapShortcuts();

  protected:
    // virtual:
    bool event(QEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // helpers:
    void canvasSizeSet(double xexpand, double yexpand);

#ifdef __APPLE__
    // for Apple Remote:
    static void appleRemoteControlObserver(void * observerContext,
                                           TRemoteControlButtonId buttonId,
                                           bool pressedDown,
                                           unsigned int clickCount,
                                           bool heldDown);
    void * appleRemoteControl_;
#endif

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutFullScreen_;
    QShortcut * shortcutFillScreen_;
    QShortcut * shortcutShowTimeline_;
    QShortcut * shortcutPlay_;
    QShortcut * shortcutNext_;
    QShortcut * shortcutPrev_;
    QShortcut * shortcutLoop_;
    QShortcut * shortcutCropNone_;
    QShortcut * shortcutAutoCrop_;
    QShortcut * shortcutCrop1_33_;
    QShortcut * shortcutCrop1_78_;
    QShortcut * shortcutCrop1_85_;
    QShortcut * shortcutCrop2_40_;
    QShortcut * shortcutCropOther_;
    QShortcut * shortcutNextChapter_;
    QShortcut * shortcutAspectRatioNone_;
    QShortcut * shortcutAspectRatio1_33_;
    QShortcut * shortcutAspectRatio1_78_;

    QShortcut * shortcutRemove_;

  public:
    // frame canvas:
    TCanvasWidget * canvas_;

    // player views:
    PlayerView view_;
    SpinnerView spinner_;
    ConfirmView confirm_;
    FrameCropView cropView_;
    yae::shared_ptr<Canvas::ILoadFrameObserver> onLoadFrame_;

  protected:
    // remember most recently used full screen render mode:
    Canvas::TRenderMode renderMode_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;

    // selected track info is used to select matching track(s)
    // after loading next file:
    TTrackInfo selAudio_;
    AudioTraits selAudioTraits_;

    TTrackInfo selSubs_;
    TSubsFormat selSubsFormat_;
  };

}


#endif // YAE_PLAYER_WIDGET_H_
