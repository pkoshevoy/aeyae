// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:43:07 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <deque>

// boost includes:
#include <boost/thread.hpp>

// Qt includes:
#define GL_GLEXT_PROTOTYPES
#include <QtOpenGL>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QTime>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#elif !defined(_WIN32)
#include <QtDBus/QtDBus>
#endif

// libass includes:
// #undef YAE_USE_LIBASS
#ifdef YAE_USE_LIBASS
extern "C"
{
#include <ass/ass.h>
}
#endif

// yae includes:
#include "yae/video/yae_video.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/thread/yae_threading.h"

// local includes:
#include <yaeCanvas.h>
#include <yaeUtilsQt.h>

namespace yae
{

#ifdef YAE_USE_LIBASS
  //----------------------------------------------------------------
  // getFontsConf
  //
  static bool
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

#if defined(__APPLE__)
    fontsConf = "/opt/local/etc/fonts/fonts.conf";

    if (QFileInfo(QString::fromUtf8(fontsConf.c_str())).exists())
    {
      // use the macports fontconfig file:
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
    os << "<?xml version=\"1.0\"?>" << std::endl
       << "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">" << std::endl
       << "<fontconfig>" << std::endl
       << "\t<dir>"
       << QDir::toNativeSeparators(fontsDir).toUtf8().constData()
       << "</dir>" << std::endl;

#ifdef __APPLE__
    os << "\t<dir>/Library/Fonts</dir>" << std::endl
       << "\t<dir>~/Library/Fonts</dir>" << std::endl;
#endif

#ifndef _WIN32
    const char * fontdir[] = {
      "/usr/share/fonts",
      "/usr/X11R6/lib/X11/fonts",
      "/opt/kde3/share/fonts",
      "/usr/local/share/fonts"
    };

    std::size_t nfontdir = sizeof(fontdir) / sizeof(fontdir[0]);
    for (std::size_t i = 0; i < nfontdir; i++)
    {
      QString path = QString::fromUtf8(fontdir[i]);
      if (QFileInfo(path).exists())
      {
        os << "\t<dir>" << fontdir[i] << "</dir>" << std::endl;
      }
    }
#endif

    os << "\t<cachedir>"
       << QDir::toNativeSeparators(fontconfigCache).toUtf8().constData()
       << "</cachedir>" << std::endl
       << "</fontconfig>" << std::endl;

    QString fn =
      tempDir +
      QString::fromUtf8("/apprenticevideo.fonts.conf.") +
      QString::number(appPid);

    fontsConf = QDir::toNativeSeparators(fn).toUtf8().constData();

#if !defined(NDEBUG)
    std::cerr << "fonts.conf: " << fontsConf << std::endl;
#endif

    std::FILE * fout = fopenUtf8(fontsConf.c_str(), "w");
    if (!fout)
    {
      return false;
    }

    std::string xml = os.str().c_str();

#if !defined(NDEBUG)
    std::cerr << "fonts.conf content:\n" << xml << std::endl;
#endif

    std::size_t nout = fwrite(xml.c_str(), 1, xml.size(), fout);
    fclose(fout);

    return nout == xml.size();
  }

  //----------------------------------------------------------------
  // TLibassInitDoneCallback
  //
  typedef void(*TLibassInitDoneCallback)(void *, TLibass *);
#endif

  //----------------------------------------------------------------
  // TLibassInit
  //
  class TLibass
  {
  public:

#ifdef YAE_USE_LIBASS
    //----------------------------------------------------------------
    // TLine
    //
    struct TLine
    {
      TLine(int64 pts = 0,
            const unsigned char * data = NULL,
            std::size_t size = 0):
        pts_(pts),
        data_((const char *)data, (const char *)data + size)
      {}

      bool operator == (const TLine & sub) const
      {
        return pts_ == sub.pts_ && data_ == sub.data_;
      }

      // presentation timestamp expressed in milliseconds:
      int64 pts_;

      // subtitle dialog line:
      std::string data_;
    };

    TLibass():
      callbackContext_(NULL),
      callback_(NULL),
      initialized_(false),
      assLibrary_(NULL),
      assRenderer_(NULL),
      assTrack_(NULL),
      bufferSize_(0)
    {}

    ~TLibass()
    {
      uninit();
    }

    void setCallback(void * context, TLibassInitDoneCallback callback)
    {
      YAE_ASSERT(!callback_ || !callback);
      callbackContext_ = context;
      callback_ = callback;
    }

    void setHeader(const unsigned char * codecPrivate = NULL,
                   std::size_t codecPrivateSize = 0)
    {
      header_.clear();
      if (codecPrivate && codecPrivateSize)
      {
        const char * header = (const char *)codecPrivate;
        header_.assign(header, header + codecPrivateSize);
      }
    }

    void setCustomFonts(const std::list<TFontAttachment> & customFonts)
    {
      customFonts_ = customFonts;
    }

    inline bool isReady() const
    {
      return initialized_;
    }

    void setFrameSize(int w, int h)
    {
      ass_set_frame_size(assRenderer_, w, h);

      double ar = double(w) / double(h);
      ass_set_aspect_ratio(assRenderer_, ar, ar);
    }

    void processData(const unsigned char * data, std::size_t size, int64 pts)
    {
      TLine line(pts, data, size);
      if (has(buffer_, line))
      {
        return;
      }

      // std::cerr << "ass_process_data: " << line.data_ << std::endl;

      if (bufferSize_)
      {
        const TLine & first = buffer_.front();
        if (pts < first.pts_)
        {
          // user skipped back in time, purge cached subs:
          ass_flush_events(assTrack_);
          buffer_.clear();
          bufferSize_ = 0;
        }
      }

      if (bufferSize_ < 10)
      {
        bufferSize_++;
      }
      else
      {
        buffer_.pop_front();
      }

      buffer_.push_back(line);
      ass_process_data(assTrack_, (char *)data, (int)size);
    }

