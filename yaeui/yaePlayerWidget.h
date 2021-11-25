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
#include "yaeCanvasWidget.h"
#include "yaeConfirmView.h"
#include "yaePlayerShortcuts.h"
#include "yaePlayerView.h"

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

    inline PlayerUxItem & get_player_ux() const
    { return *(view_.player_ux()); }

  signals:
    void playbackFinished();
    void enteringFullScreen();
    void exitingFullScreen();

  public slots:

    // helpers:
    void playbackVerticalScaling(bool enable);
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();

    void focusChanged(QWidget * prev, QWidget * curr);

    void canvasSizeBackup();
    void canvasSizeRestore();
    void canvasSizeSet(double xexpand, double yexpand);
    void canvasSizeScaleBy(double scale);

    virtual void adjustCanvasHeight();
    virtual void swapShortcuts();
    virtual void populateContextMenu();

  protected:
    // virtual:
    void mousePressEvent(QMouseEvent * e);

    // virtual:
    bool processMousePressEvent(QMouseEvent * event);

    // helpers:

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
    yae::shared_ptr<PlayerShortcuts> shortcuts_;

  public:

    // default greeting is hello:
    QString greeting_;

    // frame canvas:
    TCanvasWidget * canvas_;

    // player views:
    PlayerView view_;
    ConfirmView confirm_;

  protected:
    // remember most recently used full screen render mode:
    Canvas::TRenderMode renderMode_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;
  };

}


#endif // YAE_PLAYER_WIDGET_H_
