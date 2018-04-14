// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:24:38 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>
#include <QFontMetricsF>

// local:
#include "yaeDemuxerView.h"
#include "yaeFlickableArea.h"
#include "yaeItemFocus.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTextInput.h"


namespace yae
{

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
  // VideoFrameItem::VideoFrameItem
  //
  VideoFrameItem::VideoFrameItem(const char * id, std::size_t frame):
    Item(id),
    frame_(frame),
    opacity_(ItemRef::constant(1.0))
  {}

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
    if (!renderer_)
    {
      GopItem & gop_item = ancestor<GopItem>();
      const Canvas::ILayer * layer = gop_item.getLayer();
      if (!layer)
      {
        return;
      }

      TGopCache::TRefPtr cached = gop_item.cached();
      if (!cached)
      {
        return;
      }

      const Gop & gop = gop_item.gop();
      std::size_t i = frame_ - gop.i0_;
      const TVideoFrames & frames = *(cached->value());
      if (frames.size() <= i || !frames[i])
      {
        return;
      }

      renderer_.reset(new TLegacyCanvas());
      renderer_->loadFrame(*(layer->context()), frames[i]);

      Text & pts = parent_->get<Text>("pts");
      pts.uncache();
    }

    double x = left();
    double y = top();
    double w_max = width();
    double h_max = height();
    double opacity = opacity_.get();
    renderer_->paintImage(x, y, w_max, h_max, opacity);
  }

  //----------------------------------------------------------------
  // VideoFrameItem::unpaintContent
  //
  void
  VideoFrameItem::unpaintContent() const
  {
    renderer_.reset();
  }

  //----------------------------------------------------------------
  // VideoFrameItem::videoFrame
  //
  TVideoFramePtr
  VideoFrameItem::videoFrame() const
  {
    TVideoFramePtr vf_ptr;

    if (renderer_)
    {
      renderer_->getFrame(vf_ptr);
    }

    return vf_ptr;
  }


  //----------------------------------------------------------------
  // HasFramePts
  //
  struct HasFramePts : public TBoolExpr
  {
    HasFramePts(const VideoFrameItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      TVideoFramePtr vf_ptr = item_.videoFrame();
      result = !!vf_ptr;
    }

    const VideoFrameItem & item_;
  };

  //----------------------------------------------------------------
  // GetFramePts
  //
  struct GetFramePts : public TVarExpr
  {
    GetFramePts(const VideoFrameItem & item):
      item_(item)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      TVideoFramePtr vf_ptr = item_.videoFrame();
      if (vf_ptr)
      {
        result = QString::fromUtf8(vf_ptr->time_.to_hhmmss_ms().c_str());
      }
      else
      {
        result = QVariant();
      }
    }

    const VideoFrameItem & item_;
  };


  //----------------------------------------------------------------
  // async_task_queue
  //
  static AsyncTaskQueue &
  async_task_queue()
  {
    static AsyncTaskQueue queue;
    return queue;
  }

  //----------------------------------------------------------------
  // gop_cache
  //
  static TGopCache &
  gop_cache()
  {
    static TGopCache cache(1024);
    return cache;
  }

  //----------------------------------------------------------------
  // get_pts_order_lut
  //
  // GOP members are naturally stored in DTS order.
  //
  // Use this to generate an index mapping lookup table
  // to access GOP members in PTS order.
  //
  static void
  get_pts_order_lut(const Gop & gop, std::vector<std::size_t> & lut)
  {
    const Timeline::Track & track =
      gop.demuxer_->summary().get_track_timeline(gop.track_);

    // sort the gop so the pts is in ascending order:
    std::set<std::pair<TTime, std::size_t> > sorted_pts;
    for (std::size_t i = gop.i0_; i < gop.i1_; i++)
    {
      const TTime & pts = track.pts_[i];
      sorted_pts.insert(std::make_pair(pts, i));
    }

    lut.resize(gop.i1_ - gop.i0_);
    std::size_t j = 0;

    for (std::set<std::pair<TTime, std::size_t> >::const_iterator
           i = sorted_pts.begin(); i != sorted_pts.end(); ++i, j++)
    {
      lut[j] = i->second;
    }
  }

  //----------------------------------------------------------------
  // DecodeGop
  //
  struct YAE_API DecodeGop : public AsyncTaskQueue::Task
  {
    DecodeGop(const boost::weak_ptr<Item> & item, const Gop & gop):
      item_(item),
      gop_(gop),
      frames_(new TVideoFrames())
    {}

    virtual void run()
    {
      TGopCache & cache = gop_cache();
      if (cache.get(gop_))
      {
        // GOP is already cached:
        return;
      }

      // decode and cache the entire GOP:
      decode_gop(// source:
                 gop_.demuxer_,
                 gop_.track_,
                 gop_.i0_,
                 gop_.i1_,

                 // output:
                 128, // envelope width
                 128, // envelope height
                 0.0, // source DAR override
                 (64.0 * 16.0 / 9.0) / 128.0, // output PAR override

                 // delivery:
                 &DecodeGop::callback, this);

      // cache the decoded frames, don't bother to match them
      // to the packets because that's not useful anyway:
      cache.put(gop_, frames_);
    }

    static void callback(const TVideoFramePtr & vf_ptr, void * context)
    {
      if (!vf_ptr)
      {
        YAE_ASSERT(false);
        return;
      }

      // collect the decoded frames:
      DecodeGop & task = *((DecodeGop *)context);
      task.frames_->push_back(vf_ptr);

      const TVideoFrame & vf = *vf_ptr;
      task.pts_.t0_ = std::min(task.pts_.t0_, vf.time_);
      task.pts_.t1_ = std::max(task.pts_.t1_, vf.time_);
      task.fps_.push(vf.time_);
    }

    inline ItemPtr item() const
    { return item_.lock(); }

  protected:
    boost::weak_ptr<Item> item_;
    Gop gop_;
    TVideoFramesPtr frames_;
    FramerateEstimator fps_;
    Timespan pts_;
  };


  //----------------------------------------------------------------
  // GopItem::GopItem
  //
  GopItem::GopItem(const char * id, const Gop & gop):
    Item(id),
    gop_(gop),
    layer_(NULL),
    failed_(false)
  {}

  //----------------------------------------------------------------
  // GopItem::setContext
  //
  void
  GopItem::setContext(const Canvas::ILayer & view)
  {
    layer_ = &view;
  }

  //----------------------------------------------------------------
  // GopItem::paintContent
  //
  void
  GopItem::paintContent() const
  {
    YAE_ASSERT(layer_);

    boost::lock_guard<boost::mutex> lock(mutex_);
    if (cached_ || async_ || failed_)
    {
      return;
    }

    AsyncTaskQueue & queue = async_task_queue();
    async_.reset(new DecodeGop(self_, gop_));
    queue.add(async_, &GopItem::cb);
  }

  //----------------------------------------------------------------
  // GopItem::unpaintContent
  //
  void
  GopItem::unpaintContent() const
  {
    boost::lock_guard<boost::mutex> lock(mutex_);
    async_.reset();
    cached_.reset();
  }

  //----------------------------------------------------------------
  // GopItem::cb
  //
  void
  GopItem::cb(const boost::shared_ptr<AsyncTaskQueue::Task> & task_ptr, void *)
  {
    boost::shared_ptr<DecodeGop> task =
      boost::dynamic_pointer_cast<DecodeGop>(task_ptr);
    if (!task)
    {
      YAE_ASSERT(false);
      return;
    }

    ItemPtr item_ptr = task->item();
    if (!item_ptr)
    {
      // task owner no longer exists, ignore the results:
      return;
    }

    // shortcut:
    GopItem & item = dynamic_cast<GopItem &>(*item_ptr);

    boost::lock_guard<boost::mutex> lock(item.mutex_);
    if (!item.async_)
    {
      // task was cancelled, ignore the results:
      return;
    }

    // cleanup the async task:
    item.async_.reset();

    TGopCache & cache = gop_cache();
    item.cached_ = cache.get(item.gop_);

    if (!item.cached_)
    {
      std::cerr << "decoding failed: " << item.gop_ << std::endl;
      item.failed_ = true;
    }

    if (item.layer_)
    {
      // update the UI:
      item.layer_->delegate()->requestRepaint();
    }
  }

  //----------------------------------------------------------------
  // layout_gop
  //
  static void
  layout_gop(const Clip & clip,
             const Timeline::Track & track,
             RemuxView & view,
             const RemuxViewStyle & style,
             GopItem & root,
             const Gop & gop)
  {
    std::vector<std::size_t> lut;
    get_pts_order_lut(gop, lut);

    for (std::size_t i = gop.i0_; i < gop.i1_; i++)
    {
      std::size_t k = i - gop.i0_;
      std::size_t j = lut[k];

      RoundRect & frame = root.addNew<RoundRect>("frame");
      frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      frame.anchors_.left_ =
        ItemRef::reference(root, kPropertyLeft, 1, 1 + k * 130);

      // frame.height_ = ItemRef::reference(style.title_height_, 3.0);
      // frame.width_ = ItemRef::reference(frame.height_, 16.0 / 9.0);
      frame.width_ = ItemRef::constant(130);
      frame.height_ = ItemRef::constant(100);
      frame.radius_ = ItemRef::constant(3);

      frame.background_ = frame.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));

      Timespan span(track.pts_[j], track.pts_[j] + track.dur_[j]);
      frame.color_ = frame.
        addExpr(new FrameColor(clip, span,
                               style.cursor_.get(),
                               style.scrollbar_.get()));
      frame.color_.cachingEnabled_ = false;

      VideoFrameItem & video = frame.add(new VideoFrameItem("video", i));
      video.anchors_.fill(frame, 1);

      Text & dts = frame.addNew<Text>("dts");
      dts.font_ = style.font_large_;
      dts.anchors_.top_ = ItemRef::reference(frame, kPropertyTop, 1, 5);
      dts.anchors_.left_ = ItemRef::reference(frame, kPropertyLeft, 1, 5);
