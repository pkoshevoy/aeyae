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
#ifdef YAE_USE_QT5
#include <QSurfaceFormat>
#endif

// yae includes:
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// local:
#include "yaeMainWindow.h"
#include "yaeRemux.h"
#include "yaeUtilsQt.h"
#include "yaeVersion.h"


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
      std::set<std::string> sources;

      if (filename.endsWith(".yaerx", Qt::CaseInsensitive))
      {
        mainWindow->fileOpen(filename);
      }
      else
      {
        std::string source = filename.toUtf8().constData();
        sources.insert(source);
        mainWindow->add(sources);
      }

      return true;
    }
  };
}


//----------------------------------------------------------------
// usage
//
static void
usage(char ** argv, const char * message = NULL)
{
  std::cerr
    << "\nUSAGE:\n"
    << argv[0]
    << " [-no-ui] [-w] [-o ${output_path}]"
    << " [[-track track_id]"
    << " ${source_file}"
    << " [-t time_in time_out]*]+"
    << "\n"
    << argv[0]
    << " ${document}.yaerx"
    << "\n";

  std::cerr
    << "\nEXAMPLE:\n"
    << "\n# load and clip two sources, decode and save keyframes:\n"
    << argv[0]
    << " -track v:000"
    << " ~/Movies/foo.ts -t 25s 32s"
    << " ~/Movies/bar.ts -t 00:02:29.440 00:02:36.656"
    << " -no-ui -w -o ~/Movies/keyframes"
    << "\n"
    << "\n# load and clip two sources, remux and save output into one file:\n"
    << argv[0]
    << " -track v:000"
    << " ~/Movies/foo.ts -t 25s 32s"
    << " ~/Movies/bar.ts -t 00:02:29.440 00:02:36.656"
    << " -no-ui -o ~/Movies/two-clips-joined-together.ts"
    << "\n"
    << "\n# load source, show summary and GOP structure, then quit:\n"
    << argv[0]
    << " -no-ui ~/Movies/foo.ts"
    << "\n"
    << "\n# edit a document:\n"
    << argv[0]
    << " ~/Movies/two-clips-joined-together.yaerx"
    << "\n";

  std::cerr
    << "\nVERSION: " << YAE_REVISION
#ifndef NDEBUG
    << ", Debug build"
#endif
    << std::endl;

  if (message != NULL)
  {
    std::cerr << "\n" << message << std::endl;
  }

  ::exit(1);
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
  // show the Dock icon:
  {
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    TransformProcessType(&psn, kProcessTransformToForegroundApplication);
  }

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
  // yae::Application::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif

  yae::Application app(argc, argv);
  QStringList args = app.arguments();

  // parse input parameters:
  std::set<std::string> sources;
  std::string curr_source;
  std::list<yae::ClipInfo> clips;
  std::set<std::string> clipped;
  std::string output_path;
  std::string curr_track;
  bool save_keyframes = false;
  bool no_ui = false;

  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    std::string arg = i->toUtf8().constData();

    if (arg == "-track")
    {
      ++i;
      curr_track = i->toUtf8().constData();
    }
    else if (arg == "-t")
    {
      // clip boundaries should be evaluated after the source is analyzed
      // and framerate is known:
      clips.push_back(yae::ClipInfo(curr_source, curr_track));
      yae::ClipInfo & trim = clips.back();

      ++i;
      std::string t0 = i->toUtf8().constData();
      trim.t0_ = t0;

      ++i;
      std::string t1 = i->toUtf8().constData();
      trim.t1_ = t1;

      clipped.insert(curr_source);
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
    else if (arg == "-no-ui")
    {
      no_ui = true;
    }
    else
    {
      if (!QFile(*i).exists())
      {
        usage(argv, yae::str("unknown parameter: ",
                             i->toUtf8().constData()).c_str());
      }

      if (al::iends_with(arg, ".yaerx"))
      {
          std::string fn = i->toUtf8().constData();
          std::string json_str = yae::TOpenFile(fn.c_str(), "rb").read();

          std::set<std::string> s;
          std::list<yae::ClipInfo> c;
          if (yae::RemuxModel::parse_json_str(json_str, s, c))
          {
            clips.clear();
            sources.clear();
            clipped.clear();

            for (std::list<yae::ClipInfo>::const_iterator
                   j = c.begin(); j != c.end(); ++j)
            {
              const yae::ClipInfo & clip = *j;
              clips.push_back(clip);
              curr_source = clip.source_;
              curr_track = clip.track_;
              sources.insert(curr_source);
              clipped.insert(curr_source);
            }
          }
      }
      else
      {
        std::string filePath;
        if (yae::convert_path_to_utf8(*i, filePath))
        {
          if (!curr_source.empty() && !yae::has(clipped, curr_source))
          {
            // untrimmed:
            clips.push_back(yae::ClipInfo(curr_source));
            clipped.insert(curr_source);
          }

          sources.insert(filePath);
          curr_source = filePath;
        }
      }
    }
  }

  if (!curr_source.empty() && !yae::has(clipped, curr_source))
  {
    // untrimmed:
    clips.push_back(yae::ClipInfo(curr_source));
    clipped.insert(curr_source);
  }

  if (no_ui)
  {
    // these are expressed in seconds:
    const double buffer_duration = 1.0;
    const double discont_tolerance = 0.017;

    // load the sources:
    yae::TDemuxerInterfacePtr demuxer =
      yae::load(sources, clips, buffer_duration, discont_tolerance);

    if (!demuxer)
    {
      usage(argv, "failed to import any source files, nothing to do now");
    }

    // show the summary:
    const yae::DemuxerSummary & summary = demuxer->summary();
    std::cout << "\nsummary:\n" << summary << std::endl;

    if (!output_path.empty())
    {
      if (!demuxer)
      {
        return 1;
      }

      // start from the beginning:
      demuxer->seek(AVSEEK_FLAG_BACKWARD,
                    summary.rewind_.second,
                    summary.rewind_.first);

      if (!save_keyframes)
      {
        int err = yae::remux(output_path.c_str(), *demuxer);
        return err;
      }

      // save the keyframes:
      yae::demux(demuxer, output_path, save_keyframes);
    }

    return 0;
  }

  yae::mainWindow = new yae::MainWindow();
  yae::mainWindow->add(sources, clips);
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
    std::cout << std::flush;
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
