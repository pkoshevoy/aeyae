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
  // forward declarations:
  struct Item;

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

    double x_;
    double y_;
    double w_;
    double h_;
  };

  //----------------------------------------------------------------
  // ItemRef
  //
  struct ItemRef
  {
    //----------------------------------------------------------------
    // Property
    //
    enum Property
    {
      kUnspecified,
      kConstant,
      kWidth,
      kHeight,
      kLeft,
      kRight,
      kTop,
      kBottom,
      kHCenter,
      kVCenter
    };

    //----------------------------------------------------------------
    // ItemRef
    //
    ItemRef(const Item * referenceItem = NULL,
            Property property = kUnspecified,
            double offset = 0.0,
            double scale = 1.0);

    // constructor helpers:
    inline static ItemRef scale(const Item * ref, Property prop, double t = 1)
    { return ItemRef(ref, prop, 0.0, t); }

    inline static ItemRef offset(const Item * ref, Property prop, double t = 0)
    { return ItemRef(ref, prop, t, 1.0); }

    inline static ItemRef constant(double t)
    { return ItemRef(NULL, kConstant, t, 1.0); }

    // check whether this reference item is valid:
    inline bool isValid() const
    { return property_ != kUnspecified; }

    // check whether this reference is relative:
    inline bool isRelative() const
    { return item_ != NULL; }

    inline bool isCached() const
    { return cached_; }

    // caching is used to avoid re-calculating the same property:
    void uncache();

    // cache an externally computed value:
    void cache(double value) const;

    // NOTE: this must handle cyclical items!
    double get() const;

    // reference properties:
    const Item * item_;
    Property property_;
    double offset_;
    double scale_;

  protected:
    mutable bool visited_;
    mutable bool cached_;
    mutable double value_;
  };

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

    void fill(const Item * reference, double offset = 0.0);
    void center(const Item * reference);
    void topLeft(const Item * reference, double offset = 0.0);
    void topRight(const Item * reference, double offset = 0.0);
    void bottomLeft(const Item * reference, double offset = 0.0);
    void bottomRight(const Item * reference, double offset = 0.0);

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
  struct Item
  {

    //----------------------------------------------------------------
    // ItemPtr
    //
    typedef boost::shared_ptr<Item> ItemPtr;

    Item();
    virtual ~Item() {}

    // calculate the bounding box of item content, if any,
    // not counting nested item children.
    //
    // NOTE: default implementation returns an empty bbox
    // because it has no content besides nested children.
    virtual void calcContentBBox(BBox & bbox) const;
#if 1
    void calcOuterBBox(BBox & bbox) const;
    void calcInnerBBox(BBox & bbox) const;
#endif
    void layout();
    void uncache();

    double width() const;
    double height() const;
    double left() const;
    double right() const;
    double top() const;
    double bottom() const;
    double hcenter() const;
    double vcenter() const;

    // FIXME: for debugging only:
    void dump(std::ostream & os,
              const std::string & indent = std::string()) const;

    virtual void paint(unsigned char color) const;

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

    // cached bounding box of this item, updated during layout:
    BBox bbox_;

    // cached bounding box of this item content
    // and bounding boxes of the nested items:
    BBox bboxContent_;
  };

  //----------------------------------------------------------------
  // ItemPtr
  //
  typedef Item::ItemPtr ItemPtr;


  //----------------------------------------------------------------
  // PlaylistView
  //
  class YAE_API PlaylistView : public QObject,
                               public Canvas::ILayer
  {
    Q_OBJECT;

  public:
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
    inline static double calc_cell_width(double w)
    {
      double n = std::min<double>(5.0, std::floor(w / 160.0));
      return (n < 1.0) ? w : (w / n);
    }

    inline static double calc_cell_height(double cell_width)
    {
      double h = std::floor(cell_width * 9.0 / 16.0);
      return h;
    }

    inline unsigned int calc_items_per_row() const
    {
      double c = calc_cell_width(w_);
      double n = std::floor(w_ / c);
      return (unsigned int)n;
    }

    inline static unsigned
    calc_rows(double viewWidth, double cellWidth, unsigned int numItems)
    {
      double cellsPerRow = std::floor(viewWidth / cellWidth);
      double n = std::max(1.0, std::ceil(double(numItems) / cellsPerRow));
      return n;
    }

    Item root_;
    PlaylistModelProxy * model_;
    double w_;
    double h_;
    double position_;
    double sceneSize_;
  };

};


#endif // YAE_PLAYLIST_VIEW_H_
