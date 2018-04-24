// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_ITEM_H_
#define YAE_ITEM_H_

// standard libraries:
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/chrono/chrono.hpp>
#include <boost/shared_ptr.hpp>
#endif

// Qt interfaces:
#include <QEvent>
#include <QPersistentModelIndex>

// local interfaces:
#include "yaeCanvas.h"
#include "yaeExpression.h"
#include "yaeItemRef.h"


namespace yae
{

  // forward declarations:
  struct InputArea;
  struct Item;

  //----------------------------------------------------------------
  // itemHeightDueToItemContent
  //
  double
  itemHeightDueToItemContent(const Item & item);


  //----------------------------------------------------------------
  // InvisibleItemZeroHeight
  //
  struct InvisibleItemZeroHeight : public TDoubleExpr
  {
    InvisibleItemZeroHeight(const Item & item);

    // virtual:
    void evaluate(double & result) const;

    const Item & item_;
  };


  //----------------------------------------------------------------
  // InscribedCircleDiameterFor
  //
  struct InscribedCircleDiameterFor : public TDoubleExpr
  {
    InscribedCircleDiameterFor(const Item & item);

    // virtual:
    void evaluate(double & result) const;

    const Item & item_;
  };


  //----------------------------------------------------------------
  // Transition
  //
  struct Transition : public TDoubleExpr
  {
    //----------------------------------------------------------------
    // TimePoint
    //
    typedef boost::chrono::steady_clock::time_point TimePoint;

    //----------------------------------------------------------------
    // Polyline
    //
    // domain: [0, 1]
    //
    struct Polyline
    {
      Polyline(double duration_sec = 1.0,
               double v0 = 0.0,
               double v1 = 1.0,
               unsigned int n = 0);

      // generate intermediate smooth interpolation points
      // by sampling the cos(t) function over the [Pi, 2Pi] interval:
      Polyline & tween_smooth(unsigned int n);

      double evaluate(double pos) const;

      typedef std::map<double, double>::value_type Point;
      std::map<double, double> pt_;
      boost::uint64_t duration_ns_;
    };

    Transition(const Polyline & spinup,
               const Polyline & steady,
               const Polyline & spindown);

    // quick check whether transition is done or steady:
    bool is_done() const;
    bool is_steady() const;

    enum State
    {
      kPending,
      kSpinup,
      kSteady,
      kSpindown,
      kDone
    };

    // check whether transition is in a particular state right now:
    State get_state(const TimePoint & now,
                    const Polyline *& seg,
                    double & seg_pos) const;

    // (re)set the start time:
    void start();
    void start_from_steady();

    // virtual:
    void evaluate(double & result) const;

    // acessor to the current value:
    inline double get_value() const
    {
      double v = 0.0;
      evaluate(v);
      return v;
    }

    // helper, for simulating periodic functions:
    double get_periodic_value(double period_scale = 1.0,
                              boost::uint64_t offset_ns = 0) const;

    inline double get_spinup_value() const
    { return segment_.begin()->second->evaluate(0.0); }

    inline double get_steady_value() const
    { return steady_.evaluate(0.5); }

    inline double get_spindown_value() const
    { return segment_.rbegin()->second->evaluate(1.0); }

  protected:
    TimePoint t0_;
    Polyline spinup_;
    Polyline steady_;
    Polyline spindown_;
    std::map<double, const Polyline *> segment_;
    boost::uint64_t duration_ns_;
  };


  //----------------------------------------------------------------
  // Margins
  //
  struct Margins
  {
    Margins();

    void uncache();
    void set(const ItemRef & ref);

    ItemRef left_;
    ItemRef right_;
    ItemRef top_;
    ItemRef bottom_;
  };


  //----------------------------------------------------------------
  // Anchors
  //
  struct Anchors
  {
    void uncache();

    void offset(const TDoubleProp & reference,
                double ox0, double ox1,
                double oy0, double oy1);

    inline void fill(const TDoubleProp & reference, double offset = 0.0)
    { inset(reference, offset, offset); }

    inline void inset(const TDoubleProp & reference, double ox, double oy)
    { offset(reference, ox, -ox, oy, -oy); }

    void center(const TDoubleProp & reference);
    void topLeft(const TDoubleProp & reference, double offset = 0.0);
    void topRight(const TDoubleProp & reference, double offset = 0.0);
    void bottomLeft(const TDoubleProp & reference, double offset = 0.0);
    void bottomRight(const TDoubleProp & reference, double offset = 0.0);

