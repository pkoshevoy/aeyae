// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Fri Nov 26 11:28:37 MST 2021
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QCursor>
#include <QKeyEvent>

// yaeui:
#include "yaeAppleRemoteControl.h"
#include "yaeCanvas.h"
#include "yaeCanvasWidget.h"


namespace yae
{

#ifdef __APPLE__

  //----------------------------------------------------------------
  // new_key_event
  //
  static QKeyEvent *
  new_key_event(int key,
                bool pressed_down,
                bool auto_repeat,
                unsigned int click_count)
  {
    QEvent::Type event_type =
      pressed_down ? QEvent::KeyPress : QEvent::KeyRelease;

    return new QKeyEvent(event_type,
                         key,
                         Qt::NoModifier,
                         QString(),
                         auto_repeat,
                         click_count);
  }

  //----------------------------------------------------------------
  // send_key_event
  //
  static void
  send_key_event(QObject * target,
                 int key,
                 bool pressed,
                 bool held,
                 unsigned int clicks)
  {
    if (pressed && held)
    {
      qApp->postEvent(target,
                      new_key_event(key, true, false, 0),
                      Qt::HighEventPriority);
    }

    qApp->postEvent(target,
                    new_key_event(key, pressed, held, clicks),
                    Qt::HighEventPriority);

    if (pressed && held)
    {
      qApp->postEvent(target,
                      new_key_event(key, false, false, clicks),
                      Qt::HighEventPriority);
    }
  }

  //----------------------------------------------------------------
  // apple_remote_control_observer
  //
  static void
  apple_remote_control_observer(void * ctx,
                                TRemoteControlButtonId button_id,
                                bool pressed,
                                unsigned int clicks,
                                bool held)
  {
    CanvasWidgetSignalsSlots * sigs = (CanvasWidgetSignalsSlots *)ctx;
    QWidget & canvas = sigs->widget();
    QWidget * target = canvas.window()->focusWidget();

#ifndef NDEBUG
    static const char * button_name[] = {
      "undefined",
      "volume up",
      "volume down",
      "menu",
      "play",
      "left",
      "right"
    };

    yae_debug
      << ctx << " apple remote control event"
      << ", button id: " << button_id << "("
      << (((std::size_t)button_id) < (sizeof(button_name) /
                                      sizeof(button_name[0])) ?
          button_name[button_id] : "unknown") << ")"
      << ", pressed: " << pressed
      << ", clicks: " << clicks
      << ", held: " << held;
#endif

    if (button_id == kRemoteControlVolumeUp)
    {
      send_key_event(target, Qt::Key_Up, pressed, held, clicks);
    }
    else if (button_id == kRemoteControlVolumeDown)
    {
      send_key_event(target, Qt::Key_Down, pressed, held, clicks);
    }
    else if (button_id == kRemoteControlLeftButton)
    {
      send_key_event(target, Qt::Key_Left, pressed, held, clicks);
    }
    else if (button_id == kRemoteControlRightButton)
    {
      send_key_event(target, Qt::Key_Right, pressed, held, clicks);
    }
    else if (button_id == kRemoteControlPlayButton)
    {
      send_key_event(target, Qt::Key_Return, pressed, held, clicks);
    }
    else if (button_id == kRemoteControlMenuButton)
    {
      send_key_event(target, Qt::Key_Escape, pressed, held, clicks);
    }
  }
#endif

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::CanvasWidgetSignalsSlots
  //
  CanvasWidgetSignalsSlots::CanvasWidgetSignalsSlots(QWidget & widget):
    QObject(),
    widget_(widget)
  {
#ifdef __APPLE__
    appleRemoteControl_ = NULL;
#endif

    timerHideCursor_.setSingleShot(true);
    timerHideCursor_.setInterval(3000);

    bool ok = true;
    ok = connect(&timerHideCursor_, SIGNAL(timeout()),
                 this, SIGNAL(maybeHideCursor()));
    YAE_ASSERT(ok);

    ok = connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                 this, SLOT(focusChanged(QWidget *, QWidget *)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::~CanvasWidgetSignalsSlots
  //
  CanvasWidgetSignalsSlots::~CanvasWidgetSignalsSlots()
  {
#ifdef __APPLE__
    if (appleRemoteControl_)
    {
      appleRemoteControlClose(appleRemoteControl_);
      appleRemoteControl_ = NULL;
    }
#endif
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::emit_toggle_fullscreen
  //
  void
  CanvasWidgetSignalsSlots::emit_toggle_fullscreen()
  {
    emit toggleFullScreen();
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::emit_esc_short
  //
  void
  CanvasWidgetSignalsSlots::emit_esc_short()
  {
    // yae_error << "emit_esc_short";
    emit escShort();
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::emit_esc_long
  //
  void
  CanvasWidgetSignalsSlots::emit_esc_long()
  {
    // yae_error << "emit_esc_long";
    emit escLong();
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::stopHideCursorTimer
  //
  void
  CanvasWidgetSignalsSlots::stopHideCursorTimer()
  {
    timerHideCursor_.stop();
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::startHideCursorTimer
  //
  void
  CanvasWidgetSignalsSlots::startHideCursorTimer()
  {
    timerHideCursor_.start();
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::hideCursor
  //
  void
  CanvasWidgetSignalsSlots::hideCursor()
  {
    widget_.setCursor(QCursor(Qt::BlankCursor));
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::showCursor
  //
  void
  CanvasWidgetSignalsSlots::showCursor()
  {
    widget_.setCursor(QCursor(Qt::ArrowCursor));
  }

  //----------------------------------------------------------------
  // CanvasWidgetSignalsSlots::focusChanged
  //
  void
  CanvasWidgetSignalsSlots::focusChanged(QWidget * prev, QWidget * curr)
  {
#ifndef NDEBUG
    std::cerr << "focus changed: " << prev << " -> " << curr;
    if (curr)
    {
      std::cerr << ", " << curr->objectName().toUtf8().constData()
                << " (" << curr->metaObject()->className() << ")";
    }
    std::cerr << std::endl;
#endif

#ifdef __APPLE__
    if (curr && (curr->window() == widget_.window()))
    {
      if (!appleRemoteControl_)
      {
        appleRemoteControl_ =
          appleRemoteControlOpen(true, // exclusive
                                 false, // count clicks
                                 false, // simulate hold
                                 &apple_remote_control_observer,
                                 this);
      }
    }
    else if (!curr || (curr->window() != widget_.window()))
    {
      if (appleRemoteControl_)
      {
        appleRemoteControlClose(appleRemoteControl_);
        appleRemoteControl_ = NULL;
      }
    }
#endif
  }

}
