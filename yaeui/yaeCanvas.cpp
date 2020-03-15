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
#include <QEvent>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#elif !defined(_WIN32)
#include <QtDBus/QtDBus>
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
      yae_error
        << "GLEW init failed: " << glewGetErrorString(err);
      YAE_ASSERT(false);
    }

    return true;
  }
#endif


  //----------------------------------------------------------------
  // Canvas::Canvas
  //
  Canvas::Canvas(const yae::shared_ptr<IOpenGLContext> & ctx):
#ifndef YAE_USE_QOPENGL_WIDGET
    glewInitialized_(false),
#endif
    eventReceiver_(*this),
    context_(ctx),
    private_(NULL),
    overlay_(NULL),
    showTheGreeting_(false),
    subsInOverlay_(false),
    renderMode_(Canvas::kScaleToFit),
    devicePixelRatio_(1.0),
    w_(0),
    h_(0)
  {
    libass_.asyncInit(&Canvas::libassInitDoneCallback, this);
  }

  //----------------------------------------------------------------
  // Canvas::~Canvas
  //
  Canvas::~Canvas()
  {
    delete private_;
    delete overlay_;
  }

  //----------------------------------------------------------------
  // Canvas::setDelegate
  //
  void
  Canvas::setDelegate(const yae::shared_ptr<IDelegate> & delegate)
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
    if (!glewInitialized_)
    {
      // initialize OpenGL GLEW wrapper:
      glewInitialized_ = initializeGlew();
    }

    if (!glewInitialized_)
    {
      return;
    }
