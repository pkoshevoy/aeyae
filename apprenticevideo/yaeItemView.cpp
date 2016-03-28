// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Dec 18 22:52:30 PST 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <iostream>
#include <limits>
#include <list>

// Qt library:
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaeInputArea.h"
#include "yaeItemFocus.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeSegment.h"
#include "yaeUtilsQt.h"


//----------------------------------------------------------------
// YAE_DEBUG_ITEM_VIEW_REPAINT
//
#define YAE_DEBUG_ITEM_VIEW_REPAINT 0


namespace yae
{

  //----------------------------------------------------------------
  // ItemView::ItemView
  //
  ItemView::ItemView(const char * name):
    Canvas::ILayer(),
    root_(new Item(name)),
    w_(0.0),
    h_(0.0),
    pressed_(NULL),
    dragged_(NULL)
  {
    root_->self_ = root_;
    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    repaintTimer_.setSingleShot(true);
    animateTimer_.setInterval(16);

    bool ok = true;
    ok = connect(&repaintTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);

    ok = connect(&animateTimer_, SIGNAL(timeout()), this, SLOT(repaint()));
    YAE_ASSERT(ok);
   }

  //----------------------------------------------------------------
  // ItemView::setEnabled
  //
  void
  ItemView::setEnabled(bool enable)
  {
#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::setEnabled " << root_->id_
              << " " << enable << std::endl;
#endif

    if (!enable)
    {
      // remove item focus for this view:
      const ItemFocus::Target * focus = ItemFocus::singleton().focus();

      if (focus && focus->view_ == this)
      {
        std::string focusItemId;
        ItemPtr itemPtr = focus->item_.lock();
        if (itemPtr)
        {
          focusItemId = itemPtr->id_;
        }

        TMakeCurrentContext currentContext(*context());
        ItemFocus::singleton().clearFocus(focusItemId);
      }
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
        YAE_BENCHMARK(benchmark, "ItemView::event repaintEvent");

#if YAE_DEBUG_ITEM_VIEW_REPAINT
        std::cerr << "FIXME: ItemView::repaintEvent " << root_->id_
                  << std::endl;
#endif

        Canvas::ILayer::delegate_->requestRepaint();

        repaintEvent->accept();
        return true;
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
    std::cerr << "FIXME: ItemView::requestRepaint " << root_->id_
              << " " << !alreadyRequested << std::endl;
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
    std::cerr << "FIXME: ItemView::repaint " << root_->id_
              << " " << postThePayload << std::endl;
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
  // ItemView::resize
  //
  bool
  ItemView::resizeTo(const Canvas * canvas)
  {
    int w = canvas->canvasWidth();
    int h = canvas->canvasHeight();

    if (w == w_ && h_ == h)
    {
      return false;
    }

    w_ = w;
    h_ = h;

    Item & root = *root_;
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    requestUncache(&root);
    return true;
  }

  //----------------------------------------------------------------
  // ItemView::paint
  //
  void
  ItemView::paint(Canvas * canvas)
  {
    YAE_BENCHMARK(benchmark, "ItemView::paint ");

    requestRepaintEvent_.setDelivered(true);

    animate();

    // uncache prior to painting:
    while (!uncache_.empty())
    {
      std::map<Item *, boost::weak_ptr<Item> >::iterator i = uncache_.begin();
      ItemPtr itemPtr = i->second.lock();
      uncache_.erase(i);

      if (!itemPtr)
      {
        continue;
      }

      Item & item = *itemPtr;

      YAE_BENCHMARK(benchmark, (std::string("uncache ") + item.id_).c_str());
      item.uncache();
    }

#if YAE_DEBUG_ITEM_VIEW_REPAINT
    std::cerr << "FIXME: ItemView::paint " << root_->id_ << std::endl;
#endif

    double x = 0.0;
    double y = 0.0;
    double w = double(canvas->canvasWidth());
    double h = double(canvas->canvasHeight());

    YAE_OGL_11_HERE();
    YAE_OGL_11(glViewport(GLint(x + 0.5), GLint(y + 0.5),
                          GLsizei(w + 0.5), GLsizei(h + 0.5)));

    TGLSaveMatrixState pushMatrix0(GL_MODELVIEW);
    YAE_OGL_11(glLoadIdentity());
    TGLSaveMatrixState pushMatrix1(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(0.0, w, h, 0.0, -1.0, 1.0));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
    YAE_OGL_11(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glDisable(GL_POLYGON_SMOOTH));
    YAE_OGL_11(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glLineWidth(1.0));

    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    YAE_OGL_11(glShadeModel(GL_SMOOTH));

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
    typedef std::set<boost::weak_ptr<IAnimator> >::iterator TIter;
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
    YAE_BENCHMARK(benchmark, "ItemView::processEvent");

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
#ifdef YAE_USE_QT5
        et != QEvent::UpdateRequest &&
#endif
        et != QEvent::ShortcutOverride)
    {
#if 0 // ndef NDEBUG
      std::cerr
        << "ItemView::processEvent: "
        << yae::toString(et)
        << std::endl;
#endif
    }

    if (et == QEvent::Leave)
    {
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

      if (processMouseEvent(canvas, e))
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
  // ItemView::processKeyEvent
  //
  bool
  ItemView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
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

    QPoint pos = e->pos();
    TVec2D pt(pos.x(), pos.y());
    mousePt_ = pt;

    QEvent::Type et = e->type();
    if (!((et == QEvent::MouseMove && (e->buttons() & Qt::LeftButton)) ||
          (e->button() == Qt::LeftButton)))
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
        ItemFocus::singleton().clearFocus(focus->id_);
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

        if (ia && ia->onPress(handler.csysOrigin_, pt))
        {
          pressed_ = &handler;
          return true;
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

      // FIXME: must add DPI-aware drag threshold to avoid triggering
      // spurious drag events:
      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        InputArea * ia = handler.inputArea();

        if (!dragged_ && pressed_ != &handler &&
            ia && ia->onPress(handler.csysOrigin_, startPt_))
        {
          // previous handler didn't handle the drag event, try another:
          InputArea * pi = pressed_->inputArea();
          if (pi)
          {
            pi->onCancel();
          }

          pressed_ = &handler;
        }

        if (ia && ia->onDrag(handler.csysOrigin_, startPt_, pt))
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

        if (ia)
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
      }

      pressed_ = NULL;
      dragged_ = NULL;
      inputHandlers_.clear();

      return accept;
    }
    else if (et == QEvent::MouseButtonDblClick)
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

        if (ia && ia->onDoubleClick(handler.csysOrigin_, pt))
        {
          return true;
        }
      }

      return false;
    }

    return false;
  }

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

    QPoint pos = e->pos();
    TVec2D pt(pos.x(), pos.y());

    std::list<InputHandler> handlers;
    if (!root_->getInputHandlers(pt, handlers))
    {
      return false;
    }

    // Quoting from QWheelEvent docs:
    //
    //  " Most mouse types work in steps of 15 degrees,
    //    in which case the delta value is a multiple of 120;
    //    i.e., 120 units * 1/8 = 15 degrees. "
    //
    int delta = e->delta();
    double degrees = double(delta) * 0.125;

#if 0
    std::cerr
      << "FIXME: wheel: delta: " << delta
      << ", degrees: " << degrees
      << std::endl;
#endif

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

    return processed;
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
  // ItemView::addImageProvider
  //
  void
  ItemView::addImageProvider(const QString & providerId,
                             const TImageProviderPtr & p)
  {
    imageProviders_[providerId] = p;
  }

}
