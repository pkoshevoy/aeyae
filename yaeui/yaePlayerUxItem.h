// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Nov  7 15:26:58 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

#ifndef YAE_PLAYER_UX_ITEM_H_
#define YAE_PLAYER_UX_ITEM_H_

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
#include "yaeAspectRatioItem.h"
#include "yaeFrameCropItem.h"
#include "yaeItemView.h"
#include "yaeOptionItem.h"
#include "yaePlayerItem.h"
#include "yaePlayerShortcuts.h"
#include "yaeTimelineItem.h"


namespace yae
{

  //----------------------------------------------------------------
  // PlayerUxItem
  //
  class YAEUI_API PlayerUxItem : public QObject, public Item
  {
    Q_OBJECT;

    void init_actions();
    void translate_ui();

  public:
    PlayerUxItem(const char * id, ItemView & view);
    ~PlayerUxItem();

  protected:
    void init_player();
    void init_timeline();
    void init_frame_crop();
    void init_frame_crop_sel();
    void init_aspect_ratio_sel();
    void init_video_track_sel();
    void init_audio_track_sel();
    void init_subtt_track_sel();

  public:
    // accessors:
    inline Canvas * canvas()
    { return player_->get_canvas(); }

    inline IReader * get_reader() const
    { return player_->reader().get(); }

    inline TimelineModel & timeline_model() const
    { return player_->timeline(); }

    // virtual:
    void setVisible(bool visible);

    // virtual:
    bool event(QEvent * e);

    // virtual:
    bool processKeyEvent(Canvas * canvas, QKeyEvent * event);
    bool processWheelEvent(Canvas * canvas, QWheelEvent * event);
    bool processMouseTracking(const TVec2D & mousePt);

    // helpers:
    void set_shortcuts(const yae::shared_ptr<PlayerShortcuts> & sc);

    void insert_menus(const IReaderPtr & reader,
                      QMenuBar * menubar,
                      QAction * before = NULL);

    // helper:
    static TVideoFramePtr autocrop_cb(void * context,
                                      const TCropFrame & detected,
                                      bool detectionFinished);
  signals:
    void reader_changed(IReaderPtr reader);
    void visibility_changed(bool);

    void toggle_playlist();
    void playback_next();
    void playback_finished(TTime playhead_pos);
    void fixup_next_prev();
    void save_bookmark();
    void save_bookmark_at(double position_in_ec);
    void on_back_arrow();
    void video_track_selected();

    void select_frame_crop();
    void select_aspect_ratio();
    void select_video_track();
    void select_audio_track();
    void select_subtt_track();
    void delete_playing_file();

    void enteringFullScreen();
    void exitingFullScreen();

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
    void playbackShowInFinder();

    void playbackCropFrameNone();
    void playbackCropFrame2_40();
    void playbackCropFrame2_35();
    void playbackCropFrame1_85();
    void playbackCropFrame1_78();
    void playbackCropFrame1_60();
    void playbackCropFrame1_33();
    void playbackCropFrameAutoDetect();
    void playbackCropFrameOther();

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

    // callback from aspect ratio view reflecting current selection:
    void selectAspectRatio(const AspectRatio & ar);
    void setAspectRatio(double ar);

    // helpers:
    void selectFrameCrop(const AspectRatio & ar);
    void showFrameCropSelection();
    void showAspectRatioSelection();
    void showVideoTrackSelection();
    void showAudioTrackSelection();
    void showSubttTrackSelection();

    void dismissFrameCrop();
    void dismissFrameCropSelection();
    void dismissAspectRatioSelection();
    void dismissVideoTrackSelection();
    void dismissAudioTrackSelection();
    void dismissSubttTrackSelection();
    void dismissSelectionItems();

    void videoTrackSelectedOption(int option_index);
    void audioTrackSelectedOption(int option_index);
    void subttTrackSelectedOption(int option_index);

    // window menu:
    void windowHalfSize();
    void windowFullSize();
    void windowDoubleSize();
    void windowDecreaseSize();
    void windowIncreaseSize();

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

    void swapShortcuts();
    void populateContextMenu();
    void adjustMenuActions();

    void adjustCanvasHeight();
    void canvasSizeSet(double xexpand, double yexpand);
    void canvasSizeScaleBy(double scale);
    void canvasSizeBackup();
    void canvasSizeRestore();

    void playbackVerticalScaling(bool enable);
    void playbackShrinkWrap();
    void playbackFullScreen();
    void playbackFillScreen();
    void requestToggleFullScreen();
    void toggleFullScreen();
    void enterFullScreen(Canvas::TRenderMode renderMode);
    void exitFullScreen();

  protected:
    void adjustMenuActions(IReader * reader,
                           std::vector<TTrackInfo> & audio_info,
                           std::vector<AudioTraits> & audio_traits,
                           std::vector<TTrackInfo> & video_info,
                           std::vector<VideoTraits> & video_traits,
                           std::vector<TTrackInfo> & subs_info,
                           std::vector<TSubsFormat> & subs_sormat);

  public:
    ItemView & view_;

    yae::shared_ptr<PlayerShortcuts> shortcuts_;

    QAction * actionShowTimeline_;
    QAction * actionShowInFinder_;

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
    yae::shared_ptr<PlayerItem, Item> player_;
    yae::shared_ptr<TimelineItem, Item> timeline_;
    yae::shared_ptr<FrameCropItem, Item> frame_crop_;
    yae::shared_ptr<AspectRatioItem, Item> frame_crop_sel_;
    yae::shared_ptr<AspectRatioItem, Item> aspect_ratio_sel_;
    yae::shared_ptr<OptionItem, Item> video_track_sel_;
    yae::shared_ptr<OptionItem, Item> audio_track_sel_;
    yae::shared_ptr<OptionItem, Item> subtt_track_sel_;

    yae::shared_ptr<Canvas::ILoadFrameObserver> onLoadFrame_;

    BoolRef enableBackArrowButton_;
    BoolRef enableDeleteFileButton_;

  protected:
    // remember most recently used full screen render mode:
    Canvas::TRenderMode renderMode_;

    // shrink wrap stretch factors:
    double xexpand_;
    double yexpand_;
  };

}


#endif // YAE_PLAYER_UX_ITEM_H_
