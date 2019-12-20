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

// standard:
#include <iomanip>
#include <iostream>
#include <stdexcept>

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/thread/thread.hpp>
#endif

// yae:
#include "yae/api/yae_log.h"

// epg:
#include "yae_dvr.h"
#include "yae_hdhomerun.h"
#include "yae_signal_handler.h"


namespace yae
{

  //----------------------------------------------------------------
  // usage
  //
  static void
  usage(char ** argv, const char * msg)
  {
    yae_elog("ERROR: %s", msg);
    yae_elog("USAGE: %s [-b /yaetv/storage/path]", argv[0]);
  }

  //----------------------------------------------------------------
  // main_may_throw
  //
  int
  main_may_throw(int argc, char ** argv)
  {
    // install signal handler:
    yae::signal_handler();

    std::string basedir;
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
    }

#if 1
    DVR dvr(basedir);

#if 0
    dvr.wishlist_.items_.clear();

    // Fox 13.1, Sunday, 6pm - 9pm
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(13, 1);
      item.weekday_mask_.reset(yae::Wishlist::Item::Sun);
      item.when_ = Timespan(TTime(18 * 60 * 60, 1),
                            TTime(21 * 60 * 60, 1));
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
    }

    // PBS 7.1, Sunday, Nature
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(7, 1);
      item.title_ = "Nature";
    }

    // PBS 7.1, Sunday, Nova
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(7, 1);
      item.title_ = "Nova";
    }

    // NHK Newsline:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(9, 1);
      item.title_ = "NHK Newsline";
    }

    // The Simpsons:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(13, 1);
      item.title_ = "The Simpsons";
    }

    // Bob's Burgers:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "Bob's Burgers";
    }

    // Family Guy:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "Family Guy";
    }

    // The Big Bang Theory:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.title_ = "The Big Bang Theory";
    }

    // The Late Show With Stephen Colbert:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(2, 1);
      item.title_ = "The Late Show With Stephen Colbert";
    }

    // Late Night With Seth Meyers:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(5, 1);
      item.title_ = "Late Night With Seth Meyers";
    }

    // Saturday Night Live:
    {
      dvr.wishlist_.items_.push_back(Wishlist::Item());
      Wishlist::Item & item = dvr.wishlist_.items_.back();
      item.channel_ = std::pair<uint16_t, uint16_t>(5, 1);
      item.title_ = "Saturday Night Live";
    }

    // FIXME: there is probably a better place for this:
    dvr.save_wishlist();
#endif

    TTime channel_scan_time = TTime::now();
    TTime epg_update_time = TTime::now();
    dvr.update_epg();

    // pull EPG, evaluate wishlist, start captures, etc...
    yae::mpeg_ts::EPG epg;

    while (!signal_handler_received_sigpipe() &&
           !signal_handler_received_sigint())
    {
      dvr.get_epg(epg);

      if (dvr.load_wishlist())
      {
        dvr.save_schedule();
      }

#ifndef NDEBUG
      {
        Json::Value json;
        yae::mpeg_ts::save(json, epg);
        yae::TOpenFile((dvr.yaepg_ / "epg.json").string(), "wb").save(json);
      }
#endif

      dvr.evaluate(epg);

      if (dvr.worker_.is_idle())
      {
        TTime now = TTime::now();
        double sec_since_channel_scan = (now - channel_scan_time).sec();
        double sec_since_epg_update = (now - epg_update_time).sec();

        if (sec_since_channel_scan > dvr.channel_scan_period_.sec())
        {
          dvr.scan_channels();
          channel_scan_time = TTime::now();
        }
        else if (sec_since_epg_update > dvr.epg_refresh_period_.sec())
        {
          dvr.update_epg(true);
          epg_update_time = now;
        }
      }

      boost::this_thread::sleep_for(boost::chrono::seconds(30));
    }

    dvr.shutdown();

#else
#if 0
    const char * fn = "/tmp/473000000.ts"; // 10.1
    const char * fn = "/tmp/479000000.ts"; // 23.1 KBTU
    const char * fn = "/tmp/491000000.ts"; // 11.1 KBYU
    const char * fn = "/tmp/503000000.ts"; // 14.1 KJZZ
    const char * fn = "/tmp/509000000.ts"; // 20.1 KTMW
    const char * fn = "/tmp/515000000.ts"; // 50.1 KEJT
    const char * fn = "/tmp/527000000.ts"; // 5.1 KSL
    const char * fn = "/tmp/533000000.ts"; // 24.1 KPNZ
    const char * fn = "/tmp/539000000.ts"; // 25.1 KSVN
    const char * fn = "/tmp/551000000.ts"; // 7.1 KUED
    const char * fn = "/tmp/557000000.ts"; // 13.1 KSTU
    const char * fn = "/tmp/563000000.ts"; // 16.1 ION
    const char * fn = "/tmp/569000000.ts"; // 4.1 KTVX
    const char * fn = "/tmp/593000000.ts"; // 2.1 KUTV
    const char * fn = "/tmp/599000000.ts"; // 30.1 KUCW
#endif
    const char * fn = "/tmp/605000000.ts"; // 9.1 KUEN
#if 0
#endif

#if 0
    const char * fn = "/scratch/DataSets/Video/epg/473000000.ts"; // 10.1
    const char * fn = "/scratch/DataSets/Video/epg/479000000.ts"; // 23.1 KBTU
    const char * fn = "/scratch/DataSets/Video/epg/491000000.ts"; // 11.1 KBYU
    const char * fn = "/scratch/DataSets/Video/epg/503000000.ts"; // 14.1 KJZZ
    const char * fn = "/scratch/DataSets/Video/epg/509000000.ts"; // 20.1 KTMW
    const char * fn = "/scratch/DataSets/Video/epg/515000000.ts"; // 50.1 KEJT
    const char * fn = "/scratch/DataSets/Video/epg/527000000.ts"; // 5.1 KSL
    const char * fn = "/scratch/DataSets/Video/epg/533000000.ts"; // 24.1 KPNZ
    const char * fn = "/scratch/DataSets/Video/epg/539000000.ts"; // 25.1 KSVN
#endif
#if 0
    const char * fn = "/scratch/DataSets/Video/epg/551000000.ts"; // 7.1 KUED
    const char * fn = "/scratch/DataSets/Video/epg/557000000.ts"; // 13.1 KSTU
    const char * fn = "/scratch/DataSets/Video/epg/563000000.ts"; // 16.1 ION
    const char * fn = "/scratch/DataSets/Video/epg/569000000.ts"; // 4.1 KTVX
    const char * fn = "/scratch/DataSets/Video/epg/593000000.ts"; // 2.1 KUTV
    const char * fn = "/scratch/DataSets/Video/epg/599000000.ts"; // 30.1 KUCW
    const char * fn = "/scratch/DataSets/Video/epg/605000000.ts"; // 9.1 KUEN
#endif

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
#endif

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
    // Create and install global locale (UTF-8)
    {
#ifdef _WIN32
      // configure console for UTF-8 output:
      SetConsoleOutputCP(CP_UTF8);

      // initialize network socket support:
      WORD version_requested = MAKEWORD(2, 0);
      WSADATA wsa_data;
      WSAStartup(version_requested, &wsa_data);
#else
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

    yae::set_console_output_utf8();
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
