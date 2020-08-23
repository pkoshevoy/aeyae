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

#include <stdlib.h>
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
#include <QDir>
#ifdef YAE_USE_QT5
#include <QSurfaceFormat>
#endif

// yae includes:
#include "yae/ffmpeg/yae_live_reader.h"
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

}

//----------------------------------------------------------------
// mainMayThrowException
//
int
mainMayThrowException(int argc, char ** argv)
{
  // Create and install global locale (UTF-8)
  {
    const char * lc_type = getenv("LC_TYPE");
    const char * lc_all = getenv("LC_ALL");
    const char * lang = getenv("LANG");

#ifndef _WIN32
    if (!(lc_type || lc_all || lang))
    {
      // avoid crasing in boost+libiconv:
      setenv("LANG", "en_US.UTF-8", 1);
    }
#endif

#ifndef NDEBUG
    lang || (lang = "");
    lc_all || (lc_all = "");
    lc_type || (lc_type = "");

    std::cerr << "LC_TYPE: " << lc_type << std::endl
              << "LC_ALL: " << lc_all << std::endl
              << "LANG: " << lang << std::endl;
#endif

#if defined(__APPLE__) && defined(__BIG_ENDIAN__)
    const char * default_locale = "C";
#else
    const char * default_locale = "";
#endif

    boost::locale::generator gen;
#if !defined(__APPLE__) || __MAC_OS_X_VERSION_MAX_ALLOWED >= 1090
    std::locale loc = std::locale(gen(default_locale),
                                  std::locale::classic(),
                                  std::locale::numeric);
#else
    std::locale loc = std::locale(gen(default_locale));
#endif
    std::locale::global(loc);

    // Make boost.filesystem use global locale:
    boost::filesystem::path::imbue(loc);
  }

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

  // check for canary invocation:
  bool canary = false;
  int percentTempo = 100;
  {
    char ** src = argv + 1;
    char ** end = argv + argc;
    char ** dst = src;
    for (; src < end; src++)
    {
      if (strcmp(*src, "--canary") == 0)
      {
        canary = true;
        argc--;
      }
      else if (strcmp(*src, "--tempo") == 0)
      {
        src++;
        argc--;
        percentTempo = yae::to_scalar<int, const char *>(*src);
        argc--;
      }
      else
      {
        *dst = *src;
        dst++;
      }
    }
  }

#ifdef __APPLE__
  if (!canary)
  {
    // show the Dock icon:
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
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

#ifdef YAE_USE_QT5
  // setup opengl:
  {
    QSurfaceFormat fmt(// QSurfaceFormat::DebugContext |
                       QSurfaceFormat::DeprecatedFunctions);
    fmt.setAlphaBufferSize(0);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(fmt);
  }
  // yae::Application::setAttribute(Qt::AA_UseDesktopOpenGL, true);
  // yae::Application::setAttribute(Qt::AA_UseOpenGLES, false);
  // yae::Application::setAttribute(Qt::AA_UseSoftwareOpenGL, false);
  yae::Application::setAttribute(Qt::AA_ShareOpenGLContexts, true);
  // yae::Application::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif

  yae::Application app(argc, argv);
  QStringList args = app.arguments();

  // check for canary invocation:
  std::list<QString> playlist;

  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    const QString & arg = *i;
    yae::addToPlaylist(playlist, arg);
  }

  //----------------------------------------------------------------
  // readerFactory
  //
  yae::TReaderFactoryPtr readerFactory(new yae::ReaderFactory());

  if (canary)
  {
    yae::testEachFile(readerFactory, playlist);

    // if it didn't crash, then it's all good:
    return 0;
  }

  yae::mainWindow = new yae::MainWindow(readerFactory);

  bool ok = QObject::connect(&app,
                             SIGNAL(file_open(const QString &)),
                             yae::mainWindow,
                             SLOT(setPlaylist(const QString &)));
  YAE_ASSERT(ok);

  yae::mainWindow->show();
  yae::mainWindow->initItemViews();

  yae::mainWindow->setPlaylist(playlist);
  yae::mainWindow->playbackSetTempo(percentTempo);

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
