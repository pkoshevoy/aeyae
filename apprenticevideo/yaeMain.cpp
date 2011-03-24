// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri May 28 00:43:26 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <stdexcept>
#include <assert.h>

// Qt includes:
#include <QApplication>
#include <QFileOpenEvent>

// yae includes:
#include <yaeMainWindow.h>

// the includes:
#include <opengl/glsl.hxx>
#include <utils/the_utils.hxx>


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
#ifndef NDEBUG
  restore_console_stdio();
#endif
  
  yae::Application app(argc, argv);
  yae::mainWindow = new yae::MainWindow();
  yae::mainWindow->show();
  
  // initialize OpenGL GLEW wrapper:
  bool ok = glsl_init();
  assert(ok);
  (void)ok;
  
  // initialize the canvas:
  yae::mainWindow->canvas()->initializePrivateBackend();
  
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
