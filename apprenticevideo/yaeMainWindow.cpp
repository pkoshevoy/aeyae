// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created      : Sat Dec 18 17:55:21 MST 2010
// Copyright    : Pavel Koshevoy
// License      : MIT -- http://www.opensource.org/licenses/mit-license.php

// system includes:
#include <iostream>
#include <sstream>
#include <list>
#include <math.h>

// boost includes:
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#endif

// Qt includes:
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMimeData>
#include <QProcess>
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
#include <QQmlContext>
#include <QQuickItem>
#endif
#include <QShortcut>
#include <QSpacerItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>

// aeyae includes:
#include "yae/api/yae_version.h"
#include "yae/utils/yae_benchmark.h"
#include "yae/utils/yae_plugin_registry.h"
#include "yae/video/yae_pixel_formats.h"
#include "yae/video/yae_pixel_format_traits.h"
#include "yae/video/yae_video_renderer.h"

// local includes:
#ifdef __APPLE__
#include "yaeAudioUnitRenderer.h"
#endif
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
#include "yaeCanvasQuickFbo.h"
#include "yaeUtilsQml.h"
#endif
#include "yaeMainWindow.h"
#include "yaePortaudioRenderer.h"
#include "yaeTimelineModel.h"
#include "yaeThumbnailProvider.h"
#include "yaeUtilsQt.h"

// namespace shortcut:
namespace fs = boost::filesystem;


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
  // kShowTimeline
  //
  static const QString kShowTimeline =
    QString::fromUtf8("ShowTimeline");

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
    action_(NULL)
  {}


#ifdef YAE_USE_PLAYER_QUICK_WIDGET
  //----------------------------------------------------------------
  // getCanvas
  //
  static yae::Canvas *
  getCanvas(QQuickItem * item)
  {
    if (!item)
    {
      return NULL;
    }

    yae::CanvasQuickFbo * fbo = qobject_cast<yae::CanvasQuickFbo *>(item);
    if (fbo)
    {
      return &fbo->canvas_;
    }

    QList<QQuickItem *> children = item->childItems();
    for (QList<QQuickItem *>::iterator
           i = children.begin(), end = children.end(); i != end; ++i)
    {
      yae::Canvas * found = getCanvas(*i);
      if (found)
      {
        return found;
      }
    }

    return NULL;
  }

  //----------------------------------------------------------------
  // getCanvas
  //
  static yae::Canvas *
  getCanvas(TPlayerWidget * playerWidget)
  {
    // shortcut to the root QML item:
    QQuickItem * item = playerWidget->rootObject();
    return getCanvas(item);
  }
#else

  //----------------------------------------------------------------
  // getCanvas
  //
  static yae::Canvas *
  getCanvas(TPlayerWidget * playerWidget)
  {
    return playerWidget;
  }
#endif


  //----------------------------------------------------------------
  // context_toggle_fullscreen
  //
  static void
  context_toggle_fullscreen(void * context)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    mainWindow->requestToggleFullScreen();
  }


  //----------------------------------------------------------------
  // context_query_fullscreen
  //
  static bool
  context_query_fullscreen(void * context, bool & fullscreen)
  {
    MainWindow * mainWindow = (MainWindow *)context;
    fullscreen = mainWindow->isFullScreen();
    return true;
  }


  //----------------------------------------------------------------
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow(const IReaderPtr & readerPrototype):
    QMainWindow(NULL, 0),
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    subsTrackGroup_(NULL),
    chaptersGroup_(NULL),
    playRateMapper_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    subsTrackMapper_(NULL),
    chapterMapper_(NULL),
    bookmarksGroup_(NULL),
    bookmarksMapper_(NULL),
    bookmarksMenuSeparator_(NULL),
    readerPrototype_(readerPrototype),
    readerId_(0),
    playerWidget_(NULL),
    canvas_(NULL),
    audioRenderer_(NULL),
    videoRenderer_(NULL),
    playbackPaused_(false),
    scrollStart_(0.0),
    scrollOffset_(0.0),
    renderMode_(Canvas::kScaleToFit),
    xexpand_(1.0),
    yexpand_(1.0),
    tempo_(1.0),
    selVideo_(0, 1),
    selAudio_(0, 1),
    selSubsFormat_(kSubsNone)
  {
#ifdef __APPLE__
    appleRemoteControl_ = NULL;
#endif

    setupUi(this);
    setAcceptDrops(true);
    actionPlay->setText(tr("Pause"));

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon = QString::fromUtf8(":/images/apprenticevideo-256.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // setup the canvas widget (QML quick widget):
#ifndef YAE_USE_PLAYER_QUICK_WIDGET

#ifdef __APPLE__
    QString clickOrTap = tr("click");
#else
    QString clickOrTap = tr("tap");
#endif

    QString greeting =
      tr("drop video/music files here\n\n"
         "press Spacebar to pause/resume playback\n\n"
         "%1 %2 to toggle the playlist view\n\n"
         "press Alt %3, Alt %4 to skip through the playlist\n\n"
#ifdef __APPLE__
         "use Apple Remote to change volume or skip along the timeline\n\n"
#endif
         "explore the menus for more options").
      arg(clickOrTap).
      arg(QString::fromUtf8("\xE2""\x98""\xB0")). // hamburger
      arg(QString::fromUtf8("\xE2""\x86""\x90")). // left arrow
      arg(QString::fromUtf8("\xE2""\x86""\x92")); // right arrow

    playlistView_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    timelineView_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);
    frameCropView_.toggle_fullscreen_.reset(&context_toggle_fullscreen, this);

    playlistView_.query_fullscreen_.reset(&context_query_fullscreen, this);
    timelineView_.query_fullscreen_.reset(&context_query_fullscreen, this);
    frameCropView_.query_fullscreen_.reset(&context_query_fullscreen, this);

#ifdef YAE_USE_QOPENGL_WIDGET
    playerWidget_ = new TPlayerWidget(this, Qt::Widget);
    playerWidget_->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
#else
    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);
    contextFormat.setSampleBuffers(false);
    playerWidget_ = new TPlayerWidget(contextFormat, this, playerWidget_);
#endif
    playerWidget_->setGreeting(greeting);
    playerWidget_->append(&playlistView_);
    playerWidget_->append(&timelineView_);
    playerWidget_->append(&frameCropView_);

#else
    playerWidget_ = new TPlayerWidget(this);
    playerWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
#endif
    playerWidget_->setFocusPolicy(Qt::StrongFocus);
    playerWidget_->setAcceptDrops(true);

    // insert player widget into the main window layout:
    canvasLayout->addWidget(playerWidget_);

    YAE_ASSERT(readerPrototype_);
    reader_.reset(readerPrototype_->clone());

#ifdef __APPLE__
    audioRenderer_ = AudioUnitRenderer::create();
#else
    audioRenderer_ = PortaudioRenderer::create();
#endif
    videoRenderer_ = VideoRenderer::create();

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

#if 1
    actionFullScreen->setShortcut(tr("Ctrl+F"));
#elif defined(__APPLE__)
    actionFullScreen->setShortcut(tr("Ctrl+Shift+F"));
#else
    actionFullScreen->setShortcut(tr("F11"));
