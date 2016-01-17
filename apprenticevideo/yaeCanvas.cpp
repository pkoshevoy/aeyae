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
#ifdef YAE_USE_QOPENGL_WIDGET
#define GL_GLEXT_PROTOTYPES
#include <QtOpenGL>
#else
#include <GL/glew.h>
#endif
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
#include <yaeCanvasQPainterUtils.h>
#include <yaeUtilsQt.h>


namespace yae
{

#ifndef YAE_USE_QOPENGL_WIDGET
  //----------------------------------------------------------------
  // initializeGlew
  //
  static bool
  initializeGlew()
  {
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
      std::cerr
        << "GLEW init failed: " << glewGetErrorString(err)
        << std::endl;
      YAE_ASSERT(false);
    }

    return true;
  }
#endif

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
  Canvas::Canvas(const boost::shared_ptr<IOpenGLContext> & ctx):
    eventReceiver_(*this),
    context_(ctx),
    private_(NULL),
    overlay_(NULL),
    libass_(NULL),
    showTheGreeting_(true),
    subsInOverlay_(false),
    renderMode_(Canvas::kScaleToFit),
    w_(0),
    h_(0)
  {}

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
  // Canvas::setDelegate
  //
  void
  Canvas::setDelegate(const boost::shared_ptr<IDelegate> & delegate)
  {
    delegate_ = delegate;
    for (std::list<ILayer *>::reverse_iterator i = layers_.rbegin();
         i != layers_.rend(); ++i)
    {
      ILayer * layer = *i;
      layer->setDelegate(delegate_);
    }
  }

  //----------------------------------------------------------------
  // Canvas::initializePrivateBackend
  //
  void
  Canvas::initializePrivateBackend()
  {
    TMakeCurrentContext currentContext(context());

#ifndef YAE_USE_QOPENGL_WIDGET
    // initialize OpenGL GLEW wrapper:
    static bool initialized = initializeGlew();
#endif

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
  // Canvas::append
  //
  void
  Canvas::append(Canvas::ILayer * layer)
  {
    if (!layer || has(layers_, layer))
    {
      YAE_ASSERT(false);
      return;
    }

    layer->setContext(context_);
    layer->setDelegate(delegate_);
    layers_.push_back(layer);
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
    renderFrameEvent_.setExpectedReaderId(readerId);
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
    private_->clear(context());
    clearOverlay();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::clearOverlay
  //
  void
  Canvas::clearOverlay()
  {
    overlay_->clear(context());

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
    if (!delegate_ || !delegate_->isVisible())
    {
      return;
    }

    delegate_->repaint();
  }

  //----------------------------------------------------------------
  // Canvas::requestRepaint
  //
  void
  Canvas::requestRepaint()
  {
    bool postThePayload = paintCanvasEvent_.setDelivered(false);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(&eventReceiver_,
                      new PaintCanvasEvent(paintCanvasEvent_),
                      Qt::HighEventPriority);
    }
  }

  //----------------------------------------------------------------
  // Canvas::render
  //
  bool
  Canvas::render(const TVideoFramePtr & frame)
  {
    bool postThePayload = renderFrameEvent_.set(frame);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(&eventReceiver_,
                      new RenderFrameEvent(renderFrameEvent_),
                      Qt::HighEventPriority);
    }

    if (autoCropThread_.isRunning())
    {
      autoCrop_.setFrame(frame);
    }

    return true;
  }

  //----------------------------------------------------------------
  // InitializeBackendEvent
  //
  struct InitializeBackendEvent : public QEvent
  {
    InitializeBackendEvent():
      QEvent(QEvent::User)
    {
      YAE_LIFETIME_START(lifetime, " 0 -- InitializeBackendEvent");
    }

    YAE_LIFETIME(lifetime);
  };

  //----------------------------------------------------------------
  // UpdateOverlayEvent
  //
  struct UpdateOverlayEvent : public QEvent
  {
    UpdateOverlayEvent():
      QEvent(QEvent::User)
    {
      YAE_LIFETIME_START(lifetime, " 3 -- UpdateOverlayEvent");
    }

    YAE_LIFETIME(lifetime);
  };

  //----------------------------------------------------------------
  // LibassInitDoneEvent
  //
  struct LibassInitDoneEvent : public QEvent
  {
    LibassInitDoneEvent(TLibass * libass):
      QEvent(QEvent::User),
      libass_(libass)
    {
      YAE_LIFETIME_START(lifetime, " 1 -- LibassInitDoneEvent");
    }

    TLibass * libass_;
    YAE_LIFETIME(lifetime);
  };

  //----------------------------------------------------------------
  // Canvas::processEvent
  //
  bool
  Canvas::processEvent(QEvent * event)
  {
    if (event->type() == QEvent::User)
    {
      PaintCanvasEvent * repaintEvent = dynamic_cast<PaintCanvasEvent *>(event);
      if (repaintEvent)
      {
        refresh();
        repaintEvent->payload_.setDelivered(true);
        event->accept();
        return true;
      }

      RenderFrameEvent * renderEvent = dynamic_cast<RenderFrameEvent *>(event);
      if (renderEvent)
      {
        event->accept();

        TVideoFramePtr frame;
        renderEvent->payload_.get(frame);
        loadFrame(frame);
        return true;
      }

      InitializeBackendEvent * initBackendEvent =
        dynamic_cast<InitializeBackendEvent *>(event);
      if (initBackendEvent)
      {
        event->accept();

        initializePrivateBackend();
        refresh();
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

    // allow the layers to process the event:
    for (std::list<ILayer *>::reverse_iterator i = layers_.rbegin();
         i != layers_.rend(); ++i)
    {
      ILayer * layer = *i;
      if (!layer->isEnabled())
      {
        continue;
      }

      if (layer->processEvent(this, event))
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Canvas::resize
  //
  void
  Canvas::resize(int w, int h)
  {
    if (w_ == w && h_ == h)
    {
      return;
    }

    w_ = w;
    h_ = h;

    if (overlay_ && (subsInOverlay_ || showTheGreeting_))
    {
      updateOverlay(true);
    }

    // resize the layers:
    for (std::list<ILayer *>::iterator i = layers_.begin();
         i != layers_.end(); ++i)
    {
      ILayer * layer = *i;
      layer->resizeTo(this);
    }
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
    if (!croppedWidth || !croppedHeight)
    {
      return;
    }

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

    YAE_OGL_11_HERE();

    YAE_OGL_11(glViewport(GLint(x + 0.5), GLint(y + 0.5),
                          GLsizei(w + 0.5), GLsizei(h + 0.5)));

    TGLSaveMatrixState pushMatrix(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(0.0, croppedWidth, croppedHeight, 0.0, -1.0, 1.0));

    if (cameraRotation && cameraRotation % 90 == 0)
    {
      YAE_OGL_11(glTranslated(0.5 * croppedWidth, 0.5 * croppedHeight, 0));
      YAE_OGL_11(glRotated(double(cameraRotation), 0, 0, 1));

      if (cameraRotation % 180 != 0)
      {
        YAE_OGL_11(glTranslated(-0.5 * croppedHeight, -0.5 * croppedWidth, 0));
      }
      else
      {
        YAE_OGL_11(glTranslated(-0.5 * croppedWidth, -0.5 * croppedHeight, 0));
      }
    }

    canvas->draw();
    yae_assert_gl_no_error();

#if 0
    // FIXME: for debugging
    {
      YAE_OGL_11(glDisable(GL_LIGHTING));
      YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
      YAE_OGL_11(glLineWidth(2.0));
      YAE_OGL_11(glBegin(GL_LINES));
      {
        YAE_OGL_11(glColor3ub(0x7f, 0x00, 0x10));
        YAE_OGL_11(glVertex2i(croppedWidth / 10, croppedHeight / 10));
        YAE_OGL_11(glVertex2i(2 * croppedWidth / 10, croppedHeight / 10));
        YAE_OGL_11(glColor3ub(0xff, 0x00, 0x20));
        YAE_OGL_11(glVertex2i(2 * croppedWidth / 10, croppedHeight / 10));
        YAE_OGL_11(glVertex2i(3 * croppedWidth / 10, croppedHeight / 10));

        YAE_OGL_11(glColor3ub(0x10, 0x7f, 0x00));
        YAE_OGL_11(glVertex2i(croppedWidth / 10, croppedHeight / 10));
        YAE_OGL_11(glVertex2i(croppedWidth / 10, 2 * croppedHeight / 10));
        YAE_OGL_11(glColor3ub(0x20, 0xff, 0x00));
        YAE_OGL_11(glVertex2i(croppedWidth / 10, 2 * croppedHeight / 10));
        YAE_OGL_11(glVertex2i(croppedWidth / 10, 3 * croppedHeight / 10));
      }
      YAE_OGL_11(glEnd());
    }
#endif

    // reset OpenGL to default/initial state:
    yae_reset_opengl_to_initial_state();
  }

  //----------------------------------------------------------------
  // paintCheckerBoard
  //
  static void
  paintCheckerBoard(int canvasWidth, int canvasHeight)
  {
    YAE_OGL_11_HERE();
    YAE_OGL_11(glViewport(0, 0, canvasWidth, canvasHeight));

    TGLSaveMatrixState pushMatrix(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(0, canvasWidth, canvasHeight, 0, -1.0, 1.0));

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
        YAE_OGL_11(glColor3fv(color));

        YAE_OGL_11(glRecti(x, y, x1, y1));
      }
    }
  }

  //----------------------------------------------------------------
  // Canvas::paintCanvas
  //
  void
  Canvas::paintCanvas()
  {
    YAE_BENCHMARK(benchmark, "Canvas::paintCanvas");

    // this is just to prevent concurrent OpenGL access to the same context:
    TMakeCurrentContext lock(context());

#ifndef YAE_USE_QOPENGL_WIDGET
    // initialize OpenGL GLEW wrapper:
    static bool initialized = initializeGlew();
#endif

    // reset OpenGL to default/initial state:
    yae_reset_opengl_to_initial_state();

    if (!private_ || (!overlay_ && (showTheGreeting_ || subsInOverlay_)))
    {
      initializePrivateBackend();
      updateOverlay(true);
    }

    if (canvasWidth() == 0 || canvasHeight() == 0)
    {
      return;
    }

    TGLSaveMatrixState pushMatrix1(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glLoadIdentity());

    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    int canvasWidth = this->canvasWidth();
    int canvasHeight = this->canvasHeight();

    // draw a checkerboard to help visualize the alpha channel:
    if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                 pixelFormat::kPaletted)))
    {
      paintCheckerBoard(canvasWidth, canvasHeight);

      YAE_OGL_11(glEnable(GL_BLEND));
      YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }
    else
    {
      YAE_OGL_11(glClearColor(0, 0, 0, 1));
      YAE_OGL_11(glClear(GL_COLOR_BUFFER_BIT));
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
        YAE_OGL_11(glEnable(GL_BLEND));
        YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        paintImage(overlay_, canvasWidth, canvasHeight, kScaleToFit);
        YAE_OGL_11(glDisable(GL_BLEND));
      }
      else
      {
        qApp->postEvent(&eventReceiver_,
                        new UpdateOverlayEvent(),
                        Qt::HighEventPriority);
      }
    }

    // draw the layers:
    for (std::list<ILayer *>::iterator i = layers_.begin();
         i != layers_.end(); ++i)
    {
      ILayer * layer = *i;
      if (!layer->isEnabled())
      {
        continue;
      }

      layer->paint(this);
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

    bool ok = private_->loadFrame(context(), frame);
    showTheGreeting_ = false;
    setSubs(frame->subs_);

    refresh();

    if (ok && delegate_)
    {
      delegate_->inhibitScreenSaver();
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
    double bbox_ar = bbox_h > 0.0 ? double(bbox_w) / double(bbox_h) : 0.0;
    double frame_ar = frame_h > 0.0 ? double(frame_w) / double(frame_h) : 0.0;

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
  // Canvas::updateOverlay
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

    double w = this->canvasWidth();
    double h = this->canvasHeight();

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

    subsInOverlay_ = overlay_->loadFrame(context(), vf);
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

    double w = this->canvasWidth();
    double h = this->canvasHeight();

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

    bool ok = overlay_->loadFrame(context(), vf);
    YAE_ASSERT(ok);
    return ok;
  }

  //----------------------------------------------------------------
  // Canvas::skipColorConverter
  //
  void
  Canvas::skipColorConverter(bool enable)
  {
    private_->skipColorConverter(context(), enable);
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
    qApp->postEvent(&canvas->eventReceiver_,
                    new LibassInitDoneEvent(libass),
                    Qt::HighEventPriority);
  }
}
