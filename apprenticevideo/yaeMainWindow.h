// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:50:01 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_MAIN_WINDOW_H_
#define YAE_MAIN_WINDOW_H_

// Qt includes:
#include <QMainWindow>

// yae includes:
#include <yaeAPI.h>
#include <yaeReader.h>
#include <yaeViewer.h>

// local includes:
#include "ui_yaeMainWindow.h"


namespace yae
{
  
  //----------------------------------------------------------------
  // MainWindow
  // 
  class MainWindow : public QMainWindow,
                     public Ui::yaeMainWindow
  {
    Q_OBJECT;
    
  public:
    MainWindow();
    ~MainWindow();
    
    // open a movie file for playback:
    bool load(const QString & path);
    
  public slots:
    // the file menu:
    void fileOpen();
    void fileExit();
    
  protected:
    // virtual:
    void closeEvent(QCloseEvent * e);
    void dragEnterEvent(QDragEnterEvent * e);
    void dropEvent(QDropEvent * e);
    
    // file reader:
    IReader * reader_;
    
    // frame viewer:
    Viewer * viewer_;
  };
  
};

#endif // YAE_MAIN_WINDOW_H_
