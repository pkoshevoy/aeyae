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
  // kSupersampleText
  //
  static const double kSupersampleText = 1.0;


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
  // CalcSliderTop
  //
  struct CalcSliderTop : public TDoubleExpr
  {
    CalcSliderTop(const Scrollview & view, const Item & slider):
      view_(view),
      slider_(slider)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = view_.top();

      double sceneHeight = view_.content_.height();
      double viewHeight = view_.height();
      if (sceneHeight <= viewHeight)
      {
        return;
      }

      double scale = viewHeight / sceneHeight;
      double minHeight = slider_.width() * 5.0;
      double height = minHeight + (viewHeight - minHeight) * scale;
      double y = (viewHeight - height) * view_.position_;
      result += y;
    }

    const Scrollview & view_;
    const Item & slider_;
  };

  //----------------------------------------------------------------
  // CalcSliderHeight
  //
  struct CalcSliderHeight : public TDoubleExpr
  {
    CalcSliderHeight(const Scrollview & view, const Item & slider):
      view_(view),
      slider_(slider)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double sceneHeight = view_.content_.height();
      double viewHeight = view_.height();
      if (sceneHeight <= viewHeight)
      {
        result = viewHeight;
        return;
      }

      double scale = viewHeight / sceneHeight;
      double minHeight = slider_.width() * 5.0;
      result = minHeight + (viewHeight - minHeight) * scale;
    }

    const Scrollview & view_;
    const Item & slider_;
  };

  //----------------------------------------------------------------
  // CalcXContent
  //
  struct CalcXContent : public TSegmentExpr
  {
    CalcXContent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = item_.calcContentWidth();
      result.origin_ =
        item_.anchors_.left_.isValid() ?
        item_.left() :

        item_.anchors_.right_.isValid() ?
        item_.right() - result.length_ :

        item_.hcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->xExtent();
        result.expand(footprint);
      }
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcYContent
  //
  struct CalcYContent : public TSegmentExpr
  {
    CalcYContent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.length_ = item_.calcContentHeight();
      result.origin_ =
        item_.anchors_.top_.isValid() ?
        item_.top() :

        item_.anchors_.bottom_.isValid() ?
        item_.bottom() - result.length_ :

        item_.vcenter() - result.length_ * 0.5;

      for (std::vector<ItemPtr>::const_iterator i = item_.children_.begin();
           i != item_.children_.end(); ++i)
      {
        const ItemPtr & child = *i;
        const Segment & footprint = child->yExtent();
        result.expand(footprint);
      }
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcXExtent
  //
  struct CalcXExtent : public TSegmentExpr
  {
    CalcXExtent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = item_.left();
      result.length_ = item_.width();
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // CalcYExtent
  //
  struct CalcYExtent : public TSegmentExpr
  {
    CalcYExtent(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      result.origin_ = item_.top();
      result.length_ = item_.height();
    }

    const Item & item_;
  };

  //----------------------------------------------------------------
  // wcs_to_lcs
  //
  static TVec2D
  wcs_to_lcs(const TVec2D & origin,
             const TVec2D & u_axis,
             const TVec2D & v_axis,
             const TVec2D & wcs_pt)
  {
    TVec2D wcs_vec = wcs_pt - origin;
    double u = (u_axis * wcs_vec) / (u_axis * u_axis);
    double v = (v_axis * wcs_vec) / (v_axis * v_axis);
    return TVec2D(u, v);
  }

  //----------------------------------------------------------------
  // lcs_to_wcs
  //
  inline static TVec2D
  lcs_to_wcs(const TVec2D & origin,
             const TVec2D & u_axis,
             const TVec2D & v_axis,
             const TVec2D & lcs_pt)
  {
    return origin + u_axis * lcs_pt.x() + v_axis * lcs_pt.y();
  }

  //----------------------------------------------------------------
  // TransformedXContent
  //
  struct TransformedXContent : public TSegmentExpr
  {
    TransformedXContent(const Transform & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      Segment xcontent = item_.xContentLocal_.get();
      Segment ycontent = item_.yContentLocal_.get();

      TVec2D origin;
      TVec2D u_axis;
      TVec2D v_axis;
      item_.getCSys(origin, u_axis, v_axis);

      TVec2D p00 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.start(), ycontent.start()));

      TVec2D p01 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.start(), ycontent.end()));

      TVec2D p10 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.end(), ycontent.start()));

      TVec2D p11 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.end(), ycontent.end()));

      double x0 = std::min<double>(std::min<double>(p00.x(), p01.x()),
                                   std::min<double>(p10.x(), p11.x()));

      double x1 = std::max<double>(std::max<double>(p00.x(), p01.x()),
                                   std::max<double>(p10.x(), p11.x()));

      result = Segment(x0, x1 - x0);
    }

    const Transform & item_;
  };

  //----------------------------------------------------------------
  // TransformedYContent
  //
  struct TransformedYContent : public TSegmentExpr
  {
    TransformedYContent(const Transform & item):
      item_(item)
    {}

    // virtual:
    void evaluate(Segment & result) const
    {
      Segment xcontent = item_.xContentLocal_.get();
      Segment ycontent = item_.yContentLocal_.get();

      TVec2D origin;
      TVec2D u_axis;
      TVec2D v_axis;
      item_.getCSys(origin, u_axis, v_axis);

      TVec2D p00 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.start(), ycontent.start()));

      TVec2D p01 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.start(), ycontent.end()));

      TVec2D p10 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.end(), ycontent.start()));

      TVec2D p11 = lcs_to_wcs(origin, u_axis, v_axis,
                              TVec2D(xcontent.end(), ycontent.end()));

      double y0 = std::min<double>(std::min<double>(p00.y(), p01.y()),
                                   std::min<double>(p10.y(), p11.y()));

      double y1 = std::max<double>(std::max<double>(p00.y(), p01.y()),
                                   std::max<double>(p10.y(), p11.y()));

      result = Segment(y0, y1 - y0);
    }

    const Transform & item_;
  };

  //----------------------------------------------------------------
  // itemHeightDueToItemContent
  //
  static double
  itemHeightDueToItemContent(const Item & item);

  //----------------------------------------------------------------
  // InvisibleItemZeroHeight
  //
  struct InvisibleItemZeroHeight : public TDoubleExpr
  {
    InvisibleItemZeroHeight(const Item & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      if (item_.visible())
      {
        result = itemHeightDueToItemContent(item_);
        return;
      }

      result = 0.0;
    }

    const Item & item_;
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
  // GetFontAscent
  //
  struct GetFontAscent : public TDoubleExpr
  {
    GetFontAscent(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = item_.fontAscent();
    }

    const Text & item_;
  };

  //----------------------------------------------------------------
  // GetFontDescent
  //
  struct GetFontDescent : public TDoubleExpr
  {
    GetFontDescent(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = item_.fontDescent();
    }

    const Text & item_;
  };

  //----------------------------------------------------------------
  // GetFontHeight
  //
  struct GetFontHeight : public TDoubleExpr
  {
    GetFontHeight(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = item_.fontHeight();
    }

    const Text & item_;
  };

  //----------------------------------------------------------------
  // getMaxRect
  //
  static void
  getMaxRect(const Text & item, QRectF & maxRect)
  {
    double maxWidth =
      (item.maxWidth_.isValid() ||
       item.maxWidth_.isCached()) ? item.maxWidth_.get() :
      (item.width_.isValid() ||
       (item.anchors_.left_.isValid() &&
        item.anchors_.right_.isValid())) ? item.width() :
      double(std::numeric_limits<short int>::max());

    double maxHeight =
      (item.maxHeight_.isValid() ||
       item.maxHeight_.isCached()) ? item.maxHeight_.get() :
      (item.height_.isValid() ||
       (item.anchors_.top_.isValid() &&
        item.anchors_.bottom_.isValid())) ? item.height() :
      double(std::numeric_limits<short int>::max());

    maxRect = QRectF(qreal(0), qreal(0), qreal(maxWidth), qreal(maxHeight));
  }

  //----------------------------------------------------------------
  // getElidedText
  //
  static QString
  getElidedText(double maxWidth,
                const Text & item,
                const QFontMetricsF & fm,
                int flags)
  {
    QString text = item.text_.get().toString();

    if (item.elide_ != Qt::ElideNone)
    {
      QString textElided = fm.elidedText(text, item.elide_, maxWidth, flags);
#if 0
      if (text != textElided)
      {
        std::cerr
          << "original: " << text.toUtf8().constData() << std::endl
          << "  elided: " << textElided.toUtf8().constData() << std::endl;
        YAE_ASSERT(false);
      }
#endif
      text = textElided;
    }

    return text;
  }

  //----------------------------------------------------------------
  // calcTextBBox
  //
  static void
  calcTextBBox(const Text & item,
               BBox & bbox,
               double maxWidth,
               double maxHeight)
  {
    QFont font = item.font_;
    double fontSize = item.fontSize_.get();
    font.setPointSizeF(fontSize * item.supersample_);
    QFontMetricsF fm(font);

    QRectF maxRect(0.0, 0.0,
                   maxWidth * item.supersample_,
                   maxHeight * item.supersample_);

    int flags = item.textFlags();
    QString text =
      getElidedText(maxWidth * item.supersample_, item, fm, flags);

    QRectF rect = fm.boundingRect(maxRect, flags, text);
    bbox.x_ = rect.x() / item.supersample_;
    bbox.y_ = rect.y() / item.supersample_;
    bbox.w_ = rect.width() / item.supersample_;
    bbox.h_ = rect.height() / item.supersample_;
  }

  //----------------------------------------------------------------
  // CalcTextBBox
  //
  struct CalcTextBBox : public TBBoxExpr
  {
    CalcTextBBox(const Text & item):
      item_(item)
    {}

    // virtual:
    void evaluate(BBox & result) const
    {
      QRectF maxRect;
      getMaxRect(item_, maxRect);
      calcTextBBox(item_, result, maxRect.width(), maxRect.height());
    }

    const Text & item_;
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
  // UploadTexture
  //
  template <typename TItem>
  struct UploadTexture : public TBoolExpr
  {
    UploadTexture(const TItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = item_.p_->uploadTexture(item_);
    }

    const TItem & item_;
  };

  //----------------------------------------------------------------
  // uploadTexture2D
  //
  static bool
  uploadTexture2D(const QImage & img,
                  GLuint & texId,
                  GLuint & iw,
                  GLuint & ih,
                  GLenum textureFilter)
  {
    QImage::Format imgFormat = img.format();

    TPixelFormatId formatId = pixelFormatIdFor(imgFormat);
    const pixelFormat::Traits * ptts = pixelFormat::getTraits(formatId);
    if (!ptts)
    {
      YAE_ASSERT(false);
      return false;
    }

    unsigned char stride[4] = { 0 };
    unsigned char planes = ptts->getPlanes(stride);
    if (planes > 1 || stride[0] % 8)
    {
      YAE_ASSERT(false);
      return false;
    }

    iw = img.width();
    ih = img.height();
    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));
    YAE_OGL_11(glDeleteTextures(1, &texId));
    YAE_OGL_11(glGenTextures(1, &texId));

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId));
    if (!YAE_OGL_11(glIsTexture(texId)))
    {
      YAE_ASSERT(false);
      return false;
    }

    TGLSaveClientState pushClientAttr(GL_CLIENT_ALL_ATTRIB_BITS);
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
                               textureFilter));
    YAE_OGL_11(glTexParameteri(GL_TEXTURE_2D,
                               GL_TEXTURE_MIN_FILTER,
                               textureFilter));
    yae_assert_gl_no_error();

    GLint internalFormat = 0;
    GLenum pixelFormatGL = 0;
    GLenum dataType = 0;
    GLint shouldSwapBytes = 0;

    yae_to_opengl(formatId,
                  internalFormat,
                  pixelFormatGL,
                  dataType,
                  shouldSwapBytes);

    YAE_OGL_11(glTexImage2D(GL_TEXTURE_2D,
                            0, // mipmap level
                            internalFormat,
                            widthPowerOfTwo,
                            heightPowerOfTwo,
                            0, // border width
                            pixelFormatGL,
                            dataType,
                            NULL));
    yae_assert_gl_no_error();

    YAE_OGL_11(glPixelStorei(GL_UNPACK_SWAP_BYTES,
                             shouldSwapBytes));

    const QImage & constImg = img;
    const unsigned char * data = constImg.bits();
    const unsigned char bytesPerPixel = stride[0] >> 3;
    const int bytesPerRow = constImg.bytesPerLine();
    const int rowSize = bytesPerRow / bytesPerPixel;
    const int padding = alignmentFor(data, bytesPerRow);

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
                               pixelFormatGL,
                               dataType,
                               data));
    yae_assert_gl_no_error();

    if (ih < (unsigned int)heightPowerOfTwo)
    {
      // copy the padding row:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, ih - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 0, // x-offset
                                 ih, // y-offset
                                 iw,
                                 1,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    if (iw < (unsigned int)widthPowerOfTwo)
    {
      // copy the padding column:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, iw - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 iw, // x-offset
                                 0, // y-offset
                                 1,
                                 ih,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    if (ih < (unsigned int)heightPowerOfTwo &&
        iw < (unsigned int)widthPowerOfTwo)
    {
      // copy the bottom-right padding corner:
      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_ROWS, ih - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glPixelStorei(GL_UNPACK_SKIP_PIXELS, iw - 1));
      yae_assert_gl_no_error();

      YAE_OGL_11(glTexSubImage2D(GL_TEXTURE_2D,
                                 0, // mipmap level
                                 iw, // x-offset
                                 ih, // y-offset
                                 1,
                                 1,
                                 pixelFormatGL,
                                 dataType,
                                 data));
    }

    YAE_OGL_11(glDisable(GL_TEXTURE_2D));
    return true;
  }

  //----------------------------------------------------------------
  // paintTexture2D
  //
  static void
  paintTexture2D(const BBox & bbox, GLuint texId, GLuint iw, GLuint ih)
  {
    GLsizei widthPowerOfTwo = powerOfTwoGEQ<GLsizei>(iw);
    GLsizei heightPowerOfTwo = powerOfTwoGEQ<GLsizei>(ih);

    double u0 = 0.0;
    double u1 = double(iw) / double(widthPowerOfTwo);

    double v0 = 0.0;
    double v1 = double(ih) / double(heightPowerOfTwo);

    double x0 = bbox.x_;
    double y0 = bbox.y_;
    double x1 = x0 + bbox.w_;
    double y1 = y0 + bbox.h_;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));

    YAE_OPENGL_HERE();
    if (glActiveTexture)
    {
      YAE_OPENGL(glActiveTexture(GL_TEXTURE0));
      yae_assert_gl_no_error();
    }

    YAE_OGL_11(glBindTexture(GL_TEXTURE_2D, texId));

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
  // Segment::pixelOverlap
  //
  double
  Segment::pixelOverlap(double p) const
  {
    int64 s0 = int64(0.5 + 1000.0 * origin_);
    int64 s1 = int64(0.5 + 1000.0 * (origin_ + length_));
    int64 p0 = int64(0.5 + 1000.0 * p);
    int64 p1 = p0 + 1000;

    int64 a = std::max<int64>(p0, s0);
    int64 b = std::min<int64>(p1, s1);
    int64 o = std::max<int64>(0, b - a);
    YAE_ASSERT(o <= 1000);
    return double(o) * 1e-3;
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
    set(ItemRef::constant(0));
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
  Margins::set(const ItemRef & ref)
  {
    left_ = ref;
    right_ = ref;
    top_ = ref;
    bottom_ = ref;
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
  // Anchors::inset
  //
  void
  Anchors::inset(const TDoubleProp & ref, double ox, double oy)
  {
    left_ = ItemRef::offset(ref, kPropertyLeft, ox);
    right_ = ItemRef::offset(ref, kPropertyRight, -ox);
    top_ = ItemRef::offset(ref, kPropertyTop, oy);
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -oy);
  }

  //----------------------------------------------------------------
  // Anchors::center
  //
  void
  Anchors::center(const TDoubleProp & ref)
  {
    hcenter_ = ItemRef::offset(ref, kPropertyHCenter);
    vcenter_ = ItemRef::offset(ref, kPropertyVCenter);
  }

  //----------------------------------------------------------------
  // Anchors::topLeft
  //
  void
  Anchors::topLeft(const TDoubleProp & ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::topRight
  //
  void
  Anchors::topRight(const TDoubleProp & ref, double offset)
  {
    top_ = ItemRef::offset(ref, kPropertyTop, offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomLeft
  //
  void
  Anchors::bottomLeft(const TDoubleProp & ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    left_ = ItemRef::offset(ref, kPropertyLeft, offset);
  }

  //----------------------------------------------------------------
  // Anchors::bottomRight
  //
  void
  Anchors::bottomRight(const TDoubleProp & ref, double offset)
  {
    bottom_ = ItemRef::offset(ref, kPropertyBottom, -offset);
    right_ = ItemRef::offset(ref, kPropertyRight, -offset);
  }


  //----------------------------------------------------------------
  // Item::Item
  //
  Item::Item(const char * id):
    parent_(NULL),
    visible_(BoolRef::constant(true)),
    xContent_(addExpr(new CalcXContent(*this))),
    yContent_(addExpr(new CalcYContent(*this))),
    xExtent_(addExpr(new CalcXExtent(*this))),
    yExtent_(addExpr(new CalcYExtent(*this))),
    painted_(false)
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
    xExtent_.uncache();
    yExtent_.uncache();
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
    else if (property == kPropertyXExtent)
    {
      value = this->xExtent();
    }
    else if (property == kPropertyYExtent)
    {
      value = this->yExtent();
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
      const Segment & xExtent = this->xExtent();
      const Segment & yExtent = this->yExtent();

      bbox.x_ = xExtent.origin_;
      bbox.w_ = xExtent.length_;

      bbox.y_ = yExtent.origin_;
      bbox.h_ = yExtent.length_;
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
  // Item::get
  //
  void
  Item::get(Property property, Color & value) const
  {
    YAE_ASSERT(false);
    throw std::runtime_error("unsupported item property of type <Color>");
    value = Color();
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
  // Item::xExtent
  //
  const Segment &
  Item::xExtent() const
  {
    return xExtent_.get();
  }

  //----------------------------------------------------------------
  // Item::yExtent
  //
  const Segment &
  Item::yExtent() const
  {
    return yExtent_.get();
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
      if (anchors_.left_.isValid())
      {
        double l = left();
        double r = xContent.end();
        w = r - l;
      }
      else if (anchors_.right_.isValid())
      {
        double l = xContent.start();
        double r = right();
        w = r - l;
      }
      else
      {
        YAE_ASSERT(anchors_.hcenter_.isValid());
        w = xContent.length_;
      }
    }

    width_.cache(w);
    return w;
  }

  //----------------------------------------------------------------
  // itemHeightDueToItemContent
  //
  static double
  itemHeightDueToItemContent(const Item & item)
  {
    double h = 0.0;

    const Segment & yContent = item.yContent();
    if (!yContent.isEmpty())
    {
      if (item.anchors_.top_.isValid())
      {
        double t = item.top();
        double b = yContent.end();
        h = b - t;
      }
      else if (item.anchors_.bottom_.isValid())
      {
        double t = yContent.start();
        double b = item.bottom();
        h = b - t;
      }
      else
      {
        YAE_ASSERT(item.anchors_.vcenter_.isValid());
        h = yContent.length_;
      }
    }

    return h;
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
    double h = itemHeightDueToItemContent(*this);
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
      double hc = anchors_.hcenter_.get();
      double ml = margins_.left_.get();
      double mr = margins_.right_.get();
      double c = hc + ml - mr;
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
      double vc = anchors_.vcenter_.get();
      double mt = margins_.top_.get();
      double mb = margins_.bottom_.get();
      double c = vc + mt - mb;
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
    return visible_.get();
  }

  //----------------------------------------------------------------
  // Item::operator[]
  //
  const Item &
  Item::operator[](const char * id) const
  {
    Item & item = const_cast<Item &>(*this);
    Item & found = item.operator[](id);
    return found;
  }

  //----------------------------------------------------------------
  // Item::operator[]
  //
  Item &
  Item::operator[](const char * id)
  {
    if (strcmp(id, "/") == 0)
    {
      Item * p = this;
      while (p->parent_)
      {
        p = p->parent_;
      }

      return *p;
    }

    if (strcmp(id, ".") == 0)
    {
      return *this;
    }

    if (strcmp(id, "..") == 0)
    {
      if (parent_)
      {
        return *parent_;
      }

      std::ostringstream oss;
      oss << id_ << ": has not parent";
      throw std::runtime_error(oss.str().c_str());
      return *this;
    }

    for (std::vector<ItemPtr>::iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      Item & child = *(*i);
      if (child.id_ == id)
      {
        return child;
      }
    }

    std::ostringstream oss;
    oss << id_ << ": item not found: " << id;
    throw std::runtime_error(oss.str().c_str());
    return *this;
  }

  //----------------------------------------------------------------
  // Item::overlaps
  //
  bool
  Item::overlaps(const TVec2D & pt) const
  {
    if (!Item::visible())
    {
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yfootprint.disjoint(pt.y()))
    {
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xfootprint.disjoint(pt.x()))
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // Item::getInputHandlers
  //
  void
  Item::getInputHandlers(// coordinate system origin of
                         // the input area, expressed in the
                         // coordinate system of the root item:
                         const TVec2D & itemCSysOrigin,

                         // point expressed in the coord. system of the item,
                         // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                         const TVec2D & itemCSysPoint,

                         // pass back input areas overlapping above point,
                         // along with its coord. system origin expressed
                         // in the coordinate system of the root item:
                         std::list<InputHandler> & inputHandlers)
  {
    if (!overlaps(itemCSysPoint))
    {
      return;
    }

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);
    }
  }

  //----------------------------------------------------------------
  // Item::paint
  //
  bool
  Item::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::visible())
    {
      unpaint();
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yregion.disjoint(yfootprint))
    {
      unpaint();
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xregion.disjoint(xfootprint))
    {
      unpaint();
      return false;
    }

    this->paintContent();
    painted_ = true;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint(xregion, yregion);
    }

    return true;
  }

  //----------------------------------------------------------------
  // Item::unpaint
  //
  void
  Item::unpaint() const
  {
    if (!painted_)
    {
      return;
    }

    this->unpaintContent();
    painted_ = false;

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->unpaint();
    }
  }

