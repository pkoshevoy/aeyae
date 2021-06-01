// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard libraries:
#include <iostream>
#include <sstream>

// Qt includes:
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QString>

// yae includes:
#include "yae/utils/yae_utils.h"

// local includes:
#include "yaeLibass.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // getFontsConf
  //
  bool
  getFontsConf(std::string & fontsConf, bool & removeAfterUse)
  {
#if !defined(_WIN32)
    fontsConf = "/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the system fontconfig file:
      removeAfterUse = false;
      return true;
    }
#endif

    removeAfterUse = true;
    int64 appPid = QCoreApplication::applicationPid();

    QString tempDir = YAE_STANDARD_LOCATION(TempLocation);
    QString fontsDir = YAE_STANDARD_LOCATION(FontsLocation);
    QString cacheDir = YAE_STANDARD_LOCATION(CacheLocation);

    QString fontconfigCache =
      cacheDir + QString::fromUtf8("/apprenticevideo-fontconfig-cache");

    std::ostringstream os;
    os << "<?xml version=\"1.0\"?>\n"
       << "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n"
       << "<fontconfig>\n"
       << "\t<dir>"
       << QDir::toNativeSeparators(fontsDir).toUtf8().constData()
       << "</dir>\n";

#ifndef _WIN32
    const char * fontdir[] = {
      "/usr/share/fonts",
      "/usr/X11R6/lib/X11/fonts",
      "/opt/kde3/share/fonts",
      "/usr/local/share/fonts",
      "/Library/Fonts",
      "/Library/Application Support/Apple/Fonts/Language Support",
      "/Library/Application Support/Apple/Fonts/iLife",
      "/Library/Application Support/Apple/Fonts/iWork",
      "/System/Library/Fonts"
    };

    std::size_t nfontdir = sizeof(fontdir) / sizeof(fontdir[0]);
    for (std::size_t i = 0; i < nfontdir; i++)
    {
      QString path = QString::fromUtf8(fontdir[i]);
      if (QFileInfo(path).exists())
      {
        os << "\t<dir>" << fontdir[i] << "</dir>\n";
      }
    }
#endif

    os << "\t<cachedir>"
       << QDir::toNativeSeparators(fontconfigCache).toUtf8().constData()
       << "</cachedir>\n"
       << "</fontconfig>\n";

    QString fn =
      tempDir +
      QString::fromUtf8("/apprenticevideo.fonts.conf.") +
      QString::number(appPid);

    fontsConf = QDir::toNativeSeparators(fn).toUtf8().constData();

#if !defined(NDEBUG)
    yae_debug << "fonts.conf: " << fontsConf;
