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

// GLEW includes:
#include <GL/glew.h>

// Qt includes:
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
#include <QDesktopWidget>
#include <QMenu>
#include <QShortcut>
#include <QFileInfo>
#include <QProcess>
#include <QDesktopServices>
#include <QDirIterator>

// yae includes:
#include <yaeReaderFFMPEG.h>
#include <yaePixelFormats.h>
#include <yaePixelFormatTraits.h>
#include <yaeAudioRendererPortaudio.h>
#include <yaeVideoRenderer.h>
#include <yaeVersion.h>
#include <yaeUtils.h>

// local includes:
#include <yaeMainWindow.h>


namespace yae
{

  //----------------------------------------------------------------
  // kResumePlaybackFromBookmark
  //
  static const QString kResumePlaybackFromBookmark =
    QString::fromUtf8("ResumePlaybackFromBookmark");

  //----------------------------------------------------------------
  // kSettingTrue
  //
  static const QString kSettingTrue = QString::fromUtf8("true");

  //----------------------------------------------------------------
  // kSettingFalse
  //
  static const QString kSettingFalse = QString::fromUtf8("false");

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
  AboutDialog::AboutDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::AboutDialog()
  {
    Ui::AboutDialog::setupUi(this);
  }

  //----------------------------------------------------------------
  // AspectRatioDialog::AspectRatioDialog
  //
  AspectRatioDialog::AspectRatioDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
    Ui::AspectRatioDialog()
  {
    Ui::AspectRatioDialog::setupUi(this);
  }

  //----------------------------------------------------------------
  // OpenUrlDialog::OpenUrlDialog
  //
  OpenUrlDialog::OpenUrlDialog(QWidget * parent, Qt::WFlags f):
    QDialog(parent, f),
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
  // SignalBlocker
  //
  struct SignalBlocker
  {
    SignalBlocker(QObject * qObj = NULL)
    {
      *this << qObj;
    }

    ~SignalBlocker()
    {
      while (!blocked_.empty())
      {
        QObject * qObj = blocked_.front();
        blocked_.pop_front();

        qObj->blockSignals(false);
      }
    }

    SignalBlocker & operator << (QObject * qObj)
    {
      if (qObj && !qObj->signalsBlocked())
      {
        qObj->blockSignals(true);
        blocked_.push_back(qObj);
      }

      return *this;
    }

    std::list<QObject *> blocked_;
  };

  //----------------------------------------------------------------
  // AutoCropEvent
  //
  struct AutoCropEvent : public QEvent
  {
    AutoCropEvent(const TCropFrame & cropFrame):
      QEvent(QEvent::User),
      cropFrame_(cropFrame)
    {}

    TCropFrame cropFrame_;
  };

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
  // MainWindow::MainWindow
  //
  MainWindow::MainWindow():
    QMainWindow(NULL, 0),
    audioDeviceGroup_(NULL),
    audioDeviceMapper_(NULL),
    audioTrackGroup_(NULL),
    videoTrackGroup_(NULL),
    subsTrackGroup_(NULL),
    audioTrackMapper_(NULL),
    videoTrackMapper_(NULL),
    subsTrackMapper_(NULL),
    chapterMapper_(NULL),
    bookmarksGroup_(NULL),
    bookmarksMapper_(NULL),
    bookmarksMenuSeparator_(NULL),
    reader_(NULL),
    readerId_(0),
    canvas_(NULL),
    audioRenderer_(NULL),
    videoRenderer_(NULL),
    playbackPaused_(false),
    scrollStart_(0.0),
    scrollOffset_(0.0),
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
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(playlistWidget_);
    actionPlay->setText(tr("Pause"));

    contextMenu_ = new QMenu(this);
    contextMenu_->setObjectName(QString::fromUtf8("contextMenu_"));

#if !defined(__APPLE__) && !defined(_WIN32)
    QString fnIcon = QString::fromUtf8(":/images/apprenticevideo-64.png");
    this->setWindowIcon(QIcon(fnIcon));
#endif

    timelineControls_->setAuxWidgets(lineEditPlayhead_,
                                     lineEditDuration_,
                                     this);

    QVBoxLayout * canvasLayout = new QVBoxLayout(canvasContainer_);
    canvasLayout->setMargin(0);
    canvasLayout->setSpacing(0);

    // request vsync if available:
    QGLFormat contextFormat;
    contextFormat.setSwapInterval(1);

    canvas_ = new Canvas(contextFormat);
    canvasLayout->addWidget(canvas_);

    reader_ = ReaderFFMPEG::create();
    audioRenderer_ = AudioRendererPortaudio::create();
    videoRenderer_ = VideoRenderer::create();

    // show the timeline:
    actionShowTimeline->setChecked(true);
    timelineWidgets_->show();

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

    // for scroll-wheel event buffering,
    // used to avoid frequent seeking by small distances:
    scrollWheelTimer_.setSingleShot(true);

    // for re-running auto-crop soon after loading next file in the playlist:
    autocropTimer_.setSingleShot(true);

    // update a bookmark every 3 minutes during playback:
    bookmarkTimer_.setInterval(180000);

    bookmarksMenuSeparator_ =
      menuBookmarks->insertSeparator(actionRemoveBookmarks);

    QString resumeFromBookmark =
      loadSettingOrDefault(kResumePlaybackFromBookmark, kSettingTrue);

    if (resumeFromBookmark == kSettingTrue)
    {
      actionResumeFromBookmark->setChecked(true);
    }

    // when in fullscreen mode the menubar is hidden and all actions
    // associated with it stop working (tested on OpenSUSE 11.4 KDE 4.6),
    // so I am creating these shortcuts as a workaround:
    shortcutExit_ = new QShortcut(this);
    shortcutFullScreen_ = new QShortcut(this);
    shortcutShowPlaylist_ = new QShortcut(this);
    shortcutShowTimeline_ = new QShortcut(this);
    shortcutPlay_ = new QShortcut(this);
    shortcutNext_ = new QShortcut(this);
    shortcutPrev_ = new QShortcut(this);
    shortcutLoop_ = new QShortcut(this);
    shortcutCropNone_ = new QShortcut(this);
    shortcutCrop1_33_ = new QShortcut(this);
    shortcutCrop1_78_ = new QShortcut(this);
    shortcutAutoCrop_ = new QShortcut(this);
    shortcutRemove_ = new QShortcut(this);
    shortcutSelectAll_ = new QShortcut(this);

    shortcutExit_->setContext(Qt::ApplicationShortcut);
    shortcutFullScreen_->setContext(Qt::ApplicationShortcut);
    shortcutShowPlaylist_->setContext(Qt::ApplicationShortcut);
    shortcutShowTimeline_->setContext(Qt::ApplicationShortcut);
    shortcutPlay_->setContext(Qt::ApplicationShortcut);
    shortcutNext_->setContext(Qt::ApplicationShortcut);
    shortcutPrev_->setContext(Qt::ApplicationShortcut);
    shortcutLoop_->setContext(Qt::ApplicationShortcut);
    shortcutCropNone_->setContext(Qt::ApplicationShortcut);
    shortcutCrop1_33_->setContext(Qt::ApplicationShortcut);
    shortcutCrop1_78_->setContext(Qt::ApplicationShortcut);
    shortcutAutoCrop_->setContext(Qt::ApplicationShortcut);

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

