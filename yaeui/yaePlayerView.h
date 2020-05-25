// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:05:08 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_VIEW_H_
#define YAE_PLAYER_VIEW_H_

// Qt library:
#include <QAction>
#include <QObject>
#include <QMenu>
#include <QMenuBar>
#include <QSignalMapper>
#include <QShortcut>
#include <QString>
#include <QTimer>

// yaeui:
#ifdef __APPLE__
#include "yaeAppleRemoteControl.h"
#include "yaeAppleUtils.h"
#endif
#include "yaeItemView.h"
#include "yaeItemViewStyle.h"
#include "yaePlayerItem.h"
#include "yaeTimelineItem.h"


namespace yae
{

#ifdef __APPLE__
  //----------------------------------------------------------------
  // RemoteControlEvent
  //
  struct RemoteControlEvent : public QEvent
  {
    RemoteControlEvent(TRemoteControlButtonId buttonId,
                       bool pressedDown,
                       unsigned int clickCount,
                       bool heldDown):
      QEvent(QEvent::User),
      buttonId_(buttonId),
      pressedDown_(pressedDown),
      clickCount_(clickCount),
      heldDown_(heldDown)
    {}

    TRemoteControlButtonId buttonId_;
    bool pressedDown_;
    unsigned int clickCount_;
    bool heldDown_;
  };
#endif

  //----------------------------------------------------------------
  // PlayerView
  //
  class YAEUI_API PlayerView : public ItemView
  {
    Q_OBJECT;

    void init_actions();
    void translate_ui();

  public:
    PlayerView();
    ~PlayerView();

    // accessors:
    inline Canvas & canvas()
    { return delegate_->windowCanvas(); }

    inline IReader * get_reader() const
    { return player_->reader().get(); }

    inline TimelineModel & timeline_model() const
    { return player_->timeline(); }

    void setStyle(const yae::shared_ptr<ItemViewStyle, Item> & style);

    // virtual:
    ItemViewStyle * style() const
    { return style_.get(); }

    // virtual:
    void setContext(const yae::shared_ptr<IOpenGLContext> & context);

    // virtual:
    void setEnabled(bool enable);

    // virtual:
    bool event(QEvent * e);

    // virtual:
    bool processEvent(Canvas * canvas, QEvent * event);
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processWheelEvent(Canvas * canvas, QWheelEvent * event);
    bool processMouseTracking(const TVec2D & mousePt);

    // helper:
    void insert_menus(const IReaderPtr & reader,
                      QMenuBar * menubar,
                      QAction * before = NULL);

    // helper:
    static TVideoFramePtr autocrop_cb(void * context,
                                      const TCropFrame & detected,
                                      bool detectionFinished);
  signals:
    void adjust_canvas_height();
    void toggle_playlist();
    void playback_next();
    void playback_finished(TTime playhead_pos);
    void fixup_next_prev();
    void save_bookmark();
    void on_back_arrow();
    void rc_menu_button_pressed();
    void video_track_selected();

    void select_frame_crop();
    void select_aspect_ratio();
    void select_video_track();
    void select_audio_track();
    void select_subtt_track();
    void delete_playing_file();

  public:
    bool is_playback_paused() const;
    void playback(const IReaderPtr & reader_ptr,
                  const IBookmark * bookmark = NULL,
                  bool start_from_zero_time = false);

    // ugh, all this is because in Qt4 signals are protected:
    inline void emit_toggle_playback()
    { emit togglePlayback(); }

    inline void emit_toggle_playlist()
    { emit toggle_playlist(); }

    inline void emit_on_back_arrow()
    { emit on_back_arrow(); }

    inline void emit_select_frame_crop()
    { emit select_frame_crop(); }

    inline void emit_select_aspect_ratio()
    { emit select_aspect_ratio(); }

    inline void emit_select_video_track()
    { emit select_video_track(); }

    inline void emit_select_audio_track()
    { emit select_audio_track(); }

    inline void emit_select_subtt_track()
    { emit select_subtt_track(); }

    inline void emit_delete_playing_file()
    { emit delete_playing_file(); }

  public slots:
    // live timeline refresh:
    void sync_ui();

    // menu actions:
    void playbackShowTimeline();

    void playbackCropFrameNone();
    void playbackCropFrame2_40();
    void playbackCropFrame2_35();
    void playbackCropFrame1_85();
    void playbackCropFrame1_78();
    void playbackCropFrame1_60();
    void playbackCropFrame1_33();
    void playbackCropFrameAutoDetect();

    void playbackAspectRatioAuto();
    void playbackAspectRatio2_40();
    void playbackAspectRatio2_35();
    void playbackAspectRatio1_85();
    void playbackAspectRatio1_78();
    void playbackAspectRatio1_60();
    void playbackAspectRatio1_33();

