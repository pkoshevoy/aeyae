// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:50:01 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt includes:
#include <QDialog>
#include <QMainWindow>
#include <QShortcut>

// yae includes:
#include "yae/api/yae_shared_ptr.h"
#include "yae/video/yae_reader.h"

// local includes:
#include "yaeMainWidget.h"

// Qt uic generated files:
#include "ui_yaeAbout.h"
#include "ui_yaeMainWindow.h"
#include "ui_yaeOpenUrlDialog.h"


namespace yae
{
  // forward declarations:
  class MainWindow;

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
  // OpenUrlDialog
  //
  class OpenUrlDialog : public QDialog,
                        public Ui::OpenUrlDialog
  {
    Q_OBJECT;

  public:
    OpenUrlDialog(QWidget * parent = 0);
  };

  //----------------------------------------------------------------
  // MainWindow
  //
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;

  public:
    MainWindow(const IReaderPtr & readerPrototype);
    ~MainWindow();

    void initItemViews();

    void setPlaylist(const std::list<QString> & playlist,
                     bool beginPlaybackImmediately = true);

    void playbackSetTempo(double percentTempo);

  public slots:
    // file menu:
    void fileNewWindow();
    void fileOpen();
    void fileOpenURL();
    void fileOpenFolder();
    void fileExit();

    // help menu:
    void helpAbout();

    // helpers:
    void adjustMenus(IReaderPtr reader);
    void swapShortcuts();

  protected:
    void processDropEventUrls(const QList<QUrl> & urls);

    // virtual:
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);

    // shortcuts used during full-screen mode (when menubar is invisible)
    QShortcut * shortcutExit_;

    // frame canvas:
    MainWidget * playerWidget_;

    // dialog for opening a URL resource:
    OpenUrlDialog * openUrl_;
  };
}


#endif // YAE_MAIN_WINDOW_H_
