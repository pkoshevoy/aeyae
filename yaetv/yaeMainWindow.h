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
#include "yaeConfirmView.h"
#include "yaeSpinnerView.h"
#include "yaePlayerWindow.h"

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
               const std::string & recordings_dir,
               const IReaderPtr & reader_prototype);
    ~MainWindow();

    void initItemViews();

  public slots:
    // file menu:
    void fileExit();

    // help menu:
    void helpAbout();

    // helpers:
    void swapShortcuts();
    void watchLive(uint32_t ch_num, TTime seekPos);

    // returns NULL reader if playback failed to start:
    IReaderPtr playbackRecording(TRecordingPtr rec);

    void confirmDelete(TRecordingPtr rec);
    void playbackFinished(TTime playheadPos);
    void confirmDeletePlayingRecording();
    void saveBookmark();
    void playerWindowClosed();
    void playerEnteringFullScreen();
    void playerExitingFullScreen();
    void backToPlaylist();

  protected slots:
    void startLivePlayback();
    void stopLivePlayback();

  protected:
    // virtual:
    bool event(QEvent * e);
    void changeEvent(QEvent * e);
    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;

  public:
    PlayerWidget * playerWidget_;
    PlayerWindow playerWindow_;

    // file reader prototype factory instance:
    IReaderPtr readerPrototype_;

    // frame canvas:
    TCanvasWidget * canvas_;

    yae::DVR dvr_;
    AppView view_;
    ConfirmView confirm_;
    SpinnerView spinner_;

  protected:
    // background thread, etc...
    std::list<TAsyncTaskPtr> tasks_;
    AsyncTaskQueue async_;

    QTimer start_live_playback_;
    TTime start_live_seek_pos_;
    TRecordingPtr live_rec_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
