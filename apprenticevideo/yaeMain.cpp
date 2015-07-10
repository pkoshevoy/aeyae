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

// boost:
#include <boost/locale.hpp>
#include <boost/filesystem/path.hpp>

// APPLE includes:
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifndef _WIN32
#include <signal.h>
#endif

// Qt includes:
#include <QApplication>
#include <QFileOpenEvent>

// yae includes:
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_reader.h"
#include "yae/utils/yae_utils.h"

// local includes:
#include <yaeMainWindow.h>
#include <yaeUtilsQt.h>


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
      yae::addToPlaylist(playlist, filename);
      mainWindow->setPlaylist(playlist);

      return true;
    }
  };
}

//----------------------------------------------------------------
// plugins
//
yae::TPluginRegistry plugins;

//----------------------------------------------------------------
// readerPrototype
//
yae::IReaderPtr readerPrototype;

//----------------------------------------------------------------
// mainMayThrowException
//
int
mainMayThrowException(int argc, char ** argv)
{
  // Create and install global locale (UTF-8)
  std::locale::global(boost::locale::generator().generate(""));

  // Make boost.filesystem use it
  boost::filesystem::path::imbue(std::locale());

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

#ifndef _WIN32
  signal(SIGPIPE, SIG_IGN);
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
      yae::addToPlaylist(playlist, arg);
    }
  }

  // load plugins:
  std::string exeFolderPath;
  if (yae::getCurrentExecutableFolder(exeFolderPath) &&
      plugins.load(exeFolderPath.c_str()))
  {
    std::list<yae::IReaderPtr> readers;
    if (plugins.find<yae::IReader>(readers))
    {
      readerPrototype = readers.front();
    }
  }

  if (!readerPrototype)
  {
    std::cerr
      << "ERROR: failed to find IReader plugin here: "
      << exeFolderPath
      << std::endl;
    return -1;
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
