// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:24:38 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QFontInfo>
#include <QFontMetricsF>
#include <QKeySequence>

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
               const VideoFrameItem & item,
               const Color & drop,
               const Color & keep):
      clip_(clip),
      span_(span),
      item_(item),
      drop_(drop),
      keep_(keep)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      bool selected = clip_.keep_.contains(span_.t1_);

      TVideoFramePtr vf_ptr = item_.videoFrame();
      if (vf_ptr)
      {
        TTime frame_t1 = vf_ptr->time_ + span_.dt();
        selected = clip_.keep_.contains(frame_t1);
      }

      result = selected ? keep_ : drop_;
    }

    const Clip & clip_;
    Timespan span_;
    const VideoFrameItem & item_;
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

  static const double kFrameOffset = 1;
  static const double kFrameWidth = 130;
  static const double kFrameHeight = 100;
  static const double kFrameRadius = 3;

  //----------------------------------------------------------------
  // get_frame_pos_x
  //
  static double
  get_frame_pos_x(const RemuxView & view, std::size_t column)
  {
    double dpi = view.style_.dpi_.get();
    double s = dpi / 96.0;
    double w = std::max(kFrameWidth, kFrameWidth * s);
    double x = w * double(column);
    return x;
  }

  //----------------------------------------------------------------
  // get_frame_pos_y
  //
  static double
  get_frame_pos_y(const RemuxView & view, std::size_t row)
  {
    double dpi = view.style_.dpi_.get();
    double s = dpi / 96.0;
    double h = std::max(kFrameHeight, kFrameHeight * s);
    double y = h * double(row);
    return y;
  }


  //----------------------------------------------------------------
  // GetFramePosX
  //
  struct GetFramePosX : TDoubleExpr
  {
    GetFramePosX(const RemuxView & view, std::size_t column):
      view_(view),
      column_(column)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = get_frame_pos_x(view_, column_);
    }

    const RemuxView & view_;
    std::size_t column_;
  };

  //----------------------------------------------------------------
  // GetFramePosY
  //
  struct GetFramePosY : TDoubleExpr
  {
    GetFramePosY(const RemuxView & view, std::size_t row):
      view_(view),
      row_(row)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = get_frame_pos_y(view_, row_);
    }

    const RemuxView & view_;
    std::size_t row_;
  };


  //----------------------------------------------------------------
  // layout_gop
  //
  static void
  layout_gop(const Clip & clip,
             const Timeline::Track & track,
             RemuxView & view,
             const RemuxViewStyle & style,
             Item & gops,
             const Gop & gop,
             std::size_t row)
  {
    GopItem & root = gops.add<GopItem>(new GopItem("gop", gop));
    root.setContext(view);
    root.anchors_.left_ = ItemRef::reference(gops, kPropertyLeft);
    root.anchors_.top_ =
      root.addExpr(new GetFramePosY(view, row), 1, kFrameOffset);
    root.anchors_.bottom_ =
      root.addExpr(new GetFramePosY(view, row + 1));

    std::vector<std::size_t> lut;
    get_pts_order_lut(gop, lut);

    for (std::size_t i = gop.i0_; i < gop.i1_; i++)
    {
      std::size_t k = i - gop.i0_;
      std::size_t j = lut[k];

      RoundRect & frame = root.addNew<RoundRect>("frame");
      frame.anchors_.top_ = ItemRef::reference(root, kPropertyTop);
      frame.anchors_.bottom_ = ItemRef::reference(root, kPropertyBottom);
      frame.anchors_.left_ =
        frame.addExpr(new GetFramePosX(view, k), 1, kFrameOffset);
      frame.anchors_.right_ =
        frame.addExpr(new GetFramePosX(view, k + 1));
      frame.radius_ = ItemRef::constant(kFrameRadius);

      frame.background_ = frame.
        addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0));

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
      // dts.fontSize_ = ItemRef::constant(9.5 * kDpiScale);
      dts.fontSize_ = ItemRef::reference(style.row_height_, 0.2875);
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

      Timespan span(track.pts_[j], track.pts_[j] + track.dur_[j]);
      frame.color_ = frame.
        addExpr(new FrameColor(clip, span, video,
                               style.cursor_.get(),
                               style.scrollbar_.get()));
      frame.color_.cachingEnabled_ = false;
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
  // GopCursorItem
  //
  struct GopCursorItem : public Rectangle
  {

    //----------------------------------------------------------------
    // GetCursorPosY
    //
    struct GetCursorPosY : TDoubleExpr
    {
      GetCursorPosY(const GopCursorItem & cursor, std::size_t offset = 0):
        cursor_(cursor),
        offset_(offset)
      {}

      // virtual:
      void evaluate(double & result) const
      {
        std::size_t row = cursor_.get_row(cursor_.frame_);
        result = get_frame_pos_y(cursor_.view_, row + offset_);
      }

      const GopCursorItem & cursor_;
      std::size_t offset_;
    };

    //----------------------------------------------------------------
    // GetCursorPosX
    //
    struct GetCursorPosX : TDoubleExpr
    {
      GetCursorPosX(const GopCursorItem & cursor, std::size_t offset = 0):
        cursor_(cursor),
        offset_(offset)
      {}

      // virtual:
      void evaluate(double & result) const
      {
        std::size_t column = cursor_.get_column(cursor_.frame_);
        result = get_frame_pos_x(cursor_.view_, column + offset_);
      }

      const GopCursorItem & cursor_;
      std::size_t offset_;
    };

    //----------------------------------------------------------------
    // GopCursorItem
    //
    GopCursorItem(const RemuxView & view,
                  const char * id,
                  const std::map<std::size_t, std::size_t> & row_lut):
      Rectangle(id),
      view_(view),
      rows_(row_lut),
      frame_(0),
      column_(0)
    {
      anchors_.top_ = addExpr(new GetCursorPosY(*this), 1, kFrameOffset);
      anchors_.bottom_ = addExpr(new GetCursorPosY(*this, 1));
      anchors_.left_ = addExpr(new GetCursorPosX(*this), 1, kFrameOffset);
      anchors_.right_ = addExpr(new GetCursorPosX(*this, 1));
    }

    //----------------------------------------------------------------
    // get_row
    //
    std::size_t get_row(std::size_t frame) const
    {
      if (rows_.empty())
      {
        YAE_ASSERT(false);
        return 0;
      }

      std::map<std::size_t, std::size_t>::const_iterator
        found = rows_.upper_bound(frame);

      if (found == rows_.end())
      {
        return rows_.rbegin()->second;
      }

      return found->second - 1;
    }

    //----------------------------------------------------------------
    // get_keyframe
    //
    std::size_t get_keyframe(std::size_t frame) const
    {
      if (rows_.empty())
      {
        YAE_ASSERT(false);
        return 0;
      }

      std::map<std::size_t, std::size_t>::const_iterator
        found = rows_.upper_bound(frame);

      if (found == rows_.end())
      {
        return rows_.rbegin()->first;
      }

      if (found != rows_.begin())
      {
        std::map<std::size_t, std::size_t>::const_iterator gop = found;
        std::advance(gop, -1);
        YAE_ASSERT(gop->second + 1 == found->second);
        return gop->first;
      }

      // packet preceeds the first keyframe packet...
      YAE_ASSERT(false);
      return 0;
    }

    //----------------------------------------------------------------
    // get_column
    //
    std::size_t get_column(std::size_t frame) const
    {
      std::size_t keyframe = get_keyframe(frame);
      YAE_ASSERT(keyframe <= frame);
      return (frame - keyframe);
    }

    //----------------------------------------------------------------
    // set_frame
    //
    void set_frame(std::size_t frame)
    {
      int keyframe = get_keyframe(frame);
      frame_ = frame;
      column_ = frame - keyframe;
    }

    //----------------------------------------------------------------
    // move_up
    //
    bool move_up()
    {
      if (frame_ == 0)
      {
        return false;
      }

      std::size_t row = get_row(frame_);
      if (row == 0)
      {
        // already at the top row, go to the start of the row:
        frame_ = 0;
      }
      else
      {
        std::size_t keyframe = get_keyframe(frame_);
        std::size_t offset = get_keyframe(keyframe - 1);
        frame_ = std::min(offset + column_, keyframe - 1);
      }

      return true;
    }

    //----------------------------------------------------------------
    // move_down
    //
    bool move_down()
    {
      std::size_t row = get_row(frame_);
      if (row + 1 >= rows_.size())
      {
        // already at the bottom row:
        return false;
      }

      std::map<std::size_t, std::size_t>::const_iterator
        i1 = rows_.upper_bound(frame_);

      std::map<std::size_t, std::size_t>::const_iterator
        i2 = rows_.upper_bound(i1->first);

      frame_ = std::min(i1->first + column_,
                        std::max(i1->first, i2->first - 1));
      return true;
    }

    //----------------------------------------------------------------
    // move_left
    //
    bool move_left()
    {
      if (frame_ == 0)
      {
        return false;
      }

      frame_--;
      column_ = get_column(frame_);
      return true;
    }

    //----------------------------------------------------------------
    // move_right
    //
    bool move_right()
    {
      if (rows_.empty() || frame_ == rows_.rbegin()->first)
      {
        return false;
      }

      frame_++;
      column_ = get_column(frame_);
      return true;
    }

    //----------------------------------------------------------------
    // move_to_row_start
    //
    bool move_to_row_start()
    {
      frame_ = get_keyframe(frame_);
      return true;
    }

    //----------------------------------------------------------------
    // move_to_row_end
    //
    bool move_to_row_end()
    {
      std::map<std::size_t, std::size_t>::const_iterator
        i1 = rows_.upper_bound(frame_);
      frame_ = i1->first - 1;
      return true;
    }

    const RemuxView & view_;
    std::map<std::size_t, std::size_t> rows_; // map keyframe -> row
    std::size_t frame_;
    std::size_t column_;
  };


  //----------------------------------------------------------------
  // EndFrameItem
  //
  struct EndFrameItem : public VideoFrameItem
  {
    EndFrameItem(const char * id, std::size_t frame):
      VideoFrameItem(id, frame)
    {}

    // virtual:
    void paintContent() const {}
    void unpaintContent() const {}
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
    const Timeline::Track & track = clip.get_track_timeline();

    Item & gops = layout_scrollview(kScrollbarBoth, view, style, root,
                                    kScrollbarBoth);

    Scrollview & sv = root.get<Scrollview>("clip_layout.scrollview");
    sv.uncacheContent_ = false;

    // create a map from rows keyframe index to row index:
    std::map<std::size_t, std::size_t> row_lut;

    std::size_t row = 0;
    if (!yae::has<std::size_t>(track.keyframes_, 0))
    {
      // if the 1st packet is not a keyframe...
      // it's a malformed GOP that preceeds the 1st well formed GOP,
      // and it must be accounted for:

      std::size_t i0 = 0;
      std::size_t i1 =
        track.keyframes_.empty() ?
        track.dts_.size() :
        *track.keyframes_.begin();

      row_lut[i0] = row;

      Gop gop(clip.demuxer_, clip.track_, i0, i1);
      layout_gop(clip, track, view, style, gops, gop, row);

      row++;
    }

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

      row_lut[i0] = row;

      Gop gop(clip.demuxer_, clip.track_, i0, i1);
      layout_gop(clip, track, view, style, gops, gop, row);
    }

    row_lut[track.dts_.size()] = row;

    // add a placeholder item for the cursor position after all the frames:
    EndFrameItem & end = gops.add<EndFrameItem>
      (new EndFrameItem("end", track.dts_.size()));
    end.anchors_.left_ = ItemRef::reference(gops, kPropertyLeft);
    end.anchors_.right_ =
      end.addExpr(new GetFramePosX(view, 1));
    end.anchors_.top_ =
      end.addExpr(new GetFramePosY(view, row), 1, kFrameOffset);
    end.anchors_.bottom_ =
      end.addExpr(new GetFramePosY(view, row + 1));

    GopCursorItem & cursor =
      gops.add(new GopCursorItem(view, "cursor", row_lut));
    cursor.color_ = cursor.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_, 0));
    cursor.colorBorder_ = cursor.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_));
    cursor.border_ = ItemRef::constant(2);

    cursor.margins_.top_ = ItemRef::constant(-1);
    cursor.margins_.left_ = ItemRef::constant(-1);
    cursor.margins_.bottom_ = ItemRef::constant(1);
    cursor.margins_.right_ = ItemRef::constant(1);
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

      const Timeline::Track & track = clip.get_track_timeline();
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
      const Timeline::Track & track = clip.get_track_timeline();

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
    btn_text.fontSize_ = ItemRef::reference(style.row_height_, 0.5);

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
    btn_text.fontSize_ = ItemRef::reference(style.row_height_, 0.5);

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
  // ViewDpi
  //
  struct ViewDpi : TDoubleExpr
  {
    ViewDpi(const RemuxView & view):
      view_(view),
      dpi_(0.0)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = view_.delegate()->logical_dpi_y();

      if (result != dpi_ && dpi_ > 0.0)
      {
        // force all geometry to be recalculated:
        view_.root()->uncache();
      }

      // cache the result:
      dpi_ = result;
    }

    const RemuxView & view_;
    mutable double dpi_;
  };


  //----------------------------------------------------------------
  // get_row_height
  //
  static double
  get_row_height(const RemuxView & view)
  {
    double dpi = view.style_.dpi_.get();
    double rh = dpi / 3.5;
    double fh = QFontMetricsF(view.style_.font_).height();
    fh = std::max(fh, 13.0);
    rh = std::max(rh, fh * 2.0);
    return rh;
  }

  //----------------------------------------------------------------
  // GetRowHeight
  //
  struct GetRowHeight : TDoubleExpr
  {
    GetRowHeight(const RemuxView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = get_row_height(view_);
    }

    const RemuxView & view_;
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
      sep.anchors_.bottom_ = ItemRef::offset(root, kPropertyBottom,
                                             -3.0 * (2 + get_row_height(view)));
      sep.height_ = ItemRef::reference(style.row_height_, 0.15);
      sep.color_ = sep.addExpr(style_color_ref(view, &ItemViewStyle::fg_));

      VSplitter & splitter = root.
        add(new VSplitter("splitter",
                          ItemRef::reference(root, kPropertyTop, 1.0,
                                             3.0 * (2 + get_row_height(view))),
                          ItemRef::reference(root, kPropertyBottom, 1.0,
                                             -3.0 * (2 + get_row_height(view))),
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
  RemuxViewStyle::RemuxViewStyle(const char * id, const RemuxView & view):
    ItemViewStyle(id, view),
    layout_(new RemuxLayout())
  {
    dpi_ = addExpr(new ViewDpi(view));
    dpi_.cachingEnabled_ = false;

    row_height_ = addExpr(new GetRowHeight(view));
  }

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView():
    ItemView("RemuxView"),
    style_("RemuxViewStyle", *this),
    model_(NULL),
    actionSetInPoint_(this),
    actionSetOutPoint_(this)
  {
    actionSetInPoint_.setObjectName(QString::fromUtf8("actionSetInPoint_"));
    actionSetInPoint_.setText(tr("Set &In Point"));
    actionSetInPoint_.setShortcut(QKeySequence(Qt::Key_I));

    actionSetOutPoint_.setObjectName(QString::fromUtf8("actionSetOutPoint_"));
    actionSetOutPoint_.setText(tr("Set &Out Point"));
    actionSetOutPoint_.setShortcut(QKeySequence(Qt::Key_O));

    bool ok = connect(&t0_, SIGNAL(mapped(int)),
                      this, SLOT(timecode_changed_t0(int)));
    YAE_ASSERT(true);

    ok = connect(&t1_, SIGNAL(mapped(int)),
                 this, SLOT(timecode_changed_t1(int)));
    YAE_ASSERT(true);

    ok = connect(&actionSetInPoint_, SIGNAL(triggered()),
                 this, SLOT(set_in_point()));
    YAE_ASSERT(ok);

    ok = connect(&actionSetOutPoint_, SIGNAL(triggered()),
                 this, SLOT(set_out_point()));
    YAE_ASSERT(ok);
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
  // find_gops_item
  //
  static std::vector<ItemPtr>::iterator
  find_gops_item(Item & gops, const TClipPtr & clip)
  {
    for (std::vector<ItemPtr>::iterator
           i = gops.children_.begin(); i != gops.children_.end(); ++i)
    {
      const Item & item = *(*i);
      const IsClipSelected * found =
        dynamic_cast<const IsClipSelected *>(item.visible_.ref_);

      if (found && found->clip_ == clip)
      {
        return i;
      }
    }

    return gops.children_.end();
  }

  //----------------------------------------------------------------
  // get_cursor_item
  //
  static GopCursorItem *
  get_cursor_item(const RemuxView & view, Scrollview ** sv = NULL)
  {
    if (!view.model())
    {
      return NULL;
    }

    Item & gops = view.root()->get<Item>("gops");
    TClipPtr clip = view.model()->selected_clip();
    std::vector<ItemPtr>::iterator found = find_gops_item(gops, clip);

    if (found == gops.children_.end())
    {
      return NULL;
    }

    Scrollview & sview = (*found)->get<Scrollview>("clip_layout.scrollview");
    if (sv)
    {
      *sv = &sview;
    }

    GopCursorItem & cursor = sview.content_->get<GopCursorItem>("cursor");
    return &cursor;
  }


  //----------------------------------------------------------------
  // ensure_frame_visible
  //
  static void
  ensure_frame_visible(RemuxView & view, std::size_t frame)
  {
    Scrollview * sv = NULL;
    GopCursorItem * cursor = get_cursor_item(view, &sv);
    if (!cursor)
    {
      return;
    }

    std::size_t ir = cursor->get_row(frame);
    std::size_t ic = cursor->get_column(frame);

    Item & scene = *(sv->content_);
    double scene_h = scene.height();
    double scene_w = scene.width();

    double view_h = sv->height();
    double view_w = sv->width();

    double range_h = (view_h < scene_h) ? (scene_h - view_h) : 0.0;
    double range_w = (view_w < scene_w) ? (scene_w - view_w) : 0.0;

    while (range_h > 0.0)
    {
      double view_y0 = range_h * sv->position_.y();
      double view_y1 = view_y0 + view_h;

      double item_y0 = get_frame_pos_y(view, ir);
      double item_y1 = get_frame_pos_y(view, ir + 1);

      if (item_y0 < view_y0)
      {
        double y = item_y0 / range_h;
        y = std::min<double>(1.0, y);
        sv->position_.set_y(y);
      }
      else if (item_y1 > view_y1)
      {
        double y = (item_y1 - view_h) / range_h;
        y = std::max<double>(0.0, y);
        sv->position_.set_y(y);
      }
      else
      {
        break;
      }

      Item & vsb = sv->parent_->get<Item>("scrollbar");
      vsb.uncache();
      break;
    }

    while (range_w > 0.0)
    {
      double view_x0 = range_w * sv->position_.x();
      double view_x1 = view_x0 + view_w;

      double item_x0 = get_frame_pos_x(view, ic);
      double item_x1 = get_frame_pos_x(view, ic + 1);

      if (item_x0 < view_x0)
      {
        double x = item_x0 / range_w;
        x = std::min<double>(1.0, x);
        sv->position_.set_x(x);
      }
      else if (item_x1 > view_x1)
      {
        double x = (item_x1 - view_w) / range_w;
        x = std::max<double>(0.0, x);
        sv->position_.set_x(x);
      }
      else
      {
        break;
      }

      Item & hsb = sv->parent_->get<Item>("hscrollbar");
      hsb.uncache();
      break;
    }

    view.requestRepaint();
  }

  //----------------------------------------------------------------
  // RemuxView::processKeyEvent
  //
  bool
  RemuxView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
    e->ignore();

    if (!model_)
    {
      return false;
    }

    QEvent::Type et = e->type();
    if (et == QEvent::KeyPress && !ItemFocus::singleton().focus())
    {
      int key = e->key();

      if (key == Qt::Key_Left ||
          key == Qt::Key_Right ||
          key == Qt::Key_Up ||
          key == Qt::Key_Down)
      {
        GopCursorItem * cursor = get_cursor_item(*this);
        if (cursor)
        {
          bool ok = false;

          if (key == Qt::Key_Left)
          {
            ok = cursor->move_left();
          }
          else if (key == Qt::Key_Right)
          {
            ok = cursor->move_right();
          }
          else if (key == Qt::Key_Up)
          {
            ok = cursor->move_up();
          }
          else if (key == Qt::Key_Down)
          {
            ok = cursor->move_down();
          }

          if (ok)
          {
            ensure_frame_visible(*this, cursor->frame_);
            requestUncache(cursor);
            requestRepaint();
          }

          e->accept();
        }
      }
      else if (key == Qt::Key_I)
      {
        if (actionSetInPoint_.isEnabled())
        {
          actionSetInPoint_.trigger();
        }
      }
      else if (key == Qt::Key_O)
      {
        if (actionSetOutPoint_.isEnabled())
        {
          actionSetOutPoint_.trigger();
        }
      }
#if 0
      else if (key == Qt::Key_PageUp ||
               key == Qt::Key_PageDown ||
               key == Qt::Key_Home ||
               key == Qt::Key_End)
      {
        scroll(*this, key);
        e->accept();
      }
      else if (key == Qt::Key_Return ||
               key == Qt::Key_Enter)
      {
        QModelIndex currentIndex = model_->currentItem();
        model_->setPlayingItem(currentIndex);
        e->accept();
      }
#endif
    }

    return e->isAccepted() ? true : ItemView::processKeyEvent(canvas, e);
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
  // find_frame_under_mouse
  //
  static VideoFrameItem *
  find_frame_under_mouse(const std::list<VisibleItem> & mouseOverItems)
  {
    for (std::list<VisibleItem>::const_iterator
           i = mouseOverItems.begin(); i != mouseOverItems.end(); ++i)
    {
      Item * item = i->item_.lock().get();
      VideoFrameItem * frame_item = dynamic_cast<VideoFrameItem *>(item);
      if (frame_item)
      {
        return frame_item;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // RemuxView::processMouseEvent
  //
  bool
  RemuxView::processMouseEvent(Canvas * canvas, QMouseEvent * event)
  {
    bool r = ItemView::processMouseEvent(canvas, event);

    QEvent::Type et = event->type();
    if (et == QEvent::MouseButtonPress)
    {
      GopCursorItem * cursor = get_cursor_item(*this);
      actionSetInPoint_.setEnabled(!!cursor);
      actionSetOutPoint_.setEnabled(!!cursor);

      if (cursor)
      {
        VideoFrameItem * frame = find_frame_under_mouse(mouseOverItems_);
        if (frame)
        {
          cursor->set_frame(frame->frameIndex());
          requestUncache(cursor);
          requestRepaint();
        }
      }
    }

    return r;
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
    Item & root = *root_;
    Item & gops = root["gops"];

    RemuxModel & model = *model_;
    TClipPtr clip = model.clips_[index];
    std::vector<ItemPtr>::iterator found = find_gops_item(gops, clip);

    if (found == gops.children_.end())
    {
      YAE_ASSERT(false);
      return;
    }

    gops.children_.erase(found);

    const Item & item = *(*found);
    model.clips_.erase(model.clips_.begin() + index);
    model.selected_ = std::min(index, model.clips_.size() - 1);

    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");
    clip_list.children_.pop_back();

#ifndef NDEBUG
    sv.content_->dump(std::cerr);
#endif

    dataChanged();
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
    const Timeline::Track & track = clip.get_track_timeline();

    Timespan keep(track.pts_.front(), track.pts_.back());
    if (clip.keep_.t1_ < keep.t1_)
    {
      keep.t0_ = clip.keep_.t1_;
    }

    TClipPtr new_clip(new Clip(clip.demuxer_, clip.track_, keep));
    std::size_t new_index = model.selected_ + 1;
    if (model.selected_ < model.clips_.size())
    {
      model.clips_.insert(model.clips_.begin() + model.selected_ + 1,
                          new_clip);
    }
    else
    {
      model.clips_.push_back(new_clip);
      new_index = model.clips_.size() - 1;
    }

    // copy scrollview position from source scroll view to the new scroll view:
    append_clip(new_clip);

    Item & root = *root_;
    Item & gops = root["gops"];
    Scrollview & new_sv =
      gops.children_.back()->get<Scrollview>("clip_layout.scrollview");

    for (std::vector<ItemPtr>::iterator
           i = gops.children_.begin(); i != gops.children_.end(); ++i)
    {
      const Item & item = *(*i);
      const IsClipSelected * found =
        dynamic_cast<const IsClipSelected *>(item.visible_.ref_);

      if (found && found->clip_ == clip_ptr)
      {
        const Scrollview & src_sv =
          item.get<Scrollview>("clip_layout.scrollview");

        new_sv.position_ = src_sv.position_;
        break;
      }
    }

    // select the new clip:
    model.selected_ = new_index;
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

  //----------------------------------------------------------------
  // get_frame_under_cursor
  //
  static VideoFrameItem *
  get_frame_under_cursor(const RemuxView & view, GopItem ** gop_item = NULL)
  {
    if (!view.model())
    {
      return NULL;
    }

    TClipPtr clip_ptr = view.model()->selected_clip();
    if (!clip_ptr)
    {
      return NULL;
    }

    Scrollview * sv = NULL;
    GopCursorItem * cursor = get_cursor_item(view, &sv);
    if (!cursor)
    {
      return NULL;
    }

    Clip & clip = *clip_ptr;
    const Timeline::Track & track = clip.get_track_timeline();

    std::size_t ir = cursor->get_row(cursor->frame_);
    std::size_t ic = cursor->get_column(cursor->frame_);

    Item & gops = *(sv->content_);
    if (ir < gops.children_.size())
    {
      GopItem * item = dynamic_cast<GopItem *>(gops.children_[ir].get());
      if (gop_item)
      {
        *gop_item = item;
      }

      if (item)
      {
        if (ic < item->children_.size())
        {
          VideoFrameItem & video =
            item->children_[ic]->get<VideoFrameItem>("video");
          return &video;
        }
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // get_cursor_pts
  //
  static bool
  get_cursor_pts(const RemuxView & view, TTime & pts)
  {
    if (!view.model())
    {
      return false;
    }

    TClipPtr clip_ptr = view.model()->selected_clip();
    if (!clip_ptr)
    {
      return false;
    }

    Clip & clip = *clip_ptr;
    const Timeline::Track & track = clip.get_track_timeline();

    GopItem * gop_item = NULL;
    VideoFrameItem * frame = get_frame_under_cursor(view, &gop_item);
    if (!frame)
    {
      if (track.pts_span_.empty())
      {
        return false;
      }

      pts = track.pts_span_.back().t1_ + track.dur_.back();
      return true;
    }

    const Gop & gop = gop_item->gop();
    std::vector<std::size_t> lut;
    get_pts_order_lut(gop, lut);

    std::size_t i = frame->frameIndex();
    std::size_t k = i - gop.i0_;
    std::size_t j = lut[k];
    pts = track.pts_[j];

    // use the decoded PTS time, if available:
    TVideoFramePtr vf = frame->videoFrame();
    if (vf)
    {
      pts = vf->time_;
    }

    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::set_in_point
  //
  void
  RemuxView::set_in_point()
  {
    std::cerr << "FIXME: pkoshevoy: RemuxView::set_in_point" << std::endl;

    TTime pts;
    if (!get_cursor_pts(*this, pts))
    {
      return;
    }

    Clip & clip = *(model_->selected_clip());
    clip.keep_.t0_ = pts;

    if (clip.keep_.t1_ <= clip.keep_.t0_)
    {
      const Timeline::Track & track = clip.get_track_timeline();
      clip.keep_.t1_ = track.pts_span_.back().t1_;
    }

    Item & root = *root_;
    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    requestUncache(&sv);
    requestRepaint();
  }

  //----------------------------------------------------------------
  // RemuxView::set_out_point
  //
  void
  RemuxView::set_out_point()
  {
    std::cerr << "FIXME: pkoshevoy: RemuxView::set_out_point" << std::endl;

    TTime pts;
    if (!get_cursor_pts(*this, pts))
    {
      return;
    }

    Clip & clip = *(model_->selected_clip());
    clip.keep_.t1_ = pts;

    if (clip.keep_.t1_ <= clip.keep_.t0_)
    {
      const Timeline::Track & track = clip.get_track_timeline();
      clip.keep_.t0_ = track.pts_span_.front().t0_;
    }

    Item & root = *root_;
    Scrollview & sv = root["clips"].get<Scrollview>("clips.scrollview");
    requestUncache(&sv);
    requestRepaint();
  }

}
