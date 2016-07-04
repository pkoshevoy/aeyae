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
  // forward declarations:
  class PlaylistView;

  //----------------------------------------------------------------
  // LetterBoxExpr
  //
  struct YAE_API LetterBoxExpr : public TBBoxExpr
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
  struct YAE_API LetterBoxItem : public ExprItem<BBoxRef>
  {
    LetterBoxItem(const char * id, TBBoxExpr * expression);

    // virtual:
    void get(Property property, double & value) const;
  };

  //----------------------------------------------------------------
  // CanvasRendererItem
  //
  struct YAE_API CanvasRendererItem : public Item
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
  // FrameCropView
  //
  class YAE_API FrameCropView : public ItemView
  {
    Q_OBJECT;

  public:
    FrameCropView();

    void init(PlaylistView * playlist);

    // virtual: returns false if size didn't change
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);

    void setCrop(const Segment & xCrop, const Segment & yCrop);

    inline void emitDone()
    { emit done(); }

  signals:
    void cropped(const Segment & xCrop, const Segment & yCrop);
    void done();

  protected:
    PlaylistView * playlist_;
  };
}


#endif // YAE_FRAME_CROP_VIEW_H_
