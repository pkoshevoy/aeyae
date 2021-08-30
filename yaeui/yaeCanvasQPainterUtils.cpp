// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// ffmpeg includes:
extern "C"
{
#include <libavutil/pixdesc.h>
}

// local includes:
#include "yaeCanvasQPainterUtils.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // pixelFormatIdFor
  //
  TPixelFormatId
  pixelFormatIdFor(QImage::Format qimageFormat)
  {
    TPixelFormatId pixelFormat =
#ifdef _BIG_ENDIAN
      (qimageFormat == QImage::Format_ARGB32_Premultiplied ||
       qimageFormat == QImage::Format_ARGB32) ? kPixelFormatARGB :
#else
      (qimageFormat == QImage::Format_ARGB32_Premultiplied ||
       qimageFormat == QImage::Format_ARGB32) ? kPixelFormatBGRA :
#endif
      (qimageFormat == QImage::Format_RGB888) ? kPixelFormatRGB24 :
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
      (qimageFormat == QImage::Format_Indexed8) ? kPixelFormatGRAY8 :
#else
      (qimageFormat == QImage::Format_Grayscale8) ? kPixelFormatGRAY8 :
#endif
      kInvalidPixelFormat;

    return pixelFormat;
  }

  //----------------------------------------------------------------
  // shortenTextToFit
  //
  bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight)
  {
    static const QString ellipsis("...");

    // in case style sheet is used, get fontmetrics from painter:
    QFontMetrics fm = painter.fontMetrics();

    const int bboxWidth = bbox.width();

    textLeft.clear();
    textRight.clear();

    QSize sz = fm.size(Qt::TextSingleLine, text);
    int textWidth = sz.width();
    if (textWidth <= bboxWidth || bboxWidth <= 0)
    {
      // text fits, nothing to do:
      if (textAlignment & Qt::AlignLeft)
      {
        textLeft = text;
      }
      else
      {
        textRight = text;
      }

      return false;
    }

    // scale back the estimate to avoid cutting out too much of text,
    // because not all characters have the same width:
    const double stepScale = 0.78;
    const int textLen = text.size();

    int numToRemove = 0;
    int currLen = textLen - numToRemove;
    int aLen = currLen / 2;
    int bLen = currLen - aLen;

    while (currLen > 1)
    {
      // estimate (conservatively) how much text to remove:
      double excess = double(textWidth) / double(bboxWidth) - 1.0;
      if (excess <= 0.0)
      {
        break;
      }

      double excessLen =
        std::max<double>(1.0,
                         stepScale * double(currLen) *
                         excess / (excess + 1.0));

      numToRemove += int(excessLen);
      currLen = textLen - numToRemove;

      aLen = currLen / 2;
      bLen = currLen - aLen;
      QString tmp = text.left(aLen) + ellipsis + text.right(bLen);

      sz = fm.size(Qt::TextSingleLine, tmp);
      textWidth = sz.width();
    }

    if (currLen < 2)
    {
      // too short, give up:
      aLen = 0;
      bLen = 0;
    }

    if (textAlignment & Qt::AlignLeft)
    {
      textLeft = text.left(aLen) + ellipsis;
      textRight = text.right(bLen);
    }
    else
    {
      textLeft = text.left(aLen);
      textRight = ellipsis + text.right(bLen);
    }

    return true;
  }

  //----------------------------------------------------------------
  // drawTextToFit
  //
  void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText)
  {
    QString textLeft;
    QString textRight;

    if ((textAlignment & Qt::TextWordWrap) ||
        !shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }

    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;

    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);

    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);

    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
  }

  //----------------------------------------------------------------
  // drawTextShadow
  //
  static void
  drawTextShadow(QPainter & painter,
                 const QRect & bbox,
                 int textAlignment,
                 const QString & text,
                 bool outline,
                 int offset)
  {
    if (outline)
    {
      painter.drawText(bbox.translated(-offset, 0), textAlignment, text);
      painter.drawText(bbox.translated(offset, 0), textAlignment, text);
      painter.drawText(bbox.translated(0, -offset), textAlignment, text);
    }

    painter.drawText(bbox.translated(0, offset), textAlignment, text);
  }

  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  //
  void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & bgPen,
                          bool outlineShadow,
                          int shadowOffset,
                          QRect * bboxText)
  {
    QPen fgPen = painter.pen();

    QRect bbox(bboxBig.x() + shadowOffset,
               bboxBig.y() + shadowOffset,
               bboxBig.width() - shadowOffset * 2,
               bboxBig.height() - shadowOffset * 2);

    QString textLeft;
    QString textRight;

    if ((textAlignment & Qt::TextWordWrap) ||
        !shortenTextToFit(painter,
                          bbox,
                          textAlignment,
                          text,
                          textLeft,
                          textRight))
    {
      // text fits:
      painter.setPen(bgPen);
      drawTextShadow(painter,
                     bbox,
                     textAlignment,
                     text,
                     outlineShadow,
                     shadowOffset);

      painter.setPen(fgPen);
      painter.drawText(bbox, textAlignment, text, bboxText);
      return;
    }

    // one part will have ... added to it
    int vertAlignment = textAlignment & Qt::AlignVertical_Mask;

    painter.setPen(bgPen);
    drawTextShadow(painter,
                   bbox,
                   vertAlignment | Qt::AlignLeft,
                   textLeft,
                   outlineShadow,
                   shadowOffset);

    drawTextShadow(painter,
                   bbox,
                   vertAlignment | Qt::AlignRight,
                   textRight,
                   outlineShadow,
                   shadowOffset);

    painter.setPen(fgPen);
    QRect bboxLeft;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignLeft,
                     textLeft,
                     &bboxLeft);

    QRect bboxRight;
    painter.drawText(bbox,
                     vertAlignment | Qt::AlignRight,
                     textRight,
                     &bboxRight);

    if (bboxText)
    {
      *bboxText = bboxRight;
      *bboxText |= bboxLeft;
    }
  }

  //----------------------------------------------------------------
  // TQImageBuffer::TQImageBuffer
  //
  TQImageBuffer::TQImageBuffer(int w, int h, QImage::Format fmt):
    qimg_(w, h, fmt)
  {
    unsigned char * dst = qimg_.bits();
    int rowBytes = qimg_.bytesPerLine();
    memset(dst, 0, rowBytes * h);
  }

  //----------------------------------------------------------------
  // TQImageBuffer::TQImageBuffer
  //
  TQImageBuffer::TQImageBuffer(const QImage & qimg):
    qimg_(qimg)
  {}

  //----------------------------------------------------------------
  // TQImageBuffer::destroy
  //
  void
  TQImageBuffer::destroy()
  {
    delete this;
  }

  //----------------------------------------------------------------
  // TQImageBuffer::planes
  //
  std::size_t
  TQImageBuffer::planes() const
  {
    return 1;
  }

  //----------------------------------------------------------------
  // TQImageBuffer::data
  //
  unsigned char *
  TQImageBuffer::data(std::size_t plane) const
  {
    (void)plane;
    const uchar * bits = qimg_.bits();
    return const_cast<unsigned char *>(bits);
  }

  //----------------------------------------------------------------
  // TQImageBuffer::rowBytes
  //
  std::size_t
  TQImageBuffer::rowBytes(std::size_t planeIndex) const
  {
    (void)planeIndex;
    int n = qimg_.bytesPerLine();
    return (std::size_t)n;
  }


  //----------------------------------------------------------------
  // TPainterWrapper::TPainterWrapper
  //
  TPainterWrapper::TPainterWrapper(int w, int h):
    painter_(NULL),
    w_(w),
    h_(h)
  {}

  //----------------------------------------------------------------
  // TPainterWrapper::~TPainterWrapper
  //
  TPainterWrapper::~TPainterWrapper()
  {
    delete painter_;
  }

  //----------------------------------------------------------------
  // TPainterWrapper::getFrame
  //
  TVideoFramePtr &
  TPainterWrapper::getFrame()
  {
    if (!frame_)
    {
      frame_.reset(new TVideoFrame());

      TQImageBuffer * imageBuffer =
        new TQImageBuffer(w_, h_, QImage::Format_ARGB32);

      frame_->data_.reset(imageBuffer);
    }

    VideoTraits & vtts = frame_->traits_;
#ifdef _BIG_ENDIAN
    vtts.av_fmt_ = AV_PIX_FMT_ARGB;
    vtts.pixelFormat_ = kPixelFormatARGB;
#else
    vtts.av_fmt_ = AV_PIX_FMT_BGRA;
    vtts.pixelFormat_ = kPixelFormatBGRA;
#endif
    vtts.av_rng_ = AVCOL_RANGE_JPEG;
    vtts.av_pri_ = AVCOL_PRI_BT709;
    vtts.av_trc_ = AVCOL_TRC_LINEAR;
    vtts.av_csp_ = AVCOL_SPC_RGB;
    vtts.colorspace_ = Colorspace::get(vtts.av_csp_,
                                       vtts.av_pri_,
                                       vtts.av_trc_);

    TQImageBuffer * imageBuffer = (TQImageBuffer *)(frame_->data_.get());
    QImage & image = imageBuffer->qimg_;
    vtts.encodedWidth_ = image.bytesPerLine() / 4;
    vtts.encodedHeight_ = image.byteCount() / image.bytesPerLine();
    vtts.offsetTop_ = 0;
    vtts.offsetLeft_ = 0;
    vtts.visibleWidth_ = w_;
    vtts.visibleHeight_ = h_;
    vtts.pixelAspectRatio_ = 1.0;
    vtts.isUpsideDown_ = false;

    return frame_;
  }

  //----------------------------------------------------------------
  // TPainterWrapper::getImage
  //
  QImage &
  TPainterWrapper::getImage()
  {
    TVideoFramePtr & vf = getFrame();
    TQImageBuffer * imageBuffer = (TQImageBuffer *)(vf->data_.get());
    return imageBuffer->qimg_;
  }

  //----------------------------------------------------------------
  // TPainterWrapper::getPainter
  //
  QPainter &
  TPainterWrapper::getPainter()
  {
    if (!painter_)
    {
      QImage & image = getImage();
      painter_ = new QPainter(&image);

      painter_->setPen(Qt::white);
      painter_->setRenderHint(QPainter::SmoothPixmapTransform, true);

      QFont ft;
      int px = std::max<int>(20, 56.0 * (h_ / 1024.0));
      ft.setPixelSize(px);
      painter_->setFont(ft);
    }

    return *painter_;
  }

  //----------------------------------------------------------------
  // TPainterWrapper::painterEnd
  //
  void
  TPainterWrapper::painterEnd()
  {
    if (painter_)
    {
      painter_->end();
    }
  }

}
