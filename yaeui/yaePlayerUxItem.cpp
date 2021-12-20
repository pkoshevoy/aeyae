// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sun Nov  7 15:27:18 MST 2021
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// Qt:
#include <QApplication>
#include <QProcess>

// yaeui:
#ifdef __APPLE__
#include "yaeAppleUtils.h"
#endif
#include "yaePlayerStyle.h"
#include "yaePlayerUxItem.h"
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
    IsPlaybackPaused(const PlayerUxItem & player_ux):
      player_ux_(player_ux)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = player_ux_.is_playback_paused();
    }

    const PlayerUxItem & player_ux_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_toggle_playback);
  }

  //----------------------------------------------------------------
  // back_arrow_cb
  //
  static void
  back_arrow_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_on_back_arrow);
  }

  //----------------------------------------------------------------
  // select_frame_crop_cb
  //
  static void
  select_frame_crop_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_select_frame_crop);
  }

  //----------------------------------------------------------------
  // select_aspect_ratio_cb
  //
  static void
  select_aspect_ratio_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_select_aspect_ratio);
  }

  //----------------------------------------------------------------
  // select_video_track_cb
  //
  static void
  select_video_track_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_select_video_track);
  }

  //----------------------------------------------------------------
  // select_audio_track_cb
  //
  static void
  select_audio_track_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_select_audio_track);
  }

  //----------------------------------------------------------------
  // select_subtt_track_cb
  //
  static void
  select_subtt_track_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_select_subtt_track);
  }

  //----------------------------------------------------------------
  // delete_playing_file_cb
  //
  static void
  delete_playing_file_cb(void * context)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    yae::queue_call(*player_ux, &PlayerUxItem::emit_delete_playing_file);
  }

  //----------------------------------------------------------------
  // context_query_timeline_visible
  //
  static bool
  context_query_timeline_visible(void * context, bool & timeline_visible)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)context;
    timeline_visible = player_ux->actionShowTimeline_->isChecked();
    return true;
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_actions
  //
  void
  PlayerUxItem::init_actions()
  {
    actionShowTimeline_ = yae::add<QAction>(this, "actionShowTimeline");
    actionShowTimeline_->setCheckable(true);
    actionShowTimeline_->setChecked(false);
    actionShowTimeline_->setShortcutContext(Qt::ApplicationShortcut);

    actionShowInFinder_ = yae::add<QAction>(this, "actionShowInFinder");
    actionShowInFinder_->setCheckable(false);


    actionPlay_ = yae::add<QAction>(this, "actionPlay");
    actionPlay_->setShortcutContext(Qt::ApplicationShortcut);

    actionNextChapter_ = yae::add<QAction>(this, "actionNextChapter");
    actionNextChapter_->setShortcutContext(Qt::ApplicationShortcut);



    actionLoop_ = yae::add<QAction>(this, "actionLoop");
    actionLoop_->setCheckable(true);
    actionLoop_->setShortcutContext(Qt::ApplicationShortcut);

    actionSetInPoint_ = yae::add<QAction>(this, "actionSetInPoint");
    actionSetOutPoint_ = yae::add<QAction>(this, "actionSetOutPoint");



    actionFullScreen_ = yae::add<QAction>(this, "actionFullScreen");
    actionFullScreen_->setCheckable(true);
    actionFullScreen_->setShortcutContext(Qt::ApplicationShortcut);


    actionFillScreen_ = yae::add<QAction>(this, "actionFillScreen");
    actionFillScreen_->setCheckable(true);
    actionFillScreen_->setShortcutContext(Qt::ApplicationShortcut);



    actionCropFrameNone_ = yae::add<QAction>(this, "actionCropFrameNone");
    actionCropFrameNone_->setCheckable(true);
    actionCropFrameNone_->setChecked(true);
    actionCropFrameNone_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_33_ = yae::add<QAction>(this, "actionCropFrame1_33");
    actionCropFrame1_33_->setCheckable(true);
    actionCropFrame1_33_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_60_ = yae::add<QAction>(this, "actionCropFrame1_60");
    actionCropFrame1_60_->setCheckable(true);

    actionCropFrame1_78_ = yae::add<QAction>(this, "actionCropFrame1_78");
    actionCropFrame1_78_->setCheckable(true);
    actionCropFrame1_78_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame1_85_ = yae::add<QAction>(this, "actionCropFrame1_85");
    actionCropFrame1_85_->setCheckable(true);
    actionCropFrame1_85_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrame2_35_ = yae::add<QAction>(this, "actionCropFrame2_35");
    actionCropFrame2_35_->setCheckable(true);

    actionCropFrame2_40_ = yae::add<QAction>(this, "actionCropFrame2_40");
    actionCropFrame2_40_->setCheckable(true);
    actionCropFrame2_40_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrameOther_ = yae::add<QAction>(this, "actionCropFrameOther");
    actionCropFrameOther_->setCheckable(true);
    actionCropFrameOther_->setShortcutContext(Qt::ApplicationShortcut);

    actionCropFrameAutoDetect_ =
      yae::add<QAction>(this, "actionCropFrameAutoDetect");
    actionCropFrameAutoDetect_->setCheckable(true);
    actionCropFrameAutoDetect_->setShortcutContext(Qt::ApplicationShortcut);



    actionAspectRatioAuto_ = yae::add<QAction>(this, "actionAspectRatioAuto");
    actionAspectRatioAuto_->setCheckable(true);

    actionAspectRatio1_33_ = yae::add<QAction>(this, "actionAspectRatio1_33");
    actionAspectRatio1_33_->setCheckable(true);

    actionAspectRatio1_60_ = yae::add<QAction>(this, "actionAspectRatio1_60");
    actionAspectRatio1_60_->setCheckable(true);

    actionAspectRatio1_78_ = yae::add<QAction>(this, "actionAspectRatio1_78");
    actionAspectRatio1_78_->setCheckable(true);

    actionAspectRatio1_85_ = yae::add<QAction>(this, "actionAspectRatio1_85");
    actionAspectRatio1_85_->setCheckable(true);

    actionAspectRatio2_35_ = yae::add<QAction>(this, "actionAspectRatio2_35");
    actionAspectRatio2_35_->setCheckable(true);

    actionAspectRatio2_40_ = yae::add<QAction>(this, "actionAspectRatio2_40");
    actionAspectRatio2_40_->setCheckable(true);

    actionAspectRatioOther_ = yae::add<QAction>(this, "actionAspectRatioOther");
    actionAspectRatioOther_->setCheckable(true);




    actionHalfSize_ = yae::add<QAction>(this, "actionHalfSize");
    actionHalfSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionFullSize_ = yae::add<QAction>(this, "actionFullSize");
    actionFullSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionDoubleSize_ = yae::add<QAction>(this, "actionDoubleSize");
    actionDoubleSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionDecreaseSize_ = yae::add<QAction>(this, "actionDecreaseSize");
    actionDecreaseSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionIncreaseSize_ = yae::add<QAction>(this, "actionIncreaseSize");
    actionIncreaseSize_->setShortcutContext(Qt::ApplicationShortcut);

    actionShrinkWrap_ = yae::add<QAction>(this, "actionShrinkWrap");
    actionShrinkWrap_->setShortcutContext(Qt::ApplicationShortcut);




    actionVerticalScaling_ = yae::add<QAction>(this, "actionVerticalScaling");
    actionVerticalScaling_->setCheckable(true);

    actionDeinterlace_ = yae::add<QAction>(this, "actionDeinterlace");
    actionDeinterlace_->setCheckable(true);

    actionSkipColorConverter_ =
      yae::add<QAction>(this, "actionSkipColorConverter");
    actionSkipColorConverter_->setCheckable(true);

    actionSkipLoopFilter_ = yae::add<QAction>(this, "actionSkipLoopFilter");
    actionSkipLoopFilter_->setCheckable(true);

    actionSkipNonReferenceFrames_ =
      yae::add<QAction>(this, "actionSkipNonReferenceFrames");
    actionSkipNonReferenceFrames_->setCheckable(true);

    actionDownmixToStereo_ = yae::add<QAction>(this, "actionDownmixToStereo");
    actionDownmixToStereo_->setCheckable(true);



    actionTempo50_ = yae::add<QAction>(this, "actionTempo50");
    actionTempo50_->setCheckable(true);

    actionTempo60_ = yae::add<QAction>(this, "actionTempo60");
    actionTempo60_->setCheckable(true);

    actionTempo70_ = yae::add<QAction>(this, "actionTempo70");
    actionTempo70_->setCheckable(true);

    actionTempo80_ = yae::add<QAction>(this, "actionTempo80");
    actionTempo80_->setCheckable(true);

    actionTempo90_ = yae::add<QAction>(this, "actionTempo90");
    actionTempo90_->setCheckable(true);

    actionTempo100_ = yae::add<QAction>(this, "actionTempo100");
    actionTempo100_->setCheckable(true);

    actionTempo111_ = yae::add<QAction>(this, "actionTempo111");
    actionTempo111_->setCheckable(true);

    actionTempo125_ = yae::add<QAction>(this, "actionTempo125");
    actionTempo125_->setCheckable(true);

    actionTempo143_ = yae::add<QAction>(this, "actionTempo143");
    actionTempo143_->setCheckable(true);

    actionTempo167_ = yae::add<QAction>(this, "actionTempo167");
    actionTempo167_->setCheckable(true);

    actionTempo200_ = yae::add<QAction>(this, "actionTempo200");
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

    ok = connect(actionLoop_, SIGNAL(triggered()),
                 this, SLOT(playbackLoop()));
    YAE_ASSERT(ok);

    ok = connect(actionShowTimeline_, SIGNAL(triggered()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);

    ok = connect(actionShowInFinder_, SIGNAL(triggered()),
                 this, SLOT(playbackShowInFinder()));
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

    ok = connect(menuChapters_, SIGNAL(aboutToShow()),
                 this, SLOT(updateChaptersMenu()));
    YAE_ASSERT(ok);

    ok = connect(actionNextChapter_, SIGNAL(triggered()),
                 this, SLOT(skipToNextChapter()));
    YAE_ASSERT(ok);
 }

  //----------------------------------------------------------------
  // PlayerUxItem::translate_ui
  //
  void
  PlayerUxItem::translate_ui()
  {
    actionShowTimeline_->setText(trUtf8("Show &Timeline"));
    actionShowTimeline_->setShortcut(trUtf8("Ctrl+T"));

#ifdef __APPLE__
    actionShowInFinder_->setText(trUtf8("Show In Finder"));
#elif defined(_WIN32)
    actionShowInFinder_->setText(trUtf8("Show In Explorer"));
#else
    actionShowInFinder_->setText(trUtf8("Show In File Manager"));
#endif


    actionPlay_->setText(trUtf8("Pause"));
    actionPlay_->setShortcut(trUtf8("Space"));

    actionNextChapter_->setText(trUtf8("Skip To &Next Chapter"));
    actionNextChapter_->setShortcut(trUtf8("Ctrl+N"));



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
  // request_toggle_fullscreen
  //
  static void
  request_toggle_fullscreen(void * context)
  {
    PlayerUxItem * pl_ux = (PlayerUxItem *)context;
    pl_ux->requestToggleFullScreen();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::PlayerUxItem
  //
  PlayerUxItem::PlayerUxItem(const char * id, ItemView & view):
    QObject(),
    Item(id),
    view_(view),
    actionShowTimeline_(NULL),
    actionShowInFinder_(NULL),

    actionPlay_(NULL),
    actionNextChapter_(NULL),

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

    scrollStart_(0.0),
    scrollOffset_(0.0),

    enableBackArrowButton_(BoolRef::constant(false)),
    enableDeleteFileButton_(BoolRef::constant(false)),

    renderMode_(Canvas::kScaleToFit),
    xexpand_(1.0),
    yexpand_(1.0)
  {
    init_actions();
    translate_ui();

    // override default toggle_fullscreen function:
    view_.toggle_fullscreen_.reset(&request_toggle_fullscreen, this);

    // for scroll-wheel event buffering,
    // used to avoid frequent seeking by small distances:
    scrollWheelTimer_.setSingleShot(true);

    // for re-running auto-crop soon after loading next file in the playlist:
    autocropTimer_.setSingleShot(true);

    // update a bookmark every 10 seconds during playback:
    bookmarkTimer_.setInterval(10000);

    bool ok = true;

    TMakeCurrentContext currentContext(view_.context().get());
    init_player();
    init_timeline();
    init_frame_crop();
    init_frame_crop_sel();
    init_aspect_ratio_sel();
    init_video_track_sel();
    init_audio_track_sel();
    init_subtt_track_sel();

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
  // PlayerUxItem::~PlayerUxItem
  //
  PlayerUxItem::~PlayerUxItem()
  {
    // clear children first, in order to avoid causing a temporarily
    // dangling DataRefSrc reference to font_size_:
    PlayerUxItem::clear();

    onLoadFrame_.reset();
    subtt_track_sel_->clear();
    audio_track_sel_->clear();
    video_track_sel_->clear();
    aspect_ratio_sel_->clear();
    frame_crop_sel_->clear();
    frame_crop_->clear();
    timeline_->clear();
    player_->clear();

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
  // PlayerUxItem::init_player
  //
  void
  PlayerUxItem::init_player()
  {
    player_.reset(new PlayerItem("player"));
    PlayerItem & player = Item::add<PlayerItem>(player_);
    player.anchors_.fill(*this);
    player.setCanvasDelegate(view_.delegate());
    player.setVisible(true);

    bool ok = true;
    ok = connect(&timelineTimer_, SIGNAL(timeout()),
                 this, SLOT(sync_ui()));
    YAE_ASSERT(ok);

    ok = connect(&bookmarkTimer_, SIGNAL(timeout()),
                 this, SIGNAL(save_bookmark()));
    YAE_ASSERT(ok);

    ok = connect(actionVerticalScaling_, SIGNAL(triggered(bool)),
                 this, SLOT(playbackVerticalScaling(bool)));
    YAE_ASSERT(ok);

    ok = connect(actionShrinkWrap_, SIGNAL(triggered()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);

    ok = connect(actionFullScreen_, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(actionFillScreen_, SIGNAL(triggered()),
                 this, SLOT(playbackFillScreen()));
    YAE_ASSERT(ok);

    ok = connect(actionHalfSize_, SIGNAL(triggered()),
                 this, SLOT(windowHalfSize()));
    YAE_ASSERT(ok);

    ok = connect(actionFullSize_, SIGNAL(triggered()),
                 this, SLOT(windowFullSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDoubleSize_, SIGNAL(triggered()),
                 this, SLOT(windowDoubleSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDecreaseSize_, SIGNAL(triggered()),
                 this, SLOT(windowDecreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(actionIncreaseSize_, SIGNAL(triggered()),
                 this, SLOT(windowIncreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(enteringFullScreen()),
                 this, SLOT(swapShortcuts()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(exitingFullScreen()),
                 this, SLOT(swapShortcuts()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_timeline
  //
  void
  PlayerUxItem::init_timeline()
  {
    timeline_.reset(new TimelineItem("player_timeline",
                                     view_,
                                     player_->timeline()));

    TimelineItem & timeline = Item::add<TimelineItem>(timeline_);
    timeline.anchors_.fill(*this);
    timeline.setVisible(true);

    timeline.query_timeline_visible_.
      reset(&yae::context_query_timeline_visible, this);

    timeline.is_playback_paused_ = timeline.addExpr
      (new IsPlaybackPaused(*this));

    timeline.is_fullscreen_ = timeline.addExpr
      (new IsFullscreen(view_));

    timeline.toggle_playback_.reset(&yae::toggle_playback, this);

    timeline.toggle_fullscreen_ = view_.toggle_fullscreen_;
    timeline.layout();

    bool ok = true;
    ok = connect(&scrollWheelTimer_, SIGNAL(timeout()),
                 this, SLOT(scrollWheelTimerExpired()));
    YAE_ASSERT(ok);

    ok = connect(actionSetInPoint_, SIGNAL(triggered()),
                 &timeline_model(), SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionSetOutPoint_, SIGNAL(triggered()),
                 &timeline_model(), SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(&timeline_model(), SIGNAL(clockStopped(const SharedClock &)),
                 this, SLOT(playbackFinished(const SharedClock &)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_frame_crop
  //
  void
  PlayerUxItem::init_frame_crop()
  {
    frame_crop_.reset(new FrameCropItem("FrameCropItem", view_));
    FrameCropItem & frame_crop = this->add<FrameCropItem>(frame_crop_);
    frame_crop.anchors_.fill(*this);
    frame_crop.setVisible(false);

    bool ok = true;

    ok = connect(&frame_crop, SIGNAL(done()),
                 this, SLOT(dismissFrameCrop()));
    YAE_ASSERT(ok);

    ok = connect(&frame_crop, SIGNAL(cropped(const TVideoFramePtr &,
                                             const TCropFrame &)),
                 this, SLOT(cropped(const TVideoFramePtr &,
                                    const TCropFrame &)));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_frame_crop_sel
  //
  void
  PlayerUxItem::init_frame_crop_sel()
  {
    static const AspectRatio options[] = {
      AspectRatio(0.0, "none", AspectRatio::kNone),
      AspectRatio(4.0 / 3.0, "4:3"),
      AspectRatio(16.0 / 10.0, "16:10"),
      AspectRatio(16.0 / 9.0, "16:9"),

      AspectRatio(1.85),
      AspectRatio(2.35),
      AspectRatio(2.40),
      AspectRatio(8.0 / 3.0, "8:3"),

      AspectRatio(3.0 / 4.0, "3:4"),
      AspectRatio(9.0 / 16.0, "9:16"),
      AspectRatio(-1.0, "auto", AspectRatio::kAuto),
      AspectRatio(1e+6, "other", AspectRatio::kOther, "CropFrameOther"),
    };

    static const std::size_t num_options =
      sizeof(options) / sizeof(options[0]);

    frame_crop_sel_.reset(new AspectRatioItem("frame_crop_sel",
                                              view_,
                                              options,
                                              num_options));
    AspectRatioItem & frame_crop_sel =
      this->add<AspectRatioItem>(frame_crop_sel_);
    frame_crop_sel.anchors_.fill(*this);
    frame_crop_sel.setVisible(false);

    bool ok = true;
    ok = connect(&frame_crop_sel, SIGNAL(selected(const AspectRatio &)),
                 this, SLOT(selectFrameCrop(const AspectRatio &)));
    YAE_ASSERT(ok);

    ok = connect(&frame_crop_sel, SIGNAL(done()),
                 this, SLOT(dismissFrameCropSelection()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(select_frame_crop()),
                 this, SLOT(showFrameCropSelection()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameOther_, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameOther()));
    YAE_ASSERT(ok);

    ok = connect(&autocropTimer_, SIGNAL(timeout()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_aspect_ratio_sel
  //
  void
  PlayerUxItem::init_aspect_ratio_sel()
  {
    static const AspectRatio options[] = {
      AspectRatio(1.0, "1:1"),
      AspectRatio(4.0 / 3.0, "4:3"),
      AspectRatio(16.0 / 10.0, "16:10"),
      AspectRatio(16.0 / 9.0, "16:9"),

      AspectRatio(1.85),
      AspectRatio(2.35),
      AspectRatio(2.40),
      AspectRatio(8.0 / 3.0, "8:3"),

      AspectRatio(3.0 / 4.0, "3:4"),
      AspectRatio(9.0 / 16.0, "9:16"),
      AspectRatio(0.0, "auto", AspectRatio::kNone),
      AspectRatio(-1.0, "custom", AspectRatio::kOther),
    };

    static const std::size_t num_options =
      sizeof(options) / sizeof(options[0]);

    aspect_ratio_sel_.reset(new AspectRatioItem("aspect_ratio_sel",
                                                view_,
                                                options,
                                                num_options));
    AspectRatioItem & aspect_ratio_sel =
      this->add<AspectRatioItem>(aspect_ratio_sel_);
    aspect_ratio_sel.anchors_.fill(*this);
    aspect_ratio_sel.setVisible(false);

    bool ok = true;
    ok = connect(&aspect_ratio_sel, SIGNAL(selected(const AspectRatio &)),
                 this, SLOT(selectAspectRatio(const AspectRatio &)));
    YAE_ASSERT(ok);

    ok = connect(&aspect_ratio_sel, SIGNAL(aspectRatio(double)),
                 this, SLOT(setAspectRatio(double)));
    YAE_ASSERT(ok);

    ok = connect(&aspect_ratio_sel, SIGNAL(done()),
                 this, SLOT(dismissAspectRatioSelection()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(select_aspect_ratio()),
                 this, SLOT(showAspectRatioSelection()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioOther_, SIGNAL(triggered()),
                 this, SLOT(showAspectRatioSelection()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_video_track_sel
  //
  void
  PlayerUxItem::init_video_track_sel()
  {
    video_track_sel_.reset(new OptionItem("video_track_sel", view_));
    OptionItem & video_track_sel = this->add<OptionItem>(video_track_sel_);
    video_track_sel.anchors_.fill(*this);
    video_track_sel.setVisible(false);

    bool ok = true;
    ok = connect(&video_track_sel, SIGNAL(option_selected(int)),
                 this, SLOT(videoTrackSelectedOption(int)));
    YAE_ASSERT(ok);

    ok = connect(&video_track_sel, SIGNAL(done()),
                 this, SLOT(dismissVideoTrackSelection()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(select_video_track()),
                 this, SLOT(showVideoTrackSelection()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_audio_track_sel
  //
  void
  PlayerUxItem::init_audio_track_sel()
  {
    audio_track_sel_.reset(new OptionItem("audio_track_sel", view_));
    OptionItem & audio_track_sel = this->add<OptionItem>(audio_track_sel_);
    audio_track_sel.anchors_.fill(*this);
    audio_track_sel.setVisible(false);

    bool ok = true;
    ok = connect(&audio_track_sel, SIGNAL(option_selected(int)),
                 this, SLOT(audioTrackSelectedOption(int)));
    YAE_ASSERT(ok);

    ok = connect(&audio_track_sel, SIGNAL(done()),
                 this, SLOT(dismissAudioTrackSelection()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(select_audio_track()),
                 this, SLOT(showAudioTrackSelection()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::init_subtt_track_sel
  //
  void
  PlayerUxItem::init_subtt_track_sel()
  {
    subtt_track_sel_.reset(new OptionItem("subtt_track_sel", view_));
    OptionItem & subtt_track_sel = this->add<OptionItem>(subtt_track_sel_);
    subtt_track_sel.anchors_.fill(*this);
    subtt_track_sel.setVisible(false);

    bool ok = true;
    ok = connect(&subtt_track_sel, SIGNAL(option_selected(int)),
                 this, SLOT(subttTrackSelectedOption(int)));
    YAE_ASSERT(ok);

    ok = connect(&subtt_track_sel, SIGNAL(done()),
                 this, SLOT(dismissSubttTrackSelection()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(select_subtt_track()),
                 this, SLOT(showSubttTrackSelection()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::uncache
  //
  void
  PlayerUxItem::uncache()
  {
    TMakeCurrentContext currentContext(view_.context().get());
    enableBackArrowButton_.uncache();
    enableDeleteFileButton_.uncache();
    Item::uncache();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::setVisible
  //
  void
  PlayerUxItem::setVisible(bool enable)
  {
    bool changing = visible() != enable;

    if (!enable)
    {
      timelineTimer_.stop();
    }
    else if (changing)
    {
      timelineTimer_.start(1000);
    }

    menuPlayback_->menuAction()->setEnabled(enable);
    menuAudio_->menuAction()->setEnabled(enable);
    menuVideo_->menuAction()->setEnabled(enable);
    menuSubs_->menuAction()->setEnabled(enable);
    menuChapters_->menuAction()->setEnabled(enable);

    Item::setVisible(enable);

    if (changing)
    {
      emit visibility_changed(enable);
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::event
  //
  bool
  PlayerUxItem::event(QEvent * e)
  {
    QEvent::Type et = e->type();

    if (et == QEvent::User)
    {
      yae::AutoCropEvent * ac = dynamic_cast<yae::AutoCropEvent *>(e);
      if (ac)
      {
        ac->accept();
        canvas()->cropFrame(ac->cropFrame_);
        adjustCanvasHeight();
        return true;
      }
    }

    return QObject::event(e);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::processKeyEvent
  //
  // FIXME: pkoshevoy: must respect item focus
  //
  bool
  PlayerUxItem::processKeyEvent(Canvas * canvas, QKeyEvent * event)
  {
    int key = event->key();
    bool key_press = event->type() == QEvent::KeyPress;
    bool is_playlist_visible = timeline_->is_playlist_visible_.get();

    if (is_playlist_visible)
    {
      // let the playlist handle most events:
      return false;
    }
    else if (key == Qt::Key_I)
    {
      if (key_press)
      {
        actionSetInPoint_->activate(QAction::Trigger);
      }
    }
    else if (key == Qt::Key_O)
    {
      if (key_press)
      {
        actionSetOutPoint_->activate(QAction::Trigger);
      }
    }
    else if (key == Qt::Key_N)
    {
      if (key_press)
      {
        skipToNextFrame();
      }
    }
    else if (key == Qt::Key_MediaNext ||
             key == Qt::Key_Period ||
             key == Qt::Key_Greater ||
             key == Qt::Key_Right)
    {
      if (key_press)
      {
        skipForward();
      }
    }
    else if (key == Qt::Key_MediaPrevious ||
             key == Qt::Key_Comma ||
             key == Qt::Key_Less ||
             key == Qt::Key_Left)
    {
      if (key_press)
      {
        skipBack();
      }
    }
    else if (key == Qt::Key_MediaPlay ||
#if QT_VERSION >= 0x040700
             key == Qt::Key_MediaPause ||
             key == Qt::Key_MediaTogglePlayPause ||
#endif
             key == Qt::Key_Space ||
             key == Qt::Key_Enter ||
             key == Qt::Key_Return ||
             key == Qt::Key_MediaStop)
    {
      if (key_press)
      {
        togglePlayback();
      }
    }
    else
    {
      return false;
    }

    return true;
  }

  //----------------------------------------------------------------
  // PlayerUxItem::processWheelEvent
  //
  bool
  PlayerUxItem::processWheelEvent(Canvas * canvas, QWheelEvent * e)
  {
    TimelineItem & timeline = *timeline_;
    if (timeline.is_playlist_visible_.get())
    {
      // let the playlist handle the scroll event:
      return false;
    }

    double tNow = timeline_model().currentTime();
    if (tNow <= 1e-1)
    {
      // ignore it:
      return false;
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
  // PlayerUxItem::processMouseTracking
  //
  bool
  PlayerUxItem::processMouseTracking(const TVec2D & mousePt)
  {
    if (!view_.isEnabled())
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
  // PlayerUxItem::set_shortcuts
  //
  void
  PlayerUxItem::set_shortcuts(const yae::shared_ptr<PlayerShortcuts> & sc)
  {
    shortcuts_ = sc;

    if (!sc)
    {
      return;
    }

    PlayerShortcuts & shortcut = *shortcuts_;
    bool ok = true;

    ok = connect(&shortcut.fullScreen_, SIGNAL(activated()),
                 actionFullScreen_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.fillScreen_, SIGNAL(activated()),
                 actionFillScreen_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.showTimeline_, SIGNAL(activated()),
                 actionShowTimeline_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.play_, SIGNAL(activated()),
                 actionPlay_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.nextChapter_, SIGNAL(activated()),
                 actionNextChapter_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.loop_, SIGNAL(activated()),
                 actionLoop_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.cropNone_, SIGNAL(activated()),
                 actionCropFrameNone_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.crop1_33_, SIGNAL(activated()),
                 actionCropFrame1_33_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.crop1_78_, SIGNAL(activated()),
                 actionCropFrame1_78_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.crop1_85_, SIGNAL(activated()),
                 actionCropFrame1_85_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.crop2_40_, SIGNAL(activated()),
                 actionCropFrame2_40_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.cropOther_, SIGNAL(activated()),
                 actionCropFrameOther_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.autoCrop_, SIGNAL(activated()),
                 actionCropFrameAutoDetect_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.aspectRatioNone_, SIGNAL(activated()),
                 actionAspectRatioAuto_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.aspectRatio1_33_, SIGNAL(activated()),
                 actionAspectRatio1_33_, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&shortcut.aspectRatio1_78_, SIGNAL(activated()),
                 actionAspectRatio1_78_, SLOT(trigger()));
    YAE_ASSERT(ok);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::insert_menus
  //
  void
  PlayerUxItem::insert_menus(const IReaderPtr & reader,
                             QMenuBar * menubar,
                             QAction * before)
  {
    if (menuPlayback_->menuAction()->isEnabled())
    {
      menubar->insertAction(before, menuPlayback_->menuAction());
    }
    else
    {
      menubar->removeAction(menuPlayback_->menuAction());
    }

    if (menuAudio_->menuAction()->isEnabled())
    {
      menubar->insertAction(before, menuAudio_->menuAction());
    }
    else
    {
      menubar->removeAction(menuAudio_->menuAction());
    }

    if (menuVideo_->menuAction()->isEnabled())
    {
      menubar->insertAction(before, menuVideo_->menuAction());
    }
    else
    {
      menubar->removeAction(menuVideo_->menuAction());
    }

    if (menuSubs_->menuAction()->isEnabled())
    {
      menubar->insertAction(before, menuSubs_->menuAction());
    }
    else
    {
      menubar->removeAction(menuSubs_->menuAction());
    }

    if (menuChapters_->menuAction()->isEnabled())
    {
      menubar->insertAction(before, menuChapters_->menuAction());
    }
    else
    {
      menubar->removeAction(menuChapters_->menuAction());
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::autocrop_cb
  //
  TVideoFramePtr
  PlayerUxItem::autocrop_cb(void * callbackContext,
                            const TCropFrame & cf,
                            bool detectionFinished)
  {
    PlayerUxItem * player_ux = (PlayerUxItem *)callbackContext;
    IReader * reader = player_ux->get_reader();
    std::size_t videoTrack = reader ? reader->getSelectedVideoTrackIndex() : 0;
    std::size_t numVideoTracks = reader ? reader->getNumberOfVideoTracks() : 0;

    if (detectionFinished)
    {
      qApp->postEvent(player_ux, new AutoCropEvent(cf), Qt::HighEventPriority);
    }
    else if (player_ux->is_playback_paused() ||
             player_ux->player_->video()->isPaused() ||
             videoTrack >= numVideoTracks)
    {
      // use the same frame again:
      return player_ux->canvas()->currentFrame();
    }

    return TVideoFramePtr();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::sync_ui
  //
  void
  PlayerUxItem::sync_ui()
  {
    IReader * reader = get_reader();
    TimelineModel & timeline = timeline_model();
    timeline.updateDuration(reader);
    view_.requestRepaint();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::is_playback_paused
  //
  bool
  PlayerUxItem::is_playback_paused() const
  {
    return player_->paused();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playback
  //
  void
  PlayerUxItem::playback(const IReaderPtr & reader_ptr,
                         const IBookmark * bookmark,
                         bool start_from_zero_time)
  {
    dismissSelectionItems();

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

        if (enableBackArrowButton_.get())
        {
          timeline.back_arrow_cb_.reset(&yae::back_arrow_cb, this);
        }

        if (enableDeleteFileButton_.get())
        {
          timeline.delete_file_cb_.reset(&yae::delete_playing_file_cb, this);
        }
      }

      if (videoInfo.size() < 1)
      {
        timeline.video_track_cb_.reset();
      }
      else
      {
        timeline.video_track_cb_.reset(&yae::select_video_track_cb, this);
      }

      if (audioInfo.size() < 1)
      {
        timeline.audio_track_cb_.reset();
      }
      else
      {
        timeline.audio_track_cb_.reset(&yae::select_audio_track_cb, this);
      }

      timelineTimer_.start();
    }
    else
    {
      timelineTimer_.stop();
    }

    if (actionCropFrameAutoDetect_->isChecked())
    {
      autocropTimer_.start(1900);
    }
    else
    {
      QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
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

    emit reader_changed(reader_ptr);
    emit fixup_next_prev();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatioAuto
  //
  void
  PlayerUxItem::playbackAspectRatioAuto()
  {
    canvas()->overrideDisplayAspectRatio(0.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio2_40
  //
  void
  PlayerUxItem::playbackAspectRatio2_40()
  {
    canvas()->overrideDisplayAspectRatio(2.40);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio2_35
  //
  void
  PlayerUxItem::playbackAspectRatio2_35()
  {
    canvas()->overrideDisplayAspectRatio(2.35);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio1_85
  //
  void
  PlayerUxItem::playbackAspectRatio1_85()
  {
    canvas()->overrideDisplayAspectRatio(1.85);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio1_78
  //
  void
  PlayerUxItem::playbackAspectRatio1_78()
  {
    canvas()->overrideDisplayAspectRatio(16.0 / 9.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio1_60
  //
  void
  PlayerUxItem::playbackAspectRatio1_60()
  {
    canvas()->overrideDisplayAspectRatio(1.6);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackAspectRatio1_33
  //
  void
  PlayerUxItem::playbackAspectRatio1_33()
  {
    canvas()->overrideDisplayAspectRatio(4.0 / 3.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrameNone
  //
  void
  PlayerUxItem::playbackCropFrameNone()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(0.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame2_40
  //
  void
  PlayerUxItem::playbackCropFrame2_40()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(2.40);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame2_35
  //
  void
  PlayerUxItem::playbackCropFrame2_35()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(2.35);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame1_85
  //
  void
  PlayerUxItem::playbackCropFrame1_85()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(1.85);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame1_78
  //
  void
  PlayerUxItem::playbackCropFrame1_78()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(16.0 / 9.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame1_60
  //
  void
  PlayerUxItem::playbackCropFrame1_60()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(1.6);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrame1_33
  //
  void
  PlayerUxItem::playbackCropFrame1_33()
  {
    autocropTimer_.stop();
    canvas()->cropFrame(4.0 / 3.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrameAutoDetect
  //
  void
  PlayerUxItem::playbackCropFrameAutoDetect()
  {
    canvas()->cropAutoDetect(this, &(PlayerUxItem::autocrop_cb));
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackCropFrameOther
  //
  void
  PlayerUxItem::playbackCropFrameOther()
  {
    // shortcuts:
    Canvas * canvas = this->canvas();
    CanvasRenderer * renderer = canvas->canvasRenderer();
    FrameCropItem & frame_crop = *frame_crop_;

    TVideoFramePtr frame;
    renderer->getFrame(frame);
    if (!frame)
    {
      return;
    }

    // update the frame observer:
    canvas->delLoadFrameObserver(onLoadFrame_);
    onLoadFrame_.reset(new OnFrameLoaded(frame_crop.getRendererItem()));
    canvas->addLoadFrameObserver(onLoadFrame_);
    onLoadFrame_->frameLoaded(canvas, frame);

    // pass current frame crop info to the FrameCropItem:
    {
      TCropFrame crop;
      renderer->getCroppedFrame(crop);

      SignalBlocker blockSignals;
      blockSignals << &frame_crop;
      frame_crop.setCrop(frame, crop);
    }

    frame_crop_sel_->setVisible(false);
    aspect_ratio_sel_->setVisible(false);
    video_track_sel_->setVisible(false);
    audio_track_sel_->setVisible(false);
    subtt_track_sel_->setVisible(false);
    frame_crop_->setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackLoop
  //
  void
  PlayerUxItem::playbackLoop()
  {
    bool loop_playback = actionLoop_->isChecked();
    player_->set_loop_playback(loop_playback);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackColorConverter
  //
  void
  PlayerUxItem::playbackColorConverter()
  {
    bool skip = actionSkipColorConverter_->isChecked();
    saveBooleanSetting(kSkipColorConverter, skip);
    player_->skip_color_converter(skip);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackLoopFilter
  //
  void
  PlayerUxItem::playbackLoopFilter()
  {
    bool skip = actionSkipLoopFilter_->isChecked();
    saveBooleanSetting(kSkipLoopFilter, skip);
    player_->skip_loopfilter(skip);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackNonReferenceFrames
  //
  void
  PlayerUxItem::playbackNonReferenceFrames()
  {
    bool skip = actionSkipNonReferenceFrames_->isChecked();
    saveBooleanSetting(kSkipNonReferenceFrames, skip);
    player_->skip_nonref_frames(skip);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackDeinterlace
  //
  void
  PlayerUxItem::playbackDeinterlace()
  {
    bool deint = actionDeinterlace_->isChecked();
    saveBooleanSetting(kDeinterlaceFrames, deint);
    player_->set_deinterlace(deint);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackSetTempo
  //
  void
  PlayerUxItem::playbackSetTempo(int percent)
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
  // PlayerUxItem::playbackShowTimeline
  //
  void
  PlayerUxItem::playbackShowTimeline()
  {
    bool show_timeline = actionShowTimeline_->isChecked();
    saveBooleanSetting(kShowTimeline, show_timeline);

    if (anchors_.any_valid())
    {
      timeline_->showTimeline(show_timeline);
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackShowInFinder
  //
  void
  PlayerUxItem::playbackShowInFinder()
  {
    IReader * reader = get_reader();
    if (!reader)
    {
      return;
    }

    const char * filePath = reader->getResourcePath();
    if (!filePath)
    {
      return;
    }

#ifdef __APPLE__
    yae::showInFinder(filePath);
#else
    yae::show_in_file_manager(filePath);
#endif
  }

  //----------------------------------------------------------------
  // PlayerUxItem::showAspectRatioSelection
  //
  void
  PlayerUxItem::showAspectRatioSelection()
  {
    // shortcut:
    AspectRatioItem & aspect_ratio_sel = *aspect_ratio_sel_;

    if (aspect_ratio_sel.visible())
    {
      return;
    }

    int rotate = 0;
    double native_ar = canvas()->nativeAspectRatioRotated(rotate);
    native_ar = native_ar ? native_ar : 1.0;
    aspect_ratio_sel.setNativeAspectRatio(native_ar);

    double w = 0.0;
    double h = 0.0;
    double current_ar = canvas()->imageAspectRatio(w, h);

    // avoid creating an infinite signal loop:
    SignalBlocker blockSignals;
    blockSignals << &aspect_ratio_sel;

    current_ar = current_ar ? current_ar : 1.0;
    aspect_ratio_sel.setAspectRatio(current_ar);

    if (actionAspectRatioAuto_->isChecked())
    {
      aspect_ratio_sel.selectAspectRatioCategory(AspectRatio::kNone);
    }

    frame_crop_sel_->setVisible(false);
    aspect_ratio_sel.setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::selectAspectRatio
  //
  void
  PlayerUxItem::selectAspectRatio(const AspectRatio & option)
  {
    // update Aspect Ratio menu item selection
    double ar = option.ar_;

    if (option.category_ == AspectRatio::kNone)
    {
      ar = 0.0;
      actionAspectRatioAuto_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 4.0 / 3.0, 1e-2))
    {
      actionAspectRatio1_33_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 1.6, 1e-2))
    {
      actionAspectRatio1_60_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 16.0 / 9.0, 1e-2))
    {
      actionAspectRatio1_78_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 1.85, 1e-2))
    {
      actionAspectRatio1_85_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 2.35, 1e-2))
    {
      actionAspectRatio2_35_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 2.4, 1e-2))
    {
      actionAspectRatio2_40_->activate(QAction::Trigger);
    }
    else if (option.category_ == AspectRatio::kOther)
    {
      AspectRatioItem & aspect_ratio_sel = *aspect_ratio_sel_;
      ar = aspect_ratio_sel.currentAspectRatio();
      actionAspectRatioOther_->activate(QAction::Trigger);
    }
    else
    {
      actionAspectRatioOther_->activate(QAction::Trigger);
    }

    canvas()->overrideDisplayAspectRatio(ar);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::setAspectRatio
  //
  void
  PlayerUxItem::setAspectRatio(double ar)
  {
    // update Aspect Ratio menu item selection
    actionAspectRatioOther_->activate(QAction::Trigger);
    canvas()->overrideDisplayAspectRatio(ar);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::selectFrameCropAspectRatio
  //
  void
  PlayerUxItem::selectFrameCrop(const AspectRatio & option)
  {
    // update Crop menu item selection
    if (option.category_ == AspectRatio::kNone)
    {
      actionCropFrameNone_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 4.0 / 3.0, 1e-2))
    {
      actionCropFrame1_33_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 1.6, 1e-2))
    {
      actionCropFrame1_60_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 16.0 / 9.0, 1e-2))
    {
      actionCropFrame1_78_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 1.85, 1e-2))
    {
      actionCropFrame1_85_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 2.35, 1e-2))
    {
      actionCropFrame2_35_->activate(QAction::Trigger);
    }
    else if (close_enough(option.ar_, 2.4, 1e-2))
    {
      actionCropFrame2_40_->activate(QAction::Trigger);
    }
    else if (option.category_ == AspectRatio::kAuto)
    {
      actionCropFrameAutoDetect_->activate(QAction::Trigger);
    }
    else if (option.category_ == AspectRatio::kOther)
    {
      actionCropFrameOther_->activate(QAction::Trigger);
    }
    else
    {
      bool ok = true;

      ok = disconnect(actionCropFrameOther_, SIGNAL(triggered()),
                      this, SLOT(playbackCropFrameOther()));
      YAE_ASSERT(ok);

      canvas()->cropFrame(option.ar_);
      actionCropFrameOther_->activate(QAction::Trigger);

      ok = connect(actionCropFrameOther_, SIGNAL(triggered()),
                   this, SLOT(playbackCropFrameOther()));
      YAE_ASSERT(ok);
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::showFrameCropSelection
  //
  void
  PlayerUxItem::showFrameCropSelection()
  {
    // shortcut:
    AspectRatioItem & frame_crop_sel = *frame_crop_sel_;

    if (frame_crop_sel.visible())
    {
      return;
    }

    int rotate = 0;
    double native_ar = canvas()->nativeAspectRatioUncroppedRotated(rotate);
    double current_ar = canvas()->nativeAspectRatioRotated(rotate);

    native_ar = native_ar ? native_ar : 1.0;
    frame_crop_sel.setNativeAspectRatio(native_ar);

    // avoid creating an infinite signal loop:
    SignalBlocker blockSignals;
    blockSignals << &frame_crop_sel;

    current_ar = current_ar ? current_ar : 1.0;
    frame_crop_sel.setAspectRatio(current_ar);

    if (actionCropFrameNone_->isChecked())
    {
      frame_crop_sel.selectAspectRatioCategory(AspectRatio::kNone);
    }
    else if (actionCropFrameAutoDetect_->isChecked())
    {
      frame_crop_sel.selectAspectRatioCategory(AspectRatio::kAuto);
    }

    frame_crop_->setVisible(false);
    frame_crop_sel.setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::showVideoTrackSelection
  //
  void
  PlayerUxItem::showVideoTrackSelection()
  {
    // shortcuts:
    const PlayerItem & player = *(player_);
    IReaderPtr reader = player.reader();

    const std::vector<TTrackInfo> & tracks = player.video_tracks_info();
    const std::vector<VideoTraits> & traits = player.video_tracks_traits();

    std::size_t num_tracks = tracks.size();
    YAE_ASSERT(num_tracks == traits.size());

    std::vector<OptionItem::Option> options(num_tracks + 1);
    for (std::size_t i = 0; i < num_tracks; i++)
    {
      const TTrackInfo & info = tracks[i];
      const VideoTraits & vtts = traits[i];

      OptionItem::Option & option = options[i];
      option.index_ = i;

      // headline:
      {
        std::ostringstream oss;

        oss << "Video Track " << i + 1;
        if (info.hasLang())
        {
          oss << " (" << info.lang() << ")";
        }

        if (info.hasName())
        {
          oss << ": " << info.name();
        }

        option.headline_ = oss.str().c_str();
      }

      // fineprint:
      {
        std::ostringstream oss;

        oss << yae::strfmt("%u x %u, %.3f fps",
                           vtts.visibleWidth_,
                           vtts.visibleHeight_,
                           vtts.frameRate_);

        if (vtts.cameraRotation_)
        {
          static const char * degree_utf8 = "\xc2""\xb0";
          oss << ", rotated " << vtts.cameraRotation_ << degree_utf8;
        }

        std::string service = yae::get_program_name(*reader, info.program_);
        if (service.size())
        {
          oss << ", " << service;
        }
        else if (info.nprograms_ > 1)
        {
          oss << ", program " << info.program_;
        }

        option.fineprint_ = oss.str().c_str();
      }
    }

    // add Disabled track option:
    {
      OptionItem::Option & option = options[num_tracks];
      option.index_ = num_tracks;
      option.headline_ = "Disabled";
      option.fineprint_ = "";
    }

    int preselect = reader->getSelectedVideoTrackIndex();
    OptionItem & video_track_sel = *video_track_sel_;
    video_track_sel.setOptions(options, preselect);
    video_track_sel.setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::showAudioTrackSelection
  //
  void
  PlayerUxItem::showAudioTrackSelection()
  {
    // shortcuts:
    const PlayerItem & player = *(player_);
    IReaderPtr reader = player.reader();

    const std::vector<TTrackInfo> & tracks = player.audio_tracks_info();
    const std::vector<AudioTraits> & traits = player.audio_tracks_traits();

    std::size_t num_tracks = tracks.size();
    YAE_ASSERT(num_tracks == traits.size());

    std::vector<OptionItem::Option> options(num_tracks + 1);
    for (std::size_t i = 0; i < num_tracks; i++)
    {
      const TTrackInfo & info = tracks[i];
      const AudioTraits & atts = traits[i];

      OptionItem::Option & option = options[i];
      option.index_ = i;

      // headline:
      {
        std::ostringstream oss;

        oss << "Audio Track " << i + 1;
        if (info.hasLang())
        {
          oss << " (" << info.lang() << ")";
        }

        if (info.hasName())
        {
          oss << ": " << info.name();
        }

        option.headline_ = oss.str().c_str();
      }

      // fineprint:
      {
        std::ostringstream oss;

        oss << atts.sampleRate_ << " Hz, "
            << getNumberOfChannels(atts.channelLayout_) << " channels";

        std::string service = yae::get_program_name(*reader, info.program_);
        if (service.size())
        {
          oss << ", " << service;
        }
        else if (info.nprograms_ > 1)
        {
          oss << ", program " << info.program_;
        }

        option.fineprint_ = oss.str().c_str();
      }
    }

    // add Disabled track option:
    {
      OptionItem::Option & option = options[num_tracks];
      option.index_ = num_tracks;
      option.headline_ = "Disabled";
      option.fineprint_ = "";
    }

    int preselect = reader->getSelectedAudioTrackIndex();
    OptionItem & audio_track_sel = *audio_track_sel_;
    audio_track_sel.setOptions(options, preselect);
    audio_track_sel.setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::showSubttTrackSelection
  //
  void
  PlayerUxItem::showSubttTrackSelection()
  {
    // shortcut:
    const PlayerItem & player = *(player_);
    IReaderPtr reader = player.reader();

    const std::vector<TTrackInfo> & tracks = player.subtt_tracks_info();
    const std::vector<TSubsFormat> & formats = player.subtt_tracks_format();

    std::size_t num_tracks = tracks.size();
    YAE_ASSERT(num_tracks == formats.size());

    std::vector<OptionItem::Option> options(num_tracks + 4 + 1);
    for (std::size_t i = 0; i < num_tracks; i++)
    {
      const TTrackInfo & info = tracks[i];
      TSubsFormat format = formats[i];

      OptionItem::Option & option = options[i];
      option.index_ = i;

      // headline:
      {
        std::ostringstream oss;

        oss << "Subtitles Track " << i + 1;

        if (info.hasLang())
        {
          oss << " (" << info.lang() << ")";
        }

        if (info.hasName())
        {
          oss << ": " << info.name();
        }

        option.headline_ = oss.str().c_str();
      }

      // fineprint:
      {
        std::ostringstream oss;

        oss << "format: " << getSubsFormatLabel(format);

        std::string service = yae::get_program_name(*reader, info.program_);
        if (service.size())
        {
          oss << ", " << service;
        }
        else if (info.nprograms_ > 1)
        {
          oss << ", program " << info.program_;
        }

        option.fineprint_ = oss.str().c_str();
      }
    }

    // add fake CC1-4 tracks:
    for (unsigned int i = 0; i < 4; i++)
    {
      OptionItem::Option & option = options[num_tracks + i];
      option.index_ = num_tracks + i + 1;
      option.headline_ = yae::strfmt("Closed Captions (CC%u)", (i + 1));
      option.fineprint_ = "format: CEA-608";
    }

    // add Disabled track option:
    {
      OptionItem::Option & option = options[num_tracks + 4];
      option.index_ = num_tracks + 5;
      option.headline_ = "Disabled";
      option.fineprint_ = "";
    }

    int preselect = reader ? get_selected_subtt_track(*reader) : 4;
    OptionItem & subtt_track_sel = *subtt_track_sel_;
    subtt_track_sel.setOptions(options, preselect);
    subtt_track_sel.setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissFrameCrop
  //
  void
  PlayerUxItem::dismissFrameCrop()
  {
    frame_crop_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissFrameCropSelection
  //
  void
  PlayerUxItem::dismissFrameCropSelection()
  {
    frame_crop_sel_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissAspectRatioSelection
  //
  void
  PlayerUxItem::dismissAspectRatioSelection()
  {
    aspect_ratio_sel_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissVideoTrackSelection
  //
  void
  PlayerUxItem::dismissVideoTrackSelection()
  {
    video_track_sel_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissAudioTrackSelection
  //
  void
  PlayerUxItem::dismissAudioTrackSelection()
  {
    audio_track_sel_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissSubttTrackSelection
  //
  void
  PlayerUxItem::dismissSubttTrackSelection()
  {
    subtt_track_sel_->setVisible(false);
    player_->setVisible(true);
    timeline_->setVisible(true);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::dismissSelections
  //
  void
  PlayerUxItem::dismissSelectionItems()
  {
    frame_crop_->setVisible(false);
    frame_crop_sel_->setVisible(false);
    aspect_ratio_sel_->setVisible(false);
    video_track_sel_->setVisible(false);
    audio_track_sel_->setVisible(false);
    subtt_track_sel_->setVisible(false);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::videoTrackSelectedOption
  //
  void
  PlayerUxItem::videoTrackSelectedOption(int option_index)
  {
    if (!videoTrackGroup_)
    {
      return;
    }

    if (option_index < videoTrackGroup_->actions().size())
    {
      videoTrackGroup_->actions()[option_index]->trigger();
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::audioTrackSelectedOption
  //
  void
  PlayerUxItem::audioTrackSelectedOption(int option_index)
  {
    if (!audioTrackGroup_)
    {
      return;
    }

    if (option_index < audioTrackGroup_->actions().size())
    {
      audioTrackGroup_->actions()[option_index]->trigger();
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::subttTrackSelectedOption
  //
  void
  PlayerUxItem::subttTrackSelectedOption(int option_index)
  {
    if (!subsTrackGroup_)
    {
      return;
    }

    if (option_index < subsTrackGroup_->actions().size())
    {
      subsTrackGroup_->actions()[option_index]->trigger();
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::windowHalfSize
  //
  void
  PlayerUxItem::windowHalfSize()
  {
    canvasSizeSet(0.5, 0.5);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::windowFullSize
  //
  void
  PlayerUxItem::windowFullSize()
  {
    canvasSizeSet(1.0, 1.0);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::windowDoubleSize
  //
  void
  PlayerUxItem::windowDoubleSize()
  {
    canvasSizeSet(2.0, 2.0);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::windowDecreaseSize
  //
  void
  PlayerUxItem::windowDecreaseSize()
  {
    canvasSizeScaleBy(0.5);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::windowIncreaseSize
  //
  void
  PlayerUxItem::windowIncreaseSize()
  {
    canvasSizeScaleBy(2.0);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::audioDownmixToStereo
  //
  void
  PlayerUxItem::audioDownmixToStereo()
  {
    bool downmix = actionDownmixToStereo_->isChecked();
    saveBooleanSetting(kDownmixToStereo, downmix);
    player_->set_downmix_to_stereo(downmix);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::audioSelectTrack
  //
  void
  PlayerUxItem::audioSelectTrack(int index)
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

      // QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::videoSelectTrack
  //
  void
  PlayerUxItem::videoSelectTrack(int index)
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

      QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
    }

    emit video_track_selected();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::subsSelectTrack
  //
  void
  PlayerUxItem::subsSelectTrack(int index)
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

      // QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::updateChaptersMenu
  //
  void
  PlayerUxItem::updateChaptersMenu()
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
  // PlayerUxItem::skipToNextChapter
  //
  void
  PlayerUxItem::skipToNextChapter()
  {
    if (player_->skip_to_next_chapter())
    {
      return;
    }

    // last chapter, skip to next playlist item:
    emit playback_next();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::skipToChapter
  //
  void
  PlayerUxItem::skipToChapter(int index)
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
  // PlayerUxItem::skipToNextFrame
  //
  void
  PlayerUxItem::skipToNextFrame()
  {
    player_->skip_to_next_frame();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::skipForward
  //
  void
  PlayerUxItem::skipForward()
  {
    player_->skip_forward();
    timeline_->maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::skipBack
  //
  void
  PlayerUxItem::skipBack()
  {
    player_->skip_back();
    timeline_->maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::scrollWheelTimerExpired
  //
  void
  PlayerUxItem::scrollWheelTimerExpired()
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
  // PlayerUxItem::cropped
  //
  void
  PlayerUxItem::cropped(const TVideoFramePtr & frame, const TCropFrame & crop)
  {
    (void) frame;
    canvas()->cropFrame(crop);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::stopPlayback
  //
  void
  PlayerUxItem::stopPlayback()
  {
    timelineTimer_.stop();
    bookmarkTimer_.stop();
    player_->playback_stop();
    timeline_->modelChanged();
    timeline_->maybeAnimateOpacity();
    timeline_->maybeAnimateControls();
    actionPlay_->setText(tr("Play"));

    dismissSelectionItems();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::togglePlayback
  //
  void
  PlayerUxItem::togglePlayback()
  {
    if (!view_.isEnabled())
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
  // PlayerUxItem::playbackFinished
  //
  void
  PlayerUxItem::playbackFinished(const SharedClock & c)
  {
    if (!timeline_model().sharedClock().sharesCurrentTimeWith(c))
    {
      // ignoring stale playbackFinished
      return;
    }

    IReaderPtr reader_ptr = player_->reader();
    TTime t0(0, 0);
    TTime t1(0, 0);
    yae::get_timeline(reader_ptr.get(), t0, t1);

    // move the bookmark to the start of the recording:
    const TimelineModel & timeline = timeline_model();
    emit save_bookmark_at(timeline.timelineStart());

    // stop playback timers, close the reader:
    stopPlayback();

    emit playback_finished(t1);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::swapShortcuts
  //
  void
  PlayerUxItem::swapShortcuts()
  {
    if (!shortcuts_)
    {
      return;
    }

    PlayerShortcuts & shortcut = *shortcuts_;
    yae::swapShortcuts(&shortcut.fullScreen_, actionFullScreen_);
    yae::swapShortcuts(&shortcut.fillScreen_, actionFillScreen_);
    yae::swapShortcuts(&shortcut.showTimeline_, actionShowTimeline_);
    yae::swapShortcuts(&shortcut.play_, actionPlay_);
    yae::swapShortcuts(&shortcut.loop_, actionLoop_);
    yae::swapShortcuts(&shortcut.cropNone_, actionCropFrameNone_);
    yae::swapShortcuts(&shortcut.crop1_33_, actionCropFrame1_33_);
    yae::swapShortcuts(&shortcut.crop1_78_, actionCropFrame1_78_);
    yae::swapShortcuts(&shortcut.crop1_85_, actionCropFrame1_85_);
    yae::swapShortcuts(&shortcut.crop2_40_, actionCropFrame2_40_);
    yae::swapShortcuts(&shortcut.cropOther_, actionCropFrameOther_);
    yae::swapShortcuts(&shortcut.autoCrop_, actionCropFrameAutoDetect_);
    yae::swapShortcuts(&shortcut.nextChapter_, actionNextChapter_);
    yae::swapShortcuts(&shortcut.aspectRatioNone_, actionAspectRatioAuto_);
    yae::swapShortcuts(&shortcut.aspectRatio1_33_, actionAspectRatio1_33_);
    yae::swapShortcuts(&shortcut.aspectRatio1_78_, actionAspectRatio1_78_);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::populateContextMenu
  //
  void
  PlayerUxItem::populateContextMenu()
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

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionShowInFinder_);
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::adjustMenuActions
  //
  void
  PlayerUxItem::adjustMenuActions()
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
  // PlayerUxItem::adjustMenuActions
  //
  void
  PlayerUxItem::adjustMenuActions(IReader * reader,
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
        std::string summary = traits.summary();
        trackName += tr(", %1").arg(QString::fromUtf8(summary.c_str()));
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
  // PlayerUxItem::adjustCanvasHeight
  //
  void
  PlayerUxItem::adjustCanvasHeight()
  {
    bool playlist_visible = timeline_->is_playlist_visible_.get();
    if (playlist_visible)
    {
      return;
    }

    IReader * reader = get_reader();
    if (!reader)
    {
      return;
    }

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (!canvas_delegate.isFullScreen())
    {
      double w = 1.0;
      double h = 1.0;
      double dar = canvas()->imageAspectRatio(w, h);

      if (dar)
      {
        double cw = canvas()->canvasLogicalWidth();
        double s = cw / w;
        canvasSizeSet(s, s);
      }
    }

    if (frame_crop_->visible())
    {
      playbackCropFrameOther();
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::canvasSizeSet
  //
  void
  PlayerUxItem::canvasSizeSet(double xexpand, double yexpand)
  {
    xexpand_ = xexpand;
    yexpand_ = yexpand;

    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (canvas_delegate.isFullScreen())
    {
      return;
    }

    Canvas * canvas = this->canvas();
    double iw = canvas->imageWidth();
    double ih = canvas->imageHeight();

    int vw = int(0.5 + iw);
    int vh = int(0.5 + ih);

    if (vw < 1 || vh < 1)
    {
      return;
    }

    double cw = canvas->canvasLogicalWidth();
    double ch = canvas->canvasLogicalHeight();

    TVec2D clientPos, clientSize;
    canvas_delegate.getWindowClient(clientPos, clientSize);

    TVec2D windowPos, windowSize;
    canvas_delegate.getWindowFrame(windowPos, windowSize);
    double ww = windowSize.x();
    double wh = windowSize.y();

    // calculate width and height overhead:
    double ox = ww - cw;
    double oy = wh - ch;

    YAE_ASSERT(ox >= 0.0);
    YAE_ASSERT(oy >= 0.0);

    double ideal_w = floor(0.5 + vw * xexpand_);
    double ideal_h = floor(0.5 + vh * yexpand_);

    TVec2D screenPos, screenSize;
    canvas_delegate.getScreenGeometry(screenPos, screenSize);

    double max_w = screenSize.x() - ox;
    double max_h = screenSize.y() - oy;

    if (ideal_w > max_w || ideal_h > max_h)
    {
      // image won't fit on screen, scale it to the largest size that fits:
      double vDAR = iw / ih;
      double cDAR = double(max_w) / double(max_h);

      if (vDAR > cDAR)
      {
        ideal_w = max_w;
        ideal_h = floor(0.5 + double(max_w) / vDAR);
      }
      else
      {
        ideal_h = max_h;
        ideal_w = floor(0.5 + double(max_h) * vDAR);
      }
    }

    double new_w = std::min(ideal_w, max_w);
    double new_h = std::min(ideal_h, max_h);

    TVec2D delta;
    delta[0] = new_w - cw;
    delta[1] = new_h - ch;

    // apply the new geometry:
    canvas_delegate.resizeWindowClient(clientSize + delta);

    // repaint the frame:
    canvas->refresh();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::canvasSizeScaleBy
  //
  void
  PlayerUxItem::canvasSizeScaleBy(double scale)
  {
    canvasSizeSet(xexpand_ * scale, yexpand_ * scale);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::canvasSizeBackup
  //
  void
  PlayerUxItem::canvasSizeBackup()
  {
    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (canvas_delegate.isFullScreen())
    {
      return;
    }

    Canvas * canvas = this->canvas();
    int vw = int(0.5 + canvas->imageWidth());
    int vh = int(0.5 + canvas->imageHeight());
    if (vw < 1 || vh < 1)
    {
      return;
    }

    double cw = canvas->canvasLogicalWidth();
    double ch = canvas->canvasLogicalHeight();
    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::canvasSizeRestore
  //
  void
  PlayerUxItem::canvasSizeRestore()
  {
    canvasSizeSet(xexpand_, yexpand_);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackVerticalScaling
  //
  void
  PlayerUxItem::playbackVerticalScaling(bool enable)
  {
    canvasSizeBackup();
    canvas()->enableVerticalScaling(enable);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackShrinkWrap
  //
  void
  PlayerUxItem::playbackShrinkWrap()
  {
    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (canvas_delegate.isFullScreen())
    {
      return;
    }

    IReader * reader = get_reader();
    if (!reader)
    {
      return;
    }

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    canvasSizeBackup();

    double scale = std::min<double>(xexpand_, yexpand_);
    canvasSizeSet(scale, scale);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackFullScreen
  //
  void
  PlayerUxItem::playbackFullScreen()
  {
    // enter full screen pillars-and-bars letterbox rendering:
    enterFullScreen(Canvas::kScaleToFit);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::playbackFillScreen
  //
  void
  PlayerUxItem::playbackFillScreen()
  {
    // enter full screen crop-to-fill rendering:
    enterFullScreen(Canvas::kCropToFill);
  }

  //----------------------------------------------------------------
  // PlayerUxItem::requestToggleFullScreen
  //
  void
  PlayerUxItem::requestToggleFullScreen()
  {
    // all this to work-around apparent QML bug where
    // toggling full-screen on double-click leaves Flickable in
    // a state where it never receives the button-up event
    // and ends up interpreting all mouse movement as dragging,
    // very annoying...
    //
    // The workaround is to delay fullscreen toggle to allow
    // Flickable time to receive the button-up event

    QTimer::singleShot(178, this, SLOT(toggleFullScreen()));
  }

  //----------------------------------------------------------------
  // PlayerUxItem::toggleFullScreen
  //
  void
  PlayerUxItem::toggleFullScreen()
  {
    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (canvas_delegate.isFullScreen())
    {
      exitFullScreen();
    }
    else
    {
      enterFullScreen(renderMode_);
    }
  }

  //----------------------------------------------------------------
  // PlayerUxItem::enterFullScreen
  //
  void
  PlayerUxItem::enterFullScreen(Canvas::TRenderMode renderMode)
  {
    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    bool is_fullscreen = canvas_delegate.isFullScreen();
    if (is_fullscreen && renderMode_ == renderMode)
    {
      exitFullScreen();
      return;
    }

    if (!is_fullscreen)
    {
      emit enteringFullScreen();
    }

    SignalBlocker blockSignals;
    blockSignals
      << actionFullScreen_
      << actionFillScreen_;

    if (renderMode == Canvas::kScaleToFit)
    {
      actionFullScreen_->setChecked(true);
      actionFillScreen_->setChecked(false);
    }

    if (renderMode == Canvas::kCropToFill)
    {
      actionFillScreen_->setChecked(true);
      actionFullScreen_->setChecked(false);
    }

    canvas()->setRenderMode(renderMode);
    renderMode_ = renderMode;

    if (is_fullscreen)
    {
      return;
    }

    // enter full screen rendering:
    actionShrinkWrap_->setEnabled(false);

    canvas_delegate.showFullScreen();
  }

  //----------------------------------------------------------------
  // PlayerUxItem::exitFullScreen
  //
  void
  PlayerUxItem::exitFullScreen()
  {
    Canvas::IDelegate & canvas_delegate = *view_.delegate();
    if (!canvas_delegate.isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals
      << actionFullScreen_
      << actionFillScreen_;

    actionFullScreen_->setChecked(false);
    actionFillScreen_->setChecked(false);
    actionShrinkWrap_->setEnabled(true);

    canvas_delegate.exitFullScreen();
    canvas()->setRenderMode(Canvas::kScaleToFit);
    QTimer::singleShot(100, this, SLOT(adjustCanvasHeight()));

    emit exitingFullScreen();
  }

}