#endif

    std::string xml = os.str().c_str();
    std::size_t nout = 0;

    try
    {
      TOpenFile out(fontsConf.c_str(), "w");
#if !defined(NDEBUG)
      yae_debug << "fonts.conf content:\n" << xml;
#endif
      nout = fwrite(xml.c_str(), 1, xml.size(), out.file_);
    }
    catch (...)
    {
      return false;
    }

    return nout == xml.size();
  }


  //----------------------------------------------------------------
  // AssTrack::AssTrack
  //
  AssTrack::AssTrack(TLibass & libass,
                     const unsigned char * codecPrivate,
                     const std::size_t codecPrivateSize):
    libass_(libass)
  {
    track_ = ass_new_track(libass_.library_);

    header_.clear();
    if (codecPrivate && codecPrivateSize)
    {
      std::string tmp((const char *)codecPrivate,
                      (const char *)codecPrivate + codecPrivateSize);

      std::string badStyle("Style: Default,(null),0,");
      std::string::size_type found = tmp.find(badStyle);
      if (found != std::string::npos)
      {
        std::ostringstream oss;
        oss << tmp.substr(0, found)
            << "Style: Default,,12,"
            << tmp.substr(found + badStyle.size());
        tmp = oss.str().c_str();
      }

#ifndef NDEBUG
      yae_debug << "libass header:\n" << tmp;
#endif
      header_.assign(&(tmp[0]), &(tmp[0]) + tmp.size());

      ass_process_codec_private(track_,
                                &header_[0],
                                (int)(header_.size()));
    }
  }

  //----------------------------------------------------------------
  // AssTrack::~AssTrack
  //
  AssTrack::~AssTrack()
  {
    ass_free_track(track_);
  }

  //----------------------------------------------------------------
  // AssTrack::flushEvents
  //
  void
  AssTrack::flushEvents()
  {
    ass_flush_events(track_);
    buffer_.clear();
  }

  //----------------------------------------------------------------
  // AssTrack::processData
  //
  void
  AssTrack::processData(const unsigned char * data,
                        std::size_t size,
                        int64 pts)
  {
    if (!libass_.isReady())
    {
      return;
    }

    Dialogue line(pts, data, size);
    if (has(buffer_, line))
    {
#if 0 // ndef NDEBUG
      yae_debug << "ass_process_data: DROPPING DUPLICATE: " << line.data_;
#endif
      return;
    }

#if 0 // ndef NDEBUG
    yae_debug << "ass_process_data: " << line.data_;
#endif

    if (!buffer_.empty())
    {
      const Dialogue & first = buffer_.front();
      if (pts < first.pts_)
      {
        // user skipped back in time, purge cached subs:
        flushEvents();
      }
      else
      {
        buffer_.pop_front();
      }
    }

    buffer_.push_back(line);
    ass_process_data(track_, (char *)data, (int)size);
  }

  //----------------------------------------------------------------
  // AssTrack::renderFrame
  //
  ASS_Image *
  AssTrack::renderFrame(int64 now, int * detectChange)
  {
    return ass_render_frame(libass_.renderer_,
                            track_,
                            (long long)now,
                            detectChange);
  }


  //----------------------------------------------------------------
  // TLibass::TLibass
  //
  TLibass::TLibass():
    initialized_(false),
    library_(NULL),
    renderer_(NULL),
    fonts_(std::numeric_limits<std::size_t>::max()),
    callbackContext_(NULL),
    callback_(NULL)
  {
    library_ = ass_library_init();
    renderer_ = ass_renderer_init(library_);

    fonts_.open();
  }

  //----------------------------------------------------------------
  // TLibass::~TLibass
  //
  TLibass::~TLibass()
  {
    fonts_.close();
    asyncInitStop();

    ass_renderer_done(renderer_);
    renderer_ = NULL;

    ass_library_done(library_);
    library_ = NULL;
  }

  //----------------------------------------------------------------
  // TLibass::asyncInit
  //
  void
  TLibass::asyncInit(TLibassInitDoneCallback callback, void * context)
  {
    if (thread_.isRunning())
    {
      YAE_ASSERT(false);
      return;
    }

    callbackContext_ = context;
    callback_ = callback;

    thread_.set_context(this);
    thread_.run();
  }

  //----------------------------------------------------------------
  // TLibass::asyncInitStop
  //
  void
  TLibass::asyncInitStop()
  {
    thread_.interrupt();
    thread_.wait();
    thread_.set_context(NULL);
  }

  //----------------------------------------------------------------
  // TLibass::isReady
  //
  bool
  TLibass::isReady() const
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    return initialized_;
  }

  //----------------------------------------------------------------
  // TLibass::setFrameSize
  //
  void
  TLibass::setFrameSize(int w, int h)
  {
    ass_set_frame_size(renderer_, w, h);
    ass_set_pixel_aspect(renderer_, 1.0);
  }

  //----------------------------------------------------------------
  // TLibass::addCustomFont
  //
  void
  TLibass::addCustomFont(const TFontAttachment & font)
  {
    fonts_.push(font);

    if (isReady())
    {
      addCustomFonts();
    }
  }

  //----------------------------------------------------------------
  // TLibass::track
  //
  TAssTrackPtr
  TLibass::track(const unsigned char * codecPrivate,
                 const std::size_t codecPrivateSize)
  {
    TAssTrackPtr track(new AssTrack(*this, codecPrivate, codecPrivateSize));
    return track;
  }

  //----------------------------------------------------------------
  // TLibass::init
  //
  void
  TLibass::init()
  {
    // lookup Fontconfig configuration file path:
    std::string fontsConf;
    bool removeAfterUse = false;
    getFontsConf(fontsConf, removeAfterUse);

    std::string defaultFamily =
      QApplication::font().family().toUtf8().constData();

#if 0 // def __APPLE__
    // seems to have trouble with Italics
    int fontProvider = ASS_FONTPROVIDER_CORETEXT;
#elif defined(_WIN32)
    int fontProvider = ASS_FONTPROVIDER_DIRECTWRITE;
#else
    int fontProvider = ASS_FONTPROVIDER_FONTCONFIG;
#endif
    int updateFontCache = 1;

    ass_set_fonts(renderer_,
                  NULL, // default font file
                  defaultFamily.c_str(),
                  fontProvider,
                  fontsConf.size() ? fontsConf.c_str() : NULL,
                  updateFontCache);

    if (removeAfterUse)
    {
      // remove the temporary fontconfig file:
      QFile::remove(QString::fromUtf8(fontsConf.c_str()));
    }

    addCustomFonts();
  }

  //----------------------------------------------------------------
  // TLibass::thread_loop
  //
  void
  TLibass::thread_loop()
  {
    // begin:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      initialized_ = false;
    }

    // this can take a while to rebuild the font cache:
    init();

    // done:
    {
      boost::unique_lock<boost::mutex> lock(mutex_);
      initialized_ = true;
    }

    if (callback_)
    {
      callback_(callbackContext_, this);
    }
  }

  //----------------------------------------------------------------
  // TLibass::addCustomFonts
  //
  void
  TLibass::addCustomFonts()
  {
    while (!fonts_.isEmpty())
    {
      TFontAttachment font;
      if (!fonts_.pop(font, NULL, false))
      {
        continue;
      }

      ass_add_font(library_,
                   (char *)font.filename_,
                   (char *)font.data_,
                   (int)font.size_);
    }
  }
}
