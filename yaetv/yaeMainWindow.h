// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Wed Dec 25 17:10:53 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt:
#include <QCheckBox>
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
#include "ui_yaePreferencesDialog.h"


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
  // PreferencesDialog
  //
  class PreferencesDialog : public QDialog,
                            public Ui::PreferencesDialog
  {
    Q_OBJECT;

  public:
    PreferencesDialog(QWidget * parent = 0);
    ~PreferencesDialog();

    void init(DVR & dvr);

  public slots:
    void on_finished(int result);
    void select_storage_folder();
    void appearance_changed();

  protected:
    QVBoxLayout * tuners_layout_;
    DVR * dvr_;
    Json::Value preferences_;
    std::map<std::string, QCheckBox *> tuners_;
  };

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
    void themeChanged(const yae::Application & app);

    // file menu:
    void fileExit();
    void exitConfirmed();

    // edit preferences:
    void editPreferences();

    // help menu:
    void helpAbout();

    // helpers:
    void swapShortcuts();
    void watchLive(uint32_t ch_num, TTime seekPos);

    // returns NULL reader if playback failed to start:
    IReaderPtr playbackRecording(TRecPtr rec);

    void confirmDelete(TRecPtr rec);
    void playbackFinished(TTime playheadPos);
    void confirmDeletePlayingRecording();
    void saveBookmark();
    void saveBookmarkAt(double position_in_sec);
    void playerWindowClosed();
    void playerEnteringFullScreen();
    void playerExitingFullScreen();
    void backToPlaylist();
    void confirmExit();

  protected slots:
    void startLivePlayback();
    void stopLivePlayback();

  protected:
    // virtual:
    void changeEvent(QEvent * e);
    void closeEvent(QCloseEvent * e);
    void keyPressEvent(QKeyEvent * e);
    void mousePressEvent(QMouseEvent * e);

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;

  public:
    PreferencesDialog preferencesDialog_;
    PlayerWidget * playerWidget_;
    PlayerWindow playerWindow_;

    // file reader prototype factory instance:
    TReaderFactoryPtr readerFactory_;

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
    TRecPtr live_rec_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
