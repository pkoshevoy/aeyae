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

#ifndef YAE_USE_QOPENGL_WIDGET
// GLEW includes:
#include <GL/glew.h>
#endif

// Qt includes:
#include <QApplication>
#include <QDesktopWidget>
// #include <QDragEnterEvent>
// #include <QDropEvent>
#include <QKeyEvent>
#ifdef YAE_USE_QOPENGL_WIDGET
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif
#include <QTimer>

// aeyae:
#include "yae/api/yae_shared_ptr.h"

// yaeui:
#include "yaeCanvas.h"
#include "yaeScreenSaverInhibitor.h"
#include "yaeUtilsQt.h"


namespace yae
{

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
        YAE_OGL_11_HERE();
        return true;
      }

      virtual void doneCurrent()
      { widget_.doneCurrent(); }

      virtual const void * getCurrent() const
      {
#ifdef YAE_USE_QOPENGL_WIDGET
        QOpenGLContext * ctx = QOpenGLContext::currentContext();
#else
        const QGLContext * ctx = QGLContext::currentContext();
#endif
        return ctx;
      }

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
        // this is just to prevent concurrent OpenGL access
        // to the same context:
        TMakeCurrentContext lock(canvas_.Canvas::context());
        YAE_ASSERT(isVisible());

#ifdef YAE_USE_QOPENGL_WIDGET
        canvas_.TWidget::update();
#else
        canvas_.paintGL();
        canvas_.TWidget::swapBuffers();
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
        QRect r = QApplication::desktop()->availableGeometry(canvas_.window());
        origin[0] = r.x();
        origin[1] = r.y();
        size[0] = r.width();
        size[1] = r.height();
      }

      virtual void resizeWindowClient(const TVec2D & size)
      {
        canvas_.window()->resize(int(size.x()), int(size.y()));
      }

      static QWidget * get_screen_widget(const QWidget * widget)
      {
        QDesktopWidget * dw = QApplication::desktop();
        int sn = dw->screenNumber(widget);
        QWidget * sw = dw->screen(sn);
        return sw;
      }

      inline QWidget * screen_widget() const
      {
        return get_screen_widget(&canvas_);
      }

      static double get_device_pixel_ratio(const QWidget * sw)
      {
#if QT_VERSION < 0x050000
        (void)sw;
        double s = 1.0;
#elif QT_VERSION < 0x050600
        double s = sw->devicePixelRatio();
#else
        double s = sw->devicePixelRatioF();
#endif
        return s;
      }

      virtual double device_pixel_ratio() const
      {
        QWidget * sw = this->screen_widget();
        return get_device_pixel_ratio(sw);
      }

      virtual double screen_width() const
      {
        QWidget * sw = this->screen_widget();
        double s = this->device_pixel_ratio();
        double w = sw->width() * s;
        return w;
      }

      virtual double screen_height() const
      {
        QWidget * sw = this->screen_widget();
        double s = this->device_pixel_ratio();
        double h = sw->height() * s;
        return h;
      }

      virtual double screen_width_mm() const
      {
        QWidget * sw = this->screen_widget();
        double w_mm = sw->widthMM();
        return w_mm;
      }

      virtual double screen_height_mm() const
      {
        QWidget * sw = this->screen_widget();
        double h_mm = sw->heightMM();
        return h_mm;
      }

      virtual double physical_dpi_x() const
      {
        QWidget * sw = this->screen_widget();
        double s = this->device_pixel_ratio();
        double w = sw->width() * s;
        double w_mm = sw->widthMM();
        double dpi = (w * 25.4) / w_mm;
        return dpi;
      }

      virtual double physical_dpi_y() const
      {
        QWidget * sw = this->screen_widget();
#if 0
        double s = this->device_pixel_ratio();
        double h = sw->height() * s;
        double h_mm = sw->heightMM();
        double dpi = (h * 25.4) / h_mm;
#else
        int dpi = sw->physicalDpiX();
#endif
        return dpi;
      }

      virtual double logical_dpi_x() const
      {
        QWidget * sw = this->screen_widget();
        double s = this->device_pixel_ratio();
        double dpi = sw->logicalDpiX();
        return dpi * s;
      }

      virtual double logical_dpi_y() const
      {
        QWidget * sw = this->screen_widget();
        double s = this->device_pixel_ratio();
        double dpi = sw->logicalDpiY();
        return dpi * s;
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
      QWidget * sw = TDelegate::get_screen_widget(this);
      double devicePixelRatio = TDelegate::get_device_pixel_ratio(sw);

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
      yae_dlog("initializeGL: QGLContext::currentContext(): %p",
               QGLContext::currentContext());
    }

    // virtual:
    void resizeGL(int width, int height)
    {
      yae_dlog("resizeGL(%i, %i)", width, height);
    }

    // virtual:
    void paintGL()
    {
      QWidget * sw = TDelegate::get_screen_widget(this);
      double devicePixelRatio = TDelegate::get_device_pixel_ratio(sw);

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