    QSignalMapper * playRateMapper = new QSignalMapper(this);
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
    ok = connect(playRateMapper, SIGNAL(mapped(int)),
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

    ok = connect(actionAspectRatio1_33, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_33()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_60, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_60()));
    YAE_ASSERT(ok);

    ok = connect(actionAspectRatio1_78, SIGNAL(triggered()),
                 this, SLOT(playbackAspectRatio1_78()));
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

    ok = connect(actionCropFrame2_35, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_35()));
    YAE_ASSERT(ok);

    ok = connect(actionCropFrame2_40, SIGNAL(triggered()),
                 this, SLOT(playbackCropFrame2_40()));
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

    ok = connect(actionFullScreen, SIGNAL(triggered()),
                 this, SLOT(playbackFullScreen()));
    YAE_ASSERT(ok);

    ok = connect(shortcutFullScreen_, SIGNAL(activated()),
                 actionFullScreen, SLOT(trigger()));
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

    ok = connect(canvas_, SIGNAL(toggleFullScreen()),
                 this, SLOT(playbackFullScreen()));
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

    ok = connect(actionAbout, SIGNAL(triggered()),
                 this, SLOT(helpAbout()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setInPoint()),
                 timelineControls_, SLOT(setInPoint()));
    YAE_ASSERT(ok);

    ok = connect(this, SIGNAL(setOutPoint()),
                 timelineControls_, SLOT(setOutPoint()));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(userIsSeeking(bool)),
                 this, SLOT(userIsSeeking(bool)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(moveTimeIn(double)),
                 this, SLOT(moveTimeIn(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(moveTimeOut(double)),
                 this, SLOT(moveTimeOut(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(movePlayHead(double)),
                 this, SLOT(movePlayHead(double)));
    YAE_ASSERT(ok);

    ok = connect(timelineControls_, SIGNAL(clockStopped()),
                 this, SLOT(playbackFinished()));
    YAE_ASSERT(ok);

    ok = connect(menuAudioDevice, SIGNAL(aboutToShow()),
                 this, SLOT(populateAudioDeviceMenu()));
    YAE_ASSERT(ok);

    ok = connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                 this, SLOT(focusChanged(QWidget *, QWidget *)));
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

    ok = connect(&autocropTimer_, SIGNAL(timeout()),
                 this, SLOT(playbackCropFrameAutoDetect()));
    YAE_ASSERT(ok);

    ok = connect(&bookmarkTimer_, SIGNAL(timeout()),
                 this, SLOT(saveBookmark()));
    YAE_ASSERT(ok);

    ok = connect(menuBookmarks, SIGNAL(aboutToShow()),
                 this, SLOT(bookmarksPopulate()));
    YAE_ASSERT(ok);

    ok = connect(actionRemoveBookmarks, SIGNAL(triggered()),
                 this, SLOT(bookmarksRemove()));
    YAE_ASSERT(ok);

    ok = connect(actionResumeFromBookmark, SIGNAL(triggered()),
                 this, SLOT(bookmarksResumePlayback()));
    YAE_ASSERT(ok);

    // initialize the subtitles menu:
    {
      subsTrackGroup_ = new QActionGroup(this);
      subsTrackMapper_ = new QSignalMapper(this);
      ok = connect(subsTrackMapper_, SIGNAL(mapped(int)),
                   this, SLOT(subsSelectTrack(int)));
      YAE_ASSERT(ok);

      QAction * trackAction = new QAction(tr("Disabled"), this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      trackAction->setChecked(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(0));
    }

    menubar->removeAction(menuChapters->menuAction());
  }

  //----------------------------------------------------------------
  // MainWindow::~MainWindow
  //
  MainWindow::~MainWindow()
  {
    delete openUrl_;
    openUrl_ = NULL;

    audioRenderer_->close();
    audioRenderer_->destroy();

    videoRenderer_->close();
    videoRenderer_->destroy();

    reader_->destroy();
    canvas_->cropAutoDetectStop();
    delete canvas_;
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
  // MainWindow::setPlaylist
  //
  void
  MainWindow::setPlaylist(const std::list<QString> & playlist,
                          bool beginPlaybackImmediately)
  {
    SignalBlocker blockSignals(playlistWidget_);

    bool resumeFromBookmark =
      actionAutomaticBookmarks->isChecked() &&
      actionResumeFromBookmark->isChecked();

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
      bool found = false;

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
            found = true;
          }
        }
      }

      if (found && playlistWidget_->lookup(bookmark.groupHash_,
                                           bookmark.itemHash_,
                                           &bookmark.itemIndex_))
      {
        gotoBookmark(bookmark);
        return;
      }
    }

    playback();
  }

  //----------------------------------------------------------------
  // getExtIgnoreList
  //
  static QStringList getExtIgnoreList()
  {
    QStringList ext;
    ext
      << QString::fromUtf8("eyetvsched")
      << QString::fromUtf8("eyetvp")
      << QString::fromUtf8("eyetvr")
      << QString::fromUtf8("eyetvi")
      << QString::fromUtf8("pages")
      << QString::fromUtf8("doc")
      << QString::fromUtf8("xls")
      << QString::fromUtf8("ppt")
      << QString::fromUtf8("pdf")
      << QString::fromUtf8("rtf")
      << QString::fromUtf8("htm")
      << QString::fromUtf8("css")
      << QString::fromUtf8("less")
      << QString::fromUtf8("rar")
      << QString::fromUtf8("jar")
      << QString::fromUtf8("zip")
      << QString::fromUtf8("7z")
      << QString::fromUtf8("gz")
      << QString::fromUtf8("bz2")
      << QString::fromUtf8("war")
      << QString::fromUtf8("tar")
      << QString::fromUtf8("tgz")
      << QString::fromUtf8("tbz2")
      << QString::fromUtf8("lzma")
      << QString::fromUtf8("url")
      << QString::fromUtf8("eml")
      << QString::fromUtf8("html")
      << QString::fromUtf8("xml")
      << QString::fromUtf8("dtd")
      << QString::fromUtf8("tdt")
      << QString::fromUtf8("stg")
      << QString::fromUtf8("bat")
      << QString::fromUtf8("ini")
      << QString::fromUtf8("cfg")
      << QString::fromUtf8("cnf")
      << QString::fromUtf8("csv")
      << QString::fromUtf8("rdp")
      << QString::fromUtf8("el")
      << QString::fromUtf8("rb")
      << QString::fromUtf8("cs")
      << QString::fromUtf8("java")
      << QString::fromUtf8("php")
      << QString::fromUtf8("js")
      << QString::fromUtf8("pl")
      << QString::fromUtf8("db")
      << QString::fromUtf8("tex")
      << QString::fromUtf8("txt")
      << QString::fromUtf8("text")
      << QString::fromUtf8("srt")
      << QString::fromUtf8("ass")
      << QString::fromUtf8("ssa")
      << QString::fromUtf8("idx")
      << QString::fromUtf8("sub")
      << QString::fromUtf8("ifo")
      << QString::fromUtf8("info")
      << QString::fromUtf8("nfo")
      << QString::fromUtf8("inf")
      << QString::fromUtf8("md5")
      << QString::fromUtf8("crc")
      << QString::fromUtf8("sfv")
      << QString::fromUtf8("m3u")
      << QString::fromUtf8("smil")
      << QString::fromUtf8("app")
      << QString::fromUtf8("strings")
      << QString::fromUtf8("plist")
      << QString::fromUtf8("framework")
      << QString::fromUtf8("bundle")
      << QString::fromUtf8("rcproject")
      << QString::fromUtf8("ipmeta")
      << QString::fromUtf8("qtx")
      << QString::fromUtf8("qtr")
      << QString::fromUtf8("sc")
      << QString::fromUtf8("so")
      << QString::fromUtf8("dylib")
      << QString::fromUtf8("dll")
      << QString::fromUtf8("ax")
      << QString::fromUtf8("def")
      << QString::fromUtf8("lib")
      << QString::fromUtf8("a")
      << QString::fromUtf8("r")
      << QString::fromUtf8("t")
      << QString::fromUtf8("y")
      << QString::fromUtf8("o")
      << QString::fromUtf8("obj")
      << QString::fromUtf8("am")
      << QString::fromUtf8("in")
      << QString::fromUtf8("exe")
      << QString::fromUtf8("com")
      << QString::fromUtf8("cmd")
      << QString::fromUtf8("cab")
      << QString::fromUtf8("dat")
      << QString::fromUtf8("bat")
      << QString::fromUtf8("sys")
      << QString::fromUtf8("msi")
      << QString::fromUtf8("iss")
      << QString::fromUtf8("ism")
      << QString::fromUtf8("rul")
      << QString::fromUtf8("py")
      << QString::fromUtf8("sh")
      << QString::fromUtf8("m4")
      << QString::fromUtf8("cpp")
      << QString::fromUtf8("hpp")
      << QString::fromUtf8("tpp")
      << QString::fromUtf8("ipp")
      << QString::fromUtf8("SUNWCCh")
      << QString::fromUtf8("inc")
      << QString::fromUtf8("pch")
      << QString::fromUtf8("sed")
      << QString::fromUtf8("awk")
      << QString::fromUtf8("h")
      << QString::fromUtf8("hh")
      << QString::fromUtf8("m")
      << QString::fromUtf8("mm")
      << QString::fromUtf8("c")
      << QString::fromUtf8("cc")
      << QString::fromUtf8("ui")
      << QString::fromUtf8("as")
      << QString::fromUtf8("asm")
      << QString::fromUtf8("rc")
      << QString::fromUtf8("qrc")
      << QString::fromUtf8("cxx")
      << QString::fromUtf8("hxx")
      << QString::fromUtf8("txx")
      << QString::fromUtf8("log")
      << QString::fromUtf8("err")
      << QString::fromUtf8("out")
      << QString::fromUtf8("sqz")
      << QString::fromUtf8("xss")
      << QString::fromUtf8("xds")
      << QString::fromUtf8("xsp")
      << QString::fromUtf8("xcp")
      << QString::fromUtf8("xfs")
      << QString::fromUtf8("spfx")
      << QString::fromUtf8("iso")
      << QString::fromUtf8("dmg")
      << QString::fromUtf8("dmp")
      << QString::fromUtf8("svq")
      << QString::fromUtf8("svn")
      << QString::fromUtf8("itdb")
      << QString::fromUtf8("itl")
      << QString::fromUtf8("itc")
      << QString::fromUtf8("ipa")
      << QString::fromUtf8("vbox")
      << QString::fromUtf8("vdi")
      << QString::fromUtf8("vmdk")
      << QString::fromUtf8("sln")
      << QString::fromUtf8("suo")
      << QString::fromUtf8("manifest")
      << QString::fromUtf8("vcproj")
      << QString::fromUtf8("csproj")
      << QString::fromUtf8("mode1v3")
      << QString::fromUtf8("pbxuser")
      << QString::fromUtf8("pbxproj")
      << QString::fromUtf8("pmproj")
      << QString::fromUtf8("proj")
      << QString::fromUtf8("rsrc")
      << QString::fromUtf8("nib")
      << QString::fromUtf8("icns")
      << QString::fromUtf8("cw")
      << QString::fromUtf8("amz")
      << QString::fromUtf8("mcp")
      << QString::fromUtf8("pro")
      << QString::fromUtf8("mk")
      << QString::fromUtf8("mak")
      << QString::fromUtf8("cmake")
      << QString::fromUtf8("dxy")
      << QString::fromUtf8("dox")
      << QString::fromUtf8("doxy")
      << QString::fromUtf8("dsp")
      << QString::fromUtf8("dsw")
      << QString::fromUtf8("plg")
      << QString::fromUtf8("lst")
      << QString::fromUtf8("asx")
      << QString::fromUtf8("otf")
      << QString::fromUtf8("ttf")
      << QString::fromUtf8("fon")
      << QString::fromUtf8("license")
      << QString::fromUtf8("ignore");

    return ext;
  }

  //----------------------------------------------------------------
  // kExtIgnoreList
  //
  static QStringList kExtIgnoreList = getExtIgnoreList();

  //----------------------------------------------------------------
  // shouldIgnore
  //
  static bool shouldIgnore(const QString & ext)
  {
    QString extLowered = ext.toLower();
    return
      extLowered.isEmpty() ||
      extLowered.endsWith("~") ||
      kExtIgnoreList.contains(extLowered);
  }

  //----------------------------------------------------------------
  // shouldIgnore
  //
  static bool shouldIgnore(const QString & ext, QFileInfo & fi)
  {
    if (fi.isDir())
    {
      QString extLowered = ext.toLower();
      if (extLowered == QString::fromUtf8("eyetvsched"))
      {
        return true;
      }

      return false;
    }

    return shouldIgnore(ext);
  }

  //----------------------------------------------------------------
  // findFiles
  //
  static void
  findFiles(std::list<QString> & files,
            const QString & startHere,
            bool recursive = true)
  {
    QStringList extFilters;
    if (QFileInfo(startHere).suffix() == kExtEyetv)
    {
      extFilters << QString::fromUtf8("*.mpg");
    }

    QDirIterator iter(startHere,
                      extFilters,
                      QDir::NoDotAndDotDot |
                      QDir::AllEntries |
                      QDir::Readable,
                      QDirIterator::FollowSymlinks);

    while (iter.hasNext())
    {
      iter.next();

      QFileInfo fi = iter.fileInfo();
      QString fn = fi.absoluteFilePath();
      QString ext = fi.suffix();
      // std::cerr << "FN: " << fn.toUtf8().constData() << std::endl;

      if (!shouldIgnore(ext, fi))
      {
        if (fi.isDir() && ext != kExtEyetv)
        {
          if (recursive)
          {
            findFiles(files, fn, recursive);
          }
        }
        else
        {
          files.push_back(fn);
        }
      }
    }
  }

  //----------------------------------------------------------------
  // findFilesAndSort
  //
  static void
  findFilesAndSort(std::list<QString> & files,
                   const QString & startHere,
                   bool recursive = true)
  {
    findFiles(files, startHere, recursive);
    files.sort();
  }

  //----------------------------------------------------------------
  // findMatchingTrack
  //
  template <typename TTraits>
  static std::size_t
  findMatchingTrack(const std::vector<TTrackInfo> & trackInfo,
                    const std::vector<TTraits> & trackTraits,
                    const TTrackInfo & selInfo,
                    const TTraits & selTraits)
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
      return info.index_;
    }

    // try to match track index:
    if (trackInfo.size() == selInfo.ntracks_)
    {
      return selInfo.index_;
    }

    // default to first track:
    return 0;
  }

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

    // convert to NFD normal form (Normalization Form Canonical Decomposition):
    fn = fn.normalized(QString::NormalizationForm_D);
    std::string filename(fn.toUtf8().constData());

    actionPlay->setEnabled(false);

    ReaderFFMPEG * reader = ReaderFFMPEG::create();
    if (!reader->open(filename.c_str()))
    {
#if 0
      std::cerr << "ERROR: could not open movie: " << filename << std::endl;
#endif
      return false;
    }

    ++readerId_;
    reader->setReaderId(readerId_);

    // prevent Canvas from rendering any pending frames from previous reader:
    canvas_->acceptFramesWithReaderId(readerId_);

    // disconnect timeline from renderers:
    timelineControls_->observe(SharedClock());

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t subsCount = reader->subsCount();

    reader->threadStop();
    reader->setPlaybackInterval(true);