    ItemRef left_;
    ItemRef right_;
    ItemRef top_;
    ItemRef bottom_;
    ItemRef hcenter_;
    ItemRef vcenter_;
  };

  //----------------------------------------------------------------
  // InputAreaPtr
  //
  typedef boost::shared_ptr<InputArea> InputAreaPtr;

  //----------------------------------------------------------------
  // InputHandler
  //
  struct InputHandler
  {
    typedef InputArea item_type;

    InputHandler(InputArea * inputArea = NULL,
                 const TVec2D & csysOrigin = TVec2D());

    // shortcut:
    inline InputArea * inputArea() const
    { return input_.lock().get(); }

    boost::weak_ptr<InputArea> input_;
    TVec2D csysOrigin_;
  };

  //----------------------------------------------------------------
  // TInputHandlerRIter
  //
  typedef std::list<InputHandler>::reverse_iterator TInputHandlerRIter;

  //----------------------------------------------------------------
  // TInputHandlerCRIter
  //
  typedef std::list<InputHandler>::const_reverse_iterator TInputHandlerCRIter;


  //----------------------------------------------------------------
  // VisibleItem
  //
  struct VisibleItem
  {
    typedef Item item_type;

    VisibleItem(Item * item = NULL,
                const TVec2D & csysOrigin = TVec2D());

    boost::weak_ptr<Item> item_;
    TVec2D csysOrigin_;
  };

  //----------------------------------------------------------------
  // find
  //
  template <typename TItem>
  inline typename std::list<TItem>::const_iterator
  find(const std::list<TItem> & items, const typename TItem::item_type & item)
  {
    typedef typename std::list<TItem>::const_iterator const_iter_t;
    typedef typename TItem::item_type item_t;

    for (const_iter_t i = items.begin(); i != items.end(); ++i)
    {
      boost::shared_ptr<item_t> ptr = i->item_.lock();
      if (ptr.get() == &item)
      {
        return i;
      }
    }

    return items.end();
  }


  //----------------------------------------------------------------
  // Item
  //
  struct Item : public TDoubleProp,
                public TSegmentProp,
                public TBBoxProp,
                public TBoolProp,
                public TColorProp,
                public TVarProp
  {

    //----------------------------------------------------------------
    // ItemPtr
    //
    typedef boost::shared_ptr<Item> ItemPtr;

    Item(const char * id);
    virtual ~Item() {}

    //----------------------------------------------------------------
    // setParent
    //
    virtual void setParent(Item * parentItem, const ItemPtr & selfPtr)
    {
      parent_ = parentItem;
      self_ = selfPtr;
    }

    //----------------------------------------------------------------
    // sharedPtr
    //
    template <typename TItem>
    inline boost::shared_ptr<TItem> sharedPtr() const
    { return boost::dynamic_pointer_cast<TItem, Item>(self_.lock()); }

    //----------------------------------------------------------------
    // isParent
    //
    template <typename TParent>
    TParent * isParent() const
    { return dynamic_cast<TParent *>(parent_); }

    //----------------------------------------------------------------
    // parent
    //
    template <typename TParent>
    TParent & parent() const
    {
      TParent * p = this->isParent<TParent>();
      if (!p)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("parent item is not of the expected type");
      }

      return *p;
    }

    //----------------------------------------------------------------
    // hasAncestor
    //
    template <typename TItem>
    TItem * hasAncestor() const
    {
      TItem * found = NULL;
      for (const Item * i = this; i && !found; i = i->parent_)
      {
        found = i->isParent<TItem>();
      }

      return found;
    }

    //----------------------------------------------------------------
    // ancestor
    //
    template <typename TItem>
    TItem & ancestor() const
    {
      TItem * a = this->hasAncestor<TItem>();
      if (!a)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("item has no ancestors of the expected type");
      }

