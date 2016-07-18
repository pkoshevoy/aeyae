// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sun Feb 13 21:37:20 MST 2011
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_CANVAS_QPAINTER_UTILS_H_
#define YAE_CANVAS_QPAINTER_UTILS_H_

// Qt includes:
#include <QImage>
#include <QPainter>

// yae includes:
#include "yae/video/yae_video.h"


namespace yae
{

  //----------------------------------------------------------------
  // pixelFormatIdFor
  //
  YAE_API TPixelFormatId
  pixelFormatIdFor(QImage::Format imgFormat);

  //----------------------------------------------------------------
  // shortenTextToFit
  //
  YAE_API bool
  shortenTextToFit(QPainter & painter,
                   const QRect & bbox,
                   int textAlignment,
                   const QString & text,
                   QString & textLeft,
                   QString & textRight);

  //----------------------------------------------------------------
  // drawTextToFit
  //
  YAE_API void
  drawTextToFit(QPainter & painter,
                const QRect & bbox,
                int textAlignment,
                const QString & text,
                QRect * bboxText = NULL);

  //----------------------------------------------------------------
  // drawTextWithShadowToFit
  //
  YAE_API void
  drawTextWithShadowToFit(QPainter & painter,
                          const QRect & bboxBig,
                          int textAlignment,
                          const QString & text,
                          const QPen & bgPen,
                          bool outlineShadow = true,
                          int shadowOffset = 1,
                          QRect * bboxText = NULL);

  //----------------------------------------------------------------
  // TQImageBuffer
  //
  struct TQImageBuffer : public IPlanarBuffer
  {
    TQImageBuffer(int w, int h, QImage::Format fmt);
    TQImageBuffer(const QImage & qimg);

    // virtual:
    void destroy();

    // virtual:
    std::size_t planes() const;

    // virtual:
    unsigned char * data(std::size_t plane) const;

    // virtual:
    std::size_t rowBytes(std::size_t planeIndex) const;

    // storage:
    QImage qimg_;
  };

  //----------------------------------------------------------------
  // TPainterWrapper
  //
  struct TPainterWrapper
  {
    TPainterWrapper(int w, int h);
    ~TPainterWrapper();

    TVideoFramePtr & getFrame();

    QImage & getImage();

    QPainter & getPainter();

    void painterEnd();

  private:
    // an RGBA QImage buffer wrapped in a video frame:
    TVideoFramePtr frame_;
    QPainter * painter_;
    int w_;
    int h_;
  };

}


#endif // YAE_CANVAS_QPANTER_UTILS_H_
