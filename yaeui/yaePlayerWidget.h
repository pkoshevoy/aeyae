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
#include "yaeAspectRatioView.h"
#include "yaeCanvasWidget.h"
#include "yaeConfirmView.h"
#include "yaeFrameCropView.h"
#include "yaeOptionView.h"
#include "yaePlayerView.h"
#include "yaeSpinnerView.h"

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

    virtual void initItemViews();

    void playback(const IReaderPtr & reader,
                  const IBookmark * bookmark = NULL,
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
    void enteringFullScreen();
    void exitingFullScreen();

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

    // prompt to override output aspect ratio:
    void playbackAspectRatioOther();

    // callback from aspect ratio view reflecting current selection:
    void selectAspectRatio(const AspectRatio & ar);
    void setAspectRatio(double ar);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();
    void windowDecreaseSize();
    void windowIncreaseSize();

    // helpers:
    void selectFrameCrop(const AspectRatio & ar);
    void showFrameCropSelectionView();
    void showAspectRatioSelectionView();
    void showVideoTrackSelectionView();
    void showAudioTrackSelectionView();
    void showSubttTrackSelectionView();

    void focusChanged(QWidget * prev, QWidget * curr);
    void playbackCropFrameOther();

    void dismissFrameCropView();
    void dismissFrameCropSelectionView();
    void dismissAspectRatioSelectionView();
    void dismissVideoTrackSelectionView();
    void dismissAudioTrackSelectionView();
    void dismissSubttTrackSelectionView();

    void videoTrackSelectedOption(int option_index);
    void audioTrackSelectedOption(int option_index);
    void subttTrackSelectedOption(int option_index);

    void canvasSizeBackup();
    void canvasSizeRestore();

    virtual void adjustCanvasHeight();
    virtual void swapShortcuts();
    virtual void populateContextMenu();

  protected:
    // virtual:
    bool event(QEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    virtual bool processEvent(QEvent * event);
    virtual bool processKeyEvent(QKeyEvent * event);
    virtual bool processMousePressEvent(QMouseEvent * event);

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


  public:

    // default greeting is hello:
    QString greeting_;

    // frame canvas:
    TCanvasWidget * canvas_;

    // player views:
    PlayerView view_;
    SpinnerView spinner_;
    ConfirmView confirm_;
    FrameCropView cropView_;
    AspectRatioView frameCropSelectionView_;
    AspectRatioView aspectRatioSelectionView_;
    OptionView videoTrackSelectionView_;
    OptionView audioTrackSelectionView_;
    OptionView subttTrackSelectionView_;
    yae::shared_ptr<Canvas::ILoadFrameObserver> onLoadFrame_;

  protected:
    // remember most recently used full screen render mode:
    Canvas::TRenderMode renderMode_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;
  };

}


#endif // YAE_PLAYER_WIDGET_H_
