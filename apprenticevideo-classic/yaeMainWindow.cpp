// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:55:21 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// aeyae:
#include "yae/api/yae_version.h"
#include "yae/ffmpeg/yae_reader_ffmpeg.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video_renderer.h"

// standard:
#include <iostream>
#include <sstream>
#include <list>
#include <math.h>

// GLEW:
#include <GL/glew.h>

YAE_DISABLE_DEPRECATION_WARNINGS

// boost:
#include <boost/algorithm/string.hpp>

YAE_ENABLE_DEPRECATION_WARNINGS

// Qt:
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QWheelEvent>
#include <QDragEnterEvent>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMimeData>
#include <QUrl>
#include <QSpacerItem>
#include <QMenu>
#include <QShortcut>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QDirIterator>

// yaeui:
#ifdef __APPLE__
#include "yaeAudioUnitRenderer.h"
#include "yaeAppleUtils.h"
#else
#include "yaePortaudioRenderer.h"
#endif
#include "yaeUtilsQt.h"

// local:
#include "yaeMainWindow.h"


namespace yae
{

  //----------------------------------------------------------------
  // kCreateBookmarksAutomatically
  //
  static const QString kCreateBookmarksAutomatically =
    QString::fromUtf8("CreateBookmarksAutomatically");

  //----------------------------------------------------------------
  // kResumePlaybackFromBookmark
  //
  static const QString kResumePlaybackFromBookmark =
    QString::fromUtf8("ResumePlaybackFromBookmark");

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
  // swapLayouts
  //
  static void
  swapLayouts(QWidget * a, QWidget * b)
  {
    QWidget tmp;
    QLayout * la = a->layout();
    QLayout * lb = b->layout();
    tmp.setLayout(la);
    a->setLayout(lb);
    b->setLayout(la);
  }

