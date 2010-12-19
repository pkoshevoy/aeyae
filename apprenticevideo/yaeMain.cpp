// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri May 28 00:43:26 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>

// Qt includes:
#include <QApplication>

// yae includes:
#include <yaeMainWindow.h>


namespace yae
{

  //----------------------------------------------------------------
  // mainWindow
  // 
  MainWindow * mainWindow = NULL;
  
  //----------------------------------------------------------------
  // Application
  // 
  class Application : public QApplication
  {
  public:
    Application(int & argc, char ** argv):
      QApplication(argc, argv)
    {}
    
  protected:
    bool event(QEvent * e)
    {
      if (e->type() != QEvent::FileOpen)
      {
        return QApplication::event(e);
      }
      
      // handle the apple event to open a document:
      QString filename = static_cast<QFileOpenEvent *>(e)->file();
      mainWindow->load(filename);
      
      return true;
    }
  };
};

//----------------------------------------------------------------
// main
// 
int
main(int argc, char ** argv)
{
  yae::Application app(argc, argv);
  yae::mainWindow = new yae::MainWindow();
  yae::mainWindow->show();
  
  for (int i = 1; i < argc; i++)
  {
    QString filename = QString::fromUtf8(argv[i]);
    if (yae::mainWindow->load(filename))
    {
      break;
    }
  }
  
  app.exec();
  return 0;
}
