// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:24:38 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>

// local:
#include "yaeDemuxerView.h"
#include "yaeFlickableArea.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTextInput.h"


namespace yae
{

  //----------------------------------------------------------------
  // IsMouseOver
  //
  struct IsMouseOver : public TBoolExpr
  {
    IsMouseOver(const ItemView & view,
                const Scrollview & sview,
                const Item & item):
      view_(view),
      sview_(sview),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      TVec2D origin;
      Segment xView;
      Segment yView;
      sview_.getContentView(origin, xView, yView);

      const TVec2D & pt = view_.mousePt();
      TVec2D lcs_pt = pt - origin;
      result = item_.overlaps(lcs_pt);
    }

    const ItemView & view_;
    const Scrollview & sview_;
    const Item & item_;
  };

  //----------------------------------------------------------------
  // ClearTextInput
  //
  struct ClearTextInput : public InputArea
  {
    ClearTextInput(const char * id, TextInput & edit, Text & view):
      InputArea(id),
      edit_(edit),
      view_(view)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      edit_.setText(QString());
      edit_.uncache();
      view_.uncache();
      return true;
    }

    TextInput & edit_;
    Text & view_;
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
  // RemuxLayoutGop
  //
  struct RemuxLayoutGop : public TLayout
  {
    void layout(RemuxModel & model, const std::size_t & index,
                RemuxView & view, const RemuxViewStyle & style,
                Item & root)
    {
      const Item * prev = NULL;
      const std::size_t n = std::size_t(1.0 + 29.0 * drand48());
      for (std::size_t i = 0; i <= n; ++i)
      {
        RoundRect & frame = root.addNew<RoundRect>("frame");
        frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
        frame.anchors_.left_ = prev ?
          ItemRef::reference(*prev, kPropertyRight) :
          ItemRef::reference(root, kPropertyLeft);
        frame.width_ = ItemRef::reference(style.title_height_, 16.0 / 9.0);
        frame.height_ = ItemRef::reference(style.title_height_);
        frame.radius_ = ItemRef::constant(3);
        frame.background_ = frame.addExpr(new ColorAttr(view, "bg_", 0));
        frame.color_ = frame.addExpr(new ColorAttr(view, "scrollbar_"));

        prev = &frame;
      }

    }
  };

  //----------------------------------------------------------------
  // RemuxLayoutGops
  //
  struct RemuxLayoutGops : public TLayout
  {
    void layout(RemuxModel & model, const std::size_t & index,
                RemuxView & view, const RemuxViewStyle & style,
                Item & root)
    {
      const Item * prev = NULL;
      for (std::size_t i = 0; i <= 59; ++i)
      {
        Item & gop = root.addNew<Item>("gop");
        gop.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        gop.anchors_.top_ = prev ?
          ItemRef::reference(*prev, kPropertyBottom) :
          ItemRef::reference(root, kPropertyTop);

        style.layout_gop_->layout(model, i, view, style, gop);
        prev = &gop;
      }
    }
  };

  //----------------------------------------------------------------
  // RemuxLayoutClip
  //
  struct RemuxLayoutClip : public TLayout
  {
    void layout(RemuxModel & model, const std::size_t & index,
                RemuxView & view, const RemuxViewStyle & style,
                Item & root)
    {
      root.height_ = ItemRef::reference(style.row_height_);

      const std::size_t num_clips = model.remux_.size();
      if (index == num_clips)
      {
        RoundRect & btn = root.addNew<RoundRect>("append");
        btn.border_ = ItemRef::constant(1.0);
        btn.radius_ = ItemRef::constant(3.0);
        btn.width_ = ItemRef::reference(root.height_, 0.8);
        btn.height_ = btn.width_;
        btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
        btn.anchors_.left_ = ItemRef::reference(root.height_, 1);
        btn.color_ = btn.addExpr(new ColorAttr(view, "bg_controls_"));

        Text & label = btn.addNew<Text>("label");
        label.anchors_.center(btn);
        label.text_ = TVarRef::constant(TVar("+"));
        return;
      }

      RoundRect & btn = root.addNew<RoundRect>("remove");
      btn.border_ = ItemRef::constant(1.0);
      btn.radius_ = ItemRef::constant(3.0);
      btn.width_ = ItemRef::reference(root.height_, 0.8);
      btn.height_ = btn.width_;
      btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
      btn.anchors_.hcenter_ = ItemRef::reference(root.height_, 1.5);
      btn.color_ = btn.addExpr(new ColorAttr(view, "bg_controls_"));

      Text & label = btn.addNew<Text>("label");
      label.anchors_.center(btn);
      label.text_ = TVarRef::constant(TVar("-"));
    }
  };

  //----------------------------------------------------------------
  // RemuxLayoutClips
  //
  struct RemuxLayoutClips : public TLayout
  {
    void layout(RemuxModel & model, const std::size_t &,
                RemuxView & view, const RemuxViewStyle & style,
                Item & root)
    {
      const Item * prev = NULL;
      for (std::size_t i = 0; i <= model.remux_.size(); ++i)
      {
        Rectangle & row = root.addNew<Rectangle>("clip");
        row.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        row.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
        row.anchors_.top_ = prev ?
          ItemRef::reference(*prev, kPropertyBottom) :
          ItemRef::reference(root, kPropertyTop);
        row.color_ = row.addExpr(new ColorAttr
                                 (view, "bg_controls_", (i % 2) ? 1.0 : 0.5));

        style.layout_clip_->layout(model, i, view, style, row);
        prev = &row;
      }
    }
  };

  //----------------------------------------------------------------
  // layout_scrollview
  //
  static Item &
  layout_scrollview(ScrollbarId scrollbars,
                    ItemView & view,
                    const RemuxViewStyle & style,
                    Item & root,
                    ScrollbarId inset = kScrollbarNone)
  {
    bool inset_h = (kScrollbarHorizontal & inset) == kScrollbarHorizontal;
    bool inset_v = (kScrollbarVertical & inset) == kScrollbarVertical;

    Scrollview & sview = root.
      addNew<Scrollview>((std::string(root.id_) + ".scrollview").c_str());

    Item & scrollbar = root.addNew<Item>("scrollbar");
    Item & hscrollbar = root.addNew<Item>("hscrollbar");

    scrollbar.anchors_.top_ = ItemRef::reference(sview, kPropertyTop);
    scrollbar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    scrollbar.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    scrollbar.visible_ = scrollbar.
      addExpr(new ScrollbarRequired
              (*sview.content_,
               kScrollbarVertical,

               // vertical scrollbar width:
               (scrollbars & kScrollbarVertical) == kScrollbarVertical ?
               ItemRef::uncacheable(style.title_height_, 0.2) :
               ItemRef::constant(inset_v ? -1.0 : 0.0),

               // horizontal scrollbar width:
               (scrollbars & kScrollbarHorizontal) == kScrollbarHorizontal ?
               ItemRef::uncacheable(style.title_height_, 0.2) :
               ItemRef::constant(inset_h ? -1.0 : 0.0),

               ItemRef::uncacheable(sview, kPropertyLeft),
               ItemRef::uncacheable(root, kPropertyRight),
               ItemRef::uncacheable(sview, kPropertyTop),
               ItemRef::uncacheable(root, kPropertyBottom)));

    scrollbar.width_ = scrollbar.addExpr
      (new Conditional<ItemRef>
       (scrollbar.visible_,
        ItemRef::uncacheable(style.title_height_, 0.2),
        ItemRef::constant(0.0)));

    hscrollbar.setAttr("vertical", false);
    hscrollbar.anchors_.left_ = ItemRef::reference(sview, kPropertyLeft);
    hscrollbar.anchors_.right_ = ItemRef::reference(scrollbar, kPropertyLeft);
    hscrollbar.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);

    hscrollbar.visible_ = hscrollbar.
      addExpr(new ScrollbarRequired
              (*sview.content_,
               kScrollbarHorizontal,

               (scrollbars & kScrollbarVertical) == kScrollbarVertical ?
               ItemRef::uncacheable(style.title_height_, 0.2) :
               ItemRef::constant(inset_v ? -1.0 : 0.0),

               (scrollbars & kScrollbarHorizontal) == kScrollbarHorizontal ?
               ItemRef::uncacheable(style.title_height_, 0.2) :
               ItemRef::constant(inset_h ? -1.0 : 0.0),

               ItemRef::uncacheable(sview, kPropertyLeft),
               ItemRef::uncacheable(root, kPropertyRight),
               ItemRef::uncacheable(sview, kPropertyTop),
               ItemRef::uncacheable(root, kPropertyBottom)));

    hscrollbar.height_ = hscrollbar.addExpr
      (new Conditional<ItemRef>
       (hscrollbar.visible_,
        ItemRef::uncacheable(style.title_height_, 0.2),
        ItemRef::constant(0.0)));

    sview.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
    sview.anchors_.top_ = ItemRef::reference(root, kPropertyTop);

    sview.anchors_.right_ =
      inset_v ?
      ItemRef::reference(root, kPropertyRight) :
      ItemRef::reference(scrollbar, kPropertyLeft);

    sview.anchors_.bottom_ =
      inset_h ?
      ItemRef::reference(root, kPropertyBottom) :
      ItemRef::reference(hscrollbar, kPropertyTop);

    Item & content = *(sview.content_);
    content.anchors_.left_ = ItemRef::constant(0.0);
    content.anchors_.top_ = ItemRef::constant(0.0);

    if ((scrollbars & kScrollbarHorizontal) != kScrollbarHorizontal)
    {
      content.width_ = ItemRef::reference(sview, kPropertyWidth);
    }

    FlickableArea & maScrollview =
      sview.add(new FlickableArea("ma_sview",
                                  view,
                                  &scrollbar,
                                  &hscrollbar));
    maScrollview.anchors_.fill(sview);

    InputArea & maScrollbar = scrollbar.addNew<InputArea>("ma_scrollbar");
    maScrollbar.anchors_.fill(scrollbar);

    // configure scrollbar slider:
    RoundRect & slider = scrollbar.addNew<RoundRect>("slider");
    slider.anchors_.top_ = slider.
      addExpr(new CalcSliderTop(sview, scrollbar, slider));
    slider.anchors_.left_ = ItemRef::offset(scrollbar, kPropertyLeft, 2);
    slider.anchors_.right_ = ItemRef::offset(scrollbar, kPropertyRight, -2);
    slider.height_ = slider.
      addExpr(new CalcSliderHeight(sview, scrollbar, slider));
    slider.radius_ = ItemRef::scale(slider, kPropertyWidth, 0.5);
    slider.background_ = slider.addExpr(new ColorAttr(view, "bg_", 0));
    slider.color_ = slider.addExpr(new ColorAttr(view, "scrollbar_"));

    SliderDrag & maSlider =
      slider.add(new SliderDrag("ma_slider", view, sview, scrollbar));
    maSlider.anchors_.fill(slider);

    InputArea & maHScrollbar = scrollbar.addNew<InputArea>("ma_hscrollbar");
    maHScrollbar.anchors_.fill(hscrollbar);

    // configure horizontal scrollbar slider:
    RoundRect & hslider = hscrollbar.addNew<RoundRect>("hslider");
    hslider.anchors_.top_ = ItemRef::offset(hscrollbar, kPropertyTop, 2);
    hslider.anchors_.bottom_ = ItemRef::offset(hscrollbar, kPropertyBottom, -2);
    hslider.anchors_.left_ =
      hslider.addExpr(new CalcSliderLeft(sview, hscrollbar, hslider));
    hslider.width_ =
      hslider.addExpr(new CalcSliderWidth(sview, hscrollbar, hslider));
    hslider.radius_ = ItemRef::scale(hslider, kPropertyHeight, 0.5);
    hslider.background_ = hslider.addExpr(new ColorAttr(view, "bg_", 0));
    hslider.color_ = hslider.addExpr(new ColorAttr(view, "scrollbar_"));

    SliderDrag & maHSlider =
      hslider.add(new SliderDrag("ma_hslider", view, sview, hscrollbar));
    maHSlider.anchors_.fill(hslider);

    return content;
  }

  //----------------------------------------------------------------
  // RemuxLayout
  //
  struct RemuxLayout : public TLayout
  {
    void layout(RemuxModel & model, const std::size_t &,
                RemuxView & view, const RemuxViewStyle & style,
                Item & root)
    {
      Rectangle & bg = root.addNew<Rectangle>("background");
      bg.anchors_.fill(root);
      bg.color_ = bg.addExpr(new ColorAttr(view, "bg_"));

      Item & gops = root.addNew<Item>("gops");
      Item & clips = root.addNew<Item>("clips");

      Rectangle & sep = root.addNew<Rectangle>("separator");
      sep.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      sep.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      // sep.anchors_.vcenter_ = ItemRef::scale(root, kPropertyHeight, 0.75);
      sep.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom, -100);
      sep.height_ = ItemRef::reference(style.title_height_, 0.1);
      sep.color_ = sep.
        addExpr(new ColorAttr(view, "fg_"));

      gops.anchors_.fill(root);
      gops.anchors_.bottom_ = ItemRef::reference(sep, kPropertyTop);

      clips.anchors_.fill(root);
      clips.anchors_.top_ = ItemRef::reference(sep, kPropertyBottom);

      Item & gops_container =
        layout_scrollview(kScrollbarBoth, view, style, gops,
                          kScrollbarBoth);

      Item & clips_container =
        layout_scrollview(kScrollbarVertical, view, style, clips);

      style.layout_clips_->layout(clips_container, view, model, style);
      style.layout_gops_->layout(gops_container, view, model, style);
    }
  };


  //----------------------------------------------------------------
  // RemuxViewStyle::RemuxViewStyle
  //
  RemuxViewStyle::RemuxViewStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view),
    layout_root_(new RemuxLayout()),
    layout_clips_(new RemuxLayoutClips()),
    layout_clip_(new RemuxLayoutClip()),
    layout_gops_(new RemuxLayoutGops()),
    layout_gop_(new RemuxLayoutGop())
  {
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_small_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_small_.setStyleHint(QFont::SansSerif);
    font_small_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    // main font:
    font_ = font_small_;
    font_large_ = font_small_;

    static bool hasImpact =
      QFontInfo(QFont("impact")).family().
      contains(QString::fromUtf8("impact"), Qt::CaseInsensitive);

    if (hasImpact)
    {
      font_large_.setFamily("impact");

#if !(defined(_WIN32) ||                        \
      defined(__APPLE__) ||                     \
      QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
      font_large_.setStretch(QFont::Condensed);
#endif
    }
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0)) || !defined(__APPLE__)
    else