#ifndef NDEBUG
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
#endif

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
  // FlickableArea::TPrivate
  //
  struct FlickableArea::TPrivate
  {
    TPrivate(const Canvas::ILayer & canvasLayer, Item & scrollbar):
      canvasLayer_(canvasLayer),
      scrollbar_(scrollbar),
      startPos_(0.0),
      nsamples_(0),
      v0_(0.0)
    {}

    void addSample(double dt, double y)
    {
      int i = nsamples_ % TPrivate::kMaxSamples;
      samples_[i].x() = dt;
      samples_[i].y() = y;
      nsamples_++;
    }

    double estimateVelocity(int i0, int i1) const
    {
      if (nsamples_ < 2 || i0 == i1)
      {
        return 0.0;
      }

      const TVec2D & a = samples_[i0];
      const TVec2D & b = samples_[i1];

      double t0 = a.x();
      double t1 = b.x();
      double dt = t1 - t0;
      if (dt <= 0.0)
      {
        YAE_ASSERT(false);
        return 0.0;
      }

      double y0 = a.y();
      double y1 = b.y();
      double dy = y1 - y0;

      double v = dy / dt;
      return v;
    }

    double estimateVelocity(int nsamples) const
    {
      int n = std::min<int>(nsamples, nsamples_);
      int i0 = (nsamples_ - n) % TPrivate::kMaxSamples;
      int i1 = (nsamples_ - 1) % TPrivate::kMaxSamples;
      return estimateVelocity(i0, i1);
    }

    const Canvas::ILayer & canvasLayer_;
    Item & scrollbar_;
    double startPos_;

    // flicking animation parameters:
    QTimer timer_;
    boost::chrono::steady_clock::time_point tStart_;

    // for each sample point: x = t - tStart, y = dragEnd(t)
    enum { kMaxSamples = 5 };
    TVec2D samples_[kMaxSamples];
    std::size_t nsamples_;

    // velocity estimate based on available samples:
    double v0_;
  };

  //----------------------------------------------------------------
  // FlickableArea::FlickableArea
  //
  FlickableArea::FlickableArea(const char * id,
                               const Canvas::ILayer & canvasLayer,
                               Item & scrollbar):
    InputArea(id),
    p_(new TPrivate(canvasLayer, scrollbar))
  {
    bool ok = connect(&p_->timer_, SIGNAL(timeout()),
                      this, SLOT(onTimeout()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // FlickableArea::~FlickableArea
  //
  FlickableArea::~FlickableArea()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // FlickableArea::onScroll
  //
  bool
  FlickableArea::onScroll(const TVec2D & itemCSysOrigin,
                          const TVec2D & rootCSysPoint,
                          double degrees)
  {
    p_->timer_.stop();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;
    double y = scrollview.position_ * yRange + sh * degrees / 360.0;
    double s = std::min<double>(1.0, std::max<double>(0.0, y / yRange));
    scrollview.position_ = s;

    p_->scrollbar_.uncache();
    p_->canvasLayer_.delegate()->requestRepaint();

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onPress
  //
  bool
  FlickableArea::onPress(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
  {
    p_->timer_.stop();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    p_->startPos_ = scrollview.position_;
    p_->tStart_ = boost::chrono::steady_clock::now();
    p_->nsamples_ = 0;
    p_->addSample(0.0, rootCSysPoint.y());

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onDrag
  //
  bool
  FlickableArea::onDrag(const TVec2D & itemCSysOrigin,
                        const TVec2D & rootCSysDragStart,
                        const TVec2D & rootCSysDragEnd)
  {
    p_->timer_.stop();

    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;
    double dy = (rootCSysDragEnd.y() - rootCSysDragStart.y());
    double ds = dy / yRange;

    double s = std::min<double>(1.0, std::max<double>(0.0, p_->startPos_ + ds));
    scrollview.position_ = s;

    p_->scrollbar_.uncache();
    p_->canvasLayer_.delegate()->requestRepaint();

    double secondsElapsed = boost::chrono::duration<double>
      (boost::chrono::steady_clock::now() - p_->tStart_).count();
    p_->addSample(secondsElapsed, rootCSysDragEnd.y());

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onDragEnd
  //
  bool
  FlickableArea::onDragEnd(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysDragStart,
                           const TVec2D & rootCSysDragEnd)
  {
    double dy = (rootCSysDragEnd.y() - rootCSysDragStart.y());

    double secondsElapsed = boost::chrono::duration<double>
      (boost::chrono::steady_clock::now() - p_->tStart_).count();
    p_->addSample(secondsElapsed, rootCSysDragEnd.y());

    double vi = p_->estimateVelocity(3);
    p_->v0_ = dy / secondsElapsed;

    double k = vi / p_->v0_;

#if 0
    std::cerr << "FIXME: v0: " << p_->v0_ << ", k: " << k << std::endl;
#endif

    if (k > 0.1)
    {
      p_->timer_.start(16);
      onTimeout();
    }

    return true;
  }

  //----------------------------------------------------------------
  // FlickableArea::onTimeout
  //
  void
  FlickableArea::onTimeout()
  {
    Scrollview & scrollview = Item::ancestor<Scrollview>();
    double sh = scrollview.height();
    double ch = scrollview.content_.height();
    double yRange = sh - ch;

    boost::chrono::steady_clock::time_point tNow =
      boost::chrono::steady_clock::now();

    double secondsElapsed = boost::chrono::duration<double>
      (tNow - p_->tStart_).count();

    double v0 = p_->v0_;
    double dy = v0 * secondsElapsed;
    double ds = dy / yRange;
    double s =
      std::min<double>(1.0, std::max<double>(0.0, scrollview.position_ + ds));

    scrollview.position_ = s;
    p_->tStart_ = tNow;
    p_->scrollbar_.uncache();
    p_->canvasLayer_.delegate()->requestRepaint();

    if (s == 0.0 || s == 1.0 || v0 == 0.0)
    {
      // stop the animation:
      p_->timer_.stop();
      return;
    }

    // deceleration (friction) coefficient:
    const double k = sh * 0.1;

    double v1 = v0 * (1.0 - k * secondsElapsed / fabs(v0));
    if (v0 * v1 < 0.0)
    {
      // bounce detected:
      p_->timer_.stop();
      return;
    }

#if 0
    std::cerr
      << "FIXME: v0: " << v0
      << ", v1: " << v1
      << ", dv: " << v1 - v0 << std::endl;
#endif

    YAE_ASSERT(fabs(v1) < fabs(v0));
    p_->v0_ = v1;
  }


  //----------------------------------------------------------------
  // SliderDrag
  //
  struct SliderDrag : public InputArea
  {
    SliderDrag(const char * id,
               const Canvas::ILayer & canvasLayer,
               Scrollview & scrollview,
               Item & scrollbar):
      InputArea(id),
      canvasLayer_(canvasLayer),
      scrollview_(scrollview),
      scrollbar_(scrollbar),
      startPos_(0.0)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      startPos_ = scrollview_.position_;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double bh = scrollbar_.height();
      double sh = this->height();
      double yRange = bh - sh;

      double dy = rootCSysDragEnd.y() - rootCSysDragStart.y();
      double ds = dy / yRange;
      double t = std::min<double>(1.0, std::max<double>(0.0, startPos_ + ds));
      scrollview_.position_ = t;

      parent_->uncache();
      canvasLayer_.delegate()->requestRepaint();

      return true;
    }

    const Canvas::ILayer & canvasLayer_;
    Scrollview & scrollview_;
    Item & scrollbar_;
    double startPos_;
  };

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
      XButton & xbutton = rm.addNew<XButton>("xbutton");
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

      xbutton.anchors_.fill(rm);
      xbutton.margins_.set(fontDescentNowPlaying);

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

      XButton & xbutton = rm.addNew<XButton>("xbutton");
      ItemRef fontDescent = xbutton.addExpr(new GetFontDescent(playing));
      xbutton.anchors_.fill(rm);
      xbutton.margins_.set(fontDescent);

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
  // InputArea::InputArea
  //
  InputArea::InputArea(const char * id):
    Item(id)
  {}

  //----------------------------------------------------------------
  // InputArea::getInputHandlers
  //
  void
  InputArea::getInputHandlers(// coordinate system origin of
                              // the input area, expressed in the
                              // coordinate system of the root item:
                              const TVec2D & itemCSysOrigin,

                              // point expressed in the coord.sys. of the item,
                              // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                              const TVec2D & itemCSysPoint,

                              // pass back input areas overlapping above point,
                              // along with its coord. system origin expressed
                              // in the coordinate system of the root item:
                              std::list<InputHandler> & inputHandlers)
  {
    if (!Item::overlaps(itemCSysPoint))
    {
      return;
    }

    inputHandlers.push_back(InputHandler(this, itemCSysOrigin));
  }

  //----------------------------------------------------------------
  // InputArea::onCancel
  //
  void
  InputArea::onCancel()
  {
    if (onCancel_)
    {
      onCancel_->process(parent<Item>());
    }
  }

  //----------------------------------------------------------------
  // InputArea::onMouseOver
  //
  bool
  InputArea::onMouseOver(const TVec2D & itemCSysOrigin,
                         const TVec2D & rootCSysPoint)
  {
    if (onMouseOver_)
    {
      return onMouseOver_->process(parent<Item>(),
                                   itemCSysOrigin,
                                   rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onScroll
  //
  bool
  InputArea::onScroll(const TVec2D & itemCSysOrigin,
                      const TVec2D & rootCSysPoint,
                      double degrees)
  {
    if (onScroll_)
    {
      return onScroll_->process(parent<Item>(),
                                itemCSysOrigin,
                                rootCSysPoint,
                                degrees);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onPress
  //
  bool
  InputArea::onPress(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint)
  {
    if (onPress_)
    {
      return onPress_->process(parent<Item>(),
                               itemCSysOrigin,
                               rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onClick
  //
  bool
  InputArea::onClick(const TVec2D & itemCSysOrigin,
                     const TVec2D & rootCSysPoint)
  {
    if (onClick_)
    {
      return onClick_->process(parent<Item>(),
                               itemCSysOrigin,
                               rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDoubleClick
  //
  bool
  InputArea::onDoubleClick(const TVec2D & itemCSysOrigin,
                           const TVec2D & rootCSysPoint)
  {
    if (onDoubleClick_)
    {
      return onDoubleClick_->process(parent<Item>(),
                                     itemCSysOrigin,
                                     rootCSysPoint);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDrag
  //
  bool
  InputArea::onDrag(const TVec2D & itemCSysOrigin,
                    const TVec2D & rootCSysDragStart,
                    const TVec2D & rootCSysDragEnd)
  {
    if (onDrag_)
    {
      return onDrag_->process(parent<Item>(),
                              itemCSysOrigin,
                              rootCSysDragStart,
                              rootCSysDragEnd);
    }

    return false;
  }

  //----------------------------------------------------------------
  // InputArea::onDragEnd
  //
  bool
  InputArea::onDragEnd(const TVec2D & itemCSysOrigin,
                       const TVec2D & rootCSysDragStart,
                       const TVec2D & rootCSysDragEnd)
  {
    if (onDragEnd_)
    {
      return onDragEnd_->process(parent<Item>(),
                                 itemCSysOrigin,
                                 rootCSysDragStart,
                                 rootCSysDragEnd);
    }

    return this->onDrag(itemCSysOrigin,
                        rootCSysDragStart,
                        rootCSysDragEnd);
  }


  //----------------------------------------------------------------
  // ImagePrivate
  //
  struct ImagePrivate : public ThumbnailProvider::ICallback
  {
    typedef PlaylistView::TImageProviderPtr TImageProviderPtr;

    enum Status
    {
      kImageNotReady,
      kImageRequested,
      kImageReady
    };

    ImagePrivate():
      view_(NULL),
      status_(kImageNotReady)
    {}

    inline void setContext(PlaylistView & view)
    { view_ = &view; }

    // virtual:
    void imageReady(const QImage & image)
    {
      // update the image:
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        img_ = image;
        status_ = kImageReady;
      }

      if (view_)
      {
        view_->delegate()->requestRepaint();
      }
    }

    void clearImage()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      img_ = QImage();
      status_ = kImageNotReady;
    }

    void clearImageAndCancelRequest()
    {
      if (provider_)
      {
        provider_->cancelRequest(id_);
      }

      boost::lock_guard<boost::mutex> lock(mutex_);
      img_ = QImage();
      status_ = kImageNotReady;
    }

    inline void setImageStatusImageRequested()
    { status_ = kImageRequested; }

    inline bool isImageRequested() const
    { return status_ == kImageRequested; }

    inline bool isImageReady() const
    { return status_ == kImageReady; }

    QImage getImage() const
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      return img_;
    }

    const PlaylistView * view_;
    TImageProviderPtr provider_;
    QString resource_;
    QString id_;

  protected:
    mutable boost::mutex mutex_;
    Status status_;
    QImage img_;
  };

  //----------------------------------------------------------------
  // Image::TPrivate
  //
  struct Image::TPrivate
  {
    typedef PlaylistView::TImageProviders TImageProviders;

    TPrivate();
    ~TPrivate();

    inline void setContext(PlaylistView & view)
    {
      image_->setContext(view);
    }

    void unpaint();
    bool load(const QString & thumbnail);
    bool uploadTexture(const Image & item);
    void paint(const Image & item);

    boost::shared_ptr<ImagePrivate> image_;
    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
    BoolRef ready_;
  };

  //----------------------------------------------------------------
  // Image::TPrivate::TPrivate
  //
  Image::TPrivate::TPrivate():
    image_(new ImagePrivate()),
    texId_(0),
    iw_(0),
    ih_(0)
  {}

  //----------------------------------------------------------------
  // Image::TPrivate::~TPrivate
  //
  Image::TPrivate::~TPrivate()
  {
    unpaint();
  }

  //----------------------------------------------------------------
  // Image::TPrivate::unpaint
  //
  void
  Image::TPrivate::unpaint()
  {
    // shortcut:
    ImagePrivate & image = *image_;
    image.clearImageAndCancelRequest();

    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::load
  //
  bool
  Image::TPrivate::load(const QString & resource)
  {
    // shortcut:
    ImagePrivate & image = *image_;

    if (image.resource_ == resource)
    {
      if (image.isImageReady())
      {
        // already loaded:
        return true;
      }
      else if (image.isImageRequested())
      {
        // wait for the image request to be processed:
        return false;
      }
    }

    static const QString kImage = QString::fromUtf8("image");
    QUrl url(resource);
    if (url.scheme() != kImage || !image.view_)
    {
      YAE_ASSERT(false);
      return false;
    }

    QString host = url.host();
    const TImageProviders & providers = image.view_->imageProviders();
    TImageProviders::const_iterator found = providers.find(host);
    if (found == providers.end())
    {
      YAE_ASSERT(false);
      return false;
    }

    QString id = url.path();

    // trim the leading '/' character:
    id = id.right(id.size() - 1);

    image.provider_ = found->second;
    image.resource_ = resource;
    image.id_ = id;
    image.clearImage();
    image.setImageStatusImageRequested();

    static const QSize kDefaultSize(256, 256);
    ThumbnailProvider & provider = *(image.provider_);
    boost::weak_ptr<ThumbnailProvider::ICallback> callback(image_);
    provider.requestImageAsync(image.id_, kDefaultSize, callback);
    return false;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::uploadTexture
  //
  bool
  Image::TPrivate::uploadTexture(const Image & item)
  {
    bool ok = yae::uploadTexture2D(image_->getImage(), texId_, iw_, ih_,
                                   GL_LINEAR);

    // no need to keep a duplicate image around once the texture is ready:
    image_->clearImage();

    return ok;
  }

  //----------------------------------------------------------------
  // Image::TPrivate::paint
  //
  void
  Image::TPrivate::paint(const Image & item)
  {
    if (!texId_ && !load(item.url_.get().toString()))
    {
      // image is not yet loaded:
      return;
    }

    if (!ready_.get())
    {
      YAE_ASSERT(false);
      return;
    }

    // FIXME: write me!
    BBox bbox;
    item.get(kPropertyBBox, bbox);

    double arBBox = bbox.aspectRatio();
    double arImage = double(iw_) / double(ih_);

    if (arImage < arBBox)
    {
      // letterbox pillars:
      double w = bbox.h_ * arImage;
      double dx = (bbox.w_ - w) * 0.5;
      bbox.x_ += dx;
      bbox.w_ = w;
    }
    else if (arBBox < arImage)
    {
      double h = bbox.w_ / arImage;
      double dy = (bbox.h_ - h) * 0.5;
      bbox.y_ += dy;
      bbox.h_ = h;
    }

    paintTexture2D(bbox, texId_, iw_, ih_);
  }

  //----------------------------------------------------------------
  // Image::Image
  //
  Image::Image(const char * id):
    Item(id),
    p_(new Image::TPrivate())
  {
    p_->ready_ = addExpr(new UploadTexture<Image>(*this));
  }

  //----------------------------------------------------------------
  // Image::~Image
  //
  Image::~Image()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // Image::setContext
  //
  void
  Image::setContext(PlaylistView & view)
  {
    p_->setContext(view);
  }

  //----------------------------------------------------------------
  // Image::uncache
  //
  void
  Image::uncache()
  {
    url_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Image::paintContent
  //
  void
  Image::paintContent() const
  {
    p_->paint(*this);
  }

  //----------------------------------------------------------------
  // Image::unpaintContent
  //
  void
  Image::unpaintContent() const
  {
    p_->unpaint();
  }

  //----------------------------------------------------------------
  // Text::TPrivate
  //
  struct Text::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void uncache();
    bool uploadTexture(const Text & item);
    void paint(const Text & item);

    GLuint texId_;
    GLuint iw_;
    GLuint ih_;
    BoolRef ready_;
  };

  //----------------------------------------------------------------
  // Text::TPrivate::TPrivate
  //
  Text::TPrivate::TPrivate():
    texId_(0),
    iw_(0),
    ih_(0)
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
  // Text::TPrivate::uploadTexture
  //
  bool
  Text::TPrivate::uploadTexture(const Text & item)
  {
    QRectF maxRect;
    getMaxRect(item, maxRect);

    maxRect.setWidth(maxRect.width() * item.supersample_);
    maxRect.setHeight(maxRect.height() * item.supersample_);

    BBox bboxContent;
    item.get(kPropertyBBoxContent, bboxContent);

    int iw = (int)ceil(bboxContent.w_ * item.supersample_);
    int ih = (int)ceil(bboxContent.h_ * item.supersample_);

    QImage img(iw, ih, QImage::Format_ARGB32);
    {
      img.fill(QColor(0x7f, 0x7f, 0x7f, 0).rgba());

      QPainter painter(&img);
      QFont font = item.font_;
      double fontSize = item.fontSize_.get();
      font.setPointSizeF(fontSize * item.supersample_);
      painter.setFont(font);

      QFontMetricsF fm(font);
      int flags = item.textFlags();
      QString text = getElidedText(maxRect.width(), item, fm, flags);

      const Color & color = item.color_.get();
      painter.setPen(QColor(color.r(),
                            color.g(),
                            color.b(),
                            color.a()));

#ifdef NDEBUG
      painter.drawText(maxRect, flags, text);
#else
      QRectF result;
      painter.drawText(maxRect, flags, text, &result);

      if (result.width() / item.supersample_ != bboxContent.w_ ||
          result.height() / item.supersample_ != bboxContent.h_)
      {
        YAE_ASSERT(false);

        QFontMetricsF fm(font);
        QRectF v3 = fm.boundingRect(maxRect, flags, text);

        BBox v2;
        calcTextBBox(item, v2, maxRect.width(), maxRect.height());

        std::cerr
          << "\nfont size: " << fontSize
          << ", text: " << text.toUtf8().constData()
          << "\nexpected: " << bboxContent.w_ << " x " << bboxContent.h_
          << "\n  result: " << result.width() << " x " << result.height()
          << "\nv2 retry: " << v2.w_ << " x " << v2.h_
          << "\nv3 retry: " << v3.width() << " x " << v3.height()
          << std::endl;
      }
#endif
    }

    bool ok = yae::uploadTexture2D(img, texId_, iw_, ih_,
                                   item.supersample_ == 1.0 ?
                                   GL_NEAREST : GL_LINEAR);
    return ok;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::paint
  //
  void
  Text::TPrivate::paint(const Text & item)
  {
    BBox bbox;
    item.get(kPropertyBBoxContent, bbox);

    // avoid rendering at fractional pixel coordinates:
    bbox.x_ = std::floor(bbox.x_);
    bbox.y_ = std::floor(bbox.y_);
    bbox.w_ = std::ceil(bbox.w_);
    bbox.h_ = std::ceil(bbox.h_);

    paintTexture2D(bbox, texId_, iw_, ih_);
  }


  //----------------------------------------------------------------
  // Text::Text
  //
  Text::Text(const char * id):
    Item(id),
    p_(new Text::TPrivate()),
    alignment_(Qt::AlignLeft),
    elide_(Qt::ElideNone),
    supersample_(kSupersampleText),
    color_(ColorRef::constant(Color(0xffffff, 1.0)))
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_.setHintingPreference(QFont::PreferFullHinting);
#endif
    font_.setStyleHint(QFont::SansSerif);
    font_.setStyleStrategy((QFont::StyleStrategy)
                           (QFont::PreferOutline |
                            QFont::PreferAntialias |
                            QFont::OpenGLCompatible));

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_.setFamily("impact");
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_.setStretch(QFont::Condensed);
      font_.setWeight(QFont::Black);
    }

    fontSize_ = ItemRef::constant(font_.pointSizeF());
    bboxText_ = addExpr(new CalcTextBBox(*this));
    p_->ready_ = addExpr(new UploadTexture<Text>(*this));
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
  // Text::fontAscent
  //
  double
  Text::fontAscent() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    font.setPointSizeF(fontSize * supersample_);
    QFontMetricsF fm(font);
    double ascent = fm.ascent() / supersample_;
    return ascent;
  }

  //----------------------------------------------------------------
  // Text::fontDescent
  //
  double
  Text::fontDescent() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    font.setPointSizeF(fontSize * supersample_);
    QFontMetricsF fm(font);
    double descent = fm.descent() / supersample_;
    return descent;
  }

  //----------------------------------------------------------------
  // Text::fontHeight
  //
  double
  Text::fontHeight() const
  {
    QFont font = font_;
    double fontSize = fontSize_.get();
    font.setPointSizeF(fontSize * supersample_);
    QFontMetricsF fm(font);
    double fh = fm.height() / supersample_;
    return fh;
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
    if (elide_ != Qt::ElideNone)
    {
      // single line:
      BBox bbox;
      calcTextBBox(*this, bbox,
                   double(std::numeric_limits<short int>::max()),
                   double(std::numeric_limits<short int>::max()));
      return bbox.h_;
    }

    // possible line-wrapping:
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
    color_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Text::paintContent
  //
  void
  Text::paintContent() const
  {
    if (p_->ready_.get())
    {
      p_->paint(*this);
    }
  }

  //----------------------------------------------------------------
  // Text::unpaintContent
  //
  void
  Text::unpaintContent() const
  {
    p_->uncache();
  }


  //----------------------------------------------------------------
  // Gradient::Gradient
  //
  Gradient::Gradient(const char * id):
    Item(id),
    orientation_(Gradient::kVertical)
  {}

  //----------------------------------------------------------------
  // Gradient::paintContent
  //
  void
  Gradient::paintContent() const
  {
    if (color_.size() < 2)
    {
      return;
    }

    BBox bbox;
    this->get(kPropertyBBox, bbox);

    TVec2D xvec(bbox.w_, 0.0);
    TVec2D yvec(0.0, bbox.h_);

    TVec2D o(bbox.x_, bbox.y_);
    TVec2D u = (orientation_ == Gradient::kHorizontal) ? xvec : yvec;
    TVec2D v = (orientation_ == Gradient::kHorizontal) ? yvec : xvec;

    std::map<double, Color>::const_iterator i = color_.begin();
    double t0 = i->first;
    const Color * c0 = &(i->second);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    for (++i; i != color_.end(); ++i)
    {
      double t1 = i->first;
      const Color * c1 = &(i->second);

      TVec2D p0 = (o + u * t0);
      TVec2D p1 = (o + u * t1);

      YAE_OGL_11(glColor4ub(c0->r(), c0->g(), c0->b(), c0->a()));
      YAE_OGL_11(glVertex2dv(p0.coord_));
      YAE_OGL_11(glVertex2dv((p0 + v).coord_));

      YAE_OGL_11(glColor4ub(c1->r(), c1->g(), c1->b(), c1->a()));
      YAE_OGL_11(glVertex2dv(p1.coord_));
      YAE_OGL_11(glVertex2dv((p1 + v).coord_));

      std::swap(t0, t1);
      std::swap(c0, c1);
    }
    YAE_OGL_11(glEnd());
  }


  //----------------------------------------------------------------
  // Rectangle::Rectangle
  //
  Rectangle::Rectangle(const char * id):
    Item(id),
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
    }
  }

  //----------------------------------------------------------------
  // Rectangle::uncache
  //
  void
  Rectangle::uncache()
  {
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
    BBox bbox;
    this->get(kPropertyBBox, bbox);

    double border = border_.get();
    const Color & color = color_.get();
    const Color & colorBorder = colorBorder_.get();

    paintRect(bbox,
              border,
              color,
              colorBorder);
  }


  //----------------------------------------------------------------
  // RoundRect::TPrivate
  //
  struct RoundRect::TPrivate
  {
    TPrivate();
    ~TPrivate();

    void uncache();
    bool uploadTexture(const RoundRect & item);
    void paint(const RoundRect & item);

    GLuint texId_;
    GLuint iw_;
    BoolRef ready_;
  };

  //----------------------------------------------------------------
  // RoundRect::TPrivate::TPrivate
  //
  RoundRect::TPrivate::TPrivate():
    texId_(0),
    iw_(0)
  {}

  //----------------------------------------------------------------
  // RoundRect::TPrivate::~TPrivate
  //
  RoundRect::TPrivate::~TPrivate()
  {
    uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::uncache
  //
  void
  RoundRect::TPrivate::uncache()
  {
    ready_.uncache();

    YAE_OGL_11_HERE();
    YAE_OGL_11(glDeleteTextures(1, &texId_));
    texId_ = 0;
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::uploadTexture
  //
  bool
  RoundRect::TPrivate::uploadTexture(const RoundRect & item)
  {
    // get the corner radius:
    double r = item.radius_.get();

    // get the border width:
    double b = item.border_.get();

    // make sure radius is not less than border width:
    r = std::max<double>(r, b);

    // inner radius:
    double r0 = r - b;

    // we'll be comparing against radius values,
    Segment outerSegment(0.0, r);
    Segment innerSegment(0.0, r0);

    // make sure image is at least 2 pixels wide:
    iw_ = (int)std::ceil(std::max<double>(2.0, 2.0 * r));

    // make sure texture size is even:
    iw_ = (iw_ & 1) ? (iw_ + 1) : iw_;

    // put origin at the center:
    double w2 = iw_ / 2;

    // supersample each pixel:
#if 1
    static const TVec2D sp[] = { TVec2D(0.25, 0.25), TVec2D(0.75, 0.25),
                                 TVec2D(0.25, 0.75), TVec2D(0.75, 0.75) };
#else
    static const TVec2D sp[] = { TVec2D(0.5, 0.5) };
#endif
    static const unsigned int supersample = sizeof(sp) / sizeof(TVec2D);

    QImage img(iw_, iw_, QImage::Format_ARGB32);
    {
      Vec<double, 4> outerColor(item.colorBorder_.get());
      Vec<double, 4> innerColor(item.color_.get());
      TVec2D samplePoint;

      for (int j = 0; j < int(iw_); j++)
      {
        uchar *	row = img.scanLine(j);
        uchar * dst = row;
        samplePoint.set_y(double(j - w2));

        for (int i = 0; i < int(iw_); i++, dst += sizeof(int))
        {
          samplePoint.set_x(double(i - w2));

          double outer = 0.0;
          double inner = 0.0;

          for (unsigned int k = 0; k < supersample; k++)
          {
            double p =
              std::max<double>(0.0, (samplePoint + sp[k]).norm() - 1.0);

            double outerOverlap = outerSegment.pixelOverlap(p);
            double innerOverlap =
              (r0 < r) ? innerSegment.pixelOverlap(p) : outerOverlap;

            outerOverlap = std::max<double>(0.0, outerOverlap - innerOverlap);
            outer += outerOverlap;
            inner += innerOverlap;
          }

          double outerWeight = outer / double(supersample);
          double innerWeight = inner / double(supersample);
          Color c(outerColor * outerWeight + innerColor * innerWeight);
          memcpy(dst, &(c.argb_), sizeof(c.argb_));
        }
      }
    }

    bool ok = yae::uploadTexture2D(img, texId_, iw_, iw_, GL_NEAREST);
    return ok;
  }

  //----------------------------------------------------------------
  // RoundRect::TPrivate::paint
  //
  void
  RoundRect::TPrivate::paint(const RoundRect & item)
  {
    BBox bbox;
    item.get(kPropertyBBox, bbox);

    // avoid rendering at fractional pixel coordinates:
    double w = double(iw_);
    double dw = bbox.w_ < w ? (w - bbox.w_) : 0.0;
    double dh = bbox.h_ < w ? (w - bbox.h_) : 0.0;
    bbox.x_ -= dw * 0.5;
    bbox.y_ -= dh * 0.5;
    bbox.w_ += dw;
    bbox.h_ += dh;

    // get the corner radius:
    double r = item.radius_.get();

    // get the border width:
    double b = item.border_.get();

    // make sure radius is not less than border width:
    r = std::max<double>(r, b);

    // texture width:
    double wt = double(powerOfTwoGEQ<GLsizei>(iw_));

    double t[4];
    t[0] = 0.0;
    t[1] = (double(iw_ / 2)) / wt;
    t[2] = t[1];
    t[3] = w / wt;

    double x[4];
    x[0] = bbox.x_;
    x[1] = x[0] + iw_ / 2;
    x[3] = x[0] + bbox.w_;
    x[2] = x[3] - iw_ / 2;

    double y[4];
    y[0] = bbox.y_;
    y[1] = y[0] + iw_ / 2;
    y[3] = y[0] + bbox.h_;
    y[2] = y[3] - iw_ / 2;

    YAE_OGL_11_HERE();
    YAE_OGL_11(glEnable(GL_TEXTURE_2D));

    YAE_OPENGL_HERE();
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

    for (int j = 0; j < 3; j++)
    {
      if (y[j] == y[j + 1])
      {
        continue;
      }

      for (int i = 0; i < 3; i++)
      {
        if (x[i] == x[i + 1])
        {
          continue;
        }

        YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
        {
          YAE_OGL_11(glTexCoord2d(t[i], t[j]));
          YAE_OGL_11(glVertex2d(x[i], y[j]));

          YAE_OGL_11(glTexCoord2d(t[i], t[j + 1]));
          YAE_OGL_11(glVertex2d(x[i], y[j + 1]));

          YAE_OGL_11(glTexCoord2d(t[i + 1], t[j]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j]));

          YAE_OGL_11(glTexCoord2d(t[i + 1], t[j + 1]));
          YAE_OGL_11(glVertex2d(x[i + 1], y[j + 1]));
        }
        YAE_OGL_11(glEnd());
      }
    }

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
  // RoundRect::RoundRect
  //
  RoundRect::RoundRect(const char * id):
    Item(id),
    p_(new RoundRect::TPrivate()),
    radius_(ItemRef::constant(0.0)),
    border_(ItemRef::constant(0.0)),
    color_(ColorRef::constant(Color(0x7f7f7f, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25)))
  {
    p_->ready_ = addExpr(new UploadTexture<RoundRect>(*this));
  }

  //----------------------------------------------------------------
  // RoundRect::~RoundRect
  //
  RoundRect::~RoundRect()
  {
    delete p_;
  }

  //----------------------------------------------------------------
  // RoundRect::uncache
  //
  void
  RoundRect::uncache()
  {
    radius_.uncache();
    border_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // RoundRect::paint
  //
  void
  RoundRect::paintContent() const
  {
    if (p_->ready_.get())
    {
      p_->paint(*this);
    }
  }

  //----------------------------------------------------------------
  // RoundRect::unpaintContent
  //
  void
  RoundRect::unpaintContent() const
  {
    p_->uncache();
  }


  //----------------------------------------------------------------
  // Triangle::Triangle
  //
  Triangle::Triangle(const char * id):
    Item(id),
    collapsed_(BoolRef::constant(false)),
    border_(ItemRef::constant(1.0)),
    color_(ColorRef::constant(Color(0xffffff, 1.0))),
    colorBorder_(ColorRef::constant(Color(0x7f7f7f, 0.25)))
  {}

  //----------------------------------------------------------------
  // Triangle::uncache
  //
  void
  Triangle::uncache()
  {
    collapsed_.uncache();
    border_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Triangle::paintContent
  //
  void
  Triangle::paintContent() const
  {
    static const double sin_30 = 0.5;
    static const double cos_30 = 0.866025403784439;

    bool collapsed = collapsed_.get();
    const Color & color = color_.get();
    const Segment & xseg = this->xExtent();
    const Segment & yseg = this->yExtent();

    double radius = 0.5 * (yseg.length_ < xseg.length_ ?
                           yseg.length_ :
                           xseg.length_);

    TVec2D center(xseg.center(), yseg.center());
    TVec2D p[3];

    if (collapsed)
    {
      p[0] = center + radius * TVec2D(1.0, 0.0);
      p[1] = center + radius * TVec2D(-sin_30, -cos_30);
      p[2] = center + radius * TVec2D(-sin_30, cos_30);
    }
    else
    {
      p[0] = center + radius * TVec2D(cos_30, -sin_30);
      p[1] = center + radius * TVec2D(-cos_30, -sin_30);
      p[2] = center + radius * TVec2D(0.0, 1.0);
    }

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(),
                          color.g(),
                          color.b(),
                          color.a()));
    YAE_OGL_11(glBegin(GL_TRIANGLE_FAN));
    {
      YAE_OGL_11(glVertex2dv(center.coord_));
      YAE_OGL_11(glVertex2dv(p[0].coord_));
      YAE_OGL_11(glVertex2dv(p[1].coord_));
      YAE_OGL_11(glVertex2dv(p[2].coord_));
      YAE_OGL_11(glVertex2dv(p[0].coord_));
    }
    YAE_OGL_11(glEnd());

    double border = border_.get();
    if (border > 0.0)
    {
      const Color & colorBorder = colorBorder_.get();
      YAE_OGL_11(glColor4ub(colorBorder.r(),
                            colorBorder.g(),
                            colorBorder.b(),
                            colorBorder.a()));
      YAE_OGL_11(glLineWidth(border));
      YAE_OGL_11(glBegin(GL_LINE_LOOP));
      {
        YAE_OGL_11(glVertex2dv(p[0].coord_));
        YAE_OGL_11(glVertex2dv(p[1].coord_));
        YAE_OGL_11(glVertex2dv(p[2].coord_));
      }
      YAE_OGL_11(glEnd());
    }
  }


  //----------------------------------------------------------------
  // PlusButton::PlusButton
  //
  PlusButton::PlusButton(const char * id):
    Item(id),
    color_(ColorRef::constant(Color(0xffffff, 0.5)))
  {}

  //----------------------------------------------------------------
  // PlusButton::uncache
  //
  void
  PlusButton::uncache()
  {
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // PlusButton::paintContent
  //
  void
  PlusButton::paintContent() const
  {
    static const TVec2D xaxis(1.0, 0.0);
    static const TVec2D yaxis(0.0, 1.0);

    const Color & color = color_.get();
    const Segment & xseg = this->xExtent();
    const Segment & yseg = this->yExtent();
    TVec2D center(xseg.center(), yseg.center());

    double s = (yseg.length_ < xseg.length_ ?
                yseg.length_ :
                xseg.length_);
    double t = s * 0.2;
    double hs = s * 0.5;
    double ht = t * 0.5;

    TVec2D p[12];
    p[0]  = center - ht * xaxis - hs * yaxis;
    p[1]  = center - ht * xaxis + hs * yaxis;
    p[2]  = center + ht * xaxis - hs * yaxis;
    p[3]  = center + ht * xaxis + hs * yaxis;

    p[4]  = center + ht * (xaxis - yaxis);
    p[5]  = center + ht * (xaxis + yaxis);
    p[6]  = center + hs * xaxis - ht * yaxis;
    p[7]  = center + hs * xaxis + ht * yaxis;

    p[8]  = center - hs * xaxis - ht * yaxis;
    p[9]  = center - hs * xaxis + ht * yaxis;
    p[10] = center - ht * (xaxis + yaxis);
    p[11] = center - ht * (xaxis - yaxis);

    YAE_OGL_11_HERE();
    YAE_OGL_11(glColor4ub(color.r(), color.g(), color.b(), color.a()));

    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2dv(p[0].coord_));
      YAE_OGL_11(glVertex2dv(p[1].coord_));
      YAE_OGL_11(glVertex2dv(p[2].coord_));
      YAE_OGL_11(glVertex2dv(p[3].coord_));
    }
    YAE_OGL_11(glEnd());

    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2dv(p[4].coord_));
      YAE_OGL_11(glVertex2dv(p[5].coord_));
      YAE_OGL_11(glVertex2dv(p[6].coord_));
      YAE_OGL_11(glVertex2dv(p[7].coord_));
    }
    YAE_OGL_11(glEnd());

    YAE_OGL_11(glBegin(GL_TRIANGLE_STRIP));
    {
      YAE_OGL_11(glVertex2dv(p[8].coord_));
      YAE_OGL_11(glVertex2dv(p[9].coord_));
      YAE_OGL_11(glVertex2dv(p[10].coord_));
      YAE_OGL_11(glVertex2dv(p[11].coord_));
    }
    YAE_OGL_11(glEnd());
  }


  //----------------------------------------------------------------
  // XButton::XButton
  //
  XButton::XButton(const char * id):
    Item(id),
    color_(ColorRef::constant(Color(0xffffff, 0.5)))
  {
    Transform & xform = addNew<Transform>("xform");
    xform.anchors_.hcenter_ = ItemRef::reference(*this, kPropertyHCenter);
    xform.anchors_.vcenter_ = ItemRef::reference(*this, kPropertyVCenter);
    xform.rotation_ = ItemRef::constant(M_PI * 0.25);

    PlusButton & pb = xform.addNew<PlusButton>("plus_button");
    pb.anchors_.vcenter_ = ItemRef::constant(0.0);
    pb.anchors_.hcenter_ = ItemRef::constant(0.0);
    pb.width_ = ItemRef::scale(*this, kPropertyWidth, 1.3);
    pb.height_ = pb.width_;
    pb.color_ = ColorRef::reference(*this, kPropertyColor);
  }

  //----------------------------------------------------------------
  // XButton::uncache
  //
  void
  XButton::uncache()
  {
    color_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Item::get
  //
  void
  XButton::get(Property property, Color & value) const
  {
    if (property == kPropertyColor)
    {
      value = this->color_.get();
    }
    else
    {
      // let the base class handle it:
      Item::get(property, value);
    }
  }


  //----------------------------------------------------------------
  // GetUAxis
  //
  struct GetUAxis : public TVec2DExpr
  {
    GetUAxis(const ItemRef & rotation):
      rotation_(rotation)
    {}

    // virtual:
    void evaluate(TVec2D & result) const
    {
      double angle = rotation_.get();
      double sin_a = sin(angle);
      double cos_a = cos(angle);
      result.set_x(cos_a);
      result.set_y(sin_a);
    }

    const ItemRef & rotation_;
  };

  //----------------------------------------------------------------
  // getVAxis
  //
  inline static TVec2D
  getVAxis(const TVec2D & u_axis)
  {
    // v-axis is orthogonal to u-axis,
    // meaning it is at a 90 degree angle to u-axis;
    //
    // cos(a + pi/2) == -sin(a)
    // sin(a + pi/2) ==  cos(a)
    //
    return TVec2D(-u_axis.y(), u_axis.x());
  }

  //----------------------------------------------------------------
  // SameExpression
  //
  struct SameExpression
  {
    SameExpression(const IPropertiesBase * ref = NULL):
      ref_(ref)
    {}

    inline bool operator()(const TPropertiesBasePtr & expression) const
    {
      if (expression.get() == ref_)
      {
        return true;
      }

      return false;
    }

    const IPropertiesBase * ref_;
  };

  //----------------------------------------------------------------
  // Transform::Transform
  //
  Transform::Transform(const char * id):
    Item(id),
    rotation_(ItemRef::constant(0.0))
  {
    // override base class content and extent calculation
    // in order to handle coordinate system transformations:
    xContentLocal_ = Item::xContent_;
    yContentLocal_ = Item::yContent_;
    Item::xContent_ = addExpr(new TransformedXContent(*this));
    Item::yContent_ = addExpr(new TransformedYContent(*this));

    // get rid of base implementation of xExtent and yExtent:
    Item::expr_.remove_if(SameExpression(Item::xExtent_.ref_));
    Item::expr_.remove_if(SameExpression(Item::yExtent_.ref_));

    Item::xExtent_ = Item::xContent_;
    Item::yExtent_ = Item::yContent_;

    uAxis_ = addExpr(new GetUAxis(rotation_));
  }

  //----------------------------------------------------------------
  // Transform::uncache
  //
  void
  Transform::uncache()
  {
    rotation_.uncache();
    xContentLocal_.uncache();
    yContentLocal_.uncache();
    uAxis_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Transform::getInputHandlers
  //
  void
  Transform::getInputHandlers(// coordinate system origin of
                              // the input area, expressed in the
                              // coordinate system of the root item:
                              const TVec2D & itemCSysOrigin,

                              // point expressed in the coord.sys. of the item,
                              // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                              const TVec2D & itemCSysPoint,

                              // pass back input areas overlapping above point,
                              // along with its coord. system origin expressed
                              // in the coordinate system of the root item:
                              std::list<InputHandler> & inputHandlers)
  {
    if (!Item::overlaps(itemCSysPoint))
    {
      return;
    }

    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    getCSys(origin, u_axis, v_axis);

    // transform point to local coordinate system:
    TVec2D localCSysOffset = itemCSysOrigin + origin;
    TVec2D ptInLocalCoords = wcs_to_lcs(origin, u_axis, v_axis, itemCSysPoint);

    for (std::vector<ItemPtr>::const_iterator i = Item::children_.begin();
         i != Item::children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->getInputHandlers(localCSysOffset, ptInLocalCoords, inputHandlers);
    }
  }

  //----------------------------------------------------------------
  // Transform::paint
  //
  bool
  Transform::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::visible())
    {
      unpaint();
      return false;
    }

    const Segment & yfootprint = this->yExtent();
    if (yregion.disjoint(yfootprint))
    {
      unpaint();
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xregion.disjoint(xfootprint))
    {
      unpaint();
      return false;
    }

    this->paintContent();
    painted_ = true;

    // transform x,y-region to local coordinate system u,v-region:
    TVec2D origin;
    TVec2D u_axis;
    TVec2D v_axis;
    getCSys(origin, u_axis, v_axis);

    double angle = rotation_.get();
    double degrees = 180.0 * (angle / M_PI);

    TVec2D p00 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.start(), yregion.start()));

    TVec2D p01 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.start(), yregion.end()));

    TVec2D p10 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.end(), yregion.start()));

    TVec2D p11 = wcs_to_lcs(origin, u_axis, v_axis,
                            TVec2D(xregion.end(), yregion.end()));

    double u0 = std::min<double>(std::min<double>(p00.x(), p01.x()),
                                 std::min<double>(p10.x(), p11.x()));

    double u1 = std::max<double>(std::max<double>(p00.x(), p01.x()),
                                 std::max<double>(p10.x(), p11.x()));

    double v0 = std::min<double>(std::min<double>(p00.y(), p01.y()),
                                 std::min<double>(p10.y(), p11.y()));

    double v1 = std::max<double>(std::max<double>(p00.y(), p01.y()),
                                 std::max<double>(p10.y(), p11.y()));

    Segment uregion(u0, u1 - u0);
    Segment vregion(v0, v1 - v0);

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(origin.x(), origin.y(), 0.0));
    YAE_OGL_11(glRotated(degrees, 0.0, 0.0, 1.0));

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint(uregion, vregion);
    }

    return true;
  }

  //----------------------------------------------------------------
  // Transform::getCSys
  //
  void
  Transform::getCSys(TVec2D & origin, TVec2D & uAxis, TVec2D & vAxis) const
  {
    YAE_ASSERT(anchors_.hcenter_.isValid() && anchors_.vcenter_.isValid());
    origin.set_x(anchors_.hcenter_.get());
    origin.set_y(anchors_.vcenter_.get());
    uAxis = uAxis_.get();
    vAxis = getVAxis(uAxis);
  }


  //----------------------------------------------------------------
  // Scrollview::Scrollview
  //
  Scrollview::Scrollview(const char * id):
    Item(id),
    content_("content"),
    position_(0.0)
  {}

  //----------------------------------------------------------------
  // Scrollview::uncache
  //
  void
  Scrollview::uncache()
  {
    Item::uncache();
    content_.uncache();
  }

  //----------------------------------------------------------------
  // getContentView
  //
  static void
  getContentView(const Scrollview & sview,
                 TVec2D & origin,
                 Segment & xView,
                 Segment & yView)
  {
    double sceneHeight = sview.content_.height();
    double viewHeight = sview.height();

    const Segment & xExtent = sview.xExtent();
    const Segment & yExtent = sview.yExtent();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = sview.position_ * range;
    }

    origin.x() = xExtent.origin_;
    origin.y() = yExtent.origin_ - dy;
    xView = Segment(0.0, xExtent.length_);
    yView = Segment(dy, yExtent.length_);
  }

  //----------------------------------------------------------------
  // Scrollview::getInputHandlers
  //
  void
  Scrollview::getInputHandlers(// coordinate system origin of
                               // the input area, expressed in the
                               // coordinate system of the root item:
                               const TVec2D & itemCSysOrigin,

                               // point expressed in the coord.sys. of the item,
                               // rootCSysPoint = itemCSysOrigin + itemCSysPoint
                               const TVec2D & itemCSysPoint,

                               // pass back input areas overlapping above point,
                               // along with its coord. system origin expressed
                               // in the coordinate system of the root item:
                               std::list<InputHandler> & inputHandlers)
  {
    Item::getInputHandlers(itemCSysOrigin, itemCSysPoint, inputHandlers);

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(*this, origin, xView, yView);

    TVec2D ptInViewCoords = itemCSysPoint - origin;
    TVec2D offsetToView = itemCSysOrigin + origin;
    content_.getInputHandlers(offsetToView, ptInViewCoords, inputHandlers);
  }

  //----------------------------------------------------------------
  // Scrollview::paint
  //
  bool
  Scrollview::paint(const Segment & xregion, const Segment & yregion) const
  {
    if (!Item::paint(xregion, yregion))
    {
      content_.unpaint();
      return false;
    }

    TVec2D origin;
    Segment xView;
    Segment yView;
    getContentView(*this, origin, xView, yView);

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(origin.x(), origin.y(), 0.0));
    content_.paint(xView, yView);

    return true;
  }

  //----------------------------------------------------------------
  // Scrollview::unpaint
  //
  void
  Scrollview::unpaint()
  {
    Item::unpaint();
    content_.unpaint();
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Scrollview::dump
  //
  void
  Scrollview::dump(std::ostream & os, const std::string & indent) const
  {
    Item::dump(os, indent);
    content_.dump(os, indent + "  ");
  }
#endif


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
      QMouseEvent * e = static_cast<QMouseEvent *>(event);
      return processMouseEvent(canvas, e);
    }

    if (et == QEvent::Wheel)
    {
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
                                 const PlaylistView::TImageProviderPtr & p)
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