    ASS_Image * renderFrame(int64 now, int * detectChange)
    {
      return ass_render_frame(assRenderer_,
                              assTrack_,
                              (long long)now,
                              detectChange);
    }

    void init()
    {
      uninit();

      assLibrary_ = ass_library_init();
      assRenderer_ = ass_renderer_init(assLibrary_);
      assTrack_ = ass_new_track(assLibrary_);

      for (std::list<TFontAttachment>::const_iterator
             i = customFonts_.begin(); i != customFonts_.end(); ++i)
      {
        const TFontAttachment & font = *i;
        ass_add_font(assLibrary_,
                     (char *)font.filename_,
                     (char *)font.data_,
                     (int)font.size_);
      }

      // lookup Fontconfig configuration file path:
      std::string fontsConf;
      bool removeAfterUse = false;
      getFontsConf(fontsConf, removeAfterUse);

      const char * defaultFont = NULL;
      const char * defaultFamily = NULL;
      int useFontconfig = 1;
      int updateFontCache = 1;

      ass_set_fonts(assRenderer_,
                    defaultFont,
                    defaultFamily,
                    useFontconfig,
                    fontsConf.size() ? fontsConf.c_str() : NULL,
                    updateFontCache);

      if (removeAfterUse)
      {
        // remove the temporary fontconfig file:
        QFile::remove(QString::fromUtf8(fontsConf.c_str()));
      }

      if (assTrack_ && header_.size())
      {
        ass_process_codec_private(assTrack_,
                                  &header_[0],
                                  (int)(header_.size()));
      }
    }

    void uninit()
    {
      if (assTrack_)
      {
        ass_free_track(assTrack_);
        assTrack_ = NULL;

        ass_renderer_done(assRenderer_);
        assRenderer_ = NULL;

        ass_library_done(assLibrary_);
        assLibrary_ = NULL;
      }
    }

    void threadLoop()
    {
      // begin:
      initialized_ = false;

      // this can take a while to rebuild the font cache:
      init();

      // done:
      initialized_ = true;

      if (callback_)
      {
        callback_(callbackContext_, this);
      }
    }

    void * callbackContext_;
    TLibassInitDoneCallback callback_;
    bool initialized_;

    ASS_Library * assLibrary_;
    ASS_Renderer * assRenderer_;
    ASS_Track * assTrack_;
    std::vector<char> header_;
    std::list<TFontAttachment> customFonts_;
    std::list<TLine> buffer_;
    std::size_t bufferSize_;
#endif
  };

  //----------------------------------------------------------------
  // libassInitThread
  //
#ifdef YAE_USE_LIBASS
  static Thread<TLibass> libassInitThread;
#endif

  //----------------------------------------------------------------
  // asyncInitLibass
  //
  TLibass *
  Canvas::asyncInitLibass(const unsigned char * header,
                          const std::size_t headerSize)
  {
    TLibass * libass = NULL;

#ifdef YAE_USE_LIBASS
    libass = new TLibass();
    libass->setCallback(this, &Canvas::libassInitDoneCallback);
    libass->setHeader(header, headerSize);
    libass->setCustomFonts(customFonts_);

    if (!libassInitThread.isRunning())
    {
      libassInitThread.setContext(libass);
      libassInitThread.run();
    }
    else
    {
      YAE_ASSERT(false);
      libassInitThread.stop();
    }
#endif

    return libass;
  }

  //----------------------------------------------------------------
  // stopAsyncInitLibassThread
  //
  static void
  stopAsyncInitLibassThread()
  {
#ifdef YAE_USE_LIBASS
    if (libassInitThread.isRunning())
    {
      libassInitThread.stop();
      libassInitThread.wait();
      libassInitThread.setContext(NULL);
    }
#endif
  }

  //----------------------------------------------------------------
  // TFontAttachment::TFontAttachment
  //
  TFontAttachment::TFontAttachment(const char * filename,
                                   const unsigned char * data,
                                   std::size_t size):
    filename_(filename),
    data_(data),
    size_(size)
  {}

  //----------------------------------------------------------------
  // Canvas::Canvas
  //
  Canvas::Canvas(QWidget * parent, Qt::WindowFlags f):
    QOpenGLWidget(parent, f),
    context_(*this),
    private_(NULL),
    overlay_(NULL),
    libass_(NULL),
    showTheGreeting_(true),
    subsInOverlay_(false),
    renderMode_(Canvas::kScaleToFit),
    timerHideCursor_(this),
    timerScreenSaver_(this)
  {
    setObjectName("yae::Canvas");
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAutoFillBackground(false);
    setMouseTracking(true);

    timerHideCursor_.setSingleShot(true);
    timerHideCursor_.setInterval(3000);

    timerScreenSaver_.setSingleShot(true);
    timerScreenSaver_.setInterval(29000);

    timerScreenSaverUnInhibit_.setSingleShot(true);
    timerScreenSaverUnInhibit_.setInterval(59000);

    bool ok = true;
    ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                 this, SLOT(hideCursor()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaver_, SIGNAL(timeout()),
                 this, SLOT(screenSaverInhibit()));
    YAE_ASSERT(ok);

    ok = connect(&timerScreenSaverUnInhibit_, SIGNAL(timeout()),
                 this, SLOT(screenSaverUnInhibit()));
    YAE_ASSERT(ok);

    greeting_ = tr("drop videos/music here\n\n"
                   "press spacebar to pause/resume\n\n"
                   "alt-left/alt-right to navigate playlist\n\n"
#ifdef __APPLE__
                   "use apple remote for volume and seeking\n\n"
#endif
                   "explore the menus for more options");
  }

