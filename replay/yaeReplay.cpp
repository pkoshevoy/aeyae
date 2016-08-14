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

#include <iostream>
#include <limits>
#include <stdexcept>

// boost:
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
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_reader.h"
#include "yae/video/yae_video.h"
#include "yae/utils/yae_utils.h"

// local includes:
#include <yaeUtilsQt.h>


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

  QString fn;
  for (QStringList::const_iterator i = args.begin() + 1; i != args.end(); ++i)
  {
    fn = *i;
    break;
  }

  //----------------------------------------------------------------
  // readerPrototype
  //
  yae::IReaderPtr readerPrototype;

  // load plugins:
  std::string pluginsFolderPath;
  if (yae::getCurrentExecutablePluginsFolder(pluginsFolderPath) &&
      plugins.load(pluginsFolderPath.c_str()))
  {
    std::list<yae::IReaderPtr> readers;
    if (plugins.find<yae::IReader>(readers))
    {
      readerPrototype = readers.front();
    }
  }

  if (!readerPrototype)
  {
    std::cerr
      << "ERROR: failed to find IReader plugin here: "
      << pluginsFolderPath
      << std::endl;
    return -1;
  }

  yae::IReaderPtr reader = yae::openFile(readerPrototype, fn);
  if (!reader)
  {
    std::cerr
      << "ERROR: file open failed for " << fn.toUtf8().constData()
      << std::endl;
    return -2;
  }

  std::cerr
    << "file opened: " << fn.toUtf8().constData()
    << std::endl;

  bool hasVideo = reader->getNumberOfVideoTracks() > 0;
  yae::VideoTraits vtts;
  if (hasVideo)
  {
    reader->selectVideoTrack(0);
    reader->getVideoTraits(vtts);
  }

  yae::TTime frameDuration = yae::frameDurationForFrameRate(vtts.frameRate_);
  double fps = frameDuration.perSecond();
  std::cerr
    << "output fps: " << fps << ", original " << vtts.frameRate_ << '\n'
    << "frame duration: " << frameDuration.time_
    << " / " << frameDuration.base_
    << std::endl;

  bool hasAudio = reader->getNumberOfAudioTracks() > 0;
  yae::AudioTraits atts;
  if (hasAudio)
  {
    reader->selectAudioTrack(0);
    reader->getAudioTraits(atts);

    // override audio traits for something more palettable:
    atts.channelFormat_ = yae::kAudioChannelsPacked;
    atts.sampleFormat_ = yae::kAudio16BitNative;
    bool ok = reader->setAudioTraitsOverride(atts);
    YAE_ASSERT(ok);
  }

  double samplesPerFrame = atts.sampleRate_ / fps;
  std::cerr
    << "audio samples per frame: " << samplesPerFrame
    << std::endl;

  yae::TVideoFramePtr v0;
  yae::TVideoFramePtr v1;
  yae::TAudioFramePtr af;
  yae::QueueWaitMgr terminator;

  reader->threadStart();

  yae::int64 v_t0 = 0;
  yae::int64 v_t1 = 0;
  yae::int64 v_dt = 0;

  yae::int64 a_t0 = 0;
  yae::int64 a_t1 = 0;
  yae::int64 a_dt = 0;

  while (hasVideo && hasAudio)
  {
    // assemble audio/video pairs,
    // 1 image paired with samplerate/fps worth of audio samples
    while (hasVideo)
    {
      std::swap(v_t1, v_t0);
      v0.swap(v1);

      hasVideo = reader->readVideo(v1, &terminator);
      if (!(v1 && v1->data_))
      {
        hasVideo = false;
        v_t1 = v_t0;
        v_dt = 0;
        break;
      }

      v_t1 = v1->time_.getTime(atts.sampleRate_);
      v_dt = v_t1 - v_t0;

      if (v0)
      {
        break;
      }
    }

    // frame duration, nsamples:
    std::cerr
      << "video frame duration: " << v_dt << " / " << atts.sampleRate_
      << " @ " << v0->time_.to_hhmmss_usec()
      << std::endl;

    while (hasAudio && (a_t1 < v_t1))
    {
      hasAudio = reader->readAudio(af, &terminator);
      if (!(af && af->data_))
      {
        a_t0 = a_t1;
        a_dt = 0;
        break;
      }

      a_dt = af->numSamples();
      a_t0 = af->time_.getTime(atts.sampleRate_);
      a_t1 = a_t0 + a_dt;

      std::cerr
        << "audio frame duration: " << a_dt << " / " << atts.sampleRate_
        << " @ " << af->time_.to_hhmmss_usec()
        << std::endl;
    }

    // given v0 and v1 -- calculate exact frame duration of v0,
    // and consume corresponding number of audio samples;
    // push the resulting audio/video pair to output queue

    // FIXME:
    // break;
  }

  reader->threadStop();
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
