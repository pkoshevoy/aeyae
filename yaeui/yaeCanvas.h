// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_H_
#define YAE_CANVAS_H_

// standard libraries:
#include <list>
#include <set>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/thread.hpp>
#endif

// Qt includes:
#include <QEvent>
#include <QObject>
#include <QString>

// yae includes:
#include "yae/api/yae_shared_ptr.h"
#include "yae/thread/yae_threading.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/video/yae_video.h"
#include "yae/video/yae_auto_crop.h"
#include "yae/video/yae_video_canvas.h"
#include "yae/video/yae_synchronous.h"

// local includes:
#include "yaeCanvasRenderer.h"
#include "yaeLibass.h"
#include "yaeThumbnailProvider.h"


namespace yae
{
  // forward declarations:
  struct Canvas;

  //----------------------------------------------------------------
  // AutoCropEvent
  //
  struct AutoCropEvent : public QEvent
  {
    AutoCropEvent(const TCropFrame & cropFrame):
      QEvent(QEvent::User),
      cropFrame_(cropFrame)
    {}

    TCropFrame cropFrame_;
  };

  //----------------------------------------------------------------
  // BufferedEvent
  //
  struct BufferedEvent : public QEvent
  {
    //----------------------------------------------------------------
    // TPayload
    //
    struct TPayload
    {
      TPayload():
        delivered_(true)
      {}

      bool setDelivered(bool delivered)
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        bool postThePayload = delivered_;
        delivered_ = delivered;
        return postThePayload;
      }

    private:
      mutable boost::mutex mutex_;
      bool delivered_;
    };

    BufferedEvent(TPayload & payload):
      QEvent(QEvent::User),
      payload_(payload)
    {}

