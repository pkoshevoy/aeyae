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
// plugins
//
yae::TPluginRegistry plugins;


//----------------------------------------------------------------
// open_demuxer
//
static yae::TDemuxerPtr
open_demuxer(const char * resourcePath, std::size_t track_offset = 0)
{
  yae::TDemuxerPtr demuxer(new yae::Demuxer(track_offset,
                                            track_offset,
                                            track_offset));

  std::string path(resourcePath);
  if (al::ends_with(path, ".eyetv"))
  {
    std::set<std::string> mpg_path;
    yae::CollectMatchingFiles visitor(mpg_path, "^.+\\.mpg$");
    yae::for_each_file_at(path, visitor);

    if (mpg_path.size() == 1)
    {
      path = *(mpg_path.begin());
    }
  }

  if (!demuxer->open(path.c_str()))
  {
    return yae::TDemuxerPtr();
  }

  return demuxer;
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

  std::list<yae::TDemuxerPtr> src;
  {
    std::string folderPath;
    std::string fileName;
    yae::parseFilePath(filePath, folderPath, fileName);

    std::string baseName;
    std::string ext;
    yae::parseFileName(fileName, baseName, ext);

    src.push_back(open_demuxer(filePath.c_str()));
    if (!src.back())
    {
      // failed to open the primary resource:
      return -3;
    }

    yae::Demuxer & primary = *(src.back().get());
    std::cout
      << "file opened: " << filePath
      << ", programs: " << primary.programs().size()
      << ", a: " << primary.audioTracks().size()
      << ", v: " << primary.videoTracks().size()
      << ", s: " << primary.subttTracks().size()
      << std::endl;

    if (primary.programs().size() < 2)
    {
      // add auxiliary resources:
      std::size_t trackOffset = 100;
      yae::TOpenFolder folder(folderPath);
      while (folder.parseNextItem())
      {
        std::string nm = folder.itemName();
        if (!al::starts_with(nm, baseName) || nm == fileName)
        {
          continue;
        }

        src.push_back(open_demuxer(folder.itemPath().c_str(), trackOffset));
        if (!src.back())
        {
          src.pop_back();
          continue;
        }

        yae::Demuxer & aux = *(src.back().get());
        std::cout
          << "file opened: " << nm
          << ", programs: " << aux.programs().size()
          << ", a: " << aux.audioTracks().size()
          << ", v: " << aux.videoTracks().size()
          << ", s: " << aux.subttTracks().size()
          << std::endl;

        trackOffset += 100;
      }
    }
  }

  while (true)
  {
    bool demuxed = false;

    for (std::list<yae::TDemuxerPtr>::const_iterator
           i = src.begin(); i != src.end(); ++i)
    {
      yae::Demuxer & demuxer = *(i->get());

      yae::TPacketPtr packet(new yae::AvPkt());
      yae::AvPkt & pkt = *packet;
      int err = demuxer.demux(pkt);
      if (!err)
      {
        yae::TrackPtr track = demuxer.getTrack(pkt.trackId_);

        demuxed = true;
        std::cout
          << pkt.trackId_
          << ", pos: " << std::setw(12) << std::setfill(' ') << pkt.pos
          << ", size: " << std::setw(6) << std::setfill(' ') << pkt.size;

        YAE_ASSERT(track);
        if (track)
        {
          const AVStream & stream = track->stream();

          if (pkt.dts != AV_NOPTS_VALUE)
          {
            yae::TTime dts(stream.time_base.num * pkt.dts,
                           stream.time_base.den);

            std::string tc = dts.to_hhmmss_frac(1000, ":", ".");
            std::cout << ", dts: " << tc;
          }

          if (pkt.pts != AV_NOPTS_VALUE)
          {
            yae::TTime pts(stream.time_base.num * pkt.pts,
                           stream.time_base.den);

            std::string tc = pts.to_hhmmss_frac(1000, ":", ".");
            std::cout << ", pts: " << tc;
          }

          if (pkt.duration)
          {
            yae::TTime dur(stream.time_base.num * pkt.duration,
                           stream.time_base.den);

            std::string tc = dur.to_hhmmss_frac(1000, ":", ".");
            std::cout << ", dur: " << tc;
          }
        }

        const AVFormatContext & ctx = demuxer.getFormatContext();
        const AVStream & stream = *(ctx.streams[pkt.stream_index]);
        const AVMediaType codecType = stream.codecpar->codec_type;

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
    }

    if (!demuxed)
    {
      break;
    }
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
