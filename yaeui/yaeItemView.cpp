// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 22:52:30 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_benchmark.h"

// standard:
#include <iostream>
#include <limits>
#include <list>

// Qt:
#include <QApplication>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

// yaeui:
#include "yaeCanvasRenderer.h"
#include "yaeInputArea.h"
#include "yaeItemFocus.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeSegment.h"
#include "yaeUtilsQt.h"


//----------------------------------------------------------------
// YAE_DEBUG_ITEM_VIEW_REPAINT
//
#define YAE_DEBUG_ITEM_VIEW_REPAINT 0


namespace yae
{

  //----------------------------------------------------------------
  // device_pixel_pos
  //
  TVec2D
  device_pixel_pos(Canvas * canvas, const QWheelEvent * e)
  {
    QPointF pos = yae::get_wheel_pos(e);
    double devicePixelRatio = canvas->devicePixelRatio();
    double x = devicePixelRatio * pos.x();
    double y = devicePixelRatio * pos.y();
    return TVec2D(x, y);
  }

  //----------------------------------------------------------------
  // PostponeEvent::TPrivate
  //
  struct PostponeEvent::TPrivate
  {
    //----------------------------------------------------------------
    // TPrivate
    //
    TPrivate():
      target_(NULL),
      event_(NULL)
    {}

    //----------------------------------------------------------------
    // ~PostponeEvent
    //
    ~TPrivate()
    {
      delete event_;
    }

    //----------------------------------------------------------------
    // postpone
    //
    void
    postpone(QObject * target, QEvent * event)
    {
      delete event_;
      target_ = target;
      event_ = event;
    }

    //----------------------------------------------------------------
    // onTimeout
    //
    void
    onTimeout()
    {
      qApp->postEvent(target_, event_, Qt::HighEventPriority);
      target_ = NULL;
      event_ = NULL;
    }

    QObject * target_;
    QEvent * event_;
  };

  //----------------------------------------------------------------
  // PostponeEvent::PostponeEvent
  //
  PostponeEvent::PostponeEvent():
    private_(new TPrivate())
  {}

  //----------------------------------------------------------------
  // PostponeEvent::~PostponeEvent
  //
  PostponeEvent::~PostponeEvent()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // PostponeEvent::postpone
  //
  void
  PostponeEvent::postpone(int msec, QObject * target, QEvent * event)
  {
    private_->postpone(target, event);
    QTimer::singleShot(msec, this, SLOT(onTimeout()));
  }

  //----------------------------------------------------------------
  // PostponeEvent::onTimeout
  //
  void
  PostponeEvent::onTimeout()
  {
    private_->onTimeout();
  }

  //----------------------------------------------------------------
  // delegate_toggle_fullscreen
  //
  static void
  delegate_toggle_fullscreen(void * context)
  {
    const ItemView * view = (ItemView *)context;
    if (view->delegate())
    {
      Canvas::IDelegate & canvas_delegate = *view->delegate();
      if (canvas_delegate.isFullScreen())
      {
        canvas_delegate.exitFullScreen();
      }
      else
      {
        canvas_delegate.showFullScreen();
      }
    }
  }


  //----------------------------------------------------------------
  // delegate_query_fullscreen
  //
  static bool
  delegate_query_fullscreen(void * context, bool & fullscreen)
  {
    const ItemView * view = (ItemView *)context;
    if (!view->delegate())
    {
      return false;
    }

    const Canvas::IDelegate & canvas_delegate = *view->delegate();
    fullscreen = canvas_delegate.isFullScreen();
    return true;
  }

