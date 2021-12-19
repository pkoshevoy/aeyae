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
#include "yae/thread/yae_worker.h"
#include "yae/utils/yae_lru_cache.h"
#include "yae/video/yae_video.h"

// yaeui:
#include "yaeColor.h"
#include "yaeGradient.h"
#include "yaeInputArea.h"
#include "yaeItemRef.h"
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaeRectangle.h"
#include "yaeScrollview.h"

// local:
#include "yaeAppStyle.h"
#include "yae_dvr.h"


namespace yae
{

  //----------------------------------------------------------------
  // AppView
  //
  class AppView : public ItemView
  {
    Q_OBJECT;

  public:

    AppView(const char * name);
    ~AppView();

    // virtual:
    void setEnabled(bool enable);

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // data source:
    void setModel(yae::DVR * dvr);

    inline yae::DVR * model() const
    { return dvr_; }

    // virtual:
    AppStyle * style() const
    { return style_; }

    // virtual:
    bool resizeTo(const Canvas * canvas);

    // virtual:
    bool processMouseEvent(Canvas * canvas, QMouseEvent * event);

    // helper:
    TRecPtr now_playing() const;

    // for async scanning of available recordings:
    void found_recordings(const TFoundRecordingsPtr & found);

  signals:
    void toggle_fullscreen();
    void confirm_delete(TRecPtr);
    void playback(TRecPtr);
    void watch_live(uint32_t ch_num, TTime seek_pos);

  public:
    // signals are protected in Qt4, this is a workaround:
    inline void emit_confirm_delete(TRecPtr rec)
    { emit confirm_delete(rec); }

    inline void emit_playback(TRecPtr rec)
    { emit playback(rec); }

    inline void emit_watch_live(uint32_t ch_num, TTime seek_pos)
    { emit watch_live(ch_num, seek_pos); }

  public slots:
    void layoutChanged();
    void dataChanged();
    void requestUncacheEPG();

    void sync_ui();
    void sync_ui_epg();
    void sync_ui_channels();
    void sync_ui_schedule();
    void sync_ui_wishlist();
    void sync_ui_playlists();
    void sync_ui_playlist(const std::string & playlist_name,
                          const TRecs & playlist_recs);

    void on_watch_live(uint32_t ch_num);
    void show_program_details(uint32_t ch_num, uint32_t gps_time);
    void toggle_recording(uint32_t ch_num, uint32_t gps_time);
    void delete_recording(const std::string & name);
    void playback_recording(const std::string & name);
    void watch_now(yae::shared_ptr<DVR::Playback> playback_ptr, TRecPtr rec);
    void add_wishlist_item();
    void add_wishlist_item(const yae::shared_ptr<DVR::ChanTime> & program_sel);
    void edit_wishlist_item(const std::string & row_id);
    void remove_wishlist_item(const std::string & wi_key);
    void save_wishlist_item();

  protected slots:
    void update_wi_channel(const QString &);
    void update_wi_title(const QString &);
    void update_wi_desc(const QString &);
    void update_wi_time_start(const QString &);
    void update_wi_time_end(const QString &);
    void update_wi_date(const QString &);
    void update_wi_min_minutes(const QString &);
    void update_wi_max_minutes(const QString &);
    void update_wi_max(const QString &);

  protected:
    // helpers:
    void layout(AppView & view, AppStyle & style, Item & root);
    void layout_sidebar(AppView & view, AppStyle & style, Item & sideview);
    void layout_epg(AppView & view, AppStyle & style, Item & mainview);
    void layout_program_details(AppView & view, AppStyle & s, Item & mainview);
    void layout_channels(AppView & view, AppStyle & style, Item & mainview);
    void layout_schedule(AppView & view, AppStyle & style, Item & mainview);
    void layout_wishlist(AppView & view, AppStyle & style, Item & mainview);

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

    std::set<uint32_t> blocklist_;

    // frequency -> major:minor:name:
    std::map<std::string, TChannels> channels_;

    std::map<std::string, Wishlist::Item> wishlist_;

    // scheduled recordings, indexed by channel number:
    std::map<uint32_t, TScheduledRecordings> schedule_;

    // all recordings, indexed by filename:
    TRecs recordings_;

    // all recordings, indexed by playlist:
    std::map<std::string, TRecs> playlists_;

    // all recordings, indexed by channel and gps start time:
    std::map<uint32_t, TRecsByTime> rec_by_channel_;

    yae::shared_ptr<DVR::Playback> now_playing_;
    yae::shared_ptr<DVR::ChanTime> program_sel_;

    std::map<uint32_t, std::size_t> ch_index_;
    std::map<uint32_t, yae::shared_ptr<Gradient, Item> > ch_tile_;
    std::map<uint32_t, yae::shared_ptr<Item> > ch_row_;
    std::map<uint32_t, std::map<uint32_t, yae::shared_ptr<Item> > > ch_prog_;
    std::map<uint32_t, yae::shared_ptr<Item> > tickmark_;
    std::map<uint32_t, yae::shared_ptr<Rectangle, Item> > rec_highlight_;

    // program details:
    Layout pd_layout_;

    // channel list stuff:
    Layout ch_layout_;

    // schedule stuff:
    Layout sch_layout_;

    // playlist stuff:
    std::map<std::string, std::size_t> pl_index_;
    std::map<std::string, yae::shared_ptr<Item> > pl_sidebar_;
    std::map<std::string, yae::shared_ptr<Layout> > pl_layout_;

    // wishlist stuff:
    std::map<std::string, std::size_t> wl_index_;
    std::map<std::string, yae::shared_ptr<Item> > wl_sidebar_;
    yae::shared_ptr<Item> wishlist_ui_;
    yae::shared_ptr<std::pair<std::string, Wishlist::Item> > wi_edit_;

  protected:
    mutable boost::mutex mutex_;
    yae::Worker worker_;
    TFoundRecordingsPtr found_recordings_;
  };

}


#endif // YAE_APP_VIEW_H_
