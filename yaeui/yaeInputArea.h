// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_INPUT_AREA_H_
#define YAE_INPUT_AREA_H_

// aeyae:
#include "yae/api/yae_shared_ptr.h"

// standard:
#include <list>
#include <stdexcept>

// Qt:
#include <QObject>
#include <QPersistentModelIndex>

// yaeui:
#include "yaeItem.h"
#include "yaeVec.h"


namespace yae
{
  class PostponeEvent;

  //----------------------------------------------------------------
  // InputArea
  //
  struct InputArea : public Item
  {
    enum MouseButton
    {
      kNoButtons    = 0,
      kLeftButton   = (1 << 0),
      kRightButton  = (1 << 1),
      kMiddleButton = (1 << 2),
    };

    InputArea(const char * id,
              bool draggable = true,
              uint32_t allowed_buttons = kLeftButton);

    // virtual:
    void getInputHandlers(// coordinate system origin of
                          // the input area, expressed in the
                          // coordinate system of the root item:
                          const TVec2D & itemCSysOrigin,

                          // point expressed in the coord. system of the item,
                          // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                          const TVec2D & itemCSysPoint,

                          // pass back input areas overlapping above point,
                          // along with its coord. system origin expressed
                          // in the coordinate system of the root item:
                          std::list<InputHandler> & inputHandlers);

    virtual void onCancel();

    virtual bool onMouseOver(const TVec2D & itemCSysOrigin,
                             const TVec2D & rootCSysPoint)
    { return false; }

    virtual bool onScroll(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint,
                          double degrees)
    { return false; }

    virtual bool accepts(MouseButton button) const
    { return button != kNoButtons && (allowed_buttons_ & button) == button; }

    virtual bool isButtonPressed(MouseButton button) const
    { return pressed_button_ == button; }

    virtual bool onButtonPress(const TVec2D & itemCSysOrigin,
                               const TVec2D & rootCSysPoint,
                               MouseButton button)
    {
      if ((allowed_buttons_ & button) == button)
      {
        pressed_button_ = button;
        return this->onPress(itemCSysOrigin, rootCSysPoint);
      }

      return false;
    }

  protected:
    virtual bool onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
    { return false; }

  public:
    virtual bool onClick(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
    { return false; }

    virtual bool onSingleClick(const TVec2D & itemCSysOrigin,
                               const TVec2D & rootCSysPoint)
    { return false; }

    virtual bool onDoubleClick(const TVec2D & itemCSysOrigin,
                               const TVec2D & rootCSysPoint)
    { return false; }

    virtual bool onDrag(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd)
    { return false; }

    // NOTE: default implementation of onDragEnd will call onDragEnd_
    // if one is provided, otherwise it will call onDrag(...):
    virtual bool onDragEnd(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd);

    // whether an item is draggable affects how the mouse-up
    // event is handled -- a draggable item will receive onDragEnd,
    // a non-draggable item will receive onClick:
    virtual bool draggable() const
    { return draggable_; }

    // helper:
    yae::shared_ptr<CancelableEvent::Ticket>
    postponeSingleClickEvent(PostponeEvent & postponeEvent,
                             int msec,
                             QObject * view,
                             const TVec2D & itemCSysOrigin,
                             const TVec2D & rootCSysPoint) const;

  protected:
    bool draggable_;
    uint32_t allowed_buttons_;
    MouseButton pressed_button_;
  };

  //----------------------------------------------------------------
  // SingleClickEvent
  //
  struct SingleClickEvent : public CancelableEvent
  {
    SingleClickEvent(const yae::shared_ptr<CancelableEvent::Ticket> & ticket,
                     const yae::weak_ptr<InputArea, Item> & inputArea,
                     const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint):
      CancelableEvent(ticket),
      inputArea_(inputArea),
      itemCSysOrigin_(itemCSysOrigin),
      rootCSysPoint_(rootCSysPoint)
    {
      YAE_LIFETIME_START(lifetime, "03 -- SingleClickEvent");
    }

    // virtual:
    bool execute()
    {
      InputAreaPtr ia = inputArea_.lock();
      if (!ia)
      {
        return false;
      }

      return ia->onSingleClick(itemCSysOrigin_, rootCSysPoint_);
    }

    yae::weak_ptr<InputArea, Item> inputArea_;
    TVec2D itemCSysOrigin_;
    TVec2D rootCSysPoint_;
    YAE_LIFETIME(lifetime);
  };

  //----------------------------------------------------------------
  // ClickableItem
  //
  struct ClickableItem : public InputArea
  {
    ClickableItem(const char * id, bool draggable = false):
      InputArea(id, draggable)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    { return true; }
  };


  //----------------------------------------------------------------
  // DraggableItem
  //
  struct DraggableItem : public ClickableItem
  {
    DraggableItem(const char * id):
      ClickableItem(id, true)
    {}

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      dragVec_ = rootCSysDragEnd - rootCSysDragStart;
      return true;
    }