  //----------------------------------------------------------------
  // ItemView::ItemView
  //
  ItemView::ItemView(const char * name):
    Canvas::ILayer(),
    devicePixelRatio_(1.0),
    w_(0.0),
    h_(0.0),
    pressed_(NULL),
    dragged_(NULL),
    startPt_(std::numeric_limits<double>::max(),
             std::numeric_limits<double>::max()),
    mousePt_(std::numeric_limits<double>::max(),
             std::numeric_limits<double>::max())
  {
    setRoot(ItemPtr(new Item(name)));
    repaintTimer_.setSingleShot(true);
    animateTimer_.setInterval(16);

    toggle_fullscreen_.reset(&delegate_toggle_fullscreen, this);
    query_fullscreen_.reset(&delegate_query_fullscreen, this);

    bool ok = true;
    ok = connect(&repaintTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);

    ok = connect(&animateTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // ItemView::~ItemView
  //
  ItemView::~ItemView()
  {
    ItemView::clear();

    if (delegate())
    {
      delegate()->windowCanvas().remove(this);
    }
  }

  //----------------------------------------------------------------
  // ItemView::clear
  //
  void
  ItemView::clear()
  {
    TMakeCurrentContext currentContext(this->context().get());
    root_->clear();
  }

  //----------------------------------------------------------------
  // ItemView::setRoot
  //
  void
  ItemView::setRoot(const ItemPtr & item)
  {
    root_ = item;
    root_->self_ = root_;

    Item & root = *root_;
    root.anchors_.left_.set(0.0);
    root.anchors_.top_.set(0.0);
    root.width_.set(new GetViewWidth(*this));
    root.height_.set(new GetViewHeight(*this));
  }

  //----------------------------------------------------------------
  // ItemView::setEnabled
  //
  void
  ItemView::setEnabled(bool enable)
  {
#if YAE_DEBUG_ITEM_VIEW_REPAINT
    yae_debug << "ItemView::setEnabled " << root_->id_
              << " " << enable;
#endif

    if (!enable)
    {
      resetMouseState();

      // remove item focus for this view:
      const ItemFocus::Target * focus = ItemFocus::singleton().focus();

      if (focus && focus->view_ == this)
      {
        ItemPtr itemPtr = focus->item_.lock();
        if (itemPtr)
        {
          TMakeCurrentContext currentContext(*context());
          ItemFocus::singleton().clearFocus(itemPtr.get());
        }
      }

      animateTimer_.stop();
      animators_.clear();
    }
    else
    {
      // make sure next repaint request gets posted:
      requestRepaintEvent_.setDelivered(true);
    }

    Canvas::ILayer::setEnabled(enable);

    Item & root = *root_;
    TMakeCurrentContext currentContext(*context());
    root.notifyObservers(Item::kOnToggleItemView);
  }

  //----------------------------------------------------------------
  // ItemView::event
  //
  bool
  ItemView::event(QEvent * e)
  {
    QEvent::Type et = e->type();
    if (et == QEvent::User)
    {
      RequestRepaintEvent * repaintEvent =
        dynamic_cast<RequestRepaintEvent *>(e);

      if (repaintEvent)
      {
        // YAE_BENCHMARK(benchmark, "ItemView::event repaintEvent");

#if YAE_DEBUG_ITEM_VIEW_REPAINT
        yae_debug << "ItemView::repaintEvent " << root_->id_;
#endif

        Canvas::ILayer::delegate_->requestRepaint();

        repaintEvent->accept();
        return true;
      }

      CancelableEvent * cancelableEvent =
        dynamic_cast<CancelableEvent *>(e);

      if (cancelableEvent)
      {
        cancelableEvent->accept();
        return (!cancelableEvent->ticket_->isCanceled() &&
                cancelableEvent->execute());
      }
    }

    return QObject::event(e);
  }

  //----------------------------------------------------------------
  // ItemView::requestRepaint
  //
  void
  ItemView::requestRepaint()
  {
    bool alreadyRequested = repaintTimer_.isActive();

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    yae_debug << "ItemView::requestRepaint " << root_->id_
              << " " << !alreadyRequested;
#endif

    if (!alreadyRequested)
    {
      repaintTimer_.start(16);
    }
  }

  //----------------------------------------------------------------
  // ItemView::repaint
  //
  void
  ItemView::repaint()
  {
    bool postThePayload = requestRepaintEvent_.setDelivered(false);

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    yae_debug << "ItemView::repaint " << root_->id_
              << " " << postThePayload;
#endif

    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this,
                      new RequestRepaintEvent(requestRepaintEvent_),
                      Qt::HighEventPriority);
    }
  }