#endif
    {
      font_large_.setStretch(QFont::Condensed);
      font_large_.setWeight(QFont::Black);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    font_fixed_.setHintingPreference(QFont::PreferFullHinting);
#endif

    font_fixed_.setFamily("Menlo, "
                          "Monaco, "
                          "Droid Sans Mono, "
                          "DejaVu Sans Mono, "
                          "Bitstream Vera Sans Mono, "
                          "Consolas, "
                          "Lucida Sans Typewriter, "
                          "Lucida Console, "
                          "Courier New");
    font_fixed_.setStyleHint(QFont::Monospace);
    font_fixed_.setFixedPitch(true);
    font_fixed_.setStyleStrategy((QFont::StyleStrategy)
                                 (QFont::PreferOutline |
                                  QFont::PreferAntialias |
                                  QFont::OpenGLCompatible));

    title_height_ = addExpr(new CalcTitleHeight(view, 50.0));
    title_height_.cachingEnabled_ = false;

    frame_width_ = ItemRef::reference(title_height_, 2.0);
    row_height_ = ItemRef::reference(title_height_, 0.5);
    font_size_ = ItemRef::reference(title_height_, 0.15);

    // color palette:
    bg_ = setStyleAttr("bg_", Color(0x1f1f1f, 0.87));
    fg_ = setStyleAttr("fg_", Color(0xffffff, 1.0));

    border_ = setStyleAttr("border_", Color(0x7f7f7f, 1.0));
    cursor_ = setStyleAttr("cursor_", Color(0xf12b24, 1.0));
    scrollbar_ = setStyleAttr("scrollbar_", Color(0x7f7f7f, 0.5));
    separator_ = setStyleAttr("separator_", scrollbar_.get());
    underline_ = setStyleAttr("underline_", cursor_.get());

    bg_timecode_ = setStyleAttr("bg_timecode_", Color(0x7f7f7f, 0.25));
    fg_timecode_ = setStyleAttr("fg_timecode_", Color(0xFFFFFF, 0.5));

    bg_controls_ =
      setStyleAttr("bg_controls_", bg_timecode_.get());
    fg_controls_ =
      setStyleAttr("fg_controls_", fg_timecode_.get().opaque(0.75));

    bg_focus_ = setStyleAttr("bg_focus_", Color(0x7f7f7f, 0.5));
    fg_focus_ = setStyleAttr("fg_focus_", Color(0xffffff, 1.0));

    bg_edit_selected_ =
      setStyleAttr("bg_edit_selected_", Color(0xffffff, 1.0));
    fg_edit_selected_ =
      setStyleAttr("fg_edit_selected_", Color(0x000000, 1.0));

    timeline_excluded_ =
      setStyleAttr("timeline_excluded_", Color(0xFFFFFF, 0.2));
    timeline_included_ =
      setStyleAttr("timeline_included_", Color(0xFFFFFF, 0.5));
  }

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView():
    ItemView("RemuxView"),
    style_("RemuxViewStyle", *this),
    model_(NULL)
  {}

  //----------------------------------------------------------------
  // RemuxView::setModel
  //
  void
  RemuxView::setModel(RemuxModel * model)
  {
    if (model_ == model)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!model_);

    model_ = model;

    // connect new model:
    /*
    bool ok = true;

    ok = connect(model_, SIGNAL(layoutChanged()),
                 this, SLOT(layoutChanged()));
    YAE_ASSERT(ok);

    ok = connect(model_, SIGNAL(dataChanged()),
                 this, SLOT(dataChanged()));
    YAE_ASSERT(ok);
    */

    /*
    Item & root = *root_;
    Scrollview & sview = root.get<Scrollview>("scrollview");
    Item & sviewContent = *(sview.content_);
    Item & footer = sviewContent["footer"];
    */
  }

  //----------------------------------------------------------------
  // RemuxView::processMouseEvent
  //
  bool
  RemuxView::processMouseEvent(Canvas * canvas, QMouseEvent * e)
  {
    bool processed = ItemView::processMouseEvent(canvas, e);
    /*
    QEvent::Type et = e->type();
    if (et == QEvent::MouseButtonPress &&
        (e->button() == Qt::LeftButton) &&
        !inputHandlers_.empty())
    {
      const TModelInputArea * ia = findModelInputArea(inputHandlers_);
      if (ia)
      {
        QModelIndex index = ia->modelIndex();
        int groupRow = -1;
        int itemRow = -1;
        RemuxModel::mapToGroupRowItemRow(index, groupRow, itemRow);

        if (!(groupRow < 0 || itemRow < 0))
        {
          // update selection:
          SelectionFlags selectionFlags = get_selection_flags(e);
          select_items(*model_, groupRow, itemRow, selectionFlags);
        }
      }
    }
    */
    return processed;
  }

  //----------------------------------------------------------------
  // RemuxView::resizeTo
  //
  bool
  RemuxView::resizeTo(const Canvas * canvas)
  {
    if (!ItemView::resizeTo(canvas))
    {
      return false;
    }
    /*
    if (model_)
    {
      QModelIndex currentIndex = model_->currentItem();

      Item & root = *root_;
      Scrollview & sview = root.get<Scrollview>("scrollview");
      FlickableArea & ma_sview = sview.get<FlickableArea>("ma_sview");

      if (!ma_sview.isAnimating())
      {
        ensureVisible(currentIndex);
      }
    }
    */
    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::layoutChanged
  //
  void
  RemuxView::layoutChanged()
  {
#if 0 // ndef NDEBUG
    std::cerr << "RemuxView::layoutChanged" << std::endl;
#endif

    TMakeCurrentContext currentContext(*context());

    if (pressed_)
    {
      if (dragged_)
      {
        InputArea * ia = dragged_->inputArea();
        if (ia)
        {
          ia->onCancel();
        }

        dragged_ = NULL;
      }

      InputArea * ia = pressed_->inputArea();
      if (ia)
      {
        ia->onCancel();
      }

      pressed_ = NULL;
    }
    inputHandlers_.clear();

    Item & root = *root_;
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);
    root.uncache();
    uncache_.clear();

    style_.layout_root_->layout(root, *this, *model_, style_);

#ifndef NDEBUG
    root.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // RemuxView::dataChanged
  //
  void
  RemuxView::dataChanged()
  {
    requestUncache();
    requestRepaint();
  }

}
