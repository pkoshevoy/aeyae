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
      gop.media_->demuxer_->summary().get_track_timeline(gop.track_);

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
    DecodeGop(const Gop & gop):
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

      // shortcut:
      Media & media = *(gop_.media_);

      // decode and cache the entire GOP:
      decode_gop(// source:
                 media.demuxer_,
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

  protected:
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
    async_.reset(new DecodeGop(gop_));
    queue.add(async_, &GopItem::cb, const_cast<GopItem *>(this));
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
  GopItem::cb(const boost::shared_ptr<AsyncTaskQueue::Task> &, void * ctx)
  {
    GopItem & item = *((GopItem *)ctx);

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

    const Item * prev = NULL;
    for (std::size_t i = gop.i0_; i < gop.i1_; i++)
    {
      std::size_t j = lut[i - gop.i0_];

      RoundRect & frame = root.addNew<RoundRect>("frame");
      frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      frame.anchors_.left_ = prev ?
        frame.addExpr(new OddRoundUp(*prev, kPropertyRight)) :
        ItemRef::reference(root, kPropertyLeft, 0.0, 1);

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
#if 0 // defined(__APPLE__)
      dts.supersample_ = dts.addExpr(new Supersample<Text>(dts));
#endif
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
#if 0 // defined(__APPLE__)
      pts.supersample_ = pts.addExpr(new Supersample<Text>(pts));
#endif
      pts.elide_ = Qt::ElideNone;
      pts.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      pts.background_ = frame.color_;

      prev = &frame;
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
  // RemuxLayoutGops
  //
  struct RemuxLayoutGops : public TLayout
  {
    void layout(RemuxModel & model,
                RemuxView & view,
                const RemuxViewStyle & style,
                Item & container,
                void * context)
    {
      for (std::size_t j = 0, n = model.clips_.size(); j < n; j++)
      {
        const TClipPtr & clip_ptr = model.clips_[j];
        const Clip & clip = *clip_ptr;
        Media * media = clip.media_.get();

        const Timeline::Track & track =
          media->demuxer_->summary().get_track_timeline(clip.track_);

        Item & root = container.addNew<Item>("clip_layout");
        root.visible_ = root.addExpr(new IsClipSelected(model, clip_ptr));
        root.anchors_.fill(container);

        Item & gops = layout_scrollview(kScrollbarBoth, view, style, root,
                                        kScrollbarBoth);

        const Item * prev = NULL;
        for (std::set<std::size_t>::const_iterator
               i = track.keyframes_.begin(); i != track.keyframes_.end(); ++i)
        {
          std::set<std::size_t>::const_iterator next = i;
          std::advance(next, 1);

          std::size_t i0 = *i;
          std::size_t i1 =
            (next == track.keyframes_.end()) ?
            track.dts_.size() :
            *next;

          Gop gop(media, clip.track_, i0, i1);

          GopItem & item = gops.add<GopItem>(new GopItem("gop", gop));
          item.setContext(view);
          item.anchors_.left_ = ItemRef::reference(gops, kPropertyLeft);
          item.anchors_.top_ = prev ?
            item.addExpr(new OddRoundUp(*prev, kPropertyBottom)) :
            ItemRef::reference(gops, kPropertyTop, 0.0, 1);

          layout_gop(clip, track, view, style, item, gop);
          prev = &item;
        }
      }
    }
  };

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
      const std::string & name = yae::at(model_.source_, clip.media_);

      std::string dirname;
      std::string basename;
      parseFilePath(name, dirname, basename);

      result = QString::fromUtf8(basename.c_str());
    }

    const RemuxModel & model_;
    const std::size_t index_;
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
    src_name.anchors_.left_ = ItemRef::reference(root, kPropertyHeight, 2.6);
    src_name.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
    src_name.anchors_.vcenter_ = ItemRef::reference(root, kPropertyVCenter);
    src_name.text_ = src_name.addExpr(new GetClipName(model, index));
    src_name.elide_ = Qt::ElideLeft;
  }

  //----------------------------------------------------------------
  // layout_add_clip_btn
  //
  static void
  layout_add_clip_btn(RemuxModel & model,
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
        v += TVec4D(0.1, 0.1, 0.1, 0.1);
      }

      result = Color(v);
    }

    const RemuxModel & model_;
    const std::size_t index_;
    const ItemView & view_;
    const Item & item_;
  };

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
      for (std::size_t i = 0, n = model.clips_.size(); i < n; ++i)
      {
        ClipItem & clip = root.add(new ClipItem("clip_item", model, view, i));
        clip.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
        clip.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
        clip.anchors_.top_ = prev ?
          ItemRef::reference(*prev, kPropertyBottom) :
          ItemRef::reference(root, kPropertyTop);
        clip.height_ = ItemRef::reference(style.row_height_);

        Rectangle & row = clip.addNew<Rectangle>("bg");
        row.anchors_.fill(clip);
        row.color_ = row.addExpr(new ClipItemColor(model, i, view, clip));
        row.color_.cachingEnabled_ = false;

        layout_clip(model, view, style, clip, i);
        prev = &clip;
      }

      Item & row = root.addNew<Item>("add_clip");
      row.anchors_.left_ = ItemRef::reference(root, kPropertyLeft);
      row.anchors_.right_ = ItemRef::reference(root, kPropertyRight);
      row.anchors_.top_ = prev ?
        ItemRef::reference(*prev, kPropertyBottom) :
        ItemRef::reference(root, kPropertyTop);
      row.height_ = ItemRef::reference(style.row_height_);
      layout_add_clip_btn(model, view, style, row);
    }
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
      sep.height_ = ItemRef::reference(style.title_height_, 0.1);
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

      style.layout_clips_->layout(clips_container, view, model, style);
      style.layout_gops_->layout(gops, view, model, style);
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
