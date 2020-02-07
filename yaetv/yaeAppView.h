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
#include "yaeAppStyle.h"
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeInputArea.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeRectangle.h"
#include "yaeScrollview.h"
#include "yae_dvr.h"


namespace yae
{

  //----------------------------------------------------------------
  // Layout
  //
  struct YAEUI_API Layout
  {
    yae::shared_ptr<Item> item_;

    // index -> id
    std::vector<std::string> names_;

    // id -> index
    std::map<std::string, std::size_t> index_;

    // id -> layout
    std::map<std::string, yae::shared_ptr<Layout> > items_;
  };

  //----------------------------------------------------------------
  // AppView
  //
  class AppView : public ItemView
  {
    Q_OBJECT;

  public:

    AppView();

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // data source:
    void setModel(yae::DVR * dvr);

    inline yae::DVR * model() const
    { return dvr_; }

    // virtual:
    AppStyle * style() const
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
    void set_sidebar_selection(const std::string & sel);

  signals:
    void toggle_fullscreen();
    void confirm_delete(TRecordingPtr);
    void playback(TRecordingPtr);

  public slots:
    void layoutChanged();
    void dataChanged();

    void sync_ui();
    void sync_ui_epg();
    void sync_ui_channels();
    void sync_ui_schedule();
    void sync_ui_playlists();
    void sync_ui_playlist(const std::string & playlist_name,
                          const TRecordings & playlist_recs);

    void toggle_recording(uint32_t ch_num, uint32_t gps_time);
    void delete_recording(const std::string & name);
    void playback_recording(const std::string & name);

  protected:
    // helpers:
    void layout(AppView & view, AppStyle & style, Item & root);
    void layout_sidebar(AppView & view, AppStyle & style, Item & sideview);
    void layout_epg(AppView & view, AppStyle & style, Item & mainview);
    void layout_channels(AppView & view, AppStyle & style, Item & mainview);
    void layout_schedule(AppView & view, AppStyle & style, Item & mainview);

    // model:
    yae::DVR * dvr_;

    QTimer sync_ui_;

    // for manual uncaching of EPG layout at the top of each hour:
    int64_t gps_hour_;

  public:
    // UI state:
    std::string sidebar_sel_;

    // collapsed item groups:
    std::set<std::string> collapsed_;

    yae::shared_ptr<AppStyle, Item> style_;
    yae::shared_ptr<Item> sideview_;
    yae::shared_ptr<Item> mainview_;
    yae::shared_ptr<Item> epg_view_;

    yae::mpeg_ts::EPG epg_;
    yae::TTime epg_lastmod_;

    DVR::Blacklist blacklist_;
    std::map<std::string, TChannels> channels_;

    // scheduled recordings, indexed by channel number:
    std::map<uint32_t, TScheduledRecordings> schedule_;

    // all recordings, indexed by filename:
    TRecordings recordings_;

    // all recordings, indexed by playlist:
    std::map<std::string, TRecordings> playlists_;

    std::map<uint32_t, std::size_t> ch_index_;
    std::map<uint32_t, yae::shared_ptr<Gradient, Item> > ch_tile_;
    std::map<uint32_t, yae::shared_ptr<Item> > ch_row_;
    std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<Item> > > ch_prog_;
    std::map<uint32_t, yae::shared_ptr<Item> > tickmark_;
    std::map<uint32_t, yae::shared_ptr<Rectangle, Item> > rec_highlight_;

    // channel list stuff:
    Layout ch_layout_;

    // schedule stuff:
    Layout sch_layout_;

    // playlist stuff:
    std::map<std::string, std::size_t> pl_index_;
    std::map<std::string, yae::shared_ptr<Item> > pl_sidebar_;
    std::map<std::string, yae::shared_ptr<Layout> > pl_layout_;

    // sidebar wishlist stuff:
    std::map<std::string, std::size_t> wl_index_;
    std::map<std::string, yae::shared_ptr<Item> > wl_sidebar_;
  };

}


#endif // YAE_APP_VIEW_H_
