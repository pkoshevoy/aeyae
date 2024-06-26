// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Jul 23 21:23:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_WIDGET_H_
#define YAE_CANVAS_WIDGET_H_

// standard C++:
#include <iostream>

// Qt includes:
#include <QApplication>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QDesktopWidget>
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QScreen>
#endif
#include <QKeyEvent>
#include <QTimer>

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_benchmark.h"

// yaeui:
#include "yaeCanvas.h"
#include "yaeScreenSaverInhibitor.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // GetScreenInfo
  //
  struct GetScreenInfo
  {
    const QWidget * widget_;
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    const QWidget * screen_;
#else
    const QScreen * screen_;
#endif
    QRect geometry_;
    QSizeF size_mm_;

    GetScreenInfo(const QWidget * widget):
      widget_(widget),
      screen_(NULL)
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
      QDesktopWidget * dw = QApplication::desktop();
      int sn = dw->screenNumber(widget);
      screen_ = dw->screen(sn);
      size_mm_.setWidth(screen_->widthMM());
      size_mm_.setHeight(screen_->heightMM());
#else
      screen_ = widget->screen();
      size_mm_ = screen_->physicalSize();
#endif
      geometry_ = screen_->geometry();
    }

    inline double device_pixel_ratio() const
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
        double s = 1.0;
#else
        double s = screen_->devicePixelRatio();
#endif
        return s;
    }

    inline QRect available_geometry() const
    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
      return QApplication::desktop()->availableGeometry(widget_->window());
#else
      return screen_->availableGeometry();