    TPayload & payload_;
  };

  //----------------------------------------------------------------
  // CancelableEvent
  //
  struct CancelableEvent : public QEvent
  {
    //----------------------------------------------------------------
    // Ticket
    //
    struct Ticket
    {
      Ticket():
        canceled_(false)
      {}

      inline void cancel()
      { canceled_ = true; }

      inline bool isCanceled() const
      { return canceled_; }

    protected:
      bool canceled_;
    };

    CancelableEvent(const yae::shared_ptr<Ticket> & ticket):
      QEvent(QEvent::User),
      ticket_(ticket)
    {}

    virtual bool execute() = 0;

    yae::shared_ptr<Ticket> ticket_;
  };

  //----------------------------------------------------------------
  // Canvas
  //
  struct YAEUI_API Canvas : public IVideoCanvas
  {
#ifndef YAE_USE_QOPENGL_WIDGET
    bool glewInitialized_;
#endif

    //----------------------------------------------------------------
    // IDelegate
    //
    struct YAEUI_API IDelegate
    {
      virtual ~IDelegate() {}

      virtual Canvas & windowCanvas() = 0;
      virtual bool isVisible() = 0;
      virtual void repaint() = 0;
      virtual void requestRepaint() = 0;
      virtual void inhibitScreenSaver() = 0;

      virtual double device_pixel_ratio() const = 0;
      virtual double screen_width() const = 0;
      virtual double screen_height() const = 0;

      virtual double screen_width_mm() const = 0;
      virtual double screen_height_mm() const = 0;

      virtual double physical_dpi_x() const = 0;
      virtual double physical_dpi_y() const = 0;

      virtual double logical_dpi_x() const = 0;
      virtual double logical_dpi_y() const = 0;
    };

    //----------------------------------------------------------------
    // ILayer
    //
    struct YAEUI_API ILayer
    {
      ILayer(): enabled_(false) {}
      virtual ~ILayer() {}

      virtual void setContext(const yae::shared_ptr<IOpenGLContext> & context)
      { context_ = context; }

      inline const yae::shared_ptr<IOpenGLContext> & context() const
      { return context_; }

      virtual void setDelegate(const yae::shared_ptr<IDelegate> & delegate)
      { delegate_ = delegate; }

      inline const yae::shared_ptr<IDelegate> & delegate() const
      { return delegate_; }

      inline bool isEnabled() const
      { return enabled_; }

      virtual void setEnabled(bool enable)
      {
        enabled_ = enable;
        requestRepaint();
      }

      virtual void requestRepaint() = 0;
      virtual bool resizeTo(const Canvas * canvas) = 0;
      virtual void paint(Canvas * canvas) = 0;

      //----------------------------------------------------------------
      // IAnimator
      //
      struct IAnimator
      {
        virtual ~IAnimator() {}
        virtual void animate(ILayer &, yae::shared_ptr<IAnimator> a) = 0;
      };

      // NOTE: the layer may retain a (weak) pointer for the given animator:
      virtual void addAnimator(const yae::shared_ptr<IAnimator> & a) = 0;
      virtual void delAnimator(const yae::shared_ptr<IAnimator> & a) = 0;

      virtual bool processEvent(Canvas * canvas, QEvent * event) = 0;

      // lookup image provider for a given resource URL:
      virtual TImageProviderPtr
      getImageProvider(const QString & imageUrl, QString & imageId) const = 0;

    protected:
      yae::shared_ptr<IOpenGLContext> context_;
      yae::shared_ptr<IDelegate> delegate_;
      bool enabled_;
    };

    Canvas(const yae::shared_ptr<IOpenGLContext> & context);
    ~Canvas();

    inline double devicePixelRatio() const
    { return devicePixelRatio_; }

    inline int canvasWidth() const
    { return w_; }

    inline int canvasHeight() const
    { return h_; }

    inline const yae::shared_ptr<IOpenGLContext> & contextPtr() const
    { return context_; }

    inline IOpenGLContext & context()
    { return *context_; }

    void setDelegate(const yae::shared_ptr<IDelegate> & delegate);

    inline const yae::shared_ptr<IDelegate> & delegate() const
    { return delegate_; }

    // initialize private backend rendering object,
    // should not be called prior to initializing GLEW:
    void initializePrivateBackend();

    inline bool initialized() const
    { return private_ && overlay_; }

    inline CanvasRenderer * canvasRenderer() const
    { return private_; }

    inline CanvasRenderer * overlayRenderer() const
    { return overlay_; }

    // generic mechanism for delegating canvas painting and event processing:
    // NOTE: 1. the last appended layer is front-most
    //       2. painting -- back-to-front, for all layers
    //       3. event handling -- front-to-back, until handled
    void append(ILayer * layer);

    // lookup a fragment shader for a given pixel format, if one exits:
    const TFragmentShader * fragmentShaderFor(const VideoTraits & vtts) const;

    // specify reader ID tag so that the Canvas can discard
    // frames originating from any other reader:
    void acceptFramesWithReaderId(unsigned int readerId);

    // this is called to add custom fonts that may have been
    // embedded in the video file:
    void libassAddFont(const char * filename,
                       const unsigned char * data,
                       const std::size_t size);

    // discard currently stored image data, repaint the canvas:
    void clear();
    void clearOverlay();

    // flush previously processed dialogue lines:
    void libassFlushTrack();

    // helper:
    void refresh();

    // NOTE: thread safe, will post a PaintCanvasEvent if there isn't one
    // already posted-but-not-delivered (to avoid flooding the event queue):
    void requestRepaint();

    // virtual: this will be called from a secondary thread:
    bool render(const TVideoFramePtr & frame);

    //----------------------------------------------------------------
    // ILoadFrameObserver
    //
    struct YAEUI_API ILoadFrameObserver
    {
      virtual ~ILoadFrameObserver() {}
      virtual void frameLoaded(Canvas * c, const TVideoFramePtr & f) = 0;
    };

    void addLoadFrameObserver(const yae::shared_ptr<ILoadFrameObserver> &);
    void delLoadFrameObserver(const yae::shared_ptr<ILoadFrameObserver> &);

    // helpers:
    bool loadFrame(const TVideoFramePtr & frame);
    TVideoFramePtr currentFrame() const;

    // helpers:
    void setSubs(const std::list<TSubsFrame> & subs);
    bool updateOverlay(bool reparse);

    // helpers:
    void setGreeting(const QString & greeting);
    bool updateGreeting();

    // accessor to the current greeting message:
    inline const QString & greeting() const
    { return greeting_; }

    // if enabled, skip using a fragment shader (even if one is available)
    // for non-native pixel formats:
    void skipColorConverter(bool enable);

    // NOTE: In order to avoid blurring interlaced frames vertical scaling
    // is disabled by default.  However, if the video is not interlaced
    // and display aspect ratio is less than encoded frame aspect ratio
    // scaling down frame width would result in loss of information,
    // therefore scaling up frame height would be preferred.
    //
    // use this to enable/disable frame height scaling:
    void enableVerticalScaling(bool enable);

    // use this to override auto-detected aspect ratio:
    void overrideDisplayAspectRatio(double dar);

    // use this to crop letterbox pillars and bars:
    void cropFrame(double darCropped);

    // use this to zoom/crop a portion of the frame
    // to eliminate letterbox pillars and/or bars;
    void cropFrame(const TCropFrame & crop);

    // start crop frame detection thread and deliver the results
    // asynchronously via a callback:
    void cropAutoDetect(void * callbackContext, TAutoCropCallback callback);
    void cropAutoDetectStop();

    // accessors to full resolution frame dimensions
    // after overriding display aspect ratio and cropping:
    double imageWidth() const;
    double imageHeight() const;

    // return width/height image aspect ration
    // and pass back image width and height
    //
    // NOTE: the image rotation angle is accounted for
    // (width <-> height are swapped as necessary)
    // before calculating the aspect ratio.
    //
    // NOTE: width and height are preprocessed according to current
    // frame crop and aspect ratio settings.
    //
    double imageAspectRatio(double & w, double & h) const;

    enum TRenderMode
    {
      // this will result in letterbox bars or pillars rendering, the
      // image will be scaled up or down to fit within canvas bounding box:
      kScaleToFit = 0,

      // this will avoid letterbox bars and pillars rendering, the
      // image will be scaled up and cropped by the canvas bounding box:
      kCropToFill = 1
    };

    // crop-to-fill may be useful in a full screen canvas:
    void setRenderMode(TRenderMode renderMode);

    // this will be called from a helper thread
    // once it is done updating fontconfig cache for libass:
    static void libassInitDoneCallback(void * canvas, TLibass * libass);

    // helper:
    void resize(double devicePixelRatio, int w, int h);

    // helper:
    inline bool overlayHasContent() const
    { return subsInOverlay_ || showTheGreeting_; }

    // sets up GL_VIEWPORT to fill the canvas window
    // and inits GL_PROJECTION with identity matrix,
    // then calls the helper paint_background, paint_canvas, paint_layers:
    void paintCanvas();

    // paint everything (greeting, video, subtitles)
    // into a given canvas region.
    //
    // NOTE: these do not change GL_VIEWPORT
    void paint_black(double x, double y, double w, double h);
    void paint_checkerboard(double x, double y, double w, double h);
    void paint_background(double x, double y, double w, double h);
    void paint_canvas(double x, double y, double w, double h);
    void paint_layers(double x, double y, double w, double h);

    //----------------------------------------------------------------
    // PaintCanvasEvent
    //
    struct PaintCanvasEvent : public BufferedEvent
    {
      PaintCanvasEvent(TPayload & payload):
        BufferedEvent(payload)
      {
        YAE_LIFETIME_START(lifetime, " 2 -- PaintCanvasEvent");
      }

      YAE_LIFETIME(lifetime);
    };

    //----------------------------------------------------------------
    // RenderFrameEvent
    //
    struct RenderFrameEvent : public QEvent
    {
      //----------------------------------------------------------------
      // TPayload
      //
      struct TPayload
      {
        TPayload():
          expectedReaderId_((unsigned int)~0)
        {}

        bool set(const TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          bool postThePayload = !frame_;
          frame_ = frame;
          return postThePayload;
        }

        void get(TVideoFramePtr & frame)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          if (frame_ && frame_->readerId_ == expectedReaderId_)
          {
            frame = frame_;
          }
          frame_ = TVideoFramePtr();
        }

        void setExpectedReaderId(unsigned int readerId)
        {
          boost::lock_guard<boost::mutex> lock(mutex_);
          expectedReaderId_ = readerId;
        }

      private:
        mutable boost::mutex mutex_;
        TVideoFramePtr frame_;

        // frames with matching reader ID tag will be rendered,
        // mismatching frames will be ignored:
        unsigned int expectedReaderId_;
      };

      RenderFrameEvent(TPayload & payload):
        QEvent(QEvent::User),
        payload_(payload)
      {
        YAE_LIFETIME_START(lifetime, " 4 -- RenderFrameEvent");
      }

      TPayload & payload_;
      YAE_LIFETIME(lifetime);
    };

  protected:
    // NOTE: events will be delivered on the main thread:
    bool processEvent(QEvent * event);

    //----------------------------------------------------------------
    // TEventReceiver
    //
    struct TEventReceiver : public QObject
    {
      TEventReceiver(Canvas & canvas):
        canvas_(canvas)
      {}

      // virtual:
      bool event(QEvent * event)
      {
        if (canvas_.processEvent(event))
        {
          return true;
        }

        return QObject::event(event);
      }

      Canvas & canvas_;
    };

    friend struct TEventReceiver;
    TEventReceiver eventReceiver_;

    yae::shared_ptr<IOpenGLContext> context_;
    yae::shared_ptr<IDelegate> delegate_;
    PaintCanvasEvent::TPayload paintCanvasEvent_;
    RenderFrameEvent::TPayload renderFrameEvent_;
    CanvasRenderer * private_;
    CanvasRenderer * overlay_;
    TLibass libass_;
    TAssTrackPtr subtitles_;
    TAssTrackPtr captions_;
    bool showTheGreeting_;
    bool subsInOverlay_;
    TRenderMode renderMode_;

    // canvas size:
    double devicePixelRatio_;
    int w_;
    int h_;

    // keep track of previously displayed subtitles
    // in order to avoid re-rendering the same subtitles with every frame:
    std::list<TSubsFrame> subs_;

    // the greeting message shown to the user
    QString greeting_;

    // automatic frame margin detection:
    TAutoCropDetect autoCrop_;
    Thread<TAutoCropDetect> autoCropThread_;

    // a list of painting and event handling delegates,
    // traversed front-to-back for painting
    // and back-to-front for event processing:
    std::list<ILayer *> layers_;

    // observers notified when loadFrame is called:
    std::set<yae::weak_ptr<ILoadFrameObserver> > loadFrameObservers_;
  };


  //----------------------------------------------------------------
  // SetupModelview
  //
  struct YAEUI_API SetupModelview
  {
    SetupModelview(double canvas_x,
                   double canvas_y,
                   double canvas_w,
                   double canvas_h,
                   CanvasRenderer * renderer = NULL,
                   Canvas::TRenderMode mode = Canvas::kScaleToFit);

  protected:
    TGLSaveMatrixState modelview_;
  };

}


#endif // YAE_CANVAS_H_