  //----------------------------------------------------------------
  // Canvas::~Canvas
  //
  Canvas::~Canvas()
  {
    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    delete private_;
    delete overlay_;
  }

  //----------------------------------------------------------------
  // Canvas::initializePrivateBackend
  //
  void
  Canvas::initializePrivateBackend()
  {
    TMakeCurrentContext currentContext(context_);

    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    delete private_;
    private_ = NULL;

    delete overlay_;
    overlay_ = NULL;

    private_ = new CanvasRenderer();
    overlay_ = new CanvasRenderer();

    libass_ = asyncInitLibass();
  }

  //----------------------------------------------------------------
  // Canvas::fragmentShaderFor
  //
  const TFragmentShader *
  Canvas::fragmentShaderFor(const VideoTraits & vtts) const
  {
    return private_ ? private_->fragmentShaderFor(vtts) : NULL;
  }

  //----------------------------------------------------------------
  // Canvas::acceptFramesWithReaderId
  //
  void
  Canvas::acceptFramesWithReaderId(unsigned int readerId)
  {
    payload_.setExpectedReaderId(readerId);
  }

  //----------------------------------------------------------------
  // Canvas::libassAddFont
  //
  void
  Canvas::libassAddFont(const char * filename,
                        const unsigned char * data,
                        const std::size_t size)
  {
    customFonts_.push_back(TFontAttachment(filename, data, size));
  }

  //----------------------------------------------------------------
  // Canvas::clear
  //
  void
  Canvas::clear()
  {
    private_->clear(context_);
    clearOverlay();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::clearOverlay
  //
  void
  Canvas::clearOverlay()
  {
    overlay_->clear(context_);

    stopAsyncInitLibassThread();
    delete libass_;
    libass_ = NULL;

    showTheGreeting_ = false;
    subsInOverlay_ = false;
    subs_.clear();
    customFonts_.clear();
  }

  //----------------------------------------------------------------
  // Canvas::refresh
  //
  void
  Canvas::refresh()
  {
    if (!isVisible())
    {
      return;
    }

    QOpenGLWidget::update();
    QOpenGLWidget::doneCurrent();
  }

  //----------------------------------------------------------------
  // Canvas::render
  //
  bool
  Canvas::render(const TVideoFramePtr & frame)
  {
    bool postThePayload = payload_.set(frame);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new RenderFrameEvent(payload_));
    }

    if (autoCropThread_.isRunning())
    {
      autoCrop_.setFrame(frame);
    }

