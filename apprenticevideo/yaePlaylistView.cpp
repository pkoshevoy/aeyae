// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Tue Oct 20 19:19:59 PDT 2015
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard C++:
#include <cmath>
#include <iomanip>
#include <limits>

// Qt library:
#include <QFontMetricsF>

// local interfaces:
#include "yaeCanvasRenderer.h"
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
  struct GridCellLeft : public TDoubleExpr
  {
    GridCellLeft(const Item * root, std::size_t cell):
      root_(root),
      cell_(cell)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double rootWidth = root_->width();
      unsigned int cellsPerRow = calcItemsPerRow(rootWidth);
      std::size_t cellCol = cell_ % cellsPerRow;
      double ox = root_->left() + 2;
      result = ox + rootWidth * double(cellCol) / double(cellsPerRow);
    }

    const Item * root_;
    std::size_t cell_;
  };

  //----------------------------------------------------------------
  // GridCellTop
  //
  struct GridCellTop : public TDoubleExpr
  {
    GridCellTop(const Item * root, std::size_t cell):
      root_(root),
      cell_(cell)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t numCells = root_->children_.size();
      double rootWidth = root_->width();
      double cellWidth = calcCellWidth(rootWidth);
      double cellHeight = calcCellHeight(cellWidth);
      unsigned int cellsPerRow = calcItemsPerRow(rootWidth);
      unsigned int rowsOfCells = calcRows(rootWidth, cellWidth, numCells);
      double gridHeight = cellHeight * double(rowsOfCells);
      std::size_t cellRow = cell_ / cellsPerRow;
      double oy = root_->top() + 2;
      result = oy + gridHeight * double(cellRow) / double(rowsOfCells);
    }

    const Item * root_;
    std::size_t cell_;
  };

  //----------------------------------------------------------------
  // GridCellWidth
  //
  struct GridCellWidth : public TDoubleExpr
  {
    GridCellWidth(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double rootWidth = root_->width();
      result = calcCellWidth(rootWidth) - 2;
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // GridCellHeight
  //
  struct GridCellHeight : public TDoubleExpr
  {
    GridCellHeight(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double rootWidth = root_->width();
      double cellWidth = calcCellWidth(rootWidth);
      result = calcCellHeight(cellWidth) - 2;
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct CalcTitleHeight : public TDoubleExpr
  {
    CalcTitleHeight(const Item * root, double minHeight):
      root_(root),
      minHeight_(minHeight)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double rootWidth = root_->width();
      result = calcTitleHeight(minHeight_, rootWidth);
    }

    const Item * root_;
    double minHeight_;
  };

  //----------------------------------------------------------------
  // CalcSliderTop
  //
  struct CalcSliderTop : public TDoubleExpr
  {
    CalcSliderTop(const Scrollable * view, const Item * slider):
      view_(view),
      slider_(slider)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = view_->top();

      double sceneHeight = view_->content_.height();
      double viewHeight = view_->height();
      if (sceneHeight <= viewHeight)
      {
        return;
      }

      double scale = viewHeight / sceneHeight;
      double minHeight = slider_->width() * 5.0;
      double height = minHeight + (viewHeight - minHeight) * scale;
      double y = (viewHeight - height) * view_->position_;
      result += y;
    }

    const Scrollable * view_;
    const Item * slider_;
  };

  //----------------------------------------------------------------
  // CalcSliderHeight
  //
  struct CalcSliderHeight : public TDoubleExpr
  {
    CalcSliderHeight(const Scrollable * view, const Item * slider):
      view_(view),
      slider_(slider)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double sceneHeight = view_->content_.height();
      double viewHeight = view_->height();
      if (sceneHeight <= viewHeight)
      {
        result = viewHeight;
        return;
      }

      double scale = viewHeight / sceneHeight;
      double minHeight = slider_->width() * 5.0;
      result = minHeight + (viewHeight - minHeight) * scale;
    }

    const Scrollable * view_;
    const Item * slider_;
  };

  //----------------------------------------------------------------
  // CalcXContent
  //
  struct CalcXContent : public TSegmentExpr
  {
    CalcXContent(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = root_->calcContentWidth();
      result.origin_ =
        root_->anchors_.left_.isValid() ?
        root_->left() :

        root_->anchors_.right_.isValid() ?
        root_->right() - result.length_ :

        root_->hcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = root_->children_.begin();
           i != root_->children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->x();
        result.expand(footprint);
      }
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcYContent
  //
  struct CalcYContent : public TSegmentExpr
  {
    CalcYContent(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = root_->calcContentHeight();
      result.origin_ =
        root_->anchors_.top_.isValid() ?
        root_->top() :

        root_->anchors_.bottom_.isValid() ?
        root_->bottom() - result.length_ :

        root_->vcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = root_->children_.begin();
           i != root_->children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->y();
        result.expand(footprint);
      }
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcX
  //
  struct CalcX : public TSegmentExpr
  {
    CalcX(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = root_->left();
      result.length_ = root_->width();
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcY
  //
  struct CalcY : public TSegmentExpr
  {
    CalcY(const Item * root):
      root_(root)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = root_->top();
      result.length_ = root_->height();
    }

    const Item * root_;
  };

  //----------------------------------------------------------------
  // CalcTextBBox
  //
  struct CalcTextBBox : public TBBoxExpr
  {
    CalcTextBBox(const Text * root):
      root_(root)
    {}

    // virtual:
    void evaluate(BBox & result) const
    {
      root_->calcTextBBox(result);
    }

    const Text * root_;
  };

  //----------------------------------------------------------------
  // ModelQuery
  //
  struct ModelQuery : public TVarExpr
  {
    ModelQuery(const PlaylistModelProxy & model,
               const QModelIndex & index,
               int role):
      model_(model),
      index_(index),
      role_(role)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      static_cast<QVariant &>(result) = model_.data(index_, role_);
    }

    const PlaylistModelProxy & model_;
    QModelIndex index_;
    int role_;
  };

  //----------------------------------------------------------------
  // Segment::clear
  //
  void
  Segment::clear()
  {
    origin_ = 0.0;
    length_ = 0.0;
  }

  //----------------------------------------------------------------
  // Segment::isEmpty
  //
  bool
  Segment::isEmpty() const
  {
    return (length_ == 0.0);
  }

  //----------------------------------------------------------------
  // Segment::expand
  //
  void
  Segment::expand(const Segment & segment)
  {
    if (!segment.isEmpty())
    {
      if (isEmpty())
      {
        *this = segment;
      }
      else
      {
        double e = std::max<double>(end(), segment.end());
        origin_ = std::min<double>(origin_, segment.origin_);
        length_ = e - origin_;
      }
    }
  }


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
  Anchors::fill(const TDoubleProp * ref, double offset)
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
  Anchors::center(const TDoubleProp * ref)
  {
    hcenter_ = ItemRef::offset(ref, kPropertyHCenter);
    vcenter_ = ItemRef::offset(ref, kPropertyVCenter);
  }

  //----------------------------------------------------------------
  // Anchors::topLeft
  //
  void
  Anchors::topLeft(const TDoubleProp * ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::topRight
  //
  void
  Anchors::topRight(const TDoubleProp * ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomLeft
  //
  void
  Anchors::bottomLeft(const TDoubleProp * ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomRight
  //
  void
  Anchors::bottomRight(const TDoubleProp * ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }


  //----------------------------------------------------------------
  // Item::Item
  //
  Item::Item(const char * id):
    color_(0),
    parent_(NULL),
    xContent_(addExpr(new CalcXContent(this))),
    yContent_(addExpr(new CalcYContent(this))),
    x_(addExpr(new CalcX(this))),
    y_(addExpr(new CalcY(this))),
    visible_(TVarRef::constant(TVar(true)))
  {
    if (id)
    {
      id_.assign(id);
    }
  }

  //----------------------------------------------------------------
  // Item::calcContentWidth
  //
  double
  Item::calcContentWidth() const
  {
    return 0.0;
  }

  //----------------------------------------------------------------
  // Item::calcContentHeight
  //
  double
  Item::calcContentHeight() const
  {
    return 0.0;
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

    anchors_.uncache();
    margins_.uncache();
    width_.uncache();
    height_.uncache();
    xContent_.uncache();
    yContent_.uncache();
    x_.uncache();
    y_.uncache();
    visible_.uncache();
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, double & value) const
  {
    if (property == kPropertyWidth)
    {
      value = this->width();
    }
    else if (property == kPropertyHeight)
    {
      value = this->height();
    }
    else if (property == kPropertyLeft)
    {
      value = this->left();
    }
    else if (property == kPropertyRight)
    {
      value = this->right();
    }
    else if (property == kPropertyTop)
    {
      value = this->top();
    }
    else if (property == kPropertyBottom)
    {
      value = this->bottom();
    }
    else if (property == kPropertyHCenter)
    {
      value = this->hcenter();
    }
    else if (property == kPropertyVCenter)
    {
      value = this->vcenter();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <double>");
      value = std::numeric_limits<double>::max();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, Segment & value) const
  {
    if (property == kPropertyXContent)
    {
      value = this->xContent();
    }
    else if (property == kPropertyYContent)
    {
      value = this->yContent();
    }
    else if (property == kPropertyX)
    {
      value = this->x();
    }
    else if (property == kPropertyY)
    {
      value = this->y();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <Segment>");
      value = Segment();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, BBox & bbox) const
  {
    if (property == kPropertyBBoxContent)
    {
      const Segment & x = this->xContent();
      const Segment & y = this->yContent();

      bbox.x_ = x.origin_;
      bbox.w_ = x.length_;

      bbox.y_ = y.origin_;
      bbox.h_ = y.length_;
    }
    else if (property == kPropertyBBox)
    {
      const Segment & x = this->x();
      const Segment & y = this->y();

      bbox.x_ = x.origin_;
      bbox.w_ = x.length_;

      bbox.y_ = y.origin_;
      bbox.h_ = y.length_;
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <BBox>");
      bbox = BBox();
    }
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  Item::get(Property property, bool & value) const
  {
    if (property == kPropertyVisible)
    {
      value = this->visible();
    }
    else
    {
      YAE_ASSERT(false);
      throw std::runtime_error("unsupported item property of type <bool>");
      value = false;
    }
  }

  //----------------------------------------------------------------
  // Item::xContent
  //
  const Segment &
  Item::xContent() const
  {
    return xContent_.get();
  }

  //----------------------------------------------------------------
  // Item::yContent
  //
  const Segment &
  Item::yContent() const
  {
    return yContent_.get();
  }

  //----------------------------------------------------------------
  // Item::x
  //
  const Segment &
  Item::x() const
  {
    return x_.get();
  }

  //----------------------------------------------------------------
  // Item::y
  //
  const Segment &
  Item::y() const
  {
    return y_.get();
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

    // height is based on horizontal footprint of item content:
    const Segment & xContent = this->xContent();
    double w = 0.0;

    if (!xContent.isEmpty())
    {
      double l = anchors_.left_.isValid() ? left() : xContent.start();
      double r = anchors_.left_.isValid() ? xContent.end() : right();
      w = r - l;
    }

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

    // height is based on vertical footprint of item content:
    const Segment & yContent = this->yContent();
    double h = 0.0;

    if (!yContent.isEmpty())
    {
      double t = anchors_.top_.isValid() ? top() : yContent.start();
      double b = anchors_.top_.isValid() ? yContent.end() : bottom();
      h = b - t;
    }

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
  // Item::visible
  //
  bool
  Item::visible() const
  {
    return visible_.get().toBool();
  }

  //----------------------------------------------------------------
  // Item::dump
  //
  void
  Item::dump(std::ostream & os, const std::string & indent) const
  {
    BBox bbox;
    this->get(kPropertyBBox, bbox);

    os << indent
       << "x: " << bbox.x_
       << ", y: " << bbox.y_
       << ", w: " << bbox.w_
       << ", h: " << bbox.h_
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
  // drand
  //
  inline static double
  drand()
  {
#ifdef _WIN32
    int r = rand();
    return double(r) / double(RAND_MAX);
#else
    return drand48();
#endif
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

    double color[16];
    for (double * rgba = color, * end = color + 16; rgba < end; rgba += 4)
    {
      rgba[0] = drand();
      rgba[1] = drand();
      rgba[2] = drand();
      rgba[3] = 0.33;
    }

    YAE_OGL_11_HERE();
    if (!c)
    {
      double t = drand();
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
  bool
  Item::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::visible())
    {
      return false;
    }

    const Segment & yfootprint = this->y();
    if (yregion.disjoint(yfootprint))
    {
      return false;
    }

    const Segment & xfootprint = this->x();
    if (xregion.disjoint(xfootprint))
    {
      return false;
    }

    // std::cerr << "FIXME: paint: " << id_ << std::endl;
    this->paintContent();

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint(xregion, yregion);
    }

    return true;
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
  // layoutFilterItem
  //
  static void
  layoutFilterItem(Item & root,
                   const std::map<TLayoutHint, TLayoutPtr> & layouts,
                   const PlaylistModelProxy & model,
                   const QModelIndex & rootIndex)
  {
      Rectangle & filter = root.addNew<Rectangle>("bg");
      filter.anchors_.fill(&root);
      filter.margins_.set(2);
      filter.radius_ = ItemRef::constant(3);
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
      layoutFilterItem(filter, layouts, model, rootIndex);

      Scrollable & view = root.addNew<Scrollable>("scrollable");

      // FIXME:
      view.color_ = 0xff0000ff;

      Item & scrollbar = root.addNew<Item>("scrollbar");

      // FIXME:
      scrollbar.color_ = 0x80ff0000;

      scrollbar.anchors_.right_ = ItemRef::reference(&root, kPropertyRight);
      scrollbar.anchors_.top_ = ItemRef::offset(&filter, kPropertyBottom, 5);
      scrollbar.anchors_.bottom_ = ItemRef::offset(&root, kPropertyBottom, -5);
      scrollbar.width_ = filter.addExpr(new CalcTitleHeight(&root, 50.0), 0.2);

      view.anchors_.left_ = ItemRef::reference(&root, kPropertyLeft);
      view.anchors_.right_ = ItemRef::reference(&scrollbar, kPropertyLeft);
      // view.anchors_.right_ = ItemRef::reference(&root, kPropertyRight);
      view.anchors_.top_ = ItemRef::reference(&filter, kPropertyBottom);
      view.anchors_.bottom_ = ItemRef::reference(&root, kPropertyBottom);

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

      // configure scrollbar:
      Rectangle & slider = scrollbar.addNew<Rectangle>("slider");
      slider.anchors_.top_ = slider.addExpr(new CalcSliderTop(&view, &slider));
      slider.anchors_.left_ = ItemRef::offset(&scrollbar, kPropertyLeft, 2);
      slider.anchors_.right_ = ItemRef::offset(&scrollbar, kPropertyRight, -2);
      slider.height_ = slider.addExpr(new CalcSliderHeight(&view, &slider));
      slider.radius_ = ItemRef::scale(&slider, kPropertyWidth, 0.5);
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

      Rectangle & title = group.addNew<Rectangle>("title");
      title.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      title.anchors_.top_ = ItemRef::reference(&spacer, kPropertyBottom);
      title.width_ = ItemRef::reference(&group, kPropertyWidth);
      title.height_ = title.addExpr(new CalcTitleHeight(&group, 24.0));

      Item & grid = group.addNew<Item>("grid");
      grid.anchors_.top_ = ItemRef::reference(&title, kPropertyBottom);
      grid.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      grid.anchors_.right_ = ItemRef::reference(&group, kPropertyRight);

      // FIXME:
      grid.color_ = 0x01010100;

      const int numCells = model.rowCount(groupIndex);
      for (int i = 0; i < numCells; i++)
      {
        Rectangle & cell = grid.addNew<Rectangle>("cell");
        cell.anchors_.left_ = cell.addExpr(new GridCellLeft(&grid, i));
        cell.anchors_.top_ = cell.addExpr(new GridCellTop(&grid, i));
        cell.width_ = cell.addExpr(new GridCellWidth(&grid));
        cell.height_ = cell.addExpr(new GridCellHeight(&grid));
        cell.border_ = ItemRef::constant(1);

        QModelIndex childIndex = model.index(i, 0, groupIndex);
        ILayoutDelegate::TLayoutPtr childLayout =
           findLayoutDelegate(layouts, model, childIndex);

        if (childLayout)
        {
          childLayout->layout(cell, layouts, model, childIndex);
        }
      }

      Item & footer = group.addNew<Item>("footer");
      footer.anchors_.left_ = ItemRef::reference(&group, kPropertyLeft);
      footer.anchors_.top_ = ItemRef::reference(&grid, kPropertyBottom);
      footer.width_ = ItemRef::reference(&group, kPropertyWidth);
      footer.height_ = footer.addExpr(new GridCellHeight(&group), 0.3);

      // FIXME:
      footer.color_ = 0x02020200;
    }
  };

  //----------------------------------------------------------------
  // ItemGridCellLayout
  //
  struct ItemGridCellLayout : public ILayoutDelegate
  {
    void layout(Item & root,
                const std::map<TLayoutHint, TLayoutPtr> & layouts,
                const PlaylistModelProxy & model,
                const QModelIndex & index)
    {
      Image & thumbnail = root.addNew<Image>("thumbnail");
      thumbnail.anchors_.fill(&root);
      thumbnail.url_ = thumbnail.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleThumbnail));

#ifdef __APPLE__
      static const double dpiScale = 1.0;
#else
      static const double dpiScale = 72.0 / 96.0;
#endif

      Text & label = root.addNew<Text>("label");
      label.anchors_.bottomLeft(&root);
      label.anchors_.left_ = ItemRef::offset(&root, kPropertyLeft, 5);
      label.anchors_.bottom_ = ItemRef::offset(&root, kPropertyBottom, -5);
      label.maxWidth_ = ItemRef::offset(&root, kPropertyWidth, -10);
      label.text_ = label.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleLabel));
      label.font_.setBold(false);
      label.fontSize_ =
        ItemRef::scale(&root, kPropertyHeight, 0.15 * dpiScale);

      Item & rm = root.addNew<Item>("remove item");

      Text & playing = root.addNew<Text>("now playing");
      playing.anchors_.top_ = ItemRef::offset(&root, kPropertyTop, 5);
      playing.anchors_.right_ = ItemRef::reference(&rm, kPropertyLeft);
      playing.visible_ = playing.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRolePlaying));
      playing.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
      playing.font_.setBold(false);
      playing.fontSize_ =
        ItemRef::scale(&root, kPropertyHeight, 0.12 * dpiScale);

      rm.width_ = ItemRef::reference(&playing, kPropertyHeight);
      rm.height_ = ItemRef::reference(&playing, kPropertyHeight);
      rm.anchors_.top_ = ItemRef::reference(&playing, kPropertyTop);
      rm.anchors_.right_ = ItemRef::offset(&root, kPropertyRight, -5);
      rm.margins_.set(3);

      Rectangle & underline = root.addNew<Rectangle>("underline");
      underline.anchors_.left_ = ItemRef::offset(&playing, kPropertyLeft, -1);
      underline.anchors_.right_ = ItemRef::offset(&playing, kPropertyRight, 1);
      underline.anchors_.top_ = ItemRef::offset(&playing, kPropertyBottom, 2);
      underline.height_ = ItemRef::constant(2);
      underline.color_ = ColorRef::constant(Color(0xff0000));
      underline.visible_ = underline.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRolePlaying));

      Rectangle & sel = root.addNew<Rectangle>("selected");
      sel.anchors_.left_ = ItemRef::reference(&root, kPropertyLeft);
      sel.anchors_.right_ = ItemRef::reference(&root, kPropertyRight);
      sel.anchors_.bottom_ = ItemRef::reference(&root, kPropertyBottom);
      sel.margins_.set(3);
      sel.height_ = ItemRef::constant(2);
      sel.color_ = ColorRef::constant(Color(0xff0000));
      sel.visible_ = sel.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleSelected));
    }
  };

  //----------------------------------------------------------------
  // Image::TPrivate
  //
  struct Image::TPrivate
  {
    void load(const QString & url)
    {
      // FIXME: use ThumbnailProvider to load the image:
      url_ = url;
    }

    void paint()
    {
      // FIXME: write me!
    }

    QString url_;
  };

  //----------------------------------------------------------------
  // Image::Image
  //
  Image::Image(const char * id):
    Item(id),
    p_(new Image::TPrivate())
  {}

  //----------------------------------------------------------------
  // Image::~Image
  //
  Image::~Image()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Image::uncache
  //
  void
  Image::uncache()
  {
    url_.uncache();
  }

  //----------------------------------------------------------------
  // Image::paint
  //
  void
  Image::paintContent() const
  {
    if (!Item::visible())
    {
      return;
    }

    p_->paint();
  }

  //----------------------------------------------------------------
  // kSupersampleText
  //
  static const double kSupersampleText = 2.0;

  //----------------------------------------------------------------
  // Text::TPrivate
  //
  struct Text::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void uncache();
    bool upload(const Text & item);
    void paint(const Text & item);

    QString text_;
    GLuint texId_;
    BoolRef ready_;
  };

  //----------------------------------------------------------------
  // Text::TPrivate::TPrivate
  //
  Text::TPrivate::TPrivate():
    texId_(0)
  {}

  //----------------------------------------------------------------
  // Text::TPrivate::~TPrivate
  //
  Text::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // Text::TPrivate::uncache
  //
  void
  Text::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::upload
  //
  bool
  Text::TPrivate::upload(const Text & item)
  {
    QRectF maxRect;
    item.getMaxRect(maxRect);

    maxRect.setWidth(maxRect.width() * kSupersampleText);
    maxRect.setHeight(maxRect.height() * kSupersampleText);

    BBox bboxContent;
    item.get(kPropertyBBoxContent, bboxContent);

    int iw = (int)ceil(bboxContent.w_ * kSupersampleText);
    int ih = (int)ceil(bboxContent.h_ * kSupersampleText);
    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    QImage img(iw, ih, QImage::Format_ARGB32);
    {
      img.fill(QColor(0x7f, 0x7f, 0x7f, 0));

      QPainter painter(&img);
      QFont font = item.font_;
      double fontSize = item.fontSize_.get();
      font.setPointSizeF(fontSize * kSupersampleText);
      painter.setFont(font);

      // FIXME: this should be a Text property:
      painter.setPen(QColor(0xff, 0xff, 0xff));

      int flags = item.textFlags();
#ifdef NDEBUG
      painter.drawText(maxRect, flags, text_);
#else
      QRectF result;
      painter.drawText(maxRect, flags, text_, &result);

      if (result.width() / kSupersampleText != bboxContent.w_ ||
          result.height() / kSupersampleText != bboxContent.h_)
      {
        YAE_ASSERT(false);

        QFontMetricsF fm(font);
        QRectF v3 = fm.boundingRect(maxRect, flags, text_);

        BBox v2;
        item.calcTextBBox(v2);

        std::cerr
          << "\nfont size: " << fontSize
          << ", text: " << text_.toUtf8().constData()
          << "\nexpected: " << bboxContent.w_ << " x " << bboxContent.h_
          << "\n  result: " << result.width() << " x " << result.height()
          << "\nv2 retry: " << v2.w_ << " x " << v2.h_
          << "\nv3 retry: " << v3.width() << " x " << v3.height()
          << std::endl;
      }
#endif
    }

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    YAE_OGL_11(glGenTextures(1, &texId_));

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId_));
    if (!YAE_OGL_11(glIsTexture(texId_)))
    {
      YAE_ASSERT(false);
      return false;
    }

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_GENERATE_MIPMAP,
                               GL_TRUE));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_WRAP_S,
                               GL_CLAMP_TO_EDGE));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_WRAP_T,
                               GL_CLAMP_TO_EDGE));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_BASE_LEVEL,
                               0));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MAX_LEVEL,
                               0));

    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MAG_FILTER,
                               GL_LINEAR));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MIN_FILTER,
                               GL_LINEAR));
    yae_assert_gl_no_error();

    GLint internalFormat = 0;
    GLenum pixelFormat = 0;
    GLenum dataType = 0;
    GLint shouldSwapBytes = 0;

    yae_to_opengl(yae::kPixelFormatBGRA,
                  internalFormat,
                  pixelFormat,
                  dataType,
                  shouldSwapBytes);

    YAE_OGL_11(glTexImage2D(GL_TEXTURE_2D,
                            0, // mipmap level
                            internalFormat,
                            widthPowerOfTwo,
                            heightPowerOfTwo,
                            0, // border width
                            pixelFormat,
                            dataType,
                            NULL));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SWAP_BYTES,
                             shouldSwapBytes));

    const QImage & constImg = img;
    const unsigned char * data = constImg.bits();
    const int rowSize = constImg.bytesPerLine() / 4;
    const int padding = alignmentFor(data, rowSize);

    YAE_OGL_11(glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint)(padding)));
    YAE_OGL_11(glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint)(rowSize)));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    yae_assert_gl_no_error();

    YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                               0, // mipmap level
                               0, // x-offset
                               0, // y-offset
                               iw,
                               ih,
                               pixelFormat,
                               dataType,
                               data));
    yae_assert_gl_no_error();

    // YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, 0));
    YAE_OGL_11(glDisable(GL_TEXTURE_2D));

    return true;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::paint
  //
  void
  Text::TPrivate::paint(const Text & item)
  {
    BBox bboxContent;
    item.get(kPropertyBBoxContent, bboxContent);

    double x0 = bboxContent.x_;
    double y0 = bboxContent.y_;

    int iw = (int)ceil(bboxContent.w_ * kSupersampleText);
    int ih = (int)ceil(bboxContent.h_ * kSupersampleText);

    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    double u0 = 0.0;
    double u1 = ((bboxContent.w_ * kSupersampleText - 1.0) /
                 double(widthPowerOfTwo));

    double v0 = 0.0;
    double v1 = ((bboxContent.h_ * kSupersampleText - 1.0) /
                 double(heightPowerOfTwo));

    double x1 = x0 + bboxContent.w_;
    double y1 = y0 + bboxContent.h_;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));
    if (glActiveTexture)
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId_));

    YAE_OGL_11(glDisable(GL_LIGHTING));
    YAE_OGL_11(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    YAE_OGL_11(glColor3f(1.f, 1.f, 1.f));
    YAE_OGL_11(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));

    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glTexCoord2d(u0, v0));
      YAE_OGL_11(glVertex2d(x0, y0));

      YAE_OGL_11(glTexCoord2d(u0, v1));
      YAE_OGL_11(glVertex2d(x0, y1));

      YAE_OGL_11(glTexCoord2d(u1, v0));
      YAE_OGL_11(glVertex2d(x1, y0));

      YAE_OGL_11(glTexCoord2d(u1, v1));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());

    // un-bind:
    if (glActiveTexture)
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, 0));
    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
  }


  //----------------------------------------------------------------
  // UploadTexture
  //
  struct UploadTexture : public TBoolExpr
  {
    UploadTexture(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = item_.p_->upload(item_);
    }

    const Text & item_;
  };

  //----------------------------------------------------------------
  // Text::Text
  //
  Text::Text(const char * id):
    Item(id),
    p_(new Text::TPrivate()),
    font_("Impact, Charcoal, sans-serif"),
    alignment_(Qt::AlignLeft),
    elide_(Qt::ElideNone)
  {
    font_.setHintingPreference(QFont::PreferFullHinting);
    font_.setStyleStrategy((QFont::StyleStrategy)
                           (QFont::PreferOutline |
                            QFont::PreferAntialias |
                            QFont::OpenGLCompatible));

    fontSize_ = ItemRef::constant(font_.pointSizeF());
    bboxText_ = addExpr(new CalcTextBBox(this));
    p_->ready_ = addExpr(new UploadTexture(*this));
  }

  //----------------------------------------------------------------
  // Text::~Text
  //
  Text::~Text()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Text::textFlags
  //
  int
  Text::textFlags() const
  {
    Qt::TextFlag textFlags = (elide_ == Qt::ElideNone ?
                              Qt::TextWordWrap :
                              Qt::TextSingleLine);

    int flags = alignment_ | textFlags;
    return flags;
  }

  //----------------------------------------------------------------
  // Text::getConstraints
  //
  void
  Text::getMaxRect(QRectF & maxRect) const
  {
    double maxWidth =
      maxWidth_.isValid() || maxWidth_.isCached() ?
      maxWidth_.get() : double(std::numeric_limits<short int>::max());

    double maxHeight =
      maxHeight_.isValid() || maxHeight_.isCached() ?
      maxHeight_.get() : double(std::numeric_limits<short int>::max());

    maxRect = QRectF(qreal(0), qreal(0), qreal(maxWidth), qreal(maxHeight));
  }

  //----------------------------------------------------------------
  // Text::calcTextBBox
  //
  void
  Text::calcTextBBox(BBox & bbox) const
  {
    int flags = textFlags();

    QRectF maxRect;
    getMaxRect(maxRect);

    QFont font = font_;
    double fontSize = fontSize_.get();
    font.setPointSizeF(fontSize * kSupersampleText);
    QFontMetricsF fm(font);

    p_->text_ = text_.get().toString();
    if (elide_ != Qt::ElideNone)
    {
      p_->text_ = fm.elidedText(p_->text_, elide_, maxRect.width(), flags);
    }

    maxRect.setWidth(maxRect.width() * kSupersampleText);
    maxRect.setHeight(maxRect.height() * kSupersampleText);
    QRectF rect = fm.boundingRect(maxRect, flags, p_->text_);
    bbox.x_ = rect.x() / kSupersampleText;
    bbox.y_ = rect.y() / kSupersampleText;
    bbox.w_ = rect.width() / kSupersampleText;
    bbox.h_ = rect.height() / kSupersampleText;
  }

  //----------------------------------------------------------------
  // Text::calcContentWidth
  //
  double
  Text::calcContentWidth() const
  {
    const BBox & t = bboxText_.get();
    return t.w_;
  }

  //----------------------------------------------------------------
  // Text::calcContentHeight
  //
  double
  Text::calcContentHeight() const
  {
    const BBox & t = bboxText_.get();
    return t.h_;
  }

  //----------------------------------------------------------------
  // Text::uncache
  //
  void
  Text::uncache()
  {
    fontSize_.uncache();
    maxWidth_.uncache();
    bboxText_.uncache();
    text_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Text::paint
  //
  void
  Text::paintContent() const
  {
    if (!Item::visible())
    {
      return;
    }

    if (p_->ready_.get())
    {
      p_->paint(*this);
    }
  }


  //----------------------------------------------------------------
  // Rectangle::Rectangle
  //
  Rectangle::Rectangle(const char * id):
    Item(id),
    radius_(ItemRef::constant(0.0)),
    border_(ItemRef::constant(0.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25)))
  {}

  //----------------------------------------------------------------
  // paintRect
  //
  static void
  paintRect(const BBox & bbox,
            double border,
            const Color & color,
            const Color & colorBorder)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x0, y1));
      YAE_OGL_11(glVertex2d(x1, y0));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());

    if (border > 0.0)
    {
#if 1
      YAE_OGL_11(glColor4ub(colorBorder.r(),
                            colorBorder.g(),
                            colorBorder.b(),
                            colorBorder.a()));
      YAE_OGL_11(glLineWidth(border));
      YAE_OGL_11(glBegin(GL_LINE_LOOP));
      {
        YAE_OGL_11(glVertex2d(x0, y0));
        YAE_OGL_11(glVertex2d(x0, y1));
        YAE_OGL_11(glVertex2d(x1, y1));
        YAE_OGL_11(glVertex2d(x1, y0));
      }
      YAE_OGL_11(glEnd());
#else
#if 0
      double color[16];
      for (double * rgba = color, * end = color + 16; rgba < end; rgba += 4)
      {
        rgba[0] = drand();
        rgba[1] = drand();
        rgba[2] = drand();
        rgba[3] = 0.33;
        /*
        std::cerr
          << "// " << (rgba - color) / 4 << "\n"
          << rgba[0] << ", "
          << rgba[1] << ", "
          << rgba[2] << ", "
          << rgba[3] << ","
          << std::endl;
        */
      }
#else
      double color[16] = {
        1.0, 0.0, 0.0, 0.33,
        0.0, 1.0, 0.0, 0.33,
        1.0, 1.0, 1.0, 0.33,
        0.0, 0.5, 1.0, 0.33
      };
#endif

      YAE_OGL_11(glLineWidth(border));
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
#endif
    }
  }


  //----------------------------------------------------------------
  // paintRect
  //
  static void
  paintRoundedRect(const BBox & bbox,
                   double radius,
                   double border,
                   const Color & color,
                   const Color & colorBorder)
  {
    radius = std::min(radius, 0.5 * std::min(bbox.w_, bbox.h_));
    double r0 = radius - border;

    double cx[2];
    cx[0] = bbox.x_ + bbox.w_ - radius;
    cx[1] = bbox.x_ + radius;

    double cy[2];
    cy[0] = bbox.y_ + radius;
    cy[1] = bbox.y_ + bbox.h_ - radius;

    std::vector<TVec2D> triangleFan;
    std::vector<TVec2D> triangleStrip;

    // start the fan:
    TVec2D center = vec2d(bbox.x_ + 0.5 * bbox.w_,
                          bbox.y_ + 0.5 * bbox.h_);
    triangleFan.push_back(center);

    unsigned int ix[] = { 0, 1, 1, 0 };
    unsigned int iy[] = { 0, 0, 1, 1 };

    unsigned int nsteps = (unsigned int)std::ceil(radius);
    for (unsigned int i = 0; i < 4; i++)
    {
      double ox = cx[ix[i]];
      double oy = cy[iy[i]];

      for (unsigned int j = 0; j <= nsteps; j++)
      {
        double t = double(i * nsteps + j) / double(nsteps * 2);
        double a = M_PI * t;
        double tcos = std::cos(a);
        double tsin = std::sin(a);

        triangleFan.push_back(vec2d(ox + tcos * radius,
                                    oy - tsin * radius));

        triangleStrip.push_back(triangleFan.back());
        triangleStrip.push_back(vec2d(ox + tcos * r0,
                                      oy - tsin * r0));
      }
    }

    // close the loop:
    TVec2D f1 = triangleFan[1];
    TVec2D s1 = triangleStrip[0];
    TVec2D s2 = triangleStrip[1];
    triangleFan.push_back(f1);
    triangleStrip.push_back(s1);
    triangleStrip.push_back(s2);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_TRIANGLE_FAN));
    {
      for (std::vector<TVec2D>::const_iterator i = triangleFan.begin(),
             end = triangleFan.end(); i != end; ++i)
      {
        const TVec2D & v = *i;
        YAE_OGL_11(glVertex2dv(v.coord_));
      }
    }
    YAE_OGL_11(glEnd());

    if (border > 0.0)
    {
      YAE_OGL_11(glColor4ub(colorBorder.r(),
                            colorBorder.g(),
                            colorBorder.b(),
                            colorBorder.a()));
      YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
      {
        for (std::vector<TVec2D>::const_iterator i = triangleStrip.begin(),
               end = triangleStrip.end(); i != end; ++i)
        {
          const TVec2D & v = *i;
          YAE_OGL_11(glVertex2dv(v.coord_));
        }
      }
      YAE_OGL_11(glEnd());
    }
  }

  //----------------------------------------------------------------
  // Rectangle::uncache
  //
  void
  Rectangle::uncache()
  {
    radius_.uncache();
    border_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Rectangle::paint
  //
  void
  Rectangle::paintContent() const
  {
    if (!Item::visible())
    {
      return;
    }

    BBox bbox;
    this->get(kPropertyBBox, bbox);

    double radius = radius_.get();
    double border = border_.get();
    const Color & color = color_.get();
    const Color & colorBorder = colorBorder_.get();

    if (radius > 0.0)
    {
      paintRoundedRect(bbox,
                       radius,
                       border,
                       color,
                       colorBorder);
    }
    else
    {
      paintRect(bbox,
                border,
                color,
                colorBorder);
    }
  }

  //----------------------------------------------------------------
  // Scrollable::Scrollable
  //
  Scrollable::Scrollable(const char * id):
    Item(id),
    content_("content"),
    position_(0.0)
  {}

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
  bool
  Scrollable::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::paint(xregion, yregion))
    {
      return false;
    }

    double sceneHeight = content_.height();
    double viewHeight = this->height();

    const Segment & x = this->x();
    const Segment & y = this->y();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = position_ * range;
    }

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(x.origin_, y.origin_ + dy, 0.0));
    content_.paint(Segment(0.0, x.length_), Segment(dy, y.length_));

    return true;
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
    root_(new Item("playlist")),
    model_(NULL),
    w_(0.0),
    h_(0.0)
  {
    layoutDelegates_[PlaylistModel::kLayoutHintGroupList] =
      TLayoutPtr(new GroupListLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGrid] =
      TLayoutPtr(new ItemGridLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGridCell] =
      TLayoutPtr(new ItemGridCellLayout());
  }

  //----------------------------------------------------------------
  // PlaylistView::resize
  //
  void
  PlaylistView::resizeTo(const Canvas * canvas)
  {
    w_ = canvas->canvasWidth();
    h_ = canvas->canvasHeight();

    Item & root = *root_;
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    TMakeCurrentContext currentContext(*context());
    root.uncache();
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
    YAE_OGL_11(glHint(GL_LINE_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glEnable(GL_POLYGON_SMOOTH));
    YAE_OGL_11(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glLineWidth(1.0));

    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    YAE_OGL_11(glShadeModel(GL_SMOOTH));

    // std::cerr << "\n----------------------------------------------------\n";
    const Segment & xregion = root_->x();
    const Segment & yregion = root_->y();
    root_->paint(xregion, yregion);
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
        et != QEvent::Resize &&
        et != QEvent::MacGLWindowChange &&
        et != QEvent::Leave &&
        et != QEvent::Enter &&
        et != QEvent::WindowDeactivate &&
        et != QEvent::WindowActivate &&
        et != QEvent::FocusOut &&
        et != QEvent::FocusIn &&
        et != QEvent::ShortcutOverride)
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

    TMakeCurrentContext currentContext(*context());
    root_.reset(new Item("playlist"));
    Item & root = *root_;

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    // FIXME:
    root.color_ = 0x01010100;

    delegate->layout(root,
                     layoutDelegates_,
                     *model_,
                     rootIndex);

#if 0 // ndef NDEBUG
    root.dump(std::cerr);
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
