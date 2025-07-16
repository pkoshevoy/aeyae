// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/video/yae_mp4.h"
#include "yae/video/yae_mpeg_ts.h"

// system:
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#ifdef check
#undef check
#endif
#endif

// Qt:
#include <QCoreApplication>
#include <QDir>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QSurfaceFormat>
#endif

// standard:
#include <iomanip>
#include <iostream>
#include <stdexcept>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#endif

YAE_ENABLE_DEPRECATION_WARNINGS

// ffmpeg includes:
extern "C"
{
#include <libavutil/md5.h>
}

// yaeui:
#include "yaeUtilsQt.h"

// local:
#include "yaeMainWindow.h"
#include "yae_dvr.h"
#include "yae_hdhomerun.h"
#include "yae_signal_handler.h"

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // mainWindow
  //
  MainWindow * mainWindow = NULL;

  //----------------------------------------------------------------
  // StreamDumper
  //
  struct StreamDumper : yae::mpeg_ts::IPacketHandler
  {
    yae::mpeg_ts::Context ctx_;

    // buffer packets until we have enough info (EPG)
    // to enable us to handle them properly:
    yae::fifo<Packet> packets_;

    // where to dump the programs:
    std::string basedir_;

    // record each (PMT) program separately:
    std::map<uint16_t, yae::TOpenFilePtr> files_;

    StreamDumper(const std::string & basedir):
      ctx_("StreamDumper"),
      packets_(40000000), // 7520MB
      basedir_(basedir)
    {}

    ~StreamDumper()
    {
      handle_backlog();
    }

    yae::TOpenFile &
    get_file_for(uint16_t program_id)
    {
      yae::TOpenFilePtr & file = files_[program_id];
      if (!file)
      {
        std::string fn = yae::strfmt("pid-%04X.mpg", program_id);
        std::string mpg = (fs::path(basedir_) / fn).string();
        yae_info << "will write to " << mpg;

        file = yae::get_open_file(mpg.c_str(), "wb");
      }

      return *file;
    }

    // virtual:
    void handle(const yae::mpeg_ts::IPacketHandler::Packet & packet,
                const yae::mpeg_ts::Bucket & bucket,
                uint32_t gps_time)
    {
      packets_.push(packet);

      uint16_t program_id = ctx_.lookup_program_id(packet.pid_);
      if (!program_id)
      {
        return;
      }

      if (!packets_.full())
      {
        return;
      }

      // consume the backlog:
      handle_backlog();
    }

    void handle_backlog()
    {
      yae::mpeg_ts::IPacketHandler::Packet packet;
      while (packets_.pop(packet))
      {
        const yae::Data & data = packet.data_;

        uint16_t program_id = ctx_.lookup_program_id(packet.pid_);
        if (!program_id)
        {
          for (std::map<uint16_t, yae::TOpenFilePtr>::const_iterator
                 i = files_.begin(); i != files_.end(); ++i)
          {
            yae::TOpenFile & f = *(i->second);
            f.write(data.get(), data.size());
          }
        }
        else
        {
          yae::TOpenFile & f = get_file_for(program_id);
          f.write(data.get(), data.size());
        }
      }
    }
  };

  //----------------------------------------------------------------
  // Mp4ParserCallback
  //
  struct Mp4ParserCallback : yae::Mp4Context::ParserCallbackInterface
  {
    Mp4ParserCallback(yae::mp4::TBoxPtrVec & boxes,
                      Json::Value & out):
      boxes_(boxes),
      out_(out)
    {}

    // virtual:
    bool observe(const yae::Mp4Context & mp4,
                 const yae::mp4::TBoxPtr & box)
    {
      boxes_.push_back(box);

      Json::Value v;
      box->to_json(v);
      out_.append(v);

      // keep parsing:
      return true;
    }

    yae::mp4::TBoxPtrVec & boxes_;
    Json::Value & out_;
  };

  //----------------------------------------------------------------
  // parse_mp4
  //
  static void
  parse_mp4(const char * src_path, const char * dst_path)
  {
    yae::Mp4Context mp4;
    mp4.load_mdat_data_ = false;
    mp4.parse_mdat_data_ = false;

    yae::mp4::TBoxPtrVec top_level_boxes;
    Json::Value out;
    Mp4ParserCallback cb(top_level_boxes, out);

    bool parsed_without_errors = mp4.parse_file(src_path, &cb);
    yae_ilog("finished parsing %s %s errors",
             src_path,
             parsed_without_errors ? "without" : "with");

    yae_ilog("saving %s as JSON to %s", src_path, dst_path);
    YAE_THROW_IF(!yae::TOpenFile(dst_path, "wb").save(out, "  "));

    yae::Timeline timeline;
    yae::get_timeline(top_level_boxes, timeline);
    yae_info << timeline;

    for (yae::Timeline::TTracks::const_iterator
           i = timeline.tracks_.begin(); i != timeline.tracks_.end(); ++i)
    {
      const std::string & track_id = i->first;
      const yae::Timeline::Track & track = i->second;

      std::ostringstream oss;
      for (std::size_t k = 1, n = track.dts_.size(); k < n; ++k)
      {
        std::size_t j = k - 1;
        const TTime & pts_j = track.pts_[j];
        const TTime & dts_j = track.dts_[j];
        const TTime & dts_k = track.dts_[k];
        double dur_j = (dts_k - dts_j).sec();
        oss << "pts[" << j << "] = " << pts_j.to_hhmmss_ms()
            << ", dts[" << j << "] = " << dts_j.to_hhmmss_ms()
            << ", actual dur = " << dur_j << "\n";
      }

      yae_info
        << "\ntrack: " << track_id
        << "\n" << oss.str()
        << "\n";
    }
  }

  //----------------------------------------------------------------
  // parse_mpeg_ts
  //
  static void
  parse_mpeg_ts(const char * fn,
                const char * dst_path,
                std::size_t pkt_size = 188)
  {
    yae::TOpenFile src(fn, "rb");
    YAE_THROW_IF(!src.is_open());

    StreamDumper handler(dst_path);
    while (!src.is_eof())
    {
      yae::Data data(12 + 7 * pkt_size);
      uint64_t pos = yae::ftell64(src.file_);

      std::size_t n = src.read(data.get(), data.size());
      if (n < pkt_size)
      {
        break;
      }

      data.truncate(n);

      std::size_t offset = 0;
      while (offset + pkt_size <= n)
      {
        // find to the the sync byte:
        if (data[offset] == 0x47 &&
            (n - offset == pkt_size || data[offset + pkt_size] == 0x47))
        {
          try
          {
            // attempt to parse the packet:
            yae::TBufferPtr pkt_data = data.get(offset, pkt_size);
            pkt_data->truncate(188);

            yae::Bitstream bin(pkt_data);

            yae::mpeg_ts::TSPacket pkt;
            pkt.load(bin);

            std::size_t end_pos = bin.position();
            std::size_t bytes_consumed = end_pos >> 3;

            if (bytes_consumed < 188)
            {
              yae_wlog("TS packet too short (%i bytes), %s ...",
                       bytes_consumed,
                       yae::to_hex(pkt_data->get(), 32, 4).c_str());
              offset += pkt_size;
              continue;
            }

            handler.ctx_.push(pkt);

            yae::mpeg_ts::IPacketHandler::Packet packet(pkt.pid_, pkt_data);
            handler.ctx_.handle(packet, handler);
          }
          catch (const std::exception & e)
          {
            yae_wlog("failed to parse TS packet at %" PRIu64 ", %s",
                     pos + offset, e.what());
          }
          catch (...)
          {
            yae_wlog("failed to parse TS packet at %" PRIu64
                     ", unexpected exception",
                     pos + offset);
          }

          // skip to next packet:
          offset += pkt_size;
        }
        else
        {
          offset++;
        }
      }

      yae::fseek64(src.file_, pos + offset, SEEK_SET);
    }

    handler.ctx_.dump();
  }

  //----------------------------------------------------------------
  // calc_pkt_md5
  //
  static void
  calc_pkt_md5(const char * fn,
               const char * dst_path,
               std::size_t pkt_size = 188)
  {
    yae::TOpenFile src(fn, "rb");
    YAE_THROW_IF(!src.is_open());

    std::string fn_pos = std::string(dst_path) + ".pkt-pos";
    yae::TOpenFile dst_pos(fn_pos.c_str(), "wb");
    YAE_THROW_IF(!dst_pos.is_open());

    std::string fn_md5 = std::string(dst_path) + ".pkt-md5";
    yae::TOpenFile dst_md5(fn_md5.c_str(), "wb");
    YAE_THROW_IF(!dst_md5.is_open());

    unsigned char md5[16] = { 0 };
    while (!src.is_eof())
    {
      yae::Data data(12 + 7 * pkt_size);
      uint64_t pos = yae::ftell64(src.file_);

      std::size_t n = src.read(data.get(), data.size());
      if (n < pkt_size)
      {
        break;
      }

      data.truncate(n);

      std::size_t offset = 0;
      while (offset + pkt_size <= n)
      {
        // find to the the sync byte:
        if (data[offset] == 0x47 &&
            (n - offset == pkt_size || data[offset + pkt_size] == 0x47))
        {
          av_md5_sum(md5, data.get() + offset, pkt_size - 1);

          fprintf(dst_pos.file_, "%016" PRIx64 "\n", pos + offset);
          fprintf(dst_md5.file_,
                  "%02x%02x%02x%02x "
                  "%02x%02x%02x%02x "
                  "%02x%02x%02x%02x "
                  "%02x%02x%02x%02x\n",
                  md5[0],
                  md5[1],
                  md5[2],
                  md5[3],
                  md5[4],
                  md5[5],
                  md5[6],
                  md5[7],
                  md5[8],
                  md5[9],
                  md5[10],
                  md5[11],
                  md5[12],
                  md5[13],
                  md5[14],
                  md5[15]);

          // skip to next packet:
          offset += pkt_size;
        }
        else
        {
          offset++;
        }
      }

      yae::fseek64(src.file_, pos + offset, SEEK_SET);
    }
  }

  //----------------------------------------------------------------
  // signal_callback_dvr
  //
  static void
  signal_callback_dvr(void * context, int sig)
  {
    (void)sig;
    DVR::ServiceLoop * service_loop = (DVR::ServiceLoop *)context;

    if (signal_handler_received_sigpipe() ||
        signal_handler_received_sigint())
    {
      service_loop->cancel();
    }
  }

  //----------------------------------------------------------------
  // signal_callback_app
  //
  static void
  signal_callback_app(void * context, int sig)
  {
    (void)sig;
    Application * app = (Application *)(context);

    if (signal_handler_received_sigint())
    {
      yae::queue_call(*mainWindow, &MainWindow::exitConfirmed);
    }
  }


  //----------------------------------------------------------------
  // usage
  //
  static void
  usage(char ** argv, const char * message)
  {
    std::cerr
      << "\nUSAGE:\n"
      << argv[0]
      << " \\\n [--no-ui]"
      << " \\\n [-b|--base-dir "
      << (fs::path("path") / "to" / "storage" / "yaetv").string()
      << "] \\\n [--pkt-size 188|192|204] "
      << " \\\n [--pkt-md5 "
      << (fs::path("path") / "to" / "file.mpg").string()
      << " ["
      << (fs::path("output") / "path").string()
      << "]] \\\n [--parse "
      << (fs::path("path") / "to" / "file.mpg").string()
      << " "
      << (fs::path("output") / "path").string()
      << "]\n";

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
  }

  //----------------------------------------------------------------
  // usage
  //
  static void
  usage(char ** argv, const std::string & msg)
  {
    usage(argv, msg.c_str());
  }


  //----------------------------------------------------------------
  // main_may_throw
  //
  int
  main_may_throw(int argc, char ** argv)
  {
    std::string yaetv_dir = yae::get_user_folder_path(".yaetv");
    YAE_THROW_IF(!yae::mkdir_p(yaetv_dir));

    // instantiate the logger:
    {
      cleanup_yaetv_logs(yaetv_dir);

      yae::TLog & logger = yae::logger();

      std::string ts =
        unix_epoch_time_to_localtime_str(TTime::now().get(1), "", "-", "");

      fs::path log_path =
        fs::path(yaetv_dir) / yae::strfmt("yaetv-%s.log", ts.c_str());

      logger.assign(std::string("yaetv"), new LogToFile(log_path.string()));
    }

    // install signal handler:
    yae::signal_handler();

    // parse input parameters:
    std::string appearance;
    std::string basedir;
    std::size_t pkt_size = 188;
    bool no_ui = false;

    for (int i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-b") == 0 ||
          strcmp(argv[i], "--base-dir") == 0)
      {
        if (argc <= i + 1)
        {
          usage(argv, "-b parameter requires a /file/path");
          return i;
        }

        ++i;
        basedir = argv[i];
      }
      else if (strcmp(argv[i], "--no-ui") == 0)
      {
        no_ui = true;
      }
      else if (strcmp(argv[i], "--pkt-size") == 0)
      {
        if (argc <= i + 1)
        {
          usage(argv, "--pkt-size requires a value: 188");
          return i;
        }

        ++i;
        pkt_size = boost::lexical_cast<std::size_t>(argv[i]);
      }
      else if (strcmp(argv[i], "--pkt-md5") == 0)
      {
        const char * out_path = argv[i + 1];
        if (i + 2 < argc)
        {
          out_path = argv[i + 2];
          return i;
        }

        calc_pkt_md5(argv[i + 1], out_path, pkt_size);
        return 0;
      }
      else if (strcmp(argv[i], "--parse") == 0)
      {
        if (argc <= i + 2)
        {
          usage(argv, "--parse needs 2 params: /path/input.mpg /output/path");
          return i;
        }

        const char * src = argv[i + 1];
        const char * dst = argv[i + 2];

        std::string basedir;
        std::string basename;
        yae::parse_file_path(src, basedir, basename);

        std::string src_name;
        std::string src_ext;
        yae::parse_file_name(basename, src_name, src_ext);

        if ((src_ext == "mp4") ||
            (src_ext == "m4f") ||
            (src_ext == "m4s") ||
            (src_ext == "m4a") ||
            (src_ext == "m4v") ||
            (src_ext == "mov") ||
            (src_ext == "qt"))
        {
          parse_mp4(src, dst);
        }
        else
        {
          parse_mpeg_ts(src, dst, pkt_size);
        }
        return 0;
      }
#ifdef __APPLE__
      else if (al::starts_with(argv[i], "-psn_"))
      {
        // ignore, OSX adds it when double-clicking on the app.
        continue;
      }
#endif
      else
      {
        usage(argv, strfmt("unrecognized parameter: %s", argv[i]));
        return i;
      }
    }

    if (basedir.empty())
    {
      std::string path = (fs::path(yaetv_dir) / "settings.json").string();
      Json::Value json;
      if (yae::TOpenFile(path, "rb").load(json))
      {
        basedir = json.get("basedir", "").asString();
        appearance = json.get("appearance", std::string("auto")).asString();
      }
    }

    if (no_ui)
    {
      DVR dvr(yaetv_dir, basedir);

      DVR::ServiceLoop service_loop(dvr);
      signal_handler().add(&signal_callback_dvr, &service_loop);

#if 0
      // NOTE: this list will not include any tuners that are currently
      // locked (in use by another application):
      std::list<std::string> available_tuners;
      dvr.hdhr_.discover_tuners(available_tuners);

      for (std::list<std::string>::const_iterator
             i = available_tuners.begin(); i != available_tuners.end(); ++i)
      {
        const std::string & tuner_name = *i;

        // NOTE: this can take a while if there aren't cached channel scan
        // resuts for this tuner:
        YAE_EXPECT(dvr.hdhr_.init(tuner_name));
      }
#endif

      yae::Worker dummy;
      service_loop.execute(dummy);
      dvr.shutdown();
      return 0;
    }

    yae::kApplication = QString::fromUtf8("yaetv");
    yae::kOrganization = QString::fromUtf8("PavelKoshevoy");
    yae::Application::setApplicationName(yae::kApplication);
    yae::Application::setOrganizationName(yae::kOrganization);
    yae::Application::setOrganizationDomain("sourceforge.net");

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  yae::Application::setDesktopFileName("yaetv");
#endif

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

    yae::Application app(argc, argv);
    yae::mainWindow = new yae::MainWindow(yaetv_dir, basedir);

    bool ok = true;
    ok = QObject::connect(&app,
                          SIGNAL(theme_changed(const yae::Application &)),
                          yae::mainWindow,
                          SLOT(themeChanged(const yae::Application &)));
    YAE_ASSERT(ok);

    // initialize application style:
    app.set_appearance(appearance);

    yae::mainWindow->show();
    yae::mainWindow->initItemViews();

    signal_handler().add(&signal_callback_app, &app);
    app.exec();

    delete yae::mainWindow;
    yae::mainWindow = NULL;

    return 0;
  }
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
      HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
      if (stdout_handle != INVALID_HANDLE_VALUE)
      {
        COORD console_buffer_size;
        console_buffer_size.X = 80;
        console_buffer_size.Y = 9999;
        SetConsoleScreenBufferSize(stdout_handle, console_buffer_size);
      }
    }
