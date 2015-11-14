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
#include <QUrl>

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
  static const double kSupersampleText = 2.0;


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
  // GridCellLeft
  //
  struct GridCellLeft : public TDoubleExpr
  {
    GridCellLeft(const Item & grid, std::size_t cell):
      grid_(grid),
      cell_(cell)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double gridWidth = grid_.width();
      unsigned int cellsPerRow = calcItemsPerRow(gridWidth);
      std::size_t cellCol = cell_ % cellsPerRow;
      double ox = grid_.left() + 2;
      result = ox + gridWidth * double(cellCol) / double(cellsPerRow);
    }

    const Item & grid_;
    std::size_t cell_;
  };

  //----------------------------------------------------------------
  // GridCellTop
  //
  struct GridCellTop : public TDoubleExpr
  {
    GridCellTop(const Item & grid, std::size_t cell):
      grid_(grid),
      cell_(cell)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      std::size_t numCells = grid_.children_.size();
      double gridWidth = grid_.width();
      double cellWidth = calcCellWidth(gridWidth);
      double cellHeight = calcCellHeight(cellWidth);
      unsigned int cellsPerRow = calcItemsPerRow(gridWidth);
      unsigned int rowsOfCells = calcRows(gridWidth, cellWidth, numCells);
      double gridHeight = cellHeight * double(rowsOfCells);
      std::size_t cellRow = cell_ / cellsPerRow;
      double oy = grid_.top() + 2;
      result = oy + gridHeight * double(cellRow) / double(rowsOfCells);
    }

    const Item & grid_;
    std::size_t cell_;
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
      result = calcCellHeight(cellWidth) - 2;
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
    CalcSliderTop(const Scrollable & view, const Item & slider):
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

    const Scrollable & view_;
    const Item & slider_;
  };

  //----------------------------------------------------------------
  // CalcSliderHeight
  //
  struct CalcSliderHeight : public TDoubleExpr
  {
    CalcSliderHeight(const Scrollable & view, const Item & slider):
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

    const Scrollable & view_;
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
    font.setPointSizeF(fontSize * kSupersampleText);
    QFontMetricsF fm(font);

    QRectF maxRect(0.0, 0.0,
                   maxWidth * kSupersampleText,
                   maxHeight * kSupersampleText);

    int flags = item.textFlags();
    QString text =
      getElidedText(maxWidth * kSupersampleText, item, fm, flags);

    QRectF rect = fm.boundingRect(maxRect, flags, text);
    bbox.x_ = rect.x() / kSupersampleText;
    bbox.y_ = rect.y() / kSupersampleText;
    bbox.w_ = rect.width() / kSupersampleText;
    bbox.h_ = rect.height() / kSupersampleText;
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
                  GLuint & ih)
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
    const int rowSize = constImg.bytesPerLine() / bytesPerPixel;
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
                               pixelFormatGL,
                               dataType,
                               data));
    yae_assert_gl_no_error();
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
    double u1 = (double(iw - 1) / double(widthPowerOfTwo));

    double v0 = 0.0;
    double v1 = (double(ih - 1) / double(heightPowerOfTwo));

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
  // Anchors::fill
  //
  void
  Anchors::fill(const TDoubleProp & ref, double offset)
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
    xContent_(addExpr(new CalcXContent(*this))),
    yContent_(addExpr(new CalcYContent(*this))),
    xExtent_(addExpr(new CalcXExtent(*this))),
    yExtent_(addExpr(new CalcYExtent(*this))),
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
      if (anchors_.top_.isValid())
      {
        double t = top();
        double b = yContent.end();
        h = b - t;
      }
      else if (anchors_.bottom_.isValid())
      {
        double t = yContent.start();
        double b = bottom();
        h = b - t;
      }
      else
      {
        YAE_ASSERT(anchors_.vcenter_.isValid());
        h = yContent.length_;
      }
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
  // Item::operator
  //
  const Item &
  Item::operator[](const char * id) const
  {
    if (strcmp(id, "/") == 0)
    {
      const Item * p = this;
      while (p->parent_)
      {
        p = parent_;
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

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      if (child->id_ == id)
      {
        return *child;
      }
    }

    std::ostringstream oss;
    oss << id_ << ": item not found: " << id;
    throw std::runtime_error(oss.str().c_str());
    return *this;
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

    const Segment & yfootprint = this->yExtent();
    if (yregion.disjoint(yfootprint))
    {
      return false;
    }

    const Segment & xfootprint = this->xExtent();
    if (xregion.disjoint(xfootprint))
    {
      return false;
    }

    this->paintContent();

    for (std::vector<ItemPtr>::const_iterator i = children_.begin();
         i != children_.end(); ++i)
    {
      const ItemPtr & child = *i;
      child->paint(xregion, yregion);
    }

    return true;
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
  // findLayoutDelegate
  //
  static ILayoutDelegate::TLayoutPtr
  findLayoutDelegate(const PlaylistView & view,
                     const PlaylistModelProxy & model,
                     const QModelIndex & modelIndex)
  {
    return findLayoutDelegate(view.layouts(), model, modelIndex);
  }

  //----------------------------------------------------------------
  // layoutFilterItem
  //
  static void
  layoutFilterItem(Item & playlist,
                   Item & item,
                   const PlaylistView & view,
                   const PlaylistModelProxy & model,
                   const QModelIndex & itemIndex)
  {
      Rectangle & filter = item.addNew<Rectangle>("bg");
      filter.anchors_.fill(item);
      filter.margins_.set(2);
      filter.radius_ = ItemRef::constant(3);
  }

  //----------------------------------------------------------------
  // GroupListLayout
  //
  struct GroupListLayout : public ILayoutDelegate
  {
    void layout(Item & playlist,
                Item & root,
                const PlaylistView & view,
                const PlaylistModelProxy & model,
                const QModelIndex & rootIndex)
    {
      // setup an invisible item so its height property expression
      // could be computed once and the result reused in other places
      // that need to compute the same property expression:
      Item & titleHeight = playlist.addNewHidden<Item>("title_height");
      titleHeight.height_ =
        titleHeight.addExpr(new CalcTitleHeight(root, 24.0));

      Item & filter = root.addNew<Item>("filter");
      filter.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      filter.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      filter.width_ = ItemRef::reference(root, kPropertyWidth);
      filter.height_ = ItemRef::scale(titleHeight, kPropertyHeight, 1.5);
      layoutFilterItem(playlist, filter, view, model, rootIndex);

      Scrollable & sview = root.addNew<Scrollable>("scrollable");

      Item & scrollbar = root.addNew<Item>("scrollbar");
      scrollbar.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      scrollbar.anchors_.top_ = ItemRef::offset(filter, kPropertyBottom, 5);
      scrollbar.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom, -5);
      scrollbar.width_ =
        scrollbar.addExpr(new CalcTitleHeight(root, 50.0), 0.2);

      sview.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      sview.anchors_.right_ = ItemRef::reference(scrollbar, kPropertyLeft);
      sview.anchors_.top_ = ItemRef::reference(filter, kPropertyBottom);
      sview.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);

      Item & groups = sview.content_;
      groups.anchors_.left_ = ItemRef::reference(sview, kPropertyLeft);
      groups.anchors_.right_ = ItemRef::reference(sview, kPropertyRight);
      groups.anchors_.top_ = ItemRef::constant(0.0);

      Item & cellWidth = playlist.addNewHidden<Item>("cell_width");
      cellWidth.width_ = cellWidth.addExpr(new GridCellWidth(groups));

      Item & cellHeight = playlist.addNewHidden<Item>("cell_height");
      cellHeight.height_ = cellHeight.addExpr(new GridCellHeight(groups));

      Item & fontSize = playlist.addNewHidden<Item>("font_size");
      fontSize.height_ = fontSize.addExpr(new GetFontSize(titleHeight, 0.52,
                                                          cellHeight, 0.15));

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
        Item & group = groups.addNew<Item>("group");
        group.anchors_.left_ = ItemRef::reference(groups, kPropertyLeft);
        group.anchors_.right_ = ItemRef::reference(groups, kPropertyRight);

        if (i < 1)
        {
          group.anchors_.top_ = ItemRef::reference(groups, kPropertyTop);
        }
        else
        {
          Item & prev = *(groups.children_[i - 1]);
          group.anchors_.top_ = ItemRef::reference(prev, kPropertyBottom);
        }

        QModelIndex childIndex = model.index(i, 0, rootIndex);
        ILayoutDelegate::TLayoutPtr childLayout =
           findLayoutDelegate(view, model, childIndex);

        if (childLayout)
        {
          childLayout->layout(playlist, group, view, model, childIndex);
        }
      }

      // configure scrollbar:
      Rectangle & slider = scrollbar.addNew<Rectangle>("slider");
      slider.anchors_.top_ = slider.addExpr(new CalcSliderTop(sview, slider));
      slider.anchors_.left_ = ItemRef::offset(scrollbar, kPropertyLeft, 2);
      slider.anchors_.right_ = ItemRef::offset(scrollbar, kPropertyRight, -2);
      slider.height_ = slider.addExpr(new CalcSliderHeight(sview, slider));
      slider.radius_ = ItemRef::scale(slider, kPropertyWidth, 0.5);
    }
  };

  //----------------------------------------------------------------
  // ItemGridLayout
  //
  struct ItemGridLayout : public ILayoutDelegate
  {
    void layout(Item & playlist,
                Item & group,
                const PlaylistView & view,
                const PlaylistModelProxy & model,
                const QModelIndex & groupIndex)
    {
      // reuse pre-computed properties:
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
      {
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
          (new ModelQuery(model, groupIndex, PlaylistModel::kRoleCollapsed));

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
      }

      Rectangle & separator = group.addNew<Rectangle>("separator");
      separator.anchors_.top_ = ItemRef::offset(title, kPropertyBottom, 5);
      separator.anchors_.left_ = ItemRef::offset(group, kPropertyLeft, 2);
      separator.anchors_.right_ = ItemRef::reference(group, kPropertyRight);
      separator.height_ = ItemRef::constant(2.0);

      Item & grid = group.addNew<Item>("grid");
      grid.anchors_.top_ = ItemRef::reference(separator, kPropertyBottom);
      grid.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      grid.anchors_.right_ = ItemRef::reference(group, kPropertyRight);

      const int numCells = model.rowCount(groupIndex);
      for (int i = 0; i < numCells; i++)
      {
        Rectangle & cell = grid.addNew<Rectangle>("cell");
        cell.anchors_.left_ = cell.addExpr(new GridCellLeft(grid, i));
        cell.anchors_.top_ = cell.addExpr(new GridCellTop(grid, i));
        cell.width_ = ItemRef::reference(cellWidth, kPropertyWidth);
        cell.height_ = ItemRef::reference(cellHeight, kPropertyHeight);
        cell.border_ = ItemRef::constant(1);

        QModelIndex childIndex = model.index(i, 0, groupIndex);
        ILayoutDelegate::TLayoutPtr childLayout =
           findLayoutDelegate(view, model, childIndex);

        if (childLayout)
        {
          childLayout->layout(playlist, cell, view, model, childIndex);
        }
      }

      Item & footer = group.addNew<Item>("footer");
      footer.anchors_.left_ = ItemRef::reference(group, kPropertyLeft);
      footer.anchors_.top_ = ItemRef::reference(grid, kPropertyBottom);
      footer.width_ = ItemRef::reference(group, kPropertyWidth);
      footer.height_ = ItemRef::scale(cellHeight, kPropertyHeight, 0.3);
    }
  };

  //----------------------------------------------------------------
  // ItemGridCellLayout
  //
  struct ItemGridCellLayout : public ILayoutDelegate
  {
    void layout(Item & playlist,
                Item & cell,
                const PlaylistView & view,
                const PlaylistModelProxy & model,
                const QModelIndex & index)
    {
      const Item & fontSize = playlist["font_size"];
      Image & thumbnail = cell.addNew<Image>("thumbnail");
      thumbnail.setContext(view);
      thumbnail.anchors_.fill(cell);
      thumbnail.url_ = thumbnail.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleThumbnail));

      Text & label = cell.addNew<Text>("label");
      label.anchors_.bottomLeft(cell);
      label.anchors_.left_ = ItemRef::offset(cell, kPropertyLeft, 5);
      label.anchors_.bottom_ = ItemRef::offset(cell, kPropertyBottom, -5);
      label.maxWidth_ = ItemRef::offset(cell, kPropertyWidth, -10);
      label.text_ = label.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleLabel));
      label.font_.setBold(false);
      label.fontSize_ = ItemRef::scale(fontSize, kPropertyHeight, kDpiScale);

      Item & rm = cell.addNew<Item>("remove item");

      Text & playing = cell.addNew<Text>("now playing");
      playing.anchors_.top_ = ItemRef::offset(cell, kPropertyTop, 5);
      playing.anchors_.right_ = ItemRef::offset(rm, kPropertyLeft, -5);
      playing.visible_ = playing.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRolePlaying));
      playing.text_ = TVarRef::constant(TVar(QObject::tr("NOW PLAYING")));
      playing.font_.setBold(false);
      playing.fontSize_ = ItemRef::scale(fontSize,
                                         kPropertyHeight,
                                         0.8 * kDpiScale);

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
        (new ModelQuery(model, index, PlaylistModel::kRolePlaying));

      Rectangle & sel = cell.addNew<Rectangle>("selected");
      sel.anchors_.left_ = ItemRef::reference(cell, kPropertyLeft);
      sel.anchors_.right_ = ItemRef::reference(cell, kPropertyRight);
      sel.anchors_.bottom_ = ItemRef::reference(cell, kPropertyBottom);
      sel.margins_.set(3);
      sel.height_ = ItemRef::constant(2);
      sel.color_ = ColorRef::constant(Color(0xff0000));
      sel.visible_ = sel.addExpr
        (new ModelQuery(model, index, PlaylistModel::kRoleSelected));
    }
  };

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

    inline void setContext(const PlaylistView & view)
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

    inline void setContext(const PlaylistView & view)
    {
      image_->setContext(view);
    }

    void uncache();
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
    uncache();
  }

  //----------------------------------------------------------------
  // Image::TPrivate::uncache
  //
  void
  Image::TPrivate::uncache()
  {
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

    static const QSize kDefaultSize(256, 128);
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
    bool ok = yae::uploadTexture2D(image_->getImage(), texId_, iw_, ih_);

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
  Image::setContext(const PlaylistView & view)
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
    // p_->uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // Image::paintContent
  //
  void
  Image::paintContent() const
  {
    if (!Item::visible())
    {
      return;
    }

    p_->paint(*this);
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

    maxRect.setWidth(maxRect.width() * kSupersampleText);
    maxRect.setHeight(maxRect.height() * kSupersampleText);

    BBox bboxContent;
    item.get(kPropertyBBoxContent, bboxContent);

    int iw = (int)ceil(bboxContent.w_ * kSupersampleText);
    int ih = (int)ceil(bboxContent.h_ * kSupersampleText);

    QImage img(iw, ih, QImage::Format_ARGB32);
    {
      img.fill(QColor(0x7f, 0x7f, 0x7f, 0));

      QPainter painter(&img);
      QFont font = item.font_;
      double fontSize = item.fontSize_.get();
      font.setPointSizeF(fontSize * kSupersampleText);
      painter.setFont(font);

      QFontMetricsF fm(font);
      int flags = item.textFlags();
      QString text = getElidedText(maxRect.width(), item, fm, flags);

      // FIXME: this should be a Text property:
      painter.setPen(QColor(0xff, 0xff, 0xff));

#ifdef NDEBUG
      painter.drawText(maxRect, flags, text);
#else
      QRectF result;
      painter.drawText(maxRect, flags, text, &result);

      if (result.width() / kSupersampleText != bboxContent.w_ ||
          result.height() / kSupersampleText != bboxContent.h_)
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

    bool ok = yae::uploadTexture2D(img, texId_, iw_, ih_);
    return ok;
  }

  //----------------------------------------------------------------
  // Text::TPrivate::paint
  //
  void
  Text::TPrivate::paint(const Text & item)
  {
    BBox bboxContent;
    item.get(kPropertyBBoxContent, bboxContent);
    paintTexture2D(bboxContent, texId_, iw_, ih_);
  }


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
    font.setPointSizeF(fontSize * kSupersampleText);
    QFontMetricsF fm(font);
    double ascent = fm.ascent() / kSupersampleText;
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
    font.setPointSizeF(fontSize * kSupersampleText);
    QFontMetricsF fm(font);
    double descent = fm.descent() / kSupersampleText;
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
    font.setPointSizeF(fontSize * kSupersampleText);
    QFontMetricsF fm(font);
    double fh = fm.height() / kSupersampleText;
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
  // Triangle::Triangle
  //
  Triangle::Triangle(const char * id):
    Item(id),
    collapsed_(TVarRef::constant(TVar(false))),
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

    if (!Item::visible())
    {
      return;
    }

    bool collapsed = collapsed_.get().toBool();
    const Color & color = color_.get();
    const Segment & xseg = this->xExtent();
    const Segment & yseg = this->yExtent();

    double radius = 0.5 * (yseg.length_ < xseg.length_ ?
                           yseg.length_ :
                           xseg.length_);

    TVec2D center = vec2d(xseg.center(), yseg.center());
    TVec2D p[3];

    if (collapsed)
    {
      p[0] = center + radius * vec2d(1.0, 0.0);
      p[1] = center + radius * vec2d(-sin_30, -cos_30);
      p[2] = center + radius * vec2d(-sin_30, cos_30);
    }
    else
    {
      p[0] = center + radius * vec2d(cos_30, -sin_30);
      p[1] = center + radius * vec2d(-cos_30, -sin_30);
      p[2] = center + radius * vec2d(0.0, 1.0);
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
  // XButton::XButton
  //
  XButton::XButton(const char * id):
    Item(id),
    border_(ItemRef::constant(0.0)),
    color_(ColorRef::constant(Color(0xffffff, 0.5))),
    colorBorder_(ColorRef::constant(Color(0xffffff, 0.25)))
  {}

  //----------------------------------------------------------------
  // XButton::uncache
  //
  void
  XButton::uncache()
  {
    border_.uncache();
    color_.uncache();
    colorBorder_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // XButton::paintContent
  //
  void
  XButton::paintContent() const
  {
    static const double cos_45 = 0.707106781186548;

    if (!Item::visible())
    {
      return;
    }

    const Color & color = color_.get();
    const Segment & xseg = this->xExtent();
    const Segment & yseg = this->yExtent();

    double radius = 0.5 * (yseg.length_ < xseg.length_ ?
                           yseg.length_ :
                           xseg.length_);

    TVec2D dx = vec2d(0.33 * radius, 0.0);
    TVec2D dy = vec2d(0.0, 0.33 * radius);
    TVec2D center = vec2d(xseg.center(), yseg.center());

    TVec2D v[4] = {
      vec2d(cos_45, -cos_45),
      vec2d(-cos_45, -cos_45),
      vec2d(-cos_45, cos_45),
      vec2d(cos_45, cos_45)
    };

    TVec2D p[12];
    p[0]  = center + dx;
    p[1]  = center + dx + radius * v[0];
    p[2]  = center - dy + radius * v[0];
    p[3]  = center - dy;
    p[4]  = center - dy + radius * v[1];
    p[5]  = center - dx + radius * v[1];
    p[6]  = center - dx;
    p[7]  = center - dx + radius * v[2];
    p[8]  = center + dy + radius * v[2];
    p[9]  = center + dy;
    p[10] = center + dy + radius * v[3];
    p[11] = center + dx + radius * v[3];

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
      YAE_OGL_11(glVertex2dv(p[3].coord_));
      YAE_OGL_11(glVertex2dv(p[4].coord_));
      YAE_OGL_11(glVertex2dv(p[5].coord_));
      YAE_OGL_11(glVertex2dv(p[6].coord_));
      YAE_OGL_11(glVertex2dv(p[7].coord_));
      YAE_OGL_11(glVertex2dv(p[8].coord_));
      YAE_OGL_11(glVertex2dv(p[9].coord_));
      YAE_OGL_11(glVertex2dv(p[10].coord_));
      YAE_OGL_11(glVertex2dv(p[11].coord_));
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
        YAE_OGL_11(glVertex2dv(p[3].coord_));
        YAE_OGL_11(glVertex2dv(p[4].coord_));
        YAE_OGL_11(glVertex2dv(p[5].coord_));
        YAE_OGL_11(glVertex2dv(p[6].coord_));
        YAE_OGL_11(glVertex2dv(p[7].coord_));
        YAE_OGL_11(glVertex2dv(p[8].coord_));
        YAE_OGL_11(glVertex2dv(p[9].coord_));
        YAE_OGL_11(glVertex2dv(p[10].coord_));
        YAE_OGL_11(glVertex2dv(p[11].coord_));
        YAE_OGL_11(glVertex2dv(p[0].coord_));
      }
      YAE_OGL_11(glEnd());
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

    const Segment & xExtent = this->xExtent();
    const Segment & yExtent = this->yExtent();

    double dy = 0.0;
    if (sceneHeight > viewHeight)
    {
      double range = sceneHeight - viewHeight;
      dy = position_ * range;
    }

    TGLSaveMatrixState pushMatrix(GL_MODELVIEW);
    YAE_OGL_11_HERE();
    YAE_OGL_11(glTranslated(xExtent.origin_, yExtent.origin_ + dy, 0.0));
    content_.paint(Segment(0.0, xExtent.length_),
                   Segment(dy, yExtent.length_));

    return true;
  }

#ifndef NDEBUG
  //----------------------------------------------------------------
  // Scrollable::dump
  //
  void
  Scrollable::dump(std::ostream & os, const std::string & indent) const
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
#ifndef NDEBUG
      std::cerr
        << "PlaylistView::processEvent: "
        << yae::toString(et)
        << std::endl;
#endif
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
  // PlaylistView::addImageProvider
  //
  void
  PlaylistView::addImageProvider(const QString & providerId,
                                 const PlaylistView::TImageProviderPtr & p)
  {
    imageProviders_[providerId] = p;
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
#ifndef NDEBUG
    std::cerr
      << "PlaylistView::dataChanged, topLeft: " << toString(topLeft)
      << ", bottomRight: " << toString(bottomRight)
      << std::endl;
#endif
    Canvas::ILayer::delegate_->requestRepaint();
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
    QModelIndex rootIndex = model_->index(0, 0).parent();
    TLayoutPtr delegate = findLayoutDelegate(*this, *model_, rootIndex);
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

    delegate->layout(root, root, *this, *model_, rootIndex);

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
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsAboutToBeInserted
  //
  void
  PlaylistView::rowsAboutToBeInserted(const QModelIndex & parent,
                                      int start, int end)
  {
#ifndef NDEBUG
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
#ifndef NDEBUG
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
#ifndef NDEBUG
    std::cerr
      << "PlaylistView::rowsAboutToBeRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif
  }

  //----------------------------------------------------------------
  // PlaylistView::rowsRemoved
  //
  void
  PlaylistView::rowsRemoved(const QModelIndex & parent, int start, int end)
  {
#ifndef NDEBUG
    std::cerr
      << "PlaylistView::rowsRemoved, parent: " << toString(parent)
      << ", start: " << start << ", end: " << end
      << std::endl;
#endif
  }

}
