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
  // FrameColor
  //
  struct FrameColor : public TColorExpr
  {
    FrameColor(const Clip & clip,
               const Timespan & span,
               const Color & drop,
               const Color & keep):
      clip_(clip),
      span_(span),
      drop_(drop),
      keep_(keep)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool selected = clip_.keep_.contains(span_.t1_);
      result = selected ? keep_ : drop_;
    }

    const Clip & clip_;
    Timespan span_;
    Color drop_;
    Color keep_;
  };


  //----------------------------------------------------------------
  // DecodeFrame
  //
  struct YAE_API DecodeFrame : public AsyncTaskQueue::Task
  {
    DecodeFrame(// source:
                const TDemuxerInterfacePtr & demuxer,
                const DemuxerSummary & summary,
                const std::string & track_id,
                std::size_t gop_start,
                std::size_t gop_end,
                std::size_t frame,
                // output:
                TVideoFrameCache & cache):
      demuxer_(demuxer),
      summary_(summary),
      track_id_(track_id),
      gop_start_(gop_start),
      gop_end_(gop_end),
      frame_(frame),
      cache_(cache)
    {}

    virtual void run()
    {
      const Timeline::Track & track = summary_.get_track_timeline(track_id_);
      const TTime & pts = track.pts_[frame_];
      FrameId key(demuxer_.get(), track_id_, pts);

      if (cache_.get(key))
      {
        // frame is already available:
        return;
      }

      // decode and cache the entire GOP:
      decode_gop(// source:
                 demuxer_,
                 summary_,
                 track_id_,
                 gop_start_,
                 gop_end_,

                 // output:
                 256, // envelope width
                 256, // envelope height
                 0.0, // source DAR override
                 0.0, // output PAR override

                 // delivery:
                 &DecodeFrame::callback, this);
    }

    void decoded(const TVideoFramePtr & vf_ptr)
    {
      if (vf_ptr)
      {
        const TVideoFrame & vf = *vf_ptr;
        FrameId key(demuxer_.get(), track_id_, vf.time_);
        cache_.put(key, vf_ptr);
      }
    }

    static void callback(const TVideoFramePtr & vf_ptr, void * context)
    {
      DecodeFrame * task = (DecodeFrame *)context;
      task->decoded(vf_ptr);
    }

    TDemuxerInterfacePtr demuxer_;
    const DemuxerSummary & summary_;
    std::string track_id_;
    std::size_t gop_start_;
    std::size_t gop_end_;
    std::size_t frame_;
    TVideoFrameCache & cache_;
  };


  //----------------------------------------------------------------
  // VideoFrameItem::Private
  //
  struct VideoFrameItem::Private
  {
    Private(const TDemuxerInterfacePtr & demuxer,
            const DemuxerSummary & summary,
            const std::string & track_id,
            std::size_t gop_start,
            std::size_t gop_end,
            std::size_t frame,
            AsyncTaskQueue & queue,
            TVideoFrameCache & cache):
      demuxer_(demuxer),
      summary_(summary),
      track_id_(track_id),
      gop_start_(gop_start),
      gop_end_(gop_end),
      frame_(frame),
      queue_(queue),
      cache_(cache),
      layer_(NULL),
      failed_(false)
    {}

    static void
    callback(const boost::shared_ptr<AsyncTaskQueue::Task> & task, void * ctx)
    {
      Private * p = (Private *)ctx;
      p->done();
    }

    void done()
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      if (!async_)
      {
        // task was cancelled, ignore the results:
        return;
      }

      const Timeline::Track & track = summary_.get_track_timeline(track_id_);
      const TTime & pts = track.pts_[frame_];
      FrameId key(demuxer_.get(), track_id_, pts);
      ref_ = cache_.get(key);
      if (!ref_)
      {
        std::cerr << "FIXME: pkoshevoy: lookup failed: " << pts << std::endl;
        failed_ = true;
      }

      if (layer_)
      {
        layer_->delegate()->requestRepaint();
      }

      async_.reset();
    }

    void setContext(const Canvas::ILayer & layer)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      layer_ = &layer;
    }

    void paint(const VideoFrameItem & item)
    {
      boost::lock_guard<boost::mutex> lock(mutex_);
      if (!layer_)
      {
        return;
      }

      if (!ref_ || !ref_->value())
      {
        if (!async_ && !failed_)
        {
          async_.reset(new DecodeFrame(demuxer_,
                                       summary_,
                                       track_id_,
                                       gop_start_,
                                       gop_end_,
                                       frame_,
                                       cache_));
          queue_.add(async_, &VideoFrameItem::Private::callback, this);
        }

        return;
      }

      if (!renderer_)
      {
        renderer_.reset(new TLegacyCanvas());
        renderer_->loadFrame(*(layer_->context()), ref_->value());
      }

      double x = item.left();
      double y = item.top();
      double w_max = item.width();
      double h_max = item.height();
      double opacity = item.opacity_.get();
      renderer_->paintImage(x, y, w_max, h_max, opacity);
    }

    void unpaint()
    {
      async_.reset();
      ref_.reset();
      renderer_.reset();
    }

    TDemuxerInterfacePtr demuxer_;
    const DemuxerSummary & summary_;
    std::string track_id_;
    std::size_t gop_start_;
    std::size_t gop_end_;
    std::size_t frame_;
    AsyncTaskQueue & queue_;
    TVideoFrameCache & cache_;

    mutable boost::mutex mutex_;
    TVideoFrameCache::TRefPtr ref_;
    boost::shared_ptr<DecodeFrame> async_;

    const Canvas::ILayer * layer_;
    boost::shared_ptr<TLegacyCanvas> renderer_;
    bool failed_;
  };

  //----------------------------------------------------------------
  // VideoFrameItem::VideoFrameItem
  //
  VideoFrameItem::VideoFrameItem(const char * id,
                                 const TDemuxerInterfacePtr & demuxer,
                                 const DemuxerSummary & summary,
                                 const std::string & track_id,
                                 std::size_t gop_start,
                                 std::size_t gop_end,
                                 std::size_t frame,
                                 AsyncTaskQueue & queue,
                                 TVideoFrameCache & cache):
    Item(id),
    private_(new VideoFrameItem::Private(demuxer,
                                         summary,
                                         track_id,
                                         gop_start,
                                         gop_end,
                                         frame,
                                         queue,
                                         cache)),
    opacity_(ItemRef::constant(1.0))
  {}

  //----------------------------------------------------------------
  // VideoFrameItem::~VideoFrameItem
  //
  VideoFrameItem::~VideoFrameItem()
  {
    delete private_;
  }

  //----------------------------------------------------------------
  // VideoFrameItem::setContext
  //
  void
  VideoFrameItem::setContext(const Canvas::ILayer & view)
  {
    private_->setContext(view);
  }

  //----------------------------------------------------------------
  // VideoFrameItem::uncache
  //
  void
  VideoFrameItem::uncache()
  {
    opacity_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // VideoFrameItem::paintContent
  //
  void
  VideoFrameItem::paintContent() const
  {
    private_->paint(*this);
  }

  //----------------------------------------------------------------
  // VideoFrameItem::unpaintContent
  //
  void
  VideoFrameItem::unpaintContent() const
  {
    private_->unpaint();
  }



  //----------------------------------------------------------------
  // layout_gop
  //
  static void
  layout_gop(const Clip & clip,
             const Timeline::Track & track,
             RemuxView & view,
             const RemuxViewStyle & style,
             Item & root,
             std::size_t i0,
             std::size_t i1)
  {
    const Item * prev = NULL;
    for (std::size_t i = i0; i < i1; ++i)
    {
      RoundRect & frame = root.addNew<RoundRect>("frame");
      frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      frame.anchors_.left_ = prev ?
        frame.addExpr(new OddRoundUp(*prev, kPropertyRight)) :
        ItemRef::reference(root, kPropertyLeft, 0.0, 1);

      frame.height_ = ItemRef::reference(style.title_height_, 3.0);
      frame.width_ = ItemRef::reference(frame.height_, 16.0 / 9.0);
      frame.radius_ = ItemRef::constant(3);

      frame.background_ = frame.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));

      Timespan span(track.pts_[i], track.pts_[i] + track.dur_[i]);
      frame.color_ = frame.
        addExpr(new FrameColor(clip, span,
                               style.scrollbar_.get(),
                               style.cursor_.get()));

      VideoFrameItem & video = frame.add
        (new VideoFrameItem("video",
                            clip.src_,
                            clip.summary_,
                            clip.track_,
                            i0,
                            i1,
                            i,
                            view.async_,
                            view.frames_));
      video.anchors_.fill(frame, 2);
      video.setContext(view);

      Text & t0 = frame.addNew<Text>("t0");
      t0.font_ = style.font_large_;
      t0.anchors_.top_ = ItemRef::reference(frame, kPropertyTop, 1, 5);
      t0.anchors_.left_ = ItemRef::reference(frame, kPropertyLeft, 1, 5);
      t0.text_ = TVarRef::constant(TVar(span.t0_.to_hhmmss_ms().c_str()));
      t0.fontSize_ = ItemRef::reference(style.font_size_, 1.25 * kDpiScale);
#if defined(__APPLE__)
      t0.supersample_ = t0.addExpr(new Supersample<Text>(t0));
#endif
      t0.elide_ = Qt::ElideNone;
      t0.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      t0.background_ = frame.color_;

      prev = &frame;
    }
  }

  //----------------------------------------------------------------
  // RemuxLayoutGops
  //
  struct RemuxLayoutGops : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
    {
      if (model.remux_.size() <= model.current_)
      {
        return;
      }

      const Clip & clip = *(model.remux_[model.current_]);

      const Timeline::Track & track =
        clip.summary_.get_track_timeline(clip.track_);

      const Item * prev = NULL;
      for (std::set<std::size_t>::const_iterator
             i = track.keyframes_.begin(); i != track.keyframes_.end(); ++i)
      {
        Item & gop = root.addNew<Item>("gop");
        gop.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        gop.anchors_.top_ = prev ?
          gop.addExpr(new OddRoundUp(*prev, kPropertyBottom)) :
          ItemRef::reference(root, kPropertyTop, 0.0, 1);

        std::set<std::size_t>::const_iterator i1 = i;
        std::advance(i1, 1);

        layout_gop(clip, track, view, style, gop,
                   *i, i1 == track.keyframes_.end() ? track.dts_.size() : *i1);
        prev = &gop;
      }
    }
  };

  //----------------------------------------------------------------
  // layout_clip
  //
  static void
  layout_clip(RemuxModel & model,
              RemuxView & view,
              const RemuxViewStyle & style,
              Item & root,
              std::size_t index)
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
      btn.anchors_.left_ = ItemRef::reference(root.height_, 1.6);
      btn.color_ = btn.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

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
    btn.anchors_.left_ = ItemRef::reference(root.height_, 0.6);
    btn.color_ = btn.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

    Text & label = btn.addNew<Text>("label");
    label.anchors_.center(btn);
    label.text_ = TVarRef::constant(TVar("-"));
  }

  //----------------------------------------------------------------
  // RemuxLayoutClips
  //
  struct RemuxLayoutClips : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
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
        row.color_ = row.addExpr(style_color_ref
                                 (view,
                                  &ItemViewStyle::bg_controls_,
                                  (i % 2) ? 1.0 : 0.5));

        layout_clip(model, view, style, row, i);
        prev = &row;
      }
    }
  };

  //----------------------------------------------------------------
  // RemuxLayout
  //
  struct RemuxLayout : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & root,
                void * context)
    {
      Rectangle & bg = root.addNew<Rectangle>("background");
      bg.anchors_.fill(root);
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_));

      Item & gops = root.addNew<Item>("gops");
      Item & clips = root.addNew<Item>("clips");

      Rectangle & sep = root.addNew<Rectangle>("separator");
      sep.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      sep.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      // sep.anchors_.vcenter_ = ItemRef::scale(root, kPropertyHeight, 0.75);
      sep.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom, -100);
      sep.height_ = ItemRef::reference(style.title_height_, 0.1);
      sep.color_ = sep.addExpr(style_color_ref(view, &ItemViewStyle::fg_));

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
    layout_gops_(new RemuxLayoutGops())
  {
    row_height_ = ItemRef::reference(title_height_, 0.55);
  }

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView():
    ItemView("RemuxView"),
    style_("RemuxViewStyle", *this),
    model_(NULL)
  {
    frames_.set_capacity(8192);
  }

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
    root.children_.clear();
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
