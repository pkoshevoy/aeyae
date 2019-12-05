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
  // main_may_throw
  //
  int
  main_may_throw(int argc, char ** argv)
  {
    // install signal handler:
    yae::signal_handler();

#if 1
    DVR dvr;

    dvr.scan_channels();
    dvr.worker_.wait_until_finished();

    dvr.update_epg();
    dvr.worker_.wait_until_finished();

    yae::mpeg_ts::EPG epg;
    uint32_t ch13p1 = yae::mpeg_ts::channel_number(13, 1);
    DVR::TStreamPtr stream_ptr;

    // FIXME: pull EPG, evaluate wishlist, start captures, etc...
    while (!signal_handler_received_sigpipe() &&
           !signal_handler_received_sigint())
    {
      dvr.get_epg(epg);

      if (!stream_ptr)
      {
        std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
          found = epg.channels_.find(ch13p1);

        if (found != epg.channels_.end())
        {
          uint32_t ch_num = found->first;
          const yae::mpeg_ts::EPG::Channel & channel = found->second;

          for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator i =
                 channel.programs_.begin(); i != channel.programs_.end(); ++i)
          {
            const yae::mpeg_ts::EPG::Program & program = *i;
            uint32_t t0 = program.gps_time_;
            uint32_t t1 = program.duration_ + t0;

            if (t0 <= channel.gps_time_ + 12 && channel.gps_time_ + 12 < t1)
            {
              std::map<uint32_t, std::string> frequencies;
              dvr.hdhr_.get_channels(frequencies);

              uint64_t num_sec = t1 - channel.gps_time_;
              std::string frequency = yae::at(frequencies, ch_num);
              stream_ptr = dvr.capture_stream(frequency, TTime(num_sec, 1));
              // dvr.worker_.wait_until_finished();
            }
          }
        }

        if (!stream_ptr)
        {
          boost::this_thread::sleep_for(boost::chrono::seconds(12));
          continue;
        }
      }
      else if (!stream_ptr->is_open())
      {
        break;
      }
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
            yae::Bitstream bs(data.get(offset, 188));

            yae::mpeg_ts::TSPacket pkt;
            ts_ctx.load(bs, pkt);
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
