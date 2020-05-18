// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jun 27 20:11:55 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_FRAME_CROP_VIEW_H_
#define YAE_FRAME_CROP_VIEW_H_

// Qt interfaces:
#include <QObject>

// local interfaces:
#include "yaeItemView.h"


namespace yae
{

  //----------------------------------------------------------------
  // LetterBoxExpr
  //
  struct YAEUI_API LetterBoxExpr : public TBBoxExpr
  {
    LetterBoxExpr(const Item & container, const CanvasRenderer & renderer);

    // virtual:
    void evaluate(BBox & bbox) const;

    const Item & container_;
    const CanvasRenderer & renderer_;
  };

  //----------------------------------------------------------------
  // LetterBoxItem
  //
  struct YAEUI_API LetterBoxItem : public ExprItem<BBoxRef>
  {
    LetterBoxItem(const char * id, TBBoxExpr * expression);

    // virtual:
    void get(Property property, double & value) const;

    using ExprItem<BBoxRef>::get;
  };

  //----------------------------------------------------------------
  // CanvasRendererItem
  //
  struct YAEUI_API CanvasRendererItem : public Item
  {
    CanvasRendererItem(const char * id, Canvas::ILayer & layer);

    // virtual:
    void paintContent() const;

    void observe(Canvas * canvas, const TVideoFramePtr & frame);

    void loadFrame();

    Canvas::ILayer & layer_;
    CanvasRenderer renderer_;
    TVideoFramePtr frame_;
  };

  //----------------------------------------------------------------
  // OnFrameLoaded
  //
  struct OnFrameLoaded : public Canvas::ILoadFrameObserver
  {
    OnFrameLoaded(CanvasRendererItem & rendererItem):
      rendererItem_(rendererItem)
    {}

    // virtual:
    void frameLoaded(Canvas * canvas, const TVideoFramePtr & frame)
    {
      rendererItem_.observe(canvas, frame);
    }

  protected:
    CanvasRendererItem & rendererItem_;
  };

  //----------------------------------------------------------------
  // FrameCropView
  //
  class YAEUI_API FrameCropView : public ItemView
  {
    Q_OBJECT;

  public:
    FrameCropView();

    void init(ItemView * mainView);

    // virtual:
    ItemViewStyle * style() const;

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    // NOTE: crop region is applied to the un-rotated image:
    void setCrop(const TVideoFramePtr & frame, const TCropFrame & crop);

    // NOTE: xCrop and yCrop are expressed in the rotated coordinate system:
    void setCrop(const Segment & xCrop, const Segment & yCrop);

    inline void emit_done()
    { emit done(); }

  signals:
    void done();

    // NOTE: crop region to be applied to the un-rotated image:
    void cropped(const TVideoFramePtr & frame, const TCropFrame & crop);

  protected:
    ItemView * mainView_;
  };
}


#endif // YAE_FRAME_CROP_VIEW_H_
