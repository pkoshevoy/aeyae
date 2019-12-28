// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Dec 25 17:10:53 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt:
#include <QDialog>
#include <QMainWindow>
#include <QMenu>
#include <QSignalMapper>
#include <QShortcut>
#include <QTimer>

// aeyae:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/thread/yae_task_runner.h"

// local:
#include "yaeAppView.h"
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaeCanvasWidget.h"
#include "yaeSpinnerView.h"

// uic:
#include "ui_yaeAbout.h"
#include "ui_yaeMainWindow.h"


namespace yae
{
  // forward declarations:
  class MainWindow;

  //----------------------------------------------------------------
  // TCanvasWidget
  //
#if defined(YAE_USE_QOPENGL_WIDGET)
  typedef CanvasWidget<QOpenGLWidget> TCanvasWidget;
#else
  typedef CanvasWidget<QGLWidget> TCanvasWidget;
#endif

  //----------------------------------------------------------------
  // AboutDialog
  //
  class AboutDialog : public QDialog,
                      public Ui::AboutDialog
  {
    Q_OBJECT;

  public:
    AboutDialog(QWidget * parent = 0);
  };

  //----------------------------------------------------------------
  // MainWindow
  //
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;

  public:
    MainWindow(const std::string & yaetv_dir,
               const std::string & recordings_dir);
    ~MainWindow();

    void initItemViews();

  public slots:
    // file menu:
    void fileExit();

    // help menu:
    void helpAbout();

    // helpers:
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen();
    void exitFullScreen();
    void swapShortcuts();

  protected:
    // virtual:
    bool event(QEvent * e);
    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;
    QShortcut * shortcutFullScreen_;

    // frame canvas:
    TCanvasWidget * canvas_;

    // DVR service loop thread:
    yae::Worker thread_;

    yae::DVR dvr_;
    AppView view_;
    SpinnerView spinner_;

    // background thread, etc...
    std::list<TAsyncTaskPtr> tasks_;
    AsyncTaskQueue async_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