#endif

    delete private_;
    private_ = NULL;

    delete overlay_;
    overlay_ = NULL;

    private_ = new CanvasRenderer();
    overlay_ = new CanvasRenderer();
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

    layer->setDelegate(delegate_);
    layer->setContext(context_);
    layers_.push_back(layer);
    layer->resizeTo(this);
    layer->requestRepaint();
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
    libass_.addCustomFont(TFontAttachment(filename, data, size));
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

    subtitles_.reset();
    captions_.reset();

    showTheGreeting_ = false;
    subsInOverlay_ = false;
    subs_.clear();
  }

  //----------------------------------------------------------------
  // Canvas::libassFlushTrack
  //
  void
  Canvas::libassFlushTrack()
  {
    if (subtitles_)
    {
      subtitles_->flushEvents();
    }

    if (captions_)
    {
      captions_->flushEvents();
    }
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
    QEvent::Type et = event->type();

    if (et == QEvent::User)
    {
      PaintCanvasEvent * repaintEvent =
        dynamic_cast<PaintCanvasEvent *>(event);
      if (repaintEvent)
      {
        refresh();
        repaintEvent->payload_.setDelivered(true);
        event->accept();
        return true;
      }

      RenderFrameEvent * renderEvent =
        dynamic_cast<RenderFrameEvent *>(event);
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

        libass_.asyncInitStop();
        updateOverlay(true);
        refresh();
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
        event->accept();
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // Canvas::resize
  //
  void
  Canvas::resize(double devicePixelRatio, int logical_w, int logical_h)
  {
    int new_w = logical_w * devicePixelRatio;
    int new_h = logical_h * devicePixelRatio;
    if (devicePixelRatio_ == devicePixelRatio && w_ == new_w && h_ == new_h)
    {
      return;
    }

    devicePixelRatio_ = devicePixelRatio;
    w_ = new_w;
    h_ = new_h;

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
  // SetupModelview::SetupModelview
  //
  SetupModelview::SetupModelview(double canvas_x,
                                 double canvas_y,
                                 double canvas_w,
                                 double canvas_h,
                                 CanvasRenderer * renderer,
                                 Canvas::TRenderMode mode):
    modelview_(GL_MODELVIEW)
  {
    double cropped_w = 0.0;
    double cropped_h = 0.0;
    int camera_rotation = 0;

    if (renderer)
    {
      renderer->imageWidthHeightRotated(cropped_w,
                                        cropped_h,
                                        camera_rotation);
    }
    else
    {
      cropped_w = canvas_w;
      cropped_h = canvas_h;
    }

    if (!cropped_w || !cropped_h)
    {
      return;
    }

    double dar = cropped_w / cropped_h;
    double car = canvas_w / canvas_h;

    double x = canvas_x;
    double y = canvas_y;
    double w = canvas_w;
    double h = canvas_h;

    if (mode == Canvas::kScaleToFit)
    {
      if (dar < car)
      {
        w = canvas_h * dar;
        x = canvas_x + 0.5 * (canvas_w - w);
      }
      else
      {
        h = canvas_w / dar;
        y = canvas_y + 0.5 * (canvas_h - h);
      }
    }
    else
    {
      if (dar < car)
      {
        h = canvas_w / dar;
        y = canvas_y + 0.5 * (canvas_h - h);
      }
      else
      {
        w = canvas_h * dar;
        x = canvas_x + 0.5 * (canvas_w - w);
      }
    }

    YAE_OGL_11_HERE();

    YAE_OGL_11(glTranslated(x, y, 0));

    double scale =
      mode == Canvas::kScaleToFit ?
      std::min(canvas_w / cropped_w, canvas_h / cropped_h) :
      std::max(canvas_w / cropped_w, canvas_h / cropped_h);

    YAE_OGL_11(glScaled(scale, scale, 1));

    if (camera_rotation && camera_rotation % 90 == 0)
    {
      YAE_OGL_11(glTranslated(0.5 * cropped_w,
                              0.5 * cropped_h, 0));
      YAE_OGL_11(glRotated(double(camera_rotation), 0, 0, 1));

      if (camera_rotation % 180 != 0)
      {
        YAE_OGL_11(glTranslated(-0.5 * cropped_h,
                                -0.5 * cropped_w, 0));
      }
      else
      {
        YAE_OGL_11(glTranslated(-0.5 * cropped_w,
                                -0.5 * cropped_h, 0));
      }
    }
  }

  //----------------------------------------------------------------
  // paint_checker_board
  //
  static void
  paint_checker_board(double canvas_x,
                      double canvas_y,
                      double canvas_w,
                      double canvas_h)
  {
    YAE_OGL_11_HERE();

    SetupModelview modelview(canvas_x, canvas_y, canvas_w, canvas_h);

    float zebra[2][3] = {
      { 1.0f, 1.0f, 1.0f },
      { 0.7f, 0.7f, 0.7f }
    };

    int edgeSize = 24;
    bool evenRow = false;
    for (int y = 0; y < canvas_h; y += edgeSize, evenRow = !evenRow)
    {
      int y1 = std::min(y + edgeSize, int(canvas_h));

      bool evenCol = false;
      for (int x = 0; x < canvas_w; x += edgeSize, evenCol = !evenCol)
      {
        int x1 = std::min(x + edgeSize, int(canvas_w));

        float * color = (evenRow ^ evenCol) ? zebra[0] : zebra[1];
        YAE_OGL_11(glColor3fv(color));

        YAE_OGL_11(glRecti(x, y, x1, y1));
      }
    }
  }

  //----------------------------------------------------------------
  // paint_black_rectangle
  //
  static void
  paint_black_rectangle(double canvas_x,
                        double canvas_y,
                        double canvas_w,
                        double canvas_h)
  {
    YAE_OGL_11_HERE();

    SetupModelview modelview(canvas_x, canvas_y, canvas_w, canvas_h);

    YAE_OGL_11(glColor3d(0.0, 0.0, 0.0));
    YAE_OGL_11(glRectd(canvas_x,
                       canvas_y,
                       canvas_x + canvas_w,
                       canvas_y + canvas_h));
  }

  //----------------------------------------------------------------
  // Canvas::paintCanvas
  //
  void
  Canvas::paintCanvas()
  {
    // YAE_BENCHMARK(benchmark, "Canvas::paintCanvas");

    // this is just to prevent concurrent OpenGL access to the same context:
    TMakeCurrentContext lock(context());

#ifndef YAE_USE_QOPENGL_WIDGET
    if (!glewInitialized_)
    {
      // initialize OpenGL GLEW wrapper:
      glewInitialized_ = initializeGlew();
    }

    if (!glewInitialized_)
    {
      return;
    }
#endif

    YAE_OPENGL_HERE();
    yae_assert_gl_no_error();

    if (glCheckFramebufferStatus)
    {
      GLenum s = YAE_OPENGL(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
      if (s != GL_FRAMEBUFFER_COMPLETE)
      {
#ifndef NDEBUG
        // this happens on startup on the mac, where
        // QGLWidget::paintGL is called before the window
        // is actually created, just ignore it:
        yae_warn
          << "Canvas::paintCanvas: frambuffer incomplete, skipping...";
#endif
        return;
      }
    }

    // reset OpenGL to default/initial state:
    yae_reset_opengl_to_initial_state();

    if (!private_ || (!overlay_ && (showTheGreeting_ || subsInOverlay_)))
    {
      initializePrivateBackend();
      updateOverlay(true);
    }

    int canvasWidth = this->canvasWidth();
    int canvasHeight = this->canvasHeight();

    if (!(canvasWidth && canvasHeight))
    {
      return;
    }

    TGLSaveMatrixState push_modelview(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glViewport(0, 0, canvasWidth, canvasHeight));

    TGLSaveMatrixState push_projection(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(// left:
                       0,

                       // right:
                       canvasWidth,

                       // bottom:
                       canvasHeight,

                       // top:
                       0,

                       // near:
                       -1.0,

                       // far:
                       1.0));

    TGLSaveState restore_state(GL_ENABLE_BIT);
    TGLSaveClientState restore_client_state(GL_CLIENT_ALL_ATTRIB_BITS);

    paint_background(0, 0, double(canvasWidth), double(canvasHeight));
    paint_canvas(0, 0, double(canvasWidth), double(canvasHeight));
    paint_layers(0, 0, double(canvasWidth), double(canvasHeight));

    // reset OpenGL to default/initial state:
    yae_reset_opengl_to_initial_state();
  }

  //----------------------------------------------------------------
  // Canvas::paint_black
  //
  void
  Canvas::paint_black(double canvas_x,
                      double canvas_y,
                      double canvas_w,
                      double canvas_h)
  {
    yae::paint_black_rectangle(canvas_x, canvas_y, canvas_w, canvas_h);

    // sanity check:
    yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // Canvas::paint_checkerboard
  //
  void
  Canvas::paint_checkerboard(double canvas_x,
                             double canvas_y,
                             double canvas_w,
                             double canvas_h)
  {
    yae::paint_checker_board(canvas_x, canvas_y, canvas_w, canvas_h);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // sanity check:
    yae_assert_gl_no_error();
  }

  //----------------------------------------------------------------
  // Canvas::paint_background
  //
  void
  Canvas::paint_background(double canvas_x,
                           double canvas_y,
                           double canvas_w,
                           double canvas_h)
  {
    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    // draw a checkerboard to help visualize the alpha channel:
    if (ptts && (ptts->flags_ & (pixelFormat::kAlpha |
                                 pixelFormat::kPaletted)))
    {
      paint_checkerboard(canvas_x, canvas_y, canvas_w, canvas_h);
    }
    else
    {
      paint_black(canvas_x, canvas_y, canvas_w, canvas_h);
    }
  }

  //----------------------------------------------------------------
  // Canvas::paint_canvas
  //
  void
  Canvas::paint_canvas(double canvas_x,
                       double canvas_y,
                       double canvas_w,
                       double canvas_h)
  {
    const pixelFormat::Traits * ptts =
      private_ ? private_->pixelTraits() : NULL;

    // draw the frame:
    if (ptts)
    {
      SetupModelview modelview(canvas_x,
                               canvas_y,
                               canvas_w,
                               canvas_h,
                               private_,
                               renderMode_);
      private_->draw();
      yae_assert_gl_no_error();
    }

    // draw the overlay:
    if (overlayHasContent())
    {
      if (overlay_ && overlay_->pixelTraits())
      {
        TGLSaveState restore_state(GL_ENABLE_BIT);
        YAE_OGL_11_HERE();
        YAE_OGL_11(glEnable(GL_BLEND));
        YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        SetupModelview modelview(canvas_x,
                                 canvas_y,
                                 canvas_w,
                                 canvas_h,
                                 overlay_,
                                 kScaleToFit);

        overlay_->draw();
        yae_assert_gl_no_error();
      }
      else
      {
        qApp->postEvent(&eventReceiver_,
                        new UpdateOverlayEvent(),
                        Qt::HighEventPriority);
      }
    }
  }

  //----------------------------------------------------------------
  // Canvas::paint_layers
  //
  void
  Canvas::paint_layers(double canvas_x,
                       double canvas_y,
                       double canvas_w,
                       double canvas_h)
  {
    // draw the layers:
    for (std::list<ILayer *>::iterator i = layers_.begin();
         i != layers_.end(); ++i)
    {
      ILayer * layer = *i;
      if (!layer->isEnabled())
      {
        continue;
      }

      TGLSaveState restore_state(GL_ENABLE_BIT);
      TGLSaveClientState restore_client_state(GL_CLIENT_ALL_ATTRIB_BITS);
      SetupModelview modelview(canvas_x, canvas_y, canvas_w, canvas_h);
      layer->paint(this);
    }
  }

  //----------------------------------------------------------------
  // Canvas::addLoadFrameObserver
  //
  void
  Canvas::addLoadFrameObserver(const yae::shared_ptr<ILoadFrameObserver> & o)
  {
    loadFrameObservers_.insert(o);
  }

  //----------------------------------------------------------------
  // Canvas::delLoadFrameObserver
  //
  void
  Canvas::delLoadFrameObserver(const yae::shared_ptr<ILoadFrameObserver> & o)
  {
    loadFrameObservers_.erase(o);
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

    typedef std::set<yae::weak_ptr<ILoadFrameObserver> >::iterator TIter;
    TIter i = loadFrameObservers_.begin();
    while (i != loadFrameObservers_.end())
    {
      TIter i0 = i;
      ++i;

      yae::shared_ptr<ILoadFrameObserver> observerPtr = i0->lock();
      if (observerPtr)
      {
        ILoadFrameObserver & observer = *observerPtr;
        observer.frameLoaded(this, frame);
      }
      else
      {
        // remove expired observers:
        loadFrameObservers_.erase(i0);
      }
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

    TMakeCurrentContext currentContext(context());
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
          std::string assa = r.getAssScript(subs);
          bool done = false;

          TAssTrackPtr & assTrack = (subs.traits_ == kSubsCEA608 ?
                                     captions_ : subtitles_);

          if (!assTrack)
          {
            if (subs.traits_ == kSubsSSA && subs.extraData_)
            {
              assTrack = libass_.track(subs.extraData_->data(0),
                                       subs.extraData_->rowBytes(0));
            }
            else if (subExt->headerSize())
            {
              assTrack = libass_.track(subExt->header(),
                                       subExt->headerSize());
            }
          }

          if (assTrack && libass_.isReady())
          {
            int64 pts = (int64)(subs.time_.sec() * 1000.0 + 0.5);
            assTrack->processData((unsigned char *)&assa[0], assa.size(), pts);
            nrectsPainted++;
            done = true;
          }
        }
      }
    }

    TAssTrackPtr assTracks[2];
    assTracks[0] = subtitles_;
    assTracks[1] = captions_;

    bool readyToRender = libass_.isReady() && frame;
    if (readyToRender)
    {
      libass_.setFrameSize(iw, ih);
    }

    for (unsigned int i = 0; readyToRender && i < 2; i++)
    {
      TAssTrackPtr & assTrack = assTracks[i];
      if (!assTrack)
      {
        continue;
      }

      // the list of images is owned by libass,
      // libass is responsible for their deallocation:
      const bool closedCaptions = (i == 1);
      int64 now = (int64)(frame->time_.sec() * 1000.0 + 0.5);

      int changeDetected = 0;
      ASS_Image * pic = assTrack->renderFrame(now, &changeDetected);
      libassSameSubs = !changeDetected;
      paintedSomeSubs = changeDetected;

      unsigned char bgr[3];
      while (pic && changeDetected)
      {
        double alpha = double(0xFF & (pic->color)) / 255.0;
        if (alpha <= 0.0)
        {
          alpha = 1.0;
        }

#ifdef __BIG_ENDIAN__
        bgr[2] = 0xFF & (pic->color >> 8);
        bgr[1] = 0xFF & (pic->color >> 16);
        bgr[0] = 0xFF & (pic->color >> 24);
#else
        bgr[0] = 0xFF & (pic->color >> 8);
        bgr[1] = 0xFF & (pic->color >> 16);
        bgr[2] = 0xFF & (pic->color >> 24);
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
            unsigned char a = (unsigned char)(alpha * double(*srcLine));
#ifdef __BIG_ENDIAN__
            dstLine[0] = a;
            memcpy(dstLine + 1, bgr, 3);
#else
            memcpy(dstLine, bgr, 3);
            dstLine[3] = a;
#endif
          }
        }

        QPainter::CompositionMode cm = QPainter::CompositionMode_SourceOver;
        if (closedCaptions && alpha < 1.0 &&
            (pic->type == ASS_Image::IMAGE_TYPE_SHADOW ||
             pic->type == ASS_Image::IMAGE_TYPE_OUTLINE))
        {
          // avoid painting semi-transparent background/shadow
          // over adjacent semi-transparent background/shadow,
          cm = QPainter::RasterOp_SourceOrDestination;
        }

        QPainter & painter = wrapper.getPainter();
        painter.setCompositionMode(cm);
        painter.drawImage(QRect(pic->dst_x + ix,
                                pic->dst_y + iy,
                                pic->w,
                                pic->h),
                          tmp, tmp.rect());

        pic = pic->next;
      }
    }

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
    showTheGreeting_ = !greeting.isEmpty();
    greeting_ = greeting;

    bool ok = initialized();
    if (!ok)
    {
      return;
    }

    updateGreeting();
    refresh();
  }

  //----------------------------------------------------------------
  // Canvas::updateGreeting
  //
  bool
  Canvas::updateGreeting()
  {
    double w = this->canvasWidth();
    double h = this->canvasHeight();

    if (!(overlay_ && w && h) || greeting_.isEmpty())
    {
      return false;
    }

    TMakeCurrentContext currentContext(context());
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
    ft.setStyleHint(QFont::SansSerif);
    ft.setStyleStrategy((QFont::StyleStrategy)
                        (QFont::PreferOutline |
                         QFont::PreferAntialias |
                         QFont::OpenGLCompatible));
    int px = std::max<int>(12, 56.0 * std::min<double>(w / max_w, h / max_h));
    ft.setPixelSize(px);
    painter.setFont(ft);

    int textAlignment = Qt::TextWordWrap | Qt::AlignCenter;
    QRect canvasBBox = subsFrm.rect();
    drawTextToFit(painter, canvasBBox, textAlignment, greeting_);
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
    yae_debug << "\nCROP FRAME AUTO DETECTED: "
              << "x = " << crop.x_ << ", "
              << "y = " << crop.y_ << ", "
              << "w = " << crop.w_ << ", "
              << "h = " << crop.h_;
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
      autoCropThread_.set_context(&autoCrop_);
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
    autoCropThread_.interrupt();
    autoCropThread_.wait();
    autoCropThread_.set_context(NULL);
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
      int rotated = 0;
      dar = private_->imageWidthHeightRotated(w, h, rotated) ? w / h : 0.0;
    }

    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::nativeAspectRatioUncropped
  //
  double
  Canvas::nativeAspectRatioUncropped() const
  {
    double dar = private_ ? private_->nativeAspectRatioUncropped() : 0.0;
    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::nativeAspectRatioUncroppedRotated
  //
  double
  Canvas::nativeAspectRatioUncroppedRotated(int & rotate) const
  {
    double dar = private_ ?
      private_->nativeAspectRatioUncroppedRotated(rotate) : 0.0;
    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::nativeAspectRatio
  //
  double
  Canvas::nativeAspectRatio() const
  {
    double dar = private_ ? private_->nativeAspectRatio() : 0.0;
    return dar;
  }

  //----------------------------------------------------------------
  // Canvas::nativeAspectRatioRotated
  //
  double
  Canvas::nativeAspectRatioRotated(int & rotate) const
  {
    double dar = private_ ? private_->nativeAspectRatioRotated(rotate) : 0.0;
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
