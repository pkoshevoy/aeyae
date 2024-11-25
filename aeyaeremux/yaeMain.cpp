// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Jan 13 15:53:35 MST 2018
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/ffmpeg/yae_remux.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"

// system:
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

// standard:
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/locale.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// APPLE:
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#ifdef check
#undef check
#endif
#endif

#ifndef _WIN32
#include <signal.h>
#endif

// Qt:
#include <QCoreApplication>
#include <QDir>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QSurfaceFormat>
#endif

// local:
#include "yaeMainWindow.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // mainWindow
  //
  MainWindow * mainWindow = NULL;

  //----------------------------------------------------------------
  // demux
  //
  void
  demux(const TDemuxerInterfacePtr & demuxer,
        const std::string & output_path = std::string(),
        bool save_keyframes = false)
  {
    const DemuxerSummary & summary = demuxer->summary();

    std::map<int, TTime> prog_dts;
    while (true)
    {
      AVStream * stream = NULL;
      TPacketPtr packet_ptr = demuxer->get(stream);
      if (!packet_ptr)
      {
        break;
      }

      // shortcuts:
      AvPkt & pkt = *packet_ptr;
      AVPacket & packet = pkt.get();

      std::cout
        << pkt.trackId_
        << ", demuxer: " << std::setw(2) << pkt.demuxer_->demuxer_index()
        << ", program: " << std::setw(3) << pkt.program_
        << ", pos: " << std::setw(12) << std::setfill(' ') << packet.pos
        << ", size: " << std::setw(6) << std::setfill(' ') << packet.size;

      TTime dts;
      if (get_dts(dts, stream, packet))
      {
        std::cout << ", dts: " << dts;

        TTime prev_dts =
          yae::get(prog_dts, pkt.program_,
                   TTime(std::numeric_limits<int64_t>::min(), dts.base_));

        // keep dts for reference:
        prog_dts[pkt.program_] = dts;

        if (dts < prev_dts)
        {
          av_log(NULL, AV_LOG_ERROR,
                 "non-monotonically increasing DTS detected, "
                 "program %03i, prev %s, curr %s\n",
                 pkt.program_,
                 prev_dts.to_hhmmss_frac(1000, ":", ".").c_str(),
                 dts.to_hhmmss_frac(1000, ":", ".").c_str());

          // the demuxer should always provide monotonically increasing DTS:
          YAE_ASSERT(false);
        }
      }
      else
      {
        // the demuxer should always provide a DTS:
        YAE_ASSERT(false);
      }

      if (packet.pts != AV_NOPTS_VALUE)
      {
        TTime pts(stream->time_base.num * packet.pts,
                  stream->time_base.den);

        std::cout << ", pts: " << pts;
      }

      if (packet.duration)
      {
        TTime dur(stream->time_base.num * packet.duration,
                  stream->time_base.den);

        std::cout << ", dur: " << dur;
      }

      const AVMediaType codecType = stream->codecpar->codec_type;

      int flags = packet.flags;
      if (codecType != AVMEDIA_TYPE_VIDEO)
      {
        flags &= ~(AV_PKT_FLAG_KEY);
      }

      bool is_keyframe = false;
      if (flags)
      {
        std::cout << ", flags:";

        if ((flags & AV_PKT_FLAG_KEY))
        {
          std::cout << " keyframe";
          is_keyframe = true;
        }

        if ((flags & AV_PKT_FLAG_CORRUPT))
        {
          std::cout << " corrupt";
        }

        if ((flags & AV_PKT_FLAG_DISCARD))
        {
          std::cout << " discard";
        }

        if ((flags & AV_PKT_FLAG_TRUSTED))
        {
          std::cout << " trusted";
        }

        if ((flags & AV_PKT_FLAG_DISPOSABLE))
        {
          std::cout << " disposable";
        }
      }

      for (int j = 0; j < packet.side_data_elems; j++)
      {
        std::cout
          << ", side_data[" << j << "] = { type: "
          << packet.side_data[j].type << ", size: "
          << packet.side_data[j].size << " }";
      }

      std::cout << std::endl;

      if (is_keyframe && save_keyframes)
      {
        fs::path folder = (fs::path(output_path) /
                           boost::replace_all_copy(pkt.trackId_, ":", "."));
        fs::create_directories(folder);

        std::string fn = (dts.to_hhmmss_frac(1000, "", ".") + ".png");
        std::string path((folder / fn).string());

        TrackPtr track_ptr = yae::get(summary.decoders_, pkt.trackId_);
        VideoTrackPtr decoder_ptr =
          boost::dynamic_pointer_cast<VideoTrack, Track>(track_ptr);

        if (!save_keyframe(path, decoder_ptr, packet_ptr, 0, 0, 0.0, 1.0))
        {
          break;
        }
      }
    }
  }

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
    << " [--no-ui] [-w] [-o ${output_path}]"
    << " [[--track track_id]"
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
    << " --track v:000"
    << " ~/Movies/foo.ts -t 25s 32s"
    << " ~/Movies/bar.ts -t 00:02:29.440 00:02:36.656"
    << " --no-ui -w -o ~/Movies/keyframes"
    << "\n"
    << "\n# load and clip two sources, remux and save output into one file:\n"
    << argv[0]
    << " --track v:000"
    << " ~/Movies/foo.ts -t 25s 32s"
    << " ~/Movies/bar.ts -t 00:02:29.440 00:02:36.656"
    << " --no-ui -o ~/Movies/two-clips-joined-together.ts"
    << "\n"
    << "\n# load source, show summary and GOP structure, then quit:\n"
    << argv[0]
    << " --no-ui ~/Movies/foo.ts"
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  // setup opengl:
  {
    QSurfaceFormat fmt(// QSurfaceFormat::DebugContext |
                       QSurfaceFormat::DeprecatedFunctions);
    // fmt.setVersion(4, 2);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(fmt);
  }
  // yae::Application::setAttribute(Qt::AA_UseDesktopOpenGL, true);
  // yae::Application::setAttribute(Qt::AA_UseOpenGLES, false);
  // yae::Application::setAttribute(Qt::AA_UseSoftwareOpenGL, false);
  yae::Application::setAttribute(Qt::AA_ShareOpenGLContexts, true);
  // yae::Application::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif

  // instantiate the logger:
  yae::logger();

  // parse input parameters:
  std::set<std::string> sources;
  std::string curr_source;
  std::list<yae::ClipInfo> clips;
  std::set<std::string> clipped;
  std::map<std::string, yae::SetOfTracks> redacted;
  std::string output_path;
  std::string curr_track;
  bool save_keyframes = false;
  bool no_ui = false;

  for (int i = 1; i < argc; i++)
  {
    QString qarg = QString::fromUtf8(argv[i]);
    std::string arg(argv[i]);

    if (arg == "-track" || arg == "--track")
    {
      ++i;
      curr_track = argv[i];
    }
    else if (arg == "-t")
    {
      // clip boundaries should be evaluated after the source is analyzed
      // and framerate is known:
      clips.push_back(yae::ClipInfo(curr_source, curr_track));
      yae::ClipInfo & trim = clips.back();

      ++i;
      std::string t0 = argv[i];
      trim.t0_ = t0;

      ++i;
      std::string t1 = argv[i];
      trim.t1_ = t1;

      clipped.insert(curr_source);
    }
    else if (arg == "-o")
    {
      ++i;
      output_path = argv[i];
    }
    else if (arg == "-w")
    {
      save_keyframes = true;
    }
    else if (arg == "-no-ui" || arg == "--no-ui")
    {
      no_ui = true;
    }
#ifdef __APPLE__
    else if (al::starts_with(arg, "-psn_"))
    {
      // ignore, OSX adds it when double-clicking on the app.
      continue;
    }
#endif
    else
    {
      if (!QFile(qarg).exists())
      {
        usage(argv, yae::str("unknown parameter: ", arg).c_str());
      }

      if (al::iends_with(arg, ".yaerx"))
      {
        std::string json_str = yae::TOpenFile(arg.c_str(), "rb").read();

        std::set<std::string> s;
        std::list<yae::ClipInfo> c;
        if (yae::RemuxModel::parse_json_str(json_str, s, redacted, c))
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
        if (yae::convert_path_to_utf8(qarg, filePath))
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
    yae::TDemuxerInterfacePtr demuxer = yae::load(sources,
                                                  clips,
                                                  buffer_duration,
                                                  discont_tolerance);
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

  yae::Application app(argc, argv);
  yae::mainWindow = new yae::MainWindow();

  bool ok = QObject::connect(&app,
                             SIGNAL(file_open(const QString &)),
                             yae::mainWindow,
                             SLOT(loadFile(const QString &)));
  YAE_ASSERT(ok);

  yae::mainWindow->model().redacted_ = redacted;
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
#ifdef _WIN32
    yae::get_main_args_utf8(argc, argv);
#endif

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