    void playbackLoop();
    void playbackColorConverter();
    void playbackLoopFilter();
    void playbackNonReferenceFrames();
    void playbackDeinterlace();
    void playbackSetTempo(int percent);

    // audio/video menus:
    void audioDownmixToStereo();
    void audioSelectTrack(int index);
    void videoSelectTrack(int index);
    void subsSelectTrack(int index);

    // chapters menu:
    void updateChaptersMenu();
    void skipToNextChapter();
    void skipToChapter(int index);

    // helpers:
    void skipToNextFrame();
    void skipForward();
    void skipBack();
    void scrollWheelTimerExpired();
    void cropped(const TVideoFramePtr & frame, const TCropFrame & crop);
    void stopPlayback();
    void togglePlayback();
    void playbackFinished(const SharedClock & c);

    void populateContextMenu();
    void adjustMenuActions();

  protected:
    void adjustMenuActions(IReader * reader,
                           std::vector<TTrackInfo> & audio_info,
                           std::vector<AudioTraits> & audio_traits,
                           std::vector<TTrackInfo> & video_info,
                           std::vector<VideoTraits> & video_traits,
                           std::vector<TTrackInfo> & subs_info,
                           std::vector<TSubsFormat> & subs_sormat);
    void layout(PlayerView & view, const ItemViewStyle & style, Item & root);

  public:
    QAction * actionShowTimeline_;

    QAction * actionPlay_;
    QAction * actionNextChapter_;

    QAction * actionLoop_;
    QAction * actionSetInPoint_;
    QAction * actionSetOutPoint_;

    QAction * actionFullScreen_;
    QAction * actionFillScreen_;

    QAction * actionCropFrameNone_;
    QAction * actionCropFrame1_33_;
    QAction * actionCropFrame1_60_;
    QAction * actionCropFrame1_78_;
    QAction * actionCropFrame1_85_;
    QAction * actionCropFrame2_40_;
    QAction * actionCropFrame2_35_;
    QAction * actionCropFrameAutoDetect_;
    QAction * actionCropFrameOther_;

    QAction * actionAspectRatioAuto_;
    QAction * actionAspectRatio1_33_;
    QAction * actionAspectRatio1_60_;
    QAction * actionAspectRatio1_78_;
    QAction * actionAspectRatio1_85_;
    QAction * actionAspectRatio2_35_;
    QAction * actionAspectRatio2_40_;
    QAction * actionAspectRatioOther_;

    QAction * actionHalfSize_;
    QAction * actionFullSize_;
    QAction * actionDoubleSize_;
    QAction * actionDecreaseSize_;
    QAction * actionIncreaseSize_;
    QAction * actionShrinkWrap_;

    QAction * actionVerticalScaling_;
    QAction * actionDeinterlace_;
    QAction * actionSkipColorConverter_;
    QAction * actionSkipLoopFilter_;
    QAction * actionSkipNonReferenceFrames_;
    QAction * actionDownmixToStereo_;

    QAction * actionTempo50_;
    QAction * actionTempo60_;
    QAction * actionTempo70_;
    QAction * actionTempo80_;
    QAction * actionTempo90_;
    QAction * actionTempo100_;
    QAction * actionTempo111_;
    QAction * actionTempo125_;
    QAction * actionTempo143_;
    QAction * actionTempo167_;
    QAction * actionTempo200_;

    QMenu * menuPlayback_;
    QMenu * menuPlaybackSpeed_;
    QMenu * menuAudio_;
    QMenu * menuVideo_;
    QMenu * menuWindowSize_;
    QMenu * menuCropFrame_;
    QMenu * menuAspectRatio_;
    QMenu * menuSubs_;
    QMenu * menuChapters_;

    // context sensitive menu which includes most relevant actions:
    QMenu * contextMenu_;

    // audio/video track selection widgets:
    QActionGroup * audioTrackGroup_;
    QActionGroup * videoTrackGroup_;
    QActionGroup * subsTrackGroup_;
    QActionGroup * chaptersGroup_;

    QSignalMapper * playRateMapper_;
    QSignalMapper * audioTrackMapper_;
    QSignalMapper * videoTrackMapper_;
    QSignalMapper * subsTrackMapper_;
    QSignalMapper * chapterMapper_;

    // (live) timeline update timer:
    QTimer timelineTimer_;

    // auto-crop single shot timer:
    QTimer autocropTimer_;

    // auto-bookmark timer:
    QTimer bookmarkTimer_;

    // scroll-wheel timer:
    QTimer scrollWheelTimer_;
    double scrollStart_;
    double scrollOffset_;

    // items:
    yae::shared_ptr<ItemViewStyle, Item> style_;
    yae::shared_ptr<PlayerItem, Item> player_;
    yae::shared_ptr<TimelineItem, Item> timeline_;

    BoolRef enableBackArrowButton_;
    BoolRef enableDeleteFileButton_;
  };

}


#endif // YAE_PLAYER_VIEW_H_
