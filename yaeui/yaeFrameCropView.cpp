// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Mon Jun 27 20:23:31 MDT 2016
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// local includes:
#include "yaeColor.h"
#include "yaeDashedRect.h"
#include "yaeDonutRect.h"
#include "yaeFrameCropView.h"
#include "yaeInputArea.h"
#include "yaeItemRef.h"
#include "yaeItemViewStyle.h"
#include "yaeProperty.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeText.h"


namespace yae
{

  //----------------------------------------------------------------
  // LetterBoxExpr::LetterBoxExpr
  //
  LetterBoxExpr::LetterBoxExpr(const Item & container,
                               const CanvasRenderer & renderer):
    container_(container),
    renderer_(renderer)
  {}

  //----------------------------------------------------------------
  // LetterBoxExpr::evaluate
  //
  void
  LetterBoxExpr::evaluate(BBox & bbox) const
  {
    bbox = BBox();

    int rotate = 0;
    if (!renderer_.imageWidthHeightRotated(bbox.w_, bbox.h_, rotate) ||
        bbox.w_ == 0.0 ||
        bbox.h_ == 0.0)
    {
      return;
    }

    double w_max = container_.width();
    double h_max = container_.height();
    double car = w_max / h_max;
    double dar = bbox.w_ / bbox.h_;

    if (dar < car)
    {
      bbox.w_ = h_max * dar;
      bbox.x_ += 0.5 * (w_max - bbox.w_);
      bbox.h_ = h_max;
      bbox.y_ = 0.0;
    }
    else
    {
      bbox.h_ = w_max / dar;
      bbox.y_ += 0.5 * (h_max - bbox.h_);
      bbox.w_ = w_max;
      bbox.x_ = 0.0;
    }
  }


  //----------------------------------------------------------------
  // LetterBoxItem::LetterBoxItem
  //
  LetterBoxItem::LetterBoxItem(const char * id, TBBoxExpr * expression):
    ExprItem<BBoxRef>(id, expression)
  {}

  //----------------------------------------------------------------
  // LetterBoxItem::get
  //
  void
  LetterBoxItem::get(Property property, double & value) const
  {
    const BBox & bbox = expression_.get();
    if (property == kPropertyLeft)
    {
      value = bbox.x_;
    }
    else if (property == kPropertyTop)
    {
      value = bbox.y_;
    }
    else if (property == kPropertyWidth)
    {
      value = bbox.w_;
    }
    else if (property == kPropertyHeight)
    {
      value = bbox.h_;
    }
    else
    {
      YAE_ASSERT(false);
      Item::get(property, value);
    }
  }


  //----------------------------------------------------------------
  // Dismiss
  //
  struct Dismiss : public ClickableItem
  {
    Dismiss(FrameCropView & view):
      ClickableItem("dismiss"),
      view_(view)
    {}

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.done();
      return true;
    }