#endif
    }

    inline double screen_width() const
    {
      double s = this->device_pixel_ratio();
      double w = geometry_.width() * s;
      return w;
    }

    inline double screen_height() const
    {
      double s = this->device_pixel_ratio();
      double h = geometry_.height() * s;
      return h;
    }

    inline double screen_width_mm() const
    { return size_mm_.width(); }

    inline double screen_height_mm() const
    { return size_mm_.height(); }

    inline double physical_dpi_x() const
    {
      double w = this->screen_width();
      double w_mm = this->screen_width_mm();
      double dpi_x = (w * 25.4) / w_mm;
      return dpi_x;
    }

    inline double physical_dpi_y() const
    {
      double h = this->screen_height();
      double h_mm = this->screen_height_mm();
      double dpi_y = (h * 25.4) / h_mm;
      return dpi_y;
    }

    inline double logical_dpi_x() const
    {
      double s = this->device_pixel_ratio();
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
      double dpi = screen_->logicalDpiX();
#else
      double dpi = screen_->logicalDotsPerInchX();
#endif
      return dpi * s;
    }

    inline double logical_dpi_y() const
    {
      double s = this->device_pixel_ratio();
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
      double dpi = screen_->logicalDpiY();
#else
      double dpi = screen_->logicalDotsPerInchY();
#endif
      return dpi * s;
    }
  };


  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots
  //
  class CanvasWidgetSignalsSlots : public QObject
  {
    Q_OBJECT;

  public:
    CanvasWidgetSignalsSlots(QWidget & widget);
    ~CanvasWidgetSignalsSlots();

    // accessors:
    inline QWidget & widget()
    { return widget_; }

    // signals are protected in Qt4, this is a workaround:
    void emit_toggle_fullscreen();
    void emit_esc_short();
    void emit_esc_long();

    void stopHideCursorTimer();
    void startHideCursorTimer();

  signals:
    void toggleFullScreen();
    void maybeHideCursor();
    void escShort();
    void escLong();

  public slots:
    void hideCursor();
    void showCursor();

    void focusChanged(QWidget * prev, QWidget * curr);

  private:
    CanvasWidgetSignalsSlots(const CanvasWidgetSignalsSlots &);
    CanvasWidgetSignalsSlots & operator = (const CanvasWidgetSignalsSlots &);

    QWidget & widget_;

    // a single shot timer for hiding the cursor:
    QTimer timerHideCursor_;

#ifdef __APPLE__
    void * appleRemoteControl_;
#endif
  };

  //----------------------------------------------------------------
  // CanvasWidget
  //
  template <typename TWidget>
  struct CanvasWidget : public TWidget, public Canvas
  {
    //----------------------------------------------------------------
    // TOpenGLWidget
    //
    typedef TWidget TOpenGLWidget;

    //----------------------------------------------------------------
    // TCanvasWidget
    //
    typedef CanvasWidget<TWidget> TCanvasWidget;

    //----------------------------------------------------------------
    // OpenGLContext
    //
    struct OpenGLContext : public IOpenGLContext
    {
      OpenGLContext(TOpenGLWidget & widget):
        widget_(widget)
      {}

      virtual bool makeCurrent()
      {
        widget_.makeCurrent();
#ifndef NDEBUG
        YAE_OGL_11_HERE();
#endif
        return true;
      }

      virtual void doneCurrent()
      { widget_.doneCurrent(); }

#ifndef YAE_USE_QGL_WIDGET
      //----------------------------------------------------------------
      // CurrentContext
      //
      struct CurrentContext : ICurrentContext
      {
        CurrentContext():
          ctx_(QOpenGLContext::currentContext())
        {
          surface_ = ctx_ ? ctx_->surface() : NULL;
        }

        // virtual:
        bool restore()
        {
          return ctx_ ? ctx_->makeCurrent(surface_) : false;
        }

        QOpenGLContext * ctx_;
        QSurface * surface_;
      };
#else
      //----------------------------------------------------------------
      // CurrentContext
      //
      struct CurrentContext : ICurrentContext
      {
        CurrentContext():
          ctx_(const_cast<QGLContext *>(QGLContext::currentContext()))
        {}

        // virtual:
        bool restore()
        {
          return ctx_ ? (ctx_->makeCurrent(), true) : false;
        }

        QGLContext * ctx_;
      };
#endif

      virtual yae::shared_ptr<ICurrentContext> getCurrent() const
      { return yae::shared_ptr<ICurrentContext>(new CurrentContext()); }

    protected:
      TOpenGLWidget & widget_;
    };

    //----------------------------------------------------------------
    // TDelegate
    //
    struct TDelegate : public Canvas::IDelegate
    {
      TDelegate(CanvasWidget<TWidget> & canvas):
        canvas_(canvas)
      {}

      virtual Canvas & windowCanvas()
      {
        return canvas_;
      }

      virtual bool isVisible()
      {
        return canvas_.TWidget::isVisible();
      }

      virtual void repaint()
      {
        YAE_BENCHMARK(probe1, "TCanvasWidget::TDelegate::repaint");
        // yae_dlog("CanvasWidget::TDelegate::repaint");

        // this is just to prevent concurrent OpenGL access
        // to the same context:
        TMakeCurrentContext lock(canvas_.Canvas::context());
        YAE_ASSERT(isVisible());

#ifndef YAE_USE_QGL_WIDGET
        {
          YAE_BENCHMARK(probe2, "canvas_.TWidget::update");
          canvas_.TWidget::update();
        }
#if 0
        QOpenGLContext * ctx = QOpenGLContext::currentContext();
        QSurface * surface = ctx->surface();
        yae_dlog("surface class: %i", surface->surfaceClass());
        yae_dlog("surface type: %i", surface->surfaceType());
        YAE_ASSERT(surface->supportsOpenGL());
        ctx->swapBuffers(surface);
#endif
#else
        {
          YAE_BENCHMARK(probe3, "canvas_.paintGL");
          canvas_.paintGL();
        }

        {
          YAE_BENCHMARK(probe4, "canvas_.TWidget::swapBuffers");
          canvas_.TWidget::swapBuffers();
        }
#endif
     }

      virtual void requestRepaint()
      {
        canvas_.requestRepaint();
      }

      virtual void inhibitScreenSaver()
      {
        ssi_.screenSaverInhibit();
      }

      virtual bool isFullScreen() const
      {
        return canvas_.window()->isFullScreen();
      }

      virtual void showFullScreen()
      {
        canvas_.window()->showFullScreen();
      }

      virtual void exitFullScreen()
      {
        canvas_.window()->showNormal();
      }

      virtual void getWindowFrame(TVec2D & origin, TVec2D & size) const
      {
        QRect r = canvas_.window()->frameGeometry();
        origin[0] = r.x();
        origin[1] = r.y();
        size[0] = r.width();
        size[1] = r.height();
      }

      virtual void getWindowClient(TVec2D & origin, TVec2D & size) const
      {
        QRect r = canvas_.window()->geometry();
        origin[0] = r.x();
        origin[1] = r.y();
        size[0] = r.width();
        size[1] = r.height();
      }

      virtual void getScreenGeometry(TVec2D & origin, TVec2D & size) const
      {
        GetScreenInfo get_screen_info(canvas_.window());
        QRect r = get_screen_info.available_geometry();
        origin[0] = r.x();
        origin[1] = r.y();
        size[0] = r.width();
        size[1] = r.height();
      }

      virtual void resizeWindowClient(const TVec2D & size)
      {
        canvas_.window()->resize(int(size.x()), int(size.y()));
      }

      virtual double device_pixel_ratio() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.device_pixel_ratio();
      }

      virtual double screen_width() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.screen_width();
      }

      virtual double screen_height() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.screen_height();
      }

      virtual double screen_width_mm() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.screen_width_mm();
      }

      virtual double screen_height_mm() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.screen_height_mm();
      }

      virtual double physical_dpi_x() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.physical_dpi_x();
      }

      virtual double physical_dpi_y() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.physical_dpi_y();
      }

      virtual double logical_dpi_x() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.logical_dpi_x();
      }

      virtual double logical_dpi_y() const
      {
        GetScreenInfo get_screen_info(&canvas_);
        return get_screen_info.logical_dpi_y();
      }

    protected:
      CanvasWidget<TWidget> & canvas_;
      ScreenSaverInhibitor ssi_;
    };


    template <typename TArg1>
    CanvasWidget(TArg1 arg1):
      TWidget(arg1),
      Canvas(yae::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      init();
    }

    template <typename TArg1, typename TArg2>
    CanvasWidget(TArg1 arg1, TArg2 arg2):
      TWidget(arg1, arg2),
      Canvas(yae::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      init();
    }

    template <typename TArg1, typename TArg2, typename TArg3>
    CanvasWidget(TArg1 arg1, TArg2 arg2, TArg3 arg3):
      TWidget(arg1, arg2, arg3),
      Canvas(yae::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      init();
    }

    // virtual:
    bool event(QEvent * event)
    {
      try
      {
        QEvent::Type et = event->type();

        if (et == QEvent::MouseMove)
        {
          sigs_.showCursor();
          sigs_.startHideCursorTimer();
        }
        else if (et == QEvent::Resize)
        {
          TWidget::resizeEvent((QResizeEvent *)event);
          TCanvasWidget::updateCanvasSize();
          event->accept();
          return true;
        }

        if (Canvas::processEvent(event))
        {
          event->accept();
          return true;
        }

        if (et == QEvent::MouseButtonDblClick)
        {
          sigs_.emit_toggle_fullscreen();
          event->accept();
          return true;
        }

        if (et == QEvent::KeyPress ||
            et == QEvent::KeyRelease)
        {
          QKeyEvent * e = static_cast<QKeyEvent *>(event);
          bool key_press = e->type() == QEvent::KeyPress;
          bool auto_repeat = e->isAutoRepeat();
          int key = e->key();

          if (key == Qt::Key_Escape)
          {
            if (key_press)
            {
              escPressed_ = true;
              escRepeated_ = auto_repeat;
            }
            else if (escPressed_ && !auto_repeat)
            {
              if (escRepeated_)
              {
                sigs_.emit_esc_long();
              }
              else
              {
                sigs_.emit_esc_short();
              }

              escPressed_ = false;
              escRepeated_ = false;
            }

            event->accept();
            return true;
          }
        }

        return TWidget::event(event);
      }
      catch (const std::exception & e)
      {
        yae_error
          << "ERROR: CanvasWidget::event(...) caught unexpected exception: "
          << e.what();
      }
      catch (...)
      {
        yae_error
          << "ERROR: CanvasWidget::event(...) caught unknown exception";
      }

      return false;
    }

    // helper:
    void updateCanvasSize()
    {
      GetScreenInfo get_screen_info(this);
      double devicePixelRatio = get_screen_info.device_pixel_ratio();

      Canvas::resize(devicePixelRatio,
                     TWidget::width(),
                     TWidget::height());
    }

  protected:
    // helper:
    void init()
    {
      escPressed_ = false;
      escRepeated_ = false;

      TWidget::setAttribute(Qt::WA_NoSystemBackground);
      TWidget::setAttribute(Qt::WA_OpaquePaintEvent, true);
      TWidget::setAutoFillBackground(false);
      TWidget::setMouseTracking(true);
      Canvas::setDelegate(yae::shared_ptr<TDelegate, Canvas::IDelegate>
                          (new TDelegate(*this)));
    }

    // virtual:
    void initializeGL()
    {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      yae_dlog("initializeGL: QGLContext::currentContext(): %p",
               QGLContext::currentContext());
#endif
    }

    // virtual:
    void resizeGL(int width, int height)
    {
      yae_dlog("resizeGL(%i, %i)", width, height);
    }

    // virtual:
    void paintGL()
    {
      GetScreenInfo get_screen_info(this);
      double devicePixelRatio = get_screen_info.device_pixel_ratio();

      if (devicePixelRatio != Canvas::devicePixelRatio())
      {
        Canvas::resize(devicePixelRatio, TWidget::width(), TWidget::height());
      }

      Canvas::paintCanvas();
    }

  public:
    CanvasWidgetSignalsSlots sigs_;

  protected:
    // long-press (auto-repeated KeyPress prior to KeyRelease)
    // should be interpreted as fullscreen toggle.
    //
    // short-press (no auto-repeated KeyPress prior to KeyRelease)
    // should be interpreted as playlist toggle.
    //
    bool escPressed_;
    bool escRepeated_;
  };

}


#endif // YAE_CANVAS_WIDGET_H_
