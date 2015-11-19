// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Jul 23 21:23:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_WIDGET_H_
#define YAE_CANVAS_WIDGET_H_

#if defined(YAE_USE_QT4)
// GLEW includes:
#include <GL/glew.h>
#endif

// Qt includes:
#include <QApplication>
#include <QCursor>
#include <QDragEnterEvent>
#include <QDropEvent>
#if defined(YAE_USE_QT4)
#include <QGLWidget>
#elif defined(YAE_USE_QT5)
#include <QOpenGLWidget>
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
        canvas_.paintCanvas();
        canvas_.TWidget::swapBuffers();
     }

      virtual void requestRepaint()
      {
        qApp->postEvent(&canvas_, new Canvas::RepaintEvent());
      }

      virtual void inhibitScreenSaver()
      {
        ssi_.screenSaverInhibit();
      }

    protected:
      CanvasWidget<TWidget> & canvas_;
      ScreenSaverInhibitor ssi_;
    };


    template <typename TWidgetInitParam>
    CanvasWidget(const TWidgetInitParam & initParam):
      TWidget(initParam),
      Canvas(boost::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      TWidget::setAttribute(Qt::WA_NoSystemBackground);
      TWidget::setAttribute(Qt::WA_OpaquePaintEvent, true);
      TWidget::setAutoFillBackground(false);
      TWidget::setMouseTracking(true);
      Canvas::setDelegate(boost::shared_ptr<TDelegate>(new TDelegate(*this)));
    }

  protected:
    // virtual:
    bool event(QEvent * event)
    {
      QEvent::Type et = event->type();

      if (et == QEvent::MouseMove)
      {
        TWidget::setCursor(QCursor(Qt::ArrowCursor));
        sigs_.startHideCursorTimer();
      }
      else if (et == QEvent::MouseButtonDblClick)
      {
        sigs_.emitToggleFullScreen();
      }
      else if (et == QEvent::Resize)
      {
        TWidget::resizeEvent((QResizeEvent *)event);
        Canvas::resize(TWidget::width(), TWidget::height());
      }

      if (Canvas::processEvent(event))
      {
        return true;
      }

      return TWidget::event(event);
    }

    // virtual:
    void paintGL()
    {
      Canvas::paintCanvas();
      // Canvas::refresh();
    }

  public:
    CanvasWidgetSignalsSlots sigs_;
  };

}


#endif // YAE_CANVAS_WIDGET_H_
