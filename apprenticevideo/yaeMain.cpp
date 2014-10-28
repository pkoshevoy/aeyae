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

// APPLE includes:
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

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
      std::list<QString> playlist;
      playlist.push_back(filename);
      mainWindow->setPlaylist(playlist);

      return true;
    }
  };
};

//----------------------------------------------------------------
// mainMayThrowException
//
int
mainMayThrowException(int argc, char ** argv)
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
  /*
  std::cout.precision(4);
  std::cerr.precision(4);
  std::cout.setf(std::ios::scientific);
  std::cerr.setf(std::ios::scientific);
  */

#ifdef __APPLE__
  if (QSysInfo::MacintoshVersion == 0x000a)
  {
    // add a workaround for Qt 4.7 QTBUG-32789
    // that manifests as misaligned text on OS X Mavericks:
    QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
  }
  else if (QSysInfo::MacintoshVersion >= 0x000b)
  {
    // add a workaround for Qt 4.8 QTBUG-40833
    // that manifests as misaligned text on OS X Yosemite:
    QFont::insertSubstitution(".Helvetica Neue DeskInterface",
                              "Helvetica Neue");
  }
#endif

  yae::Application::setApplicationName("ApprenticeVideo");
  yae::Application::setOrganizationName("PavelKoshevoy");
  yae::Application::setOrganizationDomain("sourceforge.net");
  yae::Application app(argc, argv);
  QStringList args = app.arguments();

  // check for canary invocation:
  bool canary = false;
  std::list<QString> playlist;

  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    const QString & arg = *i;
    if (arg == QString::fromUtf8("--canary"))
    {
      canary = true;
    }
    else
    {
      playlist.push_back(arg);
    }
  }

  if (canary)
  {
    yae::MainWindow::testEachFile(playlist);

    // if it didn't crash, then it's all good:
    return 0;
  }

#ifdef __APPLE__
  // show the Dock icon:
  ProcessSerialNumber psn = { 0, kCurrentProcess };
  TransformProcessType(&psn, kProcessTransformToForegroundApplication);
#endif

  yae::mainWindow = new yae::MainWindow();
  yae::mainWindow->show();
  yae::mainWindow->raise();

  // initialize OpenGL GLEW wrapper:
  GLenum err = glewInit();
  if (err != GLEW_OK)
  {
    std::cerr << "GLEW init failed: " << glewGetErrorString(err) << std::endl;
    YAE_ASSERT(false);
  }

  // initialize the canvas:
  yae::mainWindow->canvas()->initializePrivateBackend();

  yae::mainWindow->setPlaylist(playlist);

  app.exec();
  return 0;
}

//----------------------------------------------------------------
// main
//
int
main(int argc, char ** argv)
{
  int r = 0;

  try
  {
    r = mainMayThrowException(argc, argv);
  }
  catch (const std::exception & e)
  {
    std::cerr << "ERROR: unexpected exception: " << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "ERROR: unknown exception" << std::endl;
    return 2;
  }

  return r;
}
