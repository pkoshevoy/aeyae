// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 26 13:36:42 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/utils/yae_utils.h"

// local:
#include "yaeAppView.h"
#include "yaeFlickableArea.h"
#include "yaeItemFocus.h"
#include "yaeRectangle.h"
#include "yaeRoundRect.h"
#include "yaeTextInput.h"
#include "yae_axis_item.h"
#include "yae_checkbox_item.h"
#include "yae_input_proxy_item.h"
#include "yae_plot_item.h"
#include "yae_tab_rect.h"


namespace yae
{

  //----------------------------------------------------------------
  // In
  //
  template <AppView::ViewMode mode>
  struct In : public TBoolExpr
  {
    In(const AppView & view): view_(view) {}

    // virtual:
    void evaluate(bool & result) const
    { result = (view_.view_mode() == mode); }

    const AppView & view_;
  };


  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(AppView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.is_playback_paused();
    }

    AppView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    AppView * view = (AppView *)context;
    view->toggle_playback();
  }


  //----------------------------------------------------------------
  // SplitterPos
  //
  struct SplitterPos : TDoubleExpr
  {
    enum Side
    {
      kLeft = 0,
      kRight = 1
    };

    SplitterPos(AppView & view, Item & container, Side side = kLeft):
      view_(view),
      container_(container),
      side_(side)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double unit_size = view_.style_->unit_size_.get();

      if (side_ == kLeft)
      {
        result = container_.left() + unit_size * 0.0;
      }
      else
      {
        result = container_.right() - unit_size * 3.0;
      }
    }

    AppView & view_;
    Item & container_;
    Side side_;
  };

  //----------------------------------------------------------------
  // Splitter
  //
  struct Splitter : public InputArea
  {
    enum Orientation
    {
      kHorizontal = 1,
      kVertical = 2,
    };

    Splitter(const char * id,
             const ItemRef & lowerBound,
             const ItemRef & upperBound,
             Orientation orientation):
      InputArea(id, true),
      orientation_(orientation),
      lowerBound_(lowerBound),
      upperBound_(upperBound),
      posAnchor_(0),
      posOffset_(0)
    {
      pos_ = addExpr(new Pos(*this));
    }

    // virtual:
    void uncache()
    {
      lowerBound_.uncache();
      upperBound_.uncache();
      pos_.uncache();
      InputArea::uncache();
    }

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      posAnchor_ = pos_.get();
      posOffset_ = 0;
      return true;
    }

    // virtual:
    bool onDrag(const TVec2D & itemCSysOrigin,
                const TVec2D & rootCSysDragStart,
                const TVec2D & rootCSysDragEnd)
    {
      TVec2D delta = rootCSysDragEnd - rootCSysDragStart;
      double d = (orientation_ == kHorizontal) ? delta.x() : delta.y();
      posOffset_ = d;
      pos_.uncache();

#if 0
      // this avoids uncaching the scrollview content,
      // but doesn't work well when there are nested scrollviews:
      parent_->uncacheSelfAndChildren();
#else
      // scrollviews with content that should not be uncached by the Splitter
      // should have set sv.uncacheContent_ = false
      parent_->uncache();
#endif
      return true;
    }

    Orientation orientation_;
    ItemRef lowerBound_;
    ItemRef upperBound_;

    //----------------------------------------------------------------
    // Pos
    //
    struct Pos : TDoubleExpr
    {
      Pos(Splitter & splitter):
        splitter_(splitter)
      {}

      // virtual:
      void evaluate(double & result) const
      {
        double v_min = splitter_.lowerBound_.get();
        double v_max = splitter_.upperBound_.get();
        double v = splitter_.posAnchor_ + splitter_.posOffset_;
        result = std::min(v_max, std::max(v_min, v));
      }

      Splitter & splitter_;
    };

    ItemRef pos_;

  protected:
    double posAnchor_;
    double posOffset_;
  };


  //----------------------------------------------------------------
  // ChannelRowTop
  //
  struct ChannelRowTop : TDoubleExpr
  {
    ChannelRowTop(const AppView & view, const Item & ch_list, uint32_t ch_num):
      view_(view),
      ch_list_(ch_list),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      double unit_size = view_.style_->unit_size_.get();
      std::size_t ch_ordinal = yae::at(view_.ch_ordinal_, ch_num_);
      result = ch_list_.top();
      result += (unit_size * 1.12 + 1.0) * double(ch_ordinal);
    }

    const AppView & view_;
    const Item & ch_list_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // gps_time_round_dn
  //
  static uint32_t
  gps_time_round_dn(uint32_t t)
  {
    // avoid rounding down by almost an hour
    // if `t` is less than 60s from a whole hour already:
    t += 59;
    t -= t % 60;

    uint32_t r = t % 3600;
    return t - r;
  }

  //----------------------------------------------------------------
  // gps_time_round_up
  //
  static uint32_t
  gps_time_round_up(uint32_t t)
  {
    // avoid rounding up by almost an hour
    // if `t` is less than 60s from a whole hour already:
    t -= t % 60;

    uint32_t r = t % 3600;
    return r ? t + (3600 - r) : t;
  }

  //----------------------------------------------------------------
  // kTimePixelScaleFactor
  //
  // static const double kTimePixelScaleFactor = 6.159;
  // static const double kTimePixelScaleFactor = 7.175;
  static const double kTimePixelScaleFactor = 7.29;

  //----------------------------------------------------------------
  // seconds_to_px
  //
  static double
  seconds_to_px(const AppView & view, uint32_t sec)
  {
    double unit_size = view.style_->unit_size_.get();
    double px = (unit_size * kTimePixelScaleFactor) * (sec / 3600.0);
    return px;
  }

  //----------------------------------------------------------------
  // px_to_seconds
  //
  static double
  px_to_seconds(const AppView & view, double px)
  {
    double unit_size = view.style_->unit_size_.get();
    double sec = (px * 3600.0) / (unit_size * kTimePixelScaleFactor);
    return sec;
  }

  //----------------------------------------------------------------
  // gps_time_to_px
  //
  static double
  gps_time_to_px(const AppView & view, uint32_t gps_time)
  {
    int64_t gps_now = TTime::gps_now().get(1);

    // round-down to whole hour:
    gps_now = gps_time_round_dn(gps_now);

    bool negative = (gps_time < gps_now);
    uint32_t sec = negative ? (gps_now - gps_time) : (gps_time - gps_now);
    double px = seconds_to_px(view, sec);
    return (negative ? -px : px) + 3.0;
  }


  //----------------------------------------------------------------
  // ProgramTilePos
  //
  struct ProgramTilePos : TDoubleExpr
  {
    ProgramTilePos(const AppView & view, uint32_t gps_time):
      view_(view),
      gps_time_(gps_time)
    {}

    void evaluate(double & result) const
    {
      result = gps_time_to_px(view_, gps_time_);
    }

    const AppView & view_;
    const uint32_t gps_time_;
  };

  //----------------------------------------------------------------
  // ProgramTileWidth
  //
  struct ProgramTileWidth : TDoubleExpr
  {
    ProgramTileWidth(const AppView & view, uint32_t duration):
      view_(view),
      duration_(duration)
    {}

    void evaluate(double & result) const
    {
      result = seconds_to_px(view_, duration_);
    }

    const AppView & view_;
    const uint32_t duration_;
  };

  //----------------------------------------------------------------
  // ProgramRowPos
  //
  struct ProgramRowPos : TDoubleExpr
  {
    ProgramRowPos(const AppView & view, uint32_t ch_num):
      view_(view),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      const yae::mpeg_ts::EPG::Channel & channel =
        yae::at(view_.epg_.channels_, ch_num_);

      if (channel.programs_.empty())
      {
        result = 0.0;
      }
      else
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        uint32_t t0 = gps_time_round_dn(p0.gps_time_);
        result = gps_time_to_px(view_, t0);
      }
    }

    const AppView & view_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // ProgramRowLen
  //
  struct ProgramRowLen : TDoubleExpr
  {
    ProgramRowLen(const AppView & view, uint32_t ch_num):
      view_(view),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      const yae::mpeg_ts::EPG::Channel & channel =
        yae::at(view_.epg_.channels_, ch_num_);

      if (channel.programs_.empty())
      {
        result = 0.0;
      }
      else
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        const yae::mpeg_ts::EPG::Program & p1 = channel.programs_.back();
        uint32_t t0 = gps_time_round_dn(p0.gps_time_);
        uint32_t t1 = gps_time_round_up(p1.gps_time_ + p1.duration_);
        uint32_t dt = t1 - t0;
        result = seconds_to_px(view_, dt);
      }
    }

    const AppView & view_;
    const uint32_t ch_num_;
  };

  //----------------------------------------------------------------
  // CalcDistanceLeftRight
  //
  struct CalcDistanceLeftRight : TDoubleExpr
  {
    CalcDistanceLeftRight(const Item & a,
                          const Item & b):
      a_(a),
      b_(b)
    {}

    void evaluate(double & result) const
    {
      double a = a_.right();
      double b = b_.left();
      result = b - a;
    }

    const Item & a_;
    const Item & b_;
  };


  //----------------------------------------------------------------
  // CalcViewPositionX
  //
  struct CalcViewPositionX : TDoubleExpr
  {
    CalcViewPositionX(const Scrollview & src, const Scrollview & dst):
      src_(src),
      dst_(dst)
    {}

    void evaluate(double & result) const
    {
      double x = 0;
      double w = 0;
      src_.get_content_view_x(x, w);

      const Segment & scene = dst_.content_->xExtent();
      const Segment & view = dst_.xExtent();

      if (scene.length_ <= view.length_)
      {
        result = 0.0;
        return;
      }

#if 0 // disabled to avoid introducing a misalignment between src/dst
      // at position == 1 ... for dst to remain aligned with src
      // position has to be allowed to exceen [0, 1] range:
      if (scene.end() < x + view.length_)
      {
        x = scene.end() - view.length_;
      }
#endif

      double range = scene.length_ - view.length_;
      result = (x - scene.origin_) / range;
    }

    const Scrollview & src_;
    const Scrollview & dst_;
  };


  //----------------------------------------------------------------
  // NowMarkerPos
  //
  struct NowMarkerPos : TDoubleExpr
  {
    NowMarkerPos(const AppView & view,
                 const Item & epg_header,
                 const Scrollview & hsv):
      view_(view),
      epg_header_(epg_header),
      hsv_(hsv)
    {}

    void evaluate(double & result) const
    {
      double x = 0;
      double w = 0;
      hsv_.get_content_view_x(x, w);
      double sec = px_to_seconds(view_, x);

      int64_t gps_now = TTime::gps_now().get(1);
      int64_t origin = gps_time_round_dn(gps_now);
      int64_t dt = gps_now - origin;
      result = epg_header_.left() + seconds_to_px(view_, dt - sec);
    }

    const AppView & view_;
    const Item & epg_header_;
    const Scrollview & hsv_;
  };


  //----------------------------------------------------------------
  // UnitSize
  //
  struct UnitSize : TDoubleExpr
  {
    UnitSize(const ItemView & view):
      view_(view)
    {}

    void evaluate(double & result) const
    {
#if 0
      if (view_.delegate())
      {
        const Canvas::IDelegate & d = *(view_.delegate());
        double h = d.screen_height();
        result = h / 28;
      }
      else
#endif
      {
        result = 38.4;
        // result = 114;
      }
    }

    const ItemView & view_;
  };


  //----------------------------------------------------------------
  // RecButtonColor
  //
  struct RecButtonColor : TColorExpr
  {
    RecButtonColor(const AppView & view, uint32_t ch_num, uint32_t gps_time):
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    void evaluate(Color & result) const
    {
      const AppStyle & style = *(view_.style_);

      std::map<uint32_t, TScheduledRecordings>::const_iterator
        found_ch = view_.schedule_.find(ch_num_);
      if (found_ch != view_.schedule_.end())
      {
        const TScheduledRecordings & schedule = found_ch->second;
        TScheduledRecordings::const_iterator found = schedule.find(gps_time_);

        if (found != schedule.end())
        {
          TRecordingPtr rec_ptr = found->second;
          const Recording & rec = *rec_ptr;

          result = (rec.cancelled_ ?
                    style.bg_epg_cancelled_.get() :
                    style.bg_epg_rec_.get());
          return;
        }
      }

      result = style.fg_epg_.get().a_scaled(0.2);
    }

    const AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };


  //----------------------------------------------------------------
  // DoesItemFit
  //
  struct DoesItemFit : TBoolExpr
  {
    DoesItemFit(const ItemRef & x0,
                const ItemRef & x1,
                const Item & item):
      x0_(x0),
      x1_(x1),
      item_(item)
    {
      x0_.disableCaching();
      x1_.disableCaching();
    }

    // virtual:
    void evaluate(bool & result) const
    {
      double x = item_.left();
      if (x < x0_.get())
      {
        result = false;
        return;
      }

      double w = item_.width();
      if (x + w > x1_.get())
      {
        result = false;
        return;
      }

      result = true;
    }

    ItemRef x0_;
    ItemRef x1_;
    const Item & item_;
  };


  //----------------------------------------------------------------
  // ToggleRecording
  //
  struct ToggleRecording : public InputArea
  {
    ToggleRecording(const char * id,
                    AppView & view,
                    uint32_t ch_num,
                    uint32_t gps_time):
      InputArea(id),
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    // virtual:
    bool onPress(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    { return true; }

    // virtual:
    bool onClick(const TVec2D & itemCSysOrigin,
                 const TVec2D & rootCSysPoint)
    {
      view_.toggle_recording(ch_num_, gps_time_);
      return true;
    }

    AppView & view_;
    uint32_t ch_num_;
    uint32_t gps_time_;
  };


  //----------------------------------------------------------------
  // AppStyle::AppStyle
  //
  AppStyle::AppStyle(const char * id, const AppView & view):
    ItemViewStyle(id, view)
  {
    font_.setFamily(QString::fromUtf8("Lucida Grande"));

    unit_size_ = addExpr(new UnitSize(view));

    bg_sidebar_ = ColorRef::constant(Color(0xE0E9F5, 1.0));
    bg_splitter_ = ColorRef::constant(Color(0x4F4F4f, 1.0));
    bg_epg_ = ColorRef::constant(Color(0xFFFFFF, 1.0));
    fg_epg_ = ColorRef::constant(Color(0x000000, 1.0));
    fg_epg_chan_ = ColorRef::constant(Color(0x3D3D3D, 1.0));
    bg_epg_tile_ = ColorRef::constant(Color(0xE1E1E1, 1.0));
    bg_epg_scrollbar_ = ColorRef::constant(Color(0xF9F9F9, 1.0));
    fg_epg_scrollbar_ = ColorRef::constant(Color(0xC0C0C0, 1.0));
    bg_epg_cancelled_ = ColorRef::constant(Color(0x000000, 1.0));
    bg_epg_rec_ = ColorRef::constant(Color(0xFF0000, 1.0));

    // FIXME: maybe use Qt to lookup system highlight color for selection,
    // and wrap it in an expression:
    bg_epg_sel_ = ColorRef::constant(Color(0x004080, 0.1));

    bg_epg_header_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_header_;
      gradient[0.00] = Color(0xFFFFFF, 1.00);
      gradient[0.05] = Color(0xF0F0F0, 1.00);
      gradient[0.50] = Color(0xE1E1E1, 1.00);
      gradient[0.55] = Color(0xD0D0D0, 1.00);
      gradient[0.95] = Color(0xB9B9B9, 1.00);
      gradient[1.00] = Color(0x8E8E8E, 1.00);
    }

    bg_epg_shadow_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_shadow_;
      gradient[0.00] = Color(0x000000, 0.50);
      gradient[0.33] = Color(0x000000, 0.20);
      gradient[1.00] = Color(0x000000, 0.00);
    }

    bg_epg_channel_.reset(new TGradient());
    {
      TGradient & gradient = *bg_epg_channel_;
      gradient[0.00] = Color(0xE9E9ED, 1.00);
      gradient[1.00] = Color(0xDCDBDF, 1.00);
    }
  }


  //----------------------------------------------------------------
  // AppView::AppView
  //
  AppView::AppView():
    ItemView("AppView"),
    dvr_(NULL),
    view_mode_(kProgramGuideMode),
    sync_ui_(this)
  {
    Item & root = *root_;

    // add style to the root item, so it could be uncached automatically:
    style_.reset(new AppStyle("AppStyle", *this));

    bool ok = connect(&sync_ui_, SIGNAL(timeout()), this, SLOT(sync_ui()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // AppView::setContext
  //
  void
  AppView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    ItemView::setContext(context);

    player_.reset(new PlayerItem("player", context));
    PlayerItem & player = *player_;
    player.visible_ = player.addExpr(new In<AppView::kPlayerMode>(*this));
    player.visible_.disableCaching();

    YAE_ASSERT(delegate_);
    player.setCanvasDelegate(delegate_);

    timeline_.reset(new TimelineItem("player_timeline",
                                     *this,
                                     player.timeline()));

    TimelineItem & timeline = *timeline_;
    timeline.is_playback_paused_ = timeline.addExpr
      (new IsPlaybackPaused(*this));

    timeline.is_fullscreen_ = timeline.addExpr
      (new IsFullscreen(*this));

    timeline.is_playlist_visible_ = BoolRef::constant(false);
    timeline.is_timeline_visible_ = BoolRef::constant(false);
    timeline.toggle_playback_.reset(&yae::toggle_playback, this);
    timeline.toggle_fullscreen_ = this->toggle_fullscreen_;
    timeline.layout();

    sync_ui_.start(1000);
  }

  //----------------------------------------------------------------
  // AppView::setModel
  //
  void
  AppView::setModel(yae::DVR * dvr)
  {
    if (dvr_ == dvr)
    {
      return;
    }

    // FIXME: disconnect previous model:
    YAE_ASSERT(!dvr_);

    dvr_ = dvr;
  }

  // bool AppView::resizeTo(const Canvas * canvas);

  //----------------------------------------------------------------
  // AppView::processKeyEvent
  //
  bool
  AppView::processKeyEvent(Canvas * canvas, QKeyEvent * e)
  {
#ifndef NDEBUG
    // FIXME: pkoshevoy: for debvugging only:
    if (root_)
    {
      root_->dump(std::cerr);
    }
#endif
    return ItemView::processKeyEvent(canvas, e);
  }

  //----------------------------------------------------------------
  // AppView::processMouseTracking
  //
  bool
  AppView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    Item & root = *root_;
    if (view_mode_ == kPlayerMode)
    {
      Item & player = root["player"];
      TimelineItem & timeline = player.get<TimelineItem>("timeline_item");
      timeline.processMouseTracking(mousePt);
    }

    return true;
  }

  //----------------------------------------------------------------
  // find_item_under_mouse
  //
  template <typename TItem>
  static TItem *
  find_item_under_mouse(const std::list<VisibleItem> & mouseOverItems)
  {
    for (std::list<VisibleItem>::const_iterator
           i = mouseOverItems.begin(); i != mouseOverItems.end(); ++i)
    {
      Item * item = i->item_.lock().get();
      TItem * found = dynamic_cast<TItem *>(item);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // AppView::processMouseEvent
  //
  bool
  AppView::processMouseEvent(Canvas * canvas, QMouseEvent * event)
  {
    bool r = ItemView::processMouseEvent(canvas, event);

    QEvent::Type et = event->type();
    if (et == QEvent::MouseButtonPress)
    {
#if 0
      ProgramItem * prog = find_item_under_mouse<ProgramItem>(mouseOverItems_);
      if (prog)
      {
        cursor->set_program(prog->index());
        requestUncache(cursor);
        requestRepaint();
      }
#endif
    }

    return r;
  }

  //----------------------------------------------------------------
  // AppView::layoutChanged
  //
  void
  AppView::layoutChanged()
  {
#if 0 // ndef NDEBUG
    std::cerr << "AppView::layoutChanged" << std::endl;
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
    root.addHidden(style_);
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);
    root.uncache();
    uncache_.clear();

    layout(*this, *style_, *root_);

#ifndef NDEBUG
    root.dump(std::cerr);
#endif
  }

  //----------------------------------------------------------------
  // AppView::dataChanged
  //
  void
  AppView::dataChanged()
  {
    requestUncache();
    requestRepaint();
  }

  //----------------------------------------------------------------
  // AppView::is_playback_paused
  //
  bool
  AppView::is_playback_paused()
  {
    return player_->paused();
  }

  //----------------------------------------------------------------
  // AppView::toggle_playback
  //
  void
  AppView::toggle_playback()
  {
    player_->toggle_playback();

    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();

    if (!is_playback_paused())
    {
      timeline_->forceAnimateControls();
    }
    else
    {
      timeline_->maybeAnimateControls();
    }
  }

  //----------------------------------------------------------------
  // AppView::sync_ui
  //
  void
  AppView::sync_ui()
  {
    if (!dvr_)
    {
      return;
    }

    // fill in the channel list:
    yae::mpeg_ts::EPG epg;
    dvr_->get_epg(epg);

    DVR::Blacklist blacklist;
    dvr_->get(blacklist);

    std::map<uint32_t, TScheduledRecordings> schedule;
    dvr_->schedule_.get(schedule);

    if (epg.channels_ == epg_.channels_ &&
        blacklist.channels_ == blacklist_.channels_ &&
        schedule == schedule_)
    {
      requestRepaint();
      return;
    }

    // update:
    epg_.channels_.swap(epg.channels_);
    blacklist_.channels_.swap(blacklist.channels_);
    schedule_.swap(schedule);

    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & root = *root_;
    Item & hidden = root.get<Item>("hidden");
    Item & panel = *epg_view_;
    Gradient & epg_header = panel.get<Gradient>("epg_header");
    Scrollview & vsv = panel.get<Scrollview>("vsv");
    Item & vsv_content = *(vsv.content_);
    Item & ch_list = vsv_content.get<Item>("ch_list");
    Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
    Item & hsv_content = *(hsv.content_);
    Scrollview & timeline = epg_header.Item::get<Scrollview>("timeline");
    Item & tc = *(timeline.content_);

    // update the layout:
    uint32_t gps_t0 = std::numeric_limits<uint32_t>::max();
    uint32_t gps_t1 = std::numeric_limits<uint32_t>::min();
    std::size_t num_channels = 0;
    ch_ordinal_.clear();

    std::map<uint32_t, yae::shared_ptr<Gradient, Item> > ch_tiles;
    std::map<uint32_t, yae::shared_ptr<Item> > ch_rows;
    std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<Item> > > ch_progs;
    std::map<uint32_t, yae::shared_ptr<Item> > tickmarks;
    std::map<uint32_t, yae::shared_ptr<Rectangle, Item> > rec_highlights;

    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg_.channels_.begin(); i != epg_.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;

      if (yae::has(blacklist_.channels_, ch_num))
      {
        continue;
      }

      // calculate [t0, t1) GPS time bounding box over all channels/programs:
      if (!channel.programs_.empty())
      {
        const yae::mpeg_ts::EPG::Program & p0 = channel.programs_.front();
        const yae::mpeg_ts::EPG::Program & p1 = channel.programs_.back();
        gps_t0 = std::min(gps_t0, p0.gps_time_);
        gps_t1 = std::max(gps_t1, p1.gps_time_ + p1.duration_);
      }

      ch_ordinal_[ch_num] = num_channels;
      num_channels++;

      yae::shared_ptr<Gradient, Item> & tile_ptr = ch_tile_[ch_num];
      if (!tile_ptr)
      {
        std::string ch_str = strfmt("%i-%i", channel.major_, channel.minor_);
        tile_ptr.reset(new Gradient(ch_str.c_str()));

        Gradient & tile = ch_list.add<Gradient>(tile_ptr);
        tile.orientation_ = Gradient::kVertical;
        tile.color_ = style.bg_epg_channel_;
        tile.height_ = ItemRef::reference(hidden, kUnitSize, 1.12);
        tile.anchors_.left_ = ItemRef::reference(ch_list, kPropertyLeft);
        tile.anchors_.right_ = ItemRef::reference(ch_list, kPropertyRight);
        tile.anchors_.top_ = tile.
          addExpr(new ChannelRowTop(view, ch_list, ch_num));

        Text & maj_min = tile.addNew<Text>("maj_min");
        maj_min.font_ = style.font_;
        maj_min.font_.setWeight(QFont::Medium);
        maj_min.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.36);
        maj_min.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
        maj_min.anchors_.left_ = ItemRef::reference(tile, kPropertyLeft);
        maj_min.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));
        maj_min.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        maj_min.text_ = TVarRef::constant(TVar(ch_str.c_str()));
        maj_min.elide_ = Qt::ElideNone;
        maj_min.color_ = maj_min.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_chan_));
        maj_min.background_ = maj_min.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

        Text & ch_name = tile.addNew<Text>("ch_name");
        ch_name.font_ = style.font_;
        ch_name.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.28);
        ch_name.anchors_.top_ = ItemRef::reference(maj_min, kPropertyBottom);
        ch_name.anchors_.left_ = ItemRef::reference(maj_min, kPropertyLeft);
        ch_name.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.13));
        ch_name.text_ = TVarRef::constant(TVar(channel.name_.c_str()));
        ch_name.elide_ = Qt::ElideNone;
        ch_name.color_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_chan_));
        ch_name.background_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      }

      ch_tiles[ch_num] = tile_ptr;

      // shortcut:
      Gradient & tile = *tile_ptr;

      yae::shared_ptr<Item> & row_ptr = ch_row_[ch_num];
      if (!row_ptr)
      {
        row_ptr.reset(new Item(("row" + tile.id_).c_str()));
        Item & row = hsv_content.add<Item>(row_ptr);

        // extend EPG to nearest whole hour, both ends:
        row.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
        row.anchors_.left_ = row.addExpr(new ProgramRowPos(view, ch_num));
        row.width_ = row.addExpr(new ProgramRowLen(view, ch_num));
        row.height_ = ItemRef::reference(tile, kPropertyHeight);
      }

      ch_rows[ch_num] = row_ptr;

      // shortcut:
      Item & row = *row_ptr;
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v0 = ch_prog_[ch_num];
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v1 = ch_progs[ch_num];

      for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
             j = channel.programs_.begin(); j != channel.programs_.end(); ++j)
      {
        const yae::mpeg_ts::EPG::Program & program = *j;
        yae::shared_ptr<Item> & prog_ptr = progs_v0[program.gps_time_];
        if (!prog_ptr)
        {
          std::string prog_ts = yae::to_yyyymmdd_hhmmss(program.tm_);
          prog_ptr.reset(new Item(prog_ts.c_str()));

          Item & prog = row.add<Item>(prog_ptr);
          prog.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
          prog.height_ = ItemRef::reference(tile, kPropertyHeight);
          prog.anchors_.left_ = prog.
            addExpr(new ProgramTilePos(view, program.gps_time_));
          prog.width_ = prog.
            addExpr(new ProgramTileWidth(view, program.duration_));

          RoundRect & bg = prog.addNew<RoundRect>("bg");
          bg.anchors_.fill(prog);
          bg.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.03));
          bg.radius_ = ItemRef::reference(hidden, kUnitSize, 0.13);
          bg.background_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_));
          bg.color_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

          Item & body = bg.addNew<Item>("body");
          body.anchors_.fill(bg);
          body.margins_.set(ItemRef::reference(hidden, kUnitSize, 0.13));

          Text & hhmm = body.addNew<Text>("hhmm");
          hhmm.font_ = style.font_;
          hhmm.font_.setWeight(62);
          hhmm.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
          hhmm.anchors_.top_ = ItemRef::reference(body, kPropertyTop);
          hhmm.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
          hhmm.elide_ = Qt::ElideNone;
          hhmm.color_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          hhmm.background_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

          int hour =
            (program.tm_.tm_hour > 12) ? program.tm_.tm_hour % 12 :
            (program.tm_.tm_hour > 00) ? program.tm_.tm_hour : 12;

          std::string hhmm_txt = strfmt("%i:%02i", hour, program.tm_.tm_min);
          hhmm.text_ = TVarRef::constant(TVar(hhmm_txt.c_str()));

          hhmm.visible_ = hhmm.addExpr
            (new DoesItemFit(ItemRef::reference(body, kPropertyLeft),
                             ItemRef::reference(body, kPropertyRight),
                             hhmm));

          RoundRect & rec = body.addNew<RoundRect>("rec");
          rec.width_ = rec.addExpr(new OddRoundUp(bg, kPropertyHeight, 0.25));
          rec.height_ = ItemRef::reference(rec.width_);
          rec.radius_ = ItemRef::reference(rec.width_, 0.5);
          rec.anchors_.bottom_ = ItemRef::reference(body, kPropertyBottom);
          rec.anchors_.left_ = ItemRef::reference(body, kPropertyLeft);
          rec.background_ = rec.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          rec.color_ = rec.
            addExpr(new RecButtonColor(view, ch_num, program.gps_time_));
          rec.visible_ = rec.addExpr
            (new DoesItemFit(ItemRef::reference(body, kPropertyLeft),
                             ItemRef::reference(body, kPropertyRight),
                             rec));

          Text & title = body.addNew<Text>("title");
          title.font_ = style.font_;
          title.font_.setWeight(62);
          title.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.312);
          title.anchors_.vcenter_ = ItemRef::reference(rec, kPropertyVCenter);
          title.anchors_.left_ = ItemRef::reference(rec, kPropertyRight);
          title.anchors_.right_ = ItemRef::reference(body, kPropertyRight);
          title.margins_.
            set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
          title.elide_ = Qt::ElideRight;
          title.color_ = title.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          title.background_ = title.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          // FIXME: this should be an expression:
          title.text_ = TVarRef::constant(TVar(program.title_.c_str()));

           ToggleRecording & toggle = body.add<ToggleRecording>
             (new ToggleRecording("toggle", view, ch_num, program.gps_time_));
           toggle.anchors_.fill(rec);
        }

        progs_v1[program.gps_time_] = prog_ptr;
      }
    }

    // update tickmarks:
    uint32_t i0 = gps_time_round_dn(gps_t0);
    uint32_t i1 = gps_time_round_up(gps_t1);
    for (uint32_t gps_time = i0; gps_time < i1; gps_time += 3600)
    {
      ItemPtr & item_ptr = tickmark_[gps_time];
      if (!item_ptr)
      {
        int64_t ts = unix_epoch_gps_offset + gps_time;
        item_ptr.reset(new Item(unix_epoch_time_to_localtime_str(ts).c_str()));

        Item & item = tc.add(item_ptr);
        item.anchors_.top_ = ItemRef::reference(tc, kPropertyTop);
        item.height_ = ItemRef::reference(tc, kPropertyHeight);
        item.anchors_.left_ = item.
          addExpr(new ProgramTilePos(view, gps_time));
        item.width_ = item.
          addExpr(new ProgramTileWidth(view, 3600));

        Rectangle & tickmark = item.addNew<Rectangle>("tickmark");
        tickmark.anchors_.left_ = ItemRef::reference(item, kPropertyLeft);
        tickmark.anchors_.bottom_ = ItemRef::reference(item, kPropertyBottom);
        tickmark.height_ = ItemRef::reference(item, kPropertyHeight);
        tickmark.width_ = ItemRef::constant(1.0);
        tickmark.color_ = tickmark.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.1));

        struct tm t;
        unix_epoch_time_to_localtime(ts, t);

        int hour =
          (t.tm_hour > 12) ? t.tm_hour % 12 :
          (t.tm_hour > 00) ? t.tm_hour : 12;

        const char * am_pm = (t.tm_hour < 12) ? "AM" : "PM";
        std::string t_str = strfmt("%i:00 %s", hour, am_pm);

        Text & label = item.addNew<Text>("time");
        label.font_ = style.font_;
        label.fontSize_ = ItemRef::reference(hidden, kUnitSize, 0.29);
        label.anchors_.vcenter_ = ItemRef::reference(item, kPropertyVCenter);
        label.anchors_.left_ = ItemRef::reference(tickmark, kPropertyRight);
        label.margins_.set_top(ItemRef::reference(hidden, kUnitSize, 0.03));
        label.margins_.set_left(ItemRef::reference(hidden, kUnitSize, 0.13));
        label.elide_ = Qt::ElideNone;
        label.color_ = label.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.5));
        label.background_ = label.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
        label.text_ = TVarRef::constant(TVar(t_str.c_str()));
      }

      tickmarks[gps_time] = item_ptr;
    }

    // highlight timespan of the recordings on the timeline:
    std::map<uint32_t, uint32_t> rec_times;
    for (std::map<uint32_t, TScheduledRecordings>::const_iterator
           i = schedule_.begin(); i != schedule_.end(); ++i)
    {
      const TScheduledRecordings & recordings = i->second;
      for (TScheduledRecordings::const_iterator
             j = recordings.begin(); j != recordings.end(); ++j)
      {
        const Recording & rec = *(j->second);
        uint32_t gps_t1 = yae::get(rec_times, rec.gps_t0_, rec.gps_t1_);
        rec_times[rec.gps_t0_] = std::max(rec.gps_t1_, gps_t1);
      }
    }

    // combine overlapping timespans:
    while (rec_times.size() > 1)
    {
      std::map<uint32_t, uint32_t>::iterator i = rec_times.begin();
      bool combined = false;

      std::map<uint32_t, uint32_t>::iterator prev = i;
      ++i;

      while (i != rec_times.end())
      {
        uint32_t & prev_t1 = prev->second;
        uint32_t t0 = i->first;
        uint32_t t1 = i->second;

        if (t0 <= prev_t1)
        {
          prev_t1 = std::max(prev_t1, t1);
          rec_times.erase(i);
          combined = true;
          break;
        }

        prev = i;
        ++i;
      }

      if (!combined)
      {
        break;
      }
    }

    for (std::map<uint32_t, uint32_t>::const_iterator
           i = rec_times.begin(); i != rec_times.end(); ++i)
    {
      uint32_t gps_t0 = i->first;

      yae::shared_ptr<Rectangle, Item> & item_ptr = rec_highlight_[gps_t0];
      if (!item_ptr)
      {
        uint32_t gps_t1 = i->second;
        int64_t ts = unix_epoch_gps_offset + gps_t0;
        std::string ts_str = unix_epoch_time_to_localtime_str(ts);
        item_ptr.reset(new Rectangle(("rec " + ts_str).c_str()));

        Rectangle & highlight = tc.add<Rectangle>(item_ptr);
        highlight.anchors_.top_ = ItemRef::reference(tc, kPropertyTop);
        highlight.height_ = ItemRef::reference(tc, kPropertyHeight);
        highlight.anchors_.left_ = highlight.
          addExpr(new ProgramTilePos(view, gps_t0));
        highlight.width_ = highlight.
          addExpr(new ProgramTileWidth(view, gps_t1 - gps_t0));
        highlight.color_ = highlight.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_rec_, 0.1));
      }

      rec_highlights[gps_t0] = item_ptr;
    }

    // old channel tiles also must be removed from ch_list:
    for (std::map<uint32_t, yae::shared_ptr<Gradient, Item> >::const_iterator
           i = ch_tile_.begin(); i != ch_tile_.end(); ++i)
    {
      uint32_t ch_num = i->first;
      if (!yae::has(ch_tiles, ch_num))
      {
        yae::shared_ptr<Gradient, Item> tile_ptr = i->second;
        YAE_ASSERT(ch_list.remove(tile_ptr));
      }
    }

    // old channel rows also must be removed from hsv_content:
    for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
           i = ch_row_.begin(); i != ch_row_.end(); ++i)
    {
      uint32_t ch_num = i->first;
      yae::shared_ptr<Item> row_ptr = i->second;

      if (!yae::has(ch_rows, ch_num))
      {
        YAE_ASSERT(hsv_content.remove(row_ptr));
        continue;
      }

      Item & row = *row_ptr;
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v0 = ch_prog_[ch_num];
      std::map<uint32_t, yae::shared_ptr<Item> > & progs_v1 = ch_progs[ch_num];

      for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
             j = progs_v0.begin(); j != progs_v0.end(); ++j)
      {
        uint32_t gps_time = j->first;
        if (!yae::has(progs_v1, gps_time))
        {
          yae::shared_ptr<Item> prog = j->second;
          YAE_ASSERT(row.remove(prog));
        }
      }
    }

    // old tickmarks also must be removed from tc:
    for (std::map<uint32_t, yae::shared_ptr<Item> >::const_iterator
           i = tickmark_.begin(); i != tickmark_.end(); ++i)
    {
      uint32_t gps_time = i->first;
      if (!yae::has(tickmarks, gps_time))
      {
        yae::shared_ptr<Item> item_ptr = i->second;
        YAE_ASSERT(tc.remove(item_ptr));
      }
    }

    // old highlights also must be removed from tc:
    for (std::map<uint32_t, yae::shared_ptr<Rectangle, Item> >::const_iterator
           i = rec_highlight_.begin(); i != rec_highlight_.end(); ++i)
    {
      uint32_t gps_time = i->first;
      if (!yae::has(rec_highlights, gps_time))
      {
        yae::shared_ptr<Item> item_ptr = i->second;
        YAE_ASSERT(tc.remove(item_ptr));
      }
    }

    // prune old items:
    ch_tile_.swap(ch_tiles);
    ch_row_.swap(ch_rows);
    ch_prog_.swap(ch_progs);
    tickmark_.swap(tickmarks);
    rec_highlight_.swap(rec_highlights);

    hsv_content.uncache();
    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::toggle_recording
  //
  void
  AppView::toggle_recording(uint32_t ch_num, uint32_t gps_time)
  {
    yae::shared_ptr<Wishlist::Item> explicitly_scheduled;
    const yae::mpeg_ts::EPG::Channel * channel = NULL;
    const yae::mpeg_ts::EPG::Program * program = NULL;

    if (epg_.find(ch_num, gps_time, channel, program))
    {
      explicitly_scheduled = dvr_->explicitly_scheduled(*channel, *program);
    }
    else
    {
      yae_elog("toggle recording: not found in EPG");
    }

    if (explicitly_scheduled)
    {
      yae_ilog("cancel recording: %02i.%02i %02i:%02i %s",
               channel->major_,
               channel->minor_,
               program->tm_.tm_hour,
               program->tm_.tm_min,
               program->title_.c_str());
      dvr_->cancel_recording(*channel, *program);
      sync_ui();
      return;
    }

    std::map<uint32_t, TScheduledRecordings>::const_iterator
      found_sched = schedule_.find(ch_num);
    if (found_sched != schedule_.end())
    {
      const TScheduledRecordings & schedule = found_sched->second;
      TScheduledRecordings::const_iterator found_rec = schedule.find(gps_time);
      if (found_rec != schedule.end())
      {
        yae_ilog("toggle recording: %02i.%02i %02i:%02i %s",
                 channel->major_,
                 channel->minor_,
                 program->tm_.tm_hour,
                 program->tm_.tm_min,
                 program->title_.c_str());
        TRecordingPtr rec_ptr = found_rec->second;
        Recording & rec = *rec_ptr;
        rec.cancelled_ = !rec.cancelled_;
        sync_ui();
        return;
      }
    }

    if (channel && program)
    {
      yae_ilog("schedule recording: %02i.%02i %02i:%02i %s",
               channel->major_,
               channel->minor_,
               program->tm_.tm_hour,
               program->tm_.tm_min,
               program->title_.c_str());
      dvr_->schedule_recording(*channel, *program);
      sync_ui();
    }
  }

  //----------------------------------------------------------------
  // AppView::layout
  //
  void
  AppView::layout(AppView & view, AppStyle & style, Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
    hidden.width_ = hidden.
      addExpr(style_item_ref(view, &AppStyle::unit_size_));

    Rectangle & bg = root.addNew<Rectangle>("background");
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_epg_));
    bg.anchors_.fill(root);

    Item & overview = root.addNew<Item>("overview");
    overview.anchors_.fill(bg);
    overview.visible_ = overview.
      addInverse(new In<AppView::kPlayerMode>(view));
    overview.visible_.disableCaching();

    PlayerItem & player = root.add(view.player_);
    player.anchors_.fill(bg);

    TimelineItem & timeline = player.add(view.timeline_);
    timeline.anchors_.fill(player);

    Item & sideview = overview.addNew<Item>("sideview");
    Item & mainview = overview.addNew<Item>("mainview");

    Rectangle & sep = overview.addNew<Rectangle>("separator");
    sep.color_ = sep.addExpr(style_color_ref(view, &AppStyle::fg_epg_));

    Splitter & splitter = overview.
      add(new Splitter("splitter",
                       // lower bound:
                       hidden.addExpr
                       (new SplitterPos(view, overview, SplitterPos::kLeft)),
                       // upper bound:
                       hidden.addExpr
                       (new SplitterPos(view, overview, SplitterPos::kRight)),
                       // orientation:
                       Splitter::kHorizontal));
    splitter.anchors_.inset(sep, -5.0, 0.0);

    sep.anchors_.top_ = ItemRef::reference(overview, kPropertyTop);
    sep.anchors_.bottom_ = ItemRef::reference(overview, kPropertyBottom);
    sep.anchors_.left_ = ItemRef::reference(splitter.pos_);
    sep.width_ = ItemRef::constant(1.0);

    sideview.anchors_.fill(overview);
    sideview.anchors_.right_ = ItemRef::reference(sep, kPropertyLeft);
    layout_sidebar(view, style, sideview);

    mainview.anchors_.fill(overview);
    mainview.anchors_.left_ = ItemRef::reference(sep, kPropertyRight);

    // layout Program Guide panel:
    epg_view_.reset(new Item("epg_view"));
    mainview.add<Item>(epg_view_);
    Item & epg_view = *epg_view_;
    epg_view.anchors_.fill(mainview);
    layout_epg(view, style, epg_view);
  }

  //----------------------------------------------------------------
  // AppView::layout_sidebar
  //
  void
  AppView::layout_sidebar(AppView & view, AppStyle & style, Item & panel)
  {
    Rectangle & bg = panel.addNew<Rectangle>("bg_sidebar");
    bg.color_ = bg.addExpr(style_color_ref(view, &AppStyle::bg_sidebar_));
    bg.anchors_.fill(panel);

    Scrollview & sv = layout_scrollview(kScrollbarVertical, view, style, panel,
                                        kScrollbarVertical);
    Item & content = *(sv.content_);
    bg.anchors_.right_ = ItemRef::reference(sv, kPropertyRight);
  }

  //----------------------------------------------------------------
  // AppView::layout_epg
  //
  void
  AppView::layout_epg(AppView & view, AppStyle & style, Item & panel)
  {
    Item & root = *(view.root_);
    Item & hidden = root.get<Item>("hidden");

    Gradient & ch_header = panel.addNew<Gradient>("ch_header");
    ch_header.anchors_.fill(panel);
    ch_header.anchors_.right_.reset();
    ch_header.width_ = ItemRef::reference(hidden, kUnitSize, 2.6);
    ch_header.anchors_.bottom_.reset();
    ch_header.height_ = ItemRef::reference(hidden, kUnitSize, 0.42);
    ch_header.orientation_ = Gradient::kVertical;
    ch_header.color_ = style.bg_epg_header_;

    Gradient & epg_header = panel.addNew<Gradient>("epg_header");
    epg_header.anchors_.fill(panel);
    epg_header.anchors_.left_ =
      ItemRef::reference(ch_header, kPropertyRight);
    epg_header.anchors_.bottom_ =
      ItemRef::reference(ch_header, kPropertyBottom);
    epg_header.orientation_ = Gradient::kVertical;
    epg_header.color_ = style.bg_epg_header_;

    FlickableArea & flickable =
      panel.add(new FlickableArea("flickable", view));

    Scrollview & vsv = panel.addNew<Scrollview>("vsv");
    vsv.clipContent_ = true;
    vsv.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    vsv.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);

    Rectangle & now_marker = panel.addNew<Rectangle>("now_marker");
    Item & chan_bar = panel.addNew<Item>("chan_bar");
    chan_bar.anchors_.fill(panel);
    chan_bar.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    chan_bar.anchors_.right_.reset();
    chan_bar.width_ = ItemRef::reference(ch_header, kPropertyWidth);

    Gradient & chan_bar_shadow = panel.addNew<Gradient>("chan_bar_shadow");
    chan_bar_shadow.anchors_.top_ =
      ItemRef::reference(chan_bar, kPropertyTop);
    chan_bar_shadow.anchors_.bottom_ =
      ItemRef::reference(chan_bar, kPropertyBottom);
    chan_bar_shadow.anchors_.left_ =
      ItemRef::reference(chan_bar, kPropertyRight);
    chan_bar_shadow.anchors_.right_.reset();
    chan_bar_shadow.width_ =
      ItemRef::reference(chan_bar, kPropertyWidth, 0.03, 1);
    chan_bar_shadow.orientation_ = Gradient::kHorizontal;
    chan_bar_shadow.color_ = style.bg_epg_shadow_;

    Item & vsv_content = *(vsv.content_);
    Scrollview & hsv = vsv_content.addNew<Scrollview>("hsv");
    Item & ch_list = vsv_content.addNew<Item>("ch_list");
    ch_list.anchors_.top_ = ItemRef::constant(0.0);
    ch_list.anchors_.left_ = ItemRef::constant(0.0);
    ch_list.width_ = ItemRef::reference(chan_bar, kPropertyWidth);

    hsv.uncacheContent_ = false;
    hsv.anchors_.top_ = ItemRef::reference(ch_list, kPropertyTop);
    hsv.anchors_.left_ = ItemRef::reference(ch_list, kPropertyRight);
    hsv.height_ = ItemRef::reference(ch_list, kPropertyHeight);

    Item & hsv_content = *(hsv.content_);
    Rectangle & hscrollbar = panel.addNew<Rectangle>("hscrollbar");
    Rectangle & vscrollbar = panel.addNew<Rectangle>("vscrollbar");

    hscrollbar.setAttr("vertical", false);
    hscrollbar.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);
    hscrollbar.anchors_.right_ = ItemRef::reference(vscrollbar, kPropertyLeft);
    hscrollbar.anchors_.bottom_ = ItemRef::reference(panel, kPropertyBottom);
    hscrollbar.height_ = ItemRef::reference(hidden, kUnitSize, 0.33);
    hscrollbar.color_ = hscrollbar.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_));

    vscrollbar.anchors_.top_ = ItemRef::reference(ch_header, kPropertyBottom);
    vscrollbar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    vscrollbar.anchors_.right_ = ItemRef::reference(panel, kPropertyRight);
    vscrollbar.width_ = ItemRef::reference(hidden, kUnitSize, 0.33);
    vscrollbar.color_ = hscrollbar.color_;

    chan_bar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    vsv.anchors_.right_ = ItemRef::reference(vscrollbar, kPropertyLeft);
    vsv.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    hsv.width_ = hsv.addExpr(new CalcDistanceLeftRight(chan_bar, vscrollbar));

    // configure vscrollbar slider:
    InputArea & vscrollbar_ia = vscrollbar.addNew<InputArea>("ia");
    vscrollbar_ia.anchors_.fill(vscrollbar);

    RoundRect & vslider = vscrollbar.addNew<RoundRect>("vslider");
    vslider.anchors_.top_ = vslider.
      addExpr(new CalcSliderTop(vsv, vscrollbar, vslider));
    vslider.anchors_.left_ =
      ItemRef::offset(vscrollbar, kPropertyLeft, 2.5);
    vslider.anchors_.right_ =
      ItemRef::offset(vscrollbar, kPropertyRight, -2.5);
    vslider.height_ = vslider.
      addExpr(new CalcSliderHeight(vsv, vscrollbar, vslider));
    vslider.radius_ =
      ItemRef::scale(vslider, kPropertyWidth, 0.5);
    vslider.color_ = vslider.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_));
    vslider.background_ = hscrollbar.color_;

    SliderDrag & vslider_ia =
      vslider.add(new SliderDrag("ia", view, vsv, vscrollbar));
    vslider_ia.anchors_.fill(vslider);

    // configure horizontal scrollbar slider:
    InputArea & hscrollbar_ia = hscrollbar.addNew<InputArea>("ia");
    hscrollbar_ia.anchors_.fill(hscrollbar);

    RoundRect & hslider = hscrollbar.addNew<RoundRect>("hslider");
    hslider.anchors_.top_ =
      ItemRef::offset(hscrollbar, kPropertyTop, 2.5);
    hslider.anchors_.bottom_ =
      ItemRef::offset(hscrollbar, kPropertyBottom, -2.5);
    hslider.anchors_.left_ = hslider.
      addExpr(new CalcSliderLeft(hsv, hscrollbar, hslider));
    hslider.width_ = hslider.
      addExpr(new CalcSliderWidth(hsv, hscrollbar, hslider));
    hslider.radius_ =
      ItemRef::scale(hslider, kPropertyHeight, 0.5);
    hslider.background_ = hscrollbar.color_;
    hslider.color_ = vslider.color_;

    SliderDrag & hslider_ia =
      hslider.add(new SliderDrag("ia", view, hsv, hscrollbar));
    hslider_ia.anchors_.fill(hslider);

    // enable flicking the scrollviews:
    flickable.setVerSlider(&vslider_ia);
    flickable.setHorSlider(&hslider_ia);
    flickable.anchors_.fill(vsv);

    // setup tickmarks scrollview:
    Scrollview & timeline = epg_header.addNew<Scrollview>("timeline");
    {
      timeline.clipContent_ = true;
      timeline.anchors_.fill(epg_header);
      timeline.position_y_ = ItemRef::constant(0.0);
      timeline.position_x_ = timeline.
        addExpr(new CalcViewPositionX(hsv, timeline));
      timeline.position_x_.disableCaching();

      Item & t = *(timeline.content_);
      t.anchors_.top_ = ItemRef::constant(0.0);
      t.anchors_.left_ = ItemRef::reference(hsv_content, kPropertyLeft);
      t.anchors_.right_ = ItemRef::reference(hsv_content, kPropertyRight);
      t.height_ = ItemRef::reference(epg_header, kPropertyHeight);
    }

    // setup "now" time marker:
    now_marker.color_ = now_marker.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_, 0.2));
    now_marker.width_ = ItemRef::constant(1.0);
    now_marker.anchors_.top_ = ItemRef::reference(panel, kPropertyTop);
    now_marker.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    now_marker.anchors_.left_ = now_marker.
      addExpr(new NowMarkerPos(view, epg_header, hsv));
    now_marker.anchors_.left_.disableCaching();
    now_marker.xExtentDisableCaching();
  }

  //----------------------------------------------------------------
  // AppView::layout_channels
  //
  void
  AppView::layout_channels(AppView & view, AppStyle & style, Item & root)
  {}

  //----------------------------------------------------------------
  // AppView::layout_wishlist
  //
  void
  AppView::layout_wishlist(AppView & view, AppStyle & style, Item & root)
  {}

  //----------------------------------------------------------------
  // AppView::layout_schedule
  //
  void
  AppView::layout_schedule(AppView & view, AppStyle & style, Item & root)
  {}

  //----------------------------------------------------------------
  // AppView::layout_recordings
  //
  void
  AppView::layout_recordings(AppView & view, AppStyle & style, Item & root)
  {}

}
