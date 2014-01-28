// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : the_qt_view.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : Sun Jun 23 21:53:36 MDT 2002
// Copyright    : (C) 2002
// License      : MIT
// Description  : A Qt4 based port of the OpenGL view widget.

// system includes:
#include <assert.h>

// local includes:
#include "Qt4/the_qt_view.hxx"
#include "Qt4/the_qt_input_device_event.hxx"
#include "eh/the_input_device_eh.hxx"

// Qt includes:
#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QKeyEvent>


//----------------------------------------------------------------
// the_qt_view_t::the_qt_view_t
//
the_qt_view_t::the_qt_view_t(QWidget * parent,
			     const char * name,
			     QGLWidget * shared,
			     const the_view_mgr_orientation_t & orientation):
  QGLWidget(parent, shared, 0),
  the_view_t(name, orientation)
{
  setObjectName(name);
  setAttribute(Qt::WA_NoSystemBackground);
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

//----------------------------------------------------------------
// the_qt_view_t::initializeGL
//
// QT/OpenGL stuff:
void
the_qt_view_t::initializeGL()
{
  QGLWidget::initializeGL();
  gl_setup();
}

//----------------------------------------------------------------
// the_qt_view_t::resizeGL
//
void
the_qt_view_t::resizeGL(int w, int h)
{
  gl_resize(w, h);
  QGLWidget::resizeGL(w, h);
}

//----------------------------------------------------------------
// the_qt_view_t::paintGL
//
void
the_qt_view_t::paintGL()
{
  gl_paint();
}

//----------------------------------------------------------------
// the_qt_view_t::change_cursor
//
void
the_qt_view_t::change_cursor(const the_cursor_id_t & cursor_id)
{
  if (view_mgr().get_stereoscopic() == the_view_mgr_t::NOT_STEREOSCOPIC_E)
  {
    the_cursor_t c(cursor_id);
    setCursor(QCursor(QBitmap::fromData(QSize(c.w_, c.h_),
                                        c.icon_,
                                        QImage::Format_Mono),
                      QBitmap::fromData(QSize(c.w_, c.h_),
                                        c.mask_,
                                        QImage::Format_Mono),
                      c.x_,
                      c.y_));
    view_mgr().set_cursor_id(THE_BLANK_CURSOR_E);
  }
  else
  {
    setCursor(QCursor(Qt::BlankCursor));
    view_mgr().set_cursor_id(cursor_id);
  }
}

//----------------------------------------------------------------
// the_qt_view_t::showEvent
//
void
the_qt_view_t::showEvent(QShowEvent * e)
{
  eh_stack().view_cb(this);
}

//----------------------------------------------------------------
// the_qt_view_t::mousePressEvent
//
void
the_qt_view_t::mousePressEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseReleaseEvent
//
void
the_qt_view_t::mouseReleaseEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseDoubleClickEvent
//
void
the_qt_view_t::mouseDoubleClickEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::mouseMoveEvent
//
void
the_qt_view_t::mouseMoveEvent(QMouseEvent * e)
{
  if (!eh_stack_->mouse_cb(the_mouse_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::wheelEvent
//
void
the_qt_view_t::wheelEvent(QWheelEvent * e)
{
  if (!eh_stack_->wheel_cb(the_wheel_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::keyPressEvent
//
void
the_qt_view_t::keyPressEvent(QKeyEvent * e)
{
  if (!eh_stack_->keybd_cb(the_keybd_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::keyReleaseEvent
//
void
the_qt_view_t::keyReleaseEvent(QKeyEvent * e)
{
  if (!eh_stack_->keybd_cb(the_keybd_event(this, e)))
  {
    e->ignore();
  }
}

//----------------------------------------------------------------
// the_qt_view_t::tabletEvent
//
void
the_qt_view_t::tabletEvent(QTabletEvent * e)
{
  if (eh_stack_->wacom_cb(the_wacom_event(this, e)))
  {
    e->accept();
  }
  else
  {
    e->ignore();
  }
}