#if 0
    std::cerr << std::endl
              << "yae: " << filename << std::endl
              << "yae: video tracks: " << numVideoTracks << std::endl
              << "yae: audio tracks: " << numAudioTracks << std::endl
              << "yae: subs tracks: " << subsCount << std::endl;
#endif

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

    // update the UI:
    delete audioTrackGroup_;
    audioTrackGroup_ = new QActionGroup(this);

    delete videoTrackGroup_;
    videoTrackGroup_ = new QActionGroup(this);

    delete subsTrackGroup_;
    subsTrackGroup_ = new QActionGroup(this);

    delete audioTrackMapper_;
    audioTrackMapper_ = new QSignalMapper(this);

    bool ok = connect(audioTrackMapper_, SIGNAL(mapped(int)),
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

    std::vector<TTrackInfo> audioInfo(numAudioTracks);
    std::vector<AudioTraits> audioTraits(numAudioTracks);
    std::vector<QAction *> audioTrackAction(numAudioTracks + 1);

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

      QAction *& trackAction = audioTrackAction[i];
      trackAction = new QAction(trackName, this);
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
      QAction *& trackAction = audioTrackAction[numAudioTracks];
      trackAction = new QAction(tr("Disabled"), this);
      menuAudio->addAction(trackAction);

      trackAction->setCheckable(true);
      audioTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   audioTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioTrackMapper_->setMapping(trackAction, int(numAudioTracks));
    }

    std::vector<TTrackInfo> videoInfo(numVideoTracks);
    std::vector<VideoTraits> videoTraits(numVideoTracks);
    std::vector<QAction *> videoTrackAction(numVideoTracks + 1);

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
        trackName +=
          tr(", %1 x %2, %3 fps").
          arg(traits.encodedWidth_).
          arg(traits.encodedHeight_).
          arg(traits.frameRate_);
      }

      QAction *& trackAction = videoTrackAction[i];
      trackAction = new QAction(trackName, this);
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
      QAction *& trackAction = videoTrackAction[numVideoTracks];
      trackAction = new QAction(tr("Disabled"), this);
      menuVideo->addAction(trackAction);

      trackAction->setCheckable(true);
      videoTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   videoTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      videoTrackMapper_->setMapping(trackAction, int(numVideoTracks));
    }

    std::vector<TTrackInfo> subsInfo(subsCount);
    std::vector<TSubsFormat> subsFormat(subsCount);
    std::vector<QAction *> subsTrackAction(subsCount + 1);

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

      QAction *& trackAction = subsTrackAction[i];
      trackAction = new QAction(trackName, this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, i);
    }

    // add an option to disable subs:
    {
      QAction *& trackAction = subsTrackAction[subsCount];
      trackAction = new QAction(tr("Disabled"), this);
      menuSubs->addAction(trackAction);

      trackAction->setCheckable(true);
      subsTrackGroup_->addAction(trackAction);

      ok = connect(trackAction, SIGNAL(triggered()),
                   subsTrackMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      subsTrackMapper_->setMapping(trackAction, int(subsCount));
    }

    std::size_t vtrack = findMatchingTrack<VideoTraits>(videoInfo,
                                                        videoTraits,
                                                        selVideo_,
                                                        selVideoTraits_);
    if (bookmark && bookmark->vtrack_ < numVideoTracks)
    {
      vtrack = bookmark->vtrack_;
    }

    selectVideoTrack(reader, vtrack);
    videoTrackAction[vtrack]->setChecked(true);

    std::size_t atrack = findMatchingTrack<AudioTraits>(audioInfo,
                                                        audioTraits,
                                                        selAudio_,
                                                        selAudioTraits_);
    if (bookmark && bookmark->atrack_ < numAudioTracks)
    {
      atrack = bookmark->atrack_;
    }

    selectAudioTrack(reader, atrack);
    audioTrackAction[atrack]->setChecked(true);

    std::size_t strack = findMatchingTrack<TSubsFormat>(subsInfo,
                                                        subsFormat,
                                                        selSubs_,
                                                        selSubsFormat_);
    if (bookmark)
    {
      if (!bookmark->subs_.empty() &&
          bookmark->subs_.front() < subsCount)
      {
        strack = bookmark->subs_.front();
      }
      else if (bookmark->subs_.empty())
      {
        strack = subsCount;
      }
    }

    selectSubsTrack(reader, strack);
    subsTrackAction[strack]->setChecked(true);


    // update the chapter menu:
    delete chapterMapper_;
    chapterMapper_ = new QSignalMapper(this);

    ok = connect(chapterMapper_, SIGNAL(mapped(int)),
                 this, SLOT(skipToChapter(int)));
    YAE_ASSERT(ok);

    std::size_t numChapters = reader->countChapters();
    menuChapters->clear();
    for (std::size_t i = 0; i < numChapters; i++)
    {
      TChapter ch;
      ok = reader->getChapterInfo(i, ch);
      YAE_ASSERT(ok);

      QTime t0 = QTime(0, 0).addMSecs((int)(0.5 + ch.start_ * 1000.0));

      QString name =
        tr("%1   %2").
        arg(t0.toString("hh:mm:ss")).
        arg(QString::fromUtf8(ch.name_.c_str()));

      QAction * chapterAction = new QAction(name, this);
      menuChapters->addAction(chapterAction);

      ok = connect(chapterAction, SIGNAL(triggered()),
                   chapterMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      chapterMapper_->setMapping(chapterAction, (int)i);
    }

    adjustMenues(reader);

    reader_->close();
    stopRenderers();

    // reset overlay plane to clean state, reset libass wrapper:
    canvas_->clearOverlay();

    // reset timeline start, duration, playhead, in/out points:
    timelineControls_->resetFor(reader);

    if (bookmark)
    {
      // skip to bookmarked position:
      reader->seek(bookmark->positionInSeconds_);
    }

    // renderers have to be started before the reader, because they
    // may need to specify reader output format override, which is
    // too late if the reader already started the decoding loops;
    // renderers are started paused, so after the reader is started
    // the rendrers have to be resumed:
    prepareReaderAndRenderers(reader, playbackPaused_);

    // this opens the output frame queues for renderers
    // and starts the decoding loops:
    reader->threadStart();

    // allow renderers to read from output frame queues:
    resumeRenderers();

    // replace the previous reader:
    reader_->destroy();
    reader_ = reader;

    this->setWindowTitle(tr("Apprentice Video: %1").
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

    return true;
  }

  //----------------------------------------------------------------
  // MainWindow::fileOpenFolder
  //
  void
  MainWindow::fileOpenFolder()
  {
    QString startHere =
      QDesktopServices::storageLocation(QDesktopServices::MoviesLocation
                                        // QDesktopServices::HomeLocation
                                        );

    QString folder =
      QFileDialog::getExistingDirectory(this,
                                        tr("Select a video folder"),
                                        startHere,
                                        QFileDialog::ShowDirsOnly |
                                        QFileDialog::DontResolveSymlinks);
    if (folder.isEmpty())
    {
      return;
    }

    // find all files in the folder, sorted alphabetically
    std::list<QString> playlist;

    QFileInfo fi(folder);
    QString ext = fi.suffix();

    if (!shouldIgnore(ext, fi))
    {
      if (fi.isDir() && ext != kExtEyetv)
      {
        findFilesAndSort(playlist, folder, true);
      }
      else
      {
        playlist.push_back(folder);
      }
    }

    setPlaylist(playlist);
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

    QString startHere =
      QDesktopServices::storageLocation(QDesktopServices::MoviesLocation
                                        // QDesktopServices::HomeLocation
                                        );
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
      QString filename = *i;
      QFileInfo fi(filename);

      if (!fi.isReadable())
      {
        continue;
      }

      if (fi.isDir() && fi.suffix() != kExtEyetv)
      {
        findFilesAndSort(playlist, filename);
      }
      else
      {
        playlist.push_back(filename);
      }
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
    MainWindow::close();
    qApp->quit();
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
    actionRemoveBookmarks->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;
    }

    bookmarks_.clear();

    if (!actionAutomaticBookmarks->isChecked())
    {
      return;
    }

    std::size_t itemIndex = 0;
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
          escapeAmpersand(item.name_) +
          QString::fromUtf8(", ") +
          QString::fromUtf8(ts.c_str());

        bookmark.action_ = new QAction(name, this);
        menuBookmarks->insertAction(bookmarksMenuSeparator_, bookmark.action_);
        bookmarksGroup_->addAction(bookmark.action_);

        bool ok = connect(bookmark.action_, SIGNAL(triggered()),
                          bookmarksMapper_, SLOT(map()));
        YAE_ASSERT(ok);

        bookmarksMapper_->setMapping(bookmark.action_,
                                     bookmarks_.size());

        bookmarks_.push_back(bookmark);
      }
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
    actionRemoveBookmarks->setEnabled(false);

    for (std::vector<PlaylistBookmark>::iterator i = bookmarks_.begin();
         i != bookmarks_.end(); ++i)
    {
      PlaylistBookmark & bookmark = *i;
      delete bookmark.action_;

      removeBookmark(bookmark.groupHash_);
    }

    bookmarks_.clear();
  }

  //----------------------------------------------------------------
  // MainWindow::bookmarksResumePlayback
  //
  void
  MainWindow::bookmarksResumePlayback()
  {
    if (actionResumeFromBookmark->isChecked())
    {
      saveSetting(kResumePlaybackFromBookmark, kSettingTrue);
    }
    else
    {
      saveSetting(kResumePlaybackFromBookmark, kSettingFalse);
    }
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
  // MainWindow::playbackCropFrameAutoDetect
  //
  void
  MainWindow::playbackCropFrameAutoDetect()
  {
    canvas_->cropAutoDetect(this, &(MainWindow::autoCropCallback));
  }

  //----------------------------------------------------------------
  // TIgnoreClockStop
  //
  struct TIgnoreClockStop
  {
    TIgnoreClockStop(TimelineControls * tc):
      tc_(tc)
    {
      count_++;
      YAE_ASSERT(count_ < 2);

      if (count_ < 2)
      {
        tc_->ignoreClockStoppedEvent(true);
      }
    }

    ~TIgnoreClockStop()
    {
      count_--;
      if (!count_)
      {
        tc_->ignoreClockStoppedEvent(false);
      }
    }

  private:
    TIgnoreClockStop(const TIgnoreClockStop &);
    TIgnoreClockStop & operator = (const TIgnoreClockStop &);

    static int count_;
    TimelineControls * tc_;
  };

  //----------------------------------------------------------------
  // TIgnoreClockStop::count_
  //
  int
  TIgnoreClockStop::count_ = 0;

  //----------------------------------------------------------------
  // MainWindow::playbackColorConverter
  //
  void
  MainWindow::playbackColorConverter()
  {
#if 0
    std::cerr << "playbackColorConverter" << std::endl;
#endif

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    std::size_t videoTrack = reader_->getSelectedVideoTrackIndex();
    selectVideoTrack(reader_, videoTrack);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::playbackLoopFilter
  //
  void
  MainWindow::playbackLoopFilter()
  {
#if 0
    std::cerr << "playbackLoopFilter" << std::endl;
#endif
    reader_->skipLoopFilter(actionSkipLoopFilter->isChecked());
  }

  //----------------------------------------------------------------
  // MainWindow::playbackNonReferenceFrames
  //
  void
  MainWindow::playbackNonReferenceFrames()
  {
#if 0
    std::cerr << "playbackNonReferenceFrames" << std::endl;
#endif
    reader_->skipNonReferenceFrames(actionSkipNonReferenceFrames->isChecked());
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
  }

  //----------------------------------------------------------------
  // MainWindow::playbackShowTimeline
  //
  void
  MainWindow::playbackShowTimeline()
  {
    SignalBlocker blockSignals(actionShowTimeline);

    QRect mainGeom = geometry();
    int ctrlHeight = timelineWidgets_->height();
    bool fullScreen = this->isFullScreen();

    if (timelineWidgets_->isVisible())
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
  // MainWindow::playbackFullScreen
  //
  void
  MainWindow::playbackFullScreen()
  {
    if (isFullScreen())
    {
      exitFullScreen();
      return;
    }

    // enter full screen rendering:
    SignalBlocker blockSignals(actionFullScreen);

    actionFullScreen->setChecked(true);
    actionShrinkWrap->setEnabled(false);
    menuBar()->hide();
    showFullScreen();

    swapShortcuts(shortcutExit_, actionExit);
    swapShortcuts(shortcutFullScreen_, actionFullScreen);
    swapShortcuts(shortcutShowPlaylist_, actionShowPlaylist);
    swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
    swapShortcuts(shortcutPlay_, actionPlay);
    swapShortcuts(shortcutNext_, actionNext);
    swapShortcuts(shortcutPrev_, actionPrev);
    swapShortcuts(shortcutLoop_, actionLoop);
    swapShortcuts(shortcutCropNone_, actionCropFrameNone);
    swapShortcuts(shortcutCrop1_33_, actionCropFrame1_33);
    swapShortcuts(shortcutCrop1_78_, actionCropFrame1_78);
    swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
  }

  //----------------------------------------------------------------
  // MainWindow::exitFullScreen
  //
  void
  MainWindow::exitFullScreen()
  {
    if (isFullScreen())
    {
      // exit full screen rendering:
      SignalBlocker blockSignals(actionFullScreen);

      actionFullScreen->setChecked(false);
      actionShrinkWrap->setEnabled(true);
      menuBar()->show();
      showNormal();
      QTimer::singleShot(100, this, SLOT(adjustCanvasHeight()));

      swapShortcuts(shortcutExit_, actionExit);
      swapShortcuts(shortcutFullScreen_, actionFullScreen);
      swapShortcuts(shortcutShowPlaylist_, actionShowPlaylist);
      swapShortcuts(shortcutShowTimeline_, actionShowTimeline);
      swapShortcuts(shortcutPlay_, actionPlay);
      swapShortcuts(shortcutNext_, actionNext);
      swapShortcuts(shortcutPrev_, actionPrev);
      swapShortcuts(shortcutLoop_, actionLoop);
      swapShortcuts(shortcutCropNone_, actionCropFrameNone);
      swapShortcuts(shortcutCrop1_33_, actionCropFrame1_33);
      swapShortcuts(shortcutCrop1_78_, actionCropFrame1_78);
      swapShortcuts(shortcutAutoCrop_, actionCropFrameAutoDetect);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::togglePlayback
  //
  void
  MainWindow::togglePlayback()
  {
#if 0
    std::cerr << "togglePlayback: "
              << !playbackPaused_ << " -> "
              << playbackPaused_
              << std::endl;
#endif

    reader_->setPlaybackInterval(playbackPaused_);

    if (playbackPaused_)
    {
      actionPlay->setText(tr("Pause"));
      prepareReaderAndRenderers(reader_);
      resumeRenderers();

      bookmarkTimer_.start();
    }
    else
    {
      actionPlay->setText(tr("Play"));
      TIgnoreClockStop ignoreClockStop(timelineControls_);
      stopRenderers();

      bookmarkTimer_.stop();
      saveBookmark();
    }

    playbackPaused_ = !playbackPaused_;
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectDevice
  //
  void
  MainWindow::audioSelectDevice(const QString & audioDevice)
  {
#if 0
    std::cerr << "audioSelectDevice: "
              << audioDevice.toUtf8().constData() << std::endl;
#endif

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    audioDevice_.assign(audioDevice.toUtf8().constData());
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::audioSelectTrack
  //
  void
  MainWindow::audioSelectTrack(int index)
  {
#if 0
    std::cerr << "audioSelectTrack: " << index << std::endl;
#endif

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectAudioTrack(reader_, index);
    reader_->getSelectedAudioTrackInfo(selAudio_);
    reader_->getAudioTraits(selAudioTraits_);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
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
    std::cerr << "videoSelectTrack: " << index << std::endl;
#endif

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectVideoTrack(reader_, index);
    reader_->getSelectedVideoTrackInfo(selVideo_);
    reader_->getVideoTraits(selVideoTraits_);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
  }

  //----------------------------------------------------------------
  // MainWindow::subsSelectTrack
  //
  void
  MainWindow::subsSelectTrack(int index)
  {
#if 0
    std::cerr << "subsSelectTrack: " << index << std::endl;
#endif

    TIgnoreClockStop ignoreClockStop(timelineControls_);
    reader_->threadStop();
    stopRenderers();

    selectSubsTrack(reader_, index);
    selSubsFormat_ = reader_->subsInfo(index, selSubs_);
    prepareReaderAndRenderers(reader_, playbackPaused_);

    double t = timelineControls_->currentTime();
    reader_->seek(t);
    reader_->threadStart();

    resumeRenderers();
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

    timelineControls_->seekTo(ch.start_);
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
      canvas_->clear();
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
  // MainWindow::helpAbout
  //
  void
  MainWindow::helpAbout()
  {
    static AboutDialog * about = NULL;
    if (!about)
    {
      about = new AboutDialog(this);
      about->setWindowTitle(tr("Apprentice Video (revision %1)").
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
      QString filename = QFileInfo(i->toLocalFile()).canonicalFilePath();
      QFileInfo fi(filename);

      if (!fi.isReadable())
      {
        continue;
      }

      if (fi.isDir() && fi.suffix() != kExtEyetv)
      {
        findFilesAndSort(playlist, filename);
      }
      else
      {
        playlist.push_back(filename);
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
    reader_->setPlaybackInterval(!seeking);
  }

  //----------------------------------------------------------------
  // MainWindow::moveTimeIn
  //
  void
  MainWindow::moveTimeIn(double seconds)
  {
    if (!reader_)
    {
      return;
    }

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
    if (!reader_)
    {
      return;
    }

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
    if (!reader_)
    {
      return;
    }

    reader_->seek(seconds);

    if (playbackPaused_)
    {
      bool forOneFrameOnly = true;
      prepareReaderAndRenderers(reader_, forOneFrameOnly);
      resumeRenderers();
    }
  }

  //----------------------------------------------------------------
  // MainWindow::populateAudioDeviceMenu
  //
  void
  MainWindow::populateAudioDeviceMenu()
  {
    menuAudioDevice->clear();
    if (!audioRenderer_)
    {
      return;
    }

    // update the UI:
    delete audioDeviceGroup_;
    audioDeviceGroup_ = new QActionGroup(this);

    delete audioDeviceMapper_;
    audioDeviceMapper_ = new QSignalMapper(this);

    bool ok = connect(audioDeviceMapper_, SIGNAL(mapped(const QString &)),
                      this, SLOT(audioSelectDevice(const QString &)));
    YAE_ASSERT(ok);

    std::string devName;
    unsigned int defaultDev = audioRenderer_->getDefaultDeviceIndex();
    audioRenderer_->getDeviceName(defaultDev, devName);

    std::size_t numDevices = audioRenderer_->countAvailableDevices();
    unsigned int deviceIndex = audioRenderer_->getDeviceIndex(audioDevice_);
    if (deviceIndex >= numDevices)
    {
      // select new default output in case original has disappeared:
      audioRenderer_->getDeviceName(defaultDev, audioDevice_);
    }

    for (std::size_t i = 0; i < numDevices; i++)
    {
      audioRenderer_->getDeviceName((unsigned int)i, devName);

      QString device = QString::fromUtf8(devName.c_str());
      QAction * deviceAction = new QAction(device, this);
      menuAudioDevice->addAction(deviceAction);
      deviceAction->setCheckable(true);
      deviceAction->setChecked(devName == audioDevice_);
      audioDeviceGroup_->addAction(deviceAction);

      ok = connect(deviceAction, SIGNAL(triggered()),
                   audioDeviceMapper_, SLOT(map()));
      YAE_ASSERT(ok);
      audioDeviceMapper_->setMapping(deviceAction, device);
    }
  }

  //----------------------------------------------------------------
  // MainWindow::focusChanged
  //
  void
  MainWindow::focusChanged(QWidget * prev, QWidget * curr)
  {
#if 0
    std::cerr << "focus changed: " << prev << " -> " << curr;
    if (curr)
    {
      std::cerr << ", " << curr->objectName().toUtf8().constData()
                << " (" << curr->metaObject()->className() << ")";
    }
    std::cerr << std::endl;
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
  MainWindow::playbackFinished()
  {
    // remove current bookmark:
    bookmarkTimer_.stop();

    std::size_t itemIndex = playlistWidget_->currentItem();
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlistWidget_->lookup(itemIndex, &group);

    if (item && group)
    {
      PlaylistGroup * nextGroup = NULL;
      std::size_t nextIndex =
        playlistWidget_->closestItem(itemIndex + 1,
                                     PlaylistWidget::kAhead,
                                     &nextGroup);

      if (group != nextGroup)
      {
        yae::removeBookmark(group->bookmarkHash_);
      }
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
    ReaderFFMPEG * reader = ReaderFFMPEG::create();
    timelineControls_->observe(SharedClock());
    timelineControls_->resetFor(reader);

    reader_->close();
    stopRenderers();

    reader_->destroy();
    reader_ = reader;

    timelineControls_->update();

    this->setWindowTitle(tr("Apprentice Video"));
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
      if (load(item->path_))
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
    std::size_t iNext = playlistWidget_->closestItem(index + 1);
    std::size_t iPrev = playlistWidget_->closestItem(index - 1,
                                                     PlaylistWidget::kBehind);

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
    if (reader_)
    {
      bool enableLooping = actionLoop->isChecked();
      reader_->setPlaybackLooping(enableLooping);
    }
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
      double t0 = timelineControls_->timelineStart();
      double dt = timelineControls_->timelineDuration();
      seconds = t0 + fmod(seconds - t0, dt);
    }

    timelineControls_->seekTo(seconds);
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

    std::size_t itemIndex = playlistWidget_->currentItem();
    PlaylistGroup * group = NULL;
    PlaylistItem * item = playlistWidget_->lookup(itemIndex, &group);
    double positionInSeconds = timelineControls_->currentTime();

    yae::saveBookmark(group->bookmarkHash_,
                      item->bookmarkHash_,
                      reader_,
                      positionInSeconds);
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
      load(item->path_, &bookmark);
    }

    fixupNextPrev();
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

#ifdef __APPLE__
      RemoteControlEvent * rc = dynamic_cast<RemoteControlEvent *>(e);
      if (rc)
      {
        rc->accept();
#if 0
        std::cerr << "remote control: " << rc->buttonId_
                  << ", down: " << rc->pressedDown_
                  << ", clicks: " << rc->clickCount_
                  << ", held down: " << rc->heldDown_
                  << std::endl;
#endif

        if (rc->buttonId_ == kRemoteControlPlayButton)
        {
          if (rc->pressedDown_ && !rc->heldDown_)
          {
            togglePlayback();
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
                canvas_->cropAutoDetectStop();
                actionCropFrameNone->trigger();
              }
              else
              {
                actionCropFrameAutoDetect->trigger();
              }
            }
            else
            {
              playbackShowTimeline();
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

            timelineControls_->seekFromCurrentTime(offset);
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
    // seek back and forth here:
    int delta = e->delta();
    double percent = floor(0.5 + fabs(double(delta)) / 120.0);
    percent = std::max<double>(1.0, percent);
    double offset = percent * ((delta < 0) ? 5.0 : -5.0);

    if (!scrollWheelTimer_.isActive())
    {
      scrollStart_ = timelineControls_->currentTime();
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
    std::size_t numAudioTracks = 0;
    std::size_t numVideoTracks = 0;
    std::size_t audioTrackIndex = 0;
    std::size_t videoTrackIndex = 0;

    if (reader_)
    {
      numAudioTracks = reader_->getNumberOfAudioTracks();
      numVideoTracks = reader_->getNumberOfVideoTracks();
      audioTrackIndex = reader_->getSelectedAudioTrackIndex();
      videoTrackIndex = reader_->getSelectedVideoTrackIndex();
    }

    int key = event->key();
    if (key == Qt::Key_Escape)
    {
      exitFullScreen();
    }
    else if (key == Qt::Key_I)
    {
      emit setInPoint();
    }
    else if (key == Qt::Key_O)
    {
      emit setOutPoint();
    }
    else if (key == Qt::Key_N &&
             playbackPaused_ &&
             videoTrackIndex < numVideoTracks)
    {
      TTime t = videoRenderer_->skipToNextFrame();
      if (audioTrackIndex < numAudioTracks)
      {
        audioRenderer_->skipToTime(t, reader_);
      }
    }
    else
    {
      event->ignore();
    }
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

      if (playlistWidget_->underMouse() &&
          playlistWidget_->countItems())
      {
        contextMenu_->addSeparator();
        contextMenu_->addAction(actionRemove_);
        contextMenu_->addAction(actionSelectAll_);
      }

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionLoop);
      contextMenu_->addAction(actionSetInPoint);
      contextMenu_->addAction(actionSetOutPoint);
      contextMenu_->addAction(actionShowTimeline);

      contextMenu_->addSeparator();
      contextMenu_->addAction(actionShrinkWrap);
      contextMenu_->addAction(actionFullScreen);
      contextMenu_->addAction(menuPlaybackSpeed->menuAction());

      if (numVideoTracks || numAudioTracks)
      {
        contextMenu_->addSeparator();

        if (numAudioTracks)
        {
          contextMenu_->addAction(menuAudio->menuAction());
        }

        if (numVideoTracks)
        {
          contextMenu_->addAction(menuVideo->menuAction());
        }

        if (numSubtitles)
        {
          contextMenu_->addAction(menuSubs->menuAction());
        }

        if (numChapters > 1)
        {
          contextMenu_->addAction(menuChapters->menuAction());
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

    QRect rectCanvas = canvas_->geometry();
    int cw = rectCanvas.width();
    int ch = rectCanvas.height();

    xexpand_ = double(cw) / double(vw);
    yexpand_ = double(ch) / double(vh);

#if 0
    std::cerr << "\ncanvas size backup: " << xexpand_ << ", " << yexpand_
              << std::endl;
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

    // apply the new window geometry:
    QRect rectClient = geometry();
    int cdx = rectWindow.width() - rectClient.width();
    int cdy = rectWindow.height() - rectClient.height();

#if 0
    std::cerr << "\ncanvas size set: " << xexpand << ", " << yexpand
              << std::endl
              << "canvas resize: " << new_w - cdx << ", " << new_h - cdy
              << std::endl
              << "canvas move to: " << new_x << ", " << new_y
              << std::endl;
#endif

    resize(new_w - cdx, new_h - cdy);
    // move(new_x, new_y);
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
    videoRenderer_->close();
    audioRenderer_->close();

    videoRenderer_->pause();
    audioRenderer_->pause();
  }

  //----------------------------------------------------------------
  // MainWindow::prepareReaderAndRenderers
  //
  void
  MainWindow::prepareReaderAndRenderers(IReader * reader, bool forOneFrameOnly)
  {
    std::size_t videoTrack = reader->getSelectedVideoTrackIndex();
    std::size_t audioTrack = reader->getSelectedAudioTrackIndex();

    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    SharedClock sharedClock;
    if (!forOneFrameOnly)
    {
      timelineControls_->observe(sharedClock);
    }

    if (!forOneFrameOnly && audioTrack < numAudioTracks)
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
    }
    else
    {
      // all tracks disabled!
      return;
    }

    // update the renderers:
    if (!forOneFrameOnly)
    {
      unsigned int audioDeviceIndex = adjustAudioTraitsOverride(reader);
      if (!audioRenderer_->open(audioDeviceIndex, reader, forOneFrameOnly))
      {
        videoRenderer_->takeThisClock(sharedClock);
        videoRenderer_->obeyThisClock(videoRenderer_->clock());
      }
    }

    videoRenderer_->open(canvas_, reader, forOneFrameOnly);

    if (!forOneFrameOnly)
    {
      timelineControls_->adjustTo(reader);

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
  MainWindow::resumeRenderers()
  {
    audioRenderer_->resume();
    videoRenderer_->resume();
  }

  //----------------------------------------------------------------
  // MainWindow::selectVideoTrack
  //
  void
  MainWindow::selectVideoTrack(IReader * reader, std::size_t videoTrackIndex)
  {
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();

    reader->selectVideoTrack(videoTrackIndex);

    VideoTraits vtts;
    if (reader->getVideoTraits(vtts))
    {
      // pixel format shortcut:
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

#if 0
      std::cerr << "yae: native format: ";
      if (ptts)
      {
        std::cerr << ptts->name_;
      }
      else
      {
        std::cerr << "unsupported" << std::endl;
      }
      std::cerr << std::endl;
#endif

#if 1
      bool unsupported = ptts == NULL;

      if (!unsupported)
      {
        unsupported = (ptts->flags_ & pixelFormat::kPaletted) != 0;
      }

      if (!unsupported)
      {
        GLint internalFormatGL;
        GLenum pixelFormatGL;
        GLenum dataTypeGL;
        GLint shouldSwapBytes;
        unsigned int supportedChannels = yae_to_opengl(vtts.pixelFormat_,
                                                       internalFormatGL,
                                                       pixelFormatGL,
                                                       dataTypeGL,
                                                       shouldSwapBytes);
        unsupported = (supportedChannels < 1 ||
                       supportedChannels != ptts->channels_ &&
                       !actionSkipColorConverter->isChecked());
      }

      if (unsupported)
      {
        vtts.pixelFormat_ = kPixelFormatGRAY8;

        if (ptts)
        {
          if ((ptts->flags_ & pixelFormat::kAlpha) &&
              (ptts->flags_ & pixelFormat::kColor))
          {
            vtts.pixelFormat_ = kPixelFormatBGRA;
          }
          else if ((ptts->flags_ & pixelFormat::kColor) ||
                   (ptts->flags_ & pixelFormat::kPaletted))
          {
            if (glewIsExtensionSupported("GL_APPLE_ycbcr_422"))
            {
              vtts.pixelFormat_ = kPixelFormatYUYV422;
            }
            else
            {
              vtts.pixelFormat_ = kPixelFormatBGR24;
            }
          }
        }

        reader->setVideoTraitsOverride(vtts);
      }
#elif 1
      vtts.pixelFormat_ = kPixelFormatYUV420P9;
      reader->setVideoTraitsOverride(vtts);
      canvas_->cropAutoDetect(this, &(MainWindow::autoCropCallback));
#endif
    }

    if (reader->getVideoTraitsOverride(vtts))
    {
      const pixelFormat::Traits * ptts =
        pixelFormat::getTraits(vtts.pixelFormat_);

      if (ptts)
      {
#if 0
        std::cerr << "yae: output format: " << ptts->name_
                  << ", par: " << vtts.pixelAspectRatio_
                  << ", " << vtts.visibleWidth_
                  << " x " << vtts.visibleHeight_;

        if (vtts.pixelAspectRatio_ != 0.0)
        {
          std::cerr << ", dar: "
                    << (double(vtts.visibleWidth_) *
                        vtts.pixelAspectRatio_ /
                        double(vtts.visibleHeight_))
                    << ", " << int(vtts.visibleWidth_ *
                                   vtts.pixelAspectRatio_ +
                                   0.5)
                    << " x " << vtts.visibleHeight_;
        }

        std::cerr << ", fps: " << vtts.frameRate_
                  << std::endl;
#endif
      }
      else
      {
        // unsupported pixel format:
        reader->selectVideoTrack(numVideoTracks);
      }
    }

    adjustMenues(reader);
  }

  //----------------------------------------------------------------
  // MainWindow::adjustMenues
  //
  void
  MainWindow::adjustMenues(IReader * reader)
  {
    std::size_t numVideoTracks = reader->getNumberOfVideoTracks();
    std::size_t numAudioTracks = reader->getNumberOfAudioTracks();
    std::size_t numSubtitles = reader->subsCount();
    std::size_t numChapters = reader->countChapters();
    std::size_t videoTrackIndex = reader->getSelectedVideoTrackIndex();

    if (!numVideoTracks)
    {
      menubar->removeAction(menuVideo->menuAction());
    }

    if (!numSubtitles)
    {
      menubar->removeAction(menuSubs->menuAction());
    }

    if (numChapters < 2 )
    {
      menubar->removeAction(menuChapters->menuAction());
    }

    if (numVideoTracks)
    {
      menubar->insertMenu(menuHelp->menuAction(), menuVideo);
    }

    if (numSubtitles)
    {
      menubar->insertMenu(menuHelp->menuAction(), menuSubs);
    }

    if (numChapters > 1)
    {
      menubar->insertMenu(menuHelp->menuAction(), menuChapters);
    }

    if (videoTrackIndex >= numVideoTracks && numAudioTracks > 0)
    {
      if (actionShowPlaylist->isEnabled())
      {
        shortcutShowPlaylist_->setEnabled(false);
        actionShowPlaylist->setEnabled(false);
        actionShrinkWrap->setEnabled(false);
        actionFullScreen->setEnabled(false);

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
    else
    {
      if (!actionShowPlaylist->isEnabled())
      {
        swapLayouts(canvasContainer_, playlistContainer_);

        if (actionShowPlaylist->isChecked())
        {
          playlistDock_->show();
        }

        shortcutShowPlaylist_->setEnabled(true);
        actionShowPlaylist->setEnabled(true);
        actionShrinkWrap->setEnabled(true);
        actionFullScreen->setEnabled(true);

        playlistWidget_->show();
        playlistWidget_->update();
        this->setFocus();
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
  unsigned int
  MainWindow::adjustAudioTraitsOverride(IReader * reader)
  {
     unsigned int numDevices = audioRenderer_->countAvailableDevices();
     unsigned int deviceIndex = audioRenderer_->getDeviceIndex(audioDevice_);
     if (deviceIndex >= numDevices)
     {
       deviceIndex = audioRenderer_->getDefaultDeviceIndex();
       audioRenderer_->getDeviceName(deviceIndex, audioDevice_);
     }

     AudioTraits native;
     if (reader->getAudioTraits(native))
     {
       AudioTraits supported;
       audioRenderer_->match(deviceIndex, native, supported);

       // FIXME: temporary for debugging on openSuSE
       // supported.channelLayout_ = kAudioStereo;
#if 0
       std::cerr << "supported: " << supported.channelLayout_ << std::endl
                 << "required:  " << native.channelLayout_ << std::endl;
#endif

       reader->setAudioTraitsOverride(supported);
     }

     return deviceIndex;
  }

  //----------------------------------------------------------------
  // MainWindow::autoCropCallback
  //
  void
  MainWindow::autoCropCallback(void * callbackContext, const TCropFrame & cf)
  {
    MainWindow * mainWindow = (MainWindow *)callbackContext;
    qApp->postEvent(mainWindow, new AutoCropEvent(cf));
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
    qApp->postEvent(mainWindow, new RemoteControlEvent(buttonId,
                                                       pressedDown,
                                                       clickCount,
                                                       heldDown));
  }
#endif
};
