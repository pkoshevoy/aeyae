// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Thu Jul 23 21:23:56 PDT 2015
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_WIDGET_H_
#define YAE_CANVAS_WIDGET_H_

// Qt includes:
#include <QCursor>
#include <QOpenGLWidget>
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
  // TCanvasWidget
  //
  template <typename TWidget>
  struct TCanvasWidget : public TWidget, public Canvas
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

      virtual void makeCurrent()
      { widget_.makeCurrent(); }

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
      TDelegate(TCanvasWidget<TWidget> & canvas):
        canvas_(canvas)
      {}

      virtual bool isVisible()
      {
        return canvas_.TWidget::isVisible();
      }

      virtual void requestRepaint()
      {
        canvas_.TWidget::update();
      }

      virtual void inhibitScreenSaver()
      {
        ssi_.screenSaverInhibit();
      }

    protected:
      TCanvasWidget<TWidget> & canvas_;
      ScreenSaverInhibitor ssi_;
    };


    TCanvasWidget():
      Canvas(boost::shared_ptr<IOpenGLContext>(new OpenGLContext(*this))),
      sigs_(*this)
    {
      TWidget::setAttribute(Qt::WA_NoSystemBackground);
      TWidget::setAttribute(Qt::WA_OpaquePaintEvent, true);
      TWidget::setAutoFillBackground(false);
      Canvas::setDelegate(boost::shared_ptr<TDelegate>(new TDelegate(*this)));
    }

  protected:
    // virtual:
    void mouseMoveEvent(QMouseEvent * event)
    {
      TWidget::setCursor(QCursor(Qt::ArrowCursor));
      sigs_.startHideCursorTimer();
    }

    // virtual:
    void mouseDoubleClickEvent(QMouseEvent * event)
    {
      (void)event;
      sigs_.emitToggleFullScreen();
    }

    // virtual:
    void resizeEvent(QResizeEvent * event)
    {
      TWidget::resizeEvent(event);
      Canvas::resize(TWidget::width(), TWidget::height());
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
