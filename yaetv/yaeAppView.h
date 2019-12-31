// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Thu Dec 26 13:36:42 MST 2019
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_APP_VIEW_H_
#define YAE_APP_VIEW_H_


// standard:
#include <vector>

// Qt library:
#include <QAction>
#include <QFont>
#include <QSignalMapper>

// aeyae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/ffmpeg/yae_demuxer.h"
#include "yae/ffmpeg/yae_demuxer_reader.h"
#include "yae/thread/yae_task_runner.h"
#include "yae/utils/yae_lru_cache.h"
#include "yae/video/yae_video.h"

// local:
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeInputArea.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeScrollview.h"
#include "yaeTimelineItem.h"
#include "yae_dvr.h"
#include "yae_player_item.h"


namespace yae
{
  //----------------------------------------------------------------
  // AppStyle
  //
  struct AppStyle : public ItemViewStyle
  {
    AppStyle(const char * id, const ItemView & view);

    ColorRef bg_sidebar_;
    ColorRef bg_splitter_;
    ColorRef bg_epg_;
    ColorRef fg_epg_;
    ColorRef bg_epg_tile_;
    ColorRef fg_epg_line_;
    ColorRef bg_epg_scrollbar_;
    ColorRef fg_epg_scrollbar_;
    ColorRef bg_epg_rec_;
    ColorRef bg_epg_sel_;

    TGradientPtr bg_epg_header_;
    TGradientPtr bg_epg_shadow_;
    TGradientPtr bg_epg_channel_;
  };

  //----------------------------------------------------------------
  // AppView
  //
  class YAEUI_API AppView : public ItemView
  {
    Q_OBJECT;

  public:

    //----------------------------------------------------------------
    // ViewMode
    //
    enum ViewMode
    {
      kChannelListMode = 0,
      kProgramGuideMode = 1,
      kWishlistMode = 2,
      kScheduleMode = 3,
      kRecordingsMode = 4,
      kPlayerMode = 5,
    };

    AppView();

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // data source:
    void setModel(yae::DVR * dvr);

    inline yae::DVR * model() const
    { return dvr_; }

    // virtual:
    const ItemViewStyle * style() const
    { return style_; }

    // virtual: returns false if size didn't change
    // bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // virtual:
    bool processMouseTracking(const TVec2D & mousePt);

    // virtual:
    bool processRightClick();

    // accessor:
    inline ViewMode view_mode() const
    { return view_mode_; }

  signals:
    void toggle_fullscreen();

  public slots:
    void layoutChanged();
    void dataChanged();

    bool is_playback_paused();
    void toggle_playback();

    void sync_ui();

  protected:
    // helpers:
    void layout(AppView & view, AppStyle & style, Item & root);
    void layout_sidebar(AppView & view, AppStyle & style, Item & root);
    void layout_epg(AppView & view, AppStyle & style, Item & root);
    void layout_channels(AppView & view, AppStyle & style, Item & root);
    void layout_wishlist(AppView & view, AppStyle & style, Item & root);
    void layout_schedule(AppView & view, AppStyle & style, Item & root);
    void layout_recordings(AppView & view, AppStyle & style, Item & root);

    // model:
    yae::DVR * dvr_;

    ViewMode view_mode_;

    QTimer sync_ui_;

  public:
    yae::shared_ptr<AppStyle, Item> style_;
    yae::shared_ptr<PlayerItem, Item> player_;
    yae::shared_ptr<TimelineItem, Item> timeline_;
    yae::shared_ptr<Item> epg_view_;

    yae::mpeg_ts::EPG epg_;
    DVR::Blacklist blacklist_;
    std::map<uint32_t, std::size_t> ch_ordinal_;
    std::map<uint32_t, yae::shared_ptr<Gradient, Item> > ch_tile_;
    std::map<uint32_t, yae::shared_ptr<Item> > ch_row_;
    std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<Item> > > ch_prog_;
    std::map<uint32_t, yae::shared_ptr<Item> > tickmark_;
  };

}


#endif // YAE_APP_VIEW_H_
