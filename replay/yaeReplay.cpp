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

  std::string filePath;
  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    filePath = i->toUtf8().constData();
    break;
  }

  std::list<yae::TDemuxerPtr> demuxers;
  if (!yae::open_primary_and_aux_demuxers(filePath, demuxers))
  {
    // failed to open the primary resource:
    return -3;
  }

  // wrap each demuxer in a DemuxerBuffer, build a summary:
  std::list<yae::TDemuxerInterfacePtr> src;
  {
    // target buffer duration, expressed in seconds:
    double duration = 1.0;

    for (std::list<yae::TDemuxerPtr>::const_iterator
           i = demuxers.begin(); i != demuxers.end(); ++i)
    {
      yae::TDemuxerInterfacePtr buffer(new yae::DemuxerBuffer(*i, duration));
      src.push_back(buffer);
    }
  }

  // summarize the demuxer(s):
  yae::DemuxerSummary summary;
  {
    double tolerance = 0.017;
    for (std::list<yae::TDemuxerInterfacePtr>::const_iterator
           i = src.begin(); i != src.end(); ++i)
    {
      const yae::TDemuxerInterfacePtr & demuxer = *i;
      summary.summarize(demuxer, tolerance);
    }

    // show the summary:
    std::cout << "\n" << summary << std::endl;
  }

  bool rewind = false;
  bool rewound = false;

  yae::ParallelDemuxer buffer(src);

  // FIXME: debugging only:
  if (summary.timeline_.size() == 1)
  {
    int err = yae::remux("/tmp/replay.mkv", summary, buffer);
    if (err < 0)
    {
      buffer.seek(AVSEEK_FLAG_BACKWARD, yae::TTime(0, 1));
      err = yae::remux("/tmp/replay.nut", summary, buffer);
      if (err < 0)
      {
        buffer.seek(AVSEEK_FLAG_BACKWARD, yae::TTime(0, 1));
        err = yae::remux("/tmp/replay.mov", summary, buffer);
        if (err < 0)
        {
          buffer.seek(AVSEEK_FLAG_BACKWARD, yae::TTime(0, 1));
          err = yae::remux("/tmp/replay.mp4", summary, buffer);
        }
      }
    }
  }
  else
  {
    yae::remux("/tmp/replay.ts", summary, buffer);
  }

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
      buffer.seek(seekFlags, seekTime);
      rewound = true;
    }

    AVStream * stream = NULL;
    yae::TPacketPtr packet = buffer.get(stream);
    if (!packet)
    {
      break;
    }

    // shortcut:
    yae::AvPkt & pkt = *packet;

    std::cout
      << pkt.trackId_
      << ", demuxer: " << std::setw(2) << pkt.demuxer_->demuxer_index()
      << ", program: " << std::setw(3) << pkt.program_
      << ", pos: " << std::setw(12) << std::setfill(' ') << pkt.pos
      << ", size: " << std::setw(6) << std::setfill(' ') << pkt.size;

    yae::TTime dts;
    if (yae::get_dts(dts, stream, pkt))
    {
      std::string tc = dts.to_hhmmss_frac(1000, ":", ".");
      std::cout << ", dts: " << tc;

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

    if (pkt.pts != AV_NOPTS_VALUE)
    {
      yae::TTime pts(stream->time_base.num * pkt.pts,
                     stream->time_base.den);

      std::string tc = pts.to_hhmmss_frac(1000, ":", ".");
      std::cout << ", pts: " << tc;
    }

    if (pkt.duration)
    {
      yae::TTime dur(stream->time_base.num * pkt.duration,
                     stream->time_base.den);

      std::string tc = dur.to_hhmmss_frac(1000, ":", ".");
      std::cout << ", dur: " << tc;
    }

    const AVMediaType codecType = stream->codecpar->codec_type;

    int flags = pkt.flags;
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

    for (int j = 0; j < pkt.side_data_elems; j++)
    {
      std::cout
        << ", side_data[" << j << "] = { type: "
        << pkt.side_data[j].type << ", size: "
        << pkt.side_data[j].size << " }";
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
