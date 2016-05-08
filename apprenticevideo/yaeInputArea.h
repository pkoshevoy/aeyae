// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_INPUT_AREA_H_
#define YAE_INPUT_AREA_H_

// standard libraries:
#include <list>
#include <stdexcept>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

// Qt interfaces:
#include <QPersistentModelIndex>

// local interfaces:
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
    InputArea(const char * id, bool draggable = true);

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

    virtual bool onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
    { return false; }

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
    boost::shared_ptr<CancelableEvent::Ticket>
    postponeSingleClickEvent(PostponeEvent & postponeEvent,
                             int msec,
                             QObject * view,
                             const TVec2D & itemCSysOrigin,
                             const TVec2D & rootCSysPoint) const;

    bool draggable_;
  };

  //----------------------------------------------------------------
  // SingleClickEvent
  //
  struct SingleClickEvent : public CancelableEvent
  {
    SingleClickEvent(const boost::shared_ptr<CancelableEvent::Ticket> & ticket,
                     const boost::weak_ptr<InputArea> & inputArea,
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
      boost::shared_ptr<InputArea> ia = inputArea_.lock();
      if (!ia)
      {
        return false;
      }

      return ia->onSingleClick(itemCSysOrigin_, rootCSysPoint_);
    }

    boost::weak_ptr<InputArea> inputArea_;
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
      boost::shared_ptr<TModelItem> modelItem = modelItem_.lock();
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
    mutable boost::weak_ptr<TModelItem> modelItem_;
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

}


#endif // YAE_INPUT_AREA_H_