      return *a;
    }

    // calculate dimensions of item content, if any,
    // not counting nested item children.
    //
    // NOTE: default implementation returns 0.0
    // because it has no content besides nested children.
    virtual double calcContentWidth() const;
    virtual double calcContentHeight() const;

    // discard cached properties so they would get re-calculated (on-demand):
    virtual void uncache();

    // discard cached properties for this item only,
    // does not recurse into children or payload (scrollview content):
    virtual void uncacheSelf();

    // discard cached properties for this item and its children recursively,
    // does not recurse into payload (scrollview content):
    virtual void uncacheSelfAndChildren();

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Segment & value) const;

    // virtual:
    void get(Property property, BBox & value) const;

    // virtual:
    void get(Property property, bool & value) const;

    // virtual:
    void get(Property property, Color & value) const;

    // virtual:
    void get(Property property, TVar & value) const;

    const Segment & xContent() const;
    const Segment & yContent() const;
    const Segment & xExtent() const;
    const Segment & yExtent() const;

    double width() const;
    double height() const;
    double left() const;
    double right() const;
    double top() const;
    double bottom() const;
    double hcenter() const;
    double vcenter() const;

    bool visible() const;

    // child item lookup, will throw a runtime exception
    // if a child with a matching id is not found here:
    const Item & operator[](const char * id) const;
    Item & operator[](const char * id);

    template <typename TItem>
    inline const TItem & get(const char * id) const
    {
      const Item & found = this->operator[](id);
      return dynamic_cast<const TItem &>(found);
    }

    template <typename TItem>
    inline TItem & get(const char * id)
    {
      Item & found = this->operator[](id);
      return dynamic_cast<TItem &>(found);
    }

    template <typename TItem>
    inline TItem & insert(int i, TItem * newItem)
    {
      YAE_ASSERT(newItem);
      ItemPtr itemPtr(newItem);
      children_.insert(children_.begin() + i, itemPtr);
      newItem->Item::setParent(this, itemPtr);
      return *newItem;
    }

    template <typename TItem>
    inline TItem & add(TItem * newItem)
    {
      YAE_ASSERT(newItem);
      ItemPtr itemPtr(newItem);
      children_.push_back(itemPtr);
      newItem->Item::setParent(this, itemPtr);
      return *newItem;
    }

    template <typename TItem>
    inline TItem & addNew(const char * id)
    {
      ItemPtr itemPtr(new TItem(id));
      children_.push_back(itemPtr);
      Item & child = *(children_.back());
      child.Item::setParent(this, itemPtr);
      return static_cast<TItem &>(child);
    }

    template <typename TItem>
    inline TItem & addHidden(TItem * newItem)
    {
      YAE_ASSERT(newItem);
      ItemPtr itemPtr(newItem);
      children_.push_back(itemPtr);
      newItem->Item::setParent(this, itemPtr);
      newItem->visible_ = BoolRef::constant(false);
      return *newItem;
    }

    template <typename TItem>
    inline TItem & addNewHidden(const char * id)
    {
      TItem & item = addNew<TItem>(id);
      item.visible_ = BoolRef::constant(false);
      return item;
    }

    template <typename TData>
    inline DataRef<TData> addExpr(Expression<TData> * e)
    {
      expr_.push_back(TPropertiesBasePtr(e));
      return DataRef<TData>::expression(*e);
    }

    inline ItemRef addExpr(TDoubleExpr * e,
                           double scale = 1.0,
                           double translate = 0.0)
    {
      expr_.push_back(TPropertiesBasePtr(e));
      return ItemRef::expression(*e, scale, translate);
    }

    inline ColorRef
    addExpr(TColorExpr * e,
            const TVec4D & scale = TVec4D(1.0, 1.0, 1.0, 1.0),
            const TVec4D & translate = TVec4D(0.0, 0.0, 0.0, 0.0))
    {
      expr_.push_back(TPropertiesBasePtr(e));
      return ColorRef::expression(*e, scale, translate);
    }

    // for user-defined item attributes:
    template <typename TData>
    inline DataRef<TData>
    setAttr(const char * key, Expression<TData> * e);

    template <typename TData>
    inline DataRef<TData>
    setAttr(const char * key, const TData & value)
    {
      Expression<TData> * e = new ConstExpression<TData>(value);
      return this->setAttr(key, e);
    }

    template <typename TData>
    inline bool
    getAttr(const std::string & key, TData & value) const;

    template <typename TData>
    TData
    attr(const char * name, const TData & defaultValue = TData()) const
    {
      TData value = defaultValue;
      getAttr(std::string(name), value);
      return value;
    }

    template <typename TData>
    TData
    attr(const std::string & name, const TData & defaultValue = TData()) const
    {
      TData value = defaultValue;
      getAttr(name, value);
      return value;
    }

    // helper:
    bool overlaps(const TVec2D & pt) const;

    // breadth-first search for the input areas overlapping a given point
    virtual void
    getInputHandlers(// coordinate system origin of
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

    inline bool
    getInputHandlers(// point expressed in the coord. system of the item,
                     // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                     const TVec2D & itemCSysPoint,

                     // pass back input areas overlapping above point,
                     // along with its coord. system origin expressed
                     // in the coordinate system of the root item:
                     std::list<InputHandler> & inputHandlers)
    {
      inputHandlers.clear();
      this->getInputHandlers(TVec2D(0.0, 0.0), itemCSysPoint, inputHandlers);
      return !inputHandlers.empty();
    }

    // breadth-first search for visible items overlapping a given point
    virtual void
    getVisibleItems(// coordinate system origin of
                    // the item, expressed in the
                    // coordinate system of the root item:
                    const TVec2D & itemCSysOrigin,

                    // point expressed in the coord. system of the item,
                    // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                    const TVec2D & itemCSysPoint,

                    // pass back visible items overlapping above point,
                    // along with its coord. system origin expressed
                    // in the coordinate system of the root item:
                    std::list<VisibleItem> & visibleItems);

    inline bool
    getVisibleItems(// point expressed in the coord. system of the item,
                    // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                    const TVec2D & itemCSysPoint,

                    // pass back visible items overlapping above point,
                    // along with its coord. system origin expressed
                    // in the coordinate system of the root item:
                    std::list<VisibleItem> & visibleItems)
    {
      visibleItems.clear();
      this->getVisibleItems(TVec2D(0.0, 0.0), itemCSysPoint, visibleItems);
      return !visibleItems.empty();
    }

    virtual void onFocus();
    virtual void onFocusOut();

    // NOTE: default implementation does not process the event
    //       and does not propagate it to nested items;
    //
    // NOTE: this will be envoked for currently focused item;
    //
    virtual bool processEvent(Canvas::ILayer & canvasLayer,
                              Canvas * canvas,
                              QEvent * event);

    // NOTE: override this to provide custom visual representation:
    virtual void paintContent() const {}

    // NOTE: this will call paintContent,
    // followed by a call to paint each nested item:
    virtual bool paint(const Segment & xregion,
                       const Segment & yregion,
                       Canvas * canvas) const;

    // NOTE: unpaint will be called for an off-screen item
    // to give it an opportunity to release unneeded resources
    // (textures, images, display lists, etc...)
    virtual void unpaintContent() const {}

    // NOTE: this will call unpaintContent,
    // followed by a call to unpaint each nested item:
    virtual void unpaint() const;

    //----------------------------------------------------------------
    // Event
    //
    enum Event
    {
      kOnToggleItemView,
      kOnUncache,
      kOnFocus,
      kOnFocusOut,
      kOnPaint,
      kOnUnpaint
    };

    //----------------------------------------------------------------
    // Observer
    //
    struct Observer
    {
      virtual ~Observer() {}
      virtual void observe(const Item & item, Event e) = 0;
    };

    //----------------------------------------------------------------
    // TObserverPtr
    //
    typedef boost::shared_ptr<Observer> TObserverPtr;

    //----------------------------------------------------------------
    // TEventObservers
    //
    typedef std::map<Event, std::set<TObserverPtr> > TEventObservers;

    // add an event observer:
    inline void addObserver(Event e, const TObserverPtr & o)
    { eo_[e].insert(o); }

    // helper:
    void notifyObservers(Event e) const;

#ifndef NDEBUG
    // FIXME: for debugging only:
    virtual void dump(std::ostream & os,
                      const std::string & indent = std::string()) const;
#endif

    // item id, used for item lookup:
    std::string id_;

    // parent item:
    Item * parent_;

    // weak reference to itself, provided by the parent:
    boost::weak_ptr<Item> self_;

    // nested items:
    std::vector<ItemPtr> children_;

    // anchors shaping the layout of this item:
    Anchors anchors_;
    Margins margins_;

    // width/height references:
    ItemRef width_;
    ItemRef height_;

    // flag indicating whether this item and its children are visible:
    BoolRef visible_;

  protected:
    // storage of expressions associated with this Item:
    std::list<TPropertiesBasePtr> expr_;

    // storage of event observers associated with this Item:
    TEventObservers eo_;

    // 1D bounding segments of this items content:
    SegmentRef xContent_;
    SegmentRef yContent_;

    // 1D bounding segments of this item:
    SegmentRef xExtent_;
    SegmentRef yExtent_;

    mutable bool painted_;

  private:
    // intentionally disabled:
    Item(const Item & item);
    Item & operator = (const Item & item);
  };

  //----------------------------------------------------------------
  // ItemPtr
  //
  typedef Item::ItemPtr ItemPtr;


  //----------------------------------------------------------------
  // ModelItem
  //
  template <typename Model>
  struct ModelItem : public Item
  {
    typedef Model TModel;

    ModelItem(const char * id, const QModelIndex & modelIndex):
      Item(id),
      modelIndex_(modelIndex)
    {}

    inline Model & model() const
    {
      if (!modelIndex_.model())
      {
        YAE_ASSERT(false);
        throw std::runtime_error("model index is invalid");
      }

      return *(const_cast<Model *>
               (dynamic_cast<const Model *>
                (modelIndex_.model())));
    }

    inline const QPersistentModelIndex & modelIndex() const
    { return modelIndex_; }

  protected:
    QPersistentModelIndex modelIndex_;
  };


  //----------------------------------------------------------------
  // ExprItem
  //
  template <typename TDataRef>
  struct ExprItem : public Item
  {
    typedef TDataRef dataref_type;
    typedef typename TDataRef::value_type value_type;
    typedef Expression<value_type> expression_type;

    ExprItem(const char * id, expression_type * expression):
      Item(id)
    {
      expression_ = this->addExpr(expression);
    }

    // virtual:
    void get(Property property, value_type & value) const
    {
      if (property == kPropertyExpression)
      {
        value = expression_.get();
      }
      else
      {
        Item::get(property, value);
      }
   }

    // virtual:
    void uncache()
    {
      expression_.uncache();
      Item::uncache();
    }

    TDataRef expression_;
  };

  //----------------------------------------------------------------
  // ExpressionItem
  //
  typedef ExprItem<ItemRef> ExpressionItem;


  //----------------------------------------------------------------
  // Item::setAttr
  //
  template <typename TData>
  inline DataRef<TData>
  Item::setAttr(const char * key, Expression<TData> * e)
  {
    typedef DataRef<TData> TDataRef;
    typedef ExprItem<TDataRef> TItem;
    TItem & item = addHidden(new TItem(key, e));
    return DataRef<TData>::reference(item, kPropertyExpression);
  }

  //----------------------------------------------------------------
  // Item::getAttr
  //
  template <typename TData>
  inline bool
  Item::getAttr(const std::string & key, TData & value) const
  {
    const Item * attr = NULL;

    for (std::vector<ItemPtr>::const_iterator
           i = children_.begin(); i != children_.end(); ++i)
    {
      const Item & child = *(i->get());
      if (child.id_ == key)
      {
        attr = &child;
        YAE_ASSERT(!attr->visible());
        break;
      }
    }

    if (!attr)
    {
      return false;
    }

    attr->get(kPropertyExpression, value);
    return true;
  }


  //----------------------------------------------------------------
  // TransitionItem
  //
  struct TransitionItem : public Item
  {
    TransitionItem(const char * id,
                   const Transition::Polyline & spinup,
                   const Transition::Polyline & steady,
                   const Transition::Polyline & spindown);

    // override transition value with another (until start):
    void pause(const ItemRef & v);

    // check whether the transition has been paused:
    bool is_paused() const;

    // start the transition, remove pause override:
    void start();

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void uncache();

    // accessor:
    inline const Transition & transition() const
    { return transition_; }

  protected:
    ItemRef override_;
    ItemRef expression_;
    Transition & transition_;
  };


  //----------------------------------------------------------------
  // Periodic
  //
  struct Periodic : public TDoubleExpr
  {
    Periodic(const TransitionItem & item,
             double period_scale = 1.0,
             boost::uint64_t offset_ns = 0):
      item_(item),
      offset_ns_(offset_ns),
      period_scale_(period_scale)
    {}

    // virtual:
    void evaluate(double & value) const
    {
      const Transition & f = item_.transition();
      value = f.get_periodic_value(period_scale_, offset_ns_);
    }

    const TransitionItem & item_;
    double period_scale_;
    boost::uint64_t offset_ns_;
  };


  //----------------------------------------------------------------
  // Conditional
  //
  template <typename TDataRef>
  struct Conditional : public Expression<typename TDataRef::value_type>
  {
    typedef typename TDataRef::value_type TData;

    Conditional(const BoolRef & predicate,
                const TDataRef & a,
                const TDataRef & b):
      predicate_(predicate),
      a_(a),
      b_(b)
    {}

    // virtual:
    void evaluate(TData & result) const
    {
      bool cond = predicate_.get();
      result = cond ? a_.get() : b_.get();
    }

    const BoolRef & predicate_;
    TDataRef a_;
    TDataRef b_;
  };

}


#endif // YAE_ITEM_H_
