// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Jan 27 18:24:38 MST 2018
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt library:
#include <QKeySequence>

// aeyae:
#include "yae/utils/yae_utils.h"

// yaeui:
#include "yaeAxisItem.h"
#include "yaeCheckboxItem.h"
#include "yaeFlickableArea.h"
#include "yaeInputProxyItem.h"
#include "yaeItemFocus.h"
#include "yaePlotItem.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTabRect.h"
#include "yaeTextInput.h"
#include "yaeUtilsQt.h"

// local:
#include "yaeDemuxerView.h"


namespace yae
{

  //----------------------------------------------------------------
  // IsRedacted
  //
  struct IsRedacted : TBoolExpr
  {
    IsRedacted(const RemuxView & view,
               const std::string & src_name,
               const std::string & track_id):
      view_(view),
      src_name_(src_name),
      track_id_(track_id)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      RemuxModel * model = view_.model();
      YAE_ASSERT(model);
      if (!model)
      {
        result = false;
        return;
      }

      std::map<std::string, SetOfTracks>::const_iterator
        found = model->redacted_.find(src_name_);
      if (found == model->redacted_.end())
      {
        result = false;
        return;
      }

      const SetOfTracks & redacted = found->second;
      result = yae::has(redacted, track_id_);
    }

    const RemuxView & view_;
    std::string src_name_;
    std::string track_id_;
  };

  //----------------------------------------------------------------
  // OnToggleRedacted
  //
  struct OnToggleRedacted : public CheckboxItem::Action
  {
    OnToggleRedacted(RemuxView & view,
                     const std::string & src_name,
                     const std::string & track_id):
      view_(view),
      src_name_(src_name),
      track_id_(track_id)
    {}

    // virtual:
    void operator()(const CheckboxItem & cbox) const
    {
      RemuxModel * model = view_.model();
      YAE_ASSERT(model);
      if (!model)
      {
        return;
      }

      std::set<std::string> prev;
      model->get_unredacted_track_ids(src_name_, prev, "v:");

      SetOfTracks & redacted = model->redacted_[src_name_];
      if (cbox.checked_.get())
      {
        redacted.insert(track_id_);
      }
      else
      {
        redacted.erase(track_id_);
      }

      std::set<std::string> curr;
      model->get_unredacted_track_ids(src_name_, curr, "v:");

      // if video track selection changed -- update the GOP view:
      if (curr.size() == 1 &&
          curr != prev &&
          model->change_clip_track_id(src_name_, *curr.begin()))
      {
        TDemuxerInterfacePtr demuxer = yae::at(model->demuxer_, src_name_);
        view_.relayout_gops(demuxer);
      }
    }

    RemuxView & view_;
    std::string src_name_;
    std::string track_id_;
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
    FrameColor(const RemuxView & view,
               const Timespan & span,
               const VideoFrameItem & item,
               const Color & drop,
               const Color & keep):
      view_(view),
      span_(span),
      item_(item),
      drop_(drop),
      keep_(keep)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      if (view_.view_mode() != RemuxView::kLayoutMode)
      {
        result = keep_;
        return;
      }

      bool selected = true;

      const Clip * clip = view_.selected_clip().get();
      if (clip)
      {
        selected = clip->keep_.contains(span_.t0_);
      }

      result = selected ? keep_ : drop_;
    }

    const RemuxView & view_;
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
    opacity_(ItemRef::constant(1.0)),
    frame_(frame)
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
      {
        IOpenGLContext & context = *(layer->context());
        renderer_->skipColorConverter(context, true);
        renderer_->loadFrame(context, frames[i]);
      }

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
        result = QString::fromUtf8
          (str("PTS ", vf_ptr->time_.to_hhmmss_ms()).c_str());
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
  struct DecodeGop : public AsyncTaskQueue::Task
  {
    DecodeGop(const yae::weak_ptr<Item> & item, const Gop & gop):
      item_(item),
      gop_(gop),
      frames_(new TVideoFrames())
    {}

    virtual ~DecodeGop()
    {}

    virtual void run()
    {
      TGopCache & cache = gop_cache();
      if (cache.get(gop_))
      {
        // GOP is already cached:
        return;
      }

      ItemPtr item_ptr = this->item();
      if (!item_ptr)
      {
        // task owner no longer exists, ignore the results:
        return;
      }

      // shortcut:
      const GopItem & item = dynamic_cast<GopItem &>(*item_ptr);

      // decode and cache the entire GOP:
      decode_gop(// source:
                 gop_.demuxer_,
                 gop_.track_,
                 gop_.i0_,
                 gop_.i1_,

                 // output:
                 item.pixelFormat(),
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
    yae::weak_ptr<Item> item_;
    Gop gop_;
    TVideoFramesPtr frames_;
    FramerateEstimator fps_;
    Timespan pts_;
  };


  //----------------------------------------------------------------
  // GopItem::GopItem
  //
  GopItem::GopItem(const char * id, const Gop & gop, TPixelFormatId fmt):
    Item(id),
    gop_(gop),
    pixelFormat_(fmt),
    layer_(NULL),
    failed_(false)
  {}

  //----------------------------------------------------------------
  // GopItem::~GopItem
  //
  GopItem::~GopItem()
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
    queue.push_front(async_, &GopItem::cb);
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
  GopItem::cb(const yae::shared_ptr<AsyncTaskQueue::Task> & task_ptr, void *)
  {
    yae::shared_ptr<DecodeGop, AsyncTaskQueue::Task>
      task = task_ptr.cast<DecodeGop>();
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
      av_log(NULL, AV_LOG_WARNING, "decoding failed: %s",
             to_text(item.gop_).c_str());
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
  static const double kFrameRadius = 7;

  //----------------------------------------------------------------
  // get_frame_pos_x
  //
  static double
  get_frame_pos_x(const RemuxView & view, std::size_t column)
  {
    double dpi = view.style_->dpi_.get();
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
    double dpi = view.style_->dpi_.get();
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
  layout_gop(TPixelFormatId outputFormat,
             const Timeline::Track & track,
             RemuxView & view,
             const RemuxViewStyle & style,
             Item & gops,
             const Gop & gop,
             std::size_t row)
  {
    GopItem & root = gops.add<GopItem>(new GopItem("gop", gop, outputFormat));
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
      dts.text_ = TVarRef::constant
        (TVar(str("DTS ", track.dts_[j].to_hhmmss_ms()).c_str()));
      dts.fontSize_ = ItemRef::reference(style.row_height_, 0.2875);
      dts.elide_ = Qt::ElideNone;
      dts.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      dts.background_ = frame.color_;

      Text & pts = frame.addNew<Text>("pts");
      pts.font_ = style.font_large_;
      pts.anchors_.bottom_ = ItemRef::reference(frame, kPropertyBottom, 1, -5);
      pts.anchors_.left_ = ItemRef::reference(frame, kPropertyLeft, 1, 5);
      pts.visible_ = pts.addExpr(new HasFramePts(video));
      pts.text_ = pts.addExpr(new GetFramePts(video));
      pts.fontSize_ = dts.fontSize_;
      pts.elide_ = Qt::ElideNone;
      pts.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      pts.background_ = frame.color_;

      Timespan span(track.pts_[j], track.pts_[j] + track.dur_[j]);
      frame.color_ = frame.
        addExpr(new FrameColor(view, span, video,
                               style.cursor_.get(),
                               style.scrollbar_.get()));
      frame.color_.disableCaching();

      // highlight the GOP start (the keyframe, usually)
      if (j == gop.i0_)
      {
        Rectangle & gs_bg = frame.addNew<Rectangle>("gs_bg");
        Text & gs = frame.addNew<Text>("gs");
        gs.font_ = style.font_large_;
        gs.anchors_.top_ = ItemRef::reference(frame, kPropertyTop, 1, 5);
        gs.anchors_.right_ = ItemRef::reference(frame, kPropertyRight, 1, -5);

        const char * txt =
          yae::has(track.keyframes_, gop.i0_) ? "KEY" : "GOP";
        gs.text_ = TVarRef::constant(TVar(txt));
        gs.fontSize_ = ItemRef::reference(style.row_height_, 0.2875);
        gs.elide_ = Qt::ElideNone;
        gs.color_ =  gs.addExpr(style_color_ref(view, &ItemViewStyle::bg_));
        gs.background_ = ColorRef::constant(style.fg_timecode_.get().opaque());
        gs_bg.anchors_.offset(gs, -3, 3, 0, 0);
        gs_bg.color_ = ColorRef::constant(style.fg_timecode_.get().opaque());
      }
    }
  }

  //----------------------------------------------------------------
  // IsCurrentClip
  //
  struct IsCurrentClip : TBoolExpr
  {
    IsCurrentClip(const RemuxView & view, const TClipPtr & clip):
      view_(view),
      clip_(clip)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.current_clip() == clip_;
    }

    const RemuxView & view_;
    TClipPtr clip_;
  };

  //----------------------------------------------------------------
  // GopCursorItem
  //
  struct GopCursorItem : public RoundRect
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
      RoundRect(id),
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

      frame_ =
        (i2 == rows_.end()) ? (i1->first) :
        std::min(i1->first + column_, std::max(i1->first, i2->first - 1));
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
  static ItemPtr
  layout_gops(RemuxModel & model,
              RemuxView & view,
              const RemuxViewStyle & style,
              Item & container,
              const TClipPtr & clip_ptr)
  {
    Item & root = container.addNew<Item>("clip_layout");
    root.anchors_.fill(container);
    root.visible_ = root.addExpr(new IsCurrentClip(view, clip_ptr));

    const Clip & clip = *clip_ptr;
    const Timeline::Track & track = clip.get_track_timeline();

    // override output pixel format, if necessary:
    TPixelFormatId outputFormat = kInvalidPixelFormat;
    {
      const DemuxerSummary & summary = clip.demuxer_->summary();

      TrackPtr track_ptr =
        yae::get(summary.decoders_, clip.track_);

      VideoTrackPtr decoder =
        boost::dynamic_pointer_cast<VideoTrack, Track>(track_ptr);

      if (decoder)
      {
        VideoTraits vtts;
        decoder->getTraits(vtts);

        const pixelFormat::Traits * ptts =
          pixelFormat::getTraits(vtts.pixelFormat_);

        if (ptts)
        {
          if ((ptts->flags_ & pixelFormat::kAlpha) &&
              (ptts->flags_ & pixelFormat::kColor))
          {
            outputFormat = kPixelFormatBGRA;
          }
          else
          {
            outputFormat = kPixelFormatBGR24;
          }
        }
        else
        {
          outputFormat = vtts.pixelFormat_;
        }
      }
    }

    Scrollview & sv =
      layout_scrollview(kScrollbarBoth, view, style, root, kScrollbarBoth);
    sv.uncacheContent_ = false;

    Item & content = *(sv.content_);

    ItemPtr reuse = yae::get(view.gops_, clip.demuxer_);
    if (reuse)
    {
      // reuse existing layout:
      content.children_.push_back(reuse);
    }
    else
    {
      reuse.reset(new Item("gop_items"));

      Item & gops = *reuse;
      gops.anchors_.left_ = ItemRef::constant(0.0);
      gops.anchors_.top_ = ItemRef::constant(0.0);

      gops.Item::setParent(NULL, reuse);
      content.children_.push_back(reuse);

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
        layout_gop(outputFormat, track, view, style, gops, gop, row);

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
        layout_gop(outputFormat, track, view, style, gops, gop, row);
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

      // save the layout so it can be reused:
      view.gops_[clip.demuxer_] = reuse;
      view.gops_row_lut_[clip.demuxer_] = row_lut;
    }

    const std::map<std::size_t, std::size_t> & row_lut =
      yae::at(view.gops_row_lut_, clip.demuxer_);

    GopCursorItem & cursor =
      content.add(new GopCursorItem(view, "cursor", row_lut));
    cursor.color_ = cursor.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_, 0));
    cursor.colorBorder_ = cursor.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_));
    cursor.radius_ = ItemRef::constant(kFrameRadius);
    cursor.border_ = ItemRef::constant(kFrameRadius * 0.33);

    cursor.margins_.set_top(ItemRef::constant(-1));
    cursor.margins_.set_left(ItemRef::constant(-1));
    cursor.margins_.set_bottom(ItemRef::constant(1));
    cursor.margins_.set_right(ItemRef::constant(1));

    return root.self_.lock();
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
      parse_file_path(name, dirname, basename);

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
      gop_cache().purge_unreferenced_entries();