#endif

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
    shortcutCropOther_ = new QShortcut(this);
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
    shortcutCropOther_->setContext(Qt::ApplicationShortcut);
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
    cropFrameGroup->addAction(actionCropFrameOther);
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

    playRateMapper_ = new QSignalMapper(this);
    playRateMapper_->setMapping(actionTempo50, 50);
    playRateMapper_->setMapping(actionTempo60, 60);
    playRateMapper_->setMapping(actionTempo70, 70);
    playRateMapper_->setMapping(actionTempo80, 80);
    playRateMapper_->setMapping(actionTempo90, 90);
    playRateMapper_->setMapping(actionTempo100, 100);
    playRateMapper_->setMapping(actionTempo111, 111);
    playRateMapper_->setMapping(actionTempo125, 125);
    playRateMapper_->setMapping(actionTempo143, 143);
    playRateMapper_->setMapping(actionTempo167, 167);
    playRateMapper_->setMapping(actionTempo200, 200);

    bool ok = true;
    ok = connect(playRateMapper_, SIGNAL(mapped(int)),
                 this, SLOT(playbackSetTempo(int)));
    YAE_ASSERT(ok);

    ok = connect(actionTempo50, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo60, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo70, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo80, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo90, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo100, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo111, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo125, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo143, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo167, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
    YAE_ASSERT(ok);

    ok = connect(actionTempo200, SIGNAL(triggered()),
                 playRateMapper_, SLOT(map()));
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

    ok = connect(actionCropFrameOther, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrameOther()));
    YAE_ASSERT(ok);

    ok = connect(shortcutCropOther_, SIGNAL(activated()),
                 actionCropFrameOther, SLOT(trigger()));
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

    ok = connect(actionSetInPoint, SIGNAL(triggered()),
                 this, SIGNAL(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(actionSetOutPoint, SIGNAL(triggered()),
                 this, SIGNAL(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(&playlistModel_, SIGNAL(itemCountChanged()),
                 this, SLOT(fixupNextPrev()));
    YAE_ASSERT(ok);

    ok = connect(actionRemove_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(actionSelectAll_, SIGNAL(triggered()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(shortcutRemove_, SIGNAL(activated()),
                 &playlistModel_, SLOT(removeSelected()));
    YAE_ASSERT(ok);

    ok = connect(shortcutSelectAll_, SIGNAL(activated()),
                 &playlistModel_, SLOT(selectAll()));
    YAE_ASSERT(ok);

    ok = connect(&playlistModel_,
                 SIGNAL(playingItemChanged(const QModelIndex &)),
                 this,
                 SLOT(setPlayingItem(const QModelIndex &)));
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

    ok = connect(actionShowPlaylist, SIGNAL(toggled(bool)),
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
                 this, SLOT(playbackDeinterlacing()));
    YAE_ASSERT(ok);

#ifndef YAE_USE_PLAYER_QUICK_WIDGET
    ok = connect(&(playerWidget_->sigs_), SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(&(playerWidget_->sigs_), SIGNAL(maybeHideCursor()),
                 &(playerWidget_->sigs_), SLOT(hideCursor()));
    YAE_ASSERT(ok);
#endif

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
                 &timelineModel_, SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setOutPoint()),
                 &timelineModel_, SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(&timelineModel_, SIGNAL(userIsSeeking(bool)),
                 this, SLOT(userIsSeeking(bool)));
    YAE_ASSERT(ok);

    ok = connect(&timelineModel_, SIGNAL(moveTimeIn(double)),
                 this, SLOT(moveTimeIn(double)));
    YAE_ASSERT(ok);

    ok = connect(&timelineModel_, SIGNAL(moveTimeOut(double)),
                 this, SLOT(moveTimeOut(double)));
    YAE_ASSERT(ok);

    ok = connect(&timelineModel_, SIGNAL(movePlayHead(double)),
                 this, SLOT(movePlayHead(double)));
    YAE_ASSERT(ok);

    ok = connect(&timelineModel_, SIGNAL(clockStopped(const SharedClock &)),
                 this, SLOT(playbackFinished(const SharedClock &)));
    YAE_ASSERT(ok);

    ok = connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                 this, SLOT(focusChanged(QWidget *, QWidget *)));
    YAE_ASSERT(ok);

    ok = connect(&scrollWheelTimer_, SIGNAL(timeout()),
                 this, SLOT(scrollWheelTimerExpired()));
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

    ok = connect(menuChapters, SIGNAL(aboutToShow()),
                 this, SLOT(updateChaptersMenu()));
    YAE_ASSERT(ok);

    ok = connect(actionNextChapter, SIGNAL(triggered()),
                 this, SLOT(skipToNextChapter()));
    YAE_ASSERT(ok);

    ok = connect(shortcutNextChapter_, SIGNAL(activated()),
                 actionNextChapter, SLOT(trigger()));
    YAE_ASSERT(ok);

    ok = connect(&frameCropView_,
                 SIGNAL(cropped(const TVideoFramePtr &, const TCropFrame &)),
                 this,
                 SLOT(cropped(const TVideoFramePtr &, const TCropFrame &)));
    YAE_ASSERT(ok);

    ok = connect(&frameCropView_, SIGNAL(done()),
                 this, SLOT(dismissFrameCropView()));
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

    canvas_->cropAutoDetectStop();
    delete playerWidget_;
  }

  //----------------------------------------------------------------
  // MainWindow::canvas
  //
  Canvas *
  MainWindow::canvas() const
  {
    return canvas_;
  }

  //----------------------------------------------------------------
  // PlaylistItemFilePath
  //
  struct PlaylistItemFilePath : ThumbnailProvider::GetFilePath
  {
    PlaylistItemFilePath(const TPlaylistModel & playlist):
      playlist_(playlist)
    {}

    // virtual:
    QString operator()(const QString & id) const
    { return playlist_.lookupItemFilePath(id); }

    const TPlaylistModel & playlist_;
  };

  //----------------------------------------------------------------
  // MainWindow::initPlayerWidget
  //
  void
  MainWindow::initPlayerWidget()
  {
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    QString greeting =
      tr("drop videos/music here\n\n"
         "press spacebar to pause/resume\n\n"
         "alt-left/alt-right to navigate playlist\n\n"
#ifdef __APPLE__
         "use apple remote for volume and seeking\n\n"
#endif
         "explore the menus for more options");

    qmlRegisterType<yae::CanvasQuickFbo>("com.aragog.apprenticevideo",
                                         1, // major
                                         0, // minor
                                         "CanvasQuickFbo");

    qmlRegisterType<yae::TPlaylistModel>("com.aragog.apprenticevideo",
                                         1, // major
                                         0, // minor
                                         "TPlaylistModel");

    qmlRegisterType<yae::TimelineModel>("com.aragog.apprenticevideo",
                                        1, // major
                                        0, // minor
                                        "TimelineModel");

    qmlRegisterType<yae::UtilsQml>("com.aragog.apprenticevideo",
                                   1, // major
                                   0, // minor
                                   "UtilsQml");

    // add image://thumbnails/... provider:
    QQmlEngine * qmlEngine = playerWidget_->engine();
    boost::shared_ptr<ThumbnailProvider::GetFilePath>
      getFilePath(new PlaylistItemFilePath(playlistModel_));
    ThumbnailProvider * imageProvider =
      new ThumbnailProvider(readerPrototype_, getFilePath);
    qmlEngine->addImageProvider(QString::fromUtf8("thumbnails"),
                                imageProvider);

    // set playlist model:
    QQmlContext * qmlContext = playerWidget_->rootContext();
    qmlContext->setContextProperty("yae_playlist_model", &playlistModel_);
    qmlContext->setContextProperty("yae_timeline_model", &timelineModel_);
    qmlContext->setContextProperty("yae_qml_utils", UtilsQml::singleton());

    // start the widget:
    playerWidget_->setSource(QUrl("qrc:///qml/Player.qml"));

    // shortcut to the root QML item:
    QQuickItem * playerItem = playerWidget_->rootObject();

    // set playlist-footer greeting message:
    {
      QQuickItem * item = playerItem->findChild<QQuickItem *>("greeting");
      if (item)
      {
        item->setProperty("text", greeting);
      }
    }

    bool ok = true;
    ok = connect(playerItem, SIGNAL(toggleFullScreen()),
                 this, SLOT(requestToggleFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(exitFullScreen()),
                 this, SLOT(exitFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(exitPlaylist()),
                 this, SLOT(exitPlaylist()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(togglePlayback()),
                 this, SLOT(togglePlayback()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(skipToInPoint()),
                 this, SLOT(skipToInPoint()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(skipToOutPoint()),
                 this, SLOT(skipToOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(skipForward()),
                 this, SLOT(skipForward()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(skipBack()),
                 this, SLOT(skipBack()));
    YAE_ASSERT(ok);

    ok = connect(playerItem, SIGNAL(stepOneFrameForward()),
                 this, SLOT(skipToNextFrame()));
    YAE_ASSERT(ok);
#endif

    // get a shortcut to the Canvas (owned by the QML canvas widget):
    canvas_ = yae::getCanvas(playerWidget_);
    YAE_ASSERT(canvas_);

    // hide the playlist:
    actionShowPlaylist->setChecked(false);

    // restore timeline preference (default == hide):
    bool showTimeline = loadBooleanSettingOrDefault(kShowTimeline, false);
    actionShowTimeline->setChecked(showTimeline);
  }

  //----------------------------------------------------------------
  // MainWindow::initItemViews
  //
  void
  MainWindow::initItemViews()
  {
    // initialize frame crop view:
    TMakeCurrentContext currentContext(canvas_->context());
    playlistView_.setup(this);
    playlistView_.setModel(&playlistModel_);
    playlistView_.setEnabled(false);

    // add image://thumbnails/... provider:
    boost::shared_ptr<ThumbnailProvider::GetFilePath>
      getFilePath(new PlaylistItemFilePath(playlistModel_));
    yae::shared_ptr<ThumbnailProvider, ImageProvider>
      imageProvider(new ThumbnailProvider(readerPrototype_, getFilePath));
    playlistView_.addImageProvider(QString::fromUtf8("thumbnails"),
                                   imageProvider);

    timelineView_.setup(this, &playlistView_, &timelineModel_);
    timelineView_.setEnabled(true);
    frameCropView_.setEnabled(false);
    frameCropView_.init(&playlistView_);

    CanvasRendererItem & rendererItem =
      frameCropView_.root()->get<CanvasRendererItem>("uncropped");

    onLoadFrame_.reset(new OnFrameLoaded(rendererItem));
    playerWidget_->addLoadFrameObserver(onLoadFrame_);
  }

  //----------------------------------------------------------------
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const std::list<QString> & playlist,
                          bool beginPlaybackImmediately)
  {
    bool resumeFromBookmark = actionResumeFromBookmark->isChecked();

    std::list<BookmarkHashInfo> hashInfo;
    {
      BlockSignal block(&playlistModel_,
                        SIGNAL(playingItemChanged(const QModelIndex &)),
                        this,
                        SLOT(setPlayingItem(const QModelIndex &)));

      playlistModel_.add(playlist, resumeFromBookmark ? &hashInfo : NULL);
    }

    if (!beginPlaybackImmediately)
    {
      return;
    }

    if (resumeFromBookmark)
    {
      // look for a matching bookmark, resume playback if a bookmark exist:
      PlaylistBookmark bookmark;
      TPlaylistItemPtr found;

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
            found = playlistModel_.lookup(bookmark.groupHash_,
                                          bookmark.itemHash_);
            bookmark.item_ = found;
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

    QModelIndex playingIndex = playlistModel_.playingItem();
    playlistView_.ensureVisible(playingIndex);
  }

  //----------------------------------------------------------------
  // find_matching_program
  //
  static std::size_t
  find_matching_program(const std::vector<TTrackInfo> & track_info,
                        const TTrackInfo & target)
  {
    std::size_t program = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0, n = track_info.size(); i < n; i++)
    {
      const TTrackInfo & info = track_info[i];
      if (target.nprograms_ == info.nprograms_ &&
          target.program_ == info.program_ &&
          target.ntracks_ == n)
      {
        return target.program_;
      }

      program = std::min(program, info.program_);
    }

    return track_info.empty() ? 0 : program;
  }

  //----------------------------------------------------------------
  // findMatchingTrack
  //
  template <typename TTraits>
  static std::size_t
  findMatchingTrack(const std::vector<TTrackInfo> & trackInfo,
                    const std::vector<TTraits> & trackTraits,
                    const TTrackInfo & selInfo,
                    const TTraits & selTraits,
                    std::size_t program)
  {
    std::size_t n = trackInfo.size();

    if (!selInfo.isValid())
    {
      // video was disabled, keep it disabled:
      return n;
    }

    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trackInfo.front();
      return info.index_;
    }

    // try to find a matching track:
    std::vector<TTrackInfo> trkInfo = trackInfo;
    std::vector<TTraits> trkTraits = trackTraits;

    std::vector<TTrackInfo> tmpInfo;
    std::vector<TTraits> tmpTraits;

    // try to match the language code:
    if (selInfo.hasLang())
    {
      const char * selLang = selInfo.lang();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        if (program != info.program_)
        {
          continue;
        }

        const TTraits & traits = trkTraits[i];
        if (info.hasLang() && strcmp(info.lang(), selLang) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track name:
    if (selInfo.hasName())
    {
      const char * selName = selInfo.name();

      for (std::size_t i = 0; i < n; i++)
      {
        const TTrackInfo & info = trkInfo[i];
        const TTraits & traits = trkTraits[i];

        if (program != info.program_)
        {
          continue;
        }

        if (info.hasName() && strcmp(info.name(), selName) == 0)
        {
          tmpInfo.push_back(info);
          tmpTraits.push_back(traits);
        }
      }

      if (!tmpInfo.empty())
      {
        trkInfo = tmpInfo;
        trkTraits = tmpTraits;
        tmpInfo.clear();
        tmpTraits.clear();
      }
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      return info.index_;
    }

    // try to match track traits:
    for (std::size_t i = 0; i < n; i++)
    {
      const TTrackInfo & info = trkInfo[i];
      const TTraits & traits = trkTraits[i];

      if (program != info.program_)
      {
        continue;
      }

      if (selTraits == traits)
      {
        tmpInfo.push_back(info);
        tmpTraits.push_back(traits);
      }
    }

    if (!tmpInfo.empty())
    {
      trkInfo = tmpInfo;
      trkTraits = tmpTraits;
      tmpInfo.clear();
      tmpTraits.clear();
    }

    n = trkInfo.size();
    if (n == 1)
    {
      // only one candidate is available:
      const TTrackInfo & info = trkInfo.front();
      if (program == info.program_)
      {
        return info.index_;
      }
    }

    // try to match track index:
    if (trackInfo.size() == selInfo.ntracks_ &&
        trackInfo[selInfo.index_].program_ == program)
    {
      return selInfo.index_;
    }

    // try to find the first track of the matching program:
    n = trackInfo.size();
    for (std::size_t i = 0; i < n; i++)
    {
      const TTrackInfo & info = trackInfo[i];
      if (info.program_ == program)
      {
        return i;
      }
    }

    // disable track:
    return n;
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
  // MainWindow::load
  //
  bool
  MainWindow::load(const QString & path, const TBookmark * bookmark)
  {
    // actionPlay->setEnabled(false);

    IReaderPtr reader =
      canaryTest(path) ? yae::openFile(readerPrototype_, path) : IReaderPtr();

    if (!reader)
    {
#if 0
      yae_debug
        << "ERROR: could not open file: " << path.toUtf8().constData();
#endif
      return false;
    }

    // keep track of current closed caption selection:
    unsigned int cc = reader_->getRenderCaptions();

    ++readerId_;
    reader->setReaderId(readerId_);

    // prevent Canvas from rendering any pending frames from previous reader:
    canvas_->acceptFramesWithReaderId(readerId_);

    // disconnect timeline from renderers:
    timelineModel_.observe(SharedClock());

    // shut down the audio renderer before calling adjustAudioTraitsOverride,
    // hopefully that will avoid triggering the portaudio deadlock:
    reader_->close();
    videoRenderer_->close();
    audioRenderer_->close();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t subsCount = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackEnabled(!playbackPaused_);

#if 0
    yae_debug
      << "\nyae: " << filename
      << "\nyae: video tracks: " << numVideoTracks
      << "\nyae: audio tracks: " << numAudioTracks
      << "\nyae: subs tracks: " << subsCount;
#endif

    std::vector<TTrackInfo>  audioInfo;
    std::vector<AudioTraits> audioTraits;
    std::vector<TTrackInfo>  videoInfo;
    std::vector<VideoTraits> videoTraits;
    std::vector<TTrackInfo>  subsInfo;
    std::vector<TSubsFormat> subsFormat;

    adjustMenuActions(reader.get(),
                      audioInfo,
                      audioTraits,
                      videoInfo,
                      videoTraits,
                      subsInfo,
                      subsFormat);

    bool rememberSelectedVideoTrack = false;

    std::size_t program = find_matching_program(videoInfo, selVideo_);
    std::size_t vtrack = findMatchingTrack<VideoTraits>(videoInfo,
                                                        videoTraits,
                                                        selVideo_,
                                                        selVideoTraits_,
                                                        program);
    if (bookmark && bookmark->vtrack_ <= numVideoTracks)
    {
      vtrack = bookmark->vtrack_;
      rememberSelectedVideoTrack = numVideoTracks > 0;
    }

    if (vtrack < numVideoTracks)
    {
      program = videoInfo[vtrack].program_;
    }

    bool rememberSelectedAudioTrack = false;
    std::size_t atrack = findMatchingTrack<AudioTraits>(audioInfo,
                                                        audioTraits,
                                                        selAudio_,
                                                        selAudioTraits_,
                                                        program);
    if (bookmark && bookmark->atrack_ <= numAudioTracks)
    {
      atrack = bookmark->atrack_;
      rememberSelectedAudioTrack = numAudioTracks > 0;
    }

    if (atrack < numAudioTracks)
    {
      YAE_ASSERT(program == audioInfo[vtrack].program_);
      program = audioInfo[vtrack].program_;
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

    selectVideoTrack(reader.get(), vtrack);
    videoTrackGroup_->actions().at((int)vtrack)->setChecked(true);

    if (rememberSelectedVideoTrack)
    {
      reader->getSelectedVideoTrackInfo(selVideo_);
      reader->getVideoTraits(selVideoTraits_);
    }

    selectAudioTrack(reader.get(), atrack);
    audioTrackGroup_->actions().at((int)atrack)->setChecked(true);

    if (rememberSelectedAudioTrack)
    {
      reader->getSelectedAudioTrackInfo(selAudio_);
      reader->getAudioTraits(selAudioTraits_);
    }

    bool rememberSelectedSubtitlesTrack = false;
    std::size_t strack = findMatchingTrack<TSubsFormat>(subsInfo,
                                                        subsFormat,
                                                        selSubs_,
                                                        selSubsFormat_,
                                                        program);
    if (bookmark)
    {
      if (!bookmark->subs_.empty() &&
          bookmark->subs_.front() < subsCount)
      {
        strack = bookmark->subs_.front();
        rememberSelectedSubtitlesTrack = true;
      }
      else if (bookmark->subs_.empty())
      {
        strack = subsCount;
        rememberSelectedSubtitlesTrack = true;
      }

      if (bookmark->cc_)
      {
        cc = bookmark->cc_;
      }
    }

    selectSubsTrack(reader.get(),
                    strack < subsCount ? strack : subsCount + cc);

    if (strack < subsCount)
    {
      subsTrackGroup_->actions().at((int)strack)->setChecked(true);
    }
    else if (cc)
    {
      int index = (int)subsCount + cc - 1;
      subsTrackGroup_->actions().at(index)->setChecked(true);
    }
    else
    {
      int index = subsTrackGroup_->actions().size() - 1;
      subsTrackGroup_->actions().at(index)->setChecked(true);
    }

    if (rememberSelectedSubtitlesTrack)
    {
      selSubsFormat_ = reader->subsInfo(strack, selSubs_);
    }

    adjustMenus(reader.get());

    // reset overlay plane to clean state, reset libass wrapper:
    canvas_->clearOverlay();

    // hide the welcome screen:
    {
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
      QQuickItem * playerItem = playerWidget_->rootObject();
      std::string playerState = playerItem->state().toUtf8().constData();
      if (playerState == "welcome")
      {
        playerItem->setState(QString::fromUtf8("playback"));
      }
#endif
    }

    // reset timeline start, duration, playhead, in/out points:
    timelineModel_.resetFor(reader.get());

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

      const char * filename =
        filenameFound != att->metadata_.end() ?
        filenameFound->second.c_str() :
        NULL;

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
            canvas_->libassAddFont(filename, att->data_, att->size_);
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

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepareReaderAndRenderers(reader.get(), playbackPaused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // replace the previous reader:
    reader_ = reader;

    // allow renderers to read from output frame queues:
    resumeRenderers(true);

    this->setWindowTitle(tr("Apprentice Video: %1").
                         arg(QFileInfo(path).fileName()));
    // actionPlay->setEnabled(true);

    if (actionCropFrameAutoDetect->isChecked())
    {
      autocropTimer_.start(1900);
    }
    else
    {
      QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
    }

    bookmarkTimer_.start();

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

    QModelIndex itemIndexNowPlaying = playlistModel_.playingItem();
    std::size_t playingBookmarkIndex = std::numeric_limits<std::size_t>::max();

    QModelIndex rootIndex = playlistModel_.makeModelIndex(-1, -1);
    const int numGroups = playlistModel_.rowCount(rootIndex);

    for (int i = 0; i < numGroups; i++)
    {
      QModelIndex groupIndex = playlistModel_.makeModelIndex(i, -1);

      TPlaylistGroupPtr group;
      playlistModel_.lookup(groupIndex, &group);

      // check whether there is a bookmark for an item in this group:
      PlaylistBookmark bookmark;
      if (!yae::loadBookmark(group->hash_, bookmark))
      {
        continue;
      }

      // check whether the item hash matches a group item:
      const int groupSize = playlistModel_.rowCount(groupIndex);
      for (int j = 0; j < groupSize; j++)
      {
        QModelIndex itemIndex = playlistModel_.makeModelIndex(i, j);
        TPlaylistItemPtr item = playlistModel_.lookup(itemIndex);

        if (!item || item->hash_ != bookmark.itemHash_)
        {
          continue;
        }

        // found a match, add it to the bookmarks menu:
        bookmark.item_ = item;

        std::string ts = TTime(bookmark.positionInSeconds_).to_hhmmss(":");

        if (!bookmarksGroup_)
        {
          bookmarksGroup_ = new QActionGroup(this);
          bookmarksMapper_ = new QSignalMapper(this);

          bool ok = connect(bookmarksMapper_, SIGNAL(mapped(int)),
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
          escapeAmpersand(item->name_) +
          QString::fromUtf8(", ") +
          QString::fromUtf8(ts.c_str());

        bookmark.action_ = new QAction(name, this);

        bool nowPlaying = (itemIndexNowPlaying == itemIndex);
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
    QModelIndex itemIndex = playlistModel_.playingItem();
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlistModel_.lookup(itemIndex, &group);
    if (!item || !group)
    {
      return;
    }

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      if (bookmark.groupHash_ != group->hash_ ||
          bookmark.itemHash_ != item->hash_)
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
    canvas_->cropAutoDetectStop();
    canvas_->cropFrame(0.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_40
  //
  void
  MainWindow::playbackCropFrame2_40()
  {
    canvas_->cropFrame(2.40);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame2_35
  //
  void
  MainWindow::playbackCropFrame2_35()
  {
    canvas_->cropFrame(2.35);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_85
  //
  void
  MainWindow::playbackCropFrame1_85()
  {
    canvas_->cropFrame(1.85);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_78
  //
  void
  MainWindow::playbackCropFrame1_78()
  {
    canvas_->cropFrame(16.0 / 9.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_60
  //
  void
  MainWindow::playbackCropFrame1_60()
  {
    canvas_->cropFrame(1.6);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrame1_33
  //
  void
  MainWindow::playbackCropFrame1_33()
  {
    canvas_->cropFrame(4.0 / 3.0);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackCropFrameOther
  //
  void
  MainWindow::playbackCropFrameOther()
  {
    CanvasRenderer * renderer = canvas_->canvasRenderer();

    TVideoFramePtr frame;
    renderer->getFrame(frame);
    if (!frame)
    {
      return;
    }

    // pass current frame crop info to the view:
    {
      TCropFrame crop;
      renderer->getCroppedFrame(crop);

      SignalBlocker blockSignals;
      blockSignals << &frameCropView_;
      frameCropView_.setCrop(frame, crop);
    }

    timelineView_.setEnabled(false);
    frameCropView_.setEnabled(true);
    onLoadFrame_->frameLoaded(canvas_, frame);
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
#if 0
    yae_debug << "playbackColorConverter";
#endif

    bool skipColorConverter = actionSkipColorConverter->isChecked();
    saveBooleanSetting(kSkipColorConverter, skipColorConverter);

    TIgnoreClockStop ignoreClockStop(timelineModel_);
    reader_->threadStop();
    stopRenderers();

    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    selectVideoTrack(reader_.get(), videoTrack);
    prepareReaderAndRenderers(reader_.get(), playbackPaused_);

    double t = timelineModel_.currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers(true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoopFilter
  //
  void
  MainWindow::playbackLoopFilter()
  {
#if 0
    yae_debug << "playbackLoopFilter";
#endif

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
#if 0
    yae_debug << "playbackNonReferenceFrames";
#endif

    bool skipNonReferenceFrames = actionSkipNonReferenceFrames->isChecked();
    saveBooleanSetting(kSkipNonReferenceFrames, skipNonReferenceFrames);

    reader_->skipNonReferenceFrames(skipNonReferenceFrames);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackDeinterlacing
  //
  void
  MainWindow::playbackDeinterlacing()
  {
    bool deint = actionDeinterlace->isChecked();
    reader_->setDeinterlacing(deint);
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
    QAction * found =
      qobject_cast<QAction *>(playRateMapper_->mapping(percent));

    if (found)
    {
      if (!found->isChecked())
      {
        found->setChecked(true);
      }

      tempo_ = double(percent) / 100.0;
      reader_->setTempo(tempo_);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowPlaylist
  //
  void
  MainWindow::playbackShowPlaylist()
  {
    timelineView_.modelChanged();

    bool showPlaylist = actionShowPlaylist->isChecked();

#ifdef YAE_USE_PLAYER_QUICK_WIDGET
    QQuickItem * playerItem = playerWidget_->rootObject();
    if (!playerItem)
    {
      return;
    }

    const char * state =
      showPlaylist ? "playlist" :
      playlistModel_.hasItems() ? "playback" :
      "welcome";
    playerItem->setState(QString::fromUtf8(state));
#else

    std::ostringstream oss;
    YAE_LIFETIME_SHOW(oss);
    YAE_BENCHMARK_SHOW(oss);
    YAE_BENCHMARK_CLEAR();
    YAE_LIFETIME_CLEAR();

#if 0 // ndef NDEBUG
    std::string desktop =
      YAE_STANDARD_LOCATION(DesktopLocation).toUtf8().constData();
    std::string timesheet = (fs::path(desktop) / "aeyae.timesheet").string();
    yae::dump(timesheet, oss.str().c_str(), oss.str().size());
#endif

    playlistView_.setEnabled(showPlaylist);
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowTimeline
  //
  void
  MainWindow::playbackShowTimeline()
  {
    saveBooleanSetting(kShowTimeline, actionShowTimeline->isChecked());
    timelineView_.root()->uncache();
    timelineView_.maybeAnimateOpacity();
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

    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader_->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    canvasSizeBackup();

    double scale = std::min<double>(xexpand_, yexpand_);
    canvasSizeSet(scale, scale);
  }

  //----------------------------------------------------------------
  // swapShortcuts
  //
  static inline void
  swapShortcuts(QShortcut * a, QAction * b)
  {
    QKeySequence tmp = a->key();
    a->setKey(b->shortcut());
    b->setShortcut(tmp);
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
    yae::swapShortcuts(shortcutCropOther_, actionCropFrameOther);
    yae::swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
    yae::swapShortcuts(shortcutNextChapter_, actionNextChapter);
    yae::swapShortcuts(shortcutAspectRatioNone_, actionAspectRatioAuto);
    yae::swapShortcuts(shortcutAspectRatio1_33_, actionAspectRatio1_33);
    yae::swapShortcuts(shortcutAspectRatio1_78_, actionAspectRatio1_78);
  }

  //----------------------------------------------------------------
  // MainWindow::requestToggleFullScreen
  //
  void
  MainWindow::requestToggleFullScreen()
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
  // MainWindow::exitPlaylist
  //
  void
  MainWindow::exitPlaylist()
  {
    actionShowPlaylist->setChecked(false);
  }

  //----------------------------------------------------------------
  // MainWindow::togglePlaylist
  //
  void
  MainWindow::togglePlaylist()
  {
    actionShowPlaylist->trigger();
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
      actionPlay->setText(tr("Pause"));
      prepareReaderAndRenderers(reader_.get(), playbackPaused_);
      resumeRenderers();

      bookmarkTimer_.start();
    }
    else
    {
      actionPlay->setText(tr("Play"));
      TIgnoreClockStop ignoreClockStop(timelineModel_);
      stopRenderers();
      prepareReaderAndRenderers(reader_.get(), playbackPaused_);

      bookmarkTimer_.stop();
      saveBookmark();
    }

    timelineView_.modelChanged();
    timelineView_.maybeAnimateOpacity();

    if (!playbackPaused_)
    {
      timelineView_.forceAnimateControls();
    }
    else
    {
      timelineView_.maybeAnimateControls();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::skipToInPoint
  //
  void
  MainWindow::skipToInPoint()
  {
    timelineModel_.seekTo(timelineModel_.timeIn());
  }

  //----------------------------------------------------------------
  // MainWindow::skipToOutPoint
  //
  void
  MainWindow::skipToOutPoint()
  {
    timelineModel_.seekTo(timelineModel_.timeOut());
  }

  //----------------------------------------------------------------
  // MainWindow::skipForward
  //
  void
  MainWindow::skipForward()
  {
    timelineModel_.seekFromCurrentTime(7.0);
    timelineView_.maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // MainWindow::skipBack
  //
  void
  MainWindow::skipBack()
  {
    timelineModel_.seekFromCurrentTime(-3.0);
    timelineView_.maybeAnimateOpacity();
  }

  //----------------------------------------------------------------
  // MainWindow::audioDownmixToStereo
  //
  void
  MainWindow::audioDownmixToStereo()
  {
    saveBooleanSetting(kDownmixToStereo, actionDownmixToStereo->isChecked());

    // reset reader:
    TIgnoreClockStop ignoreClockStop(timelineModel_);
    reader_->threadStop();

    stopRenderers();
    prepareReaderAndRenderers(reader_.get(), playbackPaused_);

    double t = timelineModel_.currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // get_curr_program
  //
  static std::size_t
  get_curr_program(IReader * reader,
                   TTrackInfo & vinfo,
                   TTrackInfo & ainfo,
                   TTrackInfo & sinfo)
  {
    vinfo = TTrackInfo(0, 0);
    ainfo = TTrackInfo(0, 0);
    sinfo = TTrackInfo(0, 0);

    std::size_t ix_vtrack = reader->getSelectedVideoTrackIndex();
    std::size_t n_vtracks = reader->getNumberOfVideoTracks();
    if (ix_vtrack < n_vtracks)
    {
      reader->getSelectedVideoTrackInfo(vinfo);
    }

    std::size_t ix_atrack = reader->getSelectedAudioTrackIndex();
    std::size_t n_atracks = reader->getNumberOfAudioTracks();
    if (ix_atrack < n_atracks)
    {
      reader->getSelectedAudioTrackInfo(ainfo);
    }

    std::size_t n_subs = reader->subsCount();
    for (std::size_t i = 0; i < n_subs; i++)
    {
      if (reader->getSubsRender(i))
      {
        reader->subsInfo(i, sinfo);
        break;
      }
    }

    return (vinfo.isValid() ? vinfo.program_ :
            ainfo.isValid() ? ainfo.program_ :
            sinfo.isValid() ? sinfo.program_ :
            0);
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
    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program =
      get_curr_program(reader_.get(), vinfo, ainfo, sinfo);

    TIgnoreClockStop ignoreClockStop(timelineModel_);
    reader_->threadStop();
    stopRenderers();

    selectAudioTrack(reader_.get(), index);
    reader_->getSelectedAudioTrackInfo(selAudio_);
    reader_->getAudioTraits(selAudioTraits_);

    // if the audio program is not the same as the video program
    // then change the video track to a matching audio program:
    if (selAudio_.isValid() && selAudio_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader_->getProgramInfo(selAudio_.program_, program));

      if (vinfo.isValid())
      {
        // select another video track:
        std::size_t i = program.video_.empty() ? 0 : program.video_.front();
        selectVideoTrack(reader_.get(), i);
        reader_->getSelectedVideoTrackInfo(selVideo_);
        reader_->getVideoTraits(selVideoTraits_);

        QList<QAction *> actions = videoTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }

      if (sinfo.isValid())
      {
        // select another subtitle track:
        std::size_t i = program.subs_.empty() ? 0 : program.subs_.front();
        selectSubsTrack(reader_.get(), i);
        reader_->subsInfo(i, selSubs_);

        QList<QAction *> actions = subsTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }
    }

    prepareReaderAndRenderers(reader_.get(), playbackPaused_);

    if (selAudio_.isValid() && selAudio_.program_ == prev_program)
    {
      double t = timelineModel_.currentTime();
      reader_->seek(t);
    }
    else
    {
      timelineModel_.resetFor(reader_.get());
    }

    reader_->threadStart();

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
    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program =
      get_curr_program(reader_.get(), vinfo, ainfo, sinfo);

    TIgnoreClockStop ignoreClockStop(timelineModel_);
    reader_->threadStop();
    stopRenderers();

    selectVideoTrack(reader_.get(), index);
    reader_->getSelectedVideoTrackInfo(selVideo_);
    reader_->getVideoTraits(selVideoTraits_);

    // if the video program is not the same as the audio program
    // then change the audio track to a matching video program:
    if (selVideo_.isValid() && selVideo_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader_->getProgramInfo(selVideo_.program_, program));

      if (ainfo.isValid())
      {
        // select another audio track:
        std::size_t i = program.audio_.empty() ? 0 : program.audio_.front();
        selectAudioTrack(reader_.get(), i);
        reader_->getSelectedAudioTrackInfo(selAudio_);
        reader_->getAudioTraits(selAudioTraits_);

        QList<QAction *> actions = audioTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }

      if (sinfo.isValid())
      {
        // select another subtitle track:
        std::size_t i = program.subs_.empty() ? 0 : program.subs_.front();
        selectSubsTrack(reader_.get(), i);
        reader_->subsInfo(i, selSubs_);

        QList<QAction *> actions = subsTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }
    }

    prepareReaderAndRenderers(reader_.get(), playbackPaused_);

    if (selVideo_.isValid() && selVideo_.program_ == prev_program)
    {
      double t = timelineModel_.currentTime();
      reader_->seek(t);
    }
    else
    {
      timelineModel_.resetFor(reader_.get());
    }

    reader_->threadStart();

    resumeRenderers(true);
    QTimer::singleShot(1900, this, SLOT(adjustCanvasHeight()));
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
    TTrackInfo vinfo(0, 0);
    TTrackInfo ainfo(0, 0);
    TTrackInfo sinfo(0, 0);
    std::size_t prev_program =
      get_curr_program(reader_.get(), vinfo, ainfo, sinfo);

    TIgnoreClockStop ignoreClockStop(timelineModel_);
    reader_->threadStop();
    stopRenderers();

    selectSubsTrack(reader_.get(), index);
    selSubsFormat_ = reader_->subsInfo(index, selSubs_);

    // if the subtitles program is not the same as the audio/video program
    // then change the audio/video track to a matching subtitles program:
    if (selSubs_.isValid() && selSubs_.program_ != prev_program)
    {
      TProgramInfo program;
      YAE_ASSERT(reader_->getProgramInfo(selSubs_.program_, program));

      if (vinfo.isValid())
      {
        // select another video track:
        std::size_t i = program.video_.empty() ? 0 : program.video_.front();
        selectVideoTrack(reader_.get(), i);
        reader_->getSelectedVideoTrackInfo(selVideo_);
        reader_->getVideoTraits(selVideoTraits_);

        QList<QAction *> actions = videoTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }

      if (ainfo.isValid())
      {
        // select another audio track:
        std::size_t i = program.audio_.empty() ? 0 : program.audio_.front();
        selectAudioTrack(reader_.get(), i);
        reader_->getSelectedAudioTrackInfo(selAudio_);
        reader_->getAudioTraits(selAudioTraits_);

        QList<QAction *> actions = audioTrackGroup_->actions();
        actions[int(i)]->setChecked(true);
      }
    }

    prepareReaderAndRenderers(reader_.get(), playbackPaused_);

    if (selSubs_.isValid() && selSubs_.program_ == prev_program)
    {
      double t = timelineModel_.currentTime();
      reader_->seek(t);
    }
    else
    {
      timelineModel_.resetFor(reader_.get());
    }

    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::updateChaptersMenu
  //
  void
  MainWindow::updateChaptersMenu()
  {
    const double playheadInSeconds = timelineModel_.currentTime();
    QList<QAction *> actions = chaptersGroup_->actions();

    const std::size_t numActions = actions.size();
    const std::size_t numChapters = reader_->countChapters();

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
      if (reader_->getChapterInfo(i, ch))
      {
        double t0_sec = ch.t0_sec();
        double t1_sec = ch.t1_sec();

        if ((playheadInSeconds >= t0_sec &&
             playheadInSeconds < t1_sec) ||
            (playheadInSeconds < t0_sec && i > 0))
        {
          std::size_t index = (playheadInSeconds >= t0_sec) ? i : i - 1;

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
    const double playheadInSeconds = timelineModel_.currentTime();
    const std::size_t numChapters = reader_->countChapters();

    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      if (reader_->getChapterInfo(i, ch))
      {
        double t0_sec = ch.t0_sec();

        if (playheadInSeconds < t0_sec)
        {
          timelineModel_.seekTo(t0_sec);
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

    double t0_sec = ch.t0_sec();
    timelineModel_.seekTo(t0_sec);
  }

  //----------------------------------------------------------------
  // MainWindow::setPlayingItem
  //
  void
  MainWindow::setPlayingItem(const QModelIndex & index)
  {
    playbackStop();

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    if (!item)
    {
      canvas_->clear();
#ifndef YAE_USE_PLAYER_QUICK_WIDGET
      // FIXME: this could be handled as a playlist view state instead:
      canvas_->setGreeting(canvas_->greeting());
#endif
      // actionPlay->setEnabled(false);
      fixupNextPrev();
    }
    else
    {
      playback(index);

      QModelIndex playingIndex = playlistModel_.playingItem();
      playlistView_.ensureVisible(playingIndex);
    }

    if (!playlistModel_.hasItems())
    {
#ifdef YAE_USE_PLAYER_QUICK_WIDGET
      QQuickItem * playerItem = playerWidget_->rootObject();
      std::string playerState = playerItem->state().toUtf8().constData();

      if (playerState == "playback")
      {
        // show the welcome screen:
        playerItem->setState(QString::fromUtf8("welcome"));
      }
#endif
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
    canvasSizeSet(xexpand_ * 0.5, yexpand_ * 0.5);
  }

  //----------------------------------------------------------------
  // MainWindow::windowIncreaseSize
  //
  void
  MainWindow::windowIncreaseSize()
  {
    canvasSizeSet(xexpand_ * 2.0, yexpand_ * 2.0);
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
      about->setWindowTitle(tr("Apprentice Video (%1)").
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
    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalStart(seconds);

    SignalBlocker blockSignals(actionLoop);
    actionLoop->setChecked(true);
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeOut
  //
  void
  MainWindow::moveTimeOut(double seconds)
  {
    reader_->setPlaybackLooping(true);
    reader_->setPlaybackIntervalEnd(seconds);

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
      << "MOVE PLAYHEAD TO: " << TTime(seconds).to_hhmmss_ms(":");
#endif

    videoRenderer_->pause();
    audioRenderer_->pause();

    reader_->seek(seconds);

    // this avoids weird non-rendering of subtitles:
    canvas_->libassFlushTrack();

    resumeRenderers(true);
  }

  //----------------------------------------------------------------
  // MainWindow::focusChanged
  //
  void
  MainWindow::focusChanged(QWidget * prev, QWidget * curr)
  {
#if 0
    std::ostringstream oss;
    oss << "focus changed: " << prev << " -> " << curr;
    if (curr)
    {
      oss << ", " << curr->objectName().toUtf8().constData()
          << " (" << curr->metaObject()->className() << ")";
    }

    yae_debug << oss.str();
#endif

#ifdef __APPLE__
    if (!appleRemoteControl_ && curr)
    {
      appleRemoteControl_ =
        appleRemoteControlOpen(true, // exclusive
                               false, // count clicks
                               false, // simulate hold
                               &MainWindow::appleRemoteControlObserver,
                               this);
    }
    else if (appleRemoteControl_ && !curr)
    {
      appleRemoteControlClose(appleRemoteControl_);
      appleRemoteControl_ = NULL;
    }
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::playbackFinished
  //
  void
  MainWindow::playbackFinished(const SharedClock & c)
  {
    if (!timelineModel_.sharedClock().sharesCurrentTimeWith(c))
    {
#ifndef NDEBUG
      yae_debug << "NOTE: ignoring stale playbackFinished";
#endif
      return;
    }

    // remove current bookmark:
    bookmarkTimer_.stop();

    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    TPlaylistItemPtr next = playlistModel_.lookup(iNext);

    if (item && next && (&(item->group_) != &(next->group_)))
    {
      const PlaylistGroup * itemGroup = &(item->group_);
      const PlaylistGroup * nextGroup = &(next->group_);

      if (itemGroup != nextGroup)
      {
        PlaylistBookmark bookmark;

        // if current item was bookmarked, then remove it from bookmarks:
        if (findBookmark(item, bookmark))
        {
          yae::removeBookmark(itemGroup->hash_);
        }

        // if a bookmark exists for the next item group, then use it:
        if (nextGroup && findBookmark(nextGroup->hash_, bookmark))
        {
          gotoBookmark(bookmark);
          return;
        }
      }
    }

    if (!next && actionRepeatPlaylist->isChecked())
    {
      // repeat the playlist:
      QModelIndex first = playlistModel_.firstItem();
      playback(first);
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
    IReaderPtr reader(readerPrototype_->clone());
    timelineModel_.observe(SharedClock());
    timelineModel_.resetFor(reader.get());

    reader_->close();
    videoRenderer_->close();
    audioRenderer_->close();

    ++readerId_;
    canvas_->acceptFramesWithReaderId(readerId_);
    reader->setReaderId(readerId_);
    reader_ = reader;

    this->setWindowTitle(tr("Apprentice Video"));

    adjustMenuActions();
    adjustMenus(reader_.get());
  }

  //----------------------------------------------------------------
  // MainWindow::playback
  //
  void
  MainWindow::playback(bool forward)
  {
    QModelIndex current = playlistModel_.playingItem();
    playback(current, forward);
  }

  //----------------------------------------------------------------
  // MainWindow::playback
  //
  void
  MainWindow::playback(const QModelIndex & startHere, bool forward)
  {
    // actionPlay->setEnabled(false);

    QModelIndex current = startHere;
    TPlaylistItemPtr item;
    bool ok = false;

    while ((item = playlistModel_.lookup(current)))
    {
      item->failed_ = !load(item->path_);

      if (!item->failed_)
      {
        ok = true;
        break;
      }

      if (forward)
      {
        current = playlistModel_.nextItem(current);
      }
      else
      {
        current = playlistModel_.prevItem(current);
      }
    }

    playlistView_.ensureVisible(current);

    if (!ok && !forward)
    {
      playback(startHere, true);
      return;
    }

    // update playlist model:
    {
      BlockSignal block(&playlistModel_,
                        SIGNAL(playingItemChanged(const QModelIndex &)),
                        this,
                        SLOT(setPlayingItem(const QModelIndex &)));
      playlistModel_.setPlayingItem(current);
    }

    if (!ok)
    {
      playbackStop();
    }

    fixupNextPrev();
  }

  //----------------------------------------------------------------
  // MainWindow::fixupNextPrev
  //
  void
  MainWindow::fixupNextPrev()
  {
    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    QModelIndex iPrev =
      index.isValid() ?
      playlistModel_.prevItem(index) :
      playlistModel_.lastItem();

    TPlaylistItemPtr prev = playlistModel_.lookup(iPrev);
    TPlaylistItemPtr next = playlistModel_.lookup(iNext);

    actionPrev->setEnabled(!!prev);
    actionNext->setEnabled(index.isValid());

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
    // actionPlay->setEnabled(false);

    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iNext = playlistModel_.nextItem(index);

    playback(iNext, true);
  }

  //----------------------------------------------------------------
  // MainWindow::playbackPrev
  //
  void
  MainWindow::playbackPrev()
  {
    // actionPlay->setEnabled(false);

    QModelIndex index = playlistModel_.playingItem();
    QModelIndex iPrev =
      index.isValid() ?
      playlistModel_.prevItem(index) :
      playlistModel_.lastItem();

    if (iPrev.isValid())
    {
      playback(iPrev, false);
    }
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
    double seconds = scrollStart_ + scrollOffset_;

    bool isLooping = actionLoop->isChecked();
    if (isLooping)
    {
      double t0 = timelineModel_.timelineStart();
      double dt = timelineModel_.timelineDuration();
      seconds = t0 + fmod(seconds - t0, dt);
    }

    timelineModel_.seekTo(seconds);
    timelineView_.maybeAnimateOpacity();
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

    if (!reader_->isSeekable())
    {
      return;
    }

    QModelIndex itemIndex = playlistModel_.playingItem();
    TPlaylistGroupPtr group;
    TPlaylistItemPtr item = playlistModel_.lookup(itemIndex, &group);

    if (group && item)
    {
      double positionInSeconds = timelineModel_.currentTime();
      yae::saveBookmark(group->hash_,
                        item->hash_,
                        reader_.get(),
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
    // actionPlay->setEnabled(false);

    QModelIndex index = playlistModel_.lookupModelIndex(bookmark.groupHash_,
                                                        bookmark.itemHash_);

    TPlaylistItemPtr item = playlistModel_.lookup(index);
    if (item)
    {
      item->failed_ = !load(item->path_, &bookmark);

      if (!item->failed_)
      {
        playlistView_.ensureVisible(index);

        // update playlist model:
        BlockSignal block(&playlistModel_,
                          SIGNAL(playingItemChanged(const QModelIndex &)),
                          this,
                          SLOT(setPlayingItem(const QModelIndex &)));
        playlistModel_.setPlayingItem(index);
      }
    }

    fixupNextPrev();
  }

  //----------------------------------------------------------------
  // MainWindow::findBookmark
  //
  bool
  MainWindow::findBookmark(const TPlaylistItemPtr & item,
                           PlaylistBookmark & found) const
  {
    for (std::vector<PlaylistBookmark>::const_iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      const PlaylistBookmark & bookmark = *i;
      if (bookmark.item_ == item)
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
  // MainWindow::cropped
  //
  void
  MainWindow::cropped(const TVideoFramePtr & frame, const TCropFrame & crop)
  {
    (void) frame;
    canvas_->cropFrame(crop);
  }

  //----------------------------------------------------------------
  // MainWindow::dismissFrameCropView
  //
  void
  MainWindow::dismissFrameCropView()
  {
    frameCropView_.setEnabled(false);
    timelineView_.setEnabled(true);
    adjustCanvasHeight();
  }

  //----------------------------------------------------------------
  // MainWindow::event
  //
  bool
  MainWindow::event(QEvent * e)
  {
    QEvent::Type et = e->type();

    if (et == QEvent::User)
    {
      AutoCropEvent * ac = dynamic_cast<AutoCropEvent *>(e);
      if (ac)
      {
        ac->accept();
        canvas_->cropFrame(ac->cropFrame_);
        adjustCanvasHeight();
        return true;
      }

#ifdef __APPLE__
      RemoteControlEvent * rc = dynamic_cast<RemoteControlEvent *>(e);
      if (rc)
      {
#ifndef NDEBUG
        yae_debug
          << "received remote control event(" << rc
          << "), buttonId: " << rc->buttonId_
          << ", down: " << rc->pressedDown_
          << ", clicks: " << rc->clickCount_
          << ", held down: " << rc->heldDown_;
#endif
        rc->accept();

        if (rc->buttonId_ == kRemoteControlPlayButton)
        {
          if (rc->pressedDown_)
          {
            if (rc->heldDown_)
            {
              toggleFullScreen();
            }
            else
            {
              togglePlayback();
            }
          }
        }
        else if (rc->buttonId_ == kRemoteControlMenuButton)
        {
          if (rc->pressedDown_)
          {
            if (rc->heldDown_)
            {
              if (actionCropFrameAutoDetect->isChecked())
              {
                actionCropFrameNone->trigger();
              }
              else
              {
                actionCropFrameAutoDetect->trigger();
              }
            }
            else
            {
              actionShowPlaylist->trigger();
            }
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeUp)
        {
          if (rc->pressedDown_)
          {
            // raise the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume + 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlVolumeDown)
        {
          if (rc->pressedDown_)
          {
            // lower the volume:
            static QStringList args;

            if (args.empty())
            {
              args << "-e" << ("set currentVolume to output "
                               "volume of (get volume settings)")
                   << "-e" << ("set volume output volume "
                               "(currentVolume - 6.25)")
                   << "-e" << ("do shell script \"afplay "
                               "/System/Library/LoginPlugins"
                               "/BezelServices.loginPlugin"
                               "/Contents/Resources/volume.aiff\"");
            }

            QProcess::startDetached("/usr/bin/osascript", args);
          }
        }
        else if (rc->buttonId_ == kRemoteControlLeftButton ||
                 rc->buttonId_ == kRemoteControlRightButton)
        {
          if (rc->pressedDown_)
          {
            double offset =
              (rc->buttonId_ == kRemoteControlLeftButton) ? -3.0 : 7.0;

            timelineModel_.seekFromCurrentTime(offset);
            timelineView_.maybeAnimateOpacity();
          }
        }

        return true;
      }
#endif
    }

    return QMainWindow::event(e);
  }

  //----------------------------------------------------------------
  // MainWindow::wheelEvent
  //
  void
  MainWindow::wheelEvent(QWheelEvent * e)
  {
    double tNow = timelineModel_.currentTime();
    if (tNow <= 1e-1)
    {
      // ignore it:
      return;
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
    if (key == Qt::Key_Escape)
    {
      if (isFullScreen())
      {
        exitFullScreen();
      }
      else if (actionShowPlaylist->isChecked())
      {
        actionShowPlaylist->trigger();
      }
    }
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
    }
    else if (key == Qt::Key_N)
    {
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

    std::size_t numVideoTracks = reader_->getNumberOfVideoTracks();
    std::size_t videoTrackIndex = reader_->getSelectedVideoTrackIndex();

    if (videoTrackIndex >= numVideoTracks)
    {
      return;
    }

    std::size_t numAudioTracks = reader_->getNumberOfAudioTracks();
    std::size_t audioTrackIndex = reader_->getSelectedAudioTrackIndex();
    bool hasAudio = audioTrackIndex < numAudioTracks;

    TIgnoreClockStop ignoreClockStop(timelineModel_);
    IReaderPtr reader = reader_;

    QTime startTime = QTime::currentTime();
    bool done = false;
    while (!done && reader && reader == reader_)
    {
      if (hasAudio && reader_->blockedOnAudio())
      {
        // VFR source (a slide show) may require the audio output
        // queues to be pulled in order to allow the demuxer
        // to push new packets into audio/video queues:

        TTime dt(1001, 60000);
        audioRenderer_->skipForward(dt, reader_.get());
      }

      TTime t;
      done = videoRenderer_->skipToNextFrame(t);

      if (!done)
      {
        if (startTime.elapsed() > 2000)
        {
          // avoid blocking the UI indefinitely:
          break;
        }

        continue;
      }

      if (hasAudio)
      {
        // attempt to nudge the audio reader to the same position:
        audioRenderer_->skipToTime(t, reader_.get());
      }
    }
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
  // MainWindow::mousePressEvent
  //
  void
  MainWindow::mousePressEvent(QMouseEvent * e)
  {
    if (e->button() == Qt::RightButton)
    {
      std::size_t numVideoTracks = reader_->getNumberOfVideoTracks();
      std::size_t numAudioTracks = reader_->getNumberOfAudioTracks();
      std::size_t numSubtitles = reader_->subsCount();
      std::size_t numChapters = reader_->countChapters();

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

      if (playlistModel_.hasItems())
      {
        contextMenu_->addSeparator();
        contextMenu_->addAction(actionSelectAll_);
        contextMenu_->addAction(actionRemove_);
      }

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionLoop);
      contextMenu_->addAction(actionSetInPoint);
      contextMenu_->addAction(actionSetOutPoint);
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
          addMenuCopyTo(contextMenu_, menuAudio);
        }

        if (numVideoTracks)
        {
          addMenuCopyTo(contextMenu_, menuVideo);
        }

        if (numSubtitles || true)
        {
          addMenuCopyTo(contextMenu_, menuSubs);
        }

        if (numChapters)
        {
          addMenuCopyTo(contextMenu_, menuChapters);
        }
      }

      contextMenu_->popup(globalPt);
    }
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

    QRect rectCanvas = playerWidget_->geometry();
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

    QRect rectCanvas = playerWidget_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    // calculate width and height overhead:
    int ox = ww - cw;
    int oy = wh - ch;

    int ideal_w = ox + int(0.5 + vw * xexpand_);
    int ideal_h = oy + int(0.5 + vh * yexpand_);

    QRect rectMax = QApplication::desktop()->availableGeometry(this);
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

    yae_debug << "\ncanvas size set: " << xexpand << ", " << yexpand
              << "\ncanvas resize: " << new_w - cdx << ", " << new_h - cdy
              << "\ncanvas move to: " << new_x << ", " << new_y
              << "\n";
#endif

    resize(new_w - cdx, new_h - cdy);
    // move(new_x, new_y);

    // repaint the frame:
    canvas_->refresh();

#if 0 // FIXME: write me!
    // avoid hiding the highlighted item:
    playlistModel_.makeSureHighlightedItemIsVisible();
#endif
  }

  //----------------------------------------------------------------
  // MainWindow::adjustCanvasHeight
  //
  void
  MainWindow::adjustCanvasHeight()
  {
    bool showPlaylist = actionShowPlaylist->isChecked();
    if (showPlaylist)
    {
      return;
    }

    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader_->getNumberOfVideoTracks();
    if (videoTrack >= numVideoTracks)
    {
      return;
    }

    if (!isFullScreen())
    {
      double w = 1.0;
      double h = 1.0;
      double dar = canvas_->imageAspectRatio(w, h);

      if (dar)
      {
        double s = double(playerWidget_->width()) / w;
        canvasSizeSet(s, s);
      }
    }

    if (frameCropView_.isEnabled())
    {
      playbackCropFrameOther();
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
  MainWindow::prepareReaderAndRenderers(IReader * reader, bool frameStepping)
  {
    videoRenderer_->pause();
    audioRenderer_->pause();

    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t audioTrack = reader->getSelectedAudioTrackIndex();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    SharedClock sharedClock;
    timelineModel_.observe(sharedClock);
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
      timelineModel_.adjustTo(reader);

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
  MainWindow::selectVideoTrack(IReader * reader, std::size_t videoTrackIndex)
  {
    reader->selectVideoTrack(videoTrackIndex);

    VideoTraits vtts;
    if (reader->getVideoTraits(vtts))
    {
#if 0
      // pixel format shortcut:
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

      yae_debug << "yae: native format: "
                << (pts ? ptts->name_.c_str() : "unsupported");
#endif

#if 1
      bool skipColorConverter = actionSkipColorConverter->isChecked();
      canvas_->skipColorConverter(skipColorConverter);

      TPixelFormatId nativeFormat = vtts.pixelFormat_;
      TPixelFormatId outputFormat = kInvalidPixelFormat;

      if (canvas_->
          canvasRenderer()->
          adjustPixelFormatForOpenGL(skipColorConverter, vtts, outputFormat))
      {
        vtts.pixelFormat_ = outputFormat;

        // NOTE: overriding frame size implies scaling, so don't do it
        // unless you really want to scale the images in the reader;
        // In general, leave scaling to OpenGL:
        vtts.encodedWidth_ = 0;
        vtts.encodedHeight_ = 0;
        vtts.pixelAspectRatio_ = 0.0;
      }

      if (nativeFormat == outputFormat)
      {
        // do not force output format, let the decoder do what's best:
        vtts.pixelFormat_ = kInvalidPixelFormat;
      }

      reader->setVideoTraitsOverride(vtts);
#endif
    }

    adjustMenus(reader);
  }

  //----------------------------------------------------------------
  // getProgramName
  //
  static std::string
  getProgramName(const IReader & reader, std::size_t program)
  {
    TProgramInfo info;

    if (reader.getProgramInfo(program, info))
    {
      return get(info.metadata_, std::string("service_name"));
    }

    return std::string();
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
                                std::vector<TTrackInfo> &  subsInfo,
                                std::vector<TSubsFormat> & subsFormat)
  {
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t subsCount = reader->subsCount();

    if (audioTrackGroup_)
    {
      // remove old actions:
      QList<QAction *> actions = audioTrackGroup_->actions();
      while (!actions.empty())
      {
        QAction * action = actions.front();
        actions.pop_front();

        menuAudio->removeAction(action);
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

        menuVideo->removeAction(action);
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

        menuSubs->removeAction(action);
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

        menuChapters->removeAction(action);
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
          arg(int(traits.channelLayout_));
      }

      std::string serviceName = getProgramName(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuAudio->addAction(trackAction);

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
      menuAudio->addAction(trackAction);

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

      std::string serviceName = getProgramName(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuVideo->addAction(trackAction);

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
      menuVideo->addAction(trackAction);

      trackAction->setCheckable(true);
      videoTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, int(numVideoTracks));
    }

    subsInfo = std::vector<TTrackInfo>(subsCount);
    subsFormat = std::vector<TSubsFormat>(subsCount);

    for (unsigned int i = 0; i < subsCount; i++)
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

      std::string serviceName = getProgramName(*reader, info.program_);
      if (serviceName.size())
      {
        trackName += tr(", %1").arg(QString::fromUtf8(serviceName.c_str()));
      }
      else if (info.nprograms_ > 1)
      {
        trackName += tr(", program %1").arg(info.program_);
      }

      QAction * trackAction = new QAction(trackName, this);
      menuSubs->addAction(trackAction);

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
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(subsCount + i + 1));
    }

    // add an option to disable subs:
    {
      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(subsCount));
    }

    // update the chapter menu:
    std::size_t numChapters = reader->countChapters();
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
      menuChapters->addAction(chapterAction);

      chapterAction->setCheckable(true);
      chaptersGroup_->addAction(chapterAction);

      ok = connect(chapterAction, SIGNAL(triggered()),
                   chapterMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      chapterMapper_->setMapping(chapterAction, (int)i);
    }

    bool isSeekable = reader->isSeekable();
    actionSetInPoint->setEnabled(isSeekable);
    actionSetOutPoint->setEnabled(isSeekable);
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
    std::size_t numChapters = reader->countChapters();
    std::size_t videoTrackIndex = reader->getSelectedVideoTrackIndex();

    if (!numVideoTracks && numAudioTracks)
    {
      menubar->removeAction(menuVideo->menuAction());
    }

    if (!numVideoTracks)
    {
      menubar->removeAction(menuSubs->menuAction());
    }

    if (!numChapters)
    {
      menubar->removeAction(menuChapters->menuAction());
    }

    if (numVideoTracks || !numAudioTracks)
    {
      menubar->removeAction(menuVideo->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuVideo);
    }

    if (numVideoTracks || !(numVideoTracks || numAudioTracks))
    {
      menubar->removeAction(menuSubs->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuSubs);
    }

    if (numChapters)
    {
      menubar->removeAction(menuChapters->menuAction());
      menubar->insertMenu(menuHelp->menuAction(), menuChapters);
    }
    else
    {
      menubar->removeAction(menuChapters->menuAction());
    }

    if (canvas_)
    {
      VideoTraits vtts;
      bool gotVideoTraits = reader && reader->getVideoTraits(vtts);

      if ((videoTrackIndex >= numVideoTracks && numAudioTracks > 0) ||
          // audio files with embeded album art poster frame
          // typically show up with ridiculous frame rate,
          // so I'll consider that as an audio file trait:
          (gotVideoTraits && vtts.frameRate_ > 240.0))
      {
        playlistView_.setStyleId(PlaylistView::kListView);
      }
      else if (numVideoTracks)
      {
        playlistView_.setStyleId(PlaylistView::kGridView);
      }
    }
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
    reader->setRenderCaptions(uint32_t(cc));

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
      if (getNumberOfChannels(native.channelLayout_) > 2 &&
          actionDownmixToStereo->isChecked())
      {
        native.channelLayout_ = kAudioStereo;
      }

      AudioTraits supported;
      audioRenderer_->match(native, supported);

#if 0
      yae_debug << "\nsupported: " << supported.channelLayout_
                << "\nrequired:  " << native.channelLayout_
                << "\n";
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
    IReaderPtr reader = mainWindow->reader_;
    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();

    if (detectionFinished)
    {
      qApp->postEvent(mainWindow,
                      new AutoCropEvent(cf),
                      Qt::HighEventPriority);
    }
    else if (mainWindow->playbackPaused_ ||
             mainWindow->videoRenderer_->isPaused() ||
             videoTrack >= numVideoTracks)
    {
      // use the same frame again:
      return mainWindow->canvas_->currentFrame();
    }

    return TVideoFramePtr();
  }

#ifdef __APPLE__
  //----------------------------------------------------------------
  // appleRemoteControlObserver
  //
  void
  MainWindow::appleRemoteControlObserver(void * observerContext,
                                         TRemoteControlButtonId buttonId,
                                         bool pressedDown,
                                         unsigned int clickCount,
                                         bool heldDown)
  {
    MainWindow * mainWindow = (MainWindow *)observerContext;

    boost::interprocess::unique_ptr<RemoteControlEvent>
      rc(new RemoteControlEvent(buttonId,
                                pressedDown,
                                clickCount,
                                heldDown));
#ifndef NDEBUG
    yae_debug
      << "posting remote control event(" << rc.get()
      << "), buttonId: " << buttonId
      << ", down: " << pressedDown
      << ", clicks: " << clickCount
      << ", held down: " << heldDown;
#endif

    qApp->postEvent(mainWindow->playerWidget_,
                    rc.release(),
                    Qt::HighEventPriority);
  }
#endif
};
