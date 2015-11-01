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
#include <vector>

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeCanvas.h"
#include "yaePlaylistModelProxy.h"


namespace yae
{

  //----------------------------------------------------------------
  // BBox
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

    inline double left() const
    { return x_; }

    inline double right() const
    { return x_ + w_; }

    inline double top() const
    { return y_; }

    inline double bottom() const
    { return y_ + h_; }

    BBox & operator *= (double scale)
    {
      w_ *= scale;
      h_ *= scale;
      return *this;
    }

    BBox & operator += (double translate)
    {
      x_ += translate;
      y_ += translate;
      return *this;
    }

    double x_;
    double y_;
    double w_;
    double h_;
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
    kPropertyBBoxContent,
    kPropertyBBox
  };

  //----------------------------------------------------------------
  // IProperties
  //
  template <typename TData>
  struct IProperties
  {
    virtual ~IProperties() {}

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
  // TBBoxProp
  //
  typedef IProperties<BBox> TBBoxProp;

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
  // TExpressionPtr
  //
  typedef boost::shared_ptr<TDoubleExpr> TDoubleExprPtr;

  //----------------------------------------------------------------
  // TBBoxExpr
  //
  typedef Expression<BBox> TBBoxExpr;

  //----------------------------------------------------------------
  // TBBoxExprPtr
  //
  typedef boost::shared_ptr<TBBoxExpr> TBBoxExprPtr;

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
            double scale = 1.0,
            double translate = 0.0,
            const TData & defaultValue = TData()):
      ref_(reference),
      property_(property),
      scale_(scale),
      translate_(translate),
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
      scale_(1.0),
      translate_(0.0),
      visited_(false),
      cached_(false),
      value_(constantValue)
    {}

    // constructor helpers:
    inline static DataRef<TData>
    reference(const TDataProperties * ref, Property prop)
    { return DataRef<TData>(ref, prop); }

    inline static DataRef<TData>
    constant(const TData & t)
    { return DataRef<TData>(t); }

    inline static DataRef<TData>
    expression(const TDataProperties * ref, double s = 1.0, double t = 0.0)
    { return DataRef<TData>(ref, kPropertyExpression, s, t); }

    inline static DataRef<TData>
    scale(const TDataProperties * ref, Property prop, double s = 1.0)
    { return DataRef<TData>(ref, prop, s, 0.0); }

    inline static DataRef<TData>
    offset(const TDataProperties * ref, Property prop, double t = 0.0)
    { return DataRef<TData>(ref, prop, 1.0, t); }

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
        v *= scale_;
        v += translate_;
        value_ = v;
      }

      cached_ = true;
      return value_;
    }

    // reference properties:
    const TDataProperties * ref_;
    Property property_;
    double scale_;
    double translate_;

  protected:
    mutable bool visited_;
    mutable bool cached_;
    mutable TData value_;
  };

  //----------------------------------------------------------------
  // ItemRef
  //
  typedef DataRef<double> ItemRef;

  //----------------------------------------------------------------
  // BBoxRef
  //
  typedef DataRef<BBox> BBoxRef;

  //----------------------------------------------------------------
  // Margins
  //
  struct Margins
  {
    Margins();

    void uncache();

    void set(double m);

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

    void fill(const TDoubleProp * reference, double offset = 0.0);
    void center(const TDoubleProp * reference);
    void topLeft(const TDoubleProp * reference, double offset = 0.0);
    void topRight(const TDoubleProp * reference, double offset = 0.0);
    void bottomLeft(const TDoubleProp * reference, double offset = 0.0);
    void bottomRight(const TDoubleProp * reference, double offset = 0.0);

    ItemRef left_;
    ItemRef right_;
    ItemRef top_;
    ItemRef bottom_;
    ItemRef hcenter_;
    ItemRef vcenter_;
  };

  //----------------------------------------------------------------
  // Item
  //
  struct Item : public TDoubleProp,
                public TBBoxProp
  {

    //----------------------------------------------------------------
    // ItemPtr
    //
    typedef boost::shared_ptr<Item> ItemPtr;

    Item(const char * id = NULL);
    virtual ~Item() {}

    // calculate the bounding box of item content, if any,
    // not counting nested item children.
    //
    // NOTE: default implementation returns an empty bbox
    // because it has no content besides nested children.
    virtual void calcContentBBox(BBox & bbox) const;

    virtual void layout();
    virtual void uncache();

    // virtual:
    void get(Property property, double & value) const;

    // virtual:
    void get(Property property, BBox & value) const;

    const BBox & bboxContent() const;
    const BBox & bbox() const;

    double width() const;
    double height() const;
    double left() const;
    double right() const;
    double top() const;
    double bottom() const;
    double hcenter() const;
    double vcenter() const;

    template <typename TItem>
    inline TItem & addNew(const char * id = NULL)
    {
      children_.push_back(ItemPtr(new TItem(id)));
      Item & child = *(children_.back());
      child.parent_ = this;
      return static_cast<TItem &>(child);
    }

    inline ItemRef addExpr(TDoubleExpr * e,
                           double scale = 1.0,
                           double translate = 0.0)
    {
      exprDouble_.push_back(TDoubleExprPtr(e));
      return ItemRef::expression(e, scale, translate);
    }

    inline BBoxRef addExpr(TBBoxExpr * e,
                           double scale = 1.0,
                           double translate = 0.0)
    {
      exprBBox_.push_back(TBBoxExprPtr(e));
      return BBoxRef::expression(e, scale, translate);
    }

    // FIXME: for debugging only:
    virtual void dump(std::ostream & os,
                      const std::string & indent = std::string()) const;

    // an item has no visual representation, but a Rectangle subclass does:
    virtual void paint() const;

    // item id, mostly used for debugging:
    std::string id_;

    // FIXME: for debugging only:
    unsigned int color_;

    // parent item:
    const Item * parent_;

    // nested items:
    std::vector<ItemPtr> children_;

    // anchors shaping the layout of this item:
    Anchors anchors_;
    Margins margins_;

    // width/height references:
    ItemRef width_;
    ItemRef height_;

    // storage of expressions associated with this Item:
    std::list<TDoubleExprPtr> exprDouble_;
    std::list<TBBoxExprPtr> exprBBox_;

    // bounding box of this items content:
    const BBoxRef bboxContent_;

    // bounding box of this item:
    const BBoxRef bbox_;

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
  // Rectangle
  //
  struct Rectangle : public Item
  {
    Rectangle(const char * id = NULL);

    virtual void paint() const;

    // corner radius:
    ItemRef radius_;

    // border:
    ItemRef border_;
    // ColorRef borderColor_;
  };

  //----------------------------------------------------------------
  // Scrollable
  //
  struct Scrollable : public Item
  {
    Scrollable(const char * id = NULL);

    virtual void layout();
    virtual void uncache();
    virtual void paint() const;

    virtual void dump(std::ostream & os,
                      const std::string & indent = std::string()) const;

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
  struct YAE_API ILayoutDelegate
  {
    typedef boost::shared_ptr<ILayoutDelegate> TLayoutPtr;
    typedef PlaylistModel::LayoutHint TLayoutHint;

    virtual ~ILayoutDelegate() {}

    virtual void layout(Item & rootItem,
                        const std::map<TLayoutHint, TLayoutPtr> & layouts,
                        const PlaylistModelProxy & model,
                        const QModelIndex & rootIndex) = 0;
  };

  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public QObject,
                               public Canvas::ILayer
  {
    Q_OBJECT;

  public:
    typedef ILayoutDelegate::TLayoutPtr TLayoutPtr;
    typedef ILayoutDelegate::TLayoutHint TLayoutHint;

    PlaylistView();

    // virtual:
    void resizeTo(const Canvas * canvas);

    // virtual:
    void paint(Canvas * canvas);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);

    // data source:
    void setModel(PlaylistModelProxy * model);

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

    ItemPtr root_;
    PlaylistModelProxy * model_;
    double w_;
    double h_;
    double position_;
    double sceneSize_;
    std::map<TLayoutHint, TLayoutPtr> layoutDelegates_;
  };

};


#endif // YAE_PLAYLIST_VIEW_H_
