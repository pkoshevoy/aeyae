// File         : the_qt_input_device_event.cxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2007/01/05 14:59:00
// Copyright    : (C) 2007
// License      : MIT
// Description  : Qt wrappers for input device events

// local includes:
#include "Qt3/the_qt_input_device_event.hxx"
#include "opengl/the_view.hxx"

// Qt includes:
#include <qwidget.h>
#include <qevent.h>


//----------------------------------------------------------------
// the_mouse_event
// 
the_mouse_event_t
the_mouse_event(QWidget * widget, const QMouseEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  int tran = e->type();
  the_mouse_event_t me(dynamic_cast<the_view_t *>(widget),
		       e->button() & Qt::MouseButtonMask,
		       tran,
		       e->state() & Qt::KeyButtonMask,
		       tran == QEvent::MouseButtonDblClick,
		       tran == QEvent::MouseMove,
		       scs_pt);
  return me;
}

//----------------------------------------------------------------
// the_wheel_event
// 
the_wheel_event_t
the_wheel_event(QWidget * widget, const QWheelEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  the_wheel_event_t we(dynamic_cast<the_view_t *>(widget),
		       0, // btns
		       0, // tran
		       e->state() & Qt::KeyButtonMask,
		       scs_pt,
		       double(e->delta()) / 8.0,
		       e->orientation() == Qt::Vertical);
#if 0
  cerr << "FIXME: wheel: " << we.degrees_rotated_ << endl;
#endif
  
  return we;
}

//----------------------------------------------------------------
// the_keybd_event
// 
the_keybd_event_t
the_keybd_event(QWidget * widget, const QKeyEvent * e)
{
#if 0
  cerr << e->key();
  if (e->key() < 256)
  {
    cerr << "\t\'" << (unsigned char)(e->key()) << '\'' << endl;
  }
  else
  {
    cerr << endl;
  }
#endif
  
  the_keybd_event_t ke(dynamic_cast<the_view_t *>(widget),
		       e->key(),
		       e->type(),
		       e->state() & Qt::KeyButtonMask,
		       e->isAutoRepeat());
  return ke;
}

//----------------------------------------------------------------
// the_wacom_event
// 
the_wacom_event_t
the_wacom_event(QWidget * widget, const QTabletEvent * e)
{
  p2x1_t scs_pt(float(e->x()) / float(widget->width()),
		float(e->y()) / float(widget->height()));
  
  p2x1_t tilt(2.0 * (float(e->xTilt() + 60) / 120.0 - 0.5),
	      2.0 * (float(e->yTilt() + 60) / 120.0 - 0.5));
  
  the_tablet_tool_t tool = THE_TABLET_UNKNOWN_E;
  switch (e->device())
  {
    case QTabletEvent::Stylus:
      tool = THE_TABLET_PEN_E;
      break;
      
    case QTabletEvent::Eraser:
      tool = THE_TABLET_ERASER_E;
      break;
      
    case QTabletEvent::Puck:
      tool = THE_TABLET_CURSOR_E;
      break;
      
    default:
      break;
  }
  
  the_wacom_event_t te(dynamic_cast<the_view_t *>(widget),
		       tool,
		       (const_cast<QTabletEvent *>(e)->uniqueId()).second,
		       scs_pt,
		       tilt,
		       float(e->pressure()) / 255.f,
		       0.f, // tangential pressure
		       0.f, // rotation
		       0.f);// z
  return te;
}
