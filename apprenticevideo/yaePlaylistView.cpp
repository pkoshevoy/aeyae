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
#include <sstream>

// boost library:
#include <boost/chrono.hpp>

// Qt library:
#include <QApplication>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QUrl>
#include <QWheelEvent>

// local interfaces:
#include "yaeCanvasRenderer.h"
#include "yaePlaylistView.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // kDpiScale
  //
#ifdef __APPLE__
  static const double kDpiScale = 1.0;
#else
  static const double kDpiScale = 72.0 / 96.0;
#endif


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

#if 0
      if (ix.model())
      {
        QString v = ix.data().toString();
        oss << " (" << v.toUtf8().constData() << ")";
      }
#endif

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
  // GroupTop
  //
  struct GroupTop : public TDoubleExpr
  {
    GroupTop(const TPlaylistModelItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & groups = item_.parent<Item>();
      unsigned int groupIndex = item_.modelIndex().row();

      if (groupIndex < 1)
      {
        result = groups.top();
      }
      else
      {
        Item & prevGroup = *(groups.children_[groupIndex - 1]);
        result = prevGroup.bottom();
      }
    }

    const TPlaylistModelItem & item_;
  };

  //----------------------------------------------------------------
  // GridCellLeft
  //
  struct GridCellLeft : public TDoubleExpr
  {
    GridCellLeft(const TPlaylistModelItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & grid = item_.parent<Item>();
      double gridWidth = grid.width();
      unsigned int cellsPerRow = calcItemsPerRow(gridWidth);
      unsigned int cellIndex = item_.modelIndex().row();
      std::size_t cellCol = cellIndex % cellsPerRow;
      double ox = grid.left() + 2;
      result = ox + gridWidth * double(cellCol) / double(cellsPerRow);
    }

    const TPlaylistModelItem & item_;
  };

  //----------------------------------------------------------------
  // GridCellTop
  //
  struct GridCellTop : public TDoubleExpr
  {
    GridCellTop(const TPlaylistModelItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      Item & grid = item_.parent<Item>();
      std::size_t numCells = grid.children_.size();
      double gridWidth = grid.width();
      double cellWidth = calcCellWidth(gridWidth);
      double cellHeight = cellWidth; // calcCellHeight(cellWidth);
      unsigned int cellsPerRow = calcItemsPerRow(gridWidth);
      unsigned int rowsOfCells = calcRows(gridWidth, cellWidth, numCells);
      double gridHeight = cellHeight * double(rowsOfCells);
      unsigned int cellIndex = item_.modelIndex().row();
      std::size_t cellRow = cellIndex / cellsPerRow;
      double oy = grid.top() + 2;
      result = oy + gridHeight * double(cellRow) / double(rowsOfCells);
    }

    const TPlaylistModelItem & item_;
  };

  //----------------------------------------------------------------
  // GridCellWidth
  //
  struct GridCellWidth : public TDoubleExpr
  {
    GridCellWidth(const Item & grid):
      grid_(grid)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double gridWidth = grid_.width();
      result = calcCellWidth(gridWidth) - 2;
    }

    const Item & grid_;
  };

  //----------------------------------------------------------------
  // GridCellHeight
  //
  struct GridCellHeight : public TDoubleExpr
  {
    GridCellHeight(const Item & grid):
      grid_(grid)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double gridWidth = grid_.width();
      double cellWidth = calcCellWidth(gridWidth);
      result = cellWidth - 2; // calcCellHeight(cellWidth) - 2;
    }

    const Item & grid_;
  };

  //----------------------------------------------------------------
  // CalcTitleHeight
  //
  struct CalcTitleHeight : public TDoubleExpr
  {
    CalcTitleHeight(const Item & titleContainer, double minHeight):
      titleContainer_(titleContainer),
      minHeight_(minHeight)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double titleContainerWidth = titleContainer_.width();
      result = calcTitleHeight(minHeight_, titleContainerWidth);
    }

    const Item & titleContainer_;
    double minHeight_;
  };

  //----------------------------------------------------------------
  // GetFontSize
  //
  struct GetFontSize : public TDoubleExpr
  {
    GetFontSize(const Item & titleHeight, double titleHeightScale,
                const Item & cellHeight, double cellHeightScale):
      titleHeight_(titleHeight),
      cellHeight_(cellHeight),
      titleHeightScale_(titleHeightScale),
      cellHeightScale_(cellHeightScale)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double t = 0.0;
      titleHeight_.get(kPropertyHeight, t);
      t *= titleHeightScale_;

      double c = 0.0;
      cellHeight_.get(kPropertyHeight, c);
      c *= cellHeightScale_;

      result = std::min(t, c);
    }

    const Item & titleHeight_;
    const Item & cellHeight_;

    double titleHeightScale_;
    double cellHeightScale_;
  };


  //----------------------------------------------------------------
  // PlaylistFooter
  //
  struct PlaylistFooter : public TVarExpr
  {
    PlaylistFooter(const PlaylistModelProxy & model):
      model_(model)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      quint64 n = model_.itemCount();
      QString t = (n == 1) ?
        QObject::tr("1 item, end of playlist") :
        QObject::tr("%1 items, end of playlist").arg(n);

      result = QVariant(t);
    }

    const PlaylistModelProxy & model_;
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
    QPersistentModelIndex index_;
    int role_;
  };

  //----------------------------------------------------------------
  // TModelQuery
  //
  template <typename TData>
  struct TModelQuery : public Expression<TData>
  {
    TModelQuery(const PlaylistModelProxy & model,
                const QModelIndex & index,
                int role):
      model_(model),
      index_(index),
      role_(role)
    {}

    // virtual:
    void evaluate(TData & result) const
    {
      QVariant v = model_.data(index_, role_);

      if (!v.canConvert<TData>())
      {
        YAE_ASSERT(false);
        throw std::runtime_error("unexpected model data type");
      }

      result = v.value<TData>();
    }

    const PlaylistModelProxy & model_;
    QPersistentModelIndex index_;
    int role_;
  };

  //----------------------------------------------------------------
  // TQueryBool
  //
  typedef TModelQuery<bool> TQueryBool;

  //----------------------------------------------------------------
  // QueryBoolInverse
  //
  struct QueryBoolInverse : public TQueryBool
  {
    QueryBoolInverse(const PlaylistModelProxy & model,
                     const QModelIndex & index,
                     int role):
      TQueryBool(model, index, role)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      bool inverseResult = false;
      TQueryBool::evaluate(inverseResult);
      result = !inverseResult;
    }
  };

  //----------------------------------------------------------------
  // IsModelSortedBy
  //
  struct IsModelSortedBy : public TBoolExpr
  {
    IsModelSortedBy(const PlaylistModelProxy & model,
                    PlaylistModelProxy::SortBy sortBy):
      model_(model),
      sortBy_(sortBy)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      PlaylistModelProxy::SortBy modelSortedBy = model_.sortBy();
      result = (modelSortedBy == sortBy_);
    }

    const PlaylistModelProxy & model_;
    PlaylistModelProxy::SortBy sortBy_;
  };

  //----------------------------------------------------------------
  // IsModelSortOrder
  //
  struct IsModelSortOrder : public TBoolExpr
  {
    IsModelSortOrder(const PlaylistModelProxy & model,
                     Qt::SortOrder sortOrder):
      model_(model),
      sortOrder_(sortOrder)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      Qt::SortOrder modelSortOrder = model_.sortOrder();
      result = (modelSortOrder == sortOrder_);
    }

    const PlaylistModelProxy & model_;
    Qt::SortOrder sortOrder_;
  };

  //----------------------------------------------------------------
  // xbuttonImage
  //
  static QImage
  xbuttonImage(unsigned int w, const Color & color)
  {
    QImage img(w, w, QImage::Format_ARGB32);

    // supersample each pixel:
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };

    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    int w2 = w / 2;
    double diameter = double(w);
    double center = diameter * 0.5;
    Segment sa(-center, diameter);
    Segment sb(-diameter * 0.1, diameter * 0.2);

    TVec2D origin(0.0, 0.0);
    TVec2D u_axis(0.707106781186548, 0.707106781186548);
    TVec2D v_axis(-0.707106781186548, 0.707106781186548);

    Vec<double, 4> outerColor(Color(0, 0.0));
    Vec<double, 4> innerColor(color);
    TVec2D samplePoint;

    for (int y = 0; y < int(w); y++)
    {
      unsigned char * dst = img.scanLine(y);
      samplePoint.set_y(double(y - w2));

      for (int x = 0; x < int(w); x++, dst += 4)
      {
        samplePoint.set_x(double(x - w2));

        double outer = 0.0;
        double inner = 0.0;

        for (unsigned int k = 0; k < supersample; k++)
        {
          TVec2D wcs_pt = samplePoint + sp[k];
          TVec2D pt = wcs_to_lcs(origin, u_axis, v_axis, wcs_pt);
          double oh = sa.pixelOverlap(pt.x()) * sb.pixelOverlap(pt.y());
          double ov = sb.pixelOverlap(pt.x()) * sa.pixelOverlap(pt.y());
          double innerOverlap = std::max<double>(oh, ov);
          double outerOverlap = 1.0 - innerOverlap;

          outer += outerOverlap;
          inner += innerOverlap;
        }

        double outerWeight = outer / double(supersample);
        double innerWeight = inner / double(supersample);
        Color c(outerColor * outerWeight + innerColor * innerWeight);
        memcpy(dst, &(c.argb_), sizeof(c.argb_));
      }
    }

    return img;
  }



  //----------------------------------------------------------------
  // TLayoutPtr
  //
  typedef PlaylistView::TLayoutPtr TLayoutPtr;

  //----------------------------------------------------------------
  // TLayoutHint
  //
  typedef PlaylistModel::LayoutHint TLayoutHint;

  //----------------------------------------------------------------
  // findLayoutDelegate
  //
  static TLayoutPtr
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
  static TLayoutPtr
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
  // findLayoutDelegate
  //
  static TLayoutPtr
  findLayoutDelegate(const PlaylistView & view,
                     const PlaylistModelProxy & model,
                     const QModelIndex & modelIndex)
  {
    return findLayoutDelegate(view.layouts(), model, modelIndex);
  }

  //----------------------------------------------------------------
  // SetSortBy
  //
  struct SetSortBy : public InputArea
  {
    SetSortBy(const char * id,
              PlaylistView & view,
              PlaylistModelProxy & model,
              Item & filter,
              PlaylistModelProxy::SortBy sortBy):
      InputArea(id),
      view_(view),
      model_(model),
      filter_(filter),
      sortBy_(sortBy)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.delegate()->requestRepaint();
      filter_.uncache();
      model_.setSortBy(sortBy_);
      return true;
    }

    PlaylistView & view_;
    PlaylistModelProxy & model_;
    Item & filter_;
    PlaylistModelProxy::SortBy sortBy_;
  };

  //----------------------------------------------------------------
  // SetSortOrder
  //
  struct SetSortOrder : public InputArea
  {
    SetSortOrder(const char * id,
                 PlaylistView & view,
                 PlaylistModelProxy & model,
                 Item & filter,
                 Qt::SortOrder sortOrder):
      InputArea(id),
      view_(view),
      model_(model),
      filter_(filter),
      sortOrder_(sortOrder)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.delegate()->requestRepaint();
      filter_.uncache();
      model_.setSortOrder(sortOrder_);
      return true;
    }

    PlaylistView & view_;
    PlaylistModelProxy & model_;
    Item & filter_;
    Qt::SortOrder sortOrder_;
  };

  //----------------------------------------------------------------
  // layoutFilterItem
  //
  static void
  layoutFilterItem(Item & item,
                   PlaylistView & view,
                   PlaylistModelProxy & model,
                   const QModelIndex & itemIndex)
  {
    // reuse pre-computed properties:
    const Item & playlist = *(view.root());
    const Item & fontSize = playlist["font_size"];

    Gradient & filterShadow = item.addNew<Gradient>("filterShadow");
    filterShadow.anchors_.fill(item);
    filterShadow.anchors_.bottom_.reset();
    filterShadow.height_ = ItemRef::reference(item, kPropertyHeight);
    filterShadow.color_[0.0] = Color(0x1f1f1f, 1.0);
    filterShadow.color_[0.42] = Color(0x1f1f1f, 0.9);
    filterShadow.color_[1.0] = Color(0x1f1f1f, 0.0);

    RoundRect & filter = item.addNew<RoundRect>("bg");
    filter.anchors_.fill(item, 2);
    filter.anchors_.bottom_.reset();
    filter.height_ = ItemRef::scale(item, kPropertyHeight, 0.333);
    filter.radius_ = ItemRef::scale(filter, kPropertyHeight, 0.1);

    Item & icon = filter.addNew<Item>("filter_icon");
    {
      RoundRect & circle = icon.addNew<RoundRect>("circle");
      circle.anchors_.hcenter_ = ItemRef::scale(filter, kPropertyHeight, 0.5);
      circle.anchors_.vcenter_ = ItemRef::reference(filter, kPropertyVCenter);
      circle.width_ = ItemRef::scale(filter, kPropertyHeight, 0.5);
      circle.height_ = circle.width_;
      circle.radius_ = ItemRef::scale(circle, kPropertyWidth, 0.5);
      circle.border_ = ItemRef::scale(circle, kPropertyWidth, 0.1);

      Transform & xform = icon.addNew<Transform>("xform");
      xform.anchors_.hcenter_ = circle.anchors_.hcenter_;
      xform.anchors_.vcenter_ = circle.anchors_.vcenter_;
      xform.rotation_ = ItemRef::constant(M_PI * 0.25);

      Rectangle & handle = xform.addNew<Rectangle>("handle");
      handle.anchors_.left_ = ItemRef::scale(filter, kPropertyHeight, 0.25);
      handle.anchors_.vcenter_ = ItemRef::constant(0.0);
      handle.width_ = handle.anchors_.left_;
      handle.height_ = ItemRef::scale(handle, kPropertyWidth, 0.25);

      icon.anchors_.left_ = ItemRef::reference(circle, kPropertyLeft);
      icon.anchors_.top_ = ItemRef::reference(circle, kPropertyTop);
    }

    // FIXME: this should be a text edit item:
    Text & text = filter.addNew<Text>("filter_text");
    text.anchors_.vcenter_ = ItemRef::reference(filter, kPropertyVCenter);
    text.anchors_.left_ = ItemRef::reference(icon, kPropertyRight);
    text.anchors_.right_ = ItemRef::reference(filter, kPropertyRight);
    text.elide_ = Qt::ElideLeft;
    text.color_ = ColorRef::constant(Color(0xffffff, 0.25));
    text.text_ = TVarRef::constant(TVar(QObject::tr("SEARCH AND FILTER")));
    text.fontSize_ =
      ItemRef::scale(fontSize, kPropertyHeight, 1.07 * kDpiScale);
#if 0
    text.font_ = QFont("Sans Serif");
    text.font_.setBold(true);
    // text.margins_.top_ = text.addExpr(new GetFontDescent(text), 0, 1);
#endif

    // layout sort-and-order:
    ColorRef underlineColor = ColorRef::constant(Color(0xff0000, 1.0));
    ColorRef sortColor = ColorRef::constant(Color(0xffffff, 0.5));
    ItemRef smallFontSize = ItemRef::scale(fontSize,
                                           kPropertyHeight,
                                           0.7 * kDpiScale);
    QFont smallFont;
    smallFont.setBold(true);

    Item & sortAndOrder = item.addNew<Item>("sort_and_order");
    sortAndOrder.anchors_.top_ = ItemRef::reference(filter, kPropertyBottom);
    sortAndOrder.anchors_.left_ = ItemRef::reference(filter, kPropertyLeft);
    sortAndOrder.margins_.left_ = ItemRef::scale(icon, kPropertyWidth, 0.25);

    Rectangle & ulName = sortAndOrder.addNew<Rectangle>("underline_name");
    Rectangle & ulTime = sortAndOrder.addNew<Rectangle>("underline_time");
    Rectangle & ulAsc = sortAndOrder.addNew<Rectangle>("underline_asc");
    Rectangle & ulDesc = sortAndOrder.addNew<Rectangle>("underline_desc");

    Text & sortBy = sortAndOrder.addNew<Text>("sort_by");
    sortBy.anchors_.left_ = ItemRef::reference(sortAndOrder, kPropertyLeft);
    sortBy.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    sortBy.text_ = TVarRef::constant(TVar(QObject::tr("sort by ")));
    sortBy.color_ = sortColor;
    sortBy.font_ = smallFont;
    sortBy.fontSize_ = smallFontSize;

    Text & byName = sortAndOrder.addNew<Text>("by_name");
    byName.anchors_.left_ = ItemRef::reference(sortBy, kPropertyRight);
    byName.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    byName.text_ = TVarRef::constant(TVar(QObject::tr("name")));
    byName.color_ = sortColor;
    byName.font_ = smallFont;
    byName.fontSize_ = smallFontSize;

    Text & nameOr = sortAndOrder.addNew<Text>("name_or");
    nameOr.anchors_.left_ = ItemRef::reference(byName, kPropertyRight);
    nameOr.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    nameOr.text_ = TVarRef::constant(TVar(QObject::tr(" or ")));
    nameOr.color_ = sortColor;
    nameOr.font_ = smallFont;
    nameOr.fontSize_ = smallFontSize;

    Text & orTime = sortAndOrder.addNew<Text>("or_time");
    orTime.anchors_.left_ = ItemRef::reference(nameOr, kPropertyRight);
    orTime.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    orTime.text_ = TVarRef::constant(TVar(QObject::tr("time")));
    orTime.color_ = sortColor;
    orTime.font_ = smallFont;
    orTime.fontSize_ = smallFontSize;

    Text & comma = sortAndOrder.addNew<Text>("comma");
    comma.anchors_.left_ = ItemRef::reference(orTime, kPropertyRight);
    comma.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    comma.text_ = TVarRef::constant(TVar(QObject::tr(", in ")));
    comma.color_ = sortColor;
    comma.font_ = smallFont;
    comma.fontSize_ = smallFontSize;

    Text & inAsc = sortAndOrder.addNew<Text>("in_asc");
    inAsc.anchors_.left_ = ItemRef::reference(comma, kPropertyRight);
    inAsc.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    inAsc.text_ = TVarRef::constant(TVar(QObject::tr("ascending")));
    inAsc.color_ = sortColor;
    inAsc.font_ = smallFont;
    inAsc.fontSize_ = smallFontSize;

    Text & ascOr = sortAndOrder.addNew<Text>("asc_or");
    ascOr.anchors_.left_ = ItemRef::reference(inAsc, kPropertyRight);
    ascOr.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    ascOr.text_ = TVarRef::constant(TVar(QObject::tr(" or ")));
    ascOr.color_ = sortColor;
    ascOr.font_ = smallFont;
    ascOr.fontSize_ = smallFontSize;

    Text & orDesc = sortAndOrder.addNew<Text>("or_desc");
    orDesc.anchors_.left_ = ItemRef::reference(ascOr, kPropertyRight);
    orDesc.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    orDesc.text_ = TVarRef::constant(TVar(QObject::tr("descending")));
    orDesc.color_ = sortColor;
    orDesc.font_ = smallFont;
    orDesc.fontSize_ = smallFontSize;

    Text & order = sortAndOrder.addNew<Text>("order");
    order.anchors_.left_ = ItemRef::reference(orDesc, kPropertyRight);
    order.anchors_.top_ = ItemRef::reference(sortAndOrder, kPropertyTop);
    order.text_ = TVarRef::constant(TVar(QObject::tr(" order")));
    order.color_ = sortColor;
    order.font_ = smallFont;
    order.fontSize_ = smallFontSize;

    ulName.anchors_.left_ = ItemRef::offset(byName, kPropertyLeft, -1);
    ulName.anchors_.right_ = ItemRef::offset(byName, kPropertyRight, 1);
    ulName.anchors_.top_ = ItemRef::offset(byName, kPropertyBottom, 0);
    ulName.height_ = ItemRef::constant(2);
    ulName.color_ = underlineColor;
    ulName.visible_ = ulName.
      addExpr(new IsModelSortedBy(model, PlaylistModelProxy::SortByName));

    ulTime.anchors_.left_ = ItemRef::offset(orTime, kPropertyLeft, -1);
    ulTime.anchors_.right_ = ItemRef::offset(orTime, kPropertyRight, 1);
    ulTime.anchors_.top_ = ItemRef::offset(orTime, kPropertyBottom, 0);
    ulTime.height_ = ItemRef::constant(2);
    ulTime.color_ = underlineColor;
    ulTime.visible_ = ulTime.
      addExpr(new IsModelSortedBy(model, PlaylistModelProxy::SortByTime));

    ulAsc.anchors_.left_ = ItemRef::offset(inAsc, kPropertyLeft, -1);
    ulAsc.anchors_.right_ = ItemRef::offset(inAsc, kPropertyRight, 1);
    ulAsc.anchors_.top_ = ItemRef::offset(inAsc, kPropertyBottom, 0);
    ulAsc.height_ = ItemRef::constant(2);
    ulAsc.color_ = underlineColor;
    ulAsc.visible_ = ulAsc.
      addExpr(new IsModelSortOrder(model, Qt::AscendingOrder));

    ulDesc.anchors_.left_ = ItemRef::offset(orDesc, kPropertyLeft, -1);
    ulDesc.anchors_.right_ = ItemRef::offset(orDesc, kPropertyRight, 1);
    ulDesc.anchors_.top_ = ItemRef::offset(orDesc, kPropertyBottom, 0);
    ulDesc.height_ = ItemRef::constant(2);
    ulDesc.color_ = underlineColor;
    ulDesc.visible_ = ulDesc.
      addExpr(new IsModelSortOrder(model, Qt::DescendingOrder));

    SetSortBy & sortByName = item.
      add(new SetSortBy("ma_sort_by_name", view, model, item,
                        PlaylistModelProxy::SortByName));
    sortByName.anchors_.fill(byName);

    SetSortBy & sortByTime = item.
      add(new SetSortBy("ma_sort_by_time", view, model, item,
                        PlaylistModelProxy::SortByTime));
    sortByTime.anchors_.fill(orTime);

    SetSortOrder & sortOrderAsc = item.
      add(new SetSortOrder("ma_order_asc", view, model, item,
                           Qt::AscendingOrder));
    sortOrderAsc.anchors_.fill(inAsc);

    SetSortOrder & sortOrderDesc = item.
      add(new SetSortOrder("ma_order_desc", view, model, item,
                           Qt::DescendingOrder));
    sortOrderDesc.anchors_.fill(orDesc);
  }

  //----------------------------------------------------------------
  // GroupListLayout
  //
  struct GroupListLayout : public PlaylistView::TLayoutDelegate
  {
    void layout(Item & root,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & rootIndex)
    {
      Item & playlist = *(view.root());

      // setup an invisible item so its height property expression
      // could be computed once and the result reused in other places
      // that need to compute the same property expression:
      Item & titleHeight = playlist.addNewHidden<Item>("title_height");
      titleHeight.height_ =
        titleHeight.addExpr(new CalcTitleHeight(root, 24.0));

      // generate an x-button texture:
      {
        QImage img = xbuttonImage(32, Color(0xffffff, 0.5));
        playlist.addHidden<Texture>(new Texture("xbutton_texture", img));
      }

      Rectangle & background = root.addNew<Rectangle>("background");
      background.anchors_.fill(root);
      background.color_ = ColorRef::constant(Color(0x1f1f1f, 0.87));

      Scrollview & sview = root.addNew<Scrollview>("scrollview");

      Item & filterItem = root.addNew<Item>("filterItem");
      filterItem.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      filterItem.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      filterItem.width_ = ItemRef::reference(root, kPropertyWidth);
      filterItem.height_ = ItemRef::scale(titleHeight, kPropertyHeight, 4.5);


      Item & scrollbar = root.addNew<Item>("scrollbar");
      scrollbar.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      scrollbar.anchors_.top_ = ItemRef::reference(sview, kPropertyTop);
      scrollbar.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom, -5);
      scrollbar.width_ =
        scrollbar.addExpr(new CalcTitleHeight(root, 50.0), 0.2);

      sview.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      sview.anchors_.right_ = ItemRef::reference(scrollbar, kPropertyLeft);
      sview.anchors_.top_ = ItemRef::reference(filterItem, kPropertyBottom);
      sview.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);
      sview.margins_.top_ = ItemRef::scale(filterItem, kPropertyHeight, -0.45);

      sview.content_.anchors_.left_ = ItemRef::constant(0.0);
      sview.content_.anchors_.top_ = ItemRef::constant(0.0);
      sview.content_.width_ = ItemRef::reference(sview, kPropertyWidth);

      Item & groups = sview.content_.addNew<Item>("groups");
      groups.anchors_.fill(sview.content_);
      groups.anchors_.bottom_.reset();

      Item & cellWidth = playlist.addNewHidden<Item>("cell_width");
      cellWidth.width_ = cellWidth.addExpr(new GridCellWidth(groups));

      Item & cellHeight = playlist.addNewHidden<Item>("cell_height");
      cellHeight.height_ = cellHeight.addExpr(new GridCellHeight(groups));

      Item & fontSize = playlist.addNewHidden<Item>("font_size");
      fontSize.height_ = fontSize.addExpr(new GetFontSize(titleHeight, 0.52,
                                                          cellHeight, 0.15));

      layoutFilterItem(filterItem, view, model, rootIndex);

      Text & nowPlaying = playlist.addNewHidden<Text>("now_playing");
      nowPlaying.anchors_.top_ = ItemRef::constant(0.0);
      nowPlaying.anchors_.left_ = ItemRef::constant(0.0);
      nowPlaying.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
      nowPlaying.font_.setBold(false);
      nowPlaying.fontSize_ = ItemRef::scale(fontSize,
                                            kPropertyHeight,
                                            0.8 * kDpiScale);

      const int numGroups = model.rowCount(rootIndex);
      for (int i = 0; i < numGroups; i++)
      {
        QModelIndex childIndex = model.index(i, 0, rootIndex);
        TPlaylistModelItem & group = groups.
          add(new TPlaylistModelItem("group", childIndex));
        group.anchors_.left_ = ItemRef::reference(groups, kPropertyLeft);
        group.anchors_.right_ = ItemRef::reference(groups, kPropertyRight);
        group.anchors_.top_ = group.addExpr(new GroupTop(group));

        TLayoutPtr childLayout = findLayoutDelegate(view, model, childIndex);
        if (childLayout)
        {
          childLayout->layout(group, view, model, childIndex);
        }
      }

      // add a footer:
      {
        Item & footer = sview.content_.addNew<Item>("footer");
        footer.anchors_.left_ =
          ItemRef::offset(sview.content_, kPropertyLeft, 2);
        footer.anchors_.right_ =
          ItemRef::reference(sview.content_, kPropertyRight);
        footer.anchors_.top_ = ItemRef::reference(groups, kPropertyBottom);
        footer.height_ = ItemRef::reference(titleHeight, kPropertyHeight);

        Rectangle & separator = footer.addNew<Rectangle>("footer_separator");
        separator.anchors_.fill(footer);
        separator.anchors_.bottom_.reset();
        separator.height_ = ItemRef::constant(2.0);

        QFont smallFont;
        smallFont.setBold(true);
        ItemRef smallFontSize = ItemRef::scale(fontSize,
                                               kPropertyHeight,
                                               0.7 * kDpiScale);

        Text & footNote = footer.addNew<Text>("footNote");
        footNote.anchors_.vcenter_ =
          ItemRef::reference(footer, kPropertyVCenter);
        footNote.anchors_.right_ =
          ItemRef::reference(footer, kPropertyRight);
        footNote.margins_.right_ =
          ItemRef::scale(fontSize, kPropertyHeight, 0.8 * kDpiScale);

        footNote.text_ = footNote.addExpr(new PlaylistFooter(model));
        footNote.font_ = smallFont;
        footNote.fontSize_ = smallFontSize;
        footNote.color_ = ColorRef::constant(Color(0xffffff, 0.5));
     }

      FlickableArea & maScrollview =
        sview.add(new FlickableArea("ma_sview", view, scrollbar));
      maScrollview.anchors_.fill(sview);

      InputArea & maScrollbar = scrollbar.addNew<InputArea>("ma_scrollbar");
      maScrollbar.anchors_.fill(scrollbar);

      // configure scrollbar slider:
      RoundRect & slider = scrollbar.addNew<RoundRect>("slider");
      slider.anchors_.top_ = slider.addExpr(new CalcSliderTop(sview, slider));
      slider.anchors_.left_ = ItemRef::offset(scrollbar, kPropertyLeft, 2);
      slider.anchors_.right_ = ItemRef::offset(scrollbar, kPropertyRight, -2);
      slider.height_ = slider.addExpr(new CalcSliderHeight(sview, slider));
      slider.radius_ = ItemRef::scale(slider, kPropertyWidth, 0.5);

      SliderDrag & maSlider =
        slider.add(new SliderDrag("ma_slider", view, sview, scrollbar));
      maSlider.anchors_.fill(slider);
    }
  };

  //----------------------------------------------------------------
  // GroupCollapse
  //
  struct GroupCollapse : public TClickablePlaylistModelItem
  {
    GroupCollapse(const char * id, PlaylistView & view):
      TClickablePlaylistModelItem(id),
      view_(view)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      TClickablePlaylistModelItem::TModel & model = this->model();
      const QPersistentModelIndex & modelIndex = this->modelIndex();
      int role = PlaylistModel::kRoleCollapsed;
      bool collapsed = modelIndex.data(role).toBool();
      model.setData(modelIndex, QVariant(!collapsed), role);
      view_.requestRepaint();
      return true;
    }

    PlaylistView & view_;
  };

  //----------------------------------------------------------------
  // RemoveModelItems
  //
  struct RemoveModelItems : public TClickablePlaylistModelItem
  {
    RemoveModelItems(const char * id):
      TClickablePlaylistModelItem(id)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      TClickablePlaylistModelItem::TModel & model = this->model();
      const QPersistentModelIndex & modelIndex = this->modelIndex();
      model.removeItems(modelIndex);
      return true;
    }
  };

  //----------------------------------------------------------------
  // ItemPlay
  //
  struct ItemPlay : public TClickablePlaylistModelItem
  {
    ItemPlay(const char * id):
      TClickablePlaylistModelItem(id)
    {}

    // virtual:
    bool onDoubleClick(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysPoint)
    {
      const QPersistentModelIndex & modelIndex = this->modelIndex();
      TClickablePlaylistModelItem::TModel & model = this->model();
      model.setPlayingItem(modelIndex);
      return true;
    }
  };

  //----------------------------------------------------------------
  // ItemGridLayout
  //
  struct ItemGridLayout : public PlaylistView::TLayoutDelegate
  {
    void layout(Item & group,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & groupIndex)
    {
      // reuse pre-computed properties:
      const Item & playlist = *(view.root());
      const Item & fontSize = playlist["font_size"];
      const Item & cellWidth = playlist["cell_width"];
      const Item & cellHeight = playlist["cell_height"];
      const Item & titleHeight = playlist["title_height"];

      const Text & nowPlaying =
        dynamic_cast<const Text &>(playlist["now_playing"]);

      const Texture & xbuttonTexture =
        dynamic_cast<const Texture &>(playlist["xbutton_texture"]);

      Item & spacer = group.addNew<Item>("spacer");
      spacer.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      spacer.anchors_.top_ = ItemRef::reference(group, kPropertyTop);
      spacer.width_ = ItemRef::reference(group, kPropertyWidth);
      spacer.height_ = ItemRef::scale(titleHeight, kPropertyHeight, 0.2);

      Item & title = group.addNew<Item>("title");
      title.anchors_.top_ = ItemRef::offset(spacer, kPropertyBottom, 5);
      title.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      title.anchors_.right_ = ItemRef::reference(group, kPropertyRight);

      Item & chevron = title.addNew<Item>("chevron");
      Triangle & collapsed = chevron.addNew<Triangle>("collapse");
      Text & text = title.addNew<Text>("text");
      Item & rm = title.addNew<Item>("rm");
      TexturedRect & xbutton =
        rm.add<TexturedRect>(new TexturedRect("xbutton", xbuttonTexture));
      ItemRef fontDescent =
        xbutton.addExpr(new GetFontDescent(text));
      ItemRef fontDescentNowPlaying =
        xbutton.addExpr(new GetFontDescent(nowPlaying));

      // open/close disclosure [>] button:
      chevron.width_ = ItemRef::reference(text, kPropertyHeight);
      chevron.height_ = ItemRef::reference(text, kPropertyHeight);
      chevron.anchors_.top_ = ItemRef::reference(text, kPropertyTop);
      chevron.anchors_.left_ = ItemRef::offset(title, kPropertyLeft);

      collapsed.anchors_.fill(chevron);
      collapsed.margins_.set(fontDescent);
      collapsed.collapsed_ = collapsed.addExpr
        (new TQueryBool(model, groupIndex, PlaylistModel::kRoleCollapsed));

      text.anchors_.top_ = ItemRef::reference(title, kPropertyTop);
      text.anchors_.left_ = ItemRef::reference(chevron, kPropertyRight);
      text.anchors_.right_ = ItemRef::reference(rm, kPropertyLeft);
      text.text_ = text.addExpr
        (new ModelQuery(model, groupIndex, PlaylistModel::kRoleLabel));
      text.fontSize_ =
        ItemRef::scale(fontSize, kPropertyHeight, 1.07 * kDpiScale);
      text.elide_ = Qt::ElideMiddle;

      // remove group [x] button:
      rm.width_ = ItemRef::reference(nowPlaying, kPropertyHeight);
      rm.height_ = ItemRef::reference(text, kPropertyHeight);
      rm.anchors_.top_ = ItemRef::reference(text, kPropertyTop);
      rm.anchors_.right_ = ItemRef::offset(title, kPropertyRight, -5);

      xbutton.anchors_.center(rm);
      xbutton.margins_.set(fontDescentNowPlaying);
      xbutton.width_ = xbutton.addExpr(new InscribedCircleDiameterFor(rm));
      xbutton.height_ = xbutton.width_;

      Rectangle & separator = group.addNew<Rectangle>("separator");
      separator.anchors_.top_ = ItemRef::offset(title, kPropertyBottom, 5);
      separator.anchors_.left_ = ItemRef::offset(group, kPropertyLeft, 2);
      separator.anchors_.right_ = ItemRef::reference(group, kPropertyRight);
      separator.height_ = ItemRef::constant(2.0);

      Item & payload = group.addNew<Item>("payload");
      payload.anchors_.top_ = ItemRef::reference(separator, kPropertyBottom);
      payload.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      payload.anchors_.right_ = ItemRef::reference(group, kPropertyRight);
      payload.visible_ = payload.addExpr
        (new QueryBoolInverse(model,
                              groupIndex,
                              PlaylistModel::kRoleCollapsed));
      payload.height_ = payload.addExpr(new InvisibleItemZeroHeight(payload));

      GroupCollapse & maCollapse = collapsed.
        add(new GroupCollapse("ma_collapse", view));
      maCollapse.anchors_.fill(collapsed);

      RemoveModelItems & maRmGroup = xbutton.
        add(new RemoveModelItems("ma_remove_group"));
      maRmGroup.anchors_.fill(xbutton);

      Item & grid = payload.addNew<Item>("grid");
      grid.anchors_.top_ = ItemRef::reference(separator, kPropertyBottom);
      grid.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      grid.anchors_.right_ = ItemRef::reference(group, kPropertyRight);

      const int numCells = model.rowCount(groupIndex);
      for (int i = 0; i < numCells; i++)
      {
        QModelIndex childIndex = model.index(i, 0, groupIndex);
        TPlaylistModelItem & cell = grid.
          add(new TPlaylistModelItem("cell", childIndex));
        cell.anchors_.left_ = cell.addExpr(new GridCellLeft(cell));
        cell.anchors_.top_ = cell.addExpr(new GridCellTop(cell));
        cell.width_ = ItemRef::reference(cellWidth, kPropertyWidth);
        cell.height_ = ItemRef::reference(cellHeight, kPropertyHeight);

        ItemPlay & maPlay = cell.add(new ItemPlay("ma_cell"));
        maPlay.anchors_.fill(cell);

        TLayoutPtr childLayout = findLayoutDelegate(view, model, childIndex);
        if (childLayout)
        {
          childLayout->layout(cell, view, model, childIndex);
        }
      }

      Item & footer = payload.addNew<Item>("footer");
      footer.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      footer.anchors_.top_ = ItemRef::reference(grid, kPropertyBottom);
      footer.width_ = ItemRef::reference(group, kPropertyWidth);
      footer.height_ = ItemRef::reference(titleHeight, kPropertyHeight);
    }
  };

  //----------------------------------------------------------------
  // ItemGridCellLayout
  //
  struct ItemGridCellLayout : public PlaylistView::TLayoutDelegate
  {
    void layout(Item & cell,
                PlaylistView & view,
                PlaylistModelProxy & model,
                const QModelIndex & index)
    {
      const Item & playlist = *(view.root());
      const Item & fontSize = playlist["font_size"];

      const Texture & xbuttonTexture =
        dynamic_cast<const Texture &>(playlist["xbutton_texture"]);

      Rectangle & frame = cell.addNew<Rectangle>("frame");
      frame.anchors_.fill(cell);

      Image & thumbnail = cell.addNew<Image>("thumbnail");
      thumbnail.setContext(view);
      thumbnail.anchors_.fill(cell);
      thumbnail.anchors_.bottom_.reset();
      thumbnail.height_ = ItemRef::scale(cell, kPropertyHeight, 0.75);
      thumbnail.url_ = thumbnail.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleThumbnail));

      Rectangle & labelBg = cell.addNew<Rectangle>("labelBg");
      Text & label = cell.addNew<Text>("label");
      label.anchors_.bottomLeft(cell);
      label.anchors_.left_ = ItemRef::offset(cell, kPropertyLeft, 7);
      label.anchors_.bottom_ = ItemRef::offset(cell, kPropertyBottom, -7);
      label.maxWidth_ = ItemRef::offset(cell, kPropertyWidth, -14);
      label.text_ = label.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleLabel));
      label.fontSize_ = ItemRef::scale(fontSize, kPropertyHeight, kDpiScale);

      labelBg.anchors_.inset(label, -3, -1);
      labelBg.color_ = ColorRef::constant(Color(0x3f3f3f, 0.5));

      Item & rm = cell.addNew<Item>("remove item");

      Rectangle & playingBg = cell.addNew<Rectangle>("playingBg");
      Text & playing = cell.addNew<Text>("now playing");
      playing.anchors_.top_ = ItemRef::offset(cell, kPropertyTop, 5);
      playing.anchors_.right_ = ItemRef::offset(rm, kPropertyLeft, -5);
      playing.visible_ = playing.addExpr
        (new TQueryBool(model, index, PlaylistModel::kRolePlaying));
      playing.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
      playing.fontSize_ = ItemRef::scale(fontSize,
                                         kPropertyHeight,
                                         0.8 * kDpiScale);

      playingBg.anchors_.inset(playing, -3, -1);
      playingBg.visible_ = BoolRef::reference(playing, kPropertyVisible);
      playingBg.color_ = ColorRef::constant(Color(0x3f3f3f, 0.5));

      rm.width_ = ItemRef::reference(playing, kPropertyHeight);
      rm.height_ = ItemRef::reference(playing, kPropertyHeight);
      rm.anchors_.top_ = ItemRef::reference(playing, kPropertyTop);
      rm.anchors_.right_ = ItemRef::offset(cell, kPropertyRight, -5);

      TexturedRect & xbutton =
        rm.add<TexturedRect>(new TexturedRect("xbutton", xbuttonTexture));
      ItemRef fontDescent = xbutton.addExpr(new GetFontDescent(playing));
      xbutton.anchors_.center(rm);
      xbutton.margins_.set(fontDescent);
      xbutton.width_ = xbutton.addExpr(new InscribedCircleDiameterFor(rm));
      xbutton.height_ = xbutton.width_;

      Rectangle & underline = cell.addNew<Rectangle>("underline");
      underline.anchors_.left_ = ItemRef::offset(playing, kPropertyLeft, -1);
      underline.anchors_.right_ = ItemRef::offset(playing, kPropertyRight, 1);
      underline.anchors_.top_ = ItemRef::offset(playing, kPropertyBottom, 2);
      underline.height_ = ItemRef::constant(2);
      underline.color_ = ColorRef::constant(Color(0xff0000));
      underline.visible_ = underline.addExpr
        (new TQueryBool(model, index, PlaylistModel::kRolePlaying));

      Rectangle & sel = cell.addNew<Rectangle>("selected");
      sel.anchors_.left_ = ItemRef::offset(cell, kPropertyLeft, 3);
      sel.anchors_.right_ = ItemRef::offset(cell, kPropertyRight, -3);
      sel.anchors_.bottom_ = ItemRef::offset(cell, kPropertyBottom, -3);
      sel.height_ = ItemRef::constant(2);
      sel.color_ = ColorRef::constant(Color(0xff0000));
      sel.visible_ = sel.addExpr
        (new TQueryBool(model, index, PlaylistModel::kRoleSelected));

      RemoveModelItems & maRmItem = xbutton.
        add(new RemoveModelItems("ma_remove_item"));
      maRmItem.anchors_.fill(xbutton);
    }
  };



  //----------------------------------------------------------------
  // PlaylistView::PlaylistView
  //
  PlaylistView::PlaylistView():
    Canvas::ILayer(),
    model_(NULL),
    root_(new Item("playlist")),
    w_(0.0),
    h_(0.0),
    pressed_(NULL),
    dragged_(NULL)
  {
    layoutDelegates_[PlaylistModel::kLayoutHintGroupList] =
      TLayoutPtr(new GroupListLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGrid] =
      TLayoutPtr(new ItemGridLayout());

    layoutDelegates_[PlaylistModel::kLayoutHintItemGridCell] =
      TLayoutPtr(new ItemGridCellLayout());
  }

  //----------------------------------------------------------------
  // PlaylistView::event
  //
  bool
  PlaylistView::event(QEvent * event)
  {
    QEvent::Type et = event->type();
    if (et == QEvent::User)
    {
      RequestRepaintEvent * repaintEvent =
        dynamic_cast<RequestRepaintEvent *>(event);
      if (repaintEvent)
      {
        TMakeCurrentContext currentContext(*context());
        Item & root = *root_;
        root.uncache();
        Canvas::ILayer::delegate_->requestRepaint();

        repaintEvent->payload_.setDelivered(true);
        repaintEvent->accept();
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistView::requestRepaint
  //
  void
  PlaylistView::requestRepaint()
  {
    bool postThePayload = requestRepaintEvent_.setDelivered(false);
    if (postThePayload)
    {
      // send an event:
      qApp->postEvent(this, new RequestRepaintEvent(requestRepaintEvent_));
    }
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
    YAE_OGL_11(glDisable(GL_POLYGON_SMOOTH));
    YAE_OGL_11(glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST));
    YAE_OGL_11(glLineWidth(1.0));

    YAE_OGL_11(glEnable(GL_BLEND));
    YAE_OGL_11(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    YAE_OGL_11(glShadeModel(GL_SMOOTH));

    const Segment & xregion = root_->xExtent();
    const Segment & yregion = root_->yExtent();
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
#ifndef NDEBUG
      std::cerr
        << "PlaylistView::processEvent: "
        << yae::toString(et)
        << std::endl;
#endif
    }

    if (et == QEvent::MouseButtonPress ||
        et == QEvent::MouseButtonRelease ||
        et == QEvent::MouseButtonDblClick ||
        et == QEvent::MouseMove)
    {
      TMakeCurrentContext currentContext(*context());
      QMouseEvent * e = static_cast<QMouseEvent *>(event);
      return processMouseEvent(canvas, e);
    }

    if (et == QEvent::Wheel)
    {
      TMakeCurrentContext currentContext(*context());
      QWheelEvent * e = static_cast<QWheelEvent *>(event);
      return processWheelEvent(canvas, e);
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistView::processMouseEvent
  //
  bool
  PlaylistView::processMouseEvent(Canvas * canvas, QMouseEvent * e)
  {
    if (!e)
    {
      return false;
    }

    QEvent::Type et = e->type();
    if (!((et == QEvent::MouseMove && (e->buttons() & Qt::LeftButton)) ||
          (e->button() == Qt::LeftButton)))
    {
      return false;
    }

    QPoint pos = e->pos();
    TVec2D pt(pos.x(), pos.y());

    if (et == QEvent::MouseButtonPress)
    {
      pressed_ = NULL;
      dragged_ = NULL;
      startPt_ = pt;

      if (!root_->getInputHandlers(pt, inputHandlers_))
      {
        return false;
      }

      for (TInputHandlerRIter i = inputHandlers_.rbegin();
           i != inputHandlers_.rend(); ++i)
      {
        InputHandler & handler = *i;
        if (handler.input_->onPress(handler.csysOrigin_, pt))
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
          if (handler.input_->onMouseOver(handler.csysOrigin_, pt))
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

        if (!dragged_ && pressed_ != &handler &&
            handler.input_->onPress(handler.csysOrigin_, startPt_))
        {
          // previous handler didn't handle the drag event, try another:
          pressed_->input_->onCancel();
          pressed_ = &handler;
        }

        if (handler.input_->onDrag(handler.csysOrigin_, startPt_, pt))
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
        if (dragged_)
        {
          accept = dragged_->input_->onDragEnd(dragged_->csysOrigin_,
                                               startPt_,
                                               pt);
        }
        else
        {
          accept = pressed_->input_->onClick(pressed_->csysOrigin_, pt);
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
        if (handler.input_->onDoubleClick(handler.csysOrigin_, pt))
        {
          return true;
        }
      }

      return false;
    }

    return false;
  }

  //----------------------------------------------------------------
  // PlaylistView::processWheelEvent
  //
  bool
  PlaylistView::processWheelEvent(Canvas * canvas, QWheelEvent * e)
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

    for (TInputHandlerRIter i = handlers.rbegin(); i != handlers.rend(); ++i)
    {
      InputHandler & handler = *i;
      if (handler.input_->onScroll(handler.csysOrigin_, pt, degrees))
      {
        break;
      }
    }

    return true;
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
  // PlaylistView::addImageProvider
  //
  void
  PlaylistView::addImageProvider(const QString & providerId,
                                 const TImageProviderPtr & p)
  {
    imageProviders_[providerId] = p;
  }

  //----------------------------------------------------------------
  // PlaylistView::dataChanged
  //
  void
  PlaylistView::dataChanged(const QModelIndex & topLeft,
                            const QModelIndex & bottomRight)
  {
#if 0 // ndef NDEBUG
    std::cerr
      << "PlaylistView::dataChanged, topLeft: " << toString(topLeft)
      << ", bottomRight: " << toString(bottomRight)
      << std::endl;
#endif

    requestRepaint();
  }

  //----------------------------------------------------------------
  // PlaylistView::layoutAboutToBeChanged
  //
  void
  PlaylistView::layoutAboutToBeChanged()
  {
#ifndef NDEBUG
    std::cerr << "PlaylistView::layoutAboutToBeChanged" << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::layoutChanged
  //
  void
  PlaylistView::layoutChanged()
  {
#ifndef NDEBUG
    std::cerr << "PlaylistView::layoutChanged" << std::endl;
#endif
    QModelIndex rootIndex = model_->index(-1, -1);
    TLayoutPtr delegate = findLayoutDelegate(*this, *model_, rootIndex);
    if (!delegate)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    if (pressed_)
    {
      if (dragged_)
      {
        dragged_->input_->onCancel();
        dragged_ = NULL;
      }

      pressed_->input_->onCancel();
      pressed_ = NULL;
    }
    inputHandlers_.clear();

    root_.reset(new Item("playlist"));
    Item & root = *root_;

    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    delegate->layout(root, *this, *model_, rootIndex);

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
#ifndef NDEBUG
    std::cerr << "PlaylistView::modelAboutToBeReset" << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::modelReset
  //
  void
  PlaylistView::modelReset()
  {
#ifndef NDEBUG
    std::cerr << "PlaylistView::modelReset" << std::endl;
#endif
    layoutChanged();
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsAboutToBeInserted
  //
  void
  PlaylistView::rowsAboutToBeInserted(const QModelIndex & parent,
                                      int start, int end)
  {
#if 0 // ndef NDEBUG
    std::cerr
      << "PlaylistView::rowsAboutToBeInserted, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsInserted
  //
  void
  PlaylistView::rowsInserted(const QModelIndex & parent, int start, int end)
  {
#if 0 // ndef NDEBUG
    std::cerr
      << "PlaylistView::rowsInserted, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsAboutToBeRemoved
  //
  void
  PlaylistView::rowsAboutToBeRemoved(const QModelIndex & parent,
                                     int start, int end)
  {
#if 0 // ndef NDEBUG
    std::cerr
      << "PlaylistView::rowsAboutToBeRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif

    Item & root = *root_;
    Scrollview & sview = dynamic_cast<Scrollview &>(root["scrollview"]);
    Item & groups = sview.content_["groups"];

    if (parent.isValid())
    {
      // removing group items:
      int groupIndex = parent.row();

      TPlaylistModelItem & group =
        dynamic_cast<TPlaylistModelItem &>(*(groups.children_[groupIndex]));

      Item & grid = group["payload"]["grid"];
      YAE_ASSERT(start >= 0 && end < int(grid.children_.size()));

      for (int i = end; i >= start; i--)
      {
        grid.children_.erase(grid.children_.begin() + i);
      }
    }
    else
    {
      // removing item groups:
      YAE_ASSERT(start >= 0 && end < int(groups.children_.size()));

      for (int i = end; i >= start; i--)
      {
        groups.children_.erase(groups.children_.begin() + i);
      }
    }
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsRemoved
  //
  void
  PlaylistView::rowsRemoved(const QModelIndex & parent, int start, int end)
  {
#if 0 // ndef NDEBUG
    std::cerr
      << "PlaylistView::rowsRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif

    requestRepaint();
  }

}
