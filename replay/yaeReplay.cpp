// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Jul 17 11:05:51 MDT 2016
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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem/path.hpp>

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

// local includes:
#include <yaeUtilsQt.h>

// namespace shortcuts:
namespace al = boost::algorithm;


namespace yae
{

  //----------------------------------------------------------------
  // Application
  //
  class Application : public QCoreApplication
  {
  public:
    Application(int & argc, char ** argv):
      QCoreApplication(argc, argv)
    {
#ifdef __APPLE__
      QString appDir = QCoreApplication::applicationDirPath();
      QString plugInsDir = QDir::cleanPath(appDir + "/../PlugIns");
      QCoreApplication::addLibraryPath(plugInsDir);
#endif
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

  yae::Application app(argc, argv);
  QStringList args = app.arguments();

  boost::shared_ptr<yae::SerialDemuxer>
    serial_demuxer(new yae::SerialDemuxer());
  std::string output_path;

  // these are expressed in seconds:
  const double buffer_duration = 1.0;
  const double discont_tolerance = 0.017;

  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    std::string arg = i->toUtf8().constData();

    if (arg == "-i")
    {
      ++i;
      std::string filePath = i->toUtf8().constData();

      std::list<yae::TDemuxerPtr> demuxers;
      if (!yae::open_primary_and_aux_demuxers(filePath, demuxers))
      {
        // failed to open the primary resource:
        av_log(NULL, AV_LOG_WARNING,
               "failed to open %s, skipping...",
               filePath.c_str());
        continue;
      }

      boost::shared_ptr<yae::ParallelDemuxer>
        parallel_demuxer(new yae::ParallelDemuxer());

      // wrap each demuxer in a DemuxerBuffer, build a summary:
      for (std::list<yae::TDemuxerPtr>::const_iterator
             i = demuxers.begin(); i != demuxers.end(); ++i)
      {
        const yae::TDemuxerPtr & demuxer = *i;

        yae::TDemuxerInterfacePtr
          buffer(new yae::DemuxerBuffer(demuxer, buffer_duration));

        yae::DemuxerSummary summary;
        buffer->summarize(summary, discont_tolerance);

        std::cout
          << "\n" << demuxer->resourcePath() << ":\n"
          << summary << std::endl;

        parallel_demuxer->append(buffer, summary);
      }

      // summarize the demuxer:
      yae::DemuxerSummary summary;
      parallel_demuxer->summarize(summary, discont_tolerance);

      // show the summary:
      std::cout << "\nparallel:\n" << summary << std::endl;

      serial_demuxer->append(parallel_demuxer, summary);
    }
    else if (arg == "-o")
    {
      ++i;
      output_path = i->toUtf8().constData();
    }
  }

  if (serial_demuxer->empty())
  {
    av_log(NULL, AV_LOG_ERROR, "failed to open any input files, done.");
    return 1;
  }

  // summarize the source:
  yae::DemuxerSummary summary;
  serial_demuxer->summarize(summary, discont_tolerance);

  // show the summary:
  std::cout << "\nserial:\n" << summary << std::endl;

  if (!output_path.empty())
  {
    serial_demuxer->seek(AVSEEK_FLAG_BACKWARD, yae::TTime(0, 1));
    int err = yae::remux(output_path.c_str(), summary, *serial_demuxer);
    return err;
  }

  bool rewind = false;
  bool rewound = false;

  std::map<int, yae::TTime> prog_dts;
  while (true)
  {
    if (rewind)
    {
      if (rewound)
      {
        break;
      }

      std::cout
        << "----------------------------------------------------------------"
        << std::endl;

      int seekFlags = AVSEEK_FLAG_BACKWARD;
      yae::TTime seekTime(0, 1);
      serial_demuxer->seek(seekFlags, seekTime);
      rewound = true;
    }

    AVStream * stream = NULL;
    yae::TPacketPtr packet_ptr = serial_demuxer->get(stream);
    if (!packet_ptr)
    {
      break;
    }

    // shortcuts:
    yae::AvPkt & pkt = *packet_ptr;
    AVPacket & packet = pkt.get();

    std::cout
      << pkt.trackId_
      << ", demuxer: " << std::setw(2) << pkt.demuxer_->demuxer_index()
      << ", program: " << std::setw(3) << pkt.program_
      << ", pos: " << std::setw(12) << std::setfill(' ') << packet.pos
      << ", size: " << std::setw(6) << std::setfill(' ') << packet.size;

    yae::TTime dts;
    if (yae::get_dts(dts, stream, packet))
    {
      std::cout << ", dts: " << dts;

      yae::TTime prev_dts =
        yae::get(prog_dts, pkt.program_,
                 yae::TTime(std::numeric_limits<int64_t>::min(), dts.base_));

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

      // rewind = dts.toSeconds() > 120.0;
    }
    else
    {
      // the demuxer should always provide a DTS:
      YAE_ASSERT(false);
    }

    if (packet.pts != AV_NOPTS_VALUE)
    {
      yae::TTime pts(stream->time_base.num * packet.pts,
                     stream->time_base.den);

      std::cout << ", pts: " << pts;
    }

    if (packet.duration)
    {
      yae::TTime dur(stream->time_base.num * packet.duration,
                     stream->time_base.den);

      std::cout << ", dur: " << dur;
    }

    const AVMediaType codecType = stream->codecpar->codec_type;

    int flags = packet.flags;
    if (codecType != AVMEDIA_TYPE_VIDEO)
    {
      flags &= ~(AV_PKT_FLAG_KEY);
    }

    if (flags)
    {
      std::cout << ", flags:";

      if ((flags & AV_PKT_FLAG_KEY))
      {
        std::cout << " keyframe";
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
  }

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
