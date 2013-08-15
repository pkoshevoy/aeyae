// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_H_
#define YAE_CANVAS_H_

// boost includes:
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

// Qt includes:
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QGLWidget>
#include <QTimer>
#include <QList>
#include <QUrl>

// yae includes:
#include <yaeAPI.h>
#include <yaeAutoCrop.h>
#include <yaeVideoCanvas.h>
#include <yaeSynchronous.h>
#include <yaeThreading.h>


//----------------------------------------------------------------
// yae_to_opengl
//
// returns number of sample planes supported by OpenGL,
// passes back parameters to use with glTexImage2D
//
YAE_API unsigned int
yae_to_opengl(yae::TPixelFormatId yaePixelFormat,
              GLint & internalFormat,
              GLenum & format,
              GLenum & dataType,
              GLint & shouldSwapBytes);

namespace yae
{
  class Canvas;
  class TLibass;

  //----------------------------------------------------------------
  // Canvas
  //
  class YAE_API Canvas : public QGLWidget,
                         public IVideoCanvas
  {
    Q_OBJECT;

  public:
    class TPrivate;

    Canvas(const QGLFormat & format,
           QWidget * parent = 0,
           const QGLWidget * shareWidget = 0,
           Qt::WindowFlags f = 0);
    ~Canvas();

    // initialize private backend rendering object,
    // should not be called prior to initializing GLEW:
    void initializePrivateBackend();

    // specify reader ID tag so that the Canvas can discard
    // frames originating from any other reader:
    void acceptFramesWithReaderId(unsigned int readerId);

    // discard currently stored image data, repaint the canvas:
    void clear();
    void clearOverlay();

    // helper:
    void refresh();

    // virtual: this will be called from a secondary thread:
    bool render(const TVideoFramePtr & frame);

    // helpers:
    bool loadFrame(const TVideoFramePtr & frame);
    TVideoFramePtr currentFrame() const;

    // helpers:
    void setSubs(const std::list<TSubsFrame> & subs);
    bool updateOverlay(bool reparse);

    // helpers:
    void setGreeting(const QString & greeting);
    bool updateGreeting();

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

  signals:
    void toggleFullScreen();

  public slots:
    void hideCursor();
    void wakeScreenSaver();

  protected:
    // virtual:
    bool event(QEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseDoubleClickEvent(QMouseEvent * event);
    void resizeEvent(QResizeEvent * event);

    // virtual: Qt/OpenGL stuff:
    void initializeGL();
    void paintGL();

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
      {}

      TPayload & payload_;
    };

    RenderFrameEvent::TPayload payload_;
    TPrivate * private_;
    TPrivate * overlay_;
    TLibass * libass_;
    bool showTheGreeting_;
    bool subsInOverlay_;
    TRenderMode renderMode_;

    // a single shot timer for hiding the cursor:
    QTimer timerHideCursor_;

    // a single shot timer for preventing screen saver:
    QTimer timerScreenSaver_;

    // keep track of previously displayed subtitles
    // in order to avoid re-rendering the same subtitles with every frame:
    std::list<TSubsFrame> subs_;

    // the greeting message shown to the user
    QString greeting_;

    // automatic frame margin detection:
    TAutoCropDetect autoCrop_;
    Thread<TAutoCropDetect> autoCropThread_;
  };
}


#endif // YAE_CANVAS_H_