    return true;
  }

  //----------------------------------------------------------------
  // UpdateOverlayEvent
  //
  struct UpdateOverlayEvent : public QEvent
  {
    UpdateOverlayEvent(): QEvent(QEvent::User) {}
  };

  //----------------------------------------------------------------
  // LibassInitDoneEvent
  //
  struct LibassInitDoneEvent : public QEvent
  {
    LibassInitDoneEvent(TLibass * libass):
      QEvent(QEvent::User),
      libass_(libass)
    {}

    TLibass * libass_;
  };

  //----------------------------------------------------------------
  // Canvas::event
  //
  bool
  Canvas::event(QEvent * event)
  {
    if (event->type() == QEvent::User)
    {
      RenderFrameEvent * renderEvent = dynamic_cast<RenderFrameEvent *>(event);
      if (renderEvent)
      {
        event->accept();

        TVideoFramePtr frame;
        renderEvent->payload_.get(frame);
        loadFrame(frame);

        return true;
      }

      UpdateOverlayEvent * overlayEvent =
        dynamic_cast<UpdateOverlayEvent *>(event);
      if (overlayEvent)
      {
        event->accept();

        updateOverlay(true);
        refresh();
        return true;
      }

      LibassInitDoneEvent * libassInitDoneEvent =
        dynamic_cast<LibassInitDoneEvent *>(event);
      if (libassInitDoneEvent)
      {
        event->accept();

#ifdef YAE_USE_LIBASS
        stopAsyncInitLibassThread();
        updateOverlay(true);
        refresh();
#endif
        return true;
      }
    }

    return QOpenGLWidget::event(event);
  }

  //----------------------------------------------------------------
  // Canvas::mouseMoveEvent
  //
  void
  Canvas::mouseMoveEvent(QMouseEvent * event)
  {
    setCursor(QCursor(Qt::ArrowCursor));
    timerHideCursor_.start();
  }

  //----------------------------------------------------------------
  // Canvas::mouseDoubleClickEvent
  //
  void
  Canvas::mouseDoubleClickEvent(QMouseEvent * event)
  {
    emit toggleFullScreen();
  }

  //----------------------------------------------------------------
  // Canvas::resizeEvent
  //
  void
  Canvas::resizeEvent(QResizeEvent * event)
  {
    QOpenGLWidget::resizeEvent(event);

    if (overlay_ && (subsInOverlay_ || showTheGreeting_))
    {
      updateOverlay(true);
    }
  }

  //----------------------------------------------------------------
  // Canvas::initializeGL
  //
  void
  Canvas::initializeGL()
  {
    QOpenGLWidget::initializeGL();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glUseProgram(0);
    yae_assert_gl_no_error();

    // glShadeModel(GL_SMOOTH);
    glShadeModel(GL_FLAT);
    glClearDepth(0);
    glClearStencil(0);
    glClearAccum(0, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
  }

  //----------------------------------------------------------------
  // calcImageWidth
  //
  static double
  calcImageWidth(const CanvasRenderer * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(w, h, cameraRotation);
    return w;
  }

  //----------------------------------------------------------------
  // calcImageHeight
  //
  static double
  calcImageHeight(const CanvasRenderer * canvas)
  {
    double w = 0.0;
    double h = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(w, h, cameraRotation);
    return h;
  }

  //----------------------------------------------------------------
  // paintImage
  //
  static void
  paintImage(CanvasRenderer * canvas,
             int canvasWidth,
             int canvasHeight,
             Canvas::TRenderMode renderMode)
  {
    double croppedWidth = 0.0;
    double croppedHeight = 0.0;
    int cameraRotation = 0;
    canvas->imageWidthHeightRotated(croppedWidth,
                                    croppedHeight,
                                    cameraRotation);

    double dar = croppedWidth / croppedHeight;
    double car = double(canvasWidth) / double(canvasHeight);

    double x = 0.0;
    double y = 0.0;
    double w = double(canvasWidth);
    double h = double(canvasHeight);

    if (renderMode == Canvas::kScaleToFit)
    {
      if (dar < car)
      {
        w = double(canvasHeight) * dar;
        x = 0.5 * (double(canvasWidth) - w);
      }
      else
      {
        h = double(canvasWidth) / dar;
        y = 0.5 * (double(canvasHeight) - h);
      }
    }
    else
    {
      if (dar < car)
      {
        h = double(canvasWidth) / dar;
        y = 0.5 * (double(canvasHeight) - h);
      }
      else
      {
        w = double(canvasHeight) * dar;
        x = 0.5 * (double(canvasWidth) - w);
      }
    }

    glViewport(GLint(x + 0.5), GLint(y + 0.5),
               GLsizei(w + 0.5), GLsizei(h + 0.5));

    double left = 0.0;
    double right = croppedWidth;
    double top = 0.0;
    double bottom = croppedHeight;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1.0, 1.0);

    if (cameraRotation && cameraRotation % 90 == 0)
    {
      glTranslated(0.5 * croppedWidth, 0.5 * croppedHeight, 0);
      glRotated(double(cameraRotation), 0, 0, 1);

      if (cameraRotation % 180 != 0)
      {
        glTranslated(-0.5 * croppedHeight, -0.5 * croppedWidth, 0);
      }
      else
      {
        glTranslated(-0.5 * croppedWidth, -0.5 * croppedHeight, 0);
      }
    }

    canvas->draw();
    yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // Canvas::paintGL
  //
  void
  Canvas::paintGL()
  {
    if (width() == 0 || height() == 0)
    {
      return;
    }

    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    if (!ptts)
    {
      // unsupported pixel format:
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      if (!showTheGreeting_)
      {
        return;
      }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int canvasWidth = width();
    int canvasHeight = height();

    // draw a checkerboard to help visualize the alpha channel:
    if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                 pixelFormat::kPaletted)))
    {
      glViewport(0, 0, canvasWidth, canvasHeight);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, canvasWidth, canvasHeight, 0, -1.0, 1.0);

      float zebra[2][3] = {
        { 1.0f, 1.0f, 1.0f },
        { 0.7f, 0.7f, 0.7f }
      };

      int edgeSize = 24;
      bool evenRow = false;
      for (int y = 0; y < canvasHeight; y += edgeSize, evenRow = !evenRow)
      {
        int y1 = std::min(y + edgeSize, canvasHeight);

        bool evenCol = false;
        for (int x = 0; x < canvasWidth; x += edgeSize, evenCol = !evenCol)
        {
          int x1 = std::min(x + edgeSize, canvasWidth);

          float * color = (evenRow ^ evenCol) ? zebra[0] : zebra[1];
          glColor3fv(color);

          glRecti(x, y, x1, y1);
        }
      }

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
    }

    if (ptts)
    {
      // draw the frame:
      paintImage(private_, canvasWidth, canvasHeight, renderMode_);
    }

    // draw the overlay:
    if (subsInOverlay_ || showTheGreeting_)
    {
      if (overlay_ && overlay_->pixelTraits())
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        paintImage(overlay_, canvasWidth, canvasHeight, kScaleToFit);
        glDisable(GL_BLEND);
      }
      else
      {
        qApp->postEvent(this, new UpdateOverlayEvent());
      }
    }
  }

  //----------------------------------------------------------------
  // Canvas::loadFrame
  //
  bool
  Canvas::loadFrame(const TVideoFramePtr & frame)
  {
    if (!frame)
    {
      return false;
    }

    bool ok = private_->loadFrame(context_, frame);
    showTheGreeting_ = false;
    setSubs(frame->subs_);

    refresh();

    if (ok && !timerScreenSaver_.isActive())
    {
      timerScreenSaver_.start();
    }

    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::currentFrame
  //
  TVideoFramePtr
  Canvas::currentFrame() const
  {
    TVideoFramePtr frame;

    if (private_)
    {
      private_->getFrame(frame);
    }

    return frame;
  }

  //----------------------------------------------------------------
  // Canvas::setSubs
  //
  void
  Canvas::setSubs(const std::list<TSubsFrame> & subs)
  {
    std::list<TSubsFrame> renderSubs;

    for (std::list<TSubsFrame>::const_iterator i = subs.begin();
         i != subs.end(); ++i)
    {
      const TSubsFrame & subs = *i;
      if (subs.render_)
      {
        renderSubs.push_back(subs);
      }
    }

    bool reparse = (renderSubs != subs_);
    if (reparse)
    {
      subs_ = renderSubs;
    }

    updateOverlay(reparse);
  }

  //----------------------------------------------------------------
  // TQImageBuffer
  //
  struct TQImageBuffer : public IPlanarBuffer
  {
    QImage qimg_;

    TQImageBuffer(int w, int h, QImage::Format fmt):
      qimg_(w, h, fmt)
    {
      unsigned char * dst = qimg_.bits();
      int rowBytes = qimg_.bytesPerLine();
      memset(dst, 0, rowBytes * h);
    }

    // virtual:
    void destroy()
    { delete this; }

    // virtual:
    std::size_t planes() const
    { return 1; }

    // virtual:
    unsigned char * data(std::size_t plane) const
    {
      const uchar * bits = qimg_.bits();
      return const_cast<unsigned char *>(bits);
    }

    // virtual:
    std::size_t rowBytes(std::size_t planeIndex) const
    {
      int n = qimg_.bytesPerLine();
      return (std::size_t)n;
    }
  };

  //----------------------------------------------------------------
  // drawPlainText
  //
  static bool
  drawPlainText(const std::string & text,
                QPainter & painter,
                QRect & bbox,
                int textAlignment)
  {
    QString qstr = QString::fromUtf8(text.c_str()).trimmed();
    if (!qstr.isEmpty())
    {
      QRect used;
      drawTextWithShadowToFit(painter,
                              bbox,
                              textAlignment,
                              qstr,
                              QPen(Qt::black),
                              true, // outline shadow
                              1, // shadow offset
                              &used);
      if (!used.isNull())
      {
        // avoid overwriting subs on top of each other:
        bbox.setBottom(used.top());
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // TPainterWrapper
  //
  struct TPainterWrapper
  {
    TPainterWrapper(int w, int h):
      painter_(NULL),
      w_(w),
      h_(h)
    {}

    ~TPainterWrapper()
    {
      delete painter_;
    }

    inline TVideoFramePtr & getFrame()
    {
      if (!frame_)
      {
        frame_.reset(new TVideoFrame());

        TQImageBuffer * imageBuffer =
          new TQImageBuffer(w_, h_, QImage::Format_ARGB32);
        // imageBuffer->qimg_.fill(0);

        frame_->data_.reset(imageBuffer);
      }

      return frame_;
    }

    inline QImage & getImage()
    {
      TVideoFramePtr & vf = getFrame();
      TQImageBuffer * imageBuffer = (TQImageBuffer *)(vf->data_.get());
      return imageBuffer->qimg_;
    }

    inline QPainter & getPainter()
    {
      if (!painter_)
      {
        QImage & image = getImage();
        painter_ = new QPainter(&image);

        painter_->setPen(Qt::white);
        painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);

        QFont ft;
        int px = std::max<int>(20, 56.0 * (h_ / 1024.0));
        ft.setPixelSize(px);
        painter_->setFont(ft);
      }

      return *painter_;
    }

    inline void painterEnd()
    {
      if (painter_)
      {
        painter_->end();
      }
    }

  private:
    TVideoFramePtr frame_;
    QPainter * painter_;
    int w_;
    int h_;
  };

  //----------------------------------------------------------------
  // calcFrameTransform
  //
  static void
  calcFrameTransform(double bbox_w,
                     double bbox_h,
                     double frame_w,
                     double frame_h,
                     double & offset_x,
                     double & offset_y,
                     double & scaled_w,
                     double & scaled_h,
                     bool fit_to_height = false)
  {
    double bbox_ar = double(bbox_w) / double(bbox_h);
    double frame_ar = double(frame_w) / double(frame_h);

    offset_x = 0;
    offset_y = 0;
    scaled_w = bbox_w;
    scaled_h = bbox_h;

    if (!fit_to_height)
    {
      fit_to_height = frame_ar < bbox_ar;
    }

    if (fit_to_height)
    {
      scaled_w = bbox_h * frame_ar;
      offset_x = 0.5 * (bbox_w - scaled_w);
    }
    else
    {
      scaled_h = bbox_w / frame_ar;
      offset_y = 0.5 * (bbox_h - scaled_h);
    }
  }

  //----------------------------------------------------------------
  // TScaledFrame
  //
  struct TScaledFrame
  {
    double x_;
    double y_;
    double w_;
    double h_;
  };

  //----------------------------------------------------------------
  // calcFrameTransform
  //
  inline static void
  calcFrameTransform(double bbox_w,
                     double bbox_h,
                     double frame_w,
                     double frame_h,
                     TScaledFrame & f,
                     bool fit_to_height = false)
  {
    calcFrameTransform(bbox_w,
                       bbox_h,
                       frame_w,
                       frame_h,
                       f.x_,
                       f.y_,
                       f.w_,
                       f.h_,
                       fit_to_height);
  }

  //----------------------------------------------------------------
  // Canvas::loadSubs
  //
  bool
  Canvas::updateOverlay(bool reparse)
  {
    if (showTheGreeting_)
    {
      return updateGreeting();
    }

    double imageWidth = 0.0;
    double imageHeight = 0.0;
    int cameraRotation = 0;
    private_->imageWidthHeightRotated(imageWidth,
                                      imageHeight,
                                      cameraRotation);

    double w = this->width();
    double h = this->height();

    double max_w = 1920.0;
    double max_h = 1080.0;

    if (h > max_h)
    {
      w *= max_h / h;
      h = max_h;
    }

    if (w > max_w)
    {
      h *= max_w / w;
      w = max_w;
    }

    double fw = w;
    double fh = h;
    double fx = 0;
    double fy = 0;

    calcFrameTransform(w, h, imageWidth, imageHeight, fx, fy, fw, fh);

    int ix = int(fx);
    int iy = int(fy);
    int iw = int(fw);
    int ih = int(fh);

    TPainterWrapper wrapper((int)w, (int)h);

    int textAlignment = Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignBottom;
    bool paintedSomeSubs = false;
    bool libassSameSubs = false;

    QRect canvasBBox(16, 16, (int)w - 32, (int)h - 32);
    TVideoFramePtr frame = currentFrame();

    for (std::list<TSubsFrame>::const_iterator i = subs_.begin();
         i != subs_.end() && reparse; ++i)
    {
      const TSubsFrame & subs = *i;
      const TSubsFrame::IPrivate * subExt = subs.private_.get();
      const unsigned int nrects = subExt ? subExt->numRects() : 0;
      unsigned int nrectsPainted = 0;

      for (unsigned int j = 0; j < nrects; j++)
      {
        TSubsFrame::TRect r;
        subExt->getRect(j, r);

        if (r.type_ == kSubtitleBitmap)
        {
          const unsigned char * pal = r.data_[1];

          QImage img(r.w_, r.h_, QImage::Format_ARGB32);
          unsigned char * dst = img.bits();
          int dstRowBytes = img.bytesPerLine();

          for (int y = 0; y < r.h_; y++)
          {
            const unsigned char * srcLine = r.data_[0] + y * r.rowBytes_[0];
            unsigned char * dstLine = dst + y * dstRowBytes;

            for (int x = 0; x < r.w_; x++, dstLine += 4, srcLine++)
            {
              int colorIndex = *srcLine;
              memcpy(dstLine, pal + colorIndex * 4, 4);
            }
          }

          // always fit to box height regardless of frame aspect ratio;
          // this may crop off part of the frame on the left and right,
          // but in practice it makes subtitles more visible when watching
          // a 4x3 video cropped from 16x9 blu-ray (FLCL, Star Trek TNG, etc...)
          bool fit_to_height = true;

          double rw = double(subs.rw_ ? subs.rw_ : imageWidth);
          double rh = double(subs.rh_ ? subs.rh_ : imageHeight);

          TScaledFrame sf;
          calcFrameTransform(w, h, rw, rh, sf, fit_to_height);

          double sx = sf.w_ / rw;
          double sy = sf.h_ / rh;

          QPoint dstPos((int)(sx * double(r.x_) + sf.x_),
                        (int)(sy * double(r.y_) + sf.y_));
          QSize dstSize((int)(sx * double(r.w_)),
                        (int)(sy * double(r.h_)));

          wrapper.getPainter().drawImage(QRect(dstPos, dstSize),
                                         img, img.rect());
          paintedSomeSubs = true;
          nrectsPainted++;
        }
        else if (r.type_ == kSubtitleASS)
        {
          std::string assa(r.assa_);
          bool done = false;

#ifdef YAE_USE_LIBASS
          if (!libass_)
          {
            if (subs.traits_ == kSubsSSA && subs.extraData_)
            {
              libass_ = asyncInitLibass(subs.extraData_->data(0),
                                        subs.extraData_->rowBytes(0));
            }
            else if (subExt->headerSize())
            {
              libass_ = asyncInitLibass(subExt->header(),
                                        subExt->headerSize());
            }
          }

          if (libass_ && libass_->isReady())
          {
            int64 pts = (int64)(subs.time_.toSeconds() * 1000.0 + 0.5);
            libass_->processData((unsigned char *)&assa[0], assa.size(), pts);
            nrectsPainted++;
            done = true;
          }
#endif
          if (!done)
          {
            std::string text = assaToPlainText(assa);
            text = convertEscapeCodes(text);

            if (drawPlainText(text,
                              wrapper.getPainter(),
                              canvasBBox,
                              textAlignment))
            {
              paintedSomeSubs = true;
              nrectsPainted++;
            }
          }
        }
      }

#ifdef YAE_USE_LIBASS
      if (!nrectsPainted && subs.data_ &&
          (subs.traits_ == kSubsSSA))
      {
        if (!libass_)
        {
          if (subs.traits_ == kSubsSSA && subs.extraData_)
          {
            libass_ = asyncInitLibass(subs.extraData_->data(0),
                                      subs.extraData_->rowBytes(0));
          }
          else if (subExt && subExt->headerSize())
          {
            libass_ = asyncInitLibass(subExt->header(),
                                      subExt->headerSize());
          }
        }

        if (libass_ && libass_->isReady())
        {
          const unsigned char * ssa = subs.data_->data(0);
          const std::size_t ssaSize = subs.data_->rowBytes(0);

          int64 pts = (int64)(subs.time_.toSeconds() * 1000.0 + 0.5);
          libass_->processData(ssa, ssaSize, pts);
          nrectsPainted++;
        }
      }
#endif

      if (!nrectsPainted && subs.data_ &&
          (subs.traits_ == kSubsSSA ||
           subs.traits_ == kSubsText ||
           subs.traits_ == kSubsSUBRIP))
      {
        const unsigned char * str = subs.data_->data(0);
        const unsigned char * end = str + subs.data_->rowBytes(0);

        std::string text(str, end);
        if (subs.traits_ == kSubsSSA)
        {
          text = assaToPlainText(text);
        }
        else
        {
          text = stripHtmlTags(text);
        }

        text = convertEscapeCodes(text);
        if (drawPlainText(text,
                          wrapper.getPainter(),
                          canvasBBox,
                          textAlignment))
        {
          paintedSomeSubs = true;
        }
      }
    }

#ifdef YAE_USE_LIBASS
    if (libass_ && libass_->isReady() && frame)
    {
      libass_->setFrameSize(iw, ih);

      // the list of images is owned by libass,
      // libass is responsible for their deallocation:
      int64 now = (int64)(frame->time_.toSeconds() * 1000.0 + 0.5);

      int changeDetected = 0;
      ASS_Image * pic = libass_->renderFrame(now, &changeDetected);
      libassSameSubs = !changeDetected;
      paintedSomeSubs = changeDetected;

      unsigned char bgra[4];
      while (pic && changeDetected)
      {
#ifdef __BIG_ENDIAN__
        bgra[3] = 0xFF & (pic->color >> 8);
        bgra[2] = 0xFF & (pic->color >> 16);
        bgra[1] = 0xFF & (pic->color >> 24);
        bgra[0] = 0xFF & (pic->color);
#else
        bgra[0] = 0xFF & (pic->color >> 8);
        bgra[1] = 0xFF & (pic->color >> 16);
        bgra[2] = 0xFF & (pic->color >> 24);
        bgra[3] = 0xFF & (pic->color);
#endif
        QImage tmp(pic->w, pic->h, QImage::Format_ARGB32);
        int dstRowBytes = tmp.bytesPerLine();
        unsigned char * dst = tmp.bits();

        for (int y = 0; y < pic->h; y++)
        {
          const unsigned char * srcLine = pic->bitmap + pic->stride * y;
          unsigned char * dstLine = dst + dstRowBytes * y;

          for (int x = 0; x < pic->w; x++, dstLine += 4, srcLine++)
          {
            unsigned char alpha = *srcLine;
#ifdef __BIG_ENDIAN__
            dstLine[0] = alpha;
            memcpy(dstLine + 1, bgra + 1, 3);
#else
            memcpy(dstLine, bgra, 3);
            dstLine[3] = alpha;
#endif
          }
        }

        wrapper.getPainter().drawImage(QRect(pic->dst_x + ix,
                                             pic->dst_y + iy,
                                             pic->w,
                                             pic->h),
                                       tmp, tmp.rect());

        pic = pic->next;
      }
    }
#endif

    wrapper.painterEnd();

    if (reparse && !libassSameSubs)
    {
      subsInOverlay_ = paintedSomeSubs;
    }

    if (!paintedSomeSubs)
    {
      return true;
    }

    TVideoFramePtr & vf = wrapper.getFrame();
    VideoTraits & vtts = vf->traits_;
    QImage & image = wrapper.getImage();

#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    subsInOverlay_ = overlay_->loadFrame(context_, vf);
    YAE_ASSERT(subsInOverlay_);
    return subsInOverlay_;
  }

  //----------------------------------------------------------------
  // Canvas::setGreeting
  //
  void
  Canvas::setGreeting(const QString & greeting)
  {
    showTheGreeting_ = true;
    greeting_ = greeting;
    updateGreeting();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::updateGreeting
  //
  bool
  Canvas::updateGreeting()
  {
    if (!overlay_)
    {
      return false;
    }

    double w = this->width();
    double h = this->height();

    double max_w = 1920.0;
    double max_h = 1080.0;

    if (h > max_h)
    {
      w *= max_h / h;
      h = max_h;
    }

    if (w > max_w)
    {
      h *= max_w / w;
      w = max_w;
    }

    TVideoFramePtr vf(new TVideoFrame());
    TQImageBuffer * imageBuffer =
      new TQImageBuffer((int)w, (int)h, QImage::Format_ARGB32);
    vf->data_.reset(imageBuffer);

    // shortcut:
    QImage & subsFrm = imageBuffer->qimg_;
    subsFrm.fill(0);

    QPainter painter(&subsFrm);
    painter.setPen(QColor(0x7f, 0x7f, 0x7f, 0x7f));

    QFont ft;
    int px = std::max<int>(12, 56.0 * std::min<double>(w / max_w, h / max_h));
    ft.setPixelSize(px);
    painter.setFont(ft);

    int textAlignment = Qt::TextWordWrap | Qt::AlignCenter;
    QRect canvasBBox = subsFrm.rect();

    std::string text(greeting_.toUtf8().constData());
    if (!drawPlainText(text, painter, canvasBBox, textAlignment))
    {
      return false;
    }

    painter.end();

    VideoTraits & vtts = vf->traits_;
#ifdef _BIG_ENDIAN
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.encodedWidth_ = subsFrm.bytesPerLine() / 4;
    vtts.encodedHeight_ = subsFrm.byteCount() / subsFrm.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = (int)w;
    vtts.visibleHeight_ = (int)h;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    bool ok = overlay_->loadFrame(context_, vf);
    YAE_ASSERT(ok);
    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::skipColorConverter
  //
  void
  Canvas::skipColorConverter(bool enable)
  {
    private_->skipColorConverter(context_, enable);
  }

  //----------------------------------------------------------------
  // Canvas::enableVerticalScaling
  //
  void
  Canvas::enableVerticalScaling(bool enable)
  {
    private_->enableVerticalScaling(enable);
  }

  //----------------------------------------------------------------
  // Canvas::overrideDisplayAspectRatio
  //
  void
  Canvas::overrideDisplayAspectRatio(double dar)
  {
    private_->overrideDisplayAspectRatio(dar);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(double darCropped)
  {
    private_->cropFrame(darCropped);
  }

  //----------------------------------------------------------------
  // Canvas::cropFrame
  //
  void
  Canvas::cropFrame(const TCropFrame & crop)
  {
    cropAutoDetectStop();

#if 0
    std::cerr << "\nCROP FRAME AUTO DETECTED: "
              << "x = " << crop.x_ << ", "
              << "y = " << crop.y_ << ", "
              << "w = " << crop.w_ << ", "
              << "h = " << crop.h_
              << std::endl;
#endif

    private_->cropFrame(crop);
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetect
  //
  void
  Canvas::cropAutoDetect(void * callbackContext, TAutoCropCallback callback)
  {
    if (!autoCropThread_.isRunning())
    {
      autoCrop_.reset(callbackContext, callback);
      autoCropThread_.setContext(&autoCrop_);
      autoCropThread_.run();
    }
  }

  //----------------------------------------------------------------
  // Canvas::cropAutoDetectStop
  //
  void
  Canvas::cropAutoDetectStop()
  {
    autoCrop_.stop();
    autoCropThread_.stop();
    autoCropThread_.wait();
    autoCropThread_.setContext(NULL);
  }

  //----------------------------------------------------------------
  // Canvas::imageWidth
  //
  double
  Canvas::imageWidth() const
  {
    return private_ ? calcImageWidth(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageHeight
  //
  double
  Canvas::imageHeight() const
  {
    return private_ ? calcImageHeight(private_) : 0.0;
  }

  //----------------------------------------------------------------
  // Canvas::imageAspectRatio
  //
  double
  Canvas::imageAspectRatio(double & w, double & h) const
  {
    double dar = 0.0;
    w = 0.0;
    h = 0.0;

    if (private_)
    {
      dar = private_->imageWidthHeight(w, h) ? w / h : 0.0;
    }

    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::setRenderMode
  //
  void
  Canvas::setRenderMode(Canvas::TRenderMode renderMode)
  {
    if (renderMode_ != renderMode)
    {
      renderMode_ = renderMode;
      refresh();
    }
  }

  //----------------------------------------------------------------
  // Canvas::libassInitDoneCallback
  //
  void
  Canvas::libassInitDoneCallback(void * context, TLibass * libass)
  {
    Canvas * canvas = (Canvas *)context;
    qApp->postEvent(canvas, new LibassInitDoneEvent(libass));
  }

  //----------------------------------------------------------------
  // Canvas::hideCursor
  //
  void
  Canvas::hideCursor()
  {
    setCursor(QCursor(Qt::BlankCursor));
  }

  //----------------------------------------------------------------
  // screenSaverUnInhibitCookie
  //
  static unsigned int screenSaverUnInhibitCookie = 0;

  //----------------------------------------------------------------
  // Canvas::screenSaverInhibit
  //
  void
  Canvas::screenSaverInhibit()
  {
#ifdef __APPLE__
    UpdateSystemActivity(UsrActivity);
#elif defined(_WIN32)
    // http://www.codeproject.com/KB/system/disablescreensave.aspx
    //
    // Call the SystemParametersInfo function to query and reset the
    // screensaver time-out value.  Use the user's default settings
    // in case your application terminates abnormally.
    //

    static UINT spiGetter[] = { SPI_GETLOWPOWERTIMEOUT,
                                SPI_GETPOWEROFFTIMEOUT,
                                SPI_GETSCREENSAVETIMEOUT };

    static UINT spiSetter[] = { SPI_SETLOWPOWERTIMEOUT,
                                SPI_SETPOWEROFFTIMEOUT,
                                SPI_SETSCREENSAVETIMEOUT };

    std::size_t numParams = sizeof(spiGetter) / sizeof(spiGetter[0]);
    for (std::size_t i = 0; i < numParams; i++)
    {
      UINT val = 0;
      BOOL ok = SystemParametersInfo(spiGetter[i], 0, &val, 0);
      YAE_ASSERT(ok);
      if (ok)
      {
        ok = SystemParametersInfo(spiSetter[i], val, NULL, 0);
        YAE_ASSERT(ok);
      }
    }

#else
    // try using DBUS to talk to the screensaver...
    bool done = false;

    if (QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        // apparently SimulateUserActivity is not enough to keep Ubuntu
        // from starting the screensaver
        screensaver.call(QDBus::NoBlock, "SimulateUserActivity");

        // try to inhibit the screensaver as well:
        if (!screenSaverUnInhibitCookie)
        {
          QDBusMessage out =
            screensaver.call(QDBus::Block,
                             "Inhibit",
                             QVariant(QApplication::applicationName()),
                             QVariant("video playback"));

          if (out.type() == QDBusMessage::ReplyMessage &&
              !out.arguments().empty())
          {
            screenSaverUnInhibitCookie = out.arguments().front().toUInt();
          }
        }

        if (screenSaverUnInhibitCookie)
        {
          timerScreenSaverUnInhibit_.start();
        }

        done = true;
      }
    }

    if (!done)
    {
      // FIXME: not sure how to do this yet
      std::cerr << "screenSaverInhibit" << std::endl;
    }
#endif
  }

  //----------------------------------------------------------------
  // Canvas::screenSaverUnInhibit
  //
  void
  Canvas::screenSaverUnInhibit()
  {
#if !defined(__APPLE__) && !defined(_WIN32)
    if (screenSaverUnInhibitCookie &&
        QDBusConnection::sessionBus().isConnected())
    {
      QDBusInterface screensaver("org.freedesktop.ScreenSaver",
                                 "/ScreenSaver");
      if (screensaver.isValid())
      {
        screensaver.call(QDBus::NoBlock,
                         "UnInhibit",
                         QVariant(screenSaverUnInhibitCookie));
        screenSaverUnInhibitCookie = 0;
      }
    }
#endif
  }
}
