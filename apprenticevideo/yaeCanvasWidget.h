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
#include <QCursor>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#ifdef YAE_USE_QOPENGL_WIDGET
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif
#include <QTimer>

// local includes:
#include "yaeCanvas.h"
#include "yaeScreenSaverInhibitor.h"


namespace yae
{

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots
  //
  class CanvasWidgetSignalsSlots : public QObject
  {
    Q_OBJECT;

  public:
    CanvasWidgetSignalsSlots(QWidget & widget):
      widget_(widget)
    {
      timerHideCursor_.setSingleShot(true);
      timerHideCursor_.setInterval(3000);

      bool ok = true;
      ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                   this, SLOT(hideCursor()));
      YAE_ASSERT(ok);
    }

  signals:
    void toggleFullScreen();

  public slots:
    void hideCursor()
    {
      widget_.setCursor(QCursor(Qt::BlankCursor));
    }

  public:
    void emitToggleFullScreen()
    {
      emit toggleFullScreen();
    }

    void startHideCursorTimer()
    {
      timerHideCursor_.start();
    }

    QWidget & widget_;

    // a single shot timer for hiding the cursor:
    QTimer timerHideCursor_;
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
    // OpenGLContext
    //
    struct OpenGLContext : public IOpenGLContext
    {
      OpenGLContext(TOpenGLWidget & widget):
        widget_(widget)
      {}

      virtual bool makeCurrent()
      { widget_.makeCurrent(); return true; }

      virtual void doneCurrent()
      { widget_.doneCurrent(); }

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

      virtual bool isVisible()
      {
        return canvas_.TWidget::isVisible();
      }

      virtual void repaint()
      {
        // this is just to prevent concurrent OpenGL access to the same context:
        TMakeCurrentContext lock(canvas_.Canvas::context());

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

      virtual double logicalDpiX() const
      {
        QWidget * sw = screenWidget();
        double dpi = sw->logicalDpiX();
        return dpi;
      }

      virtual double logicalDpiY() const
      {
        QWidget * sw = screenWidget();
        double dpi = sw->logicalDpiY();
        return dpi;
      }

      inline QWidget * screenWidget() const
      {
        QDesktopWidget * dw = QApplication::desktop();
        int sn = dw->screenNumber(&canvas_);
        QWidget * sw = dw->screen(sn);
        return sw;
      }

    protected:
      CanvasWidget<TWidget> & canvas_;
      ScreenSaverInhibitor ssi_;
    };


    template <typename TArg1>
    CanvasWidget(TArg1 arg1):
      TWidget(arg1),
      Canvas(boost::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      init();
    }

    template <typename TArg1, typename TArg2, typename TArg3>
    CanvasWidget(TArg1 arg1, TArg2 arg2, TArg3 arg3):
      TWidget(arg1, arg2, arg3),
      Canvas(boost::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      init();
    }

  protected:
    // helper:
    void init()
    {
      TWidget::setAttribute(Qt::WA_NoSystemBackground);
      TWidget::setAttribute(Qt::WA_OpaquePaintEvent, true);
      TWidget::setAutoFillBackground(false);
      TWidget::setMouseTracking(true);
      Canvas::setDelegate(boost::shared_ptr<TDelegate>(new TDelegate(*this)));
    }

    // virtual:
    bool event(QEvent * event)
    {
      try
      {
        QEvent::Type et = event->type();

        if (et == QEvent::MouseMove)
        {
          TWidget::setCursor(QCursor(Qt::ArrowCursor));
          sigs_.startHideCursorTimer();
        }
        else if (et == QEvent::Resize)
        {
          TWidget::resizeEvent((QResizeEvent *)event);

#if QT_VERSION < 0x050000
          double devicePixelRatio = 1.0;
#else
          double devicePixelRatio = TWidget::devicePixelRatio();
#endif

          Canvas::resize(devicePixelRatio,
                         TWidget::width(),
                         TWidget::height());
        }

        if (Canvas::processEvent(event))
        {
          return true;
        }

        if (et == QEvent::MouseButtonDblClick)
        {
          sigs_.emitToggleFullScreen();
          event->accept();
          return true;
        }

        return TWidget::event(event);
      }
      catch (const std::exception & e)
      {
        std::cerr
          << "ERROR: CanvasWidget::event(...) caught unexpected exception: "
          << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr
          << "ERROR: CanvasWidget::event(...) caught unknown exception"
          << std::endl;
      }

      return false;
    }

    // virtual:
    void paintGL()
    {
#if QT_VERSION < 0x050000
      double devicePixelRatio = 1.0;
#else
      double devicePixelRatio = TWidget::devicePixelRatio();
#endif
      if (devicePixelRatio != Canvas::devicePixelRatio())
      {
        Canvas::resize(devicePixelRatio, TWidget::width(), TWidget::height());
      }

      Canvas::paintCanvas();
    }

  public:
    CanvasWidgetSignalsSlots sigs_;
  };

}


#endif // YAE_CANVAS_WIDGET_H_