#ifdef YAE_ENABLE_MEMORY_FOOTPRINT_ANALYSIS
      yae::TFootprint::show(std::cerr);
#endif

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
    focus.copyViewToEdit_ = BoolRef::constant(true);
    focus.bgNoFocus_ = ColorRef::constant(Color(0, 0.0));
    focus.bgOnFocus_ = ColorRef::constant(Color(0, 0.3));

    ItemFocus::singleton().
      setFocusable(view, focus, "remux_layout", focus_index);

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
    text_bg.color_.disableCaching();

    edit.anchors_.fill(text);
    edit.margins_.set_right(ItemRef::scale(edit, kPropertyCursorWidth, -1.0));
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
  // ClipTimelineHeight
  //
  struct ClipTimelineHeight : public TDoubleExpr
  {
    ClipTimelineHeight(ItemView & view, Item & container, Item & timeline):
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
      result = view_.isMouseOverItem(item_);
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
    t0.text_->margins_.
      set_left(ItemRef::reference(root, kPropertyHeight, 0.1));
    bool ok = view.connect(t0.edit_, SIGNAL(editingFinished(const QString &)),
                           &view.t0_, SLOT(map()));
    YAE_ASSERT(ok);
    view.t0_.setMapping(t0.edit_, int(index));

    TextEdit t1 =
      layout_timeedit(model, view, style, root, index, &Timespan::t1_);

    t1.text_->anchors_.right_ = ItemRef::offset(root, kPropertyRight, -3);
    t1.text_->margins_.
      set_right(ItemRef::reference(root, kPropertyHeight, 0.5));

    ok = view.connect(t1.edit_, SIGNAL(editingFinished(const QString &)),
                      &view.t1_, SLOT(map()));
    YAE_ASSERT(ok);
    view.t1_.setMapping(t1.edit_, int(index));

    Item & timeline = root.addNew<Item>("timeline");
    timeline.anchors_.fill(root);
    timeline.anchors_.left_ = ItemRef::reference(*t0.bg_, kPropertyRight);
    timeline.anchors_.right_ = ItemRef::reference(*t1.bg_, kPropertyLeft);
    timeline.margins_.set_left(ItemRef::reference(root, kPropertyHeight, 0.5));
    timeline.margins_.set_right(timeline.margins_.get_left());

    Rectangle & ra = timeline.addNew<Rectangle>("ra");
    ra.anchors_.left_ = ItemRef::reference(timeline, kPropertyLeft);
    ra.anchors_.right_ = ra.addExpr(new TimelinePos(timeline,
                                                    model,
                                                    index,
                                                    &Timespan::t0_));
    ra.anchors_.vcenter_ = ItemRef::reference(timeline, kPropertyVCenter);
    ra.height_ =
      ra.addExpr(new ClipTimelineHeight(view, *root.parent_, timeline));
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
    // src_name.font_ = style.font_small_;
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
      if (view_.selected_ != index_)
      {
        view_.selected_ = index_;
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
        view_.selected_ = index;
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
                  const RemuxView & view,
                  std::size_t index,
                  const InputArea & item):
      model_(model),
      view_(view),
      index_(index),
      item_(item)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const ItemViewStyle & style = *view_.style();

      TVec4D v = style.bg_.get().a_scaled(0.0);
      if (view_.selected_clip() == model_.clips_[index_])
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
    const RemuxView & view_;
    const std::size_t index_;
    const Item & item_;
  };


  //----------------------------------------------------------------
  // VSplitter
  //
  struct VSplitter : public InputArea
  {
    typedef ItemRef::CachedRef<ItemRef::Affine<ItemRef::PropDataSrc> > TPropRef;

    VSplitter(const char * id,
              const ItemRef & lowerBound,
              const ItemRef & upperBound,
              ItemRef & anchorRef):
      InputArea(id, true),
      lowerBound_(lowerBound),
      upperBound_(upperBound),
      anchor_(anchorRef.private_.cast<TPropRef>()),
      anchorPos_(0),
      offsetPos_(0)
    {
      YAE_ASSERT(anchor_);
    }

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
      anchorPos_ = anchor_->get_value();
      offsetPos_ = anchor_->src_.translate_;
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
      anchor_->src_.translate_ = offsetPos_ + dv;

      // this avoids uncaching the scrollview content:
      parent_->uncacheSelfAndChildren();

      return true;
    }

    ItemRef lowerBound_;
    ItemRef upperBound_;

    TPropRef * anchor_;

    double anchorPos_;
    double offsetPos_;
  };

  //----------------------------------------------------------------
  // layout_control_button
  //
  static Text &
  layout_control_button(RemuxModel & model,
                        RemuxView & view,
                        const RemuxViewStyle & style,
                        Rectangle & controls,
                        TabRect & btn)
  {
    btn.height_ = ItemRef::reference(style.row_height_, 0.95);
    btn.r1_ = ItemRef::constant(9.0);
    btn.r2_ = ItemRef::constant(8.0);
#if 1
    btn.color_ = btn.
      addExpr(style_color_ref(view, &ItemViewStyle::bg_, 0.5, 0));
#else
    btn.color_ = ColorRef::constant(0x7f7f7f);
#endif

    Text & txt = controls.addNew<Text>("layout_txt");
    txt.anchors_.vcenter_ = ItemRef::reference(controls, kPropertyVCenter);
    txt.anchors_.left_ = ItemRef::reference(btn, kPropertyLeft);
    txt.margins_.set_left(ItemRef::reference(controls, kPropertyHeight, 0.7));
    txt.margins_.set_right(txt.margins_.get_left());
    // txt.font_ = style.font_small_;
    txt.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
    txt.color_ = txt.addExpr
      (style_color_ref(view, &ItemViewStyle::fg_));

    btn.anchors_.top_ = ItemRef::reference(controls, kPropertyTop);
    // btn.anchors_.left_ = ItemRef::reference(txt, kPropertyLeft);
    btn.anchors_.right_ = ItemRef::reference(txt, kPropertyRight);
    btn.margins_.set_right(ItemRef::reference(controls, kPropertyHeight, -0.7));
    // btn.margins_.set_right(btn.margins_.get_left());

    return txt;
  }

  //----------------------------------------------------------------
  // layout_text_underline
  //
  static void
  layout_text_underline(RemuxView & view,
                        TabRect & btn,
                        Text & txt,
                        Rectangle & underline)
  {
    underline.color_ = underline.addExpr
      (style_color_ref(view, &ItemViewStyle::cursor_, 0, 0.5));

    underline.anchors_.left_ = ItemRef::offset(txt, kPropertyLeft, -2);
    underline.anchors_.right_ = ItemRef::offset(txt, kPropertyRight, 2);
    underline.anchors_.top_ = ItemRef::reference(txt, kPropertyBottom);
    underline.margins_.
      set_top(underline.addExpr(new GetFontDescent(txt), -1.0, 1));
    underline.height_ = underline.addExpr
      (new OddRoundUp(btn, kPropertyHeight, 0.05, -1));
  }

  //----------------------------------------------------------------
  // In
  //
  template <RemuxView::ViewMode mode>
  struct In : public TBoolExpr
  {
    In(const RemuxView & view): view_(view) {}

    // virtual:
    void evaluate(bool & result) const
    { result = (view_.view_mode() == mode); }

    const RemuxView & view_;
  };


  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(RemuxView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.is_playback_paused();
    }

    RemuxView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    RemuxView * view = (RemuxView *)context;
    view->toggle_playback();
  }


  //----------------------------------------------------------------
  // HiddenItem
  //
  struct HiddenItem : Item
  {
    HiddenItem(const char * id, const RemuxViewStyle & style):
      Item(id)
    {
      row_height_ = ItemRef::reference(style.row_height_);
      row_height_with_odd_roundup_.set(new OddRoundUp(row_height_));
    }

    // virtual:
    void uncache()
    {
      row_height_.uncache();
      row_height_with_odd_roundup_.uncache();
      Item::uncache();
    }

    ItemRef row_height_;
    ItemRef row_height_with_odd_roundup_;
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
      HiddenItem & hidden = root.
        addHidden<HiddenItem>(new HiddenItem("hidden", style));

      Rectangle & bg = root.addNew<Rectangle>("background");
      Rectangle & controls = root.addNew<Rectangle>("controls");
      bg.color_ = bg.addExpr(style_color_ref(view, &ItemViewStyle::bg_));
      bg.anchors_.fill(root);
      bg.anchors_.bottom_ = ItemRef::reference(controls, kPropertyTop);

      Item & sources = root.addNew<Item>("sources");
      sources.anchors_.fill(bg);
      sources.visible_ = sources.addExpr(new In<RemuxView::kSourceMode>(view));
      sources.visible_.disableCaching();
      layout_scrollview(kScrollbarVertical, view, style, sources,
                        kScrollbarVertical);

      Item & layout = root.addNew<Item>("layout");
      layout.anchors_.fill(bg);
      layout.visible_ = layout.addExpr(new In<RemuxView::kLayoutMode>(view));
      layout.visible_.disableCaching();

      Item & preview = root.addNew<Item>("preview");
      preview.anchors_.fill(bg);
      preview.visible_ = layout.addExpr(new In<RemuxView::kPreviewMode>(view));
      preview.visible_.disableCaching();

      PlayerUxItem & pl_ux = root.add<PlayerUxItem>(view.pl_ux_);
      pl_ux.anchors_.fill(bg);

      Item & output = root.addNew<Item>("export");
      output.anchors_.fill(bg);
      output.visible_ = output.addExpr(new In<RemuxView::kExportMode>(view));
      output.visible_.disableCaching();
      {
        RoundRect & btn = output.addNew<RoundRect>("export_btn");
        btn.anchors_.bottom_ = ItemRef::reference(output, kPropertyBottom);
        btn.height_ = ItemRef::reference(style.row_height_);
        btn.border_ = ItemRef::constant(1.0);
        btn.radius_ = ItemRef::constant(3.0);

        btn.color_ = btn.addExpr
          (style_color_ref(view, &ItemViewStyle::bg_controls_));

        Text & txt = output.addNew<Text>("layout_txt");
        txt.anchors_.vcenter_ = ItemRef::reference(btn, kPropertyVCenter);
        txt.font_ = style.font_small_;
        txt.fontSize_ = ItemRef::reference(btn, kPropertyHeight, 0.2775);

        btn.anchors_.left_ = ItemRef::reference(txt, kPropertyLeft);
        btn.anchors_.right_ = ItemRef::reference(txt, kPropertyRight);
        btn.margins_.set_left(ItemRef::reference(btn, kPropertyHeight, -1.0));
        btn.margins_.set_right(btn.margins_.get_left());
        btn.margins_.set_bottom(ItemRef::reference(btn, kPropertyHeight, 0.5));

        txt.anchors_.right_ = ItemRef::reference(output, kPropertyRight);
        txt.margins_.
          set_right(ItemRef::reference(btn, kPropertyHeight, 1.5));
        txt.text_ = TVarRef::constant(TVar(QObject::tr("Export")));

        Item & ia = output.add
          (new InvokeMethodOnClick("export_ia", view, "remux"));
        ia.anchors_.fill(btn);
      }

      Item & gops = layout.addNew<Item>("gops");
      Item & clips = layout.addNew<Item>("clips");

      Rectangle & sep = layout.addNew<Rectangle>("separator");
      sep.anchors_.left_ = ItemRef::reference(layout, kPropertyLeft);
      sep.anchors_.right_ = ItemRef::reference(layout, kPropertyRight);
      sep.anchors_.bottom_ = ItemRef::offset(controls, kPropertyTop,
                                             -3.0 * (2 + get_row_height(view)));
      sep.height_ = ItemRef::reference(style.row_height_, 0.15);
      sep.color_ = sep.addExpr(style_color_ref(view, &ItemViewStyle::fg_));

      VSplitter & splitter = layout.
        add(new VSplitter("splitter",
                          ItemRef::reference(layout, kPropertyTop, 1.0,
                                             3.0 * (2 + get_row_height(view))),
                          ItemRef::reference(controls, kPropertyTop, 1.0,
                                             -3.0 * (2 + get_row_height(view))),
                          sep.anchors_.bottom_));
      splitter.anchors_.fill(sep);

      gops.anchors_.fill(layout);
      gops.anchors_.bottom_ = ItemRef::reference(sep, kPropertyTop);

      clips.anchors_.fill(layout);
      clips.anchors_.top_ = ItemRef::reference(sep, kPropertyBottom);
      clips.anchors_.bottom_ = ItemRef::reference(controls, kPropertyTop);

      Scrollview & sv =
        layout_scrollview(kScrollbarVertical, view, style, clips,
                          kScrollbarVertical);

      Item & clips_container = *(sv.content_);

      Item & clip_list = clips_container.addNew<Item>("clip_list");
      clip_list.anchors_.fill(clips_container);
      clip_list.anchors_.bottom_.reset();

      Item & clips_add = clips_container.addNew<Item>("clips_add");
      clips_add.anchors_.fill(clips_container);
      clips_add.anchors_.top_ = ItemRef::reference(clip_list, kPropertyBottom);
      clips_add.anchors_.bottom_.reset();
      clips_add.height_ = ItemRef::reference(hidden.row_height_);
      layout_clips_add(model, view, style, clips_add);

      std::size_t num_clips = model.clips_.size();
      for (std::size_t i = 0; i < num_clips; i++)
      {
        const TClipPtr & clip = model.clips_[i];
        view.append_clip(clip);
      }

      // layout the controls:
      controls.anchors_.fill(root);
      controls.anchors_.top_.reset();
      controls.visible_ = controls.addInverse(new IsFullscreen(view));

      controls.height_ = controls.addExpr
        (new Conditional<ItemRef>
         (controls.visible_,
          ItemRef::reference(hidden.row_height_with_odd_roundup_),
          ItemRef::constant(0.0)));

      // add a button to switch to clip source view:
      TabRect & source_btn =
        controls.add(new TabRect("source_btn", view, kTabBottom));
      {
        source_btn.anchors_.left_ =
          ItemRef::reference(controls, kPropertyLeft);
        source_btn.margins_.
          set_left(ItemRef::reference(source_btn, kPropertyR1));

        Rectangle & underline = controls.addNew<Rectangle>("source_ul");
        underline.visible_ = sources.visible_;
        underline.visible_.disableCaching();

        Text & txt = layout_control_button(model,
                                           view,
                                           style,
                                           controls,
                                           source_btn);

        txt.text_ = TVarRef::constant(TVar(QObject::tr("Source")));
        layout_text_underline(view, source_btn, txt, underline);

        // NOTE: QGenericArgument does not store the arg value,
        //       so we must provide storage instead:
        static const RemuxView::ViewMode mode = RemuxView::kSourceMode;

        Item & ia = controls.add
          (new InvokeMethodOnClick("source_ia", view, "set_view_mode",
                                   Q_ARG(RemuxView::ViewMode, mode)));
        ia.anchors_.fill(source_btn);
      }

      // add a button to switch to clip layout view:
      TabRect & layout_btn =
        controls.add(new TabRect("layout_btn", view, kTabBottom));
      {
        layout_btn.anchors_.left_ =
          ItemRef::reference(source_btn, kPropertyRight);

        Rectangle & underline = controls.addNew<Rectangle>("layout_ul");
        underline.visible_ = layout.visible_;
        underline.visible_.disableCaching();

        Text & txt = layout_control_button(model,
                                           view,
                                           style,
                                           controls,
                                           layout_btn);

        txt.text_ = TVarRef::constant(TVar(QObject::tr("Layout")));
        layout_text_underline(view, layout_btn, txt, underline);

        // NOTE: QGenericArgument does not store the arg value,
        //       so we must provide storage instead:
        static const RemuxView::ViewMode mode = RemuxView::kLayoutMode;

        Item & ia = controls.add
          (new InvokeMethodOnClick("layout_ia", view, "set_view_mode",
                                   Q_ARG(RemuxView::ViewMode, mode)));
        ia.anchors_.fill(layout_btn);
      }

      // add a button to switch to preview:
      TabRect & preview_btn =
        controls.add(new TabRect("preview_btn", view, kTabBottom));
      {
        preview_btn.anchors_.left_ =
          ItemRef::reference(layout_btn, kPropertyRight);

        Rectangle & underline = controls.addNew<Rectangle>("preview_ul");
        underline.visible_ = preview.visible_;
        underline.visible_.disableCaching();

        Text & txt = layout_control_button(model,
                                           view,
                                           style,
                                           controls,
                                           preview_btn);

        txt.text_ = TVarRef::constant(TVar(QObject::tr("Preview")));
        layout_text_underline(view, preview_btn, txt, underline);

        // NOTE: QGenericArgument does not store the arg value,
        //       so we must provide storage instead:
        static const RemuxView::ViewMode mode = RemuxView::kPreviewMode;
        Item & ia = controls.add
          (new InvokeMethodOnClick("preview_ia", view, "set_view_mode",
                                   Q_ARG(RemuxView::ViewMode, mode)));
        ia.anchors_.fill(preview_btn);
      }

      // add a button to switch to player:
      TabRect & player_btn =
        controls.add(new TabRect("player_btn", view, kTabBottom));
      {
        player_btn.anchors_.left_ =
          ItemRef::reference(preview_btn, kPropertyRight);

        Rectangle & underline = controls.addNew<Rectangle>("player_ul");
        underline.visible_ = pl_ux.visible_;
        underline.visible_.disableCaching();

        Text & txt = layout_control_button(model,
                                           view,
                                           style,
                                           controls,
                                           player_btn);

        txt.text_ = TVarRef::constant(TVar(QObject::tr("Player")));
        layout_text_underline(view, player_btn, txt, underline);

        // NOTE: QGenericArgument does not store the arg value,
        //       so we must provide storage instead:
        static const RemuxView::ViewMode mode = RemuxView::kPlayerMode;
        Item & ia = controls.add
          (new InvokeMethodOnClick("player_ia", view, "set_view_mode",
                                   Q_ARG(RemuxView::ViewMode, mode)));
        ia.anchors_.fill(player_btn);
      }

      // add a button to generate the output file:
      TabRect & export_btn =
        controls.add(new TabRect("export_btn", view, kTabBottom));
      {
        export_btn.anchors_.left_ =
          ItemRef::reference(player_btn, kPropertyRight);

        Rectangle & underline = controls.addNew<Rectangle>("export_ul");
        underline.visible_ = output.visible_;
        underline.visible_.disableCaching();

        Text & txt = layout_control_button(model,
                                           view,
                                           style,
                                           controls,
                                           export_btn);

        txt.text_ = TVarRef::constant(TVar(QObject::tr("Export")));
        layout_text_underline(view, export_btn, txt, underline);

        static const RemuxView::ViewMode mode = RemuxView::kExportMode;
        Item & ia = controls.add
          (new InvokeMethodOnClick("export_ia", view, "set_view_mode",
                                   Q_ARG(RemuxView::ViewMode, mode)));
        ia.anchors_.fill(export_btn);
      }

#if 0 // ndef NDEBUG
      // FIXME: just testing:
      TabRect & tab_left = bg.add(new TabRect("tab_left", view, kTabLeft));
      tab_left.anchors_.right_ = ItemRef::reference(bg, kPropertyRight);
      tab_left.anchors_.vcenter_ = ItemRef::reference(bg, kPropertyVCenter);
      tab_left.height_ = ItemRef::reference(style.row_height_, 4);
      tab_left.width_ = ItemRef::reference(style.row_height_);
      tab_left.opacity_ = ItemRef::constant(1.0);
      tab_left.r1_ = ItemRef::constant(9.0);
      tab_left.r2_ = ItemRef::constant(9.0);
      tab_left.color_ = ColorRef::constant(Color(0xffffff));

      TabRect & tab_right = bg.add(new TabRect("tab_right", view, kTabRight));
      tab_right.anchors_.left_ = ItemRef::reference(bg, kPropertyLeft);
      tab_right.anchors_.vcenter_ = ItemRef::reference(bg, kPropertyVCenter);
      tab_right.height_ = ItemRef::reference(style.row_height_, 4);
      tab_right.width_ = ItemRef::reference(style.row_height_);
      tab_right.opacity_ = ItemRef::constant(1.0);
      tab_right.r1_ = ItemRef::constant(9.0);
      tab_right.r2_ = ItemRef::constant(9.0);
      tab_right.color_ = ColorRef::constant(Color(0xffffff));
#endif
    }
  };

  //----------------------------------------------------------------
  // RemuxViewStyle::RemuxViewStyle
  //
  RemuxViewStyle::RemuxViewStyle(const char * id, const RemuxView & view):
    ItemViewStyle(id, view),
    layout_(new RemuxLayout())
  {}

  //----------------------------------------------------------------
  // RemuxView::RemuxView
  //
  RemuxView::RemuxView(const char * name):
    ItemView(name),
    menuEdit_(NULL),
    menuView_(NULL),
    popup_(NULL),
    model_(NULL),
    view_mode_(RemuxView::kUndefined),
    time_range_(new Segment(0, 1)),
    size_range_(new Segment())
  {
    enable_focus_group();

    // add style to the root item, so it could be uncached automatically:
    style_.reset(new RemuxViewStyle("RemuxViewStyle", *this));

    // init actions:
    menuEdit_ = add_menu("menuEdit");
    menuView_ = add_menu("menuView");
    popup_ = add_menu("contextMenu");

    // translate ui:
    menuEdit_->setTitle(tr("&Edit"));
    menuView_->setTitle(tr("&View"));

    bool ok = connect(&t0_, SIGNAL(mapped(int)),
                      this, SLOT(timecode_changed_t0(int)));
    YAE_ASSERT(ok);

    ok = connect(&t1_, SIGNAL(mapped(int)),
                 this, SLOT(timecode_changed_t1(int)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // RemuxView::~RemuxView
  //
  RemuxView::~RemuxView()
  {
    TMakeCurrentContext currentContext(context().get());
    RemuxView::clear();
    clips_.clear();
    gops_.clear();
    pl_ux_.reset();
    style_.reset();

    delete menuEdit_;
    delete menuView_;
    delete popup_;
  }

  //----------------------------------------------------------------
  // RemuxView::clear
  //
  void
  RemuxView::clear()
  {
    TMakeCurrentContext currentContext(context().get());
    ItemView::clear();
    pl_ux_->clear();
  }

  //----------------------------------------------------------------
  // RemuxView::setContext
  //
  void
  RemuxView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    YAE_ASSERT(delegate_);
    ItemView::setContext(context);

    pl_ux_.reset(new PlayerUxItem("pl_ux", *this));
    PlayerUxItem & pl_ux = *pl_ux_;
    pl_ux.actionLoop_->setChecked(true);
    pl_ux.actionLoop_->setEnabled(false);
    pl_ux.player_->makePersonalCanvas(context);
    pl_ux.player_->set_loop_playback(true);
    pl_ux.setVisible(false);

    TimelineItem & timeline = *pl_ux.timeline_;
    timeline.is_playlist_visible_ = BoolRef::constant(false);
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
    set_view_mode(RemuxView::kUndefined);
    model_ = model;
    set_view_mode(RemuxView::kLayoutMode);
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

    Item & root = *view.root();
    Item & gops = (view.view_mode() == RemuxView::kLayoutMode ?
                   root["layout"]["gops"] :
                   root["preview"]);

    TClipPtr clip = view.current_clip();
    ItemPtr container = view.get_gops_container(clip);
    if (!container)
    {
      return NULL;
    }

    Scrollview & sview = container->get<Scrollview>("clip_layout.scrollview");
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
      double view_y0 = range_h * sv->position_y();
      double view_y1 = view_y0 + view_h;

      double item_y0 = get_frame_pos_y(view, ir);
      double item_y1 = get_frame_pos_y(view, ir + 1);

      if (item_y0 < view_y0)
      {
        double y = item_y0 / range_h;
        y = std::min<double>(1.0, y);
        sv->set_position_y(y);
      }
      else if (item_y1 > view_y1)
      {
        double y = (item_y1 - view_h) / range_h;
        y = std::max<double>(0.0, y);
        sv->set_position_y(y);
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
      double view_x0 = range_w * sv->position_x();
      double view_x1 = view_x0 + view_w;

      double item_x0 = get_frame_pos_x(view, ic);
      double item_x1 = get_frame_pos_x(view, ic + 1);

      if (item_x0 < view_x0)
      {
        double x = item_x0 / range_w;
        x = std::min<double>(1.0, x);
        sv->set_position_x(x);
      }
      else if (item_x1 > view_x1)
      {
        double x = (item_x1 - view_w) / range_w;
        x = std::max<double>(0.0, x);
        sv->set_position_x(x);
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

    // shortcuts:
    PlayerUxItem & pl_ux = *pl_ux_;
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
        pl_ux.actionSetInPoint_->activate(QAction::Trigger);
      }
      else if (key == Qt::Key_O)
      {
        pl_ux.actionSetOutPoint_->activate(QAction::Trigger);
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

    if (view_mode_ == kSourceMode)
    {
      requestUncache(&(root["sources"]));
    }
    else if (view_mode_ == kLayoutMode)
    {
      requestUncache(&(root["layout"]["clips"]));
    }
    else if (view_mode_ == kPlayerMode)
    {
      pl_ux_->processMouseTracking(mousePt);
    }

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
    PlayerUxItem & pl_ux = *pl_ux_;
    bool r = ItemView::processMouseEvent(canvas, event);

    QEvent::Type et = event->type();
    if (et == QEvent::MouseButtonPress)
    {
      GopCursorItem * cursor =
        (view_mode_ == kLayoutMode ||
         view_mode_ == kPreviewMode) ?
        get_cursor_item(*this) :
        NULL;

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
  // SourceItemTop
  //
  struct SourceItemTop : TDoubleExpr
  {
    SourceItemTop(const RemuxView & view, const std::string & name):
      view_(view),
      name_(name)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      result = 0.0;

      // const RemuxViewStyle & style = *(view_.style());
      RemuxModel & model = *(view_.model());

      for (std::list<std::string>::const_iterator i = model.sources_.begin();
           i != model.sources_.end(); ++i)
      {
        const std::string & name = *i;
        if (name == name_)
        {
          return;
        }

        const ItemPtr & item = yae::at(view_.source_item_, name);
        double h = item->height();
        // h += style.row_height_.get();
        result += ceil(h);
      }
    }

    const RemuxView & view_;
    std::string name_;
  };

  //----------------------------------------------------------------
  // SourceItemVisible
  //
  struct SourceItemVisible : public TBoolExpr
  {
    SourceItemVisible(const RemuxView & view, const std::string & name):
      view_(view),
      name_(name)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = false;

      RemuxModel & model = *(view_.model());
      for (std::list<std::string>::const_iterator i = model.sources_.begin();
           i != model.sources_.end(); ++i)
      {
        const std::string & name = *i;
        if (name == name_)
        {
          return;
        }

        result = !result;
      }
    }

    const RemuxView & view_;
    std::string name_;
  };


  //----------------------------------------------------------------
  // TrackDataSource
  //
  struct TrackDataSource : public TDataSource
  {
    TrackDataSource():
      min_(std::numeric_limits<double>::max()),
      max_(-std::numeric_limits<double>::max())
    {}

    // virtual:
    std::size_t size() const
    { return data_.size(); }

    // virtual:
    double get(std::size_t i) const
    { return data_[i]; }

    // virtual:
    void get_range(double & min, double & max) const
    {
      min = min_;
      max = max_;
    }

    std::vector<double> data_;
    double min_;
    double max_;
  };

  //----------------------------------------------------------------
  // DtsDataSource
  //
  struct DtsDataSource : public TrackDataSource
  {
    DtsDataSource(const Timeline::Track & track)
    {
      data_.resize(track.dts_.size());
      for (std::size_t i = 0, end = data_.size(); i < end; i++)
      {
        double v = track.dts_[i].sec();
        min_ = std::min(min_, v);
        max_ = std::max(max_, v);
        data_[i] = v;
      }
    }
  };

  //----------------------------------------------------------------
  // DtsDtsDataSource
  //
  // f(i) = dts(i+1) - dts(i), should be monotonically increasing
  //
  struct DtsDtsDataSource : public TrackDataSource
  {
    DtsDtsDataSource(const Timeline::Track & track)
    {
      data_.resize(track.dts_.size() - 1);
      for (std::size_t i = 0, end = data_.size(); i < end; i++)
      {
        double v = (track.dts_[i + 1] - track.dts_[i]).sec();
        min_ = std::min(min_, v);
        max_ = std::max(max_, v);
        data_[i] = v;
      }
    }
  };


  //----------------------------------------------------------------
  // PtsPtsDataSource
  //
  // f(i) = pts(i+1) - pts(i), use only for audio tracks
  //
  struct PtsPtsDataSource : public TrackDataSource
  {
    PtsPtsDataSource(const Timeline::Track & track)
    {
      data_.resize(track.pts_.size() - 1);
      for (std::size_t i = 0, end = data_.size(); i < end; i++)
      {
        double v = (track.pts_[i + 1] - track.pts_[i]).sec();
        min_ = std::min(min_, v);
        max_ = std::max(max_, v);
        data_[i] = v;
      }
    }
  };


  //----------------------------------------------------------------
  // PtsDtsDataSource
  //
  // f(i) = pts(i) - dts(i), must be non-negative, reveals b-frames
  //
  struct PtsDtsDataSource : public TrackDataSource
  {
    PtsDtsDataSource(const Timeline::Track & track)
    {
      data_.resize(track.dts_.size());
      for (std::size_t i = 0, end = data_.size(); i < end; i++)
      {
        double v = (track.pts_[i] - track.dts_[i]).sec();
        min_ = std::min(min_, v);
        max_ = std::max(max_, v);
        data_[i] = v;
      }
    }
  };


  //----------------------------------------------------------------
  // PktSizeDataSource
  //
  struct PktSizeDataSource : public TrackDataSource
  {
    PktSizeDataSource(const Timeline::Track & track)
    {
      data_.resize(track.size_.size());
      for (std::size_t i = 0, end = data_.size(); i < end; i++)
      {
        double v = double(track.size_[i]);
        min_ = std::min(min_, v);
        max_ = std::max(max_, v);
        data_[i] = v;
      }
    }
  };

  //----------------------------------------------------------------
  // pick_color
  //
  ColorRef
  pick_color(const TGradient & g, std::size_t plot_index)
  {
    std::size_t num_colors = g.size();
    double t = double(plot_index % num_colors);
    const Color & c = get_ordinal(g, t);
    return ColorRef::constant(c);
  }

  //----------------------------------------------------------------
  // TogglePlot
  //
  struct TogglePlot : public InputArea
  {
    TogglePlot(const char * id,
               ItemView & view,
               PlotItem & plot):
      InputArea(id),
      view_(view),
      plot_(plot)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      bool visible = plot_.visible_.get();
      plot_.visible_ = BoolRef::constant(!visible);
      view_.requestUncache(&plot_);
      view_.requestRepaint();
      return true;
    }

    ItemView & view_;
    PlotItem & plot_;
  };

  //----------------------------------------------------------------
  // PlotOpacity
  //
  struct PlotOpacity : public TDoubleExpr
  {
    PlotOpacity(const PlotItem & plot):
      plot_(plot)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      bool visible = plot_.visible_.get();
      result = visible ? 1.0 : 0.3;
    }

    const PlotItem & plot_;
  };

  //----------------------------------------------------------------
  // add_plot_tag
  //
  static Text &
  add_plot_tag(RemuxView & view,
               Item & item,
               PlotItem & plot,
               const std::string & label,
               Text * prev_tag = NULL)
  {
    const RemuxViewStyle & style = *view.style();

    Text & tag = item.addNew<Text>((plot.id_ + ".tag").c_str());
    tag.anchors_.top_ = prev_tag ?
      ItemRef::reference(*prev_tag, kPropertyBottom, 1, 5) :
      ItemRef::reference(item, kPropertyTop, 1, 5);
    tag.anchors_.right_ = ItemRef::reference(item, kPropertyRight, 1, -5);
    tag.text_ = TVarRef::constant(TVar(label.c_str()));
    tag.fontSize_ = ItemRef::reference(style.row_height_, 0.2875);
    tag.elide_ = Qt::ElideNone;
    tag.color_ = ColorRef::reference(plot, kPropertyColor);
    tag.opacity_  = tag.addExpr(new PlotOpacity(plot));

    TogglePlot & toggle = item.
      add(new TogglePlot((plot.id_ + ".toggle").c_str(), view, plot));
    toggle.anchors_.fill(tag);

    return tag;
  }

  //----------------------------------------------------------------
  // kPlotVertexSpacing
  //
  static const double kPlotVertexSpacing = 2.0;

  //----------------------------------------------------------------
  // GetPlotItemWidth
  //
  struct GetPlotItemWidth : TDoubleExpr
  {
    GetPlotItemWidth(const ItemView & view, double timespan_sec):
      view_(view),
      seconds_(timespan_sec)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double scale = view_.delegate()->device_pixel_ratio();

      // assume 60 fps:
      result = scale * seconds_ * 60 * kPlotVertexSpacing;
    }

    const ItemView & view_;
    double seconds_;
  };

  //----------------------------------------------------------------
  // add_track_plots
  //
  static void
  add_track_plots(RemuxView & view,
                  Item & tags,
                  const TSegmentPtr & timeline_domain,
                  const ItemRef & plot_item_width,
                  const std::string & track_id,
                  const Timeline::Track & track,
                  Item & sv_content,
                  Text *& prev_plot_tag,
                  std::size_t & plot_index)
  {
    static const TGradient gradient = make_gradient
      (
#if 0
       // d3.schemePaired from
       // https://github.com/d3/d3-scale-chromatic/
       // blob/master/src/categorical/Paired.js
       "a6cee3"
       "1f78b4"
       "b2df8a"
       "33a02c"
       "fb9a99"
       "e31a1c"
       "fdbf6f"
       "ff7f00"
       "cab2d6"
       "6a3d9a"
       "ffff99"
       "b15928"
#elif 0
       // https://github.com/d3/d3-scale-chromatic/
                    // blob/master/src/categorical/Set3.js
       "8dd3c7"
       "ffffb3"
       "bebada"
       "fb8072"
       "80b1d3"
       "fdb462"
       "b3de69"
       "fccde5"
       "d9d9d9"
       "bc80bd"
       "ccebc5"
       "ffed6f"
#endif
       );

    bool audio = al::starts_with(track_id, "a:");
    bool video = al::starts_with(track_id, "v:");

    const ItemViewStyle & style = *(view.style());
    ItemRef line_width = ItemRef::reference(style.device_pixel_ratio_);

    TDataSourcePtr data_x(new DtsDataSource(track));

    if (audio || video)
    {
      PlotItem & pkt_size = sv_content.
        addNew<PlotItem>((track_id + ".pkt_size").c_str());
      pkt_size.set_data(data_x, TDataSourcePtr(new PktSizeDataSource(track)));
      pkt_size.anchors_.fill(sv_content);
      pkt_size.anchors_.right_.reset();
      pkt_size.width_ = plot_item_width;
      pkt_size.color_ = pick_color(gradient, plot_index);
      pkt_size.line_width_ = line_width;

      pkt_size.set_domain(timeline_domain);
      timeline_domain->expand(pkt_size.data_x()->range());

      pkt_size.set_range(view.size_range_);
      view.size_range_->expand(pkt_size.data_y()->range());

      prev_plot_tag = &add_plot_tag(view,
                                    tags,
                                    pkt_size,
                                    "packet size, " + track_id,
                                    prev_plot_tag);
      plot_index++;
    }
#if 1
    if (audio)
    {
      PlotItem & pts_pts = sv_content.
        addNew<PlotItem>((track_id + ".pts_pts").c_str());
      pts_pts.set_data(data_x, TDataSourcePtr(new PtsPtsDataSource(track)));
      pts_pts.anchors_.fill(sv_content);
      pts_pts.anchors_.right_.reset();
      pts_pts.width_ = plot_item_width;
      pts_pts.color_ = pick_color(gradient, plot_index);
      pts_pts.line_width_ = line_width;

      pts_pts.set_domain(timeline_domain);
      timeline_domain->expand(pts_pts.data_x()->range());

      pts_pts.set_range(view.time_range_);
      view.time_range_->expand(pts_pts.data_y()->range());

      prev_plot_tag = &add_plot_tag(view,
                                    tags,
                                    pts_pts,
                                    "pts(i+1) - pts(i), " + track_id,
                                    prev_plot_tag);
      plot_index++;
    }
#endif
    if (video)
    {
      PlotItem & pts_dts = sv_content.
        addNew<PlotItem>((track_id + ".pts_dts").c_str());
      pts_dts.set_data(data_x, TDataSourcePtr(new PtsDtsDataSource(track)));
      pts_dts.anchors_.fill(sv_content);
      pts_dts.anchors_.right_.reset();
      pts_dts.width_ = plot_item_width;
      pts_dts.color_ = pick_color(gradient, plot_index);
      pts_dts.line_width_ = line_width;

      pts_dts.set_domain(timeline_domain);
      timeline_domain->expand(pts_dts.data_x()->range());

      pts_dts.set_range(view.time_range_);
      view.time_range_->expand(pts_dts.data_y()->range());

      prev_plot_tag = &add_plot_tag(view,
                                    tags,
                                    pts_dts,
                                    "pts(i) - dts(i), " + track_id,
                                    prev_plot_tag);
      plot_index++;

      PlotItem & dts_dts = sv_content.
        addNew<PlotItem>((track_id + ".dts_dts").c_str());
      dts_dts.set_data(data_x, TDataSourcePtr(new DtsDtsDataSource(track)));
      dts_dts.anchors_.fill(sv_content);
      dts_dts.anchors_.right_.reset();
      dts_dts.width_ = plot_item_width;
      dts_dts.color_ = pick_color(gradient, plot_index);
      dts_dts.line_width_ = line_width;

      dts_dts.set_domain(timeline_domain);
      timeline_domain->expand(dts_dts.data_x()->range());

      dts_dts.set_range(view.time_range_);
      view.time_range_->expand(dts_dts.data_y()->range());

      prev_plot_tag = &add_plot_tag(view,
                                    tags,
                                    dts_dts,
                                    "dts(i+1) - dts(i), " + track_id,
                                    prev_plot_tag);
      plot_index++;
    }
  }

  //----------------------------------------------------------------
  // layout_source_item_track
  //
  static Item &
  layout_source_item_track(RemuxView & view,
                           Item & prog,
                           const std::string & src_name,
                           const DemuxerSummary & summary,
                           const std::string & track_id,
                           const Timeline::Track & tt,
                           bool redact_track)
  {
    if (redact_track)
    {
      RemuxModel * model = view.model();
      SetOfTracks & redacted = model->redacted_[src_name];
      redacted.insert(track_id);
    }

    // shortcuts:
    const ItemViewStyle & style = *(view.style());
    TrackPtr track = yae::get(summary.decoders_, track_id);
    const char * track_name = track ? track->getName() : NULL;
    const char * track_lang = track ? track->getLang() : NULL;

    std::ostringstream oss;
    oss << track_id;

    const char * codec_name = track ? track->getCodecName() : NULL;
    if (codec_name)
    {
      oss << ", " << codec_name;
    }

    if (track_name)
    {
      oss << ", " << track_name;
    }

    if (track_lang)
    {
      oss << " (" << track_lang << ")";
    }

    VideoTrackPtr video =
      boost::dynamic_pointer_cast<VideoTrack, Track>(track);

    if (video)
    {
      VideoTraits traits;
      video->getTraits(traits);

      double par = (traits.pixelAspectRatio_ != 0.0 &&
                    traits.pixelAspectRatio_ != 1.0 ?
                    traits.pixelAspectRatio_ : 1.0);
      unsigned int w = (unsigned int)(0.5 + par * traits.visibleWidth_);
      oss << ", " << w << " x " << traits.visibleHeight_
          << ", " << traits.frameRate_ << " fps";
    }

    AudioTrackPtr audio =
      boost::dynamic_pointer_cast<AudioTrack, Track>(track);

    if (audio)
    {
      AudioTraits traits;
      audio->getTraits(traits);

      oss << ", " << traits.sample_rate_ << " Hz"
          << ", " << traits.ch_layout_.nb_channels << " channels";
    }

    Item & row = prog.addNew<Item>(str("track_", track_id).c_str());
    row.height_ = ItemRef::reference(style.row_height_);
    row.margins_.set_left(ItemRef::reference(style.row_height_, 1));

    CheckboxItem & cbox = row.add(new CheckboxItem("cbox", view));
    cbox.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
    cbox.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
    cbox.height_ = ItemRef::scale(style.row_height_, 0.75);
    cbox.width_ = cbox.height_;
    cbox.checked_ = cbox.addInverse(new IsRedacted(view, src_name, track_id));
    cbox.on_toggle_.reset(new OnToggleRedacted(view, src_name, track_id));

    Text & text = row.addNew<Text>("text");
    text.anchors_.left_ = ItemRef::reference(cbox, kPropertyRight);
    text.margins_.set_left(ItemRef::reference(cbox, kPropertyWidth, 0.5));
    text.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
    text.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
    text.text_ = TVarRef::constant(TVar(oss.str().c_str()));

    InputProxy & proxy = row.addNew<InputProxy>
      (str("proxy_", track_id).c_str());
    proxy.anchors_.left_ = ItemRef::reference(cbox, kPropertyLeft);
    proxy.anchors_.right_ = ItemRef::reference(text, kPropertyRight);
    proxy.anchors_.top_ = ItemRef::reference(row, kPropertyTop);
    proxy.height_ = ItemRef::reference(row, kPropertyHeight);

    proxy.ia_ = cbox.self_;

    int track_focus_index = ItemFocus::singleton().getGroupOffset("sources");
    ItemFocus::singleton().setFocusable(view,
                                        proxy,
                                        "sources",
                                        track_focus_index);
    return row;
  }

  //----------------------------------------------------------------
  // layout_source_item_prog
  //
  static Item *
  layout_source_item_prog(RemuxView & view,
                          Item & src_item,
                          Item * prev_row,
                          const std::string & src_name,
                          const DemuxerSummary & summary,
                          int prog_id,
                          const Timeline & timeline,
                          double min_rows = 0.0,
                          bool redact_program = false)
  {
    // shortcut:
    const ItemViewStyle & style = *(view.style());
    const TProgramInfo & program = yae::at(summary.programs_, prog_id);

    Item & prog = src_item.addNew<Item>(str("prog_", prog_id).c_str());
    prog.anchors_.top_ =
      prev_row ?
      ItemRef::reference(*prev_row, kPropertyBottom) :
      ItemRef::reference(src_item, kPropertyTop);
    prog.anchors_.left_ = ItemRef::reference(src_item, kPropertyLeft);
    prog.margins_.set_left(ItemRef::reference(style.row_height_, 1));

    // keep track of number of rows:
    int num_rows = 0;

    // add source/program name row:
    {
      std::ostringstream oss;
      oss << src_name;

      if (summary.timeline_.size() > 1)
      {
        oss << ", program " << prog_id;

        std::string prog_name = yae::get(program.metadata_, "service_name");
        if (!prog_name.empty())
        {
          oss << ", " << prog_name;
        }
      }

      Item & row = src_item.addNew<Item>("title_row");
      row.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft);
      row.anchors_.top_ = ItemRef::reference(prog, kPropertyTop);
      row.height_ = ItemRef::reference(style.row_height_);

      Text & text = row.addNew<Text>("text");
      text.anchors_.left_ = ItemRef::reference(row, kPropertyLeft);
      text.anchors_.vcenter_ = ItemRef::reference(row, kPropertyVCenter);
      text.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
      QString title = QString::fromUtf8(oss.str().c_str());
      text.text_ = TVarRef::constant(TVar(title));
      num_rows++;
      prev_row = &row;
    }

    // first video:
    for (std::map<std::string, Timeline::Track>::const_iterator
           j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
    {
      // shortcuts:
      const std::string & track_id = j->first;
      if (!al::starts_with(track_id, "v:"))
      {
        continue;
      }

      const Timeline::Track & track = j->second;
      Item & row = layout_source_item_track(view,
                                            prog,
                                            src_name,
                                            summary,
                                            track_id,
                                            track,
                                            redact_program);
      row.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
      row.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft);
      num_rows++;
      prev_row = &row;
    }

    // then everything except video:
    for (std::map<std::string, Timeline::Track>::const_iterator
           j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
    {
      // shortcuts:
      const std::string & track_id = j->first;
      if (// al::starts_with(track_id, "_:") ||
          al::starts_with(track_id, "v:"))
      {
        continue;
      }

      const Timeline::Track & track = j->second;
      Item & row = layout_source_item_track(view,
                                            prog,
                                            src_name,
                                            summary,
                                            track_id,
                                            track,
                                            redact_program);
      row.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
      row.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft);
      num_rows++;
      prev_row = &row;
    }

    double padding_rows = min_rows - double(num_rows);
    if (padding_rows > 0.0)
    {
      Item & spacer = src_item.addNew<Item>("spacer");
      spacer.anchors_.left_ = ItemRef::constant(0);
      spacer.anchors_.top_ = ItemRef::reference(*prev_row, kPropertyBottom);
      spacer.height_ = ItemRef::reference(style.row_height_, padding_rows);
      spacer.width_ = ItemRef::constant(0);
      prev_row = &spacer;
    }

    return prev_row;
  }

  //----------------------------------------------------------------
  // TimeFormatter
  //
  struct TimeFormatter : public IDataFormatter
  {
    // virtual:
    std::string get(double seconds) const
    {
      std::string txt = yae::to_short_txt(seconds);
      return txt;
    }
  };

  //----------------------------------------------------------------
  // PlotBottomPos
  //
  struct PlotBottomPos : TDoubleExpr
  {
    PlotBottomPos(Item & tracks, Item & tags, Item & plot):
      tracks_(tracks),
      tags_(tags),
      plot_(plot)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double top = std::min(tracks_.top(), tags_.top());
      double bottom = std::max(tracks_.bottom(), tags_.bottom());
      double tags_h = bottom - top;
      double plot_h = plot_.height();
      double height = std::max(plot_h, tags_h + plot_h * 0.78);
      result = top + height;
    }

    Item & tracks_;
    Item & tags_;
    Item & plot_;
  };

  //----------------------------------------------------------------
  // RemuxView::append_source
  //
  void
  RemuxView::append_source(const std::string & name,
                           const TDemuxerInterfacePtr & src)
  {
    RemuxModel & model = *model_;
    if (yae::has(model.demuxer_, name))
    {
      return;
    }

    model.sources_.push_back(name);
    model.demuxer_[name] = src;
    model.source_[src] = name;

    RemuxView & view = *this;
    const RemuxViewStyle & style = *style_;
    Item & root = *root_;

    Item & sources = root["sources"];
    Scrollview & ssv = sources.get<Scrollview>("sources.scrollview");
    Item & ssv_content = *(ssv.content_);

    Item & item = ssv_content.addNew<Item>(name.c_str());
    source_item_[name] = item.self_.lock();
    item.anchors_.left_ = ItemRef::reference(ssv_content, kPropertyLeft);
    item.anchors_.top_ = item.addExpr(new SourceItemTop(view, name));

    Rectangle & bg = item.addNew<Rectangle>("bg");

    Item & plots = item.addNew<Item>("plots");
    plots.anchors_.top_ = ItemRef::reference(item, kPropertyTop);
    plots.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
    plots.anchors_.right_ = ItemRef::reference(ssv, kPropertyRight);

    Item & src_item = item.addNew<Item>("src_item");
    src_item.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
    src_item.anchors_.top_ = ItemRef::reference(item, kPropertyTop);

    // layout the source name row:
    Item * prev_row = NULL;

    // layout the source programs and program track plots:
    double rows_per_plot = 10.0;
    const DemuxerSummary & summary = src->summary();
    const std::string clip_track_id = summary.suggest_clip_track_id();

    for (std::map<int, Timeline>::const_iterator
           i = summary.timeline_.begin(); i != summary.timeline_.end(); ++i)
    {
      int prog_id = i->first;
      const Timeline & timeline = i->second;

      bool redact_program = !yae::has(timeline.tracks_, clip_track_id);
      layout_source_item_prog(view,
                              src_item,
                              prev_row,
                              name,
                              summary,
                              prog_id,
                              timeline,
                              rows_per_plot,
                              redact_program);
      std::string prog_str = str("prog_", prog_id);
      Item & tracks = src_item[prog_str.c_str()];

      Item & prog = plots.addNew<Item>(prog_str.c_str());
      prev_row = &prog;

      Item & tags = plots.addNew<Item>(str("tags_", prog_id).c_str());
      tags.anchors_.top_ = ItemRef::reference(tracks, kPropertyTop);
      tags.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
      tags.anchors_.right_ = ItemRef::reference(ssv, kPropertyRight);
      tags.margins_.set_top(ItemRef::reference(style.row_height_, 0.33));
      tags.margins_.set_right(ItemRef::reference(style.row_height_, 0.67));

      prog.height_ = ItemRef::reference(style.row_height_, rows_per_plot);
      prog.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
      prog.anchors_.right_ = ItemRef::reference(ssv, kPropertyRight);
      prog.anchors_.bottom_ = prog.
        addExpr(new PlotBottomPos(tracks, tags, prog));


      Scrollview & psv = layout_scrollview(kScrollbarHorizontal,
                                           view,
                                           style,
                                           prog,
                                           // kScrollbarHorizontal,
                                           kScrollbarNone,
                                           false); // no clipping
      Item & psv_content = *(psv.content_);
      psv_content.height_ = ItemRef::reference(psv, kPropertyHeight);

      Text * prev_plot_tag = NULL;
      std::size_t plot_index = 0;
      std::size_t max_points = 0;

      // put all the plots on the same time scale:
      const Timespan & timespan = timeline.bbox_dts_;
      double timespan_sec = timespan.duration_sec();

      ItemRef plot_item_width = psv_content.addExpr
        (new GetPlotItemWidth(view, timespan_sec));

      TSegmentPtr timeline_domain(new Segment(timespan.t0_.sec(),
                                              timespan_sec));

      // first video:
      for (Timeline::TTracks::const_iterator
             j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
      {
        const Timeline::Track & track = j->second;
        max_points = std::max(max_points, track.dts_.size());

        const std::string & track_id = j->first;
        if (!al::starts_with(track_id, "v:"))
        {
          continue;
        }

        add_track_plots(view,
                        tags,
                        timeline_domain,
                        plot_item_width,
                        track_id,
                        track,
                        psv_content,
                        prev_plot_tag,
                        plot_index);
      }
#if 1
      // then everything except video:
      for (Timeline::TTracks::const_iterator
             j = timeline.tracks_.begin(); j != timeline.tracks_.end(); ++j)
      {
        const std::string & track_id = j->first;
        if (al::starts_with(track_id, "v:"))
        {
          continue;
        }

        const Timeline::Track & track = j->second;
        add_track_plots(view,
                        tags,
                        timeline_domain,
                        plot_item_width,
                        track_id,
                        track,
                        psv_content,
                        prev_plot_tag,
                        plot_index);
      }
#endif

      AxisItem & x_axis = psv_content.addNew<AxisItem>("x_axis");
      x_axis.anchors_.fill(psv_content);
      x_axis.anchors_.right_.reset();
      x_axis.width_ = plot_item_width;
      x_axis.t0_ = ItemRef::constant(timespan.t0_.sec());
      x_axis.t1_ = ItemRef::constant(timespan.t1_.sec());
      x_axis.font_size_ = ItemRef::reference(style.row_height_, 0.2875);
      x_axis.tick_dt_ = 1;
      x_axis.mark_n_ = 2;
      x_axis.formatter_.reset(new TimeFormatter());
    }

    // setup source background:
    bg.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
    bg.anchors_.top_ = ItemRef::reference(item, kPropertyTop);
    bg.anchors_.right_ = ItemRef::reference(ssv, kPropertyRight);
    bg.anchors_.bottom_ = ItemRef::reference(plots, kPropertyBottom);
    bg.visible_ = bg.addExpr(new SourceItemVisible(view, name));
    bg.opacity_ = ItemRef::constant(0.25);

    dataChanged();
  }

  //----------------------------------------------------------------
  // RemuxView::append_clip
  //
  void
  RemuxView::append_clip(const TClipPtr & clip)
  {
    RemuxModel & model = *model_;
    model.clips_.push_back(clip);
    add_clip_row(clip);
  }

  //----------------------------------------------------------------
  // RemuxView::add_clip_row
  //
  void
  RemuxView::add_clip_row(const TClipPtr & clip)
  {
    RemuxModel & model = *model_;
    RemuxView & view = *this;
    Item & root = *root_;

    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");
    Item & clips_add = sv.content_->get<Item>("clips_add");
    clips_add.uncache();

    std::size_t index = clip_list.children_.size();
    ClipItem & row = clip_list.add(new ClipItem("row", model, view, index));
    row.anchors_.left_ = ItemRef::reference(clip_list, kPropertyLeft);
    row.anchors_.right_ = ItemRef::reference(clip_list, kPropertyRight);
    row.anchors_.top_ = (index > 0) ?
      row.addExpr(new OddRoundUp(*(clip_list.children_[index - 1]),
                                 kPropertyBottom)) :
      row.addExpr(new OddRoundUp(clip_list, kPropertyTop));
    row.height_ =
      row.addExpr(new OddRoundUp(clips_add, kPropertyHeight));

    Rectangle & bg = row.addNew<Rectangle>("bg");
    bg.anchors_.fill(row);
    bg.color_ = bg.addExpr(new ClipItemColor(model, view, index, row));
    bg.color_.disableCaching();

    layout_clip(model, view, *style_, row, index);

    Item & gops = root["layout"]["gops"];
    clips_[clip] = layout_gops(model, view, *style_, gops, clip);

    capture_playhead_position();
    maybe_layout_gops();
    maybe_update_player();

    dataChanged();

#if 0 // ndef NDEBUG
    sv.content_->dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // rebuild_ranges
  //
  static void
  rebuild_ranges(Item & src_item)
  {
    Item & plots = src_item.get<Item>("plots");
    for (std::vector<ItemPtr>::iterator i = plots.children_.begin();
         i != plots.children_.end(); ++i)
    {
      Item & item = *(*i);
      if (!al::starts_with(item.id_, "prog_"))
      {
        continue;
      }

      Scrollview & sv =
        item.get<Scrollview>((item.id_ + ".scrollview").c_str());

      Item & container = *sv.content_;
      for (std::vector<ItemPtr>::iterator j = container.children_.begin();
           j != container.children_.end(); ++j)
      {
        yae::shared_ptr<PlotItem, Item> plot_ptr = *j;
        if (!plot_ptr)
        {
          continue;
        }

        PlotItem & plot = *plot_ptr;
        if (!(plot.data_y() && plot.range()))
        {
          continue;
        }

        Segment & range = *(plot.range());
        range.expand(plot.data_y()->range());
      }
    }
  }

  //----------------------------------------------------------------
  // prune
  //
  static void
  prune(RemuxModel & model, RemuxView & view)
  {
    AsyncTaskQueue::Pause pause(async_task_queue());

    std::set<TDemuxerInterfacePtr> set_of_demuxers;
    for (std::vector<TClipPtr>::const_iterator i = model.clips_.begin();
         i != model.clips_.end(); ++i)
    {
      const TClipPtr & clip = *i;
      set_of_demuxers.insert(clip->demuxer_);
    }

    Item & root = *(view.root());
    Item & sources = root["sources"];
    Scrollview & ssv = sources.get<Scrollview>("sources.scrollview");
    Item & ssv_content = *(ssv.content_);

    // rebuild ranges:
    view.time_range_->clear();
    view.size_range_->clear();
    view.time_range_->expand(Segment(0, 1));

    std::map<std::string, TDemuxerInterfacePtr>::iterator
      i = model.demuxer_.begin();
    while (i != model.demuxer_.end())
    {
      std::string source = i->first;
      TDemuxerInterfacePtr demuxer = i->second;
      if (yae::has(set_of_demuxers, demuxer))
      {
        // rebuild ranges:
        ItemPtr source_item = view.source_item_[source];
        rebuild_ranges(*source_item);
        ++i;
      }
      else
      {
        // unused, remove:
        std::map<std::string, TDemuxerInterfacePtr>::iterator next = i;
        std::advance(next, 1);
        model.sources_.remove(source);
        model.demuxer_.erase(i);
        model.source_.erase(demuxer);
        view.gops_.erase(demuxer);
        view.gops_row_lut_.erase(demuxer);
        ItemPtr source_item = view.source_item_[source];
        view.source_item_.erase(source);
        ssv_content.remove(source_item);
        i = next;
      }
    }
  }

  //----------------------------------------------------------------
  // RemuxView::remove_clip
  //
  void
  RemuxView::remove_clip(std::size_t index)
  {
    if (view_mode_ != RemuxView::kLayoutMode)
    {
      YAE_ASSERT(false);
      return;
    }

    Item & root = *root_;
    Item & gops = root["layout"]["gops"];

    RemuxModel & model = *model_;
    TClipPtr clip = model.clips_[index];

    ItemPtr container = get_gops_container(clip);
    YAE_ASSERT(container);
    if (!container)
    {
      return;
    }

    gops.remove(container);
    clips_.erase(clip);

    model.clips_.erase(model.clips_.begin() + index);
    selected_ = std::min(index, model.clips_.size() - 1);
    prune(model, *this);

    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
    Item & clip_list = sv.content_->get<Item>("clip_list");
    clip_list.children_.pop_back();

#if 0 // ndef NDEBUG
    sv.content_->dump(std::cerr);
#endif

    AsyncTaskQueue & queue = async_task_queue();
    queue.wait_until_empty();

    dataChanged();
  }

  //----------------------------------------------------------------
  // RemuxView::repeat_clip
  //
  void
  RemuxView::repeat_clip()
  {
    if (view_mode_ != RemuxView::kLayoutMode)
    {
      YAE_ASSERT(false);
      return;
    }

    RemuxModel & model = *model_;
    TClipPtr clip_ptr = selected_clip();
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
    std::size_t new_index = selected_ + 1;
    if (selected_ < model.clips_.size())
    {
      model.clips_.insert(model.clips_.begin() + selected_ + 1,
                          new_clip);
    }
    else
    {
      model.clips_.push_back(new_clip);
      new_index = model.clips_.size() - 1;
    }

    // copy scrollview position from source scroll view to the new scroll view:
    add_clip_row(new_clip);

    Item & root = *root_;
    Item & gops = root["layout"]["gops"];
    Scrollview & new_sv =
      gops.children_.back()->get<Scrollview>("clip_layout.scrollview");

    for (std::vector<ItemPtr>::iterator
           i = gops.children_.begin(); i != gops.children_.end(); ++i)
    {
      const Item & item = *(*i);

      yae::shared_ptr<IsCurrentClip, TBoolExpr> found =
        item.visible_.get_expr<IsCurrentClip>();

      if (found && found->clip_ == clip_ptr)
      {
        const Scrollview & src_sv =
          item.get<Scrollview>("clip_layout.scrollview");

        new_sv.set_position_x(src_sv.position_x());
        new_sv.set_position_y(src_sv.position_y());
        break;
      }
    }

    // select the new clip:
    selected_ = new_index;
  }

  //----------------------------------------------------------------
  // RemuxView::relayout_gops
  //
  void
  RemuxView::relayout_gops(const TDemuxerInterfacePtr & demuxer)
  {
    RemuxModel & model = *model_;
    RemuxView & view = *this;
    Item & root = *root_;
    Item & gops = root["layout"]["gops"];

    for (std::map<TClipPtr, ItemPtr>::iterator
           i = clips_.begin(); i != clips_.end(); ++i)
    {
      const TClipPtr & clip_ptr = i->first;
      if (clip_ptr->demuxer_ != demuxer)
      {
        continue;
      }

      ItemPtr & container = i->second;
      gops.remove(container);
      container.reset();
    }

    gops_.erase(demuxer);
    gops_row_lut_.erase(demuxer);

    TGopCache & cache = gop_cache();
    cache.purge_unreferenced_entries();

    for (std::map<TClipPtr, ItemPtr>::iterator
           i = clips_.begin(); i != clips_.end(); ++i)
    {
      const TClipPtr & clip_ptr = i->first;
      if (clip_ptr->demuxer_ != demuxer)
      {
        continue;
      }

      ItemPtr & container = i->second;
      container = layout_gops(model, view, *style_, gops, clip_ptr);
    }

    model_json_str_.clear();
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
    root.clear();
    root.addHidden(style_);
    root.uncache();
    uncache_.clear();

    style_->layout_->layout(root, *this, *model_, *style_);

#if 0 // ndef NDEBUG
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
    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
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
    const FramerateEstimator & fe = yae::at(summary.fps_, clip.track_);
    double fps = fe.best_guess();
    parse_time(clip.keep_.*field, text.c_str(), NULL, NULL, fps);

    Item & root = *view.root();
    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
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

    TClipPtr clip_ptr = view.current_clip();
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

    Item & gops = sv->content_->get<Item>("gop_items");
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
  // get_cursor_dst_pts
  //
  static bool
  get_cursor_dst_pts(const RemuxView & view,
                     TTime & dts,
                     TTime & pts,
                     const Gop *& gop_ptr,
                     std::size_t & dts_order_index,
                     std::size_t & pts_order_index)
  {
    if (!view.model())
    {
      return false;
    }

    TClipPtr clip_ptr = view.current_clip();
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

      dts = track.dts_span_.back().t1_ + track.dur_.back();
      pts = track.pts_span_.back().t1_ + track.dur_.back();
      return true;
    }

    const Gop & gop = gop_item->gop();
    gop_ptr = &gop;

    std::vector<std::size_t> lut;
    get_pts_order_lut(gop, lut);

    pts_order_index = frame->frameIndex();
    std::size_t k = pts_order_index - gop.i0_;
    dts_order_index = lut[k];
    dts = track.dts_[dts_order_index];
    pts = track.pts_[dts_order_index];
#if 0
    // use the decoded PTS time, if available:
    TVideoFramePtr vf = frame->videoFrame();
    if (vf)
    {
      pts = vf->time_;
    }
#endif
    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::set_in_point
  //
  void
  RemuxView::set_in_point()
  {
    if (view_mode_ != RemuxView::kLayoutMode)
    {
      return;
    }

    TTime dts;
    TTime pts;
    const Gop * gop = NULL;
    std::size_t dts_order_ix = std::numeric_limits<std::size_t>::max();
    std::size_t pts_order_ix = std::numeric_limits<std::size_t>::max();

    if (!get_cursor_dst_pts(*this, dts, pts, gop, dts_order_ix, pts_order_ix))
    {
      return;
    }

    Clip & clip = *selected_clip();
    clip.keep_.t0_ = pts;

    if (clip.keep_.t1_ <= clip.keep_.t0_)
    {
      const Timeline::Track & track = clip.get_track_timeline();
      clip.keep_.t1_ = track.pts_span_.back().t1_;
    }

    Item & root = *root_;
    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
    requestUncache(&sv);
    requestRepaint();
  }

  //----------------------------------------------------------------
  // RemuxView::set_out_point
  //
  void
  RemuxView::set_out_point()
  {
    if (view_mode_ != RemuxView::kLayoutMode)
    {
      return;
    }

    TTime dts;
    TTime pts;
    const Gop * gop = NULL;
    std::size_t dts_order_ix = std::numeric_limits<std::size_t>::max();
    std::size_t pts_order_ix = std::numeric_limits<std::size_t>::max();

    if (!get_cursor_dst_pts(*this, dts, pts, gop, dts_order_ix, pts_order_ix))
    {
      return;
    }

    Clip & clip = *selected_clip();
    clip.keep_.t1_ = pts;

    if (clip.keep_.t1_ <= clip.keep_.t0_)
    {
      const Timeline::Track & track = clip.get_track_timeline();
      clip.keep_.t0_ = track.pts_span_.front().t0_;
    }

    Item & root = *root_;
    Scrollview & sv =
      root["layout"]["clips"].get<Scrollview>("clips.scrollview");
    requestUncache(&sv);
    requestRepaint();
  }

  //----------------------------------------------------------------
  // RemuxView::set_view_mode
  //
  void
  RemuxView::set_view_mode(RemuxView::ViewMode mode)
  {
    PlayerUxItem & pl_ux = *pl_ux_;

    // re-use some of the player actions:
    bool ok = true;
    ok = disconnect(pl_ux.actionSetInPoint_, SIGNAL(triggered()),
                    this, SLOT(set_in_point()));

    ok = disconnect(pl_ux.actionSetOutPoint_, SIGNAL(triggered()),
                    this, SLOT(set_out_point()));

    ok = disconnect(pl_ux.actionSetInPoint_, SIGNAL(triggered()),
                    &pl_ux.timeline_model(), SLOT(setInPoint()));

    ok = disconnect(pl_ux.actionSetOutPoint_, SIGNAL(triggered()),
                    &pl_ux.timeline_model(), SLOT(setOutPoint()));

    if (mode == RemuxView::kPlayerMode)
    {
      pl_ux.actionFullScreen_->setText(tr("&Full Screen (letterbox)"));

      ok = connect(pl_ux.actionSetInPoint_, SIGNAL(triggered()),
                   &pl_ux.timeline_model(), SLOT(setInPoint()));
      YAE_ASSERT(ok);

      ok = connect(pl_ux.actionSetOutPoint_, SIGNAL(triggered()),
                   &pl_ux.timeline_model(), SLOT(setOutPoint()));
      YAE_ASSERT(ok);
    }
    else
    {
      pl_ux.actionFullScreen_->setText(tr("&Full Screen"));

      if (mode == RemuxView::kLayoutMode)
      {
        ok = connect(pl_ux.actionSetInPoint_, SIGNAL(triggered()),
                     this, SLOT(set_in_point()));
        YAE_ASSERT(ok);

        ok = connect(pl_ux.actionSetOutPoint_, SIGNAL(triggered()),
                     this, SLOT(set_out_point()));
        YAE_ASSERT(ok);
      }
    }

    menuEdit_->clear();
    menuView_->clear();

    if (mode == RemuxView::kLayoutMode)
    {
      menuEdit_->addAction(pl_ux.actionSetInPoint_);
      menuEdit_->addAction(pl_ux.actionSetOutPoint_);
    }

    if (mode != RemuxView::kPlayerMode)
    {
      menuView_->addAction(pl_ux.actionFullScreen_);
    }

    bool changing = view_mode_ != mode;
    if (!changing)
    {
      return;
    }

    capture_playhead_position();

    // change view mode:
    view_mode_ = mode;

    maybe_layout_gops();
    maybe_update_player();

    enable_focus_group();

    requestRepaint();

    emit view_mode_changed();
  }

  //----------------------------------------------------------------
  // RemuxView::enable_focus_group
  //
  void
  RemuxView::enable_focus_group()
  {
    ItemFocus::singleton().enable("remux_layout", view_mode_ == kLayoutMode);
    ItemFocus::singleton().enable("player", view_mode_ == kPlayerMode);
    ItemFocus::singleton().enable("sources", view_mode_ == kSourceMode);
  }

  //----------------------------------------------------------------
  // RemuxView::is_playback_paused
  //
  bool
  RemuxView::is_playback_paused()
  {
    return pl_ux_->is_playback_paused();
  }

  //----------------------------------------------------------------
  // RemuxView::toggle_playback
  //
  void
  RemuxView::toggle_playback()
  {
    pl_ux_->togglePlayback();
  }

  //----------------------------------------------------------------
  // RemuxModel::output_clip
  //
  TClipPtr
  RemuxView::output_clip() const
  {
    if (!output_clip_ && model_ && !model_->clips_.empty())
    {
      serial_demuxer_ = model_->make_serial_demuxer();

      const Clip & clip = *(model_->clips_.front());
      std::string track_id;
      Timespan keep;

      if (clip.get(serial_demuxer_, track_id, keep))
      {
        const yae::DemuxerSummary & summary = serial_demuxer_->summary();
        const Timeline::Track & track = summary.get_track_timeline(track_id);
        keep = Timespan(track.pts_.front(), track.pts_.back());
        output_clip_.reset(new Clip(serial_demuxer_, track_id, keep));
      }
    }

    return output_clip_;
  }

  //----------------------------------------------------------------
  // RemuxView::capture_playhead_position
  //
  void
  RemuxView::capture_playhead_position()
  {
    if (view_mode_ != kPlayerMode || !serial_demuxer_ || !reader_)
    {
      return;
    }

    // capture playhead position:
    TTrackInfo track_info;
    reader_->getSelectedVideoTrackInfo(track_info);

    TProgramInfo prog_info;
    reader_->getProgramInfo(track_info.program_, prog_info);

    playhead_.prog_id_ = prog_info.program_;
    playhead_.pts_ = pl_ux_->timeline_model().currentTime();

    if (!serial_demuxer_->map_to_source(playhead_.prog_id_,
                                        playhead_.pts_,
                                        playhead_.src_index_,
                                        playhead_.src_,
                                        playhead_.src_dts_))
    {
      playhead_.src_.reset();
      playhead_.src_index_ = 0;
    }

    pl_ux_->stopPlayback();
    reader_.reset();
  }

  //----------------------------------------------------------------
  // RemuxView::maybe_layout_gops
  //
  void
  RemuxView::maybe_layout_gops()
  {
    if (view_mode_ != kPreviewMode &&
        view_mode_ != kPlayerMode &&
        view_mode_ != kExportMode)
    {
      return;
    }

    Item & root = *root_;
    Item & preview = root["preview"];

    // avoid rebuilding the preview if clip layout hasn't changed:
    std::string model_json_str = model_->to_json_str();
    if (model_json_str_ != model_json_str)
    {
      // remove cached layout data for serial demuxer:
      gops_.erase(serial_demuxer_);
      gops_row_lut_.erase(serial_demuxer_);
      clips_.erase(output_clip_);

      model_json_str_ = model_json_str;
      output_clip_.reset();
      serial_demuxer_.reset();
      preview.clear();
    }

    TClipPtr clip = output_clip();
    if (clip && view_mode_ == kPreviewMode && preview.children_.empty())
    {
      clips_[clip] = layout_gops(*model_, *this, *style_, preview, clip);
    }

    preview.uncache();

#if 0 // ndef NDEBUG
    preview.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // RemuxView::update_player
  //
  void
  RemuxView::maybe_update_player()
  {
    PlayerUxItem & pl_ux = *pl_ux_;

    if (view_mode_ != kPlayerMode)
    {
      pl_ux.setVisible(false);
      return;
    }

    pl_ux.setVisible(true);

    bool hwdec = true;
    reader_.reset(DemuxerReader::create(serial_demuxer_, hwdec));

    const IBookmark * bookmark = NULL;
    bool start_from_zero_time = true;
    pl_ux.playback(reader_, bookmark, start_from_zero_time);

    TTime playhead_pts = playhead_.pts_;
    TDemuxerInterfacePtr playhead_src = playhead_.src_.lock();

    if (!playhead_src && serial_demuxer_)
    {
      // src was removed, seek to the start of the next source:
      if (playhead_.src_index_ < serial_demuxer_->num_sources())
      {
        playhead_src = serial_demuxer_->sources().at(playhead_.src_index_);
      }
      else
      {
        playhead_src = serial_demuxer_->sources().front();
      }

      playhead_.src_dts_ = TTime(0, 1);
    }

    if (serial_demuxer_ &&
        serial_demuxer_->has_program(playhead_.prog_id_))
    {
      serial_demuxer_->map_to_output(playhead_.prog_id_,
                                     playhead_src,
                                     playhead_.src_dts_,
                                     playhead_pts);

      YAE_ASSERT(playhead_pts.valid());
      pl_ux.timeline_model().seekTo(playhead_pts.sec());
    }

    TimelineItem & timeline = *pl_ux.timeline_;
    timeline.maybeAnimateOpacity();
    timeline.forceAnimateControls();

    pl_ux.uncache();
  }

  //----------------------------------------------------------------
  // RemuxView::populateContextMenu
  //
  bool
  RemuxView::populateContextMenu(QMenu & menu)
  {
    PlayerUxItem & pl_ux = *pl_ux_;
    if (pl_ux.visible() && pl_ux.populateContextMenu(menu))
    {
      return true;
    }

    if (view_mode_ == RemuxView::kLayoutMode)
    {
      menu.addAction(pl_ux.actionSetInPoint_);
      menu.addAction(pl_ux.actionSetOutPoint_);
      menu.addSeparator();
    }

    menu.addAction(pl_ux.actionFullScreen_);
    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::popup_context_menu
  //
  bool
  RemuxView::popup_context_menu(const QPoint & global_pos)
  {
    popup_->clear();
    populateContextMenu(*popup_);
    popup_->popup(global_pos);
    return true;
  }

  //----------------------------------------------------------------
  // RemuxView::insert_menus
  //
  void
  RemuxView::insert_menus(QMenuBar * menubar, QAction * before)
  {
    if (view_mode_ == RemuxView::kPlayerMode)
    {
      yae::PlayerUxItem & pl_ux = *pl_ux_;
      pl_ux.insert_menus(pl_ux.player_->reader(), menubar, before);
      pl_ux.uncache();
    }
    else
    {
      if (view_mode_ == RemuxView::kLayoutMode)
      {
        menubar->insertAction(before, menuEdit_->menuAction());
      }

      menubar->insertAction(before, menuView_->menuAction());
    }
  }

}
