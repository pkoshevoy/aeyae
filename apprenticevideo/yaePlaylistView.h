// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYLIST_VIEW_H_
#define YAE_PLAYLIST_VIEW_H_

// standard libraries:
#include <cmath>
#include <list>
#include <map>
#include <vector>

// boost includes:
#include <boost/shared_ptr.hpp>

// Qt interfaces:
#include <QFont>
#include <QFontMetricsF>
#include <QMouseEvent>
#include <QObject>
#include <QPersistentModelIndex>
#include <QString>
#include <QTimer>
#include <QVariant>

// local interfaces:
#include "yaeCanvas.h"
#include "yaePlaylistModelProxy.h"
#include "yaeThumbnailProvider.h"


namespace yae
{
  // forward declarations:
  class PlaylistView;
  struct InputArea;

  //----------------------------------------------------------------
  // Vec
  //
  template <typename TData, unsigned int Cardinality>
  struct Vec
  {
    enum { kCardinality = Cardinality };
    enum { kDimension = Cardinality };
    typedef TData value_type;
    typedef Vec<TData, Cardinality> TVec;
    TData coord_[Cardinality];

    inline TVec & operator *= (const TData & scale)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] *= scale;
      }

      return *this;
    }

    inline TVec operator * (const TData & scale) const
    {
      TVec result(*this);
      result *= scale;
      return result;
    }

    inline TVec & operator += (const TData & normDelta)
    {
      TData n0 = norm();
      if (n0 > 0.0)
      {
        TData n1 = n0 + normDelta;
        TData scale = n1 / n0;
        return this->operator *= (scale);
      }

      const TData v = normDelta / std::sqrt(TData(Cardinality));
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] = v;
      }
      return *this;
    }

    inline TVec operator + (const TData & normDelta) const
    {
      TVec result(*this);
      result += normDelta;
      return result;
    }

    inline TVec & operator -= (const TData & normDelta)
    {
      return this->operator += (-normDelta);
    }

    inline TVec operator - (const TData & normDelta) const
    {
      return this->operator + (-normDelta);
    }

    inline TData operator * (const TVec & other) const
    {
      TData result = TData(0);

      for (unsigned int i = 0; i < Cardinality; i++)
      {
        result += (coord_[i] * other.coord_[i]);
      }

      return result;
    }

    inline TVec & operator += (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] += other.coord_[i];
      }

      return *this;
    }

    inline TVec operator + (const TVec & other) const
    {
      TVec result(*this);
      result += other;
      return result;
    }

    inline TVec & operator -= (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] -= other.coord_[i];
      }

      return *this;
    }

    inline TVec operator - (const TVec & other) const
    {
      TVec result(*this);
      result -= other;
      return result;
    }

    inline TVec & negate()
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        coord_[i] = -coord_[i];
      }

      return *this;
    }

    inline TVec negated() const
    {
      TVec result(*this);
      result.negate();
      return result;
    }

    inline TData normSqrd() const
    {
      TData result = TData(0);

      for (unsigned int i = 0; i < Cardinality; i++)
      {
        result += (coord_[i] * coord_[i]);
      }

      return result;
    }

    inline TData norm() const
    {
      return std::sqrt(this->normSqrd());
    }

    inline bool
    normalize(const TData & epsilon = std::numeric_limits<TData>::min())
    {
      TData n = this->norm();
      if (n > epsilon)
      {
        this->operator *= (TData(1) / n);
        return true;
      }

      this->operator *= (TData(0));
      return false;
    }

    inline TVec
    normalized(const TData & epsilon = std::numeric_limits<TData>::min()) const
    {
      TVec result(*this);
      result.normalize(epsilon);
      return result;
    }

    inline TVec
    resized(const TData & newsize)
    {
      TVec result(*this);
      double size = result.norm();
      double scale = size ? (newsize / size) : 1.0;
      result *= (scale);
      return result;
    }

    inline TVec & operator < (const TVec & other)
    {
      for (unsigned int i = 0; i < Cardinality; i++)
      {
        if (!(coord_[i] < other.coord_[i]))
        {
          return false;
        }
      }

      return true;
    }
  };

  //----------------------------------------------------------------
  // operator -
  //
  template <typename TData, unsigned int Cardinality>
  inline static Vec<TData, Cardinality>
  operator - (const Vec<TData, Cardinality> & vec)
  {
    return vec.negated();
  }

  //----------------------------------------------------------------
  // operator *
  //
  template <typename TData, unsigned int Cardinality>
  inline static Vec<TData, Cardinality>
  operator * (const TData & scale, const Vec<TData, Cardinality> & vec)
  {
    return vec * scale;
  }

  //----------------------------------------------------------------
  // TVec2D
  //
  struct TVec2D : public Vec<double, 2>
  {
    typedef Vec<double, 2> TBase;

    inline TVec2D(double x = 0.0, double y = 0.0)
    {
      TBase::coord_[0] = x;
      TBase::coord_[1] = y;
    }

    inline TVec2D(const TBase & v): TBase(v) {}
    inline TVec2D & operator = (const TBase & v)
    {
      TBase::operator = (v);
      return *this;
    }

    inline const double & x() const { return TBase::coord_[0]; }
    inline const double & y() const { return TBase::coord_[1]; }

    inline double & x() { return TBase::coord_[0]; }
    inline double & y() { return TBase::coord_[1]; }
  };

  //----------------------------------------------------------------
  // TVar
  //
  struct TVar : public QVariant
  {
    TVar():
      QVariant()
    {}

    template <typename TData>
    TVar(const TData & value):
      QVariant(value)
    {}

    inline TVar & operator *= (double scale)
    {
      (void)scale;
      return *this;
    }

    inline TVar & operator += (double translate)
    {
      (void)translate;
      return *this;
    }
  };

  //----------------------------------------------------------------
  // Segment
  //
  // 1D bounding box
  //
  struct Segment
  {
    Segment(double origin = 0.0, double length = 0.0):
      origin_(origin),
      length_(length)
    {}

    void clear();
    bool isEmpty() const;
    void expand(const Segment & seg);

    inline bool disjoint(const Segment & b) const
    { return this->start() > b.end() || b.start() > this->end(); }

    inline bool overlap(const Segment & b) const
    { return !this->disjoint(b); }

    inline bool disjoint(double pt) const
    { return this->start() > pt || pt > this->end(); }

    inline bool overlap(double pt) const
    { return !this->disjoint(pt); }

    inline double start() const
    { return origin_; }

    inline double end() const
    { return origin_ + length_; }

    inline Segment & operator *= (double scale)
    {
      length_ *= scale;
      return *this;
    }

    inline Segment & operator += (double translate)
    {
      origin_ += translate;
      return *this;
    }

    inline double center() const
    { return origin_ + 0.5 * length_; }

    inline double radius() const
    { return 0.5 * length_; }

    double origin_;
    double length_;
  };

  //----------------------------------------------------------------
  // BBox
  //
  // 2D bounding box
  //
  struct BBox
  {
    BBox():
      x_(0.0),
      y_(0.0),
      w_(0.0),
      h_(0.0)
    {}

    void clear();
    bool isEmpty() const;
    void expand(const BBox & bbox);

    inline bool disjoint(const BBox & b) const
    {
      return
        (this->left() > b.right() || b.left() > this->right()) ||
        (this->top() > b.bottom() || b.top() > this->bottom());
    }

    inline bool overlap(const BBox & b) const
    { return !this->disjoint(b); }

    inline double left() const
    { return x_; }

    inline double right() const
    { return x_ + w_; }

    inline double top() const
    { return y_; }

    inline double bottom() const
    { return y_ + h_; }

    inline Segment x() const
    { return Segment(x_, w_); }

    inline Segment y() const
    { return Segment(y_, h_); }

    inline BBox & operator *= (double scale)
    {
      w_ *= scale;
      h_ *= scale;
      return *this;
    }

    inline BBox & operator += (double translate)
    {
      x_ += translate;
      y_ += translate;
      return *this;
    }

    inline TVec2D center() const
    {
      TVec2D pt;
      pt.coord_[0] = x_ + 0.5 * w_;
      pt.coord_[1] = y_ + 0.5 * h_;
      return pt;
    }

    inline double radius() const
    { return 0.5 * (h_ < w_ ? h_ : w_); }

    inline double aspectRatio() const
    { return h_ == 0.0 ? 0.0 : (w_ / h_); }

    double x_;
    double y_;
    double w_;
    double h_;
  };

  //----------------------------------------------------------------
  // Color
  //
  struct Color
  {
    Color(unsigned int rgb = 0, double a = 1.0):
      argb_(rgb)
    {
      this->a() = (unsigned char)(std::max(0.0, std::min(255.0, 255.0 * a)));
    }

    inline const unsigned char & a() const { return this->operator[](0); }
    inline const unsigned char & r() const { return this->operator[](1); }
    inline const unsigned char & g() const { return this->operator[](2); }
    inline const unsigned char & b() const { return this->operator[](3); }

    inline unsigned char & a() { return this->operator[](0); }
    inline unsigned char & r() { return this->operator[](1); }
    inline unsigned char & g() { return this->operator[](2); }
    inline unsigned char & b() { return this->operator[](3); }

    inline const unsigned char & operator[] (unsigned int i) const
    {
      const unsigned char * argb = (const unsigned char *)&argb_;
#if __BIG_ENDIAN__
      return argb[i];
#else
      return argb[3 - i];
#endif
    }

    inline unsigned char & operator[] (unsigned int i)
    {
      unsigned char * argb = (unsigned char *)&argb_;
#if __BIG_ENDIAN__
      return argb[i];
#else
      return argb[3 - i];
#endif
    }

    Color & operator *= (double scale)
    {
      unsigned char * argb = (unsigned char *)&argb_;
      argb[0] = (unsigned char)(std::min(255.0, double(argb[0]) * scale));
      argb[1] = (unsigned char)(std::min(255.0, double(argb[1]) * scale));
      argb[2] = (unsigned char)(std::min(255.0, double(argb[2]) * scale));
      argb[3] = (unsigned char)(std::min(255.0, double(argb[3]) * scale));
      return *this;
    }

    Color & operator += (double translate)
    {
      unsigned char * argb = (unsigned char *)&argb_;

      argb[0] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[0]) + translate)));

      argb[1] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[1]) + translate)));

      argb[2] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[2]) + translate)));

      argb[3] = (unsigned char)
        (std::max(0.0, std::min(255.0, double(argb[3]) + translate)));

      return *this;
    }

    unsigned int argb_;
  };

  //----------------------------------------------------------------
  // Property
  //
  enum Property
  {
    kPropertyUnspecified,
    kPropertyConstant,
    kPropertyExpression,
    kPropertyWidth,
    kPropertyHeight,
    kPropertyLeft,
    kPropertyRight,
    kPropertyTop,
    kPropertyBottom,
    kPropertyHCenter,
    kPropertyVCenter,
    kPropertyXContent,
    kPropertyYContent,
    kPropertyXExtent,
    kPropertyYExtent,
    kPropertyBBoxContent,
    kPropertyBBox,
    kPropertyVisible
  };

  //----------------------------------------------------------------
  // IPropertiesBase
  //
  struct IPropertiesBase
  {
    virtual ~IPropertiesBase() {}
  };

  //----------------------------------------------------------------
  // TPropertiesBasePtr
  //
  typedef boost::shared_ptr<IPropertiesBase> TPropertiesBasePtr;

  //----------------------------------------------------------------
  // IProperties
  //
  template <typename TData>
  struct IProperties : public IPropertiesBase
  {
    // property accessors:
    //
    // 1. accessing a property specified via a cyclical reference
    //    will throw a runtime exception.
    // 2. accessing an unsupported property will throw a runtime exception.
    //
    virtual void get(Property property, TData & data) const = 0;
  };

  //----------------------------------------------------------------
  // TDoubleProp
  //
  typedef IProperties<double> TDoubleProp;

  //----------------------------------------------------------------
  // TSegmentProp
  //
  typedef IProperties<Segment> TSegmentProp;

  //----------------------------------------------------------------
  // TBBoxProp
  //
  typedef IProperties<BBox> TBBoxProp;

  //----------------------------------------------------------------
  // TBoolProp
  //
  typedef IProperties<bool> TBoolProp;

  //----------------------------------------------------------------
  // TVarProp
  //
  typedef IProperties<TVar> TVarProp;

  //----------------------------------------------------------------
  // Expression
  //
  template <typename TData>
  struct Expression : public IProperties<TData>
  {
    virtual void evaluate(TData & result) const = 0;

    // virtual:
    void get(Property property, TData & result) const
    {
      if (property != kPropertyExpression)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("requested a non-expression property");
      }

      evaluate(result);
    }
  };

  //----------------------------------------------------------------
  // TDoubleExpr
  //
  typedef Expression<double> TDoubleExpr;

  //----------------------------------------------------------------
  // TSegmentExpr
  //
  typedef Expression<Segment> TSegmentExpr;

  //----------------------------------------------------------------
  // TBBoxExpr
  //
  typedef Expression<BBox> TBBoxExpr;

  //----------------------------------------------------------------
  // TBoolExpr
  //
  typedef Expression<bool> TBoolExpr;

  //----------------------------------------------------------------
  // TVarExpr
  //
  typedef Expression<TVar> TVarExpr;

  //----------------------------------------------------------------
  // DataRef
  //
  template <typename TData>
  struct DataRef
  {
    //----------------------------------------------------------------
    // value_type
    //
    typedef TData value_type;

    //----------------------------------------------------------------
    // TDataProperties
    //
    typedef IProperties<TData> TDataProperties;

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            const TData & defaultValue = TData()):
      ref_(reference),
      property_(property),
      visited_(false),
      cached_(false),
      value_(defaultValue)
    {}

    //----------------------------------------------------------------
    // DataRef
    //
    DataRef(const TData & constantValue):
      ref_(NULL),
      property_(kPropertyConstant),
      visited_(false),
      cached_(false),
      value_(constantValue)
    {}

    inline void reset()
    {
      ref_ = NULL;
      property_ = kPropertyUnspecified;
    }

    // constructor helpers:
    inline static DataRef<TData>
    reference(const TDataProperties & ref, Property prop)
    { return DataRef<TData>(&ref, prop); }

    inline static DataRef<TData>
    constant(const TData & t)
    { return DataRef<TData>(t); }

    inline static DataRef<TData>
    expression(const TDataProperties & ref)
    { return DataRef<TData>(&ref, kPropertyExpression); }

    // check whether this property reference is valid:
    inline bool isValid() const
    { return property_ != kPropertyUnspecified; }

    // check whether this reference is relative:
    inline bool isRelative() const
    { return ref_ != NULL; }

    inline bool isCached() const
    { return cached_; }

    // caching is used to avoid re-calculating the same property:
    void uncache() const
    {
      visited_ = false;
      cached_ = false;
    }

    // cache an externally computed value:
    void cache(const TData & value) const
    {
      cached_ = true;
      value_ = value;
    }

    const TData & get() const
    {
      if (cached_)
      {
        return value_;
      }

      if (!ref_)
      {
        YAE_ASSERT(property_ == kPropertyConstant);
      }
      else if (visited_)
      {
        // cycle detected:
        YAE_ASSERT(false);
        throw std::runtime_error("property reference cycle detected");
      }
      else
      {
        visited_ = true;

        TData v;
        ref_->get(property_, v);
        value_ = v;
      }

      cached_ = true;
      return value_;
    }

    // reference properties:
    const TDataProperties * ref_;
    Property property_;

  protected:
    mutable bool visited_;
    mutable bool cached_;
    mutable TData value_;
  };

  //----------------------------------------------------------------
  // ItemRef
  //
  struct ItemRef : public DataRef<double>
  {
    typedef DataRef<double> TDataRef;
    typedef IProperties<double> TDataProperties;

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const TDataProperties * reference = NULL,
            Property property = kPropertyUnspecified,
            double scale = 1.0,
            double translate = 0.0,
            const double & defaultValue = 0.0):
      TDataRef(reference, property, defaultValue),
      scale_(scale),
      translate_(translate)
    {}

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const double & constantValue):
      TDataRef(constantValue),
      scale_(1.0),
      translate_(0.0)
    {}

    inline void reset()
    {
      TDataRef::reset();
      scale_ = 1.0;
      translate_ = 0.0;
    }

    // constructor helpers:
    inline static ItemRef
    reference(const TDataProperties & ref, Property prop)
    { return ItemRef(&ref, prop); }

    inline static ItemRef
    constant(const double & t)
    { return ItemRef(t); }

    inline static ItemRef
    expression(const TDataProperties & ref, double s = 1.0, double t = 0.0)
    { return ItemRef(&ref, kPropertyExpression, s, t); }

    inline static ItemRef
    scale(const TDataProperties & ref, Property prop, double s = 1.0)
    { return ItemRef(&ref, prop, s, 0.0); }

    inline static ItemRef
    offset(const TDataProperties & ref, Property prop, double t = 0.0)
    { return ItemRef(&ref, prop, 1.0, t); }

    const double & get() const
    {
      if (TDataRef::cached_)
      {
        return TDataRef::value_;
      }

      if (!TDataRef::ref_)
      {
        YAE_ASSERT(TDataRef::property_ == kPropertyConstant);
      }
      else if (TDataRef::visited_)
      {
        // cycle detected:
        YAE_ASSERT(false);
        throw std::runtime_error("property reference cycle detected");
      }
      else
      {
        TDataRef::visited_ = true;

        double v;
        ref_->get(TDataRef::property_, v);
        v *= scale_;
        v += translate_;
        TDataRef::value_ = v;
      }

      TDataRef::cached_ = true;
      return TDataRef::value_;
    }

    // reference properties:
    double scale_;
    double translate_;
  };

  //----------------------------------------------------------------
  // SegmentRef
  //
  typedef DataRef<Segment> SegmentRef;

  //----------------------------------------------------------------
  // BBoxRef
  //
  typedef DataRef<BBox> BBoxRef;

  //----------------------------------------------------------------
  // BoolRef
  //
  typedef DataRef<bool> BoolRef;

  //----------------------------------------------------------------
  // TVarRef
  //
  typedef DataRef<TVar> TVarRef;

  //----------------------------------------------------------------
  // ColorRef
  //
  typedef DataRef<Color> ColorRef;

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

    inline void fill(const TDoubleProp & reference, double offset = 0.0)
    { inset(reference, offset, offset); }

    void inset(const TDoubleProp & reference, double ox, double oy);
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
  // InputHandler
  //
  struct InputHandler
  {
    InputHandler(InputArea * inputArea = NULL,
                 const TVec2D & csysOrigin = TVec2D()):
      input_(inputArea),
      csysOrigin_(csysOrigin)
    {}

    InputArea * input_;
    TVec2D csysOrigin_;
  };

  //----------------------------------------------------------------
  // TInputHandlerRIter
  //
  typedef std::list<InputHandler>::reverse_iterator TInputHandlerRIter;


  //----------------------------------------------------------------
  // Item
  //
  struct Item : public TDoubleProp,
                public TSegmentProp,
                public TBBoxProp,
                public TBoolProp
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
    virtual void setParent(Item * parentItem)
    { parent_ = parentItem; }

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

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, Segment & value) const;

    // virtual:
    void get(Property property, BBox & value) const;

    // virtual:
    void get(Property property, bool & value) const;

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
    inline TItem & add(TItem * newItem)
    {
      YAE_ASSERT(newItem);
      children_.push_back(ItemPtr(newItem));
      newItem->Item::setParent(this);
      return *newItem;
    }

    template <typename TItem>
    inline TItem & addNew(const char * id)
    {
      children_.push_back(ItemPtr(new TItem(id)));
      Item & child = *(children_.back());
      child.Item::setParent(this);
      return static_cast<TItem &>(child);
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

    // NOTE: override this to provide custom visual representation:
    virtual void paintContent() const {}

    // NOTE: this will call paintContent,
    // followed by a call to paint each nested item:
    virtual bool paint(const Segment & xregion,
                       const Segment & yregion) const;

    // NOTE: unpaint will be called for an off-screen item
    // to give it an opportunity to release unneeded resources
    // (textures, images, display lists, etc...)
    virtual void unpaintContent() const {}

    // NOTE: this will call unpaintContent,
    // followed by a call to unpaint each nested item:
    virtual void unpaint() const;

#ifndef NDEBUG
    // FIXME: for debugging only:
    virtual void dump(std::ostream & os,
                      const std::string & indent = std::string()) const;
#endif

    // item id, used for item lookup:
    std::string id_;

    // parent item:
    Item * parent_;

    // nested items:
    std::vector<ItemPtr> children_;

    // anchors shaping the layout of this item:
    Anchors anchors_;
    Margins margins_;

    // width/height references:
    ItemRef width_;
    ItemRef height_;

    // storage of expressions associated with this Item:
    std::list<TPropertiesBasePtr> expr_;

    // 1D bounding segments of this items content:
    const SegmentRef xContent_;
    const SegmentRef yContent_;

    // 1D bounding segments of this item:
    const SegmentRef xExtent_;
    const SegmentRef yExtent_;

    // flag indicating whether this item and its children are visible:
    BoolRef visible_;

  protected:
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

    inline const QModelIndex & modelIndex() const
    { return modelIndex_; }

  protected:
    QPersistentModelIndex modelIndex_;
  };

  //----------------------------------------------------------------
  // TPlaylistModelItem
  //
  typedef ModelItem<PlaylistModelProxy> TPlaylistModelItem;


  //----------------------------------------------------------------
  // InputArea
  //
  struct InputArea : public Item
  {
    InputArea(const char * id);

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

    // NOTE: default implementation will simply call the onXxx_ delegate
    // if one is provided, otherwise it will return false:

    virtual void onCancel();

    virtual bool onMouseOver(const TVec2D & itemCSysOrigin,
                             const TVec2D & rootCSysPoint);

    virtual bool onScroll(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint,
                          double degrees);

    virtual bool onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint);

    virtual bool onClick(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint);

    virtual bool onDoubleClick(const TVec2D & itemCSysOrigin,
                               const TVec2D & rootCSysPoint);

    virtual bool onDrag(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd);

    // NOTE: default implementation of onDragEnd will call onDragEnd_
    // if one is provided, otherwise it will call onDrag(...):
    virtual bool onDragEnd(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd);

    struct OnCancel
    {
      virtual ~OnCancel() {}
      virtual void process(Item & inputAreaParent) = 0;
    };

    struct OnScroll
    {
      virtual ~OnScroll() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint,
                           double degrees) = 0;
    };

    struct OnInput
    {
      virtual ~OnInput() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint) = 0;
    };

    struct OnDrag
    {
      virtual ~OnDrag() {}
      virtual bool process(Item & inputAreaParent,
                           const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd) = 0;
    };

    typedef boost::shared_ptr<OnCancel> TOnCancel;
    typedef boost::shared_ptr<OnScroll> TOnScroll;
    typedef boost::shared_ptr<OnInput> TOnInput;
    typedef boost::shared_ptr<OnDrag> TOnDrag;

    // one does not have to subclass the InputArea to override
    // default behavior -- simply provide a delegate for
    // the behavior that should be customized:
    TOnCancel onCancel_;
    TOnInput onMouseOver_;
    TOnScroll onScroll_;
    TOnInput onPress_;
    TOnInput onClick_;
    TOnInput onDoubleClick_;
    TOnDrag onDrag_;
    TOnDrag onDragEnd_;
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
      InputArea(id),
      modelItem_(NULL)
    {}

    // lookup the closest ancestor model item associated with this input area:
    TModelItem & lookupModelItem() const
    {
      if (!modelItem_)
      {
        modelItem_ = this->hasAncestor<TModelItem>();
      }

      if (!modelItem_)
      {
        YAE_ASSERT(false);
        throw std::runtime_error("ModelInputArea requires ModelItem ancestor");
      }

      return *modelItem_;
    }

    inline Model & model() const
    { return lookupModelItem().model(); }

    inline const QModelIndex & modelIndex() const
    { return lookupModelItem().modelIndex(); }

    // virtual:
    void uncache()
    {
      InputArea::uncache();
      modelItem_ = NULL;
    }

  protected:
    // cached model item associated with this input area:
    mutable TModelItem * modelItem_;
  };

  //----------------------------------------------------------------
  // ClickableItem
  //
  template <typename Model>
  struct ClickableItem : public ModelInputArea<Model>
  {
    ClickableItem(const char * id):
      ModelInputArea<Model>(id)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }
  };


  //----------------------------------------------------------------
  // TClickablePlaylistModelItem
  //
  typedef ClickableItem<PlaylistModelProxy> TClickablePlaylistModelItem;


  //----------------------------------------------------------------
  // FlickableArea
  //
  class FlickableArea : public QObject,
                        public InputArea
  {
    Q_OBJECT;

    FlickableArea(const FlickableArea &);
    FlickableArea & operator = (const FlickableArea &);

  public:
    FlickableArea(const char * id,
                  const Canvas::ILayer & canvasLayer,
                  Item & scrollbar);
    ~FlickableArea();

    // virtual:
    bool onScroll(const TVec2D & itemCSysOrigin,
                  const TVec2D & rootCSysPoint,
                  double degrees);

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint);

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd);

    // virtual:
    bool onDragEnd(const TVec2D & itemCSysOrigin,
                   const TVec2D & rootCSysDragStart,
                   const TVec2D & rootCSysDragEnd);

  protected slots:
    void onTimeout();

  protected:
    struct TPrivate;
    TPrivate * p_;
  };


  //----------------------------------------------------------------
  // Image
  //
  class Image : public Item
  {
    Image(const Image &);
    Image & operator = (const Image &);

  public:
    Image(const char * id);
    ~Image();

    // for thumbnail providers, repaint requests, etc...
    void setContext(const PlaylistView & view);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    // this gets complicated due to asynchronous loading of images:
    struct TPrivate;
    TPrivate * p_;

    // what to load, requires a matching image provider:
    TVarRef url_;
  };

  //----------------------------------------------------------------
  // Text
  //
  class Text : public Item
  {
    Text(const Text &);
    Text & operator = (const Text &);

    BBoxRef bboxText_;

  public:
    Text(const char * id);
    ~Text();

    // helper: flag bitmask used for QFontMetricsF and QPainter::drawText
    int textFlags() const;

    // helpers:
    double fontAscent() const;
    double fontDescent() const;
    double fontHeight() const;

    // virtual:
    double calcContentWidth() const;
    double calcContentHeight() const;

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;
    void unpaintContent() const;

    struct TPrivate;
    TPrivate * p_;

    QFont font_;
    Qt::AlignmentFlag alignment_;
    Qt::TextElideMode elide_;
    double supersample_;

    TVarRef text_;
    ItemRef fontSize_; // in points
    ItemRef maxWidth_;
    ItemRef maxHeight_;
    ColorRef color_;
  };

  //----------------------------------------------------------------
  // Gradient
  //
  struct Gradient : public Item
  {
    enum Orientation { kHorizontal, kVertical };

    Gradient(const char * id);

    // virtual:
    void paintContent() const;

    std::map<double, Color> color_;
    Orientation orientation_;
  };

  //----------------------------------------------------------------
  // Rectangle
  //
  struct Rectangle : public Item
  {
    Rectangle(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    // corner radius:
    ItemRef radius_;

    // border width:
    ItemRef border_;

    ColorRef color_;
    ColorRef colorBorder_;
  };

  //----------------------------------------------------------------
  // Triangle
  //
  struct Triangle : public Item
  {
    Triangle(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    BoolRef collapsed_;
    ItemRef border_;
    ColorRef color_;
    ColorRef colorBorder_;
  };

  //----------------------------------------------------------------
  // XButton
  //
  struct XButton : public Item
  {
    XButton(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    ItemRef border_;
    ColorRef color_;
    ColorRef colorBorder_;
  };

  //----------------------------------------------------------------
  // FilterIcon
  //
  struct FilterIcon : public Item
  {
    FilterIcon(const char * id);

    // virtual:
    void uncache();

    // virtual:
    void paintContent() const;

    ColorRef color_;
  };

  //----------------------------------------------------------------
  // Scrollview
  //
  struct Scrollview : public Item
  {
    Scrollview(const char * id);

    // virtual:
    void uncache();
    bool paint(const Segment & xregion, const Segment & yregion) const;
    void unpaint();

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

#ifndef NDEBUG
    // virtual:
    void dump(std::ostream & os,
              const std::string & indent = std::string()) const;
#endif

    // item container:
    Item content_;

    // [0, 1] view position relative to content size
    // where 0 corresponds to the beginning of content
    // and 1 corresponds to the end of content
    double position_;
  };

  //----------------------------------------------------------------
  // ILayoutDelegate
  //
  template <typename TView, typename Model>
  struct YAE_API ILayoutDelegate
  {
    virtual ~ILayoutDelegate() {}

    virtual void layout(Item & item,
                        const TView & view,
                        Model & model,
                        const QModelIndex & itemIndex) = 0;
  };

  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public QObject,
                               public Canvas::ILayer
  {
    Q_OBJECT;

  public:
    typedef ILayoutDelegate<PlaylistView, PlaylistModelProxy> TLayoutDelegate;
    typedef boost::shared_ptr<TLayoutDelegate> TLayoutPtr;
    typedef PlaylistModel::LayoutHint TLayoutHint;
    typedef std::map<TLayoutHint, TLayoutPtr> TLayoutDelegates;

    typedef boost::shared_ptr<ThumbnailProvider> TImageProviderPtr;
    typedef std::map<QString, TImageProviderPtr> TImageProviders;

    PlaylistView();

    // virtual:
    void resizeTo(const Canvas * canvas);

    // virtual:
    void paint(Canvas * canvas);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);

    // helpers:
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);
    bool processWheelEvent(Canvas * canvas, QWheelEvent * event);

    // data source:
    void setModel(PlaylistModelProxy * model);

    void addImageProvider(const QString & providerId,
                          const TImageProviderPtr & p);

    // accessors:
    inline const TImageProviders & imageProviders() const
    { return imageProviders_; }

    inline const TLayoutDelegates & layouts() const
    { return layoutDelegates_; }

    inline const ItemPtr & root() const
    { return root_; }

  public slots:

    void dataChanged(const QModelIndex & topLeft,
                     const QModelIndex & bottomRight);

    void layoutAboutToBeChanged();
    void layoutChanged();

    void modelAboutToBeReset();
    void modelReset();

    void rowsAboutToBeInserted(const QModelIndex & parent, int start, int end);
    void rowsInserted(const QModelIndex & parent, int start, int end);

    void rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
    void rowsRemoved(const QModelIndex & parent, int start, int end);

  protected:
    PlaylistModelProxy * model_;
    TLayoutDelegates layoutDelegates_;
    TImageProviders imageProviders_;
    ItemPtr root_;
    double w_;
    double h_;

    // input handlers corresponding to the point where a mouse
    // button press occurred, will be cleared if layout changes
    // or mouse button release occurs:
    std::list<InputHandler> inputHandlers_;

    // mouse event handling house keeping helpers:
    InputHandler * pressed_;
    InputHandler * dragged_;
    TVec2D startPt_;
  };

}


#endif // YAE_PLAYLIST_VIEW_H_
