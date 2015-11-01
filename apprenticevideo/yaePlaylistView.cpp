// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local interfaces:
#include "yaeCanvasQPainterUtils.h"
#include "yaePlaylistView.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // calcCellWidth
  //
  inline static double
  calcCellWidth(double rowWidth)
  {
    double n = std::min<double>(5.0, std::floor(rowWidth / 160.0));
    return (n < 1.0) ? rowWidth : (rowWidth / n);
  }

  //----------------------------------------------------------------
  // calcCellHeight
  //
  inline static double
  calcCellHeight(double cellWidth)
  {
    double h = std::floor(cellWidth * 9.0 / 16.0);
    return h;
  }

  //----------------------------------------------------------------
  // calcItemsPerRow
  //
  inline static unsigned int
  calcItemsPerRow(double rowWidth)
  {
    double c = calcCellWidth(rowWidth);
    double n = std::floor(rowWidth / c);
    return (unsigned int)n;
  }

  //----------------------------------------------------------------
  // calcRows
  //
  inline static unsigned int
  calcRows(double viewWidth, double cellWidth, unsigned int numItems)
  {
    double cellsPerRow = std::floor(viewWidth / cellWidth);
    double n = std::max(1.0, std::ceil(double(numItems) / cellsPerRow));
    return n;
  }

  //----------------------------------------------------------------
  // calcTitleHeight
  //
  inline static double
  calcTitleHeight(double minHeight, double w)
  {
    return std::max<double>(minHeight, 24.0 * w / 800.0);
  }

  //----------------------------------------------------------------
  // GridCellLeft
  //
  struct GridCellLeft : public Expression
  {
    GridCellLeft(const Item * root, std::size_t cell):
      root_(root),
      cell_(cell)
    {}

    // virtual:
    double evaluate() const
    {
      double rootWidth = root_->width();
      unsigned int cellsPerRow = calcItemsPerRow(rootWidth);
      std::size_t cellCol = cell_ % cellsPerRow;
      double ox = root_->left();
      double x = ox + rootWidth * double(cellCol) / double(cellsPerRow);
      return x;
    }

    const Item * root_;
    std::size_t cell_;
  };

  //----------------------------------------------------------------
  // GridCellTop
  //
  struct GridCellTop : public Expression
  {
    GridCellTop(const Item * root, std::size_t cell):
      root_(root),
      cell_(cell)
    {}

    // virtual:
    double evaluate() const
    {
      std::size_t numCells = root_->children_.size();
      double rootWidth = root_->width();
      double cellWidth = calcCellWidth(rootWidth);
      double cellHeight = calcCellHeight(cellWidth);
      unsigned int cellsPerRow = calcItemsPerRow(rootWidth);
      unsigned int rowsOfCells = calcRows(rootWidth, cellWidth, numCells);
      double gridHeight = cellHeight * double(rowsOfCells);
      std::size_t cellRow = cell_ / cellsPerRow;
      double oy = root_->top();
      double y = oy + gridHeight * double(cellRow) / double(rowsOfCells);
      return y;
    }

    const Item * root_;
    std::size_t cell_;
  };

  //----------------------------------------------------------------
  // GridCellWidth
  //
  struct GridCellWidth : public Expression
  {
    GridCellWidth(const Item * root):
      root_(root)
    {}

    // virtual:
    double evaluate() const
    {
      double rootWidth = root_->width();
      double cellWidth = calcCellWidth(rootWidth);
      return cellWidth;
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // GridCellHeight
  //
  struct GridCellHeight : public Expression
  {
    GridCellHeight(const Item * root):
      root_(root)
    {}

    // virtual:
    double evaluate() const
    {
      double rootWidth = root_->width();
      double cellWidth = calcCellWidth(rootWidth);
      double cellHeight = calcCellHeight(cellWidth);
      return cellHeight;
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct CalcTitleHeight : public Expression
  {
    CalcTitleHeight(const Item * root, double minHeight):
      root_(root),
      minHeight_(minHeight)
    {}

    // virtual:
    double evaluate() const
    {
      double rootWidth = root_->width();
      double titleHeight = calcTitleHeight(minHeight_, rootWidth);
      return titleHeight;
    }

    const Item * root_;
    double minHeight_;
  };

  //----------------------------------------------------------------
  // CalcContentTop
  //
  struct CalcContentTop : public Expression
  {
    CalcContentTop(const Scrollable * view):
      view_(view)
    {}

    // virtual:
    double evaluate() const
    {
      double sceneHeight = view_->content_.height();
      double viewHeight = view_->height();
      if (sceneHeight <= viewHeight)
      {
        return 0.0;
      }

      double range = sceneHeight - viewHeight;
      double top = view_->position_ * range;
      return -top;
    }

    const Scrollable * view_;
  };

  //----------------------------------------------------------------
  // BBox::clear
  //
  void
  BBox::clear()
  {
    x_ = 0.0;
    y_ = 0.0;
    w_ = 0.0;
    h_ = 0.0;
  }

  //----------------------------------------------------------------
  // BBox::isEmpty
  //
  bool
  BBox::isEmpty() const
  {
    return (w_ == 0.0) && (h_ == 0.0);
  }

  //----------------------------------------------------------------
  // BBox::expand
  //
  void
  BBox::expand(const BBox & bbox)
  {
    if (!bbox.isEmpty())
    {
      if (isEmpty())
      {
        *this = bbox;
      }
      else
      {
        double r = std::max<double>(right(), bbox.right());
        double b = std::max<double>(bottom(), bbox.bottom());
        x_ = std::min<double>(x_, bbox.x_);
        y_ = std::min<double>(y_, bbox.y_);
        w_ = r - x_;
        h_ = b - y_;
      }
    }
  }


  //----------------------------------------------------------------
  // Margins::Margins
  //
  Margins::Margins()
  {
    set(0);
  }

  //----------------------------------------------------------------
  // Margins::uncache
  //
  void
  Margins::uncache()
  {
    left_.uncache();
    right_.uncache();
    top_.uncache();
    bottom_.uncache();
  }

  //----------------------------------------------------------------
  // Margins::set
  //
  void
  Margins::set(double m)
  {
    left_ = ItemRef::constant(m);
    right_ = ItemRef::constant(m);
    top_ = ItemRef::constant(m);
    bottom_ = ItemRef::constant(m);
  }


  //----------------------------------------------------------------
  // Anchors::uncache
  //
  void
  Anchors::uncache()
  {
    left_.uncache();
    right_.uncache();
    top_.uncache();
    bottom_.uncache();
    hcenter_.uncache();
    vcenter_.uncache();
  }

  //----------------------------------------------------------------
  // Anchors::fill
  //
  void
  Anchors::fill(const IProperties * ref, double offset)
  {
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::center
  //
  void
  Anchors::center(const IProperties * ref)
  {
    hcenter_ = ItemRef::offset(ref, kPropertyHCenter);
    vcenter_ = ItemRef::offset(ref, kPropertyVCenter);
  }

  //----------------------------------------------------------------
  // Anchors::topLeft
  //
  void
  Anchors::topLeft(const IProperties * ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::topRight
  //
  void
  Anchors::topRight(const IProperties * ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomLeft
  //
  void
  Anchors::bottomLeft(const IProperties * ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomRight
  //
  void
  Anchors::bottomRight(const IProperties * ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }


  //----------------------------------------------------------------
  // Item::Item
  //
  Item::Item(const char * id):
    parent_(NULL),
    color_(0)
  {
    if (id)
    {
      id_.assign(id);
    }
  }

  //----------------------------------------------------------------
  // Item::calcContentBBox
  //
  void
  Item::calcContentBBox(BBox & bbox) const
  {
    bbox.clear();
  }
#if 0
  //----------------------------------------------------------------
  // Item::calcOuterBBox
  //
  void
  Item::calcOuterBBox(BBox & bbox) const
  {
    double ml = (margins_.left_.get() < 0 ? 0 : margins_.left_.get());
    double mr = (margins_.right_.get() < 0 ? 0 : margins_.right_.get());
    double mt = (margins_.top_.get() < 0 ? 0 : margins_.top_.get());
    double mb = (margins_.bottom_.get() < 0 ? 0 : margins_.bottom_.get());

    double r = bbox_.right();
    double b = bbox_.bottom();

    bbox.x_ = bbox_.x_ - ml;
    bbox.y_ = bbox_.y_ - mt;
    bbox.w_ = r + mr - bbox.x_;
    bbox.h_ = b + mb - bbox.y_;
  }

  //----------------------------------------------------------------
  // Item::calcInnerBBox
  //
  void
  Item::calcInnerBBox(BBox & bbox) const
  {
    double ml = (margins_.left_.get() > 0 ? 0 : margins_.left_.get());
    double mr = (margins_.right_.get() > 0 ? 0 : margins_.right_.get());
    double mt = (margins_.top_.get() > 0 ? 0 : margins_.top_.get());
    double mb = (margins_.bottom_.get() > 0 ? 0 : margins_.bottom_.get());

    double r = bbox_.right();
    double b = bbox_.bottom();

    bbox.x_ = bbox_.x_ - ml;
    bbox.y_ = bbox_.y_ - mt;
    bbox.w_ = r + mr - bbox.x_;
    bbox.h_ = b + mb - bbox.y_;
  }
#endif
  //----------------------------------------------------------------
  // Item::layout
  //
  void
  Item::layout()
  {
    calcContentBBox(bboxContent_);

    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->layout();
      bboxContent_.expand(child->bbox_);
    }

    bbox_.x_ = left();
    bbox_.y_ = top();
    bbox_.w_ = width();
    bbox_.h_ = height();
  }

  //----------------------------------------------------------------
  // Item::uncache
  //
  void
  Item::uncache()
  {
    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->uncache();
    }

    bbox_.clear();
    bboxContent_.clear();

    width_.uncache();
    height_.uncache();
    anchors_.uncache();
    margins_.uncache();
  }

  //----------------------------------------------------------------
  // Item::get
  //
  double
  Item::get(Property property) const
  {
    if (property == kPropertyWidth)
    {
      return this->width();
    }
    else if (property == kPropertyHeight)
    {
      return this->height();
    }
    else if (property == kPropertyLeft)
    {
      return this->left();
    }
    else if (property == kPropertyRight)
    {
      return this->right();
    }
    else if (property == kPropertyTop)
    {
      return this->top();
    }
    else if (property == kPropertyBottom)
    {
      return this->bottom();
    }
    else if (property == kPropertyHCenter)
    {
      return this->hcenter();
    }
    else if (property == kPropertyVCenter)
    {
      return this->vcenter();
    }

    YAE_ASSERT(false);
    throw std::runtime_error("unsupported item property");
    return std::numeric_limits<double>::max();
  }

  //----------------------------------------------------------------
  // Item::width
  //
  double
  Item::width() const
  {
    if (width_.isValid() || width_.isCached())
    {
      return width_.get();
    }

    if (anchors_.left_.isValid() && anchors_.right_.isValid())
    {
      double l = anchors_.left_.get();
      double r = anchors_.right_.get();
      l += margins_.left_.get();
      r -= margins_.right_.get();

      double w = r - l;
      width_.cache(w);

      return w;
    }

    // width is based on the bounding box of item content:
    double l = left();
    double r = bboxContent_.right();
    double w = r - l;
    width_.cache(w);
    return w;
  }

  //----------------------------------------------------------------
  // Item::height
  //
  double
  Item::height() const
  {
    if (height_.isValid() || height_.isCached())
    {
      return height_.get();
    }

    if (anchors_.top_.isValid() && anchors_.bottom_.isValid())
    {
      double t = anchors_.top_.get();
      double b = anchors_.bottom_.get();
      t += margins_.top_.get();
      b -= margins_.bottom_.get();

      double h = b - t;
      height_.cache(h);

      return h;
    }

    // height is based on the bounding box of item content:
    double t = top();
    double b = bboxContent_.bottom();
    double h = b - t;
    height_.cache(h);
    return h;
  }

  //----------------------------------------------------------------
  // Item::left
  //
  double
  Item::left() const
  {
    if (anchors_.left_.isValid())
    {
      double l = anchors_.left_.get();
      l += margins_.left_.get();
      return l;
    }

    if (anchors_.right_.isValid())
    {
      double w = width();
      double r = anchors_.right_.get();
      double l = (r - margins_.right_.get()) - w;
      return l;
    }

    if (anchors_.hcenter_.isValid())
    {
      double w = width();
      double c = anchors_.hcenter_.get();
      double l = c - 0.5 * w;
      return l;
    }

    return margins_.left_.get();
  }

  //----------------------------------------------------------------
  // Item::right
  //
  double
  Item::right() const
  {
    if (anchors_.right_.isValid())
    {
      double r = anchors_.right_.get();
      r -= margins_.right_.get();
      return r;
    }

    double l = left();
    double w = width();
    double r = l + w;
    return r;
  }

  //----------------------------------------------------------------
  // Item::top
  //
  double
  Item::top() const
  {
    if (anchors_.top_.isValid())
    {
      double t = anchors_.top_.get();
      t += margins_.top_.get();
      return t;
    }

    if (anchors_.bottom_.isValid())
    {
      double h = height();
      double b = anchors_.bottom_.get();
      double t = (b - margins_.bottom_.get()) - h;
      return t;
    }

    if (anchors_.vcenter_.isValid())
    {
      double h = height();
      double c = anchors_.vcenter_.get();
      double t = c - 0.5 * h;
      return t;
    }

    return margins_.top_.get();
  }

  //----------------------------------------------------------------
  // Item::bottom
  //
  double
  Item::bottom() const
  {
    if (anchors_.bottom_.isValid())
    {
      double b = anchors_.bottom_.get();
      b += margins_.bottom_.get();
      return b;
    }

    double t = top();
    double h = height();
    double b = t + h;
    return b;
  }

  //----------------------------------------------------------------
  // Item::hcenter
  //
  double
  Item::hcenter() const
  {
    if (anchors_.hcenter_.isValid())
    {
      double c = anchors_.hcenter_.get();
      return c;
    }

    double l = left();
    double w = width();
    double c = l + 0.5 * w;
    return c;
  }

  //----------------------------------------------------------------
  // Item::vcenter
  //
  double
  Item::vcenter() const
  {
    if (anchors_.vcenter_.isValid())
    {
      double c = anchors_.vcenter_.get();
      return c;
    }

    double t = top();
    double h = height();
    double c = t + 0.5 * h;
    return c;
  }

  //----------------------------------------------------------------
  // Item::dump
  //
  void
  Item::dump(std::ostream & os, const std::string & indent) const
  {
    os << indent
       << "x: " << bbox_.x_
       << ", y: " << bbox_.y_
       << ", w: " << bbox_.w_
       << ", h: " << bbox_.h_
       << ", id: " << id_
       << std::endl;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->dump(os, indent + "  ");
    }
  }

  //----------------------------------------------------------------
  // paintBBox
  //
  static void
  paintBBox(const BBox & bbox, unsigned int c = 0)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    double r0 = std::min(bbox.w_, bbox.h_) * 0.2;
    double w0 = bbox.w_ - 2.0 * r0;
    double h0 = bbox.h_ - 2.0 * r0;

    double color[16];
    for (double * rgba = color, * end = color + 16; rgba < end; rgba += 4)
    {
      rgba[0] = drand48();
      rgba[1] = drand48();
      rgba[2] = drand48();
      rgba[3] = 0.33;
    }

    YAE_OGL_11_HERE();
    if (!c)
    {
      double t = drand48();
      YAE_OGL_11(glColor4d(t, t, t, 0.33));
    }
    else
    {
      unsigned char r = 0xff & (c >> 24);
      unsigned char g = 0xff & (c >> 16);
      unsigned char b = 0xff & (c >> 8);
      unsigned char a = 0xff & (c);
      YAE_OGL_11(glColor4ub(r, g, b, a));
    }

    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      // YAE_OGL_11(glColor4dv(color));
      YAE_OGL_11(glVertex2d(x0, y0));

      // YAE_OGL_11(glColor4dv(color + 4));
      YAE_OGL_11(glVertex2d(x0, y1));

      // YAE_OGL_11(glColor4dv(color + 8));
      YAE_OGL_11(glVertex2d(x1, y0));

      // YAE_OGL_11(glColor4dv(color + 12));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());

    YAE_OGL_11(glBegin(GL_LINE_LOOP));
    {
      YAE_OGL_11(glColor4dv(color + 0));
      YAE_OGL_11(glVertex2d(x0, y0));

      YAE_OGL_11(glColor4dv(color + 4));
      YAE_OGL_11(glVertex2d(x0, y1));

      YAE_OGL_11(glColor4dv(color + 12));
      YAE_OGL_11(glVertex2d(x1, y1));

      YAE_OGL_11(glColor4dv(color + 8));
      YAE_OGL_11(glVertex2d(x1, y0));
    }
    YAE_OGL_11(glEnd());
  }

  //----------------------------------------------------------------
  // Item::paint
  //
  void
  Item::paint() const
  {
    paintBBox(bbox_, color_);

#if 0
    BBox outerBBox;
    calcOuterBBox(outerBBox);
    paintBBox(outerBBox);
#endif

#if 0
    BBox innerBBox;
    calcInnerBBox(innerBBox);
    paintBBox(innerBBox);
#endif

    double x = left();
    double y = top();

    // TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    // YAE_OGL_11_HERE();
    // YAE_OGL_11(glTranslated(x, y, 0.0));

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint();
    }
  }


  //----------------------------------------------------------------
  // TLayoutPtr
  //
  typedef ILayoutDelegate::TLayoutPtr TLayoutPtr;

  //----------------------------------------------------------------
  // TLayoutHint
  //
  typedef PlaylistModel::LayoutHint TLayoutHint;

  //----------------------------------------------------------------
  // findLayoutDelegate
  //
  static ILayoutDelegate::TLayoutPtr
  findLayoutDelegate(const std::map<TLayoutHint, TLayoutPtr> & delegates,
                     TLayoutHint layoutHint)
  {
    std::map<TLayoutHint, TLayoutPtr>::const_iterator found =
      delegates.find(layoutHint);

    if (found != delegates.end())
    {
      return found->second;
    }

    YAE_ASSERT(false);
    return TLayoutPtr();
  }

  //----------------------------------------------------------------
  // findLayoutDelegate
  //
  static ILayoutDelegate::TLayoutPtr
  findLayoutDelegate(const std::map<TLayoutHint, TLayoutPtr> & delegates,
                     const PlaylistModelProxy & model,
                     const QModelIndex & modelIndex)
  {
    QVariant v = model.data(modelIndex, PlaylistModel::kRoleLayoutHint);

    if (v.canConvert<TLayoutHint>())
    {
      TLayoutHint layoutHint = v.value<TLayoutHint>();
      return findLayoutDelegate(delegates, layoutHint);
    }

    YAE_ASSERT(false);
    return TLayoutPtr();
  }


  //----------------------------------------------------------------
  // GroupListLayout
  //
  struct GroupListLayout : public ILayoutDelegate
  {
    void layout(Item & root,
                const std::map<TLayoutHint, TLayoutPtr> & layouts,
                const PlaylistModelProxy & model,
                const QModelIndex & rootIndex)
    {
      Item & filter = root.addNew<Item>("filter");
      filter.anchors_.left_ = ItemRef::reference(&root, kPropertyLeft);
      filter.anchors_.top_ = ItemRef::reference(&root, kPropertyTop);
      filter.width_ = ItemRef::reference(&root, kPropertyWidth);
      filter.height_ = filter.addExpr(new CalcTitleHeight(&root, 24.0), 1.5);

      // FIXME:
      filter.color_ = 0xffffff7f;

      // FIXME: what about scrollbar?
      Scrollable & view = root.addNew<Scrollable>("scrollable");
      view.anchors_.left_ = ItemRef::reference(&root, kPropertyLeft);
      view.anchors_.right_ = ItemRef::reference(&root, kPropertyRight);
      view.anchors_.top_ = ItemRef::reference(&filter, kPropertyBottom);
      view.anchors_.bottom_ = ItemRef::reference(&root, kPropertyBottom);

      // FIXME:
      view.color_ = 0xff0000ff;

      Item & groups = view.content_;
      groups.anchors_.left_ = ItemRef::reference(&view, kPropertyLeft);
      groups.anchors_.right_ = ItemRef::reference(&view, kPropertyRight);
      groups.anchors_.top_ = ItemRef::constant(0.0);

      // FIXME:
      groups.color_ = 0x7fff0000;

      const int numGroups = model.rowCount(rootIndex);
      for (int i = 0; i < numGroups; i++)
      {
        Item & group = groups.addNew<Item>("group");
        group.anchors_.left_ = ItemRef::reference(&groups, kPropertyLeft);
        group.anchors_.right_ = ItemRef::reference(&groups, kPropertyRight);

        // FIXME:
        group.color_ = 0x7fff7f00;

        if (i < 1)
        {
          group.anchors_.top_ = ItemRef::reference(&groups, kPropertyTop);
        }
        else
        {
          Item & prev = *(groups.children_[i - 1]);
          group.anchors_.top_ = ItemRef::reference(&prev, kPropertyBottom);
        }

        QModelIndex childIndex = model.index(i, 0, rootIndex);
        ILayoutDelegate::TLayoutPtr childLayout =
           findLayoutDelegate(layouts, model, childIndex);

        if (childLayout)
        {
          childLayout->layout(group,
                              layouts,
                              model,
                              childIndex);
        }
      }
    }
  };

  //----------------------------------------------------------------
  // ItemGridLayout
  //
  struct ItemGridLayout : public ILayoutDelegate
  {
    void layout(Item & group,
                const std::map<TLayoutHint, TLayoutPtr> & layouts,
                const PlaylistModelProxy & model,
                const QModelIndex & groupIndex)
    {
      Item & spacer = group.addNew<Item>("spacer");
      spacer.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      spacer.anchors_.top_ = ItemRef::reference(&group, kPropertyTop);
      spacer.width_ = ItemRef::reference(&group, kPropertyWidth);
      spacer.height_ = spacer.addExpr(new GridCellHeight(&group), 0.2);

      // FIXME:
      spacer.color_ = 0x01010100;

      Item & title = group.addNew<Item>("title");
      title.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      title.anchors_.top_ = ItemRef::reference(&spacer, kPropertyBottom);
      title.width_ = ItemRef::reference(&group, kPropertyWidth);
      title.height_ = title.addExpr(new CalcTitleHeight(&group, 24.0));

      // FIXME:
      title.color_ = 0x7f7f7f7f;

      Item & grid = group.addNew<Item>("grid");
      grid.anchors_.top_ = ItemRef::reference(&title, kPropertyBottom);
      grid.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      grid.anchors_.right_ = ItemRef::reference(&group, kPropertyRight);

      // FIXME:
      grid.color_ = 0x01010100;

      const int numCells = model.rowCount(groupIndex);
      for (int i = 0; i < numCells; i++)
      {
        Item & cell = grid.addNew<Item>("cell");
        cell.anchors_.left_ = cell.addExpr(new GridCellLeft(&grid, i));
        cell.anchors_.top_ = cell.addExpr(new GridCellTop(&grid, i));
        cell.width_ = cell.addExpr(new GridCellWidth(&grid));
        cell.height_ = cell.addExpr(new GridCellHeight(&grid));

        // FIXME:
        cell.color_ = 0xff7f007f;

        QModelIndex childIndex = model.index(i, 0, groupIndex);
        ILayoutDelegate::TLayoutPtr childLayout =
           findLayoutDelegate(layouts, model, childIndex);

        if (childLayout)
        {
          childLayout->layout(cell, layouts, model, childIndex);
        }
      }
    }
  };

  //----------------------------------------------------------------
  // ItemGridCellLayout
  //
  struct ItemGridCellLayout : public ILayoutDelegate
  {
    void layout(Item & item,
                const std::map<TLayoutHint, TLayoutPtr> & layouts,
                const PlaylistModelProxy & model,
                const QModelIndex & itemIndex)
    {
      // FIXME: write me!
    }
  };

  //----------------------------------------------------------------
  // Scrollable::Scrollable
  //
  Scrollable::Scrollable(const char * id):
    Item(id),
    content_("content"),
    position_(0.0)
  {}

  //----------------------------------------------------------------
  // Scrollable::layout
  //
  void
  Scrollable::layout()
  {
    Item::layout();
    content_.layout();
  }

  //----------------------------------------------------------------
  // Scrollable::uncache
  //
  void
  Scrollable::uncache()
  {
    Item::uncache();
    content_.uncache();
  }

  //----------------------------------------------------------------
  // Scrollable::paint
  //
  void
  Scrollable::paint() const
  {
    double sceneHeight = content_.height();
    double viewHeight = this->height();

    double x = left();
    double y = top();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = position_ * range;
    }

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(x, y + dy, 0.0));
    content_.paint();
  }

  //----------------------------------------------------------------
  // Scrollable::dump
  //
  void
  Scrollable::dump(std::ostream & os, const std::string & indent) const
  {
    Item::dump(os, indent);
    content_.dump(os, indent + "  ");
  }


  //----------------------------------------------------------------
  // PlaylistView::PlaylistView
  //
  PlaylistView::PlaylistView():
    Canvas::ILayer(),
    model_(NULL),
    w_(0.0),
    h_(0.0),
    position_(0.0),
    sceneSize_(0.0)
  {
    layoutDelegates_[PlaylistModel::kLayoutHintGroupList] =
      TLayoutPtr(new GroupListLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGrid] =
      TLayoutPtr(new ItemGridLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGridCell] =
      TLayoutPtr(new ItemGridCellLayout());

#if 0 // FIXME: just for testing
    root_.margins_.set(20.0);
    root_.children_.push_back(ItemPtr(new Item()));
    Item & g0 = *(root_.children_.back());
    g0.anchors_.topLeft(&root_);
    g0.width_ = ItemRef::constant(160.0);
    g0.height_ = ItemRef::constant(90.0);
    g0.margins_.set(20);

    root_.children_.push_back(ItemPtr(new Item()));
    Item & g1 = *(root_.children_.back());
    g1.anchors_.top_ = ItemRef::offset(&g0, kPropertyBottom, 20.0);
    g1.anchors_.left_ = ItemRef::offset(&g0, kPropertyRight, 20.0);
    g1.width_ = ItemRef::scale(&g0, kPropertyWidth, 0.5);
    g1.height_ = ItemRef::scale(&g0, kPropertyHeight, 0.5);
    g1.margins_.set(-10);

    g0.children_.push_back(ItemPtr(new Item()));
    Item & g2 = *(g0.children_.back());
    g2.anchors_.fill(&g0, 5);
    g2.margins_.set(5);

    root_.layout();
    root_.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::resize
  //
  void
  PlaylistView::resizeTo(const Canvas * canvas)
  {
    w_ = canvas->canvasWidth();
    h_ = canvas->canvasHeight();
    root_.width_ = ItemRef::constant(w_);
    root_.height_ = ItemRef::constant(h_);
    root_.uncache();
    root_.layout();
  }

  //----------------------------------------------------------------
  // PlaylistView::paint
  //
  void
  PlaylistView::paint(Canvas * canvas)
  {
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
    YAE_OGL_11(glLineWidth(1.0));

    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    YAE_OGL_11(glShadeModel(GL_SMOOTH));

    root_.paint();

#if 0
    // FIXME: for debugging
    {
      YAE_OGL_11(glDisable(GL_LIGHTING));
      YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
      YAE_OGL_11(glLineWidth(2.0));
      YAE_OGL_11(glBegin(GL_LINES));
      {
        YAE_OGL_11(glColor3ub(0x7f, 0x00, 0x10));
        YAE_OGL_11(glVertex2i(w / 10, h / 10));
        YAE_OGL_11(glVertex2i(2 * w / 10, h / 10));
        YAE_OGL_11(glColor3ub(0xff, 0x00, 0x20));
        YAE_OGL_11(glVertex2i(2 * w / 10, h / 10));
        YAE_OGL_11(glVertex2i(3 * w / 10, h / 10));

        YAE_OGL_11(glColor3ub(0x10, 0x7f, 0x00));
        YAE_OGL_11(glVertex2i(w / 10, h / 10));
        YAE_OGL_11(glVertex2i(w / 10, 2 * h / 10));
        YAE_OGL_11(glColor3ub(0x20, 0xff, 0x00));
        YAE_OGL_11(glVertex2i(w / 10, 2 * h / 10));
        YAE_OGL_11(glVertex2i(w / 10, 3 * h / 10));
      }
      YAE_OGL_11(glEnd());
    }
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::processEvent
  //
  bool
  PlaylistView::processEvent(Canvas * canvas, QEvent * event)
  {
    QEvent::Type et = event->type();
    if (et != QEvent::Paint &&
        et != QEvent::MouseMove &&
        et != QEvent::CursorChange &&
        et != QEvent::Resize)
    {
      std::cerr
        << "PlaylistView::processEvent: "
        << yae::toString(et)
        << std::endl;
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistView::setModel
  //
  void
  PlaylistView::setModel(PlaylistModelProxy * model)
  {
    if (model_ == model)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!model_);

    model_ = model;

    // connect new model:
    bool ok = true;

    ok = connect(model_, SIGNAL(dataChanged(const QModelIndex &,
                                            const QModelIndex &)),
                 this, SLOT(dataChanged(const QModelIndex &,
                                        const QModelIndex &)));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(layoutAboutToBeChanged()),
                 this, SLOT(layoutAboutToBeChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(layoutChanged()),
                 this, SLOT(layoutChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(modelAboutToBeReset()),
                 this, SLOT(modelAboutToBeReset()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(modelReset()),
                 this, SLOT(modelReset()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(rowsAboutToBeInserted(const QModelIndex &,
                                                      int, int)),
                 this, SLOT(rowsAboutToBeInserted(const QModelIndex &,
                                                  int, int)));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(rowsInserted(const QModelIndex &,
                                             int, int)),
                 this, SLOT(rowsInserted(const QModelIndex &,
                                         int, int)));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &,
                                                     int, int)),
                 this, SLOT(rowsAboutToBeRemoved(const QModelIndex &,
                                                 int, int)));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(rowsRemoved(const QModelIndex &,
                                            int, int)),
                 this, SLOT(rowsRemoved(const QModelIndex &,
                                        int, int)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // toString
  //
  static std::string
  toString(const QModelIndex & index)
  {
    std::string path;

    QModelIndex ix = index;
    do
    {
      int row = ix.row();

      std::ostringstream oss;
      oss << row;
      if (!path.empty())
      {
        oss << '.' << path;
      }

      path = oss.str().c_str();
      ix = ix.parent();
    }
    while (ix.isValid());

    return path;
  }

  //----------------------------------------------------------------
  // PlaylistView::dataChanged
  //
  void
  PlaylistView::dataChanged(const QModelIndex & topLeft,
                            const QModelIndex & bottomRight)
  {
    std::cerr
      << "PlaylistView::dataChanged, topLeft: " << toString(topLeft)
      << ", bottomRight: " << toString(bottomRight)
      << std::endl;

    Canvas::ILayer::delegate_->requestRepaint();
  }

  //----------------------------------------------------------------
  // PlaylistView::layoutAboutToBeChanged
  //
  void
  PlaylistView::layoutAboutToBeChanged()
  {
    std::cerr << "PlaylistView::layoutAboutToBeChanged" << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::layoutChanged
  //
  void
  PlaylistView::layoutChanged()
  {
    std::cerr << "PlaylistView::layoutChanged" << std::endl;

    QModelIndex rootIndex = model_->index(0, 0).parent();
    TLayoutPtr delegate = findLayoutDelegate(layoutDelegates_,
                                             *model_,
                                             rootIndex);
    if (!delegate)
    {
      return;
    }

    root_ = Item("playlist");
    root_.anchors_.left_ = ItemRef::constant(0.0);
    root_.anchors_.top_ = ItemRef::constant(0.0);
    root_.width_ = ItemRef::constant(w_);
    root_.height_ = ItemRef::constant(h_);

    // FIXME:
    root_.color_ = 0x01010100;

    delegate->layout(root_,
                     layoutDelegates_,
                     *model_,
                     rootIndex);

#ifndef NDEBUG
    // FIXME: for debugging only:
    root_.layout();
    root_.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::modelAboutToBeReset
  //
  void
  PlaylistView::modelAboutToBeReset()
  {
    std::cerr << "PlaylistView::modelAboutToBeReset" << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::modelReset
  //
  void
  PlaylistView::modelReset()
  {
    std::cerr << "PlaylistView::modelReset" << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsAboutToBeInserted
  //
  void
  PlaylistView::rowsAboutToBeInserted(const QModelIndex & parent,
                                      int start, int end)
  {
    std::cerr
      << "PlaylistView::rowsAboutToBeInserted, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsInserted
  //
  void
  PlaylistView::rowsInserted(const QModelIndex & parent, int start, int end)
  {
    std::cerr
      << "PlaylistView::rowsInserted, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsAboutToBeRemoved
  //
  void
  PlaylistView::rowsAboutToBeRemoved(const QModelIndex & parent,
                                     int start, int end)
  {
    std::cerr
      << "PlaylistView::rowsAboutToBeRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsRemoved
  //
  void
  PlaylistView::rowsRemoved(const QModelIndex & parent, int start, int end)
  {
    std::cerr
      << "PlaylistView::rowsRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
  }

}
