// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 13 15:53:35 MST 2018
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

#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

// boost:
#include <boost/locale.hpp>

// APPLE includes:
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#ifdef check
#undef check
#endif
#endif

#ifndef _WIN32
#include <signal.h>
#endif

// Qt includes:
#include <QCoreApplication>
#include <QDir>

// yae includes:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// local:
#include "yaeMainWindow.h"
#include "yaeReplay.h"
#include "yaeUtilsQt.h"


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
    {
#ifdef __APPLE__
      QString appDir = QApplication::applicationDirPath();
      QString plugInsDir = QDir::cleanPath(appDir + "/../PlugIns");
      QApplication::addLibraryPath(plugInsDir);
#endif
    }

    // virtual: overridden to propagate custom events to the parent:
    bool notify(QObject * receiver, QEvent * e)
    {
      YAE_ASSERT(receiver && e);
      bool result = false;

      QEvent::Type et = e ? e->type() : QEvent::None;
      if (et >= QEvent::User)
      {
        e->ignore();
      }

      while (receiver)
      {
        result = QApplication::notify(receiver, e);
        if (et < QEvent::User || (result && e->isAccepted()))
        {
          break;
        }

        receiver = receiver->parent();
      }

      return result;
    }

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

  yae::Application::setApplicationName("ApprenticeVideoRemux");
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
#endif

  yae::Application app(argc, argv);
  QStringList args = app.arguments();

  // parse input parameters:
  std::list<std::string> sources;
  std::map<std::string, std::pair<std::string, std::string> > trim;
  std::string output_path;
  bool save_keyframes = false;

  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    std::string arg = i->toUtf8().constData();

    if (arg == "-i")
    {
      ++i;
      std::string filePath;
      if (yae::convert_path_to_utf8(*i, filePath))
      {
        sources.push_back(filePath);
      }
    }
    else if (arg == "-t")
    {
      // trim timestamps should be evaluated after the source is analyzed
      // and framerate is known:
      std::pair<std::string, std::string> & t = trim[sources.back()];

      ++i;
      t.first = i->toUtf8().constData();

      ++i;
      t.second = i->toUtf8().constData();
    }
    else if (arg == "-o")
    {
      ++i;
      output_path = i->toUtf8().constData();
    }
    else if (arg == "-w")
    {
      save_keyframes = true;
    }
  }

  // these are expressed in seconds:
  const double buffer_duration = 1.0;
  const double discont_tolerance = 0.017;

  // load the sources:
  yae::DemuxerSummary summary;
  yae::TDemuxerInterfacePtr demuxer =
    yae::load(summary, sources, trim, buffer_duration, discont_tolerance);

  if (!output_path.empty())
  {
    if (!demuxer)
    {
      return 1;
    }

    if (!save_keyframes)
    {
      demuxer->seek(AVSEEK_FLAG_BACKWARD,
                    summary.rewind_.second,
                    summary.rewind_.first);
      int err = yae::remux(output_path.c_str(), summary, *demuxer);
      return err;
    }

    // save the keyframes:
    yae::demux(demuxer, summary, output_path, save_keyframes);
    return 0;
  }

  yae::mainWindow = new yae::MainWindow(demuxer);
  yae::mainWindow->show();

  // initialize the player widget canvas, connect additional signals/slots:
  yae::mainWindow->initCanvasWidget();
  yae::mainWindow->canvas()->initializePrivateBackend();
  yae::mainWindow->initItemViews();

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