  //----------------------------------------------------------------
  // ItemView::resizeTo
  //
  bool
  ItemView::resizeTo(const Canvas * canvas)
  {
    double devicePixelRatio = canvas->devicePixelRatio();
    int w = canvas->canvasWidth();
    int h = canvas->canvasHeight();

    if (devicePixelRatio == devicePixelRatio_ && w == w_ && h_ == h)
    {
      return false;
    }

    devicePixelRatio_ = devicePixelRatio;
    w_ = w;
    h_ = h;

    requestUncache(root_.get());
    return true;
  }

  //----------------------------------------------------------------
  // ItemView::paint
  //
  void
  ItemView::paint(Canvas * canvas)
  {
    YAE_BENCHMARK(benchmark, ("ItemView::paint " + root_->id_).c_str());

    requestRepaintEvent_.setDelivered(true);

    animate();

    // uncache prior to painting:
    while (!uncache_.empty())
    {
      std::map<Item *, yae::weak_ptr<Item> >::iterator i = uncache_.begin();
      ItemPtr itemPtr = i->second.lock();
      uncache_.erase(i);

      if (!itemPtr)
      {
        continue;
      }

      Item & item = *itemPtr;

      // YAE_BENCHMARK(benchmark, (std::string("uncache ") + item.id_).c_str());
      item.uncache();
    }

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    yae_debug << "ItemView::paint " << root_->id_;
#endif

    double x = 0.0;
    double y = 0.0;
    double w = double(canvas->canvasWidth());
    double h = double(canvas->canvasHeight());

    YAE_OPENGL_HERE();
    YAE_OPENGL(glViewport(GLint(x + 0.5), GLint(y + 0.5),
                          GLsizei(w + 0.5), GLsizei(h + 0.5)));

    TGLSaveMatrixState pushMatrix0(GL_MODELVIEW);
    YAE_OPENGL(glLoadIdentity());
    TGLSaveMatrixState pushMatrix1(GL_PROJECTION);
    YAE_OPENGL(glLoadIdentity());
    YAE_OPENGL(glOrtho(0.0, w, h, 0.0, -1.0, 1.0));

    YAE_OPENGL(glDisable(GL_LIGHTING));
    YAE_OPENGL(glEnable(GL_LINE_SMOOTH));
    YAE_OPENGL(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    YAE_OPENGL(glDisable(GL_POLYGON_SMOOTH));
    YAE_OPENGL(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));
    YAE_OPENGL(glLineWidth(1.0));

    YAE_OPENGL(glShadeModel(GL_SMOOTH));
    YAE_OPENGL(glEnable(GL_BLEND));

    if (YAE_OGL_FN(glBlendFuncSeparate))
    {
      // for Wayland compatibility:
      YAE_OPENGL(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                                     GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    }
    else
    {
      YAE_OPENGL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    }

    const Segment & xregion = root_->xExtent();
    const Segment & yregion = root_->yExtent();

    Item & root = *root_;
    root.paint(xregion, yregion, canvas);
  }

  //----------------------------------------------------------------
  // ItemView::addAnimator
  //
  void
  ItemView::addAnimator(const ItemView::TAnimatorPtr & animator)
  {
    animators_.insert(animator);

    if (!animateTimer_.isActive())
    {
      animateTimer_.start();
    }
  }

  //----------------------------------------------------------------
  // ItemView::delAnimator
  //
  void
  ItemView::delAnimator(const ItemView::TAnimatorPtr & animator)
  {
    if (animators_.erase(animator) && animators_.empty())
    {
      animateTimer_.stop();
    }
  }

  //----------------------------------------------------------------
  // ItemView::animate
  //
  void
  ItemView::animate()
  {
    typedef std::set<yae::weak_ptr<IAnimator> >::iterator TIter;
    TIter i = animators_.begin();
    while (i != animators_.end())
    {
      TIter i0 = i;
      ++i;

      TAnimatorPtr animatorPtr = i0->lock();
      if (animatorPtr)
      {
        IAnimator & animator = *animatorPtr;
        animator.animate(*this, animatorPtr);
      }
      else
      {
        // remove expired animators:
        animators_.erase(i0);
      }
    }
  }

  //----------------------------------------------------------------
  // ItemView::requestUncache
  //
  void
  ItemView::requestUncache(Item * root)
  {
    if (!root)
    {
      root = root_.get();
    }

    uncache_[root] = root->self_;
  }

  //----------------------------------------------------------------
  // ItemView::processEvent
  //
  bool
  ItemView::processEvent(Canvas * canvas, QEvent * event)
  {
    // YAE_BENCHMARK(benchmark, "ItemView::processEvent");

    QEvent::Type et = event->type();
    if (et != QEvent::Paint &&
        et != QEvent::Wheel &&
        et != QEvent::MouseButtonPress &&
        et != QEvent::MouseButtonRelease &&
        et != QEvent::MouseButtonDblClick &&
        et != QEvent::MouseMove &&
        et != QEvent::CursorChange &&
        et != QEvent::Resize &&
        et != QEvent::MacGLWindowChange &&
        et != QEvent::Leave &&
        et != QEvent::Enter &&
        et != QEvent::WindowDeactivate &&
        et != QEvent::WindowActivate &&
        et != QEvent::FocusOut &&
        et != QEvent::FocusIn &&
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        et != QEvent::UpdateRequest &&
#endif
        et != QEvent::ShortcutOverride)
    {
#if 0 // ndef NDEBUG
      yae_debug
        << "ItemView::processEvent: "
        << yae::toString(et);
#endif
    }

    if (et == QEvent::Leave)
    {
      mouseOverItems_.clear();
      mousePt_.set_x(std::numeric_limits<double>::max());
      mousePt_.set_y(std::numeric_limits<double>::max());
      if (processMouseTracking(mousePt_))
      {
        requestRepaint();
      }
    }

    if (et == QEvent::MouseButtonPress ||
        et == QEvent::MouseButtonRelease ||
        et == QEvent::MouseButtonDblClick ||
        et == QEvent::MouseMove)
    {
      TMakeCurrentContext currentContext(*context());
      QMouseEvent * e = static_cast<QMouseEvent *>(event);
      bool processed = processMouseEvent(canvas, e);

#if 0 // ndef NDEBUG
      if (et != QEvent::MouseMove)
      {
        yae_warn
          << '\n'
          << yae::get_stacktrace_str() << '\n'
          << root_->id_ << ": "
          << "processed mouse event(" << event << ") == " << processed << ", "
          << e->button() << " button, "
          << e->buttons() << " buttons, "
          << yae::to_str(et);
      }
#endif

      if (processed)
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    if (et == QEvent::Wheel)
    {
      TMakeCurrentContext currentContext(*context());
      QWheelEvent * e = static_cast<QWheelEvent *>(event);
      if (processWheelEvent(canvas, e))
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    if (et == QEvent::KeyPress)
    {
      QKeyEvent & ke = *(static_cast<QKeyEvent *>(event));

      if (ke.key() == Qt::Key_Tab && !ke.modifiers())
      {
        TMakeCurrentContext currentContext(*context());
        if (ItemFocus::singleton().focusNext())
        {
          requestRepaint();
          return true;
        }
      }
      else if (ke.key() == Qt::Key_Backtab ||
               (ke.key() == Qt::Key_Tab &&
                ke.modifiers() == Qt::Key_Shift))
      {
        TMakeCurrentContext currentContext(*context());
        if (ItemFocus::singleton().focusPrevious())
        {
          requestRepaint();
          return true;
        }
      }
    }

    ItemPtr focus = ItemFocus::singleton().focusedItem();
    if (focus && focus->processEvent(*this, canvas, event))
    {
      requestRepaint();
      return true;
    }

    if (et == QEvent::KeyPress ||
        et == QEvent::KeyRelease)
    {
      TMakeCurrentContext currentContext(*context());
      QKeyEvent * e = static_cast<QKeyEvent *>(event);
      if (processKeyEvent(canvas, e))
      {
        requestRepaint();
        return true;
      }

      return false;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ProcessKeyEvent
  //
  struct ProcessKeyEvent : Item::IVisitor
  {
    ProcessKeyEvent(Canvas * canvas, QKeyEvent * event):
      canvas_(canvas),
      event_(event)
    {}

    // virtual:
    bool visit(Item & item)
    {
      return item.processKeyEvent(canvas_, event_);
    }

    Canvas * canvas_;
    QKeyEvent * event_;
  };

  //----------------------------------------------------------------
  // ItemView::processKeyEvent
  //
  bool
  ItemView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
#if 0 // ndef NDEBUG
    yae_elog("ItemView(%s)::processKeyEvent: %s, key 0x%x %s%s %i",
             root_->id_.c_str(),
             e->type() == QEvent::KeyPress ? "KeyPress" :
             e->type() == QEvent::KeyRelease ? "KeyRelease" :
             e->type() == QEvent::ShortcutOverride ? "ShortcutOverride" :
             yae::strfmt("type: %i", int(e->type())).c_str(),
             e->key(),
             e->text().toUtf8().constData(),
             e->isAutoRepeat() ? " auto-repeat" : "",
             e->count());
#endif

    if (isEnabled())
    {
      ProcessKeyEvent visitor(canvas, e);
      if (root_->visit(visitor))
      {
        e->accept();
        return true;
      }
    }

    e->ignore();
    return false;
  }

  //----------------------------------------------------------------
  // has
  //
  static bool
  has(const std::list<InputHandler> & handlers, const Item * item)
  {
    for (std::list<InputHandler>::const_iterator
           i = handlers.begin(), end = handlers.end(); i != end; ++i)
    {
      const InputHandler & handler = *i;
      const InputArea * ia = handler.inputArea();

      if (ia == item)
      {
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // ItemView::processMouseEvent
  //
  bool
  ItemView::processMouseEvent(Canvas * canvas, QMouseEvent * e)
  {
    if (!e)
    {
      return false;
    }

    TVec2D pt = device_pixel_pos(canvas, e);
    mousePt_ = pt;

    root_->getVisibleItems(mousePt_, mouseOverItems_);
    itemsUnderMouse_.clear();
#if 1
    for (std::list<VisibleItem>::const_iterator i = mouseOverItems_.begin();
         i != mouseOverItems_.end(); ++i)
    {
      const VisibleItem & visible_item = *i;
      ItemPtr item = visible_item.item_.lock();
      itemsUnderMouse_[item.get()] = visible_item.csysOrigin_;

      yae::shared_ptr<InputArea, Item> ia = item;
      if (ia)
      {
        ia->onMouseOver(i->csysOrigin_, pt);
      }
    }
#endif
    const QEvent::Type et = e->type();
    const Qt::MouseButton qt_btn = e->button();
    const Qt::MouseButtons qt_btns = e->buttons();

    const InputArea::MouseButton btn =
      (qt_btn == Qt::LeftButton) ? InputArea::kLeftButton :
      (qt_btn == Qt::RightButton) ? InputArea::kRightButton :
      (qt_btn == Qt::MiddleButton) ? InputArea::kMiddleButton :
      !!(qt_btns & Qt::LeftButton) ? InputArea::kLeftButton :
      !!(qt_btns & Qt::RightButton) ? InputArea::kRightButton :
      !!(qt_btns & Qt::MiddleButton) ? InputArea::kMiddleButton :
      InputArea::kNoButtons;

    if (!((et == QEvent::MouseMove && (e->buttons() & Qt::LeftButton)) ||
          (et == QEvent::MouseButtonPress ||
           et == QEvent::MouseButtonRelease ||
           et == QEvent::MouseButtonDblClick )))
    {
      if (!e->buttons())
      {
        if (processMouseTracking(pt))
        {
          requestRepaint();
        }
      }

      return false;
    }

    if (et == QEvent::MouseButtonPress)
    {
      pressed_ = NULL;
      dragged_ = NULL;
      startPt_ = pt;

      bool foundHandlers = root_->getInputHandlers(pt, inputHandlers_);

      // check for focus loss/transfer:
      ItemPtr focus = ItemFocus::singleton().focusedItem();
      if (focus && !has(inputHandlers_, focus.get()))
      {
        ItemFocus::singleton().clearFocus(focus.get());
        requestRepaint();
      }

      if (!foundHandlers)
      {
        return false;
      }

      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (ia && ia->onButtonPress(handler.csysOrigin_, pt, btn))
        {
          pressed_ = &handler;

          // consume left-button clicks,
          // allow other button clicks to bubble up:
          return btn == InputArea::kLeftButton;
        }
      }

      return false;
    }
    else if (et == QEvent::MouseMove)
    {
      if (inputHandlers_.empty())
      {
        std::list<InputHandler> handlers;
        if (!root_->getInputHandlers(pt, handlers))
        {
          return false;
        }

        for (TInputHandlerRIter i = handlers.rbegin();
             i != handlers.rend(); ++i)
        {
          InputHandler & handler = *i;
          InputArea * ia = handler.inputArea();

          if (ia && ia->onMouseOver(handler.csysOrigin_, pt))
          {
            return true;
          }
        }

        return false;
      }

      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (!dragged_ && pressed_ && pressed_ != &handler &&
            ia && ia->onButtonPress(handler.csysOrigin_, startPt_, btn))
        {
          // avoid changing clicked item due to an accidental drag,
          // check the threshold:
          double dist = (pt - startPt_).norm();
          double dpi = delegate()->logical_dpi_y();
          if (dist < dpi * 0.2)
          {
            continue;
          }

          // previous handler didn't handle the drag event, try another:
          InputArea * pi = pressed_->inputArea();
          if (pi)
          {
            pi->onCancel();
          }

          pressed_ = &handler;
        }

        if (ia && pressed_ && pressed_ == &handler &&
            ia->onDrag(handler.csysOrigin_, startPt_, pt))
        {
          dragged_ = &handler;
          return true;
        }
      }

      return false;
    }
    else if (et == QEvent::MouseButtonRelease)
    {
      bool accept = false;
      if (pressed_)
      {
        InputArea * ia =
          dragged_ ? dragged_->inputArea() : pressed_->inputArea();

        ItemPtr keep_alive;
        if (ia)
        {
          keep_alive = ia->self_.lock();
        }

        if (ia && ia->isButtonPressed(btn))
        {
          if (dragged_ && ia->draggable())
          {
            accept = ia->onDragEnd(dragged_->csysOrigin_, startPt_, pt);
          }
          else
          {
            accept = ia->onClick(pressed_->csysOrigin_, pt);
          }
        }

        if (keep_alive)
        {
          // to clear InputArea::pressed_button_:
          ia->onCancel();
        }
      }

      if (accept && btn != InputArea::kLeftButton)
      {
        // consume left-button clicks,
        // allow other button clicks to bubble up:
        accept = false;
      }

      pressed_ = NULL;
      dragged_ = NULL;
      inputHandlers_.clear();

      return accept;
    }
    else if (et == QEvent::MouseButtonDblClick &&
             btn == InputArea::kLeftButton)
    {
      pressed_ = NULL;
      dragged_ = NULL;
      inputHandlers_.clear();

      std::list<InputHandler> handlers;
      if (!root_->getInputHandlers(pt, handlers))
      {
        return false;
      }

      for (TInputHandlerRIter i = handlers.rbegin();
           i != handlers.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (ia &&
            ia->accepts(btn) &&
            ia->onDoubleClick(handler.csysOrigin_, pt))
        {
          return true;
        }
      }

      return false;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ProcessWheelEvent
  //
  struct ProcessWheelEvent : Item::IVisitor
  {
    ProcessWheelEvent(Canvas * canvas, QWheelEvent * event):
      canvas_(canvas),
      event_(event)
    {}

    // virtual:
    bool visit(Item & item)
    {
      return item.processWheelEvent(canvas_, event_);
    }

    Canvas * canvas_;
    QWheelEvent * event_;
  };

  //----------------------------------------------------------------
  // ItemView::processWheelEvent
  //
  bool
  ItemView::processWheelEvent(Canvas * canvas, QWheelEvent * e)
  {
    if (!e)
    {
      return false;
    }

    if (!isEnabled())
    {
      return false;
    }

    TVec2D pt = device_pixel_pos(canvas, e);
    std::list<InputHandler> handlers;
    if (root_->getInputHandlers(pt, handlers))
    {
      double degrees = yae::get_wheel_delta_degrees(e);

      bool processed = false;
      for (TInputHandlerRIter i = handlers.rbegin(); i != handlers.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (ia && ia->onScroll(handler.csysOrigin_, pt, degrees))
        {
          processed = true;
          break;
        }
      }

      if (processed)
      {
        return true;
      }
    }

    ProcessWheelEvent visitor(canvas, e);
    return root_->visit(visitor);
  }

  //----------------------------------------------------------------
  // ItemView::processMouseTracking
  //
  bool
  ItemView::processMouseTracking(const TVec2D & mousePt)
  {
    (void)mousePt;
    return isEnabled();
  }

  //----------------------------------------------------------------
  // ItemView::isMouseOverItem
  //
  bool
  ItemView::isMouseOverItem(const Item & item) const
  {
#if 0
    std::list<VisibleItem>::const_iterator found =
      yae::find(mouseOverItems_, item);
    return (found != mouseOverItems_.end());
#else
    return yae::has(itemsUnderMouse_, &item);
#endif
  }

  //----------------------------------------------------------------
  // ItemView::isMousePressed
  //
  bool
  ItemView::isMousePressed(const InputArea & item) const
  {
    if (!pressed_)
    {
      return false;
    }

    yae::shared_ptr<InputArea, Item> pressed = pressed_->input_.lock();
    if (pressed && pressed.get() == &item)
    {
      return true;
    }

    return false;
  }

  //----------------------------------------------------------------
  // ItemView::resetMouseState
  //
  void
  ItemView::resetMouseState()
  {
    mouseOverItems_.clear();
    mousePt_.set_x(std::numeric_limits<double>::max());
    mousePt_.set_y(std::numeric_limits<double>::max());

    inputHandlers_.clear();
    startPt_ = mousePt_;
    pressed_ = NULL;
    dragged_ = NULL;
  }

  //----------------------------------------------------------------
  // ItemView::addImageProvider
  //
  void
  ItemView::addImageProvider(const QString & providerId,
                             const TImageProviderPtr & p)
  {
    imageProviders_[providerId] = p;
  }


  //----------------------------------------------------------------
  // get_row_height
  //
  double
  get_row_height(const ItemView & view)
  {
    double dpi = view.style()->dpi_.get();
    double rh = dpi / 2.5;
    double fh = QFontMetricsF(view.style()->font_).height();
    fh = std::max(fh, 13.0);
    rh = std::max(rh, fh * 2.0);
    return rh;
  }

  //----------------------------------------------------------------
  // calc_items_per_row
  //
  unsigned int
  calc_items_per_row(double row_width, double cell_width)
  {
    double n = std::max(1.0, std::floor(row_width / cell_width));
    return (unsigned int)n;
  }


  //----------------------------------------------------------------
  // ColorOnHover::ColorOnHover
  //
  ColorOnHover::ColorOnHover(const ItemView & view,
                             const Item & item,
                             const ColorRef & c0,
                             const ColorRef & c1):
    view_(view),
    item_(item),
    c0_(c0),
    c1_(c1)
  {
    YAE_ASSERT(!(c0_.isCacheable() || c1_.isCacheable()));
  }

  //----------------------------------------------------------------
  // ColorOnHover::evaluate
  //
  void
  ColorOnHover::evaluate(Color & result) const
  {
    const TVec2D & pt = view_.mousePt();
    const bool overlaps = item_.overlaps(pt);
    result = overlaps ? c1_.get() : c0_.get();
  }

}