    FrameCropView & view_;
  };


  //----------------------------------------------------------------
  // ColorOnHover
  //
  struct ColorOnHover : public TColorExpr
  {
    ColorOnHover(FrameCropView & view,
                 Item & item,
                 const Color & c0,
                 const Color & c1):
      view_(view),
      item_(item),
      c0_(c0),
      c1_(c1)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const TVec2D & pt = view_.mousePt();
      bool overlaps = item_.overlaps(pt);
      result = overlaps ? c1_ : c0_;
    }

    FrameCropView & view_;
    Item & item_;
    Color c0_;
    Color c1_;
  };


  //----------------------------------------------------------------
  // RegionSelect
  //
  // d00 d01 d02
  // d10     d12
  // d20 d21 d22
  //
  struct RegionSelect : public InputArea
  {
    RegionSelect(FrameCropView & view,
                 DonutRect & rect,

                 Item & d00,
                 Item & d01,
                 Item & d02,

                 Item & d10,
                 Item & d12,

                 Item & d20,
                 Item & d21,
                 Item & d22):
      InputArea("region_select"),
      view_(view),
      rect_(rect),
      d00_(d00),
      d01_(d01),
      d02_(d02),
      d10_(d10),
      d12_(d12),
      d20_(d20),
      d21_(d21),
      d22_(d22)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      TVec2D itemCSysPoint = rootCSysPoint - itemCSysOrigin;
      if (d22_.overlaps(itemCSysPoint))
      {
        dragging_ = &d22_;
      }
      else if (d02_.overlaps(itemCSysPoint))
      {
        dragging_ = &d02_;
      }
      else if (d20_.overlaps(itemCSysPoint))
      {
        dragging_ = &d20_;
      }
      else if (d00_.overlaps(itemCSysPoint))
      {
        dragging_ = &d00_;
      }
      else if (d12_.overlaps(itemCSysPoint))
      {
        dragging_ = &d12_;
      }
      else if (d10_.overlaps(itemCSysPoint))
      {
        dragging_ = &d10_;
      }
      else if (d21_.overlaps(itemCSysPoint))
      {
        dragging_ = &d21_;
      }
      else if (d01_.overlaps(itemCSysPoint))
      {
        dragging_ = &d01_;
      }
      else
      {
        dragging_ = NULL;
      }

      xAnchor_ = rect_.xHole_;
      yAnchor_ = rect_.yHole_;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      BBox bbox;
      rect_.Item::get(kPropertyBBox, bbox);

      TVec2D origin = bbox.topLeft();
      TVec2D p0 = rootCSysDragStart - origin;
      TVec2D p1 = rootCSysDragEnd - origin;
      TVec2D drag = p1 - p0;

      double x0 = bbox.w_ * xAnchor_.origin_ + bbox.x_ - origin.x();
      double x1 = bbox.w_ * xAnchor_.length_ + x0;
      double y0 = bbox.h_ * yAnchor_.origin_ + bbox.y_ - origin.y();
      double y1 = bbox.h_ * yAnchor_.length_ + y0;

      if (!dragging_)
      {
        x0 = p0.x();
        x1 = p1.x();
        y0 = p0.y();
        y1 = p1.y();
      }
      else if (dragging_ == &d22_)
      {
        // dragging bottom-right corner:
        x1 += drag.x();
        y1 += drag.y();
      }
      else if (dragging_ == &d02_)
      {
        // dragging top-right corner:
        x1 += drag.x();
        y0 += drag.y();
      }
      else if (dragging_ == &d20_)
      {
        // dragging bottom-left corner:
        x0 += drag.x();
        y1 += drag.y();
      }
      else if (dragging_ == &d00_)
      {
        // dragging top-left corner:
        x0 += drag.x();
        y0 += drag.y();
      }
      else if (dragging_ == &d12_)
      {
        // dragging right edge:
        x1 += drag.x();
      }
      else if (dragging_ == &d10_)
      {
        // dragging left edge:
        x0 += drag.x();
      }
      else if (dragging_ == &d21_)
      {
        // dragging bottom edge:
        y1 += drag.y();
      }
      else if (dragging_ == &d01_)
      {
        // dragging top edge:
        y0 += drag.y();
      }

      if (x1 < x0)
      {
        std::swap(x0, x1);
      }

      if (y1 < y0)
      {
        std::swap(y0, y1);
      }

      double u0 = std::max(0.0, std::min(1.0, x0 / bbox.w_));
      double u1 = std::max(0.0, std::min(1.0, x1 / bbox.w_));
      double v0 = std::max(0.0, std::min(1.0, y0 / bbox.h_));
      double v1 = std::max(0.0, std::min(1.0, y1 / bbox.h_));

      Segment xCrop(u0, u1 - u0);
      Segment yCrop(v0, v1 - v0);
      view_.setCrop(xCrop, yCrop);

      return true;
    }

    FrameCropView & view_;
    DonutRect & rect_;

    // d00 d01 d02
    // d10     d12
    // d20 d21 d22
    //
    Item & d00_;
    Item & d01_;
    Item & d02_;
    Item & d10_;
    Item & d12_;
    Item & d20_;
    Item & d21_;
    Item & d22_;

    // a pointer to the dragged handle:
    Item * dragging_;

    // bounding box of the donut hole at the time of drag start:
    Segment xAnchor_;
    Segment yAnchor_;
  };

  //----------------------------------------------------------------
  // CanvasRendererItem::CanvasRendererItem
  //
  CanvasRendererItem::CanvasRendererItem(const char * id, Canvas::ILayer & l):
    Item(id),
    layer_(l)
  {}

  //----------------------------------------------------------------
  // CanvasRendererItem::paintContent
  //
  void
  CanvasRendererItem::paintContent() const
  {
    double x = this->left();
    double y = this->top();
    double w_max = this->width();
    double h_max = this->height();

    renderer_.paintImage(x, y, w_max, h_max);
  }

  //----------------------------------------------------------------
  // CanvasRendererItem::observe
  //
  void
  CanvasRendererItem::observe(Canvas * canvas, const TVideoFramePtr & frame)
  {
    frame_ = frame;

    const CanvasRenderer * renderer = canvas->canvasRenderer();

    bool skipColorConverter = renderer->skipColorConverter();
    if (renderer_.skipColorConverter() != skipColorConverter)
    {
      IOpenGLContext & context = *(layer_.context());
      renderer_.skipColorConverter(context, skipColorConverter);
    }

    if (layer_.isEnabled())
    {
      this->loadFrame();
    }

    TCropFrame crop;
    double w = 0.0;
    double h = 0.0;
    if (frame &&
        renderer->getCroppedFrame(crop) &&
        renderer->imageWidthHeight(w, h))
    {
      const VideoTraits & vtts = frame_->traits_;
      double dar =
        (w * double(vtts.visibleWidth_) / double(crop.w_)) /
        (h * double(vtts.visibleHeight_) / double(crop.h_));
      renderer_.overrideDisplayAspectRatio(dar);
    }
  }

  //----------------------------------------------------------------
  // CanvasRendererItem::loadFrame
  //
  void
  CanvasRendererItem::loadFrame()
  {
    IOpenGLContext & context = *(layer_.context());
    renderer_.loadFrame(context, frame_);
    uncache();
    layer_.requestRepaint();
  }


  //----------------------------------------------------------------
  // FrameCropView::FrameCropView
  //
  FrameCropView::FrameCropView():
    ItemView("frameCrop"),
    mainView_(NULL)
  {}

  //----------------------------------------------------------------
  // FrameCropView::init
  //
  void
  FrameCropView::init(ItemView * mainView)
  {
    mainView_ = mainView;
    Item & root = *root_;

    ExpressionItem & titleHeight = root.
      addHidden(new ExpressionItem("style_title_height",
                                   new StyleTitleHeight(*mainView)));

    ColorRef colorControlsBg = root.addExpr
      (style_color_ref(*mainView, &ItemViewStyle::bg_controls_));

    ColorRef colorTextBg = root.addExpr
      (style_color_ref(*mainView, &ItemViewStyle::bg_timecode_));

    ColorRef colorTextFg = root.addExpr
      (style_color_ref(*mainView, &ItemViewStyle::fg_timecode_));

    ItemViewStyle & style = *(mainView->style());

    // setup mouse trap to prevent unintended click-through to mainView:
    MouseTrap & mouseTrap = root.addNew<MouseTrap>("mouse_trap");
    mouseTrap.onScroll_ = false;
    mouseTrap.anchors_.fill(root);

    CanvasRendererItem & uncropped =
      root.add(new CanvasRendererItem("uncropped", *this));

    LetterBoxItem & letterbox =
      uncropped.addHidden
      (new LetterBoxItem
       ("letterbox", new LetterBoxExpr(root, uncropped.renderer_)));

    // anchors preserve uncropped frame aspect ratio:
    uncropped.anchors_.left_ = ItemRef::reference(letterbox, kPropertyLeft);
    uncropped.anchors_.top_ = ItemRef::reference(letterbox, kPropertyTop);
    uncropped.width_ = ItemRef::reference(letterbox, kPropertyWidth);
    uncropped.height_ = ItemRef::reference(letterbox, kPropertyHeight);

    DonutRect & donut = uncropped.addNew<DonutRect>("donut");
    donut.anchors_.fill(uncropped);
    donut.color_ = ColorRef::constant(Color(0x7f7f7f, 0.5));
    donut.xHole_ = Segment(0.0, 1.0);
    donut.yHole_ = Segment(0.0, 1.0);

    // d00 d01 d02
    // d10     d12
    // d20 d21 d22
    Rectangle & d00 = donut.addNew<Rectangle>("d00");
    d00.anchors_.left_ = ItemRef::offset(donut, kPropertyDonutHoleLeft, -20);
    d00.anchors_.top_ = ItemRef::offset(donut, kPropertyDonutHoleTop, -20);
    d00.width_ = ItemRef::constant(40);
    d00.height_ = ItemRef::constant(40);
    d00.color_ = ColorRef::constant(Color(0x7f7f7f, 0.3));

    Rectangle & d01 = donut.addNew<Rectangle>("d01");
    d01.anchors_.left_ = ItemRef::offset(donut, kPropertyDonutHoleLeft, 20);
    d01.anchors_.right_ =
      ItemRef::offset(donut, kPropertyDonutHoleRight, -20);
    d01.anchors_.top_ = d00.anchors_.top_;
    d01.height_ = d00.height_;
    d01.color_ = d00.color_;

    Rectangle & d02 = donut.addNew<Rectangle>("d02");
    d02.anchors_.left_ = d01.anchors_.right_;
    d02.anchors_.top_ = d00.anchors_.top_;
    d02.width_ = d00.width_;
    d02.height_ = d00.height_;
    d02.color_ = d00.color_;

    Rectangle & d10 = donut.addNew<Rectangle>("d10");
    d10.anchors_.left_ = d00.anchors_.left_;
    d10.anchors_.top_ = ItemRef::offset(donut, kPropertyDonutHoleTop, 20);
    d10.anchors_.bottom_ =
      ItemRef::offset(donut, kPropertyDonutHoleBottom, -20);
    d10.width_ = d00.width_;
    d10.color_ = d00.color_;

    Rectangle & d12 = donut.addNew<Rectangle>("d12");
    d12.anchors_.left_ = d02.anchors_.left_;
    d12.anchors_.top_ = d10.anchors_.top_;
    d12.anchors_.bottom_ = d10.anchors_.bottom_;
    d12.width_ = d00.width_;
    d12.color_ = d00.color_;

    Rectangle & d20 = donut.addNew<Rectangle>("d20");
    d20.anchors_.left_ = d00.anchors_.left_;
    d20.anchors_.top_ = d10.anchors_.bottom_;
    d20.width_ = d00.width_;
    d20.height_ = d00.height_;
    d20.color_ = d00.color_;

    Rectangle & d21 = donut.addNew<Rectangle>("d21");
    d21.anchors_.left_ = d01.anchors_.left_;
    d21.anchors_.right_ = d01.anchors_.right_;
    d21.anchors_.top_ = d20.anchors_.top_;
    d21.height_ = d00.height_;
    d21.color_ = d00.color_;

    Rectangle & d22 = donut.addNew<Rectangle>("d22");
    d22.anchors_.left_ = d02.anchors_.left_;
    d22.anchors_.top_ = d10.anchors_.bottom_;
    d22.width_ = d00.width_;
    d22.height_ = d00.height_;
    d22.color_ = d00.color_;

    DashedRect & outline = donut.addNew<DashedRect>("outline");
    outline.anchors_.left_ = ItemRef::offset(donut, kPropertyDonutHoleLeft, 1);
    outline.anchors_.top_ = ItemRef::offset(donut, kPropertyDonutHoleTop, 1);
    outline.width_ = ItemRef::offset(donut, kPropertyDonutHoleWidth, -2);
    outline.height_ = ItemRef::offset(donut, kPropertyDonutHoleHeight, -2);

    RoundRect & doneBg = donut.addNew<RoundRect>("done_bg");
    Text & done = donut.addNew<Text>("done");
    done.anchors_.right_ = ItemRef::offset(d22, kPropertyLeft);
    done.anchors_.bottom_ = ItemRef::offset(d22, kPropertyTop);
    done.margins_.
      set_right(ItemRef::reference(titleHeight, kPropertyExpression, 2.0));
    done.margins_.
      set_bottom(ItemRef::reference(titleHeight, kPropertyExpression, 1.0));
    done.color_ = ColorRef::constant(Color(0x000000, 1.0));
    done.text_ = TVarRef::constant(QVariant(tr("Done")));
    done.font_ = style.font_small_;
    done.fontSize_ = ItemRef::scale(titleHeight, kPropertyExpression, 0.5);

    doneBg.color_ = doneBg.addExpr(new ColorOnHover(*this, doneBg,
                                                    Color(0xffffff, 0.7),
                                                    Color(0xffffff, 1.0)));
    doneBg.colorBorder_ = ColorRef::constant(Color(0x000000, 1.0));
    doneBg.anchors_.fill(done);
    doneBg.margins_.
      set_left(ItemRef::scale(titleHeight, kPropertyExpression, -0.9));
    doneBg.margins_.
      set_top(ItemRef::scale(titleHeight, kPropertyExpression, -0.3));
    doneBg.margins_.set_right(doneBg.margins_.get_left());
    doneBg.margins_.set_bottom(doneBg.margins_.get_top());
    doneBg.radius_ = ItemRef::reference(doneBg, kPropertyHeight, 0.05, 3.0);
    doneBg.border_ = ItemRef::constant(1.0);

    RegionSelect & selector = root.add(new RegionSelect(*this,
                                                        donut,
                                                        d00, d01, d02,
                                                        d10,      d12,
                                                        d20, d21, d22));
    selector.anchors_.fill(root);

    Dismiss & dismiss = root.add(new Dismiss(*this));
    dismiss.anchors_.fill(doneBg);
    doneBg.addObserver(Item::kOnUncache,
                       Item::TObserverPtr(new Uncache(dismiss)));
  }

  //----------------------------------------------------------------
  // FrameCropView::style
  //
  ItemViewStyle *
  FrameCropView::style() const
  {
    return mainView_ ? mainView_->style() : NULL;
  }

  //----------------------------------------------------------------
  // FrameCropView::resizeTo
  //
  bool
  FrameCropView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }

    if (mainView_)
    {
      ItemViewStyle * style = mainView_->style();
      requestUncache(style);
    }

    return true;
  }

  //----------------------------------------------------------------
  // FrameCropView::processMouseTracking
  //
  bool
  FrameCropView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    Item & root = *root_;
    CanvasRendererItem & uncropped = root.get<CanvasRendererItem>("uncropped");
    DonutRect & donut = uncropped.get<DonutRect>("donut");
    requestUncache(&donut);

    return true;
 }

  //----------------------------------------------------------------
  // FrameCropView::processKeyEvent
  //
  bool
  FrameCropView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    e->ignore();

    QEvent::Type et = e->type();
    if (et == QEvent::KeyPress)
    {
      int key = e->key();

      if (key == Qt::Key_Return ||
          key == Qt::Key_Enter ||
          key == Qt::Key_Escape)
      {
        this->done();
        e->accept();
      }
    }

    return e->isAccepted();
  }

  //----------------------------------------------------------------
  // FrameCropView::setCrop
  //
  void
  FrameCropView::setCrop(const TVideoFramePtr & frame, const TCropFrame & crop)
  {
    const VideoTraits & vtts = frame->traits_;
    double w = double(vtts.visibleWidth_);
    double h = double(vtts.visibleHeight_);

    double ct = cos(M_PI * double(vtts.cameraRotation_) / 180.0);
    double st = sin(M_PI * double(vtts.cameraRotation_) / 180.0);
    TVec2D u_axis(ct, -st);
    TVec2D v_axis(st, ct);

    Segment xCrop(double(crop.x_) / w, double(crop.w_) / w);
    Segment yCrop(double(crop.y_) / h, double(crop.h_) / h);

    TVec2D p00(xCrop.start(), yCrop.start());
    TVec2D p10(xCrop.end(), yCrop.start());
    TVec2D p01(xCrop.start(), yCrop.end());
    TVec2D p11(xCrop.end(), yCrop.end());

    TVec2D center(0.5, 0.5);
    TVec2D q00 = center + wcs_to_lcs(center, u_axis, v_axis, p00);
    TVec2D q01 = center + wcs_to_lcs(center, u_axis, v_axis, p01);
    TVec2D q10 = center + wcs_to_lcs(center, u_axis, v_axis, p10);
    TVec2D q11 = center + wcs_to_lcs(center, u_axis, v_axis, p11);

    double u0 = std::min<double>(std::min<double>(q00.x(), q01.x()),
                                 std::min<double>(q10.x(), q11.x()));

    double u1 = std::max<double>(std::max<double>(q00.x(), q01.x()),
                                 std::max<double>(q10.x(), q11.x()));

    double v0 = std::min<double>(std::min<double>(q00.y(), q01.y()),
                                 std::min<double>(q10.y(), q11.y()));

    double v1 = std::max<double>(std::max<double>(q00.y(), q01.y()),
                                 std::max<double>(q10.y(), q11.y()));

    Segment u(u0, u1 - u0);
    Segment v(v0, v1 - v0);
    this->setCrop(u, v);
  }

  //----------------------------------------------------------------
  // FrameCropView::setCrop
  //
  void
  FrameCropView::setCrop(const Segment & xCrop, const Segment & yCrop)
  {
    Item & root = *root_;
    CanvasRendererItem & uncropped = root.get<CanvasRendererItem>("uncropped");
    DonutRect & donut = uncropped.get<DonutRect>("donut");

    if (donut.xHole_ == xCrop && donut.yHole_ == yCrop)
    {
      // nothing changed:
      return;
    }

    donut.xHole_ = xCrop;
    donut.yHole_ = yCrop;
    donut.uncache();
    this->requestRepaint();

    if (!uncropped.frame_)
    {
      YAE_ASSERT(false);
      return;
    }

    const VideoTraits & vtts = uncropped.frame_->traits_;
    double ct = cos(M_PI * double(vtts.cameraRotation_) / 180.0);
    double st = sin(M_PI * double(vtts.cameraRotation_) / 180.0);
    TVec2D u_axis(ct, st);
    TVec2D v_axis(-st, ct);

    TVec2D p00(xCrop.start(), yCrop.start());
    TVec2D p10(xCrop.end(), yCrop.start());
    TVec2D p01(xCrop.start(), yCrop.end());
    TVec2D p11(xCrop.end(), yCrop.end());

    TVec2D center(0.5, 0.5);
    TVec2D q00 = center + wcs_to_lcs(center, u_axis, v_axis, p00);
    TVec2D q01 = center + wcs_to_lcs(center, u_axis, v_axis, p01);
    TVec2D q10 = center + wcs_to_lcs(center, u_axis, v_axis, p10);
    TVec2D q11 = center + wcs_to_lcs(center, u_axis, v_axis, p11);

    double u0 = std::min<double>(std::min<double>(q00.x(), q01.x()),
                                 std::min<double>(q10.x(), q11.x()));

    double u1 = std::max<double>(std::max<double>(q00.x(), q01.x()),
                                 std::max<double>(q10.x(), q11.x()));

    double v0 = std::min<double>(std::min<double>(q00.y(), q01.y()),
                                 std::min<double>(q10.y(), q11.y()));

    double v1 = std::max<double>(std::max<double>(q00.y(), q01.y()),
                                 std::max<double>(q10.y(), q11.y()));

    Segment u(u0, u1 - u0);
    Segment v(v0, v1 - v0);

    TCropFrame crop;
    crop.x_ = u.origin_ * vtts.visibleWidth_;
    crop.y_ = v.origin_ * vtts.visibleHeight_;
    crop.w_ = u.length_ * vtts.visibleWidth_;
    crop.h_ = v.length_ * vtts.visibleHeight_;

    emit cropped(uncropped.frame_, crop);
  }

}
