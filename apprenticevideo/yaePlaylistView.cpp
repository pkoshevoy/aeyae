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
  // ItemRef::ItemRef
  //
  ItemRef::ItemRef(const Item * referenceItem,
                   Property property,
                   double offset,
                   double scale):
    item_(referenceItem),
    property_(property),
    offset_(offset),
    scale_(scale),
    visited_(false),
    cached_(false),
    value_(std::numeric_limits<double>::max())
  {}

  //----------------------------------------------------------------
  // ItemRef::uncache
  //
  void
  ItemRef::uncache()
  {
    visited_ = false;
    cached_ = false;
    value_ = std::numeric_limits<double>::max();
  }

  //----------------------------------------------------------------
  // ItemRef::cache
  //
  void
  ItemRef::cache(double value) const
  {
    cached_ = true;
    value_ = value;
  }

  //----------------------------------------------------------------
  // ItemRef::get
  //
  double
  ItemRef::get() const
  {
    if (cached_)
    {
      return value_;
    }

    if (!item_)
    {
      YAE_ASSERT(property_ == ItemRef::kConstant);
      value_ = offset_;
    }
    else if (visited_)
    {
      // item cycle detected:
      YAE_ASSERT(false);
      value_ = std::numeric_limits<double>::max();
    }
    else
    {
      visited_ = true;

      value_ =
        (property_ == ItemRef::kWidth) ? item_->width() :
        (property_ == ItemRef::kHeight) ? item_->height() :
        (property_ == ItemRef::kLeft) ? item_->left() :
        (property_ == ItemRef::kRight) ? item_->right() :
        (property_ == ItemRef::kTop) ? item_->top() :
        (property_ == ItemRef::kBottom) ? item_->bottom() :
        (property_ == ItemRef::kHCenter) ? item_->hcenter() :
        (property_ == ItemRef::kVCenter) ? item_->vcenter() :
        std::numeric_limits<double>::max();

      if (value_ != std::numeric_limits<double>::max())
      {
        value_ = offset_ + scale_ * value_;
      }
      else
      {
        YAE_ASSERT(false);
      }
    }

    cached_ = true;
    return value_;
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
  Anchors::fill(const Item * item, double offset)
  {
    left_ = ItemRef::offset(item, ItemRef::kLeft, offset);
    right_ = ItemRef::offset(item, ItemRef::kRight, -offset);
    top_ = ItemRef::offset(item, ItemRef::kTop, offset);
    bottom_ = ItemRef::offset(item, ItemRef::kBottom, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::center
  //
  void
  Anchors::center(const Item * item)
  {
    hcenter_ = ItemRef::offset(item, ItemRef::kHCenter);
    vcenter_ = ItemRef::offset(item, ItemRef::kVCenter);
  }

  //----------------------------------------------------------------
  // Anchors::topLeft
  //
  void
  Anchors::topLeft(const Item * item, double offset)
  {
    top_ = ItemRef::offset(item, ItemRef::kTop, offset);
    left_ = ItemRef::offset(item, ItemRef::kLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::topRight
  //
  void
  Anchors::topRight(const Item * item, double offset)
  {
    top_ = ItemRef::offset(item, ItemRef::kTop, offset);
    right_ = ItemRef::offset(item, ItemRef::kRight, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomLeft
  //
  void
  Anchors::bottomLeft(const Item * item, double offset)
  {
    bottom_ = ItemRef::offset(item, ItemRef::kBottom, -offset);
    left_ = ItemRef::offset(item, ItemRef::kLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomRight
  //
  void
  Anchors::bottomRight(const Item * item, double offset)
  {
    bottom_ = ItemRef::offset(item, ItemRef::kBottom, -offset);
    right_ = ItemRef::offset(item, ItemRef::kRight, -offset);
  }


  //----------------------------------------------------------------
  // Item::Item
  //
  Item::Item():
    parent_(NULL)
  {}

  //----------------------------------------------------------------
  // Item::calcContentBBox
  //
  void
  Item::calcContentBBox(BBox & bbox) const
  {
    bbox.clear();
  }
#if 1
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
  paintBBox(const BBox & bbox,
            unsigned char r,
            unsigned char g,
            unsigned char b)
  {
    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = bbox.w_ + x0;
    double y1 = bbox.h_ + y0;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glBegin(GL_LINES));
    {
      YAE_OGL_11(glColor3ub(r, g, b));
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x0, y1));
      // YAE_OGL_11(glColor3ub(r, g, b));
      YAE_OGL_11(glVertex2d(x0, y0));
      YAE_OGL_11(glVertex2d(x1, y0));

      // YAE_OGL_11(glColor3ub(r, g, b));
      YAE_OGL_11(glVertex2d(x0, y1));
      YAE_OGL_11(glVertex2d(x1, y1));
      // YAE_OGL_11(glColor3ub(r, g, b));
      YAE_OGL_11(glVertex2d(x1, y0));
      YAE_OGL_11(glVertex2d(x1, y1));
    }
    YAE_OGL_11(glEnd());
  }

  //----------------------------------------------------------------
  // Item::paint
  //
  void
  Item::paint(unsigned char c) const
  {
    paintBBox(bbox_, c, 0, 0);

#if 1
    BBox outerBBox;
    calcOuterBBox(outerBBox);
    paintBBox(outerBBox, 0, c, c);

    BBox innerBBox;
    calcInnerBBox(innerBBox);
    paintBBox(innerBBox, 0, c, 0);
#endif

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      unsigned int c1 = 64 + (((unsigned int)(c) - 64) + 32) % 192;
      child->paint(c1);
    }
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
    g1.anchors_.top_ = ItemRef::offset(&g0, ItemRef::kBottom, 20.0);
    g1.anchors_.left_ = ItemRef::offset(&g0, ItemRef::kRight, 20.0);
    g1.width_ = ItemRef::scale(&g0, ItemRef::kWidth, 0.5);
    g1.height_ = ItemRef::scale(&g0, ItemRef::kHeight, 0.5);
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
    // FIXME: rebuild the view!
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

    TGLSaveMatrixState pushMatrix(GL_PROJECTION);
    YAE_OGL_11(glLoadIdentity());
    YAE_OGL_11(glOrtho(0.0, w, h, 0.0, -1.0, 1.0));

      YAE_OGL_11(glDisable(GL_LIGHTING));
      YAE_OGL_11(glEnable(GL_LINE_SMOOTH));
      YAE_OGL_11(glLineWidth(2.0));

      root_.paint(64);

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
