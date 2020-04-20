// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php


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
#ifdef YAE_USE_QT5
#include <QSurfaceFormat>
#endif

// standard:
#include <iomanip>
#include <iostream>
#include <stdexcept>

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread/thread.hpp>
#endif

// aeyae:
#include "yae/api/yae_log.h"
#include "yae/api/yae_version.h"

// local:
#include "yaeMainWindow.h"
#include "yae_dvr.h"
#include "yae_hdhomerun.h"
#include "yae_live_reader.h"
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
  };


  //----------------------------------------------------------------
  // parse_mpeg_ts
  //
  static void
  parse_mpeg_ts(const char * fn)
  {
    yae::TOpenFile src(fn, "rb");
    YAE_THROW_IF(!src.is_open());

    yae::mpeg_ts::Context ts_ctx;
    while (!src.is_eof())
    {
      yae::Data data(12 + 7 * 188);
      uint64_t pos = yae::ftell64(src.file_);

      std::size_t n = src.read(data.get(), data.size());
      if (n < 188)
      {
        break;
      }

      data.truncate(n);

      std::size_t offset = 0;
      while (offset + 188 <= n)
      {
        // find to the the sync byte:
        if (data[offset] == 0x47 &&
            (n - offset == 188 || data[offset + 188] == 0x47))
        {
          try
          {
            // attempt to parse the packet:
            yae::TBufferPtr pkt_data = data.get(offset, 188);
            yae::Bitstream bin(pkt_data);

            yae::mpeg_ts::TSPacket pkt;
            pkt.load(bin);

            std::size_t end_pos = bin.position();
            std::size_t bytes_consumed = end_pos >> 3;

            if (bytes_consumed != 188)
            {
              yae_wlog("TS packet too short (%i bytes), %s ...",
                       bytes_consumed,
                       yae::to_hex(pkt_data->get(), 32, 4).c_str());
              continue;
            }

            ts_ctx.push(pkt);
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
          offset += 188;
        }
        else
        {
          offset++;
        }
      }

      yae::fseek64(src.file_, pos + offset, SEEK_SET);
    }
    ts_ctx.dump();
  }

  //----------------------------------------------------------------
  // bootstrap_blacklist
  //
  static void
  bootstrap_blacklist(DVR & dvr)
  {
    dvr.blacklist_.channels_.clear();
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(9, 91));

    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(10, 1));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(10, 2));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(10, 3));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(10, 4));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(11, 2));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(11, 3));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(13, 3));

    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 1));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 2));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 3));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 4));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 5));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(16, 6));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 1));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 2));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 3));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 4));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 5));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 6));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(19, 7));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(20, 1));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(20, 2));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(20, 3));
    // dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(20, 4));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 1));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 2));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 3));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 4));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 5));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 6));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(23, 7));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(24, 1));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(24, 2));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(24, 3));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(24, 4));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(25, 1));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(25, 2));
    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(25, 3));

    dvr.blacklist_.channels_.insert(yae::mpeg_ts::channel_number(50, 1));
    dvr.save_blacklist();
  }

  //----------------------------------------------------------------
  // bootstrap_wishlist
  //
  static void
  bootstrap_wishlist(DVR & dvr)
  {
    dvr.wishlist_.items_.clear();

    // Fox 13.1, Sunday, 6pm - 9pm
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(13, 1);
      item.weekday_mask_.reset(yae::Wishlist::Item::Sun);
      item.when_ = Timespan(TTime(18 * 60 * 60, 1),
                            TTime(21 * 60 * 60, 1));
      item.skip_duplicates_ = true;
    }

    // Movies 14.2, Saturday - Sunday, 11am - 5am
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(14, 2);
      item.weekday_mask_.reset(yae::Wishlist::Item::Sat |
                               yae::Wishlist::Item::Sun);
      item.when_ = Timespan(TTime(11 * 60 * 60, 1),
                            TTime(29 * 60 * 60, 1));
      item.max_recordings_ = 1;
      item.skip_duplicates_ = true;
    }

    // PBS 7.1, Sunday, Nature
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(7, 1);
      item.title_ = "Nature";
      item.max_recordings_ = 12;
      item.skip_duplicates_ = true;
    }

    // PBS 7.1, Sunday, Nova
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(7, 1);
      item.title_ = "Nova";
      item.max_recordings_ = 12;
      item.skip_duplicates_ = true;
    }

    // NHK Newsline:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(9, 1);
      item.title_ = "NHK Newsline";
      item.max_recordings_ = 1;
    }

    // The Simpsons:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(13, 1);
      item.title_ = "The Simpsons";
      item.skip_duplicates_ = true;
    }

    // Bob's Burgers:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "Bob's Burgers";
      item.skip_duplicates_ = true;
    }

    // Family Guy:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "Family Guy";
      item.skip_duplicates_ = true;
    }

    // The Big Bang Theory:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "The Big Bang Theory";
      item.skip_duplicates_ = true;
    }

    // The Late Show With Stephen Colbert:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(2, 1);
      item.title_ = "The Late Show With Stephen Colbert";
      item.max_recordings_ = 5;
      item.skip_duplicates_ = true;
    }

    // Late Night With Seth Meyers:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(5, 1);
      item.title_ = "Late Night With Seth Meyers";
      item.max_recordings_ = 5;
      item.skip_duplicates_ = true;
    }

    // Saturday Night Live:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(5, 1);
      item.title_ = "Saturday Night Live";
      item.max_recordings_ = 10;
      item.skip_duplicates_ = true;
    }

    dvr.save_wishlist();
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
      app->postEvent(mainWindow, new QCloseEvent());
    }
  }

  //----------------------------------------------------------------
  // LogToFile
  //
  struct LogToFile : public IMessageCarrier
  {
    LogToFile(const std::string & path):
      file_(get_open_file(path.c_str(), "wb")),
      threshold_(TLog::kDebug)
    {}

    // virtual:
    void destroy()
    { delete this; }

    //! a prototype factory method for constructing objects of the same kind,
    //! but not necessarily deep copies of the original prototype object:
    // virtual:
    LogToFile * clone() const
    { return new LogToFile(*this); }

    // virtual:
    const char * name() const
    { return "LogToFile"; }

    // virtual:
    const char * guid() const
    { return "6cab86bf-402b-4251-8eae-fe105359bf8b"; }

    // virtual:
    ISettingGroup * settings()
    { return NULL; }

    // virtual:
    int priorityThreshold() const
    { return threshold_; }

    // virtual:
    void setPriorityThreshold(int priority)
    { threshold_ = priority; }

    // virtual:
    void deliver(int priority, const char * source, const char * message)
    {
      if (priority < threshold_)
      {
        return;
      }

      // add timestamp to the message:
      std::ostringstream oss;
      TTime now = TTime::now();
      int64_t now_usec = now.get(1000000);
      oss << yae::unix_epoch_time_to_localtime_str(now.get(1))
          << '.'
          << std::setw(6) << std::setfill('0') << (now_usec % 1000000)
          << " [" << yae::to_str((TLog::TPriority)(priority)) << "] "
          << source << ": "
          << message << std::endl;

      if (file_)
      {
        file_->write(oss.str());
        file_->flush();
      }
    }

  protected:
    TOpenFilePtr file_;
    int threshold_;
  };


  //----------------------------------------------------------------
  // usage
  //
  static void
  usage(char ** argv, const char * message)
  {
    std::cerr
      << "\nUSAGE:\n"
      << argv
      << " -b " << (fs::path("path") / "to" / "storage" / "yaetv").string()
      << " [--parse " << (fs::path("path") / "to" / "file.mpg").string() << "]"
      << " [--no-ui]"
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
    std::string basedir;
    bool no_ui = false;

    for (int i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-b") == 0)
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
      else if (strcmp(argv[i], "--parse") == 0)
      {
        if (argc <= i + 1)
        {
          usage(argv, "--parse parameter requires a /file/path/to/some.mpg");
          return i;
        }

        ++i;
        parse_mpeg_ts(argv[i]);
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
      }
    }

    if (no_ui)
    {
      DVR dvr(yaetv_dir, basedir);

      DVR::ServiceLoop service_loop(dvr);
      signal_handler().add(&signal_callback_dvr, &service_loop);

      // bootstrap_blacklist(dvr);
      // bootstrap_wishlist(dvr);
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

    // instantiate reader prototype:
    yae::IReaderPtr reader(yae::LiveReader::create());

    yae::Application app(argc, argv);
    yae::mainWindow = new yae::MainWindow(yaetv_dir, basedir, reader);
    yae::mainWindow->show();
    yae::mainWindow->initItemViews();

    signal_handler().add(&signal_callback_app, &app);
    app.exec();
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

      std::locale::global(boost::locale::generator().generate(""));
    }

    // Make boost.filesystem use global locale:
    boost::filesystem::path::imbue(std::locale());

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
