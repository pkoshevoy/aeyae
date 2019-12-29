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

    SplitterPos(ItemView & view, Item & container, Side side = kLeft):
      view_(view),
      container_(container),
      side_(side)
    {}

    // virtual:
    void evaluate(double & result) const
    {
      double row_height = get_row_height(view_);

      if (side_ == kLeft)
      {
        result = container_.left() + row_height * 9.0;
      }
      else
      {
        result = container_.right() - row_height * 9.0;
      }
    }

    ItemView & view_;
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

      // this avoids uncaching the scrollview content:
      parent_->uncacheSelfAndChildren();
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
    ChannelRowTop(const AppView & view, uint32_t ch_num):
      view_(view),
      ch_num_(ch_num)
    {}

    void evaluate(double & result) const
    {
      double row_height = view_.style_->row_height_.get();
      std::size_t ch_ordinal = yae::at(view_.ch_ordinal_, ch_num_);
      result = (row_height * 1.12 + 1.0) * double(ch_ordinal);
    }

    const AppView & view_;
    const uint32_t ch_num_;
  };


  //----------------------------------------------------------------
  // ProgramTilePos
  //
  struct ProgramTilePos : TDoubleExpr
  {
    ProgramTilePos(const AppView & view, uint32_t ch_num, uint32_t gps_time):
      view_(view),
      ch_num_(ch_num),
      gps_time_(gps_time)
    {}

    void evaluate(double & result) const
    {
      const yae::mpeg_ts::EPG::Channel & channel =
        yae::at(view_.epg_.channels_, ch_num_);

      uint32_t gps_now = channel.gps_time();
      gps_now -= gps_now % 3600;
      double x = double(gps_time_) - double(gps_now);

      double row_height = view_.style_->row_height_.get();
      result = (row_height * 8) * (x / 3600.0);
    }

    const AppView & view_;
    const uint32_t ch_num_;
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
      double row_height = view_.style_->row_height_.get();
      result = (row_height * 8) * double(duration_) / 3600.0;
    }

    const AppView & view_;
    const uint32_t duration_;
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
  // AppStyle::AppStyle
  //
  AppStyle::AppStyle(const char * id, const ItemView & view):
    ItemViewStyle(id, view)
  {
    bg_sidebar_ = ColorRef::constant(Color(0xE0E9F5, 1.0));
    bg_splitter_ = ColorRef::constant(Color(0x4F4F4f, 1.0));
    bg_epg_ = ColorRef::constant(Color(0xFFFFFF, 1.0));
    fg_epg_ = ColorRef::constant(Color(0x000000, 1.0));
    bg_epg_tile_ = ColorRef::constant(Color(0xE1E1E1, 1.0));
    fg_epg_line_ = ColorRef::constant(Color(0xB9B9B9, 1.0));
    bg_epg_scrollbar_ = ColorRef::constant(Color(0xF9F9F9, 1.0));
    fg_epg_scrollbar_ = ColorRef::constant(Color(0xC0C0C0, 1.0));
    bg_epg_rec_ = ColorRef::constant(Color(0xFF0000, 0.1));

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

    if (epg.channels_ == epg_.channels_)
    {
      return;
    }

    epg_ = epg;
    DVR::Blacklist blacklist;
    dvr_->get(blacklist);

    // shortcuts:
    AppView & view = *this;
    AppStyle & style = *style_;
    Item & panel = *epg_view_;
    Scrollview & vsv = panel.get<Scrollview>("vsv");
    Item & vsv_content = *(vsv.content_);
    Item & chan_list = vsv_content.get<Item>("chan_list");
    Scrollview & hsv = vsv_content.get<Scrollview>("hsv");
    Item & epg_grid = *(hsv.content_);


    // update the layout:
    std::size_t num_channels = 0;
    for (std::map<uint32_t, yae::mpeg_ts::EPG::Channel>::const_iterator
           i = epg.channels_.begin(); i != epg.channels_.end(); ++i)
    {
      const uint32_t ch_num = i->first;
      const yae::mpeg_ts::EPG::Channel & channel = i->second;

      if (yae::has(blacklist.channels_, ch_num))
      {
        continue;
      }

      ch_ordinal_[ch_num] = num_channels;
      num_channels++;

      yae::shared_ptr<Gradient, Item> & tile_ptr = ch_num_[ch_num];
      if (!tile_ptr)
      {
        std::string ch_str = strfmt("%i-%i", channel.major_, channel.minor_);
        tile_ptr.reset(new Gradient(ch_str.c_str()));

        Gradient & tile = chan_list.add<Gradient>(tile_ptr);
        tile.orientation_ = Gradient::kVertical;
        tile.color_ = style.bg_epg_channel_;
        tile.height_ = ItemRef::reference(style.row_height_, 1.12);
        tile.anchors_.left_ = ItemRef::reference(chan_list, kPropertyLeft);
        tile.anchors_.right_ = ItemRef::reference(chan_list, kPropertyRight);
        tile.anchors_.top_ = tile.addExpr(new ChannelRowTop(*this, ch_num));

        Text & maj_min = tile.addNew<Text>("maj_min");
        maj_min.font_ = style.font_;
        maj_min.font_.setBold(true);
        maj_min.anchors_.top_ = ItemRef::reference(tile, kPropertyTop, 1, 5);
        maj_min.anchors_.left_ = ItemRef::reference(tile, kPropertyLeft, 1, 5);
        maj_min.text_ = TVarRef::constant(TVar(ch_str.c_str()));
        maj_min.fontSize_ = ItemRef::reference(style.row_height_, 0.4);
        maj_min.elide_ = Qt::ElideNone;
        maj_min.color_ = maj_min.
        addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        maj_min.background_ = maj_min.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

        Text & ch_name = tile.addNew<Text>("ch_name");
        ch_name.font_ = style.font_;
        ch_name.anchors_.top_ =
          ItemRef::reference(maj_min, kPropertyBottom, 1, 2);
        ch_name.anchors_.left_ =
          ItemRef::reference(maj_min, kPropertyLeft);
        ch_name.text_ = TVarRef::constant(TVar(channel.name_.c_str()));
        ch_name.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
        ch_name.elide_ = Qt::ElideNone;
        ch_name.color_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::fg_epg_));
        ch_name.background_ = ch_name.
          addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
      }

      // shortcut:
      Gradient & tile = *tile_ptr;

      yae::shared_ptr<Item> & row_ptr = ch_row_[ch_num];
      if (!row_ptr)
      {
        row_ptr.reset(new Item(tile.id_.c_str()));
        Item & row = epg_grid.add<Item>(row_ptr);
        // row.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
        // row.height_ = ItemRef::reference(tile, kPropertyHeight);
      }

      // shortcut:
      Item & row = *row_ptr;
      std::map<uint32_t, yae::shared_ptr<Item> > & ch_progs = ch_prog_[ch_num];

      for (std::list<yae::mpeg_ts::EPG::Program>::const_iterator
             j = channel.programs_.begin(); j != channel.programs_.end(); ++j)
      {
        const yae::mpeg_ts::EPG::Program & program = *j;
        yae::shared_ptr<Item> & prog_ptr = ch_progs[program.gps_time_];
        if (!prog_ptr)
        {
          std::string prog_ts = yae::to_yyyymmdd_hhmmss(program.tm_);
          prog_ptr.reset(new Item(prog_ts.c_str()));

          Item & prog = row.add<Item>(prog_ptr);
          prog.anchors_.top_ = ItemRef::reference(tile, kPropertyTop);
          prog.height_ = ItemRef::reference(tile, kPropertyHeight);
          prog.anchors_.left_ = prog.
            addExpr(new ProgramTilePos(*this, ch_num, program.gps_time_));
          prog.width_ = prog.
            addExpr(new ProgramTileWidth(*this, program.duration_));

          RoundRect & bg = prog.addNew<RoundRect>("bg");
          bg.anchors_.inset(prog, 1, 1);
          bg.radius_ = ItemRef::constant(5.0);
          bg.background_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_));
          bg.color_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));

          Text & hhmm = prog.addNew<Text>("hhmm");
          hhmm.font_ = style.font_;
          hhmm.font_.setBold(true);
          hhmm.anchors_.top_ = ItemRef::reference(prog, kPropertyTop, 1, 7);
          hhmm.anchors_.left_ = ItemRef::reference(prog, kPropertyLeft, 1, 10);
          hhmm.fontSize_ = ItemRef::reference(style.row_height_, 0.33);
          hhmm.elide_ = Qt::ElideNone;
          hhmm.color_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          hhmm.background_ = hhmm.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          int hour = (program.tm_.tm_hour < 13 ?
                      program.tm_.tm_hour :
                      program.tm_.tm_hour % 12);

          std::string hhmm_txt = strfmt("%i:%02i", hour, program.tm_.tm_min);
          hhmm.text_ = TVarRef::constant(TVar(hhmm_txt.c_str()));

          RoundRect & rec = prog.addNew<RoundRect>("rec");
          rec.anchors_.top_ = ItemRef::reference(hhmm, kPropertyBottom, 1, 3);
          rec.anchors_.left_ = ItemRef::reference(hhmm, kPropertyLeft, 1, -1);
          rec.width_ = rec.
            addExpr(new OddRoundUp(prog, kPropertyHeight, 0.2));
          rec.height_ = ItemRef::reference(rec.width_);
          rec.radius_ = ItemRef::reference(rec.width_, 0.5);
          rec.background_ = bg.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          rec.color_ = bg.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_line_, 0.90));

          Text & title = prog.addNew<Text>("title");
          title.font_ = style.font_;
          title.font_.setBold(true);
          title.anchors_.bottom_ =
            ItemRef::reference(rec, kPropertyBottom);
          title.anchors_.left_ =
            ItemRef::reference(rec, kPropertyRight, 1, 7);
          title.anchors_.right_ =
            ItemRef::reference(prog, kPropertyRight, 1, -10);
          title.margins_.
            set_bottom(ItemRef::reference(title, kPropertyFontDescent, -1));
          title.fontSize_ = ItemRef::reference(style.row_height_, 0.3);
          title.elide_ = Qt::ElideRight;
          title.color_ = title.
            addExpr(style_color_ref(view, &AppStyle::fg_epg_));
          title.background_ = title.
            addExpr(style_color_ref(view, &AppStyle::bg_epg_tile_));
          // FIXME: this should be an expression:
          title.text_ = TVarRef::constant(TVar(program.title_.c_str()));
        }
      }
    }

    // update epg:
    epg_.channels_.swap(epg.channels_);

    dataChanged();
  }

  //----------------------------------------------------------------
  // AppView::layout
  //
  void
  AppView::layout(AppView & view, AppStyle & style, Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
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
    sv.clipContentTo(sv);

    Item & content = *(sv.content_);
    bg.anchors_.right_ = ItemRef::reference(sv, kPropertyRight);
  }

  //----------------------------------------------------------------
  // AppView::layout_epg
  //
  void
  AppView::layout_epg(AppView & view, AppStyle & style, Item & panel)
  {
    Gradient & header = panel.addNew<Gradient>("header");
    header.anchors_.fill(panel);
    header.anchors_.bottom_.reset();
    header.height_ = header.
      addExpr(style_item_ref(view, &AppStyle::row_height_, 0.42));
    header.orientation_ = Gradient::kVertical;
    header.color_ = style.bg_epg_header_;

    Scrollview & vsv = panel.addNew<Scrollview>("vsv");
    vsv.clipContentTo(vsv);
    vsv.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);
    vsv.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);

    Item & chan_bar = panel.addNew<Item>("chan_bar");
    chan_bar.anchors_.fill(panel);
    chan_bar.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);
    chan_bar.anchors_.right_.reset();
    chan_bar.width_ = chan_bar.
      addExpr(style_item_ref(view, &AppStyle::row_height_, 2.6));

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
    vsv_content.anchors_.top_ = ItemRef::constant(0.0);
    vsv_content.anchors_.left_ = ItemRef::constant(0.0);

    Item & chan_list = vsv_content.addNew<Item>("chan_list");
    chan_list.anchors_.top_ = ItemRef::constant(0.0);
    chan_list.anchors_.left_ = ItemRef::constant(0.0);
    chan_list.width_ = ItemRef::reference(chan_bar, kPropertyWidth);

    Scrollview & hsv = vsv_content.addNew<Scrollview>("hsv");
    Item & hsv_clip = hsv.clipContentTo(vsv);
    hsv_clip.anchors_.left_ = ItemRef::reference(chan_bar, kPropertyRight);
    hsv.anchors_.top_ = ItemRef::reference(chan_list, kPropertyTop);
    hsv.anchors_.left_ = ItemRef::reference(chan_list, kPropertyRight);
    hsv.height_ = ItemRef::reference(chan_list, kPropertyHeight);

    Item & epg_grid = *(hsv.content_);
    epg_grid.anchors_.top_ = ItemRef::constant(0.0);
    epg_grid.anchors_.left_ = ItemRef::constant(0.0);

    Item & hscrollbar = panel.addNew<Item>("hscrollbar");
    Item & vscrollbar = panel.addNew<Item>("vscrollbar");

    hscrollbar.setAttr("vertical", false);
    hscrollbar.anchors_.left_ = ItemRef::reference(panel, kPropertyLeft);
    hscrollbar.anchors_.right_ = ItemRef::reference(vscrollbar, kPropertyLeft);
    hscrollbar.anchors_.bottom_ = ItemRef::reference(panel, kPropertyBottom);
    hscrollbar.height_ = ItemRef::reference(style.row_height_, 0.33);

    vscrollbar.anchors_.top_ = ItemRef::reference(header, kPropertyBottom);
    vscrollbar.anchors_.bottom_ = ItemRef::reference(hscrollbar, kPropertyTop);
    vscrollbar.anchors_.right_ = ItemRef::reference(panel, kPropertyRight);
    vscrollbar.width_ = ItemRef::reference(style.row_height_, 0.33);

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
    vslider.background_ = vslider.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_));
    vslider.color_ = vslider.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_));

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
    hslider.background_ = hslider.
      addExpr(style_color_ref(view, &AppStyle::bg_epg_scrollbar_));
    hslider.color_ = hslider.
      addExpr(style_color_ref(view, &AppStyle::fg_epg_scrollbar_));

    SliderDrag & hslider_ia =
      hslider.add(new SliderDrag("ia", view, hsv, hscrollbar));
    hslider_ia.anchors_.fill(hslider);

    // enable flicking the scrollviews:
    FlickableArea & flickable =
      panel.add(new FlickableArea("flickable",
                                  view,
                                  &vslider_ia,
                                  &hslider_ia));
    flickable.anchors_.fill(vsv);
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
