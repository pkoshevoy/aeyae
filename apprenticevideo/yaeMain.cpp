// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri May 28 00:43:26 MDT 2010
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#ifdef _WIN32
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <wchar.h>
#endif

#include <iostream>
#include <stdexcept>

// GLEW includes:
#include <GL/glew.h>

// Qt includes:
#include <QApplication>
#include <QFileOpenEvent>

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
#if defined(_WIN32) && !defined(NDEBUG)
  // restore console stdio:
  {
    AllocConsole();
    
#pragma warning(push)
#pragma warning(disable: 4996)
    
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);
    
#pragma warning(pop)
    
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdOut != INVALID_HANDLE_VALUE)
    {
      COORD consoleBufferSize;
      consoleBufferSize.X = 80;
      consoleBufferSize.Y = 9999;
      SetConsoleScreenBufferSize(hStdOut, consoleBufferSize);
    }
  }
#endif
  
  yae::Application app(argc, argv);
  yae::mainWindow = new yae::MainWindow();
  yae::mainWindow->show();
  
  // initialize OpenGL GLEW wrapper:
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    std::cerr << "GLEW init failed: " << glewGetErrorString(err) << std::endl;
    YAE_ASSERT(false);
  }
  
  // initialize the canvas:
  yae::mainWindow->canvas()->initializePrivateBackend();

  QStringList args = app.arguments();
  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    QString filename = *i;
    if (yae::mainWindow->load(filename))
    {
      break;
    }
  }
  
  app.exec();
  return 0;
}