  //----------------------------------------------------------------
  // AboutDialog::AboutDialog
  //
  AboutDialog::AboutDialog(QWidget * parent):
    QDialog(parent),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);

    textBrowser->setSearchPaths(QStringList() << ":/");
    textBrowser->setSource(QUrl("qrc:///yaeAbout.html"));
  }

  //----------------------------------------------------------------
  // AspectRatioDialog::AspectRatioDialog
  //
  AspectRatioDialog::AspectRatioDialog(QWidget * parent):
    QDialog(parent),
    Ui::AspectRatioDialog()
  {
    Ui::AspectRatioDialog::setupUi(this);
  }

  //----------------------------------------------------------------
  // OpenUrlDialog::OpenUrlDialog
  //
  OpenUrlDialog::OpenUrlDialog(QWidget * parent):
    QDialog(parent),
    Ui::OpenUrlDialog()
  {
    Ui::OpenUrlDialog::setupUi(this);
  }


  //----------------------------------------------------------------
  // PlaylistBookmark::PlaylistBookmark
  //
  PlaylistBookmark::PlaylistBookmark():
    TBookmark(),
    itemIndex_(std::numeric_limits<std::size_t>::max()),
    action_(NULL)
  {}


  //----------------------------------------------------------------
  // setTimelineCss
  //
  static void
  setTimelineCss(QWidget * timeline, bool noVideo = false)
  {
    static const char * cssVideo =
      "QWidget#timelineSeparator_ {\n"
      "  background-color: #000000;\n"
      "}\n"
      "\n"
      "QWidget {\n"
      "    background-color: #000000;\n"
      "}\n"
      "\n"
      "QToolButton {\n"
      "    background-color: #000000;\n"
      "    border: -px solid #000000;\n"
      "}\n"
      "\n"
      "QLineEdit {\n"
      "    color: #e0e0e0;\n"
      "    border: 0px solid #404040;\n"
      "    border-radius: 3px;\n"
      "    background-color: #000000;\n"
      "}\n"
      "\n"
      "QLineEdit:disabled {\n"
      "    color: #404040;\n"
      "}\n";

    static const char * cssAudio =
      "QWidget#timelineSeparator_ {\n"
      "  background-color: %1;\n"
      "}\n"
      "\n"
      "QWidget {\n"
      "    background-color: %2;\n"
      "}\n"
      "\n"
      "QToolButton {\n"
      "    background-color: %2;\n"
      "    border: 0px solid %2;\n"
      "}\n"
      "\n"
      "QLineEdit {\n"
      "    color: %3;\n"
      "    border: 0px solid %2;\n"
      "    border-radius: 3px;\n"
      "    background-color: %2;\n"
      "}\n"
      "\n"
      "QLineEdit:disabled {\n"
      "    color: %4;\n"
      "}\n";

    QString css;

    if (noVideo)
    {
      QPalette palette;

      css =
        QString::fromUtf8(cssAudio).
        arg(palette.color(QPalette::Mid).name(),
            palette.color(QPalette::Window).name(),
            palette.color(QPalette::WindowText).name(),
            palette.color(QPalette::Disabled,
                          QPalette::WindowText).name());
    }
    else
    {
      css = QString::fromUtf8(cssVideo);
    }

    timeline->setStyleSheet(css);
  }

  //----------------------------------------------------------------
  // setTimelineCssForVideo
  //
  static void
  setTimelineCssForVideo(QWidget * timeline)
  {
    setTimelineCss(timeline, false);
  }

  //----------------------------------------------------------------
  // setTimelineCssForAudio
  //
  static void
  setTimelineCssForAudio(QWidget * timeline)
  {
    setTimelineCss(timeline, true);
  }

  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow(const TReaderFactoryPtr & readerFactory):
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    subsTrackGroup_(NULL),
    chaptersGroup_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    subsTrackMapper_(NULL),
    chapterMapper_(NULL),
    bookmarksGroup_(NULL),
    bookmarksMapper_(NULL),
    bookmarksMenuSeparator_(NULL),
    readerFactory_(readerFactory),
    readerId_(0),
    canvas_(NULL),
    playbackPaused_(false),
    scrollStart_(0.0),
    scrollOffset_(0.0),
    renderMode_(Canvas::kScaleToFit),
    xexpand_(1.0),
    yexpand_(1.0),
    tempo_(1.0),
    selVideo_(0, 1),
    selAudio_(0, 1),
    selSubsFormat_(kSubsNone),
    selClosedCaptions_(0),
    style_("classic", view_)
  {
    setupUi(this);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(playlistWidget_);
    actionPlay->setText(tr("Pause"));

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon = QString::fromUtf8(":/images/apprenticevideo-256.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    timelineControls_->setAuxWidgets(lineEditPlayhead_,
                                     lineEditDuration_,
                                     this);

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

    // setup the canvas widget (QML quick widget):
#ifndef YAE_USE_QGL_WIDGET
    canvas_ = new TCanvasWidget(this);
    canvas_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    TCanvasWidget * shared_ctx = NULL;
    canvas_ = new TCanvasWidget(contextFormat, this, shared_ctx);
#endif

    canvas_->setObjectName(tr("player view canvas"));

    canvas_->setFocusPolicy(Qt::StrongFocus);
    canvas_->setAcceptDrops(false);

    QString greeting =
      tr("drop video/music files here\n\n"
         "press Spacebar to pause/resume playback\n\n"
         "press %1 to toggle the playlist\n\n"
         "press %2 to toggle the timeline\n\n"
         "press Alt %3, Alt %4 to skip through the playlist\n\n"
#ifdef __APPLE__
         "use Apple Remote to change volume or skip along the timeline\n\n"
#endif
         "explore the menus for more options").
      arg(actionShowPlaylist->shortcut().toString()). // playlist
      arg(actionShowTimeline->shortcut().toString()). // timeline
      arg(QString::fromUtf8("\xE2""\x86""\x90")). // left arrow
      arg(QString::fromUtf8("\xE2""\x86""\x92")); // right arrow

    canvas_->setGreeting(greeting);

    // insert canvas widget into the main window layout:
    canvasLayout->addWidget(canvas_);

    reader_.reset(ReaderFFMPEG::create());
#ifdef __APPLE__
    audioRenderer_.reset(AudioUnitRenderer::create());
#else
    audioRenderer_.reset(PortaudioRenderer::create());
#endif
    videoRenderer_.reset(VideoRenderer::create());

    // restore timeline preference (default == show):
    setTimelineCssForVideo(timelineWidgets_);
    {
      SignalBlocker blockSignals(actionShowTimeline);
      bool showTimeline = loadBooleanSettingOrDefault(kShowTimeline, true);
      actionShowTimeline->setChecked(showTimeline);

      if (showTimeline)
      {
        timelineWidgets_->show();
      }
      else
      {
        timelineWidgets_->hide();
      }
    }

    // hide the playlist:
    actionShowPlaylist->setChecked(false);
    playlistDock_->hide();

    // setup the Open URL dialog:
    {
      std::list<std::string> protocols;
      reader_->getUrlProtocols(protocols);

      QString supported = tr("Supported URL protocols include:\n");
      unsigned int column = 0;
      for (std::list<std::string>::const_iterator i = protocols.begin();
           i != protocols.end(); ++i)
      {
        const std::string & name = *i;
        if (column)
        {
          supported += "  ";
        }

        supported += QString::fromUtf8(name.c_str());
        column = (column + 1) % 6;

        if (!column)
        {
          supported += "\n";
        }
      }

      openUrl_ = new OpenUrlDialog(this);
      openUrl_->lineEdit->setToolTip(supported);
    }

    durationTimer_.setInterval(1000);

    // for scroll-wheel event buffering,
    // used to avoid frequent seeking by small distances:
    scrollWheelTimer_.setSingleShot(true);

    // for re-running auto-crop soon after loading next file in the playlist:
    autocropTimer_.setSingleShot(true);

    // update a bookmark every 3 minutes during playback:
    bookmarkTimer_.setInterval(180000);

    bookmarksMenuSeparator_ =
      menuBookmarks->insertSeparator(actionRemoveBookmarkNowPlaying);

    bool automaticBookmarks =
      loadBooleanSettingOrDefault(kCreateBookmarksAutomatically, true);
    actionAutomaticBookmarks->setChecked(automaticBookmarks);

    bool resumeFromBookmark =
      loadBooleanSettingOrDefault(kResumePlaybackFromBookmark, true);
    actionResumeFromBookmark->setChecked(resumeFromBookmark);

    bool downmixToStereo =
      loadBooleanSettingOrDefault(kDownmixToStereo, true);
    actionDownmixToStereo->setChecked(downmixToStereo);

    bool skipColorConverter =
      loadBooleanSettingOrDefault(kSkipColorConverter, false);
    actionSkipColorConverter->setChecked(skipColorConverter);

    bool skipLoopFilter =
      loadBooleanSettingOrDefault(kSkipLoopFilter, false);
    actionSkipLoopFilter->setChecked(skipLoopFilter);

    bool skipNonReferenceFrames =
      loadBooleanSettingOrDefault(kSkipNonReferenceFrames, false);
    actionSkipNonReferenceFrames->setChecked(skipNonReferenceFrames);

    bool deinterlaceFrames =
      loadBooleanSettingOrDefault(kDeinterlaceFrames, false);
    actionDeinterlace->setChecked(deinterlaceFrames);

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);
    shortcutFillScreen_ = new QShortcut(this);
    shortcutShowPlaylist_ = new QShortcut(this);
    shortcutShowTimeline_ = new QShortcut(this);
    shortcutPlay_ = new QShortcut(this);
    shortcutNext_ = new QShortcut(this);
    shortcutPrev_ = new QShortcut(this);
    shortcutLoop_ = new QShortcut(this);
    shortcutCropNone_ = new QShortcut(this);
    shortcutCrop1_33_ = new QShortcut(this);
    shortcutCrop1_78_ = new QShortcut(this);
    shortcutCrop1_85_ = new QShortcut(this);
    shortcutCrop2_40_ = new QShortcut(this);
    shortcutAutoCrop_ = new QShortcut(this);
    shortcutNextChapter_ = new QShortcut(this);
    shortcutRemove_ = new QShortcut(this);
    shortcutSelectAll_ = new QShortcut(this);
    shortcutAspectRatioNone_ = new QShortcut(this);
    shortcutAspectRatio1_33_ = new QShortcut(this);
    shortcutAspectRatio1_78_ = new QShortcut(this);

    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);
    shortcutFillScreen_->setContext(Qt::ApplicationShortcut);
    shortcutShowPlaylist_->setContext(Qt::ApplicationShortcut);
    shortcutShowTimeline_->setContext(Qt::ApplicationShortcut);
    shortcutPlay_->setContext(Qt::ApplicationShortcut);
    shortcutNext_->setContext(Qt::ApplicationShortcut);
    shortcutPrev_->setContext(Qt::ApplicationShortcut);
    shortcutLoop_->setContext(Qt::ApplicationShortcut);
    shortcutCropNone_->setContext(Qt::ApplicationShortcut);
    shortcutCrop1_33_->setContext(Qt::ApplicationShortcut);
    shortcutCrop1_78_->setContext(Qt::ApplicationShortcut);
    shortcutCrop1_85_->setContext(Qt::ApplicationShortcut);
    shortcutCrop2_40_->setContext(Qt::ApplicationShortcut);
    shortcutAutoCrop_->setContext(Qt::ApplicationShortcut);
    shortcutNextChapter_->setContext(Qt::ApplicationShortcut);
    shortcutAspectRatioNone_->setContext(Qt::ApplicationShortcut);
    shortcutAspectRatio1_33_->setContext(Qt::ApplicationShortcut);
    shortcutAspectRatio1_78_->setContext(Qt::ApplicationShortcut);

    shortcutRemove_->setContext(Qt::ApplicationShortcut);
    shortcutSelectAll_->setContext(Qt::ApplicationShortcut);

    shortcutRemove_->setKey(QKeySequence(QKeySequence::Delete));
    shortcutSelectAll_->setKey(QKeySequence(QKeySequence::SelectAll));

    actionRemove_ = new QAction(this);
    actionRemove_->setObjectName(QString::fromUtf8("actionRemove_"));
    actionRemove_->setShortcutContext(Qt::ApplicationShortcut);
    actionRemove_->setText(tr("&Remove Selected"));
    actionRemove_->setShortcut(tr("Delete"));

    actionSelectAll_ = new QAction(this);
    actionSelectAll_->setObjectName(QString::fromUtf8("actionSelectAll_"));
    actionSelectAll_->setShortcutContext(Qt::ApplicationShortcut);
    actionSelectAll_->setText(tr("&Select All"));
    actionSelectAll_->setShortcut(tr("Ctrl+A"));

    QActionGroup * aspectRatioGroup = new QActionGroup(this);
    aspectRatioGroup->addAction(actionAspectRatioAuto);
    aspectRatioGroup->addAction(actionAspectRatio1_33);
    aspectRatioGroup->addAction(actionAspectRatio1_60);
    aspectRatioGroup->addAction(actionAspectRatio1_78);
    aspectRatioGroup->addAction(actionAspectRatio1_85);
    aspectRatioGroup->addAction(actionAspectRatio2_35);
    aspectRatioGroup->addAction(actionAspectRatio2_40);
    aspectRatioGroup->addAction(actionAspectRatioOther);
    actionAspectRatioAuto->setChecked(true);

    QActionGroup * cropFrameGroup = new QActionGroup(this);
    cropFrameGroup->addAction(actionCropFrameNone);
    cropFrameGroup->addAction(actionCropFrame1_33);
    cropFrameGroup->addAction(actionCropFrame1_60);
    cropFrameGroup->addAction(actionCropFrame1_78);
    cropFrameGroup->addAction(actionCropFrame1_85);
    cropFrameGroup->addAction(actionCropFrame2_35);
    cropFrameGroup->addAction(actionCropFrame2_40);
    cropFrameGroup->addAction(actionCropFrameAutoDetect);
    actionCropFrameNone->setChecked(true);

    QActionGroup * playRateGroup = new QActionGroup(this);
    playRateGroup->addAction(actionTempo50);
    playRateGroup->addAction(actionTempo60);
    playRateGroup->addAction(actionTempo70);
    playRateGroup->addAction(actionTempo80);
    playRateGroup->addAction(actionTempo90);
    playRateGroup->addAction(actionTempo100);
    playRateGroup->addAction(actionTempo111);
    playRateGroup->addAction(actionTempo125);
    playRateGroup->addAction(actionTempo143);
    playRateGroup->addAction(actionTempo167);
    playRateGroup->addAction(actionTempo200);
    actionTempo100->setChecked(true);

    yae::SignalMapper * playRateMapper = new yae::SignalMapper(this);
    playRateMapper->setMapping(actionTempo50, 50);
    playRateMapper->setMapping(actionTempo60, 60);
    playRateMapper->setMapping(actionTempo70, 70);
    playRateMapper->setMapping(actionTempo80, 80);
    playRateMapper->setMapping(actionTempo90, 90);
    playRateMapper->setMapping(actionTempo100, 100);
    playRateMapper->setMapping(actionTempo111, 111);
    playRateMapper->setMapping(actionTempo125, 125);
    playRateMapper->setMapping(actionTempo143, 143);
    playRateMapper->setMapping(actionTempo167, 167);
    playRateMapper->setMapping(actionTempo200, 200);

    bool ok = true;
    ok = connect(playRateMapper, SIGNAL(mapped_to(int)),
                 this, SLOT(playbackSetTempo(int)));
    YAE_ASSERT(ok);

    ok = connect(actionTempo50, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo60, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo70, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo80, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo90, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo100, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo111, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo125, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo143, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo167, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo200, SIGNAL(triggered()),
                 playRateMapper, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionNewWindow, SIGNAL(triggered()),
                 this, SLOT(fileNewWindow()));
    YAE_ASSERT(ok);

    ok = connect(actionOpen, SIGNAL(triggered()),
                 this, SLOT(fileOpen()));
    YAE_ASSERT(ok);

    ok = connect(actionOpenURL, SIGNAL(triggered()),
                 this, SLOT(fileOpenURL()));
    YAE_ASSERT(ok);

    ok = connect(actionOpenFolder, SIGNAL(triggered()),
                 this, SLOT(fileOpenFolder()));
    YAE_ASSERT(ok);

    ok = connect(actionExit, SIGNAL(triggered()),
                 this, SLOT(fileExit()));
    YAE_ASSERT(ok);

    ok = connect(shortcutExit_, SIGNAL(activated()),
                 actionExit, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioAuto, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioAuto()));
    YAE_ASSERT(ok);

    ok = connect(shortcutAspectRatioNone_, SIGNAL(activated()),
                 actionAspectRatioAuto, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_33, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_33()));
    YAE_ASSERT(ok);

    ok = connect(shortcutAspectRatio1_33_, SIGNAL(activated()),
                 actionAspectRatio1_33, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_60, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_60()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_78, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_78()));
    YAE_ASSERT(ok);

    ok = connect(shortcutAspectRatio1_78_, SIGNAL(activated()),
                 actionAspectRatio1_78, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_85, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_85()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio2_35, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_35()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio2_40, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio2_40()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatioOther, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatioOther()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameNone, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameNone()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCropNone_, SIGNAL(activated()),
                 actionCropFrameNone, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_33, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_33()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCrop1_33_, SIGNAL(activated()),
                 actionCropFrame1_33, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_60, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_60()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_78, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_78()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCrop1_78_, SIGNAL(activated()),
                 actionCropFrame1_78, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame1_85, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame1_85()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCrop1_85_, SIGNAL(activated()),
                 actionCropFrame1_85, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame2_35, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_35()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame2_40, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_40()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCrop2_40_, SIGNAL(activated()),
                 actionCropFrame2_40, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrameAutoDetect, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(shortcutAutoCrop_, SIGNAL(activated()),
                 actionCropFrameAutoDetect, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionPlay, SIGNAL(triggered()),
                 this, SLOT(togglePlayback()));
    YAE_ASSERT(ok);

    ok = connect(shortcutPlay_, SIGNAL(activated()),
                 actionPlay, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(playbackToggle_, SIGNAL(clicked()),
                 actionPlay, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionNext, SIGNAL(triggered()),
                 this, SLOT(playbackNext()));
    YAE_ASSERT(ok);

    ok = connect(shortcutNext_, SIGNAL(activated()),
                 actionNext, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionPrev, SIGNAL(triggered()),
                 this, SLOT(playbackPrev()));
    YAE_ASSERT(ok);

    ok = connect(shortcutPrev_, SIGNAL(activated()),
                 actionPrev, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionLoop, SIGNAL(triggered()),
                 this, SLOT(playbackLoop()));
    YAE_ASSERT(ok);

    ok = connect(shortcutLoop_, SIGNAL(activated()),
                 actionLoop, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionSetInPoint_, SIGNAL(triggered()),
                 this, SIGNAL(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionSetOutPoint_, SIGNAL(triggered()),
                 this, SIGNAL(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionRemove_, SIGNAL(triggered()),
                 playlistWidget_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(actionSelectAll_, SIGNAL(triggered()),
                 playlistWidget_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(shortcutRemove_, SIGNAL(activated()),
                 playlistWidget_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSelectAll_, SIGNAL(activated()),
                 playlistWidget_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(playlistWidget_, SIGNAL(currentItemChanged(std::size_t)),
                 this, SLOT(playlistItemChanged(std::size_t)));
    YAE_ASSERT(ok);

    ok = connect(fullscreenToggle_, SIGNAL(clicked()),
                 this, SLOT(toggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 actionFullScreen, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionFillScreen, SIGNAL(triggered()),
                 this, SLOT(playbackFillScreen()));
    YAE_ASSERT(ok);

    ok = connect(shortcutFillScreen_, SIGNAL(activated()),
                 actionFillScreen, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionShrinkWrap, SIGNAL(triggered()),
                 this, SLOT(playbackShrinkWrap()));
    YAE_ASSERT(ok);

    ok = connect(actionShowPlaylist, SIGNAL(triggered()),
                 this, SLOT(playbackShowPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(shortcutShowPlaylist_, SIGNAL(activated()),
                 actionShowPlaylist, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionShowTimeline, SIGNAL(triggered()),
                 this, SLOT(playbackShowTimeline()));
    YAE_ASSERT(ok);

    ok = connect(shortcutShowTimeline_, SIGNAL(activated()),
                 actionShowTimeline, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipColorConverter, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipLoopFilter, SIGNAL(triggered()),
                 this, SLOT(playbackLoopFilter()));
    YAE_ASSERT(ok);

    ok = connect(actionSkipNonReferenceFrames, SIGNAL(triggered()),
                 this, SLOT(playbackNonReferenceFrames()));
    YAE_ASSERT(ok);

    ok = connect(actionVerticalScaling, SIGNAL(triggered()),
                 this, SLOT(playbackVerticalScaling()));
    YAE_ASSERT(ok);

    ok = connect(actionDeinterlace, SIGNAL(triggered()),
                 this, SLOT(playbackColorConverter()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(toggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(escLong()),
                 this, SLOT(toggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(escShort()),
                 actionShowPlaylist, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&(canvas_->sigs_), SIGNAL(maybeHideCursor()),
                 &(canvas_->sigs_), SLOT(hideCursor()));
    YAE_ASSERT(ok);

    ok = connect(actionHalfSize, SIGNAL(triggered()),
                 this, SLOT(windowHalfSize()));
    YAE_ASSERT(ok);

    ok = connect(actionFullSize, SIGNAL(triggered()),
                 this, SLOT(windowFullSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDoubleSize, SIGNAL(triggered()),
                 this, SLOT(windowDoubleSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDecreaseSize, SIGNAL(triggered()),
                 this, SLOT(windowDecreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(actionIncreaseSize, SIGNAL(triggered()),
                 this, SLOT(windowIncreaseSize()));
    YAE_ASSERT(ok);

    ok = connect(actionDownmixToStereo, SIGNAL(triggered()),
                 this, SLOT(audioDownmixToStereo()));
    YAE_ASSERT(ok);

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setInPoint()),
                 &(timelineControls_->model_), SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setOutPoint()),
                 &(timelineControls_->model_), SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(&(timelineControls_->model_), SIGNAL(userIsSeeking(bool)),
                 this, SLOT(userIsSeeking(bool)));
    YAE_ASSERT(ok);

    ok = connect(&(timelineControls_->model_), SIGNAL(moveTimeIn(double)),
                 this, SLOT(moveTimeIn(double)));
    YAE_ASSERT(ok);

    ok = connect(&(timelineControls_->model_), SIGNAL(moveTimeOut(double)),
                 this, SLOT(moveTimeOut(double)));
    YAE_ASSERT(ok);

    ok = connect(&(timelineControls_->model_), SIGNAL(movePlayHead(double)),
                 this, SLOT(movePlayHead(double)));
    YAE_ASSERT(ok);

    ok = connect(&(timelineControls_->model_),
                 SIGNAL(clockStopped(const SharedClock &)),
                 this,
                 SLOT(playbackFinished(const SharedClock &)));
    YAE_ASSERT(ok);

    ok = connect(&scrollWheelTimer_, SIGNAL(timeout()),
                 this, SLOT(scrollWheelTimerExpired()));
    YAE_ASSERT(ok);

    ok = connect(playlistDock_, SIGNAL(visibilityChanged(bool)),
                 this, SLOT(playlistVisibilityChanged(bool)));
    YAE_ASSERT(ok);

    ok = connect(playlistFilter_, SIGNAL(textChanged(const QString &)),
                 playlistWidget_, SLOT(filterChanged(const QString &)));
    YAE_ASSERT(ok);

    ok = connect(playlistFilter_, SIGNAL(textChanged(const QString &)),
                 this, SLOT(fixupNextPrev()));
    YAE_ASSERT(ok);

    ok = connect(&durationTimer_, SIGNAL(timeout()),
                 this, SLOT(updateTimelineDuration()));
    YAE_ASSERT(ok);

    ok = connect(&autocropTimer_, SIGNAL(timeout()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(&bookmarkTimer_, SIGNAL(timeout()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);

    ok = connect(actionAutomaticBookmarks, SIGNAL(triggered()),
                 this, SLOT(bookmarksAutomatic()));
    YAE_ASSERT(ok);

    ok = connect(menuBookmarks, SIGNAL(aboutToShow()),
                 this, SLOT(bookmarksPopulate()));
    YAE_ASSERT(ok);

    ok = connect(actionRemoveBookmarkNowPlaying, SIGNAL(triggered()),
                 this, SLOT(bookmarksRemoveNowPlaying()));
    YAE_ASSERT(ok);

    ok = connect(actionRemoveBookmarks, SIGNAL(triggered()),
                 this, SLOT(bookmarksRemove()));
    YAE_ASSERT(ok);

    ok = connect(actionResumeFromBookmark, SIGNAL(triggered()),
                 this, SLOT(bookmarksResumePlayback()));
    YAE_ASSERT(ok);

    ok = connect(menuChapters_, SIGNAL(aboutToShow()),
                 this, SLOT(updateChaptersMenu()));
    YAE_ASSERT(ok);

    ok = connect(actionNextChapter, SIGNAL(triggered()),
                 this, SLOT(skipToNextChapter()));
    YAE_ASSERT(ok);

    ok = connect(shortcutNextChapter_, SIGNAL(activated()),
                 actionNextChapter, SLOT(trigger()));
    YAE_ASSERT(ok);

    adjustMenuActions();
    adjustMenus(reader_.get());
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    delete openUrl_;
    openUrl_ = NULL;

    reader_->close();
    audioRenderer_->close();
    videoRenderer_->close();

    reader_->destroy();
    audioRenderer_->destroy();
    videoRenderer_->destroy();

    delete canvas_;
    canvas_ = NULL;
  }

  //----------------------------------------------------------------
  // MainWindow::isPlaybackPaused
  //
  bool
  MainWindow::isPlaybackPaused() const
  {
    return playbackPaused_;
  }

  //----------------------------------------------------------------
  // IsPlaybackPaused
  //
  struct IsPlaybackPaused : public TBoolExpr
  {
    IsPlaybackPaused(const MainWindow & window):
      window_(window)
    {}

    // virtual:
    void evaluate(bool & result) const
    {
      result = window_.isPlaybackPaused();
    }

    const MainWindow & window_;
  };

  //----------------------------------------------------------------
  // toggle_playback
  //
  static void
  toggle_playback(void * context)
  {
    yae_debug << "TOGGLE PLAYBACK!";
    MainWindow * window = (MainWindow *)context;
    yae::queue_call(*window, &MainWindow::togglePlayback);
  }

  //----------------------------------------------------------------
  // is_playback_paused
  //
  static bool
  is_playback_paused(void * context, bool & playback_paused)
  {
    MainWindow * window = (MainWindow *)context;
    playback_paused = window->isPlaybackPaused();
    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
    canvas_->initializePrivateBackend();

    TMakeCurrentContext currentContext(canvas_->Canvas::context());
    view_.toggle_playback_.reset(&yae::toggle_playback, this);
    view_.is_playback_paused_.reset(&yae::is_playback_paused, this);
    view_.setStyle(&style_);
    canvas_->append(&view_);
    view_.setEnabled(true);
  }

  //----------------------------------------------------------------
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const std::list<QString> & playlist,
                          bool beginPlaybackImmediately)
  {
    SignalBlocker blockSignals(playlistWidget_);

    bool resumeFromBookmark = actionResumeFromBookmark->isChecked();

    std::list<BookmarkHashInfo> hashInfo;
    playlistWidget_->add(playlist, resumeFromBookmark ? &hashInfo : NULL);

    if (!beginPlaybackImmediately)
    {
      return;
    }

    if (resumeFromBookmark)
    {
      // look for a matching bookmark, resume playback if a bookmark exist:
      PlaylistBookmark bookmark;
      const PlaylistItem * found = NULL;

      for (std::list<BookmarkHashInfo>::const_iterator i = hashInfo.begin();
           !found && i != hashInfo.end(); ++i)
      {
        // shortcuts:
        const std::string & groupHash = i->groupHash_;

        if (!yae::loadBookmark(groupHash, bookmark))
        {
          continue;
        }

        // shortcut:
        const std::list<std::string> & itemHashList = i->itemHash_;
        for (std::list<std::string>::const_iterator j = itemHashList.begin();
             !found && j != itemHashList.end(); ++j)
        {
          const std::string & itemHash = *j;
          if (itemHash == bookmark.itemHash_)
          {
            found = playlistWidget_->lookup(bookmark.groupHash_,
                                            bookmark.itemHash_,
                                            &bookmark.itemIndex_);
            if (found->excluded_)
            {
              found = NULL;
            }
          }
        }
      }

      if (found)
      {
        gotoBookmark(bookmark);
        return;
      }
    }

    playback();
  }

  //----------------------------------------------------------------
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const QString & filename)
  {
    std::list<QString> playlist;
    yae::addToPlaylist(playlist, filename);
    this->setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // hasFileExt
  //
  inline static bool
  hasFileExt(const QString & fn, const char * ext)
  {
    return fn.endsWith(QString::fromUtf8(ext), Qt::CaseInsensitive);
  }

  //----------------------------------------------------------------
  // canaryTest
  //
  static bool
  canaryTest(const QString & fn)
  {
    bool modFile =
      hasFileExt(fn, ".xm") ||
      hasFileExt(fn, ".ult") ||
      hasFileExt(fn, ".s3m") ||
      hasFileExt(fn, ".ptm") ||
      hasFileExt(fn, ".plm") ||
      hasFileExt(fn, ".mus") ||
      hasFileExt(fn, ".mtm") ||
      hasFileExt(fn, ".mod") ||
      hasFileExt(fn, ".med") ||
      hasFileExt(fn, ".mdl") ||
      hasFileExt(fn, ".md") ||
      hasFileExt(fn, ".lbm") ||
      hasFileExt(fn, ".it") ||
      hasFileExt(fn, ".hsp") ||
      hasFileExt(fn, ".dsm") ||
      hasFileExt(fn, ".dmf") ||
      hasFileExt(fn, ".dat") ||
      hasFileExt(fn, ".cfg") ||
      hasFileExt(fn, ".bmw") ||
      hasFileExt(fn, ".ams") ||
      hasFileExt(fn, ".669");

    if (!modFile)
    {
      return true;
    }

    QProcess canary;

    QString exePath = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QString::fromUtf8("--canary");
    args << fn;

    // send in the canary:
    canary.start(exePath, args);

    if (!canary.waitForFinished(30000))
    {
      // failed to finish in 30 seconds, assume it hanged:
      return false;
    }

    int exitCode = canary.exitCode();
    QProcess::ExitStatus canaryStatus = canary.exitStatus();

    if (canaryStatus != QProcess::NormalExit || exitCode)
    {
      // dead canary:
      return false;
    }

    // seems to be safe enough to at least open the file,
    // may still die during playback:
    return true;
  }

  //----------------------------------------------------------------
  // ReaderEventHandler
  //
  struct ReaderEventHandler : IEventObserver
  {
    ReaderEventHandler(MainWindow & main_window, const IReaderPtr & reader):
      main_window_(main_window),
      reader_(reader)
    {}

    virtual void note(const Json::Value & event)
    {
      IReaderPtr reader = reader_.lock();
      if (reader)
      {
        main_window_.handle_reader_event(reader, event);
      }
    }

    MainWindow & main_window_;
    IReaderWPtr reader_;
  };

  //----------------------------------------------------------------
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path, const TBookmark * bookmark)
  {
    QString fn = path;
    QFileInfo fi(fn);
    if (fi.suffix() == kExtEyetv)
    {
      std::list<QString> found;
      findFiles(found, path, false);

      if (!found.empty())
      {
        fn = found.front();
      }
    }

    actionPlay->setEnabled(false);

    IReaderPtr reader_ptr;
    if (canaryTest(path))
    {
      bool hwdec = true;
      reader_ptr = yae::openFile(readerFactory_, path, hwdec);
    }

    if (!reader_ptr)
    {
#if 0
      yae_debug
        << "ERROR: could not open file: " << fn.toUtf8().constData();
#endif
      return false;
    }

    ++readerId_;
    IReader * reader = reader_ptr.get();
    reader->setReaderId(readerId_);

    TEventObserverPtr eo(new ReaderEventHandler(*this, reader_ptr));
    reader->setEventObserver(eo);

    // prevent Canvas from rendering any pending frames from previous reader:
    canvas_->acceptFramesWithReaderId(readerId_);

    // disconnect timeline from renderers:
    timelineControls_->model_.observe(SharedClock());

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t subsCount = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackEnabled(!playbackPaused_);

    std::vector<TTrackInfo>  audioInfo;
    std::vector<AudioTraits> audioTraits;
    std::vector<TTrackInfo>  videoInfo;
    std::vector<VideoTraits> videoTraits;
    std::vector<TTrackInfo>  subsInfo;
    std::vector<TSubsFormat> subsFormat;

    adjustMenuActions(reader,
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat);

    reader_->close();
    videoRenderer_->close();
    audioRenderer_->close();

    bool rememberSelectedVideoTrack = false;
    std::size_t program = find_matching_program(videoInfo, selVideo_);
    std::size_t vtrack = find_matching_track<VideoTraits>(videoInfo,
                                                          videoTraits,
                                                          selVideo_,
                                                          selVideoTraits_,
                                                          program);

    if (bookmark && bookmark->vtrack_ <= numVideoTracks)
    {
      vtrack = bookmark->vtrack_;
      rememberSelectedVideoTrack = numVideoTracks > 0;
    }

    bool rememberSelectedAudioTrack = false;
    std::size_t atrack = find_matching_track<AudioTraits>(audioInfo,
                                                          audioTraits,
                                                          selAudio_,
                                                          selAudioTraits_,
                                                          program);
    if (bookmark && bookmark->atrack_ <= numAudioTracks)
    {
      atrack = bookmark->atrack_;
      rememberSelectedAudioTrack = numAudioTracks > 0;
    }

    if (vtrack >= numVideoTracks &&
        atrack >= numAudioTracks)
    {
      // avoid disabling both audio and video due to
      // previous custom or bookmarked track selections:

      if (numVideoTracks)
      {
        vtrack = 0;
      }
      else if (numAudioTracks)
      {
        atrack = 0;
      }
    }

    selectVideoTrack(reader, vtrack);
    videoTrackGroup_->actions().at((int)vtrack)->setChecked(true);

    if (rememberSelectedVideoTrack)
    {
      std::size_t i = reader->getSelectedVideoTrackIndex();
      reader->getVideoTrackInfo(i, selVideo_, selVideoTraits_);
    }

    selectAudioTrack(reader, atrack);
    audioTrackGroup_->actions().at((int)atrack)->setChecked(true);

    if (rememberSelectedAudioTrack)
    {
      std::size_t i = reader->getSelectedAudioTrackIndex();
      reader->getAudioTrackInfo(i, selAudio_, selAudioTraits_);
    }

    bool rememberSelectedSubtitlesTrack = false;
    std::size_t strack = selClosedCaptions_ ?
      (subsCount + selClosedCaptions_) :
      find_matching_track<TSubsFormat>(subsInfo,
                                       subsFormat,
                                       selSubs_,
                                       selSubsFormat_,
                                       program);

    if (bookmark)
    {
      if (!bookmark->subs_.empty() &&
          bookmark->subs_.front() != subsCount)
      {
        strack = bookmark->subs_.front();
        rememberSelectedSubtitlesTrack = true;
      }
      else if (bookmark->subs_.empty())
      {
        strack = subsCount + bookmark->cc_;
        rememberSelectedSubtitlesTrack = true;
      }
    }

    selectSubsTrack(reader, strack);

    int subsActionIndex =
      (strack < subsCount) ? strack :
      (subsCount < strack) ? strack - 1:
      subsCount + 4;
    subsTrackGroup_->actions().at(subsActionIndex)->setChecked(true);

    if (rememberSelectedSubtitlesTrack)
    {
      selSubsFormat_ = reader->subsInfo(strack, selSubs_);
    }

    adjustMenus(reader);

    // reset overlay plane to clean state, reset libass wrapper:
    canvas_->clearOverlay();

    // reset timeline start, duration, playhead, in/out points:
    timelineControls_->model_.resetFor(reader);

    if (bookmark)
    {
      // skip to bookmarked position:
      reader->seek(bookmark->positionInSeconds_);
    }

    // process attachments:
    std::size_t numAttachments = reader->getNumberOfAttachments();
    for (std::size_t i = 0; i < numAttachments; i++)
    {
      const TAttachment * att = reader->getAttachmentInfo(i);

      typedef std::map<std::string, std::string>::const_iterator TIter;
      TIter mimetypeFound = att->metadata_.find(std::string("mimetype"));
      TIter filenameFound = att->metadata_.find(std::string("filename"));

      const std::string & filename =
        filenameFound != att->metadata_.end() ?
        filenameFound->second : std::string();

      if (mimetypeFound != att->metadata_.end())
      {
        static const std::string fontTypes[] = {
          std::string("application/x-truetype-font"),
          std::string("application/vnd.ms-opentype"),
          std::string("application/x-font-ttf"),
          std::string("application/x-font")
        };

        static const std::size_t numFontTypes =
          sizeof(fontTypes) / sizeof(fontTypes[0]);

        std::string mimetype(mimetypeFound->second);
        boost::algorithm::to_lower(mimetype);

        for (std::size_t j = 0; j < numFontTypes; j++)
        {
          if (mimetype == fontTypes[j])
          {
            canvas_->libassAddFont(filename, att->data_);
            break;
          }
        }
      }
#if 0
      else
      {
        yae_debug << "attachment: " << att->size_ << " bytes";

        for (std::map<std::string, std::string>::const_iterator
               j = att->metadata_.begin(); j != att->metadata_.end(); ++j)
        {
          yae_debug << "  " << j->first << ": " << j->second;
        }
      }
#endif
    }

    // (re)init libass:
    canvas_->libassAsyncInit();

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepareReaderAndRenderers(reader_ptr, playbackPaused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // replace the previous reader:
    reader_ = reader_ptr;

    // allow renderers to read from output frame queues:
    resumeRenderers(true);

    this->setWindowTitle(tr("Apprentice Video Classic: %1").
                         arg(QFileInfo(path).fileName()));
    actionPlay->setEnabled(true);

    if (actionCropFrameAutoDetect->isChecked())
    {
      autocropTimer_.start(1900);
    }
    else
    {
      QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
    }

    bookmarkTimer_.start();
    durationTimer_.start();

    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpenFolder
  //
  void
  MainWindow::fileOpenFolder()
  {
    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);

    QString folder =
      QFileDialog::getExistingDirectory(this,
                                        tr("Select a video folder"),
                                        startHere,
                                        QFileDialog::ShowDirsOnly |
                                        QFileDialog::DontResolveSymlinks);

    // find all files in the folder, sorted alphabetically
    std::list<QString> playlist;
    if (yae::addFolderToPlaylist(playlist, folder))
    {
      setPlaylist(playlist);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpenURL
  //
  void
  MainWindow::fileOpenURL()
  {
    openUrl_->lineEdit->setFocus();
    openUrl_->lineEdit->selectAll();

    int r = openUrl_->exec();
    if (r == QDialog::Accepted)
    {
      QString url = openUrl_->lineEdit->text();

      std::list<QString> playlist;
      playlist.push_back(url);

      bool beginPlaybackImmediately = false;
      setPlaylist(playlist, beginPlaybackImmediately);

      // begin playback:
      playback();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpen
  //
  void
  MainWindow::fileOpen()
  {
    QString filter =
      tr("movies ("
         "*.avi "
         "*.asf "
         "*.divx "
         "*.eyetv "
         "*.flv "
         "*.f4v "
         "*.m2t "
         "*.m2ts "
         "*.m4v "
         "*.mkv "
         "*.mod "
         "*.mov "
         "*.mpg "
         "*.mp4 "
         "*.mpeg "
         "*.mpts "
         "*.ogm "
         "*.ogv "
         "*.ts "
         "*.wmv "
         "*.webm "
         ")");

    QString startHere = YAE_STANDARD_LOCATION(MoviesLocation);

#ifndef __APPLE__
    QStringList filenames =
      QFileDialog::getOpenFileNames(this,
                                    tr("Select one or more files"),
                                    startHere,
                                    filter);
#else
    QFileDialog dialog(this,
                       tr("Select one or more files"),
                       startHere,
                       filter);
    int r = dialog.exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    QStringList filenames = dialog.selectedFiles();
#endif

    if (filenames.empty())
    {
      return;
    }

    std::list<QString> playlist;
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
    {
      yae::addToPlaylist(playlist, *i);
    }

    setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // MainWindow::fileExit
  //
  void
  MainWindow::fileExit()
  {
    reader_->close();
    videoRenderer_->close();
    audioRenderer_->close();
    MainWindow::close();
    qApp->quit();
  }

  //----------------------------------------------------------------
  // MainWindow::fileNewWindow
  //
  void
  MainWindow::fileNewWindow()
  {
    QString exePath = QCoreApplication::applicationFilePath();
    QStringList args;

    if (!QProcess::startDetached(exePath, args))
    {
      actionNewWindow->setEnabled(false);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksAutomatic
  //
  void
  MainWindow::bookmarksAutomatic()
  {
    saveBooleanSetting(kCreateBookmarksAutomatically,
                       actionAutomaticBookmarks->isChecked());
  }

  //----------------------------------------------------------------
  // escapeAmpersand
  //
  static QString
  escapeAmpersand(const QString & menuText)
  {
    QString out = menuText;
    out.replace(QString::fromUtf8("&"), QString::fromUtf8("&&"));
    return out;
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksPopulate
  //
  void
  MainWindow::bookmarksPopulate()
  {
    delete bookmarksGroup_;
    bookmarksGroup_ = NULL;

    delete bookmarksMapper_;
    bookmarksMapper_ = NULL;

    bookmarksMenuSeparator_->setVisible(false);
    actionRemoveBookmarkNowPlaying->setVisible(false);
    actionRemoveBookmarks->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;
    }

    bookmarks_.clear();

    std::size_t itemIndexNowPlaying = playlistWidget_->currentItem();
    std::size_t itemIndex = 0;
    std::size_t playingBookmarkIndex = std::numeric_limits<std::size_t>::max();

    while (true)
    {
      PlaylistGroup * group = playlistWidget_->lookupGroup(itemIndex);
      std::size_t groupSize = group ? group->items_.size() : 0;
      if (!groupSize)
      {
        break;
      }

      itemIndex += groupSize;

      if (group->excluded_)
      {
        continue;
      }

      // check whether there is a bookmark for an item in this group:
      PlaylistBookmark bookmark;
      if (!yae::loadBookmark(group->bookmarkHash_, bookmark))
      {
        continue;
      }

      // check whether the item hash matches a group item:
      for (std::size_t i = 0; i < groupSize; i++)
      {
        const PlaylistItem & item = group->items_[i];
        if (item.excluded_ || item.bookmarkHash_ != bookmark.itemHash_)
        {
          continue;
        }

        // found a match, add it to the bookmarks menu:
        bookmark.itemIndex_ = group->offset_ + i;

        std::string ts = TTime(bookmark.positionInSeconds_).to_hhmmss(":");

        if (!bookmarksGroup_)
        {
          bookmarksGroup_ = new QActionGroup(this);
          bookmarksMapper_ = new yae::SignalMapper(this);

          bool ok = connect(bookmarksMapper_, SIGNAL(mapped_to(int)),
                            this, SLOT(bookmarksSelectItem(int)));
          YAE_ASSERT(ok);

          bookmarksMenuSeparator_->setVisible(true);
          actionRemoveBookmarks->setEnabled(true);
        }

        QString name =
          escapeAmpersand(group->name_) +
#ifdef __APPLE__
          // right-pointing double angle bracket:
          QString::fromUtf8(" ""\xc2""\xbb"" ") +
#else
          QString::fromUtf8("\t") +
#endif
          escapeAmpersand(item.name_) +
          QString::fromUtf8(", ") +
          QString::fromUtf8(ts.c_str());

        bookmark.action_ = new QAction(name, this);

        bool nowPlaying = (itemIndexNowPlaying == bookmark.itemIndex_);
        if (nowPlaying)
        {
          playingBookmarkIndex = bookmarks_.size();
        }

        bookmark.action_->setCheckable(true);
        bookmark.action_->setChecked(nowPlaying);

        menuBookmarks->insertAction(bookmarksMenuSeparator_, bookmark.action_);
        bookmarksGroup_->addAction(bookmark.action_);

        bool ok = connect(bookmark.action_, SIGNAL(triggered()),
                          bookmarksMapper_, SLOT(map()));
        YAE_ASSERT(ok);

        bookmarksMapper_->setMapping(bookmark.action_,
                                     (int)(bookmarks_.size()));

        bookmarks_.push_back(bookmark);
      }
    }

    if (playingBookmarkIndex != std::numeric_limits<std::size_t>::max())
    {
      actionRemoveBookmarkNowPlaying->setVisible(true);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksRemoveNowPlaying
  //
  void
  MainWindow::bookmarksRemoveNowPlaying()
  {
    std::size_t itemIndex = playlistWidget_->currentItem();
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlistWidget_->lookup(itemIndex, &group);
    if (!item || !group)
    {
      return;
    }

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      if (bookmark.groupHash_ != group->bookmarkHash_ ||
          bookmark.itemHash_ != item->bookmarkHash_)
      {
        continue;
      }

      yae::removeBookmark(bookmark.groupHash_);
      break;
    }
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksRemove
  //
  void
  MainWindow::bookmarksRemove()
  {
    delete bookmarksGroup_;
    bookmarksGroup_ = NULL;

    delete bookmarksMapper_;
    bookmarksMapper_ = NULL;

    bookmarksMenuSeparator_->setVisible(false);
    actionRemoveBookmarkNowPlaying->setVisible(false);
    actionRemoveBookmarks->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;

      yae::removeBookmark(bookmark.groupHash_);
    }

    bookmarks_.clear();
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksResumePlayback
  //
  void
  MainWindow::bookmarksResumePlayback()
  {
    saveBooleanSetting(kResumePlaybackFromBookmark,
                       actionResumeFromBookmark->isChecked());
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksSelectItem
  //
  void
  MainWindow::bookmarksSelectItem(int index)
  {
    if (index >= (int)bookmarks_.size())
    {
      return;
    }

    const PlaylistBookmark & bookmark = bookmarks_[index];
    gotoBookmark(bookmark);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatioAuto
  //
  void
  MainWindow::playbackAspectRatioAuto()
  {
    canvas_->overrideDisplayAspectRatio(0.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatioOther
  //
  void
  MainWindow::playbackAspectRatioOther()
  {
    static AspectRatioDialog * aspectRatioDialog = NULL;
    if (!aspectRatioDialog)
    {
      aspectRatioDialog = new AspectRatioDialog(this);
    }

    double w = 0.0;
    double h = 0.0;
    double dar = canvas_->imageAspectRatio(w, h);
    dar = dar != 0.0 ? dar : 1.777777;
    aspectRatioDialog->doubleSpinBox->setValue(dar);

    int r = aspectRatioDialog->exec();
    if (r != QDialog::Accepted)
    {
      return;
    }

    dar = aspectRatioDialog->doubleSpinBox->value();
    canvas_->overrideDisplayAspectRatio(dar);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_40
  //
  void
  MainWindow::playbackAspectRatio2_40()
  {
    canvas_->overrideDisplayAspectRatio(2.40);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio2_35
  //
  void
  MainWindow::playbackAspectRatio2_35()
  {
    canvas_->overrideDisplayAspectRatio(2.35);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_85
  //
  void
  MainWindow::playbackAspectRatio1_85()
  {
    canvas_->overrideDisplayAspectRatio(1.85);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_78
  //
  void
  MainWindow::playbackAspectRatio1_78()
  {
    canvas_->overrideDisplayAspectRatio(16.0 / 9.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_60
  //
  void
  MainWindow::playbackAspectRatio1_60()
  {
    canvas_->overrideDisplayAspectRatio(1.6);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackAspectRatio1_33
  //
  void
  MainWindow::playbackAspectRatio1_33()
  {
    canvas_->overrideDisplayAspectRatio(4.0 / 3.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameNone
  //
  void
  MainWindow::playbackCropFrameNone()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(0.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_40
  //
  void
  MainWindow::playbackCropFrame2_40()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(2.40);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_35
  //
  void
  MainWindow::playbackCropFrame2_35()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(2.35);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_85
  //
  void
  MainWindow::playbackCropFrame1_85()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(1.85);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_78
  //
  void
  MainWindow::playbackCropFrame1_78()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(16.0 / 9.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_60
  //
  void
  MainWindow::playbackCropFrame1_60()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(1.6);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_33
  //
  void
  MainWindow::playbackCropFrame1_33()
  {
    autocropTimer_.stop();
    canvas_->cropFrame(4.0 / 3.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameAutoDetect
  //
  void
  MainWindow::playbackCropFrameAutoDetect()
  {
    canvas_->cropAutoDetect(this, &(MainWindow::autoCropCallback));
  }

  //----------------------------------------------------------------
  // MainWindow::playbackColorConverter
  //
  void
  MainWindow::playbackColorConverter()
  {
    // shortcut:
    IReader * reader = reader_.get();

    bool skipColorConverter = actionSkipColorConverter->isChecked();
    saveBooleanSetting(kSkipColorConverter, skipColorConverter);

    bool deint = actionDeinterlace->isChecked();
    saveBooleanSetting(kDeinterlaceFrames, deint);

    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
    reader->threadStop();
    stopRenderers();

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    selectVideoTrack(reader_.get(), videoTrack);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->model_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resumeRenderers(true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoopFilter
  //
  void
  MainWindow::playbackLoopFilter()
  {
    bool skipLoopFilter = actionSkipLoopFilter->isChecked();
    saveBooleanSetting(kSkipLoopFilter, skipLoopFilter);

    reader_->skipLoopFilter(skipLoopFilter);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackNonReferenceFrames
  //
  void
  MainWindow::playbackNonReferenceFrames()
  {
    bool skipNonReferenceFrames = actionSkipNonReferenceFrames->isChecked();
    saveBooleanSetting(kSkipNonReferenceFrames, skipNonReferenceFrames);

    reader_->skipNonReferenceFrames(skipNonReferenceFrames);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackVerticalScaling
  //
  void
  MainWindow::playbackVerticalScaling()
  {
    canvasSizeBackup();
    bool enable = actionVerticalScaling->isChecked();
    canvas_->enableVerticalScaling(enable);
    canvasSizeRestore();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackSetTempo
  //
  void
  MainWindow::playbackSetTempo(int percent)
  {
    tempo_ = double(percent) / 100.0;
    reader_->setTempo(tempo_);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowPlaylist
  //
  void
  MainWindow::playbackShowPlaylist()
  {
    SignalBlocker blockSignals(actionShowPlaylist);
    canvasSizeBackup();

    if (playlistDock_->isVisible())
    {
      actionShowPlaylist->setChecked(false);
      playlistDock_->hide();
    }
    else
    {
      actionShowPlaylist->setChecked(true);
      playlistDock_->show();
    }

    // repaint the frame:
    canvas_->requestRepaint();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowTimeline
  //
  void
  MainWindow::playbackShowTimeline()
  {
    SignalBlocker blockSignals(actionShowTimeline);

    bool showTimeline = !timelineWidgets_->isVisible();
    saveBooleanSetting(kShowTimeline, showTimeline);

    QRect mainGeom = geometry();
    int ctrlHeight = timelineWidgets_->height();
    bool fullScreen = this->isFullScreen();

    if (!showTimeline)
    {
      actionShowTimeline->setChecked(false);
      timelineWidgets_->hide();

      if (!fullScreen)
      {
        resize(width(), mainGeom.height() - ctrlHeight);
      }
    }
    else
    {
      actionShowTimeline->setChecked(true);

      if (!fullScreen)
      {
        resize(width(), mainGeom.height() + ctrlHeight);
      }

      timelineWidgets_->show();
    }

    // repaint the frame:
    canvas_->requestRepaint();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShrinkWrap
  //
  void
  MainWindow::playbackShrinkWrap()
  {
    if (isFullScreen())
    {
      return;
    }

    // shortcut:
    IReader * reader = reader_.get();

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
  // MainWindow::swapShortcuts
  //
  void
  MainWindow::swapShortcuts()
  {
    yae::swapShortcuts(shortcutExit_, actionExit);
    yae::swapShortcuts(shortcutFullScreen_, actionFullScreen);
    yae::swapShortcuts(shortcutFillScreen_, actionFillScreen);
    yae::swapShortcuts(shortcutShowPlaylist_, actionShowPlaylist);
    yae::swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
    yae::swapShortcuts(shortcutPlay_, actionPlay);
    yae::swapShortcuts(shortcutNext_, actionNext);
    yae::swapShortcuts(shortcutPrev_, actionPrev);
    yae::swapShortcuts(shortcutLoop_, actionLoop);
    yae::swapShortcuts(shortcutCropNone_, actionCropFrameNone);
    yae::swapShortcuts(shortcutCrop1_33_, actionCropFrame1_33);
    yae::swapShortcuts(shortcutCrop1_78_, actionCropFrame1_78);
    yae::swapShortcuts(shortcutCrop1_85_, actionCropFrame1_85);
    yae::swapShortcuts(shortcutCrop2_40_, actionCropFrame2_40);
    yae::swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
    yae::swapShortcuts(shortcutNextChapter_, actionNextChapter);
    yae::swapShortcuts(shortcutAspectRatioNone_, actionAspectRatioAuto);
    yae::swapShortcuts(shortcutAspectRatio1_33_, actionAspectRatio1_33);
    yae::swapShortcuts(shortcutAspectRatio1_78_, actionAspectRatio1_78);
  }

  //----------------------------------------------------------------
  // MainWindow::toggleFullScreen
  //
  void
  MainWindow::toggleFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
    }
    else
    {
      enterFullScreen(renderMode_);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::enterFullScreen
  //
  void
  MainWindow::enterFullScreen(Canvas::TRenderMode renderMode)
  {
    if (isFullScreen() && renderMode_ == renderMode)
    {
      exitFullScreen();
      return;
    }

    SignalBlocker blockSignals;
    blockSignals
      << actionFullScreen
      << actionFillScreen;

    fullscreenToggle_->setChecked(true);

    if (renderMode == Canvas::kScaleToFit)
    {
      actionFullScreen->setChecked(true);
      actionFillScreen->setChecked(false);
    }

    if (renderMode == Canvas::kCropToFill)
    {
      actionFillScreen->setChecked(true);
      actionFullScreen->setChecked(false);
    }

    canvas_->setRenderMode(renderMode);
    renderMode_ = renderMode;

    if (isFullScreen())
    {
      return;
    }

    // enter full screen rendering:
    actionShrinkWrap->setEnabled(false);
    menuBar()->hide();
    showFullScreen();

    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackFullScreen
  //
  void
  MainWindow::playbackFullScreen()
  {
    // enter full screen pillars-and-bars letterbox rendering:
    enterFullScreen(Canvas::kScaleToFit);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackFillScreen
  //
  void
  MainWindow::playbackFillScreen()
  {
    // enter full screen crop-to-fill rendering:
    enterFullScreen(Canvas::kCropToFill);
  }

  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  //
  void
  MainWindow::exitFullScreen()
  {
    if (!isFullScreen())
    {
      return;
    }

    // exit full screen rendering:
    SignalBlocker blockSignals;
    blockSignals
      << actionFullScreen
      << actionFillScreen;

    fullscreenToggle_->setChecked(false);
    actionFullScreen->setChecked(false);
    actionFillScreen->setChecked(false);
    actionShrinkWrap->setEnabled(true);
    menuBar()->show();
    showNormal();
    canvas_->setRenderMode(Canvas::kScaleToFit);
    QTimer::singleShot(100, this, SLOT(adjustCanvasHeight()));

    this->swapShortcuts();
  }

  //----------------------------------------------------------------
  // MainWindow::togglePlayback
  //
  void
  MainWindow::togglePlayback()
  {
#if 0
    yae_debug << "togglePlayback: "
              << !playbackPaused_ << " -> "
              << playbackPaused_;
#endif

    reader_->setPlaybackEnabled(playbackPaused_);
    playbackPaused_ = !playbackPaused_;

    if (!playbackPaused_)
    {
      playbackToggle_->setChecked(false);
      actionPlay->setText(tr("Pause"));
      prepareReaderAndRenderers(reader_, playbackPaused_);
      resumeRenderers();

      bookmarkTimer_.start();
    }
    else
    {
      playbackToggle_->setChecked(true);
      actionPlay->setText(tr("Play"));
      TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
      stopRenderers();
      prepareReaderAndRenderers(reader_, playbackPaused_);

      bookmarkTimer_.stop();
      saveBookmark();
    }

    view_.force_animate_opacity();
  }

  //----------------------------------------------------------------
  // MainWindow::audioDownmixToStereo
  //
  void
  MainWindow::audioDownmixToStereo()
  {
    saveBooleanSetting(kDownmixToStereo, actionDownmixToStereo->isChecked());

    // shortcut:
    IReader * reader = reader_.get();

    // reset reader:
    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
    reader->threadStop();

    stopRenderers();
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->model_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectTrack
  //
  void
  MainWindow::audioSelectTrack(int index)
  {
#if 0
    yae_debug << "audioSelectTrack: " << index;
#endif

    // shortcut:
    IReader * reader = reader_.get();

    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
    reader->threadStop();
    stopRenderers();

    VideoTraits video_traits;
    AudioTraits audio_traits;

    // detect current program selection change:
    TTrackInfo prevSel = selAudio_;
    if (!reader->hasSelectedAudioTrack())
    {
      std::size_t i = reader->getSelectedVideoTrackIndex();
      reader->getVideoTrackInfo(i, prevSel, video_traits);
    }

    selectAudioTrack(reader, index);

    if (reader->getAudioTrackInfo(index, selAudio_, audio_traits) &&
        selAudio_.program_ != prevSel.program_)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(selAudio_.program_, program));

      // must adjust other track selections to match the selected program:
      if (reader->hasSelectedVideoTrack())
      {
        std::size_t i = program.video_.empty() ?
          reader->getNumberOfVideoTracks() : program.video_.front();
        selectVideoTrack(reader, i);
        videoTrackGroup_->actions().at((int)i)->setChecked(true);
      }

      if (reader->hasSelectedSubsTrack())
      {
        std::size_t i = program.subtt_.empty() ?
          reader->subsCount() : program.subtt_.front();
        selectSubsTrack(reader, i);
        subsTrackGroup_->actions().at((int)i)->setChecked(true);
      }
    }

    reader->getAudioTraits(selAudioTraits_);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->model_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::videoSelectTrack
  //
  void
  MainWindow::videoSelectTrack(int index)
  {
#if 0
    yae_debug << "videoSelectTrack: " << index;
#endif

    // shortcut:
    IReader * reader = reader_.get();

    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
    reader->threadStop();
    stopRenderers();

    VideoTraits video_traits;
    AudioTraits audio_traits;

    // detect current program selection change:
    TTrackInfo prevSel = selVideo_;
    if (!reader->hasSelectedVideoTrack())
    {
      std::size_t i = reader->getSelectedAudioTrackIndex();
      reader->getAudioTrackInfo(i, prevSel, audio_traits);
    }

    selectVideoTrack(reader, index);

    if (reader->getVideoTrackInfo(index, selVideo_, video_traits) &&
        selVideo_.program_ != prevSel.program_)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(selVideo_.program_, program));

      // must adjust other track selections to match the selected program:
      if (reader->hasSelectedAudioTrack())
      {
        std::size_t i = program.audio_.empty() ?
          reader->getNumberOfAudioTracks()  : program.audio_.front();
        selectAudioTrack(reader, i);
        audioTrackGroup_->actions().at((int)i)->setChecked(true);
      }

      if (reader->hasSelectedSubsTrack())
      {
        std::size_t i = program.subtt_.empty() ?
          reader->subsCount() : program.subtt_.front();
        selectSubsTrack(reader, i);
        subsTrackGroup_->actions().at((int)i)->setChecked(true);
      }
    }

    reader->getVideoTraits(selVideoTraits_);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->model_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resumeRenderers(true);
  }

  //----------------------------------------------------------------
  // MainWindow::subsSelectTrack
  //
  void
  MainWindow::subsSelectTrack(int index)
  {
#if 0
    yae_debug << "subsSelectTrack: " << index;
#endif

    // shortcut:
    IReader * reader = reader_.get();

    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);
    reader->threadStop();
    stopRenderers();

    VideoTraits video_traits;
    AudioTraits audio_traits;

    // detect current program selection change:
    TTrackInfo prevSel = selSubs_;
    if (!reader->hasSelectedSubsTrack())
    {
      std::size_t i = reader->getSelectedVideoTrackIndex();
      reader->getVideoTrackInfo(i, prevSel, video_traits);
    }

    selectSubsTrack(reader, index);
    selSubsFormat_ = reader->subsInfo(index, selSubs_);

    if (selSubsFormat_ != kSubsNone &&
        selSubs_.program_ != prevSel.program_)
    {
      TProgramInfo program;
      YAE_ASSERT(reader->getProgramInfo(selVideo_.program_, program));

      // must adjust other track selections to match the selected program:
      if (reader->hasSelectedVideoTrack())
      {
        std::size_t i = program.video_.empty() ?
          reader->getNumberOfVideoTracks() : program.video_.front();
        selectVideoTrack(reader, i);
        videoTrackGroup_->actions().at((int)i)->setChecked(true);
      }

      if (reader->hasSelectedAudioTrack())
      {
        std::size_t i = program.audio_.empty() ?
          reader->getNumberOfAudioTracks() : program.audio_.front();
        selectAudioTrack(reader, i);
        audioTrackGroup_->actions().at((int)i)->setChecked(true);
      }
    }

    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->model_.currentTime();
    reader->seek(t);
    reader->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::updateChaptersMenu
  //
  void
  MainWindow::updateChaptersMenu()
  {
    // shortcut:
    IReader * reader = reader_.get();

    const double playheadInSeconds = timelineControls_->model_.currentTime();
    QList<QAction *> actions = chaptersGroup_->actions();

    const std::size_t numActions = actions.size();
    const std::size_t numChapters = reader->countChapters();

    if (numChapters != numActions)
    {
      YAE_ASSERT(false);
      return;
    }

    // check-mark the current chapter:
    SignalBlocker blockSignals(chapterMapper_);

    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      if (reader->getChapterInfo(i, ch))
      {
        double chEnd = ch.t1_sec();

        if ((playheadInSeconds >= ch.t0_sec() &&
             playheadInSeconds < chEnd) ||
            (playheadInSeconds < ch.t0_sec() && i > 0))
        {
          std::size_t index = (playheadInSeconds >= ch.t0_sec()) ? i : i - 1;

          QAction * chapterAction = actions[(int)index];
          chapterAction->setChecked(true);
          return;
        }
      }
      else
      {
        YAE_ASSERT(false);
      }

      QAction * chapterAction = actions[(int)i];
      chapterAction->setChecked(false);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::skipToNextChapter
  //
  void
  MainWindow::skipToNextChapter()
  {
    // shortcut:
    IReader * reader = reader_.get();

    const double playheadInSeconds = timelineControls_->model_.currentTime();
    const std::size_t numChapters = reader->countChapters();

    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      if (reader->getChapterInfo(i, ch))
      {
        if (playheadInSeconds < ch.t0_sec())
        {
          timelineControls_->model_.seekTo(ch.t0_sec());
          return;
        }
      }
      else
      {
        YAE_ASSERT(false);
      }
    }

    // last chapter, skip to next playlist item:
    playbackNext();
  }

  //----------------------------------------------------------------
  // MainWindow::skipToChapter
  //
  void
  MainWindow::skipToChapter(int index)
  {
    TChapter ch;
    bool ok = reader_->getChapterInfo(index, ch);
    YAE_ASSERT(ok);

    timelineControls_->model_.seekTo(ch.t0_sec());
  }

  //----------------------------------------------------------------
  // MainWindow::playlistItemChanged
  //
  void
  MainWindow::playlistItemChanged(std::size_t index)
  {
    playbackStop();

    PlaylistItem * item = playlistWidget_->lookup(index);
    if (!item)
    {
      // prevent Canvas from rendering any pending frames from previous reader:
      ++readerId_;
      canvas_->acceptFramesWithReaderId(readerId_);
      canvas_->clear();
      canvas_->setGreeting(canvas_->greeting());

      actionPlay->setEnabled(false);
      fixupNextPrev();
    }
    else
    {
      playback();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::windowHalfSize
  //
  void
  MainWindow::windowHalfSize()
  {
    canvasSizeSet(0.5, 0.5);
  }

  //----------------------------------------------------------------
  // MainWindow::windowFullSize
  //
  void
  MainWindow::windowFullSize()
  {
    canvasSizeSet(1.0, 1.0);
  }

  //----------------------------------------------------------------
  // MainWindow::windowDoubleSize
  //
  void
  MainWindow::windowDoubleSize()
  {
    canvasSizeSet(2.0, 2.0);
  }

  //----------------------------------------------------------------
  // MainWindow::windowDecreaseSize
  //
  void
  MainWindow::windowDecreaseSize()
  {
    canvasSizeScaleBy(0.5);
  }

  //----------------------------------------------------------------
  // MainWindow::windowIncreaseSize
  //
  void
  MainWindow::windowIncreaseSize()
  {
    canvasSizeScaleBy(2.0);
  }

  //----------------------------------------------------------------
  // MainWindow::helpAbout
  //
  void
  MainWindow::helpAbout()
  {
    static AboutDialog * about = NULL;
    if (!about)
    {
      about = new AboutDialog(this);
      about->setWindowTitle(tr("Apprentice Video Classic (revision %1)").
                            arg(QString::fromUtf8(YAE_REVISION)));
    }

    about->show();
  }

  //----------------------------------------------------------------
  // MainWindow::processDropEventUrls
  //
  void
  MainWindow::processDropEventUrls(const QList<QUrl> & urls)
  {
    std::list<QString> playlist;
    for (QList<QUrl>::const_iterator i = urls.begin(); i != urls.end(); ++i)
    {
      QUrl url = *i;

#ifdef __APPLE__
      if (url.toString().startsWith("file:///.file/id="))
      {
        std::string strUrl = url.toString().toUtf8().constData();
        strUrl = yae::absoluteUrlFrom(strUrl.c_str());
        url = QUrl::fromEncoded(QByteArray(strUrl.c_str()));
      }
#endif

      QString fullpath = QFileInfo(url.toLocalFile()).canonicalFilePath();
      if (!addToPlaylist(playlist, fullpath))
      {
        QString strUrl = url.toString();
        addToPlaylist(playlist, strUrl);
      }
    }

    setPlaylist(playlist);
  }

  //----------------------------------------------------------------
  // MainWindow::userIsSeeking
  //
  void
  MainWindow::userIsSeeking(bool seeking)
  {
    reader_->setPlaybackEnabled(!seeking && !playbackPaused_);
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeIn
  //
  void
  MainWindow::moveTimeIn(double seconds)
  {
    // shortcut:
    IReader * reader = reader_.get();

    reader->setPlaybackLooping(true);
    reader->setPlaybackIntervalStart(seconds);

    SignalBlocker blockSignals(actionLoop);
    actionLoop->setChecked(true);
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeOut
  //
  void
  MainWindow::moveTimeOut(double seconds)
  {
    // shortcut:
    IReader * reader = reader_.get();

    reader->setPlaybackLooping(true);
    reader->setPlaybackIntervalEnd(seconds);

    SignalBlocker blockSignals(actionLoop);
    actionLoop->setChecked(true);
  }

  //----------------------------------------------------------------
  // MainWindow::movePlayHead
  //
  void
  MainWindow::movePlayHead(double seconds)
  {
#ifndef NDEBUG
    yae_debug
      << "MOVE PLAYHEAD TO: " << TTime(seconds).to_hhmmss_us(":");
#endif

    videoRenderer_->pause();
    audioRenderer_->pause();

    reader_->seek(seconds);

    resumeRenderers(true);
  }

  //----------------------------------------------------------------
  // MainWindow::updateTimelineDuration
  //
  void
  MainWindow::updateTimelineDuration()
  {
    timelineControls_->model_.updateDuration(reader_.get());
  }

  //----------------------------------------------------------------
  // MainWindow::playbackFinished
  //
  void
  MainWindow::playbackFinished(const SharedClock & c)
  {
    if (!timelineControls_->model_.sharedClock().sharesCurrentTimeWith(c))
    {
#ifndef NDEBUG
      yae_debug << "NOTE: ignoring stale playbackFinished";
#endif
      return;
    }

    // remove current bookmark:
    bookmarkTimer_.stop();
    durationTimer_.stop();

    std::size_t itemIndex = playlistWidget_->currentItem();
    std::size_t nNext = playlistWidget_->countItemsAhead();
    std::size_t iNext = playlistWidget_->closestItem(itemIndex + 1);

    PlaylistItem * next =
      nNext && iNext > itemIndex ? playlistWidget_->lookup(iNext) : NULL;

    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlistWidget_->lookup(itemIndex, &group);

    if (item && group)
    {
      PlaylistGroup * nextGroup = NULL;
      playlistWidget_->closestItem(itemIndex + 1,
                                   PlaylistWidget::kAhead,
                                   &nextGroup);

      if (group != nextGroup)
      {
        PlaylistBookmark bookmark;

        // if current item was bookmarked, then remove it from bookmarks:
        if (findBookmark(itemIndex, bookmark))
        {
          yae::removeBookmark(group->bookmarkHash_);
        }

        // if a bookmark exists for the next item group, then use it:
        if (nextGroup && findBookmark(nextGroup->bookmarkHash_, bookmark))
        {
          gotoBookmark(bookmark);
          return;
        }
      }
    }

    if (!next && actionRepeatPlaylist->isChecked())
    {
      // repeat the playlist:
      std::size_t first = playlistWidget_->closestItem(0);
      playlistWidget_->setCurrentItem(first, true);
      return;
    }

    playbackNext();
    saveBookmark();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackStop
  //
  void
  MainWindow::playbackStop()
  {
    IReaderPtr reader_ptr(ReaderFFMPEG::create());
    IReader * reader = reader_ptr.get();

    timelineControls_->model_.observe(SharedClock());
    timelineControls_->model_.resetFor(reader);

    reader_->close();
    videoRenderer_->close();
    audioRenderer_->close();

    reader_ = reader_ptr;

    timelineControls_->update();

    this->setWindowTitle(tr("Apprentice Video Classic"));

    adjustMenuActions();
    adjustMenus(reader);
  }

  //----------------------------------------------------------------
  // MainWindow::playback
  //
  void
  MainWindow::playback(bool forward)
  {
    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t current = playlistWidget_->currentItem();
    PlaylistItem * item = NULL;
    bool ok = false;

    while ((item = playlistWidget_->lookup(current)))
    {
      item->failed_ = !load(item->path_);

      if (!item->failed_)
      {
        ok = true;
        break;
      }

      if (forward)
      {
        current = playlistWidget_->closestItem(current + 1);
      }
      else
      {
        current = playlistWidget_->closestItem(current - 1,
                                               PlaylistWidget::kBehind);
      }

      playlistWidget_->setCurrentItem(current);
    }

    fixupNextPrev();

    if (!ok && !forward)
    {
      playback(true);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::fixupNextPrev
  //
  void
  MainWindow::fixupNextPrev()
  {
    std::size_t index = playlistWidget_->currentItem();
    std::size_t nNext = playlistWidget_->countItemsAhead();
    std::size_t nPrev = playlistWidget_->countItemsBehind();

    std::size_t iNext =
      nNext ?
      playlistWidget_->closestItem(index + 1) :
      index;

    std::size_t iPrev =
      nPrev ?
      playlistWidget_->closestItem(index - 1, PlaylistWidget::kBehind) :
      index;

    PlaylistItem * prev =
      nPrev && iPrev < index ? playlistWidget_->lookup(iPrev) : NULL;

    PlaylistItem * next =
      nNext && iNext > index ? playlistWidget_->lookup(iNext) : NULL;

    actionPrev->setEnabled(iPrev < index);
    actionNext->setEnabled(iNext > index);

    if (prev)
    {
      actionPrev->setText(tr("Go Back To %1").arg(prev->name_));
    }
    else
    {
      actionPrev->setText(tr("Go Back"));
    }

    if (next)
    {
      actionNext->setText(tr("Skip To %1").arg(next->name_));
    }
    else
    {
      actionNext->setText(tr("Skip"));
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playbackNext
  //
  void
  MainWindow::playbackNext()
  {
    playbackStop();

    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t index = playlistWidget_->currentItem();
    std::size_t iNext = playlistWidget_->closestItem(index + 1);
    if (iNext > index)
    {
      playlistWidget_->setCurrentItem(iNext);
    }

    playback(true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackPrev
  //
  void
  MainWindow::playbackPrev()
  {
    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    std::size_t index = playlistWidget_->currentItem();
    std::size_t iPrev = playlistWidget_->closestItem(index - 1,
                                                     PlaylistWidget::kBehind);
    if (iPrev < index)
    {
      playlistWidget_->setCurrentItem(iPrev);
    }

    playback(false);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoop
  //
  void
  MainWindow::playbackLoop()
  {
    bool enableLooping = actionLoop->isChecked();
    reader_->setPlaybackLooping(enableLooping);
  }

  //----------------------------------------------------------------
  // MainWindow::scrollWheelTimerExpired
  //
  void
  MainWindow::scrollWheelTimerExpired()
  {
    timelineControls_->model_.seekFromCurrentTime(scrollOffset_);
  }

  //----------------------------------------------------------------
  // MainWindow::playlistVisibilityChanged
  //
  void
  MainWindow::playlistVisibilityChanged(bool visible)
  {
    if (actionShowPlaylist->isEnabled())
    {
      actionShowPlaylist->setChecked(visible);
    }

    QTimer::singleShot(1, this, SLOT(canvasSizeRestore()));
  }

  //----------------------------------------------------------------
  // MainWindow::saveBookmark
  //
  void
  MainWindow::saveBookmark()
  {
    if (!actionAutomaticBookmarks->isChecked())
    {
      return;
    }

    // shortcut:
    IReader * reader = reader_.get();

    if (!reader->isSeekable())
    {
      return;
    }

    std::size_t itemIndex = playlistWidget_->currentItem();
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlistWidget_->lookup(itemIndex, &group);

    if (group && item)
    {
      double positionInSeconds = timelineControls_->model_.currentTime();
      yae::saveBookmark(group->bookmarkHash_,
                        item->bookmarkHash_,
                        reader,
                        positionInSeconds);

      // refresh the bookmarks list:
      bookmarksPopulate();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::gotoBookmark
  //
  void
  MainWindow::gotoBookmark(const PlaylistBookmark & bookmark)
  {
    playbackStop();

    SignalBlocker blockSignals(playlistWidget_);
    actionPlay->setEnabled(false);

    playlistWidget_->setCurrentItem(bookmark.itemIndex_);
    PlaylistItem * item = playlistWidget_->lookup(bookmark.itemIndex_);

    if (item)
    {
      item->failed_ = !load(item->path_, &bookmark);
    }

    fixupNextPrev();
  }

  //----------------------------------------------------------------
  // MainWindow::findBookmark
  //
  bool
  MainWindow::findBookmark(std::size_t itemIndex,
                           PlaylistBookmark & found) const
  {
    for (std::vector<PlaylistBookmark>::const_iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      const PlaylistBookmark & bookmark = *i;
      if (bookmark.itemIndex_ == itemIndex)
      {
        found = bookmark;
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // MainWindow::findBookmark
  //
  bool
  MainWindow::findBookmark(const std::string & groupHash,
                           PlaylistBookmark & found) const
  {
    for (std::vector<PlaylistBookmark>::const_iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      const PlaylistBookmark & bookmark = *i;
      if (bookmark.groupHash_ == groupHash)
      {
        found = bookmark;
        return true;
      }
    }

    return false;
  }

  //----------------------------------------------------------------
  // ReaderEvent
  //
  struct ReaderEvent : public QEvent
  {
    ReaderEvent(const IReaderPtr & reader,
                const Json::Value & event):
      QEvent(QEvent::User),
      reader_(reader),
      event_(event)
    {}

    IReaderPtr reader_;
    Json::Value event_;
  };

  //----------------------------------------------------------------
  // MainWindow::handle_reader_event
  //
  void
  MainWindow::handle_reader_event(const IReaderPtr & reader,
                                  const Json::Value & event)
  {
    yae_dlog("MainWindow::handle_reader_event: %s",
             yae::to_str(event).c_str());

    qApp->postEvent(this, new yae::ReaderEvent(reader, event));
  }

  //----------------------------------------------------------------
  // MainWindow::event
  //
  bool
  MainWindow::event(QEvent * e)
  {
    if (e->type() == QEvent::User)
    {
      AutoCropEvent * ac = dynamic_cast<AutoCropEvent *>(e);
      if (ac)
      {
        ac->accept();
        canvas_->cropFrame(ac->cropFrame_);
        adjustCanvasHeight();
        return true;
      }

      yae::ReaderEvent * re = dynamic_cast<yae::ReaderEvent *>(e);
      if (re)
      {
        re->accept();
        if (re->reader_ == reader_)
        {
          std::string event_type = re->event_["event_type"].asString();
          if (event_type == "traits_changed")
          {
            re->reader_->refreshInfo();
          }
          else if (event_type == "info_refreshed")
          {
            this->adjustMenuActions();
            this->adjustMenus(reader_.get());
          }
        }
        return true;
      }
    }

    return QMainWindow::event(e);
  }

  //----------------------------------------------------------------
  // MainWindow::wheelEvent
  //
  void
  MainWindow::wheelEvent(QWheelEvent * e)
  {
    double tNow = timelineControls_->model_.currentTime();
    if (tNow <= 1e-1)
    {
      // ignore it:
      return;
    }

    // seek back and forth here:
    int delta = yae::get_wheel_delta(e);
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
  }

  //----------------------------------------------------------------
  // MainWindow::closeEvent
  //
  void
  MainWindow::closeEvent(QCloseEvent * e)
  {
    e->accept();
    fileExit();
  }

  //----------------------------------------------------------------
  // MainWindow::dragEnterEvent
  //
  void
  MainWindow::dragEnterEvent(QDragEnterEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }

    e->acceptProposedAction();
  }

  //----------------------------------------------------------------
  // MainWindow::dropEvent
  //
  void
  MainWindow::dropEvent(QDropEvent * e)
  {
    if (!e->mimeData()->hasUrls())
    {
      e->ignore();
      return;
    }

    e->acceptProposedAction();
    processDropEventUrls(e->mimeData()->urls());
  }

  //----------------------------------------------------------------
  // MainWindow::keyPressEvent
  //
  void
  MainWindow::keyPressEvent(QKeyEvent * event)
  {
    int key = event->key();
    bool key_press = event->type() == QEvent::KeyPress;

    if (key == Qt::Key_I)
    {
      if (key_press)
      {
        emit setInPoint();
      }
    }
    else if (key == Qt::Key_O)
    {
      if (key_press)
      {
        emit setOutPoint();
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
        timelineControls_->model_.seekFromCurrentTime(7.0);
      }
    }
    else if (key == Qt::Key_MediaPrevious ||
             key == Qt::Key_Comma ||
             key == Qt::Key_Less ||
             key == Qt::Key_Left)
    {
      if (key_press)
      {
        timelineControls_->model_.seekFromCurrentTime(-3.0);
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
      QMainWindow::keyPressEvent(event);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::skipToNextFrame
  //
  void
  MainWindow::skipToNextFrame()
  {
    if (!playbackPaused_)
    {
      return;
    }

    // shortcut:
    IReader * reader = reader_.get();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t videoTrackIndex = reader->getSelectedVideoTrackIndex();

    if (videoTrackIndex >= numVideoTracks)
    {
      return;
    }

    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t audioTrackIndex = reader->getSelectedAudioTrackIndex();
    bool hasAudio = audioTrackIndex < numAudioTracks;

    TIgnoreClockStop ignoreClockStop(timelineControls_->model_);

    TTime startTime = TTime::now();
    bool done = false;
    while (!done && reader && reader == reader_.get())
    {
      if (hasAudio && reader->blockedOnAudio())
      {
        // VFR source (a slide show) may require the audio output
        // queues to be pulled in order to allow the demuxer
        // to push new packets into audio/video queues:

        TTime dt(1001, 60000);
        audioRenderer_->skipForward(dt, reader);
      }

      TTime t;
      done = videoRenderer_->skipToNextFrame(t);

      if (!done)
      {
        TTime now = TTime::now();
        if ((now - startTime).get(1000) > 2000)
        {
          // avoid blocking the UI indefinitely:
          break;
        }

        continue;
      }

      if (hasAudio)
      {
        // attempt to nudge the audio reader to the same position:
        audioRenderer_->skipToTime(t, reader);
      }
    }
  }

  //----------------------------------------------------------------
  // MainWindow::mousePressEvent
  //
  void
  MainWindow::mousePressEvent(QMouseEvent * e)
  {
    // shortcut:
    IReader * reader = reader_.get();

    if (e->button() == Qt::RightButton)
    {
      std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
      std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
      std::size_t numSubtitles = reader->subsCount();
      std::size_t numChapters = reader->countChapters();

      QPoint localPt = e->pos();
      QPoint globalPt = QWidget::mapToGlobal(localPt);

      // populate the context menu:
      contextMenu_->clear();
      contextMenu_->addAction(actionPlay);

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionPrev);
      contextMenu_->addAction(actionNext);
      contextMenu_->addAction(actionShowPlaylist);
      contextMenu_->addAction(actionRepeatPlaylist);

      if (playlistWidget_->underMouse() &&
          playlistWidget_->countItems())
      {
        contextMenu_->addSeparator();
        contextMenu_->addAction(actionRemove_);
        contextMenu_->addAction(actionSelectAll_);
      }

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionLoop);
      contextMenu_->addAction(actionSetInPoint_);
      contextMenu_->addAction(actionSetOutPoint_);
      contextMenu_->addAction(actionShowTimeline);

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionShrinkWrap);
      contextMenu_->addAction(actionFullScreen);
      contextMenu_->addAction(actionFillScreen);
      addMenuCopyTo(contextMenu_, menuPlaybackSpeed);

      contextMenu_->addSeparator();
      addMenuCopyTo(contextMenu_, menuBookmarks);

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

        if (numSubtitles)
        {
          addMenuCopyTo(contextMenu_, menuSubs_);
        }

        if (numChapters)
        {
          addMenuCopyTo(contextMenu_, menuChapters_);
        }
      }

      contextMenu_->popup(globalPt);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeScaleBy
  //
  void
  MainWindow::canvasSizeScaleBy(double scale)
  {
    canvasSizeSet(xexpand_ * scale, yexpand_ * scale);
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeBackup
  //
  void
  MainWindow::canvasSizeBackup()
  {
    if (isFullScreen())
    {
      return;
    }

    int vw = int(0.5 + canvas_->imageWidth());
    int vh = int(0.5 + canvas_->imageHeight());
    if (vw < 1 || vh < 1)
    {
      return;
    }

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);

#if 0
    yae_debug << "\ncanvas size backup: " << xexpand_ << ", " << yexpand_;
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeRestore
  //
  void
  MainWindow::canvasSizeRestore()
  {
    canvasSizeSet(xexpand_, yexpand_);
  }

  //----------------------------------------------------------------
  // MainWindow::canvasSizeSet
  //
  void
  MainWindow::canvasSizeSet(double xexpand, double yexpand)
  {
    xexpand_ = xexpand;
    yexpand_ = yexpand;

    if (isFullScreen())
    {
      return;
    }

    double iw = canvas_->imageWidth();
    double ih = canvas_->imageHeight();

    int vw = int(0.5 + iw);
    int vh = int(0.5 + ih);

    if (vw < 1 || vh < 1)
    {
      return;
    }

    QRect rectWindow = frameGeometry();
    int ww = rectWindow.width();
    int wh = rectWindow.height();

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    // calculate width and height overhead:
    int ox = ww - cw;
    int oy = wh - ch;

    int ideal_w = ox + int(0.5 + vw * xexpand_);
    int ideal_h = oy + int(0.5 + vh * yexpand_);

    yae::GetScreenInfo get_screen_info(this);
    QRect rectMax = get_screen_info.available_geometry();
    int max_w = rectMax.width();
    int max_h = rectMax.height();

    if (ideal_w > max_w || ideal_h > max_h)
    {
      // image won't fit on screen, scale it to the largest size that fits:
      double vDAR = canvas_->imageWidth() / canvas_->imageHeight();
      double cDAR = double(max_w - ox) / double(max_h - oy);

      if (vDAR > cDAR)
      {
        ideal_w = max_w;
        ideal_h = oy + int(0.5 + double(max_w - ox) / vDAR);
      }
      else
      {
        ideal_h = max_h;
        ideal_w = ox + int(0.5 + double(max_h - oy) * vDAR);
      }
    }

    int new_w = std::min(ideal_w, max_w);
    int new_h = std::min(ideal_h, max_h);

    // apply the new window geometry:
    QRect rectClient = geometry();
    int cdx = rectWindow.width() - rectClient.width();
    int cdy = rectWindow.height() - rectClient.height();

#if 0
    int max_x0 = rectMax.x();
    int max_y0 = rectMax.y();
    int max_x1 = max_x0 + max_w - 1;
    int max_y1 = max_y0 + max_h - 1;

    int new_x0 = rectWindow.x();
    int new_y0 = rectWindow.y();
    int new_x1 = new_x0 + new_w - 1;
    int new_y1 = new_y0 + new_h - 1;

    int shift_x = std::min(0, max_x1 - new_x1);
    int shift_y = std::min(0, max_y1 - new_y1);

    int new_x = new_x0 + shift_x;
    int new_y = new_y0 + shift_y;

    yae_debug << "canvas size set: " << xexpand << ", " << yexpand;
    yae_debug << "canvas resize: " << new_w - cdx << ", " << new_h - cdy;
    yae_debug << "canvas move to: " << new_x << ", " << new_y;
#endif

    resize(new_w - cdx, new_h - cdy);
    // move(new_x, new_y);

    // repaint the frame:
    canvas_->requestRepaint();

    // avoid hiding the highlighted item:
    playlistWidget_->makeSureHighlightedItemIsVisible();
  }

  //----------------------------------------------------------------
  // MainWindow::adjustCanvasHeight
  //
  void
  MainWindow::adjustCanvasHeight()
  {
    if (!isFullScreen())
    {
      double w = 1.0;
      double h = 1.0;
      double dar = canvas_->imageAspectRatio(w, h);

      if (dar)
      {
        double s = double(canvas_->width()) / w;
        canvasSizeSet(s, s);
      }
    }
  }

  //----------------------------------------------------------------
  // MainWindow::stopRenderers
  //
  void
  MainWindow::stopRenderers()
  {
    videoRenderer_->stop();
    audioRenderer_->stop();

    videoRenderer_->pause();
    audioRenderer_->pause();
  }

  //----------------------------------------------------------------
  // MainWindow::prepareReaderAndRenderers
  //
  void
  MainWindow::prepareReaderAndRenderers(const IReaderPtr & reader_ptr,
                                        bool frameStepping)
  {
    videoRenderer_->pause();
    audioRenderer_->pause();

    // shortcut:
    IReader * reader = reader_ptr.get();

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t audioTrack = reader->getSelectedAudioTrackIndex();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    SharedClock sharedClock;
    timelineControls_->model_.observe(sharedClock);
    reader->setSharedClock(sharedClock);

    if (audioTrack < numAudioTracks &&
        (videoTrack >= numVideoTracks || !frameStepping))
    {
      audioRenderer_->takeThisClock(sharedClock);
      audioRenderer_->obeyThisClock(audioRenderer_->clock());

      if (videoTrack < numVideoTracks)
      {
        videoRenderer_->obeyThisClock(audioRenderer_->clock());
      }
    }
    else if (videoTrack < numVideoTracks)
    {
      videoRenderer_->takeThisClock(sharedClock);
      videoRenderer_->obeyThisClock(videoRenderer_->clock());

      if (audioTrack < numAudioTracks)
      {
        audioRenderer_->obeyThisClock(videoRenderer_->clock());
      }
    }
    else
    {
      // all tracks disabled!
      return;
    }

    // update the renderers:
    if (!frameStepping)
    {
      adjustAudioTraitsOverride(reader);

      if (!audioRenderer_->open(reader))
      {
        videoRenderer_->takeThisClock(sharedClock);
        videoRenderer_->obeyThisClock(videoRenderer_->clock());
      }
    }

    videoRenderer_->open(canvas_, reader);

    if (!frameStepping)
    {
      timelineControls_->model_.adjustTo(reader);

      // request playback at currently selected playback rate:
      reader->setTempo(tempo_);
    }

    bool enableLooping = actionLoop->isChecked();
    reader->setPlaybackLooping(enableLooping);

    bool skipLoopFilter = actionSkipLoopFilter->isChecked();
    reader->skipLoopFilter(skipLoopFilter);

    bool skipNonRefFrames = actionSkipNonReferenceFrames->isChecked();
    reader->skipNonReferenceFrames(skipNonRefFrames);

    bool deint = actionDeinterlace->isChecked();
    reader->setDeinterlacing(deint);
  }

  //----------------------------------------------------------------
  // MainWindow::resumeRenderers
  //
  void
  MainWindow::resumeRenderers(bool loadNextFrameIfPaused)
  {
    if (!playbackPaused_)
    {
      // allow renderers to read from output frame queues:
      audioRenderer_->resume();
      videoRenderer_->resume();
    }
    else if (loadNextFrameIfPaused)
    {
      // render the next video frame:
      skipToNextFrame();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::selectVideoTrack
  //
  void
  MainWindow::selectVideoTrack(IReader * reader, std::size_t video_track)
  {
    std::size_t num_video_tracks = reader->getNumberOfVideoTracks();
    reader->selectVideoTrack(video_track);

    VideoTraits vtts;
    if (reader->getVideoTraits(vtts))
    {
      const bool luminance16_not_supported = !yae::get_supports_luminance16();

      bool skip_color_converter = actionSkipColorConverter->isChecked();
      canvas_->skipColorConverter(skip_color_converter);

      TPixelFormatId format = kInvalidPixelFormat;
      if (canvas_->
          canvasRenderer()->
          adjustPixelFormatForOpenGL(skip_color_converter, vtts, format) ||
          luminance16_not_supported)
      {
        const pixelFormat::Traits * ptts_native =
          pixelFormat::getTraits(vtts.pixelFormat_);

        const pixelFormat::Traits * ptts_output =
          pixelFormat::getTraits(format);

        const bool adjusted_pixel_format = (format != vtts.pixelFormat_);
        vtts.setPixelFormat(format);

        const unsigned int native_w = vtts.encodedWidth_;
        const unsigned int native_h = vtts.encodedHeight_;

        if (luminance16_not_supported)
        {
          while (vtts.encodedWidth_ > 1280 ||
                 vtts.encodedHeight_ > 720)
          {
            vtts.encodedWidth_ >>= 1;
            vtts.encodedHeight_ >>= 1;
            vtts.offsetTop_ >>= 1;
            vtts.offsetLeft_ >>= 1;
            vtts.visibleWidth_ >>= 1;
            vtts.visibleHeight_ >>= 1;
          }
        }
        else
        {
          // NOTE: overriding frame size implies scaling, so don't do it
          // unless you really want to scale the images in the reader;
          // In general, leave scaling to OpenGL:
          vtts.encodedWidth_ = 0;
          vtts.encodedHeight_ = 0;
        }

        // preserve pixel aspect ratio:
        vtts.pixelAspectRatio_ = 0.0;

        yae_dlog("native: %s %ux%u, output: %s %ux%u",
                 ptts_native ? ptts_native->name_ : "none",
                 native_w,
                 native_h,
                 ptts_output ? ptts_output->name_ : "none",
                 vtts.encodedWidth_ ? vtts.encodedWidth_ : native_w,
                 vtts.encodedHeight_ ? vtts.encodedHeight_ : native_h);

        reader->setVideoTraitsOverride(vtts);
      }
    }

    if (reader->getVideoTraitsOverride(vtts))
    {
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

      if (!ptts && vtts.pixelFormat_ != kInvalidPixelFormat)
      {
        // unsupported pixel format:
        reader->selectVideoTrack(num_video_tracks);
      }
    }

    this->adjustMenus(reader);
  }

  //----------------------------------------------------------------
  // MainWindow::adjustMenuActions
  //
  void
  MainWindow::adjustMenuActions(IReader * reader,
                                std::vector<TTrackInfo> &  audioInfo,
                                std::vector<AudioTraits> & audioTraits,
                                std::vector<TTrackInfo> &  videoInfo,
                                std::vector<VideoTraits> & videoTraits,
                                std::vector<TTrackInfo> & subsInfo,
                                std::vector<TSubsFormat> & subsFormat)
  {
    std::size_t numVideoTracks =
      reader ? reader->getNumberOfVideoTracks() : 0;

    std::size_t numAudioTracks =
      reader ? reader->getNumberOfAudioTracks() : 0;

    std::size_t numSubtitles =
      reader ? reader->subsCount() : 0;

    std::size_t cc =
      reader ? reader->getRenderCaptions() : 0;

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
    audioTrackMapper_ = new yae::SignalMapper(this);

    ok = connect(audioTrackMapper_, SIGNAL(mapped_to(int)),
                 this, SLOT(audioSelectTrack(int)));
    YAE_ASSERT(ok);

    delete videoTrackMapper_;
    videoTrackMapper_ = new yae::SignalMapper(this);

    ok = connect(videoTrackMapper_, SIGNAL(mapped_to(int)),
                 this, SLOT(videoSelectTrack(int)));
    YAE_ASSERT(ok);

    delete subsTrackMapper_;
    subsTrackMapper_ = new yae::SignalMapper(this);

    ok = connect(subsTrackMapper_, SIGNAL(mapped_to(int)),
                 this, SLOT(subsSelectTrack(int)));
    YAE_ASSERT(ok);

    delete chapterMapper_;
    chapterMapper_ = new yae::SignalMapper(this);

    ok = connect(chapterMapper_, SIGNAL(mapped_to(int)),
                 this, SLOT(skipToChapter(int)));
    YAE_ASSERT(ok);


    audioInfo = std::vector<TTrackInfo>(numAudioTracks);
    audioTraits = std::vector<AudioTraits>(numAudioTracks);

    for (unsigned int i = 0; i < numAudioTracks; i++)
    {
      QString trackName = tr("Track %1").arg(i + 1);

      TTrackInfo & info = audioInfo[i];
      AudioTraits & traits = audioTraits[i];
      reader->getAudioTrackInfo(i, info, traits);

      if (info.hasLang())
      {
        trackName += tr(" (%1)").arg(QString::fromUtf8(info.lang()));
      }

      if (info.hasName())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.name()));
      }

      trackName +=
        tr(", %1 Hz, %2 channels").
        arg(traits.sample_rate_).
        arg(traits.ch_layout_.nb_channels);

      std::string serviceName = yae::get_program_name(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      if (info.hasCodec())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.codec()));
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
      QString trackName = tr("Track %1").arg(i + 1);

      TTrackInfo & info = videoInfo[i];
      VideoTraits & traits = videoTraits[i];
      reader->getVideoTrackInfo(i, info, traits);

      if (info.hasName())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.name()));
      }

      std::string summary = traits.summary();
      trackName += tr(", %1").arg(QString::fromUtf8(summary.c_str()));

      std::string serviceName = yae::get_program_name(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      if (info.hasCodec())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(info.codec()));
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
    lineEditPlayhead_->setReadOnly(!isSeekable);

    // check the audio track check-boxes:
    {
      std::size_t ai =
        reader ? reader->getSelectedAudioTrackIndex() : numAudioTracks;
      ai = std::min(ai, numAudioTracks);
      SignalBlocker blockSignals(audioTrackGroup_);
      audioTrackGroup_->actions().at(ai)->setChecked(true);
    }

    // check the video track check-boxes:
    {
      std::size_t vi =
        reader ? reader->getSelectedVideoTrackIndex() : numVideoTracks;
      vi = std::min(vi, numVideoTracks);
      SignalBlocker blockSignals(videoTrackGroup_);
      videoTrackGroup_->actions().at(vi)->setChecked(true);
    }

    // check the captions/subtitles check-boxes:
    if (cc)
    {
      std::size_t si = numSubtitles + cc - 1;
      SignalBlocker blockSignals(subsTrackGroup_);
      subsTrackGroup_->actions().at(si)->setChecked(true);
    }
    else
    {
      std::size_t si = 0;
      for (; si < numSubtitles && reader && !reader->getSubsRender(si); si++)
      {}

      SignalBlocker blockSignals(subsTrackGroup_);
      if (si < numSubtitles)
      {
        subsTrackGroup_->actions().at(si)->setChecked(true);
      }
      else
      {
        int index = subsTrackGroup_->actions().size() - 1;
        subsTrackGroup_->actions().at(index)->setChecked(true);
      }
    }
  }

  //----------------------------------------------------------------
  // MainWindow::adjustMenuActions
  //
  void
  MainWindow::adjustMenuActions()
  {
    std::vector<TTrackInfo>  audioInfo;
    std::vector<AudioTraits> audioTraits;
    std::vector<TTrackInfo>  videoInfo;
    std::vector<VideoTraits> videoTraits;
    std::vector<TTrackInfo>  subsInfo;
    std::vector<TSubsFormat> subsFormat;

    adjustMenuActions(reader_.get(),
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat);
  }

  //----------------------------------------------------------------
  // MainWindow::adjustMenus
  //
  void
  MainWindow::adjustMenus(IReader * reader)
  {
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t numSubtitles = reader->subsCount();
    std::size_t numChapters = reader->countChapters();
    std::size_t videoTrackIndex = reader->getSelectedVideoTrackIndex();

    if (!numVideoTracks && numAudioTracks)
    {
      menubar->removeAction(menuVideo_->menuAction());
    }

    if (!numSubtitles && !numVideoTracks)
    {
      menubar->removeAction(menuSubs_->menuAction());
    }

    if (!numChapters)
    {
      menubar->removeAction(menuChapters_->menuAction());
    }

    if (numVideoTracks || !numAudioTracks)
    {
      menubar->removeAction(menuVideo_->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuVideo_);
    }

    if (numSubtitles || numVideoTracks)
    {
      menubar->removeAction(menuSubs_->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuSubs_);
    }

    if (numChapters)
    {
      menubar->removeAction(menuChapters_->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuChapters_);
    }
    else
    {
      menubar->removeAction(menuChapters_->menuAction());
    }

    if (videoTrackIndex >= numVideoTracks && numAudioTracks > 0)
    {
      if (actionShowPlaylist->isEnabled())
      {
        setTimelineCssForAudio(timelineWidgets_);
        shortcutShowPlaylist_->setEnabled(false);
        actionShowPlaylist->setEnabled(false);

        if (actionShowPlaylist->isChecked())
        {
          playlistDock_->hide();
        }

        swapLayouts(canvasContainer_, playlistContainer_);

        playlistWidget_->show();
        playlistWidget_->update();
        playlistWidget_->setFocus();
      }
    }
    else if (numVideoTracks || !numAudioTracks)
    {
      if (!actionShowPlaylist->isEnabled())
      {
        setTimelineCssForVideo(timelineWidgets_);
        swapLayouts(canvasContainer_, playlistContainer_);

        if (actionShowPlaylist->isChecked())
        {
          playlistDock_->show();
        }

        shortcutShowPlaylist_->setEnabled(true);
        actionShowPlaylist->setEnabled(true);

        playlistWidget_->show();
        playlistWidget_->update();
        this->setFocus();
      }
    }

    // repaint the frame:
    canvas_->requestRepaint();
  }

  //----------------------------------------------------------------
  // MainWindow::selectAudioTrack
  //
  void
  MainWindow::selectAudioTrack(IReader * reader, std::size_t audioTrackIndex)
  {
     reader->selectAudioTrack(audioTrackIndex);
     adjustAudioTraitsOverride(reader);
  }

  //----------------------------------------------------------------
  // MainWindow::selectSubsTrack
  //
  void
  MainWindow::selectSubsTrack(IReader * reader, std::size_t subsTrackIndex)
  {
    const std::size_t nsubs = reader->subsCount();
    const std::size_t cc = nsubs < subsTrackIndex ? subsTrackIndex - nsubs : 0;
    reader->setRenderCaptions(cc);
    selClosedCaptions_ = cc;

    for (std::size_t i = 0; i < nsubs; i++)
    {
      bool enable = (i == subsTrackIndex);
      reader->setSubsRender(i, enable);
    }

    canvas_->setSubs(std::list<TSubsFrame>());
  }

  //----------------------------------------------------------------
  // MainWindow::adjustAudioTraitsOverride
  //
  void
  MainWindow::adjustAudioTraitsOverride(IReader * reader)
  {
     AudioTraits native;
     if (reader->getAudioTraits(native))
     {
       if (native.ch_layout_.nb_channels > 2 &&
           actionDownmixToStereo->isChecked())
       {
         native.ch_layout_.set_default_layout(2);
       }

       AudioTraits supported;
       audioRenderer_->match(native, supported);

#if 0
       yae_debug << "supported: " << supported.ch_layout_.describe()
                 << ", required:  " << native.ch_layout_.describe();
#endif

       reader->setAudioTraitsOverride(supported);
     }
  }

  //----------------------------------------------------------------
  // MainWindow::autoCropCallback
  //
  TVideoFramePtr
  MainWindow::autoCropCallback(void * callbackContext,
                               const TCropFrame & cf,
                               bool detectionFinished)
  {
    MainWindow * mainWindow = (MainWindow *)callbackContext;

    if (detectionFinished)
    {
      qApp->postEvent(mainWindow, new AutoCropEvent(cf));
    }
    else if (mainWindow->playbackPaused_ ||
             mainWindow->videoRenderer_->isPaused())
    {
      // use the same frame again:
      return mainWindow->canvas_->currentFrame();
    }

    return TVideoFramePtr();
  }

};