#endif

    // configure console for UTF-8 output:
    yae::set_console_output_utf8();

#ifdef _WIN32
    // initialize network socket support:
    {
      WORD version_requested = MAKEWORD(2, 0);
      WSADATA wsa_data;
      WSAStartup(version_requested, &wsa_data);
    }
#endif

    // Create and install global locale (UTF-8)
    {
#ifndef _WIN32
      const char * lc_type = getenv("LC_TYPE");
      const char * lc_all = getenv("LC_ALL");
      const char * lang = getenv("LANG");

      if (!(lc_type || lc_all || lang))
      {
        // avoid crasing in boost+libiconv:
        setenv("LANG", "en_US.UTF-8", 1);
      }
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

#if defined(__APPLE__) // && (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    const uint32_t macos_major_minor = yae::get_macos_semver_u32(1000) / 1000;
    const uint32_t macos_mavericks = yae::semver_u32(10,  9, 0, 1000) / 1000;
    const uint32_t macos_yosemite = yae::semver_u32(10, 10, 0, 1000) / 1000;

    if (macos_major_minor == macos_mavericks)
    {
      // add a workaround for Qt 4.7 QTBUG-32789
      // that manifests as misaligned text on OS X Mavericks:
      QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    }
    else if (macos_major_minor >= macos_yosemite)
    {
      // add a workaround for Qt 4.8 QTBUG-40833
      // that manifests as misaligned text on OS X Yosemite:
      QFont::insertSubstitution(".Helvetica Neue DeskInterface",
                                "Helvetica Neue");
    }
#endif

    yae::get_main_args_utf8(argc, argv);
    r = yae::main_may_throw(argc, argv);
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