    // virtual:
    bool onDragEnd(const TVec2D & itemCSysOrigin,
                   const TVec2D & rootCSysDragStart,
                   const TVec2D & rootCSysDragEnd)
    {
      dragVec_ = TVec2D();
      return true;
    }

    bool paint(const Segment & xregion,
               const Segment & yregion,
               Canvas * canvas) const
    {
      TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
      YAE_OPENGL_HERE();
      YAE_OPENGL(glTranslated(dragVec_.x(), dragVec_.y(), 0.0));
      return ClickableItem::paint(xregion, yregion, canvas);
    }

  protected:
    TVec2D dragVec_;
  };


  //----------------------------------------------------------------
  // ModelInputArea
  //
  template <typename Model>
  struct ModelInputArea : public InputArea
  {
    typedef Model TModel;
    typedef ModelItem<Model> TModelItem;

    ModelInputArea(const char * id):
      InputArea(id)
    {}

    // lookup the closest ancestor model item associated with this input area:
    TModelItem & lookupModelItem() const
    {
      yae::shared_ptr<TModelItem, Item> modelItem = modelItem_.lock();
      if (!modelItem)
      {
        TModelItem * found = this->hasAncestor<TModelItem>();

        if (!found)
        {
          YAE_ASSERT(false);
          throw std::runtime_error("ModelInputArea requires "
                                   "ModelItem ancestor");
        }

        // update weak reference:
        modelItem = found->template sharedPtr<TModelItem>();
        modelItem_ = modelItem;
      }

      return *modelItem;
    }

    inline Model & model() const
    { return lookupModelItem().model(); }

    inline const QPersistentModelIndex & modelIndex() const
    { return lookupModelItem().modelIndex(); }

  protected:
    // cached model item associated with this input area:
    mutable yae::weak_ptr<TModelItem, Item> modelItem_;
  };


  //----------------------------------------------------------------
  // ClickableModelItem
  //
  template <typename Model>
  struct ClickableModelItem : public ModelInputArea<Model>
  {
    ClickableModelItem(const char * id):
      ModelInputArea<Model>(id)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }
  };

  //----------------------------------------------------------------
  // MouseTrap
  //
  // Consume mouse events to prevent them from
  // propagating to a lower level:
  //
  struct MouseTrap : public InputArea
  {
    MouseTrap(const char * id):
      InputArea(id),
      onScroll_(true),
      onPress_(true),
      onClick_(true),
      onDoubleClick_(true),
      onDrag_(true)
    {}

    // virtual:
    bool onScroll(const TVec2D & itemCSysOrigin,
                  const TVec2D & rootCSysPoint,
                  double degrees)
    { return onScroll_; }

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return onPress_; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return onClick_; }

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    { return onDoubleClick_; }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    { return onDrag_; }

    bool onScroll_;
    bool onPress_;
    bool onClick_;
    bool onDoubleClick_;
    bool onDrag_;
  };


  //----------------------------------------------------------------
  // CallOnClick
  //
  template <typename TCallable>
  struct CallOnClick : public ClickableItem
  {
    CallOnClick(const char * name, const TCallable & callable):
      ClickableItem(name),
      callable_(callable)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      callable_();
      return true;
    }

    TCallable callable_;
  };


  //----------------------------------------------------------------
  // Call
  //
  template <typename TObject, typename TCallable>
  struct Call : public ClickableItem
  {
    Call(const char * name,
         const TObject & object,
         TCallable TObject::* const callable):
      ClickableItem(name),
      object_(object),
      callable_(callable)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      (object_.*callable_)();
      return true;
    }

    const TObject & object_;
    TCallable TObject::* const callable_;
  };


  //----------------------------------------------------------------
  // InvokeMethod
  //
  struct YAEUI_API InvokeMethodOnClick : public InputArea
  {
    InvokeMethodOnClick(const char * id,
                        QObject & object,
                        const char * method,

                        // NOTE: QGenericArgument does not store the arg value,
                        // it only stores a reference to the arg,
                        // so the caller must ensure the lifetime of the arg
                        // does not end before the method is called,
                        // otherwise the method will be called
                        // with a dangling reference to the arg:
#if (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
                        const QGenericArgument & arg = QGenericArgument(0)
#else
                        const QMetaMethodArgument & arg = QMetaMethodArgument()
#endif
                        ):
      InputArea(id),
      object_(object),
      method_(method),
      arg_(arg)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      QMetaObject::invokeMethod(&object_, method_, arg_);
      return true;
    }

    QObject & object_;
    const char * method_;

#if (QT_VERSION < QT_VERSION_CHECK(6, 6, 0))
    QGenericArgument arg_;
#else
    QMetaMethodArgument arg_;
#endif
  };

}


#endif // YAE_INPUT_AREA_H_
