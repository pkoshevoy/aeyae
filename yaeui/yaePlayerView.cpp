// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Fri Jan 31 21:07:25 MST 2020
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QApplication>

// yaeui:
#include "yaePlayerStyle.h"
#include "yaePlayerView.h"
#include "yaeUtilsQt.h"


namespace yae
{

  //----------------------------------------------------------------
  // kDownmixToStereo
  //
  static const QString kDownmixToStereo =
    QString::fromUtf8("DownmixToStereo");

  //----------------------------------------------------------------
  // kSkipColorConverter
  //
  static const QString kSkipColorConverter =
    QString::fromUtf8("SkipColorConverter");

  //----------------------------------------------------------------
  // kSkipLoopFilter
  //
  static const QString kSkipLoopFilter =
    QString::fromUtf8("SkipLoopFilter");

  //----------------------------------------------------------------
  // kSkipNonReferenceFrames
  //
  static const QString kSkipNonReferenceFrames =
    QString::fromUtf8("SkipNonReferenceFrames");

  //----------------------------------------------------------------
  // kDeinterlaceFrames
  //
  static const QString kDeinterlaceFrames =
    QString::fromUtf8("DeinterlaceFrames");

  //----------------------------------------------------------------
  // kShowTimeline
  //
  static const QString kShowTimeline =
    QString::fromUtf8("ShowTimeline");


  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(const PlayerView & view):
      view_(view)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = view_.is_playback_paused();
    }

    const PlayerView & view_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->togglePlayback();
  }

  //----------------------------------------------------------------
  // toggle_playlist
  //
  static void
  toggle_playlist(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->togglePlaylist();
  }

  //----------------------------------------------------------------
  // back_arrow_cb
  //
  static void
  back_arrow_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->onBackArrow();
  }

  //----------------------------------------------------------------
  // select_frame_crop_cb
  //
  static void
  select_frame_crop_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerSelectFrameCrop();
  }

  //----------------------------------------------------------------
  // select_aspect_ratio_cb
  //
  static void
  select_aspect_ratio_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerSelectAspectRatio();
  }

  //----------------------------------------------------------------
  // select_video_track_cb
  //
  static void
  select_video_track_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerSelectVideoTrack();
  }

  //----------------------------------------------------------------
  // select_audio_track_cb
  //
  static void
  select_audio_track_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerSelectAudioTrack();
  }

  //----------------------------------------------------------------
  // select_subtt_track_cb
  //
  static void
  select_subtt_track_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerSelectSubttTrack();
  }

  //----------------------------------------------------------------
  // delete_playing_file_cb
  //
  static void
  delete_playing_file_cb(void * context)
  {
    PlayerView * view = (PlayerView *)context;
    view->triggerDeletePlayingFile();
  }

  //----------------------------------------------------------------
  // new_qaction
  //
  template <typename TQObj>
  static TQObj *
  add(QObject * parent, const char * objectName)
  {
    TQObj * obj = new TQObj(parent);
    obj->setObjectName(QString::fromUtf8(objectName));
    return obj;
  }

  //----------------------------------------------------------------
  // add_menu
  //
  static QMenu *
  add_menu(const char * objectName)
  {
    QMenu * menu = new QMenu();
    menu->setObjectName(QString::fromUtf8(objectName));
    return menu;
  }

  //----------------------------------------------------------------
  // PlayerView::init_actions
  //
  void
  PlayerView::init_actions()
  {
    actionShowTimeline_ = add<QAction>(this, "actionShowTimeline");
    actionShowTimeline_->setCheckable(true);
    actionShowTimeline_->setChecked(false);
    actionShowTimeline_->setShortcutContext(Qt::ApplicationShortcut);


    actionPlay_ = add<QAction>(this, "actionPlay");
    actionPlay_->setShortcutContext(Qt::ApplicationShortcut);

    actionNextChapter_ = add<QAction>(this, "actionNextChapter");
    actionNextChapter_->setShortcutContext(Qt::ApplicationShortcut);

    actionNext_ = add<QAction>(this, "actionNext");
    actionNext_->setShortcutContext(Qt::ApplicationShortcut);

    actionPrev_ = add<QAction>(this, "actionPrev");
    actionPrev_->setShortcutContext(Qt::ApplicationShortcut);



    actionLoop_ = add<QAction>(this, "actionLoop");
    actionLoop_->setCheckable(true);
    actionLoop_->setShortcutContext(Qt::ApplicationShortcut);

    actionSetInPoint_ = add<QAction>(this, "actionSetInPoint");
    actionSetOutPoint_ = add<QAction>(this, "actionSetOutPoint");



    actionFullScreen_ = add<QAction>(this, "actionFullScreen");
    actionFullScreen_->setCheckable(true);
    actionFullScreen_->setShortcutContext(Qt::ApplicationShortcut);


    actionFillScreen_ = add<QAction>(this, "actionFillScreen");
    actionFillScreen_->setCheckable(true);
    actionFillScreen_->setShortcutContext(Qt::ApplicationShortcut);



    actionCropFrameNone_ = add<QAction>(this, "actionCropFrameNone");
    actionCropFrameNone_->setCheckable(true);
    actionCropFrameNone_->setChecked(true);
    actionCropFrameNone_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_33_ = add<QAction>(this, "actionCropFrame1_33");
    actionCropFrame1_33_->setCheckable(true);
    actionCropFrame1_33_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_60_ = add<QAction>(this, "actionCropFrame1_60");
    actionCropFrame1_60_->setCheckable(true);

    actionCropFrame1_78_ = add<QAction>(this, "actionCropFrame1_78");
    actionCropFrame1_78_->setCheckable(true);
    actionCropFrame1_78_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_85_ = add<QAction>(this, "actionCropFrame1_85");
    actionCropFrame1_85_->setCheckable(true);
    actionCropFrame1_85_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame2_35_ = add<QAction>(this, "actionCropFrame2_35");
    actionCropFrame2_35_->setCheckable(true);

    actionCropFrame2_40_ = add<QAction>(this, "actionCropFrame2_40");
    actionCropFrame2_40_->setCheckable(true);
    actionCropFrame2_40_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrameOther_ = add<QAction>(this, "actionCropFrameOther");
    actionCropFrameOther_->setCheckable(true);
    actionCropFrameOther_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrameAutoDetect_ =
      add<QAction>(this, "actionCropFrameAutoDetect");
    actionCropFrameAutoDetect_->setCheckable(true);
    actionCropFrameAutoDetect_->setShortcutContext(Qt::ApplicationShortcut);



    actionAspectRatioAuto_ = add<QAction>(this, "actionAspectRatioAuto");
    actionAspectRatioAuto_->setCheckable(true);

    actionAspectRatio1_33_ = add<QAction>(this, "actionAspectRatio1_33");
    actionAspectRatio1_33_->setCheckable(true);

    actionAspectRatio1_60_ = add<QAction>(this, "actionAspectRatio1_60");
    actionAspectRatio1_60_->setCheckable(true);

    actionAspectRatio1_78_ = add<QAction>(this, "actionAspectRatio1_78");
    actionAspectRatio1_78_->setCheckable(true);

    actionAspectRatio1_85_ = add<QAction>(this, "actionAspectRatio1_85");
    actionAspectRatio1_85_->setCheckable(true);

    actionAspectRatio2_35_ = add<QAction>(this, "actionAspectRatio2_35");
    actionAspectRatio2_35_->setCheckable(true);

    actionAspectRatio2_40_ = add<QAction>(this, "actionAspectRatio2_40");
    actionAspectRatio2_40_->setCheckable(true);

    actionAspectRatioOther_ = add<QAction>(this, "actionAspectRatioOther");
    actionAspectRatioOther_->setCheckable(true);




    actionHalfSize_ = add<QAction>(this, "actionHalfSize");
    actionHalfSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionFullSize_ = add<QAction>(this, "actionFullSize");
    actionFullSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionDoubleSize_ = add<QAction>(this, "actionDoubleSize");
    actionDoubleSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionDecreaseSize_ = add<QAction>(this, "actionDecreaseSize");
    actionDecreaseSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionIncreaseSize_ = add<QAction>(this, "actionIncreaseSize");
    actionIncreaseSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionShrinkWrap_ = add<QAction>(this, "actionShrinkWrap");
    actionShrinkWrap_->setShortcutContext(Qt::ApplicationShortcut);




    actionVerticalScaling_ = add<QAction>(this, "actionVerticalScaling");
    actionVerticalScaling_->setCheckable(true);

    actionDeinterlace_ = add<QAction>(this, "actionDeinterlace");
    actionDeinterlace_->setCheckable(true);

    actionSkipColorConverter_ = add<QAction>(this, "actionSkipColorConverter");
    actionSkipColorConverter_->setCheckable(true);

    actionSkipLoopFilter_ = add<QAction>(this, "actionSkipLoopFilter");
    actionSkipLoopFilter_->setCheckable(true);

    actionSkipNonReferenceFrames_ =
      add<QAction>(this, "actionSkipNonReferenceFrames");
    actionSkipNonReferenceFrames_->setCheckable(true);

    actionDownmixToStereo_ = add<QAction>(this, "actionDownmixToStereo");
    actionDownmixToStereo_->setCheckable(true);



    actionTempo50_ = add<QAction>(this, "actionTempo50");
    actionTempo50_->setCheckable(true);

    actionTempo60_ = add<QAction>(this, "actionTempo60");
    actionTempo60_->setCheckable(true);

    actionTempo70_ = add<QAction>(this, "actionTempo70");
    actionTempo70_->setCheckable(true);

    actionTempo80_ = add<QAction>(this, "actionTempo80");
    actionTempo80_->setCheckable(true);

    actionTempo90_ = add<QAction>(this, "actionTempo90");
    actionTempo90_->setCheckable(true);

    actionTempo100_ = add<QAction>(this, "actionTempo100");
    actionTempo100_->setCheckable(true);

    actionTempo111_ = add<QAction>(this, "actionTempo111");
    actionTempo111_->setCheckable(true);

    actionTempo125_ = add<QAction>(this, "actionTempo125");
    actionTempo125_->setCheckable(true);

    actionTempo143_ = add<QAction>(this, "actionTempo143");
    actionTempo143_->setCheckable(true);

    actionTempo167_ = add<QAction>(this, "actionTempo167");
    actionTempo167_->setCheckable(true);

    actionTempo200_ = add<QAction>(this, "actionTempo200");
    actionTempo200_->setCheckable(true);



    menuPlayback_ = add_menu("menuPlayback");
    menuPlaybackSpeed_ = add_menu("menuPlaybackSpeed");
    menuAudio_ = add_menu("menuAudio");
    menuVideo_ = add_menu("menuVideo");
    menuWindowSize_ = add_menu("menuWindowSize");
    menuCropFrame_ = add_menu("menuCropFrame");
    menuAspectRatio_ = add_menu("menuAspectRatio");
    menuSubs_ = add_menu("menuSubs");
    menuChapters_ = add_menu("menuChapters");
    contextMenu_ = add_menu("contextMenu");

    menuPlayback_->addAction(actionPlay_);
    menuPlayback_->addSeparator();
#if 0
    menuPlayback_->addAction(actionPrev_);
    menuPlayback_->addAction(actionNext_);
    menuPlayback_->addSeparator();
#endif
    menuPlayback_->addAction(actionLoop_);
    menuPlayback_->addAction(actionSetInPoint_);
    menuPlayback_->addAction(actionSetOutPoint_);
    menuPlayback_->addAction(actionShowTimeline_);
    menuPlayback_->addSeparator();
    menuPlayback_->addAction(actionShrinkWrap_);
    menuPlayback_->addAction(actionFullScreen_);
    menuPlayback_->addAction(actionFillScreen_);
    menuPlayback_->addSeparator();
    menuPlayback_->addAction(menuPlaybackSpeed_->menuAction());

    menuPlaybackSpeed_->addAction(actionTempo50_);
    menuPlaybackSpeed_->addAction(actionTempo60_);
    menuPlaybackSpeed_->addAction(actionTempo70_);
    menuPlaybackSpeed_->addAction(actionTempo80_);
    menuPlaybackSpeed_->addAction(actionTempo90_);
    menuPlaybackSpeed_->addAction(actionTempo100_);
    menuPlaybackSpeed_->addAction(actionTempo111_);
    menuPlaybackSpeed_->addAction(actionTempo125_);
    menuPlaybackSpeed_->addAction(actionTempo143_);
    menuPlaybackSpeed_->addAction(actionTempo167_);
    menuPlaybackSpeed_->addAction(actionTempo200_);

    menuAudio_->addAction(actionDownmixToStereo_);
    menuAudio_->addSeparator();

    menuVideo_->addAction(menuCropFrame_->menuAction());
    menuVideo_->addAction(menuAspectRatio_->menuAction());
    menuVideo_->addAction(menuWindowSize_->menuAction());
    menuVideo_->addSeparator();
    menuVideo_->addAction(actionVerticalScaling_);
    menuVideo_->addAction(actionDeinterlace_);
    menuVideo_->addAction(actionSkipColorConverter_);
    menuVideo_->addAction(actionSkipLoopFilter_);
    menuVideo_->addAction(actionSkipNonReferenceFrames_);
    menuVideo_->addSeparator();

    menuCropFrame_->addAction(actionCropFrameNone_);
    menuCropFrame_->addAction(actionCropFrame1_33_);
    menuCropFrame_->addAction(actionCropFrame1_60_);
    menuCropFrame_->addAction(actionCropFrame1_78_);
    menuCropFrame_->addAction(actionCropFrame1_85_);
    menuCropFrame_->addAction(actionCropFrame2_35_);
    menuCropFrame_->addAction(actionCropFrame2_40_);
    menuCropFrame_->addAction(actionCropFrameOther_);
    menuCropFrame_->addAction(actionCropFrameAutoDetect_);

    menuAspectRatio_->addAction(actionAspectRatioAuto_);
    menuAspectRatio_->addAction(actionAspectRatio1_33_);
    menuAspectRatio_->addAction(actionAspectRatio1_60_);
    menuAspectRatio_->addAction(actionAspectRatio1_78_);
    menuAspectRatio_->addAction(actionAspectRatio1_85_);
    menuAspectRatio_->addAction(actionAspectRatio2_35_);
    menuAspectRatio_->addAction(actionAspectRatio2_40_);
    menuAspectRatio_->addAction(actionAspectRatioOther_);

    menuWindowSize_->addAction(actionHalfSize_);
    menuWindowSize_->addAction(actionFullSize_);
    menuWindowSize_->addAction(actionDoubleSize_);
    menuWindowSize_->addSeparator();
    menuWindowSize_->addAction(actionDecreaseSize_);
    menuWindowSize_->addAction(actionIncreaseSize_);

    menuChapters_->addAction(actionNextChapter_);
    menuChapters_->addSeparator();


    // restore timeline preference (default == hide):
    bool showTimeline =
      loadBooleanSettingOrDefault(kShowTimeline, false);
    actionShowTimeline_->setChecked(showTimeline);

    bool downmixToStereo =
      loadBooleanSettingOrDefault(kDownmixToStereo, true);
    actionDownmixToStereo_->setChecked(downmixToStereo);

    bool skipColorConverter =
      loadBooleanSettingOrDefault(kSkipColorConverter, false);
    actionSkipColorConverter_->setChecked(skipColorConverter);

    bool skipLoopFilter =
      loadBooleanSettingOrDefault(kSkipLoopFilter, false);
    actionSkipLoopFilter_->setChecked(skipLoopFilter);

    bool skipNonReferenceFrames =
      loadBooleanSettingOrDefault(kSkipNonReferenceFrames, false);
    actionSkipNonReferenceFrames_->setChecked(skipNonReferenceFrames);

    bool deinterlaceFrames =
      loadBooleanSettingOrDefault(kDeinterlaceFrames, false);
    actionDeinterlace_->setChecked(deinterlaceFrames);

    actionRemove_ = add<QAction>(this, "actionRemove_");
    actionRemove_->setShortcutContext(Qt::ApplicationShortcut);

    QActionGroup * aspectRatioGroup = new QActionGroup(this);
    aspectRatioGroup->addAction(actionAspectRatioAuto_);
    aspectRatioGroup->addAction(actionAspectRatio1_33_);
    aspectRatioGroup->addAction(actionAspectRatio1_60_);
    aspectRatioGroup->addAction(actionAspectRatio1_78_);
    aspectRatioGroup->addAction(actionAspectRatio1_85_);
    aspectRatioGroup->addAction(actionAspectRatio2_35_);
    aspectRatioGroup->addAction(actionAspectRatio2_40_);
    aspectRatioGroup->addAction(actionAspectRatioOther_);
    actionAspectRatioAuto_->setChecked(true);

    QActionGroup * cropFrameGroup = new QActionGroup(this);
    cropFrameGroup->addAction(actionCropFrameNone_);
    cropFrameGroup->addAction(actionCropFrame1_33_);
    cropFrameGroup->addAction(actionCropFrame1_60_);
    cropFrameGroup->addAction(actionCropFrame1_78_);
    cropFrameGroup->addAction(actionCropFrame1_85_);
    cropFrameGroup->addAction(actionCropFrame2_35_);
    cropFrameGroup->addAction(actionCropFrame2_40_);
    cropFrameGroup->addAction(actionCropFrameOther_);
    cropFrameGroup->addAction(actionCropFrameAutoDetect_);
    actionCropFrameNone_->setChecked(true);

    QActionGroup * playRateGroup = new QActionGroup(this);
    playRateGroup->addAction(actionTempo50_);
    playRateGroup->addAction(actionTempo60_);
    playRateGroup->addAction(actionTempo70_);
    playRateGroup->addAction(actionTempo80_);
    playRateGroup->addAction(actionTempo90_);
    playRateGroup->addAction(actionTempo100_);
    playRateGroup->addAction(actionTempo111_);
    playRateGroup->addAction(actionTempo125_);
    playRateGroup->addAction(actionTempo143_);
    playRateGroup->addAction(actionTempo167_);
    playRateGroup->addAction(actionTempo200_);
    actionTempo100_->setChecked(true);

    playRateMapper_ = new QSignalMapper(this);
    playRateMapper_->setMapping(actionTempo50_, 50);
    playRateMapper_->setMapping(actionTempo60_, 60);
    playRateMapper_->setMapping(actionTempo70_, 70);
    playRateMapper_->setMapping(actionTempo80_, 80);
    playRateMapper_->setMapping(actionTempo90_, 90);
    playRateMapper_->setMapping(actionTempo100_, 100);
    playRateMapper_->setMapping(actionTempo111_, 111);
    playRateMapper_->setMapping(actionTempo125_, 125);
    playRateMapper_->setMapping(actionTempo143_, 143);
    playRateMapper_->setMapping(actionTempo167_, 167);
    playRateMapper_->setMapping(actionTempo200_, 200);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    bool ok = true;
    ok = connect(playRateMapper_, SIGNAL(mapped(int)),
                 this, SLOT(playbackSetTempo(int)));
    YAE_ASSERT(ok);

    ok = connect(actionTempo50_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo60_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo70_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo80_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo90_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo100_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo111_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo125_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo143_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo167_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo200_, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioAuto_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioAuto()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_33_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_33()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_60_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_60()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_78_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_78()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_85_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_85()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio2_35_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_35()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio2_40_, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_40()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameNone_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameNone()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_33_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_33()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_60_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_60()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_78_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_78()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_85_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_85()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame2_35_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_35()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame2_40_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_40()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameAutoDetect_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(actionPlay_, SIGNAL(triggered()),
                 this, SLOT(togglePlayback()));
    YAE_ASSERT(ok);

    ok = connect(actionNext_, SIGNAL(triggered()),
                 this, SIGNAL(playback_next()));
    YAE_ASSERT(ok);

    ok = connect(actionPrev_, SIGNAL(triggered()),
                 this, SIGNAL(playback_prev()));
    YAE_ASSERT(ok);

    ok = connect(actionRemove_, SIGNAL(triggered()),
                 this, SIGNAL(playback_remove()));
    YAE_ASSERT(ok);

    ok = connect(actionLoop_, SIGNAL(triggered()),
                 this, SLOT(playbackLoop()));
    YAE_ASSERT(ok);

    ok = connect(actionShowTimeline_, SIGNAL(triggered()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipColorConverter_, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipLoopFilter_, SIGNAL(triggered()),
                 this, SLOT(playbackLoopFilter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipNonReferenceFrames_, SIGNAL(triggered()),
                 this, SLOT(playbackNonReferenceFrames()));
    YAE_ASSERT(ok);

    ok = connect(actionDeinterlace_, SIGNAL(triggered()),
                 this, SLOT(playbackDeinterlace()));
    YAE_ASSERT(ok);

    ok = connect(actionDownmixToStereo_, SIGNAL(triggered()),
                 this, SLOT(audioDownmixToStereo()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerView::translate_ui
  //
  void
  PlayerView::translate_ui()
  {
    actionShowTimeline_->setText(trUtf8("Show &Timeline"));
    actionShowTimeline_->setShortcut(trUtf8("Ctrl+T"));


    actionPlay_->setText(trUtf8("Pause"));
    actionPlay_->setShortcut(trUtf8("Space"));

    actionNextChapter_->setText(trUtf8("Skip To &Next Chapter"));
    actionNextChapter_->setShortcut(trUtf8("Ctrl+N"));

    actionNext_->setText(trUtf8("Skip"));
    actionNext_->setShortcut(trUtf8("Alt+Right"));

    actionPrev_->setText(trUtf8("Go Back"));
    actionPrev_->setShortcut(trUtf8("Alt+Left"));



    actionLoop_->setText(trUtf8("&Loop"));
    actionLoop_->setShortcut(trUtf8("Ctrl+L"));

    actionSetInPoint_->setText(trUtf8("Set &In Point"));
    actionSetInPoint_->setShortcut(trUtf8("I"));

    actionSetOutPoint_->setText(trUtf8("Set &Out Point"));
    actionSetOutPoint_->setShortcut(trUtf8("O"));



    actionFullScreen_->setText(trUtf8("&Full Screen (letterbox)"));
    actionFullScreen_->setShortcut(trUtf8("Ctrl+F"));

    actionFillScreen_->setText(trUtf8("F&ull Screen"));
    actionFillScreen_->setShortcut(trUtf8("Ctrl+G"));



    actionCropFrameNone_->setText(trUtf8("None"));
    actionCropFrameNone_->setShortcut(trUtf8("Alt+X"));

    actionCropFrame1_33_->setText(trUtf8("4:3"));
    actionCropFrame1_33_->setShortcut(trUtf8("Alt+4"));

    actionCropFrame1_60_->setText(trUtf8("16:10"));

    actionCropFrame1_78_->setText(trUtf8("16:9"));
    actionCropFrame1_78_->setShortcut(trUtf8("Alt+6"));

    actionCropFrame1_85_->setText(trUtf8("1.85"));
    actionCropFrame1_85_->setShortcut(trUtf8("Alt+8"));

    actionCropFrame2_35_->setText(trUtf8("2.35"));

    actionCropFrame2_40_->setText(trUtf8("2.40"));
    actionCropFrame2_40_->setShortcut(trUtf8("Alt+2"));

    actionCropFrameOther_->setText(trUtf8("Other"));
    actionCropFrameOther_->setToolTip(trUtf8("Edit crop region manually"));
    actionCropFrameOther_->setShortcut(trUtf8("Alt+Z"));

    actionCropFrameAutoDetect_->setText(trUtf8("Auto Crop"));
    actionCropFrameAutoDetect_->setShortcut(trUtf8("Alt+C"));




    actionAspectRatioAuto_->setText(trUtf8("Automatic"));
    actionAspectRatioAuto_->setShortcut(trUtf8("Ctrl+Alt+X"));

    actionAspectRatio1_33_->setText(trUtf8("4:3"));
    actionAspectRatio1_33_->setShortcut(trUtf8("Ctrl+Alt+4"));

    actionAspectRatio1_60_->setText(trUtf8("16:10"));

    actionAspectRatio1_78_->setText(trUtf8("16:9"));
    actionAspectRatio1_78_->setShortcut(trUtf8("Ctrl+Alt+6"));

    actionAspectRatio1_85_->setText(trUtf8("1.85"));
    actionAspectRatio2_35_->setText(trUtf8("2.35"));
    actionAspectRatio2_40_->setText(trUtf8("2.40"));

    actionAspectRatioOther_->setText(trUtf8("Other"));
    actionAspectRatioOther_->setShortcut(trUtf8("Ctrl+Alt+Z"));
    actionAspectRatioOther_->
      setToolTip(trUtf8("Specify custom frame aspect ratio"));



    actionHalfSize_->setText(trUtf8("Half Size"));
    actionHalfSize_->setShortcut(trUtf8("Ctrl+0"));

    actionFullSize_->setText(trUtf8("Full Size"));
    actionFullSize_->setShortcut(trUtf8("Ctrl+1"));

    actionDoubleSize_->setText(trUtf8("Double Size"));
    actionDoubleSize_->setShortcut(trUtf8("Ctrl+2"));

    actionDecreaseSize_->setText(trUtf8("Decrease Size"));
    actionDecreaseSize_->setShortcut(trUtf8("Ctrl+-"));

    actionIncreaseSize_->setText(trUtf8("Increase Size"));
    actionIncreaseSize_->setShortcut(trUtf8("Ctrl+="));

    actionShrinkWrap_->setText(trUtf8("Shrink &Wrap"));
    actionShrinkWrap_->setShortcut(trUtf8("Ctrl+R"));



    actionVerticalScaling_->setText(trUtf8("Allow Vertical Upscaling"));
    actionDeinterlace_->setText(trUtf8("&Deinterlace"));
    actionSkipColorConverter_->setText(trUtf8("Skip Color Converter"));
    actionSkipLoopFilter_->setText(trUtf8("Skip Loop Filter"));
    actionSkipNonReferenceFrames_->
      setText(trUtf8("Skip Non-Reference Frames"));
    actionDownmixToStereo_->setText(trUtf8("Mix Down To Stereo"));



    actionTempo50_->setText(trUtf8("50%"));
    actionTempo60_->setText(trUtf8("60%"));
    actionTempo70_->setText(trUtf8("70%"));
    actionTempo80_->setText(trUtf8("80%"));
    actionTempo90_->setText(trUtf8("90%"));
    actionTempo100_->setText(trUtf8("100%"));
    actionTempo111_->setText(trUtf8("111%"));
    actionTempo125_->setText(trUtf8("125%"));
    actionTempo143_->setText(trUtf8("143%"));
    actionTempo167_->setText(trUtf8("167%"));
    actionTempo200_->setText(trUtf8("200%"));



    actionRemove_->setText(trUtf8("&Remove Selected"));
    actionRemove_->setShortcut(trUtf8("Delete"));


    menuPlayback_->setTitle(trUtf8("&Playback"));
    menuPlaybackSpeed_->setTitle(trUtf8("Playback Speed"));
    menuAudio_->setTitle(trUtf8("&Audio"));
    menuVideo_->setTitle(trUtf8("&Video"));
    menuWindowSize_->setTitle(trUtf8("Window Size"));
    menuCropFrame_->setTitle(trUtf8("Crop Frame"));
    menuAspectRatio_->setTitle(trUtf8("Aspect Ratio"));
    menuSubs_->setTitle(trUtf8("&Subtitles"));
    menuChapters_->setTitle(trUtf8("Chap&ters"));
  }

  //----------------------------------------------------------------
  // PlayerView::PlayerView
  //
  PlayerView::PlayerView():
    ItemView("PlayerView"),
    actionShowTimeline_(NULL),

    actionPlay_(NULL),
    actionNextChapter_(NULL),
    actionNext_(NULL),
    actionPrev_(NULL),

    actionLoop_(NULL),
    actionSetInPoint_(NULL),
    actionSetOutPoint_(NULL),

    actionFullScreen_(NULL),
    actionFillScreen_(NULL),

    actionCropFrameNone_(NULL),
    actionCropFrame1_33_(NULL),
    actionCropFrame1_60_(NULL),
    actionCropFrame1_78_(NULL),
    actionCropFrame1_85_(NULL),
    actionCropFrame2_40_(NULL),
    actionCropFrame2_35_(NULL),
    actionCropFrameAutoDetect_(NULL),
    actionCropFrameOther_(NULL),

    actionAspectRatioAuto_(NULL),
    actionAspectRatio1_33_(NULL),
    actionAspectRatio1_60_(NULL),
    actionAspectRatio1_78_(NULL),
    actionAspectRatio1_85_(NULL),
    actionAspectRatio2_35_(NULL),
    actionAspectRatio2_40_(NULL),
    actionAspectRatioOther_(NULL),

    actionHalfSize_(NULL),
    actionFullSize_(NULL),
    actionDoubleSize_(NULL),
    actionDecreaseSize_(NULL),
    actionIncreaseSize_(NULL),
    actionShrinkWrap_(NULL),

    actionVerticalScaling_(NULL),
    actionDeinterlace_(NULL),
    actionSkipColorConverter_(NULL),
    actionSkipLoopFilter_(NULL),
    actionSkipNonReferenceFrames_(NULL),
    actionDownmixToStereo_(NULL),

    actionTempo50_(NULL),
    actionTempo60_(NULL),
    actionTempo70_(NULL),
    actionTempo80_(NULL),
    actionTempo90_(NULL),
    actionTempo100_(NULL),
    actionTempo111_(NULL),
    actionTempo125_(NULL),
    actionTempo143_(NULL),
    actionTempo167_(NULL),
    actionTempo200_(NULL),

    actionRemove_(NULL),

    menuPlayback_(NULL),
    menuPlaybackSpeed_(NULL),
    menuAudio_(NULL),
    menuVideo_(NULL),
    menuWindowSize_(NULL),
    menuCropFrame_(NULL),
    menuAspectRatio_(NULL),
    menuSubs_(NULL),
    menuChapters_(NULL),

    // context sensitive menu which includes most relevant actions:
    contextMenu_(NULL),

    // audio/video track selection widgets:
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    subsTrackGroup_(NULL),
    chaptersGroup_(NULL),

    playRateMapper_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    subsTrackMapper_(NULL),
    chapterMapper_(NULL),

    timelineTimer_(this),
    showNextPrev_(BoolRef::constant(false)),
    enableBackArrowButton_(BoolRef::constant(false)),
    enableDeleteFileButton_(BoolRef::constant(false))
  {
    init_actions();
    translate_ui();

    // for scroll-wheel event buffering,
    // used to avoid frequent seeking by small distances:
    scrollWheelTimer_.setSingleShot(true);

    // for re-running auto-crop soon after loading next file in the playlist:
    autocropTimer_.setSingleShot(true);

    // update a bookmark every 10 seconds during playback:
    bookmarkTimer_.setInterval(10000);

    bool ok = true;

    ok = connect(&autocropTimer_, SIGNAL(timeout()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(&bookmarkTimer_, SIGNAL(timeout()),
                 this, SIGNAL(save_bookmark()));
    YAE_ASSERT(ok);

    ok = connect(&timelineTimer_, SIGNAL(timeout()),
                 this, SLOT(sync_ui()));
    YAE_ASSERT(ok);

    ok = connect(&scrollWheelTimer_, SIGNAL(timeout()),
                 this, SLOT(scrollWheelTimerExpired()));
    YAE_ASSERT(ok);

    style_.reset(new PlayerStyle("player_style", *this));
  }

  //----------------------------------------------------------------
  // PlayerView::~PlayerView
  //
  PlayerView::~PlayerView()
  {
    delete menuPlayback_;
    delete menuPlaybackSpeed_;
    delete menuAudio_;
    delete menuVideo_;
    delete menuWindowSize_;
    delete menuCropFrame_;
    delete menuAspectRatio_;
    delete menuSubs_;
    delete menuChapters_;
    delete contextMenu_;
  }

  //----------------------------------------------------------------
  // PlayerView::setStyle
  //
  void
  PlayerView::setStyle(const yae::shared_ptr<ItemViewStyle, Item> & style)
  {
    style_ = style;
  }

  //----------------------------------------------------------------
  // PlayerView::setContext
  //
  void
  PlayerView::setContext(const yae::shared_ptr<IOpenGLContext> & context)
  {
    ItemView::setContext(context);

    player_.reset(new PlayerItem("player"));
    PlayerItem & player = *player_;

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
    // timeline.toggle_playlist_.reset(&yae::toggle_playlist, this);
    timeline.toggle_playback_.reset(&yae::toggle_playback, this);

    if (enableBackArrowButton_.get())
    {
      timeline.back_arrow_cb_.reset(&yae::back_arrow_cb, this);
    }

    timeline.toggle_fullscreen_ = this->toggle_fullscreen_;
    timeline.layout();

    bool ok = true;

    ok = connect(actionSetInPoint_, SIGNAL(triggered()),
                 &timeline_model(), SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionSetOutPoint_, SIGNAL(triggered()),
                 &timeline_model(), SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(&timeline_model(), SIGNAL(clockStopped(const SharedClock &)),
                 this, SLOT(playbackFinished(const SharedClock &)));
    YAE_ASSERT(ok);
#if 0
    ok = connect(player_.get(), SIGNAL(maybe_animate_opacity()),
                 timeline_.get(), SLOT(maybeAnimateOpacity()));
    YAE_ASSERT(ok);
#endif

    // apply default settings:
    playbackLoop();

    // apply saved settings:
    playbackColorConverter();
    playbackLoopFilter();
    playbackNonReferenceFrames();
    playbackDeinterlace();
    playbackShowTimeline();
    audioDownmixToStereo();
  }

  //----------------------------------------------------------------
  // PlayerView::setEnabled
  //
  void
  PlayerView::setEnabled(bool enable)
  {
    if (!style_ || isEnabled() == enable)
    {
      return;
    }

    TMakeCurrentContext currentContext(*context());

    Item & root = *root_;
    root.children_.clear();
    root.anchors_.left_ = ItemRef::constant(0.0);
    root.anchors_.top_ = ItemRef::constant(0.0);
    root.width_ = ItemRef::constant(w_);
    root.height_ = ItemRef::constant(h_);

    if (enable)
    {
      layout(*this, *style_, root);
      timelineTimer_.start(1000);
    }
    else
    {
      timelineTimer_.stop();
    }

    root.uncache();
    uncache_.clear();

    ItemView::setEnabled(enable);
  }

  //----------------------------------------------------------------
  // PlayerView::event
  //
  bool
  PlayerView::event(QEvent * e)
  {
    QEvent::Type et = e->type();

    if (et == QEvent::User)
    {
      yae::AutoCropEvent * ac = dynamic_cast<yae::AutoCropEvent *>(e);
      if (ac)
      {
        ac->accept();
        canvas().cropFrame(ac->cropFrame_);
        emit adjust_canvas_height();
        return true;
      }
    }

    return ItemView::event(e);
  }

  //----------------------------------------------------------------
  // PlayerView::processKeyEvent
  //
  bool
  PlayerView::processKeyEvent(Canvas * canvas, QKeyEvent * event)
  {
    if (ItemView::processKeyEvent(canvas, event))
    {
      return true;
    }

    int key = event->key();
    if (key == Qt::Key_N)
    {
      // FIXME: pkoshevoy: must respect item focus
      skipToNextFrame();
    }
    else if (key == Qt::Key_MediaNext ||
             key == Qt::Key_Period ||
             key == Qt::Key_Greater)
    {
      skipForward();
    }
    else if (key == Qt::Key_MediaPrevious ||
             key == Qt::Key_Comma ||
             key == Qt::Key_Less)
    {
      skipBack();
    }
    else if (key == Qt::Key_MediaPlay ||
#if QT_VERSION >= 0x040700
             key == Qt::Key_MediaPause ||
             key == Qt::Key_MediaTogglePlayPause ||
#endif
             key == Qt::Key_MediaStop)
    {
      togglePlayback();
    }
    else
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // PlayerView::processWheelEvent
  //
  bool
  PlayerView::processWheelEvent(Canvas * canvas, QWheelEvent * e)
  {
    double tNow = timeline_model().currentTime();
    if (tNow <= 1e-1)
    {
      // ignore it:
      return ItemView::processWheelEvent(canvas, e);
    }

    // seek back and forth here:
    int delta = e->delta();
    double percent = floor(0.5 + fabs(double(delta)) / 120.0);
    percent = std::max<double>(1.0, percent);
    double offset = percent * ((delta < 0) ? 5.0 : -5.0);

    if (!scrollWheelTimer_.isActive())
    {
      scrollStart_ = tNow;
      scrollOffset_ = 0.0;
    }

    scrollOffset_ += offset;
    scrollWheelTimer_.start(200);
    return true;
  }

  //----------------------------------------------------------------
  // PlayerView::processMouseTracking
  //
  bool
  PlayerView::processMouseTracking(const TVec2D & mousePt)
  {
    if (!this->isEnabled())
    {
      return false;
    }

    if (timeline_)
    {
      timeline_->processMouseTracking(mousePt);
    }

    return true;
  }

  //----------------------------------------------------------------
  // PlayerView::insert_menus
  //
  void
  PlayerView::insert_menus(const IReaderPtr & reader,
                           QMenuBar * menubar,
                           QAction * before)
  {
    menubar->insertAction(before, menuPlayback_->menuAction());
    menubar->insertAction(before, menuAudio_->menuAction());
    menubar->insertAction(before, menuVideo_->menuAction());
    menubar->insertAction(before, menuSubs_->menuAction());

    std::size_t numChapters = reader ? reader->countChapters() : 0;
    if (numChapters)
    {
      menubar->insertAction(before, menuChapters_->menuAction());
    }
  }

  //----------------------------------------------------------------
  // PlayerView::autocrop_cb
  //
  TVideoFramePtr
  PlayerView::autocrop_cb(void * callbackContext,
                          const TCropFrame & cf,
                          bool detectionFinished)
  {
    PlayerView * view = (PlayerView *)callbackContext;
    IReader * reader = view->get_reader();
    std::size_t videoTrack = reader ? reader->getSelectedVideoTrackIndex() : 0;
    std::size_t numVideoTracks = reader ? reader->getNumberOfVideoTracks() : 0;

    if (detectionFinished)
    {
      qApp->postEvent(view, new AutoCropEvent(cf), Qt::HighEventPriority);
    }
    else if (view->is_playback_paused() ||
             view->player_->video()->isPaused() ||
             videoTrack >= numVideoTracks)
    {
      // use the same frame again:
      return view->canvas().currentFrame();
    }

    return TVideoFramePtr();
  }

  //----------------------------------------------------------------
  // PlayerView::sync_ui
  //
  void
  PlayerView::sync_ui()
  {
    IReader * reader = get_reader();
    TimelineModel & timeline = timeline_model();
    timeline.updateDuration(reader);
    requestRepaint();
  }

  //----------------------------------------------------------------
  // PlayerView::is_playback_paused
  //
  bool
  PlayerView::is_playback_paused() const
  {
    return player_->paused();
  }

  //----------------------------------------------------------------
  // PlayerView::playback
  //
  void
  PlayerView::playback(const IReaderPtr & reader_ptr,
                       const IBookmark * bookmark,
                       bool start_from_zero_time)
  {
    TimelineModel & timeline = timeline_model();
    timeline.startFromZero(start_from_zero_time);

    std::vector<TTrackInfo>  audioInfo;
    std::vector<AudioTraits> audioTraits;
    std::vector<TTrackInfo>  videoInfo;
    std::vector<VideoTraits> videoTraits;
    std::vector<TTrackInfo>  subsInfo;
    std::vector<TSubsFormat> subsFormat;

    adjustMenuActions(reader_ptr,
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat);

    player_->playback(reader_ptr,
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat,
                      bookmark);

    if (reader_ptr)
    {
      IReader & reader = *reader_ptr;

      int ai = int(reader.getSelectedAudioTrackIndex());
      audioTrackGroup_->actions().at(ai)->setChecked(true);


      int vi = int(reader.getSelectedVideoTrackIndex());
      videoTrackGroup_->actions().at(vi)->setChecked(true);

      int nsubs = int(reader.subsCount());
      unsigned int cc = reader.getRenderCaptions();

      if (cc)
      {
        int index = (int)nsubs + cc - 1;
        subsTrackGroup_->actions().at(index)->setChecked(true);
      }
      else
      {
        int si = 0;
        for (; si < nsubs && !reader.getSubsRender(si); si++)
        {}

        if (si < nsubs)
        {
          subsTrackGroup_->actions().at(si)->setChecked(true);
        }
        else
        {
          int index = subsTrackGroup_->actions().size() - 1;
          subsTrackGroup_->actions().at(index)->setChecked(true);
        }
      }

      TimelineItem & timeline = *timeline_;
      if (videoInfo.empty())
      {
        timeline.frame_crop_cb_.reset();
        timeline.aspect_ratio_cb_.reset();
        timeline.subtt_track_cb_.reset();
        timeline.delete_file_cb_.reset();
      }
      else
      {
        timeline.frame_crop_cb_.reset(&yae::select_frame_crop_cb, this);
        timeline.aspect_ratio_cb_.reset(&yae::select_aspect_ratio_cb, this);
        timeline.subtt_track_cb_.reset(&yae::select_subtt_track_cb, this);

        if (enableDeleteFileButton_.get())
        {
          timeline.delete_file_cb_.reset(&yae::delete_playing_file_cb, this);
        }
      }

      if (videoInfo.size() < 2)
      {
        timeline.video_track_cb_.reset();
      }
      else
      {
        timeline.video_track_cb_.reset(&yae::select_video_track_cb, this);
      }

      if (audioInfo.size() < 2)
      {
        timeline.audio_track_cb_.reset();
      }
      else
      {
        timeline.audio_track_cb_.reset(&yae::select_audio_track_cb, this);
      }
    }

    if (actionCropFrameAutoDetect_->isChecked())
    {
      autocropTimer_.start(1900);
    }
    else
    {
      QTimer::singleShot(1900, this, SIGNAL(adjust_canvas_height()));
    }

    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();

    if (!is_playback_paused())
    {
      timeline_->forceAnimateControls();
      actionPlay_->setText(tr("Pause"));

      bookmarkTimer_.start();
    }
    else
    {
      timeline_->maybeAnimateControls();
      actionPlay_->setText(tr("Play"));

      bookmarkTimer_.stop();
    }

    emit fixup_next_prev();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatioAuto
  //
  void
  PlayerView::playbackAspectRatioAuto()
  {
    canvas().overrideDisplayAspectRatio(0.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio2_40
  //
  void
  PlayerView::playbackAspectRatio2_40()
  {
    canvas().overrideDisplayAspectRatio(2.40);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio2_35
  //
  void
  PlayerView::playbackAspectRatio2_35()
  {
    canvas().overrideDisplayAspectRatio(2.35);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio1_85
  //
  void
  PlayerView::playbackAspectRatio1_85()
  {
    canvas().overrideDisplayAspectRatio(1.85);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio1_78
  //
  void
  PlayerView::playbackAspectRatio1_78()
  {
    canvas().overrideDisplayAspectRatio(16.0 / 9.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio1_60
  //
  void
  PlayerView::playbackAspectRatio1_60()
  {
    canvas().overrideDisplayAspectRatio(1.6);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackAspectRatio1_33
  //
  void
  PlayerView::playbackAspectRatio1_33()
  {
    canvas().overrideDisplayAspectRatio(4.0 / 3.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrameNone
  //
  void
  PlayerView::playbackCropFrameNone()
  {
    autocropTimer_.stop();
    canvas().cropAutoDetectStop();
    canvas().cropFrame(0.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame2_40
  //
  void
  PlayerView::playbackCropFrame2_40()
  {
    canvas().cropFrame(2.40);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame2_35
  //
  void
  PlayerView::playbackCropFrame2_35()
  {
    canvas().cropFrame(2.35);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame1_85
  //
  void
  PlayerView::playbackCropFrame1_85()
  {
    canvas().cropFrame(1.85);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame1_78
  //
  void
  PlayerView::playbackCropFrame1_78()
  {
    canvas().cropFrame(16.0 / 9.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame1_60
  //
  void
  PlayerView::playbackCropFrame1_60()
  {
    canvas().cropFrame(1.6);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrame1_33
  //
  void
  PlayerView::playbackCropFrame1_33()
  {
    canvas().cropFrame(4.0 / 3.0);
    emit adjust_canvas_height();
  }

  //----------------------------------------------------------------
  // PlayerView::playbackCropFrameAutoDetect
  //
  void
  PlayerView::playbackCropFrameAutoDetect()
  {
    canvas().cropAutoDetect(this, &(PlayerView::autocrop_cb));
  }

  //----------------------------------------------------------------
  // PlayerView::playbackLoop
  //
  void
  PlayerView::playbackLoop()
  {
    bool loop_playback = actionLoop_->isChecked();
    player_->set_loop_playback(loop_playback);
  }

  //----------------------------------------------------------------
  // PlayerView::playbackColorConverter
  //
  void
  PlayerView::playbackColorConverter()
  {
    bool skip = actionSkipColorConverter_->isChecked();
    saveBooleanSetting(kSkipColorConverter, skip);
    player_->skip_color_converter(skip);
  }

  //----------------------------------------------------------------
  // PlayerView::playbackLoopFilter
  //
  void
  PlayerView::playbackLoopFilter()
  {
    bool skip = actionSkipLoopFilter_->isChecked();
    saveBooleanSetting(kSkipLoopFilter, skip);
    player_->skip_loopfilter(skip);
  }

  //----------------------------------------------------------------
  // PlayerView::playbackNonReferenceFrames
  //
  void
  PlayerView::playbackNonReferenceFrames()
  {
    bool skip = actionSkipNonReferenceFrames_->isChecked();
    saveBooleanSetting(kSkipNonReferenceFrames, skip);
    player_->skip_nonref_frames(skip);
  }

  //----------------------------------------------------------------
  // PlayerView::playbackDeinterlace
  //
  void
  PlayerView::playbackDeinterlace()
  {
    bool deint = actionDeinterlace_->isChecked();
    saveBooleanSetting(kDeinterlaceFrames, deint);
    player_->set_deinterlace(deint);
  }

  //----------------------------------------------------------------
  // PlayerView::playbackSetTempo
  //
  void
  PlayerView::playbackSetTempo(int percent)
  {
    QAction * found =
      qobject_cast<QAction *>(playRateMapper_->mapping(percent));

    if (found)
    {
      if (!found->isChecked())
      {
        found->setChecked(true);
      }

      double tempo = double(percent) / 100.0;
      player_->set_playback_tempo(tempo);
    }
  }

  //----------------------------------------------------------------
  // PlayerView::playbackShowTimeline
  //
  void
  PlayerView::playbackShowTimeline()
  {
    bool show_timeline = actionShowTimeline_->isChecked();
    saveBooleanSetting(kShowTimeline, show_timeline);
    timeline_->showTimeline(show_timeline);
  }

  //----------------------------------------------------------------
  // PlayerView::audioDownmixToStereo
  //
  void
  PlayerView::audioDownmixToStereo()
  {
    bool downmix = actionDownmixToStereo_->isChecked();
    saveBooleanSetting(kDownmixToStereo, downmix);
    player_->set_downmix_to_stereo(downmix);
  }

  //----------------------------------------------------------------
  // PlayerView::audioSelectTrack
  //
  void
  PlayerView::audioSelectTrack(int index)
  {
    PlayerItem::Tracks tracks;
    if (player_->audio_select_track(std::size_t(index), tracks))
    {
      // update video track selection:
      {
        QList<QAction *> actions = videoTrackGroup_->actions();
        actions[int(tracks.video_)]->setChecked(true);
      }

      // update subtitles selection:
      {
        QList<QAction *> actions = subsTrackGroup_->actions();
        actions[int(tracks.subtt_)]->setChecked(true);
      }

      // QTimer::singleShot(1900, this, SIGNAL(adjust_canvas_height()));
    }
  }

  //----------------------------------------------------------------
  // PlayerView::videoSelectTrack
  //
  void
  PlayerView::videoSelectTrack(int index)
  {
    PlayerItem::Tracks tracks;
    if (player_->video_select_track(std::size_t(index), tracks))
    {
      // update audio track selection:
      {
        QList<QAction *> actions = audioTrackGroup_->actions();
        actions[int(tracks.audio_)]->setChecked(true);
      }

      // update subtitles selection:
      {
        QList<QAction *> actions = subsTrackGroup_->actions();
        actions[int(tracks.subtt_)]->setChecked(true);
      }

      QTimer::singleShot(1900, this, SIGNAL(adjust_canvas_height()));
    }
  }

  //----------------------------------------------------------------
  // PlayerView::subsSelectTrack
  //
  void
  PlayerView::subsSelectTrack(int index)
  {
    PlayerItem::Tracks tracks;
    if (player_->subtt_select_track(std::size_t(index), tracks))
    {
      // update video track selection:
      {
        QList<QAction *> actions = videoTrackGroup_->actions();
        actions[int(tracks.video_)]->setChecked(true);
      }

      // update audio track selection:
      {
        QList<QAction *> actions = audioTrackGroup_->actions();
        actions[int(tracks.audio_)]->setChecked(true);
      }

      // QTimer::singleShot(1900, this, SIGNAL(adjust_canvas_height()));
    }
  }

  //----------------------------------------------------------------
  // PlayerView::updateChaptersMenu
  //
  void
  PlayerView::updateChaptersMenu()
  {
    QList<QAction *> actions = chaptersGroup_->actions();
    const std::size_t numActions = actions.size();
    const std::size_t chapterIdx = player_->get_current_chapter();

    // check-mark the current chapter:
    SignalBlocker blockSignals(chapterMapper_);

    for (std::size_t i = 0; i < numActions; i++)
    {
      QAction * chapterAction = actions[(int)i];
      chapterAction->setChecked(chapterIdx == i);
    }
  }

  //----------------------------------------------------------------
  // PlayerView::skipToNextChapter
  //
  void
  PlayerView::skipToNextChapter()
  {
    if (player_->skip_to_next_chapter())
    {
      return;
    }

    // last chapter, skip to next playlist item:
    emit playback_next();
  }

  //----------------------------------------------------------------
  // PlayerView::skipToChapter
  //
  void
  PlayerView::skipToChapter(int index)
  {
    IReader * reader = get_reader();
    if (!reader)
    {
      return;
    }

    TChapter ch;
    bool ok = reader->getChapterInfo(index, ch);
    YAE_ASSERT(ok);

    double t0_sec = ch.t0_sec();
    timeline_model().seekTo(t0_sec);
  }

  //----------------------------------------------------------------
  // PlayerView::skipToNextFrame
  //
  void
  PlayerView::skipToNextFrame()
  {
    player_->skip_to_next_frame();
  }

  //----------------------------------------------------------------
  // PlayerView::skipForward
  //
  void
  PlayerView::skipForward()
  {
    player_->skip_forward();
    timeline_->maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // PlayerView::skipBack
  //
  void
  PlayerView::skipBack()
  {
    player_->skip_back();
    timeline_->maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // PlayerView::scrollWheelTimerExpired
  //
  void
  PlayerView::scrollWheelTimerExpired()
  {
    double seconds = scrollStart_ + scrollOffset_;

    bool isLooping = actionLoop_->isChecked();
    if (isLooping)
    {
      double t0 = timeline_model().timelineStart();
      double dt = timeline_model().timelineDuration();
      seconds = t0 + fmod(seconds - t0, dt);
    }

    timeline_model().seekTo(seconds);
    timeline_->maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // PlayerView::cropped
  //
  void
  PlayerView::cropped(const TVideoFramePtr & frame, const TCropFrame & crop)
  {
    (void) frame;
    canvas().cropFrame(crop);
  }

  //----------------------------------------------------------------
  // PlayerView::stopPlayback
  //
  void
  PlayerView::stopPlayback()
  {
    bookmarkTimer_.stop();
    player_->playback_stop();
    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();
    timeline_->maybeAnimateControls();
    actionPlay_->setText(tr("Play"));
  }

  //----------------------------------------------------------------
  // PlayerView::togglePlayback
  //
  void
  PlayerView::togglePlayback()
  {
    if (!isEnabled())
    {
      return;
    }

    player_->toggle_playback();

    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();

    if (!is_playback_paused())
    {
      timeline_->forceAnimateControls();
      actionPlay_->setText(tr("Pause"));

      bookmarkTimer_.start();
    }
    else
    {
      timeline_->maybeAnimateControls();
      actionPlay_->setText(tr("Play"));

      bookmarkTimer_.stop();

      if (get_reader())
      {
        emit save_bookmark();
      }
    }
  }

  //----------------------------------------------------------------
  // PlayerView::playbackFinished
  //
  void
  PlayerView::playbackFinished(const SharedClock & c)
  {
    if (!timeline_model().sharedClock().sharesCurrentTimeWith(c))
    {
      // ignoring stale playbackFinished
      return;
    }

    TTime t0(0, 0);
    TTime t1(0, 0);
    IReader * reader = get_reader();
    yae::get_timeline(reader, t0, t1);

    emit playback_finished(t1);

    stopPlayback();
  }

  //----------------------------------------------------------------
  // PlayerView::togglePlaylist
  //
  void
  PlayerView::togglePlaylist()
  {
    emit toggle_playlist();
  }

  //----------------------------------------------------------------
  // PlayerView::onBackArrow
  //
  void
  PlayerView::onBackArrow()
  {
    emit on_back_arrow();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerSelectFrameCrop
  //
  void
  PlayerView::triggerSelectFrameCrop()
  {
    emit select_frame_crop();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerSelectAspectRatio
  //
  void
  PlayerView::triggerSelectAspectRatio()
  {
    emit select_aspect_ratio();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerSelectVideoTrack
  //
  void
  PlayerView::triggerSelectVideoTrack()
  {
    emit select_video_track();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerSelectAudioTrack
  //
  void
  PlayerView::triggerSelectAudioTrack()
  {
    emit select_audio_track();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerSelectSubttTrack
  //
  void
  PlayerView::triggerSelectSubttTrack()
  {
    emit select_subtt_track();
  }

  //----------------------------------------------------------------
  // PlayerView::triggerDeletePlayingFile
  //
  void
  PlayerView::triggerDeletePlayingFile()
  {
    emit delete_playing_file();
  }

  //----------------------------------------------------------------
  // addMenuCopyTo
  //
  static void
  addMenuCopyTo(QMenu * dst, QMenu * src)
  {
    dst->addAction(src->menuAction());
  }

  //----------------------------------------------------------------
  // PlayerView::populateContextMenu
  //
  void
  PlayerView::populateContextMenu()
  {
    IReader * reader = get_reader();

    std::size_t numVideoTracks =
      reader ? reader->getNumberOfVideoTracks() : 0;

    std::size_t numAudioTracks =
      reader ? reader->getNumberOfAudioTracks() : 0;

    std::size_t numSubtitles =
      reader ? reader->subsCount() : 0;

    std::size_t numChapters =
      reader ? reader->countChapters() : 0;

    // populate the context menu:
    contextMenu_->clear();
    contextMenu_->addAction(actionPlay_);

#if 0
    contextMenu_->addSeparator();
    contextMenu_->addAction(actionPrev_);
    contextMenu_->addAction(actionNext_);

    if (reader)
    {
      contextMenu_->addSeparator();
      contextMenu_->addAction(actionRemove_);
    }
#endif

    contextMenu_->addSeparator();
    contextMenu_->addAction(actionLoop_);
    contextMenu_->addAction(actionSetInPoint_);
    contextMenu_->addAction(actionSetOutPoint_);
    contextMenu_->addAction(actionShowTimeline_);

    contextMenu_->addSeparator();
    contextMenu_->addAction(actionShrinkWrap_);
    contextMenu_->addAction(actionFullScreen_);
    contextMenu_->addAction(actionFillScreen_);
    addMenuCopyTo(contextMenu_, menuPlaybackSpeed_);

    contextMenu_->addSeparator();

    if (numVideoTracks || numAudioTracks)
    {
      if (numAudioTracks)
      {
        addMenuCopyTo(contextMenu_, menuAudio_);
      }

      if (numVideoTracks)
      {
        addMenuCopyTo(contextMenu_, menuVideo_);
      }

      if (numSubtitles || true)
      {
        addMenuCopyTo(contextMenu_, menuSubs_);
      }

      if (numChapters)
      {
        addMenuCopyTo(contextMenu_, menuChapters_);
      }
    }
  }

  //----------------------------------------------------------------
  // PlayerView::adjustMenuActions
  //
  void
  PlayerView::adjustMenuActions()
  {
    std::vector<TTrackInfo>  audioInfo;
    std::vector<AudioTraits> audioTraits;
    std::vector<TTrackInfo>  videoInfo;
    std::vector<VideoTraits> videoTraits;
    std::vector<TTrackInfo>  subsInfo;
    std::vector<TSubsFormat> subsFormat;

    adjustMenuActions(get_reader(),
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat);

  }

  //----------------------------------------------------------------
  // PlayerView::adjustMenuActions
  //
  void
  PlayerView::adjustMenuActions(IReader * reader,
                                std::vector<TTrackInfo> &  audioInfo,
                                std::vector<AudioTraits> & audioTraits,
                                std::vector<TTrackInfo> &  videoInfo,
                                std::vector<VideoTraits> & videoTraits,
                                std::vector<TTrackInfo> &  subsInfo,
                                std::vector<TSubsFormat> & subsFormat)
  {
    std::size_t numVideoTracks =
      reader ? reader->getNumberOfVideoTracks() : 0;

    std::size_t numAudioTracks =
      reader ? reader->getNumberOfAudioTracks() : 0;

    std::size_t numSubtitles =
      reader ? reader->subsCount() : 0;

    std::size_t numChapters =
      reader ? reader->countChapters() : 0;

    if (audioTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = audioTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuAudio_->removeAction(action);
      }
    }

    if (videoTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = videoTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuVideo_->removeAction(action);
      }
    }

    if (subsTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = subsTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuSubs_->removeAction(action);
      }
    }

    if (chaptersGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = chaptersGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuChapters_->removeAction(action);
      }
    }

    // cleanup action groups:
    delete audioTrackGroup_;
    audioTrackGroup_ = new QActionGroup(this);

    delete videoTrackGroup_;
    videoTrackGroup_ = new QActionGroup(this);

    delete subsTrackGroup_;
    subsTrackGroup_ = new QActionGroup(this);

    delete chaptersGroup_;
    chaptersGroup_ = new QActionGroup(this);

    // cleanup signal mappers:
    bool ok = true;

    delete audioTrackMapper_;
    audioTrackMapper_ = new QSignalMapper(this);

    ok = connect(audioTrackMapper_, SIGNAL(mapped(int)),
                 this, SLOT(audioSelectTrack(int)));
    YAE_ASSERT(ok);

    delete videoTrackMapper_;
    videoTrackMapper_ = new QSignalMapper(this);

    ok = connect(videoTrackMapper_, SIGNAL(mapped(int)),
                 this, SLOT(videoSelectTrack(int)));
    YAE_ASSERT(ok);

    delete subsTrackMapper_;
    subsTrackMapper_ = new QSignalMapper(this);

    ok = connect(subsTrackMapper_, SIGNAL(mapped(int)),
                 this, SLOT(subsSelectTrack(int)));
    YAE_ASSERT(ok);

    delete chapterMapper_;
    chapterMapper_ = new QSignalMapper(this);

    ok = connect(chapterMapper_, SIGNAL(mapped(int)),
                 this, SLOT(skipToChapter(int)));
    YAE_ASSERT(ok);


    audioInfo = std::vector<TTrackInfo>(numAudioTracks);
    audioTraits = std::vector<AudioTraits>(numAudioTracks);

    for (unsigned int i = 0; i < numAudioTracks; i++)
    {
      reader->selectAudioTrack(i);
      QString trackName = tr("Track %1").arg(i + 1);

      TTrackInfo & info = audioInfo[i];
      reader->getSelectedAudioTrackInfo(info);

      if (info.hasLang())
      {
        trackName += tr(" (%1)").arg(QString::fromUtf8(info.lang()));
      }

      if (info.hasName())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.name()));
      }

      AudioTraits & traits = audioTraits[i];
      if (reader->getAudioTraits(traits))
      {
        trackName +=
          tr(", %1 Hz, %2 channels").
          arg(traits.sampleRate_).
          arg(getNumberOfChannels(traits.channelLayout_));
      }

      std::string serviceName = yae::get_program_name(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuAudio_->addAction(trackAction);

      trackAction->setCheckable(true);
      audioTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   audioTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable audio:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuAudio_->addAction(trackAction);

      trackAction->setCheckable(true);
      audioTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   audioTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioTrackMapper_->setMapping(trackAction, int(numAudioTracks));
    }

    videoInfo = std::vector<TTrackInfo>(numVideoTracks);
    videoTraits = std::vector<VideoTraits>(numVideoTracks);

    for (unsigned int i = 0; i < numVideoTracks; i++)
    {
      reader->selectVideoTrack(i);
      QString trackName = tr("Track %1").arg(i + 1);

      TTrackInfo & info = videoInfo[i];
      reader->getSelectedVideoTrackInfo(info);

      if (info.hasName())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.name()));
      }

      VideoTraits & traits = videoTraits[i];
      if (reader->getVideoTraits(traits))
      {
        double par = (traits.pixelAspectRatio_ != 0.0 &&
                      traits.pixelAspectRatio_ != 1.0 ?
                      traits.pixelAspectRatio_ : 1.0);

        unsigned int w = (unsigned int)(0.5 + par * traits.visibleWidth_);
        trackName +=
          tr(", %1 x %2, %3 fps").
          arg(w).
          arg(traits.visibleHeight_).
          arg(traits.frameRate_);

        if (traits.cameraRotation_)
        {
          static const char * degree_utf8 = "\xc2""\xb0";
          trackName +=
            tr(", rotated %1%2").
            arg(traits.cameraRotation_).
            arg(QString::fromUtf8(degree_utf8));
        }
      }

      std::string serviceName = yae::get_program_name(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuVideo_->addAction(trackAction);

      trackAction->setCheckable(true);
      videoTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable video:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuVideo_->addAction(trackAction);

      trackAction->setCheckable(true);
      videoTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, int(numVideoTracks));
    }

    subsInfo = std::vector<TTrackInfo>(numSubtitles);
    subsFormat = std::vector<TSubsFormat>(numSubtitles);

    for (unsigned int i = 0; i < numSubtitles; i++)
    {
      QString trackName = tr("Track %1").arg(i + 1);

      TTrackInfo & info = subsInfo[i];
      TSubsFormat & subsFmt = subsFormat[i];
      subsFmt = reader->subsInfo(i, info);

      if (info.hasLang())
      {
        trackName += tr(" (%1)").arg(QString::fromUtf8(info.lang()));
      }

      if (info.hasName())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.name()));
      }

      if (subsFmt != kSubsNone)
      {
        const char * label = getSubsFormatLabel(subsFmt);
        trackName += tr(", %1").arg(QString::fromUtf8(label));
      }

      std::string serviceName = yae::get_program_name(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuSubs_->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to show closed captions:
    for (unsigned int i = 0; i < 4; i++)
    {
      QAction * trackAction =
        new QAction(tr("Closed Captions (CC%1)").arg(i + 1), this);
      menuSubs_->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(numSubtitles + i + 1));
    }

    // add an option to disable subs:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuSubs_->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(numSubtitles));
    }

    // update the chapter menu:
    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      ok = reader->getChapterInfo(i, ch);
      YAE_ASSERT(ok);

      double t0_sec = ch.t0_sec();
      QTime t0 = QTime(0, 0).addMSecs((int)(0.5 + t0_sec * 1000.0));

      QString name =
        tr("%1   %2").
        arg(t0.toString("hh:mm:ss")).
        arg(QString::fromUtf8(ch.name_.c_str()));

      QAction * chapterAction = new QAction(name, this);
      menuChapters_->addAction(chapterAction);

      chapterAction->setCheckable(true);
      chaptersGroup_->addAction(chapterAction);

      ok = connect(chapterAction, SIGNAL(triggered()),
                   chapterMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      chapterMapper_->setMapping(chapterAction, (int)i);
    }

    bool isSeekable = reader ? reader->isSeekable() : false;
    actionSetInPoint_->setEnabled(isSeekable);
    actionSetOutPoint_->setEnabled(isSeekable);
  }

  //----------------------------------------------------------------
  // PlayerView::layout
  //
  void
  PlayerView::layout(PlayerView & view,
                     const ItemViewStyle & style,
                     Item & root)
  {
    Item & hidden = root.addHidden(new Item("hidden"));
    hidden.width_ = hidden.
      addExpr(style_item_ref(view, &ItemViewStyle::unit_size_));

    // add style to root item, so that it could be uncached
    // together with all the view:
    root.addHidden(view.style_);

    PlayerItem & player = root.add(view.player_);
    player.anchors_.fill(root);

    TimelineItem & timeline = root.add(view.timeline_);
    timeline.anchors_.fill(root);
  }

}