#if 1
      dts.text_ = TVarRef::constant(TVar(track.dts_[j].to_hhmmss_ms().c_str()));
#else
      dts.text_ = TVarRef::constant(TVar(track.pts_[j].to_hhmmss_ms().c_str()));
#endif
      dts.fontSize_ = ItemRef::constant(9.5 * kDpiScale);
      dts.elide_ = Qt::ElideNone;
      dts.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      dts.background_ = frame.color_;

      Text & pts = frame.addNew<Text>("pts");
      pts.font_ = style.font_large_;
#if 0
      pts.anchors_.bottom_ = ItemRef::reference(frame, kPropertyBottom, 1, -5);
      pts.anchors_.right_ = ItemRef::reference(frame, kPropertyRight, 1, -5);
#else
      pts.anchors_.bottom_ = ItemRef::reference(frame, kPropertyBottom, 1, -5);
      pts.anchors_.left_ = ItemRef::reference(frame, kPropertyLeft, 1, 5);
#endif
      pts.visible_ = pts.addExpr(new HasFramePts(video));
      pts.text_ = pts.addExpr(new GetFramePts(video));
      pts.fontSize_ = dts.fontSize_;
      pts.elide_ = Qt::ElideNone;
      pts.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      pts.background_ = frame.color_;
    }
  }

  //----------------------------------------------------------------
  // IsClipSelected
  //
  struct IsClipSelected : TBoolExpr
  {
    IsClipSelected(const RemuxModel & model, const TClipPtr & clip):
      model_(model),
      clip_(clip)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = model_.selected_clip() == clip_;
    }

    const RemuxModel & model_;
    TClipPtr clip_;
  };


  //----------------------------------------------------------------
  // layout_gops
  //
  static void
  layout_gops(RemuxModel & model,
              RemuxView & view,
              const RemuxViewStyle & style,
              Item & container,
              const TClipPtr & clip_ptr)
  {
    Item & root = container.addNew<Item>("clip_layout");
    root.anchors_.fill(container);
    root.visible_ = root.addExpr(new IsClipSelected(model, clip_ptr));

    const Clip & clip = *clip_ptr;

    const Timeline::Track & track =
      clip.demuxer_->summary().get_track_timeline(clip.track_);

    Item & gops = layout_scrollview(kScrollbarBoth, view, style, root,
                                    kScrollbarBoth);

    Scrollview & sv = root.get<Scrollview>("clip_layout.scrollview");
    sv.uncacheContent_ = false;

    std::size_t row = 0;
    for (std::set<std::size_t>::const_iterator i = track.keyframes_.begin();
         i != track.keyframes_.end(); ++i, row++)
    {
      std::set<std::size_t>::const_iterator next = i;
      std::advance(next, 1);

      std::size_t i0 = *i;
      std::size_t i1 =
        (next == track.keyframes_.end()) ?
        track.dts_.size() :
        *next;

      Gop gop(clip.demuxer_, clip.track_, i0, i1);

      GopItem & item = gops.add<GopItem>(new GopItem("gop", gop));
      item.setContext(view);
      item.anchors_.left_ = ItemRef::reference(gops, kPropertyLeft);
      item.anchors_.top_ =
        ItemRef::reference(gops, kPropertyTop, 1, 1 + row * 100);

      layout_gop(clip, track, view, style, item, gop);
    }
  }

  //----------------------------------------------------------------
  // GetClipName
  //
  struct GetClipName : public TVarExpr
  {
    GetClipName(const RemuxModel & model, std::size_t index):
      model_(model),
      index_(index)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      const Clip & clip = *(model_.clips_[index_]);
      const std::string & name = yae::at(model_.source_, clip.demuxer_);

      std::string dirname;
      std::string basename;
      parseFilePath(name, dirname, basename);

      result = QString::fromUtf8(basename.c_str());
    }

    const RemuxModel & model_;
    const std::size_t index_;
  };

  //----------------------------------------------------------------
  // RemoveClip
  //
  struct RemoveClip : public InputArea
  {
    RemoveClip(const char * id, RemuxView & view, std::size_t index):
      InputArea(id),
      view_(view),
      index_(index)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.remove_clip(index_);
      return true;
    }

    RemuxView & view_;
    std::size_t index_;
  };

  //----------------------------------------------------------------
  // GetTimecodeText
  //
  struct GetTimecodeText : public TVarExpr
  {
    GetTimecodeText(const RemuxModel & model,
                    std::size_t index,
                    TTime Timespan::* field):
      model_(model),
      index_(index),
      field_(field)
    {}

    // virtual:
    void evaluate(TVar & result) const
    {
      if (index_ < model_.clips_.size())
      {
        const Clip & clip = *(model_.clips_[index_]);
        const TTime & t = clip.keep_.*field_;
        result = QString::fromUtf8(t.to_hhmmss_ms().c_str());
      }
      else
      {
        result = QVariant();
      }
    }

    const RemuxModel & model_;
    const std::size_t index_;
    TTime Timespan::* field_;
  };

  //----------------------------------------------------------------
  // str
  //
  template <typename TData>
  inline static std::string
  str(const std::string & a, const TData & b)
  {
    std::ostringstream oss;
    oss << a << b;
    return oss.str();
  }

  //----------------------------------------------------------------
  // str
  //
  template <typename TData>
  inline static std::string
  str(const char * a, const TData & b)
  {
    std::ostringstream oss;
    oss << a << b;
    return oss.str();
  }

  //----------------------------------------------------------------
  // TextEdit
  //
  struct TextEdit
  {
    TextEdit(Rectangle * bg = NULL,
             Text * text = NULL,
             TextInput * edit = NULL,
             TextInputProxy * focus = NULL):
      bg_(bg),
      text_(text),
      edit_(edit),
      focus_(focus)
    {}

    TextEdit(Rectangle & bg,
             Text & text,
             TextInput & edit,
             TextInputProxy & focus):
      bg_(&bg),
      text_(&text),
      edit_(&edit),
      focus_(&focus)
    {}

    Rectangle * bg_;
    Text * text_;
    TextInput * edit_;
    TextInputProxy * focus_;
  };

  //----------------------------------------------------------------
  // layout_timeedit
  //
  static TextEdit
  layout_timeedit(RemuxModel & model,
                  RemuxView & view,
                  const RemuxViewStyle & style,
                  Item & root,
                  std::size_t index,
                  TTime Timespan::* field)
  {
    int subindex = (field == &Timespan::t0_) ? 0 : 1;
    int focus_index = index * 2 + subindex;

    Rectangle & text_bg = root.addNew<Rectangle>
      (str("text_bg_", focus_index).c_str());

    Text & text = root.addNew<Text>
      (str("text_", focus_index).c_str());

    TextInput & edit = root.addNew<TextInput>
      (str("edit_", focus_index).c_str());

    TextInputProxy & focus = root.
      add(new TextInputProxy(str("focus_", focus_index).c_str(), text, edit));

    focus.anchors_.fill(text_bg);
    focus.copyViewToEdit_ = true;
    focus.bgNoFocus_ = ColorRef::constant(Color(0, 0.0));
    focus.bgOnFocus_ = ColorRef::constant(Color(0, 0.3));

    ItemFocus::singleton().setFocusable(view, focus, focus_index);

    text.anchors_.vcenter_ =
      text.addExpr(new OddRoundUp(root, kPropertyVCenter), 1.0, -1);

    text.visible_ = text.addExpr(new ShowWhenFocused(focus, false));
    text.color_ = style.fg_timecode_;
    text.background_ = ColorRef::transparent(focus, kPropertyColorNoFocusBg);
    text.text_ = text.addExpr(new GetTimecodeText(model, index, field));
    text.font_ = style.font_fixed_;
    text.fontSize_ = ItemRef::scale(root, kPropertyHeight, 0.285);

    text_bg.anchors_.offset(text, -3, 3, -3, 3);
    text_bg.color_ = text_bg.addExpr(new ColorWhenFocused(focus));
    text_bg.color_.cachingEnabled_ = false;

    edit.anchors_.fill(text);
    edit.margins_.right_ = ItemRef::scale(edit, kPropertyCursorWidth, -1.0);
    edit.visible_ = edit.addExpr(new ShowWhenFocused(focus, true));

    edit.color_ = style.fg_timecode_;
    edit.background_ = ColorRef::transparent(focus, kPropertyColorOnFocusBg);
    edit.cursorColor_ = style.cursor_;
    edit.font_ = text.font_;
    edit.fontSize_ = text.fontSize_;
    edit.selectionBg_ = style.bg_edit_selected_;
    edit.selectionFg_ = style.fg_edit_selected_;

    TextEdit r(text_bg, text, edit, focus);
    return r;
  }

  //----------------------------------------------------------------
  // TimelinePos
  //
  struct TimelinePos : public TDoubleExpr
  {
    TimelinePos(const Item & timeline,
                const RemuxModel & model,
                std::size_t index,
                TTime Timespan::* field):
      timeline_(timeline),
      model_(model),
      index_(index),
      field_(field)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      if (index_ >= model_.clips_.size())
      {
        YAE_ASSERT(false);
        return;
      }

      const Clip & clip = *(model_.clips_[index_]);
      const TTime & tt = clip.keep_.*field_;

      const Timeline::Track & track =
        clip.demuxer_->summary().get_track_timeline(clip.track_);

      const TTime & t0 = track.pts_span_.front().t0_;
      const TTime & t1 = track.pts_span_.back().t1_;

      double dt = (t1 - t0).sec();
      double t = (tt - t0).sec();

      double x0 = timeline_.left();
      double w = timeline_.width();
      double s = dt <= 0 ? 0.0 : (t / dt);
      result = x0 + s * w;
    }

    const Item & timeline_;
    const RemuxModel & model_;
    std::size_t index_;
    TTime Timespan::* field_;
  };

  //----------------------------------------------------------------
  // TimelineHeight
  //
  struct TimelineHeight : public TDoubleExpr
  {
    TimelineHeight(ItemView & view, Item & container, Item & timeline):
      view_(view),
      container_(container),
      timeline_(timeline)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      int h = std::max<int>(1, ~1 & (int(0.5 + timeline_.height()) / 8));

      const std::list<VisibleItem> & items = view_.mouseOverItems();
      if (yae::find(items, container_) != items.end())
      {
        h *= 2;
      }

      result = double(h);
    }

    ItemView & view_;
    Item & container_;
    Item & timeline_;
  };

  //----------------------------------------------------------------
  // IsMouseOverItem
  //
  struct IsMouseOverItem : public TBoolExpr
  {
    IsMouseOverItem(const ItemView & view, const Item & item):
      view_(view),
      item_(item)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      const std::list<VisibleItem> & items = view_.mouseOverItems();
      std::list<VisibleItem>::const_iterator found = yae::find(items, item_);
      result = (found != items.end());
    }

    const ItemView & view_;
    const Item & item_;
  };

  //----------------------------------------------------------------
  // TimelineSlider
  //
  struct TimelineSlider : public InputArea
  {
    TimelineSlider(const char * id,
                   RemuxView & view,
                   Item & timeline,
                   RemuxModel & model,
                   std::size_t index,
                   TTime Timespan::* field):
      InputArea(id),
      view_(view),
      timeline_(timeline),
      model_(model),
      index_(index),
      field_(field)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (index_ >= model_.clips_.size())
      {
        YAE_ASSERT(false);
        return false;
      }

      const Clip & clip = *(model_.clips_[index_]);
      const Timeline::Track & track =
        clip.demuxer_->summary().get_track_timeline(clip.track_);

      t0_ = track.pts_span_.front().t0_;
      const TTime & t1 = track.pts_span_.back().t1_;
      dt_ = (t1 - t0_).sec();
      if (dt_ <= 0)
      {
        YAE_ASSERT(false);
        return false;
      }

      const TTime & tp = clip.keep_.*field_;
      const bool is_t0 = (&tp == &clip.keep_.t0_);
      const TTime & tq = is_t0 ? clip.keep_.t1_ : clip.keep_.t0_;

      min_ = ((is_t0 ? t0_ : tq) - t0_).sec() / dt_;
      max_ = ((is_t0 ? tq : t1) - t0_).sec() / dt_;
      pos_ = (tp - t0_).sec() / dt_;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double w = timeline_.width();
      double dx = rootCSysDragEnd.x() - rootCSysDragStart.x();
      double pos = std::min(max_, std::max(min_, pos_ + dx / w));

      Clip & clip = *(model_.clips_[index_]);
      TTime & t = clip.keep_.*field_;
      t = t0_ + pos * dt_;
      view_.requestUncache(timeline_.parent_);
      view_.requestRepaint();
      return true;
    }

    RemuxView & view_;
    Item & timeline_;
    RemuxModel & model_;
    std::size_t index_;
    TTime Timespan::* field_;
    TTime t0_;
    double dt_;
    double pos_;
    double min_;
    double max_;
  };

  //----------------------------------------------------------------
  // layout_timeline
  //
  static void
  layout_timeline(RemuxModel & model,
                  RemuxView & view,
                  const RemuxViewStyle & style,
                  Item & root,
                  std::size_t index)
  {
    TextEdit t0 =
      layout_timeedit(model, view, style, root, index, &Timespan::t0_);

    t0.text_->anchors_.left_ = ItemRef::offset(root, kPropertyLeft, 3);
    t0.text_->margins_.left_ = ItemRef::reference(root, kPropertyHeight, 0.1);
    bool ok = view.connect(t0.edit_, SIGNAL(editingFinished(const QString &)),
                           &view.t0_, SLOT(map()));
    YAE_ASSERT(ok);
    view.t0_.setMapping(t0.edit_, int(index));

    TextEdit t1 =
      layout_timeedit(model, view, style, root, index, &Timespan::t1_);

    t1.text_->anchors_.right_ = ItemRef::offset(root, kPropertyRight, -3);
    t1.text_->margins_.right_ = ItemRef::reference(root, kPropertyHeight, 0.5);

    ok = view.connect(t1.edit_, SIGNAL(editingFinished(const QString &)),
                      &view.t1_, SLOT(map()));
    YAE_ASSERT(ok);
    view.t1_.setMapping(t1.edit_, int(index));

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.fill(root);
    timeline.anchors_.left_ = ItemRef::reference(*t0.bg_, kPropertyRight);
    timeline.anchors_.right_ = ItemRef::reference(*t1.bg_, kPropertyLeft);
    timeline.margins_.left_ = ItemRef::reference(root, kPropertyHeight, 0.5);
    timeline.margins_.right_ = ItemRef::reference(root, kPropertyHeight, 0.5);

    Rectangle & ra = timeline.addNew<Rectangle>("ra");
    ra.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    ra.anchors_.right_ = ra.addExpr(new TimelinePos(timeline,
                                                    model,
                                                    index,
                                                    &Timespan::t0_));
    ra.anchors_.vcenter_ = ItemRef::reference(timeline, kPropertyVCenter);
    ra.height_ =
      ra.addExpr(new TimelineHeight(view, *root.parent_, timeline));
    // ra.addExpr(new OddRoundUp(root, kPropertyHeight, 0.05, -1));
    // ra.color_ = style.cursor_;
    ra.color_ = ra.addExpr
      (style_color_ref(view, &ItemViewStyle::cursor_, 0, 0.75));
    // ra.opacity_ = shadow.opacity_;

    Rectangle & rb = timeline.addNew<Rectangle>("rb");
    rb.anchors_.left_ = ItemRef::reference(ra, kPropertyRight);
    rb.anchors_.right_ = rb.addExpr(new TimelinePos(timeline,
                                                    model,
                                                    index,
                                                    &Timespan::t1_));
    rb.anchors_.vcenter_ = ra.anchors_.vcenter_;
    rb.height_ = ra.height_;
    rb.color_ = style.timeline_included_;
    // rb.opacity_ = shadow.opacity_;

    Rectangle & rc = timeline.addNew<Rectangle>("rc");
    rc.anchors_.left_ = ItemRef::reference(rb, kPropertyRight);
    rc.anchors_.right_ = ItemRef::reference(timeline, kPropertyRight);
    rc.anchors_.vcenter_ = ra.anchors_.vcenter_;
    rc.height_ = ra.height_;
    rc.color_ = ra.color_;
    // rc.opacity_ = shadow.opacity_;

    RoundRect & p0 = timeline.addNew<RoundRect>("p0");
    p0.anchors_.hcenter_ = ItemRef::reference(ra, kPropertyRight);
#ifdef __APPLE__
    p0.anchors_.vcenter_ = ItemRef::reference(ra, kPropertyVCenter, 1.0, -1);
#else
    p0.anchors_.vcenter_ = ItemRef::reference(ra, kPropertyVCenter);
#endif
    p0.width_ = ItemRef::scale(ra, kPropertyHeight, 1.6);
    p0.height_ = p0.width_;
    p0.radius_ = ItemRef::scale(p0, kPropertyHeight, 0.5);
    p0.color_ = p0.addExpr
      (style_color_ref(view, &ItemViewStyle::timeline_included_, 0, 1));
    p0.background_ = p0.addExpr
      (style_color_ref(view, &ItemViewStyle::timeline_included_, 0));
    p0.visible_ = p0.addExpr(new IsMouseOverItem(view, *root.parent_));
    // p0.opacity_ = shadow.opacity_;

    RoundRect & p1 = timeline.addNew<RoundRect>("p1");
    p1.anchors_.hcenter_ = ItemRef::reference(rb, kPropertyRight);
    p1.anchors_.vcenter_ = p0.anchors_.vcenter_;
    p1.width_ = ItemRef::scale(ra, kPropertyHeight, 1.5);
    p1.height_ = p1.width_;
    p1.radius_ = ItemRef::scale(p1, kPropertyHeight, 0.5);
    p1.color_ = p1.addExpr
      (style_color_ref(view, &ItemViewStyle::cursor_, 0, 1));
    p1.background_ = p1.addExpr
      (style_color_ref(view, &ItemViewStyle::cursor_, 0));
    p1.visible_ = p0.visible_;
    // p1.opacity_ = shadow.opacity_;

    TimelineSlider & sa = root.add
      (new TimelineSlider("s0", view, timeline, model, index, &Timespan::t0_));
    sa.anchors_.offset(p0, -1, 0, -1, 0);

    TimelineSlider & sb = root.add
      (new TimelineSlider("s1", view, timeline, model, index, &Timespan::t1_));
    sb.anchors_.offset(p1, -1, 0, -1, 0);
  }

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
    RoundRect & btn = root.addNew<RoundRect>("remove");
    btn.border_ = ItemRef::constant(1.0);
    btn.radius_ = ItemRef::constant(3.0);
    btn.width_ = ItemRef::reference(root, kPropertyHeight, 0.8);
    btn.height_ = btn.width_;
    btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
    btn.anchors_.left_ = ItemRef::reference(root, kPropertyHeight, 0.6);
    btn.color_ = btn.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

    Text & btn_text = btn.addNew<Text>("btn_text");
    btn_text.anchors_.center(btn);
    btn_text.text_ = TVarRef::constant(TVar("-"));

    Text & src_name = root.addNew<Text>("src_name");
    src_name.anchors_.left_ = ItemRef::reference(root, kPropertyHeight, 1.6);
    src_name.anchors_.vcenter_ =
      src_name.addExpr(new OddRoundUp(root, kPropertyVCenter), 1.0, -1);
    src_name.width_ = ItemRef::reference(style.row_height_, 10.0);
    src_name.text_ = src_name.addExpr(new GetClipName(model, index));
    src_name.elide_ = Qt::ElideMiddle;
    src_name.font_ = style.font_small_;
    src_name.color_ = style.fg_timecode_;
    src_name.background_ = ColorRef::constant(style.bg_.get().transparent());
    src_name.fontSize_ = ItemRef::scale(root, kPropertyHeight, 0.2775);

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
    timeline.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);
    timeline.anchors_.left_ = ItemRef::reference(src_name, kPropertyRight);
    timeline.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    layout_timeline(model, view, style, timeline, index);

    RemoveClip & btn_ia = root.add<RemoveClip>
      (new RemoveClip("btn_ia", view, index));
    btn_ia.anchors_.fill(btn);
  }

  //----------------------------------------------------------------
  // RepeatClip
  //
  struct RepeatClip : public InputArea
  {
    RepeatClip(const char * id, RemuxView & view):
      InputArea(id),
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
      view_.repeat_clip();
      return true;
    }

    RemuxView & view_;
  };

  //----------------------------------------------------------------
  // layout_clips_add
  //
  static void
  layout_clips_add(RemuxModel & model,
                   RemuxView & view,
                   const RemuxViewStyle & style,
                   Item & root)
  {
    RoundRect & btn = root.addNew<RoundRect>("append");
    btn.border_ = ItemRef::constant(1.0);
    btn.radius_ = ItemRef::constant(3.0);
    btn.width_ = ItemRef::reference(root, kPropertyHeight, 0.8);
    btn.height_ = btn.width_;
    btn.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
    btn.anchors_.left_ = ItemRef::reference(root, kPropertyHeight, 1.6);
    btn.color_ = btn.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_controls_));

    Text & btn_text = btn.addNew<Text>("btn_text");
    btn_text.anchors_.center(btn);
    btn_text.text_ = TVarRef::constant(TVar("+"));

    RepeatClip & btn_ia = root.add<RepeatClip>(new RepeatClip("btn_ia", view));
    btn_ia.anchors_.fill(btn);
  }


  //----------------------------------------------------------------
  // ClipItem
  //
  struct ClipItem : public DraggableItem
  {
    ClipItem(const char * id,
             RemuxModel & model,
             RemuxView & view,
             std::size_t clip_index):
      DraggableItem(id),
      model_(model),
      view_(view),
      index_(clip_index)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      if (model_.selected_ != index_)
      {
        model_.selected_ = index_;
        view_.dataChanged();
      }

      return true;
    }

    // virtual:
    bool onDragEnd(const TVec2D & itemCSysOrigin,
                   const TVec2D & rootCSysDragStart,
                   const TVec2D & rootCSysDragEnd)
    {
      // reorder clips:
      TVec2D offset = rootCSysDragEnd - rootCSysDragStart;

      // Item & root = parent<Item>();
      double h = this->height();
      double d = round(offset.y() / h);
      double n = model_.clips_.size();

      std::size_t index =
        std::size_t(std::max(0.0, std::min(n - 1, double(index_) + d)));

      // shortcut:
      TClipPtr clip = model_.clips_[index_];

      if (index != index_)
      {
        model_.clips_.erase(model_.clips_.begin() + index_);
        model_.clips_.insert(model_.clips_.begin() + index, clip);
        model_.selected_ = index;
        view_.requestUncache();
      }

      view_.requestRepaint();
      DraggableItem::onDragEnd(itemCSysOrigin,
                               rootCSysDragStart,
                               rootCSysDragEnd);
      return true;
    }

    RemuxModel & model_;
    RemuxView & view_;
    std::size_t index_;
    TVec2D offset_;
  };

  //----------------------------------------------------------------
  // ClipItemColor
  //
  struct ClipItemColor : public TColorExpr
  {
    ClipItemColor(const RemuxModel & model,
                  std::size_t index,
                  const ItemView & view,
                  const InputArea & item):
      model_(model),
      index_(index),
      view_(view),
      item_(item)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const ItemViewStyle & style = *view_.style();

      TVec4D v = style.bg_.get().a_scaled(0.0);
      if (model_.selected_clip() == model_.clips_[index_])
      {
        v = style.bg_controls_.get();
      }

      const std::list<VisibleItem> & items = view_.mouseOverItems();
      std::list<VisibleItem>::const_iterator found = yae::find(items, item_);
      if (found != items.end())
      {
        v = style.bg_controls_.get();
      }

      result = Color(v);
    }

    const RemuxModel & model_;
    const std::size_t index_;
    const ItemView & view_;
    const Item & item_;
  };


  //----------------------------------------------------------------
  // VSplitter
  //
  struct VSplitter : public InputArea
  {
    VSplitter(const char * id,
              const ItemRef & lowerBound,
              const ItemRef & upperBound,
              ItemRef & anchorRef):
      InputArea(id, true),
      lowerBound_(lowerBound),
      upperBound_(upperBound),
      anchorRef_(anchorRef),
      anchorPos_(0),
      offsetPos_(0)
    {}

    // virtual:
    void uncache()
    {
      lowerBound_.uncache();
      upperBound_.uncache();
      InputArea::uncache();
    }

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      anchorPos_ = anchorRef_.get();
      offsetPos_ = anchorRef_.translate_;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      double dy = rootCSysDragEnd.y() - rootCSysDragStart.y();
      double v_min = lowerBound_.get();
      double v_max = upperBound_.get();
      double v = anchorPos_ + dy;
      v = std::min(v_max, std::max(v_min, v));

      double dv = v - anchorPos_;
      anchorRef_.translate_ = offsetPos_ + dv;

      // this avoids uncaching the scrollview content:
      parent_->uncacheSelfAndChildren();

      return true;
    }

    ItemRef lowerBound_;
    ItemRef upperBound_;
    ItemRef & anchorRef_;
    double anchorPos_;
    double offsetPos_;
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
      sep.height_ = ItemRef::reference(style.row_height_, 0.15);
      sep.color_ = sep.addExpr(style_color_ref(view, &ItemViewStyle::fg_));

      VSplitter & splitter = root.
        add(new VSplitter("splitter",
                          ItemRef::reference(root, kPropertyTop, 1.0, 100),
                          ItemRef::reference(root, kPropertyBottom, 1.0, -100),
                          sep.anchors_.bottom_));
      splitter.anchors_.fill(sep);

      gops.anchors_.fill(root);
      gops.anchors_.bottom_ = ItemRef::reference(sep, kPropertyTop);

      clips.anchors_.fill(root);
      clips.anchors_.top_ = ItemRef::reference(sep, kPropertyBottom);

      Item & clips_container =
        layout_scrollview(kScrollbarVertical, view, style, clips,
                          kScrollbarVertical);

      Item & clip_list = clips_container.addNew<Item>("clip_list");
      clip_list.anchors_.fill(clips_container);
      clip_list.anchors_.bottom_.reset();

      Item & clips_add = clips_container.addNew<Item>("clips_add");
      clips_add.anchors_.fill(clips_container);
      clips_add.anchors_.top_ = ItemRef::reference(clip_list, kPropertyBottom);
      clips_add.anchors_.bottom_.reset();
      clips_add.height_ = ItemRef::reference(style.row_height_);
      layout_clips_add(model, view, style, clips_add);

      std::size_t num_clips = model.clips_.size();
      for (std::size_t i = 0; i < num_clips; i++)
      {
        const TClipPtr & clip = model.clips_[i];
        view.append_clip(clip);
      }
    }
  };


  //----------------------------------------------------------------
  // RemuxViewStyle::RemuxViewStyle
  //
  RemuxViewStyle::RemuxViewStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view),
    layout_(new RemuxLayout())
  {
#if 0
    row_height_ = ItemRef::reference(title_height_, 0.55);
#else
    double fh = QFontMetricsF(font_).height();
    fh = std::max(fh, 13.0);
    row_height_ = ItemRef::constant(fh * 2.0);
#endif
  }

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView():
    ItemView("RemuxView"),
    style_("RemuxViewStyle", *this),
    model_(NULL)
  {
    bool ok = connect(&t0_, SIGNAL(mapped(int)),
                      this, SLOT(timecode_changed_t0(int)));
    YAE_ASSERT(true);

    ok = connect(&t1_, SIGNAL(mapped(int)),
                 this, SLOT(timecode_changed_t1(int)));
    YAE_ASSERT(true);
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

    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::processMouseTracking
  //
  bool
  RemuxView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    Item & root = *root_;
    requestUncache(&(root["clips"]));

    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::append_clip
  //
  void
  RemuxView::append_clip(const TClipPtr & clip)
  {
    RemuxModel & model = *model_;
    RemuxView & view = *this;
    Item & root = *root_;

    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");
    Item & clips_add = sv.content_->get<Item>("clips_add");
    clips_add.uncache();

    std::size_t index = clip_list.children_.size();
    ClipItem & row = clip_list.add(new ClipItem("row", model, view, index));
    row.anchors_.left_ = ItemRef::reference(clip_list, kPropertyLeft);
    row.anchors_.right_ = ItemRef::reference(clip_list, kPropertyRight);
#if 0
    row.anchors_.top_ = (index > 0) ?
      ItemRef::reference(*(clip_list.children_[index - 1]), kPropertyBottom) :
      ItemRef::reference(clip_list, kPropertyTop);
    row.height_ = ItemRef::reference(style_.row_height_);
#else
    row.anchors_.top_ = (index > 0) ?
      row.addExpr(new OddRoundUp(*(clip_list.children_[index - 1]),
                                 kPropertyBottom)) :
      row.addExpr(new OddRoundUp(clip_list, kPropertyTop));
    row.height_ =
      row.addExpr(new OddRoundUp(clips_add, kPropertyHeight));
#endif

    Rectangle & bg = row.addNew<Rectangle>("bg");
    bg.anchors_.fill(row);
    bg.color_ = bg.addExpr(new ClipItemColor(model, index, view, row));
    bg.color_.cachingEnabled_ = false;

    layout_clip(model, view, style_, row, index);

    Item & gops = root["gops"];
    layout_gops(model, view, style_, gops, clip);

    dataChanged();

#ifndef NDEBUG
    sv.content_->dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // RemuxView::remove_clip
  //
  void
  RemuxView::remove_clip(std::size_t index)
  {
    RemuxModel & model = *model_;
    RemuxView & view = *this;
    Item & root = *root_;

    Item & gops = root["gops"];
    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");

    TClipPtr clip = model.clips_[index];
    for (std::vector<ItemPtr>::iterator
           i = gops.children_.begin(); i != gops.children_.end(); ++i)
    {
      const Item & item = *(*i);
      const IsClipSelected * found =
        dynamic_cast<const IsClipSelected *>(item.visible_.ref_);

      if (found && found->clip_ == clip)
      {
        model.clips_.erase(model.clips_.begin() + index);
        model.selected_ = std::min(index, model.clips_.size() - 1);
        gops.children_.erase(i);
        clip_list.children_.pop_back();

        dataChanged();

#ifndef NDEBUG
        sv.content_->dump(std::cerr);
#endif
        break;
      }
    }
  }

  //----------------------------------------------------------------
  // RemuxView::repeat_clip
  //
  void
  RemuxView::repeat_clip()
  {
    RemuxModel & model = *model_;
    TClipPtr clip_ptr = model.selected_clip();
    if (!clip_ptr)
    {
      return;
    }

    const Clip & clip = *clip_ptr;
    const Timeline::Track & track =
      clip.demuxer_->summary().get_track_timeline(clip.track_);

    Timespan keep(track.pts_.front(), track.pts_.back());
    if (clip.keep_.t1_ < keep.t1_)
    {
      keep.t0_ = clip.keep_.t1_;
    }

    TClipPtr new_clip(new Clip(clip.demuxer_, clip.track_, keep));
    if (model.selected_ < model.clips_.size())
    {
      model.clips_.insert(model.clips_.begin() + model.selected_ + 1,
                          new_clip);
      model.selected_++;
    }
    else
    {
      model.clips_.push_back(new_clip);
      model.selected_ = model.clips_.size() - 1;
    }

    append_clip(new_clip);
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

    style_.layout_->layout(root, *this, *model_, style_);

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

  //----------------------------------------------------------------
  // find_clip_item
  //
  static ItemPtr
  find_clip_item(RemuxView & view, std::size_t index)
  {
    Item & root = *view.root();
    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");

    if (index < clip_list.children_.size())
    {
      return clip_list.children_[index];
    }

    return ItemPtr();
  }

  //----------------------------------------------------------------
  // update_time
  //
  static void
  update_time(RemuxView & view,
              const RemuxModel & model,
              std::size_t index,
              TTime Timespan::* field,
              const std::string & text)
  {
    if (text.empty() || model.clips_.size() <= index)
    {
      return;
    }

    Clip & clip = *(model.clips_[index]);
    const DemuxerSummary & summary = clip.demuxer_->summary();
    const Timeline::Track & track = summary.get_track_timeline(clip.track_);
    const FramerateEstimator & fe = yae::at(summary.fps_, clip.track_);
    double fps = fe.best_guess();
    parse_time(clip.keep_.*field, text.c_str(), NULL, NULL, fps);

    Item & root = *view.root();
    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    view.requestUncache(&sv);
    view.requestRepaint();
  }

  //----------------------------------------------------------------
  // RemuxView::timecode_changed_t0
  //
  void
  RemuxView::timecode_changed_t0(int i)
  {
    ItemPtr found = find_clip_item(*this, i);
    if (!found)
    {
      YAE_ASSERT(false);
      return;
    }

    Item & item = *found;
    std::string id = str("edit_", i * 2);
    TextInput & edit = item["timeline"].get<TextInput>(id.c_str());
    std::string text = edit.text().toUtf8().constData();
    update_time(*this, *model_, i, &Timespan::t0_, text);
  }

  //----------------------------------------------------------------
  // RemuxView::timecode_changed_t1
  //
  void
  RemuxView::timecode_changed_t1(int i)
  {
    ItemPtr found = find_clip_item(*this, i);
    if (!found)
    {
      YAE_ASSERT(false);
      return;
    }

    Item & item = *found;
    std::string id = str("edit_", i * 2 + 1);
    TextInput & edit = item["timeline"].get<TextInput>(id.c_str());
    std::string text = edit.text().toUtf8().constData();
    update_time(*this, *model_, i, &Timespan::t1_, text);
  }

}
