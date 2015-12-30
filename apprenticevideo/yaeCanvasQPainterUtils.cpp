// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// local includes:
#include "yaeCanvasQPainterUtils.h"
#include "yaeUtilsQt.h"


namespace yae
{

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
